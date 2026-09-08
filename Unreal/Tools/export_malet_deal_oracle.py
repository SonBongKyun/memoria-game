"""Phase 1H: exact source Accept, deal and both callbacks; reward request only."""
from pathlib import Path
import argparse,json,subprocess
import export_malet_refusal_oracle as g
base=g.base
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_deal'

def bodies():
    originals,normal,cleanup,npc=g.bodies()
    original=g.source_method('scenes/maps/verdan_market.gd','_on_deal_ended')
    originals['_on_deal_ended']=original
    normal=normal.replace('timings.append((Time.get_ticks_usec()-delay_started)/1000.0)','timings.append({"delay_us":300000,"elapsed_us":Time.get_ticks_usec()-delay_started,"start_frame_us":delay_frame_us})').replace('print("MEMORIA_CALLBACK_DELAY_MS=",timings.back())','print("MEMORIA_CALLBACK_DELAY_US=",timings.back())').replace('assert(timings.back() >= 280.0)','assert(timings.back().elapsed_us + timings.back().start_frame_us >= 299000)')
    deal=g.instrument(original,{
        'await get_tree().create_timer(0.5).timeout':['timings.append({"delay_us":500000,"elapsed_us":Time.get_ticks_usec()-delay_started,"start_frame_us":delay_frame_us})','assert(timings.back().elapsed_us + timings.back().start_frame_us >= 499000)','GameManager.events.append("delay:elapsed:500")'],
        'DialogueManager.dialogue_ended.connect(_on_reward_ended, CONNECT_ONE_SHOT)':['GameManager.events.append("callback:reward:connect")']},'callback:deal:enter')
    line='\tGameManager.events.append("callback:deal:enter")'+g.MARK
    deal=deal.replace(line,line+'\n\tGameManager.events.append("delay:scheduled:500")'+g.MARK+'\n\tdelay_started = Time.get_ticks_usec()'+g.MARK)
    # The timer is created within an in-progress engine frame. Record that
    # frame's delta so a long frame cannot masquerade as an early callback.
    def stamp_frame(body):
        lines=[]
        for line in body.splitlines():
            lines.append(line)
            if line.lstrip().startswith('delay_started = Time.get_ticks_usec()'):
                indent=line[:len(line)-len(line.lstrip())]
                lines.append(indent+'delay_frame_us = int(ceil(get_process_delta_time()*1000000.0))'+g.MARK)
        return '\n'.join(lines)+'\n'
    normal=stamp_frame(normal)
    deal=stamp_frame(deal)
    for observed,name in [(normal,'_on_any_dialogue_ended'),(deal,'_on_deal_ended')]:
        assert '\n'.join(x for x in observed.splitlines() if not x.endswith(g.MARK))+'\n'==originals[name]
    return originals,normal,deal,cleanup,npc

def inputs():
    return [dict(id=name,locale=locale,edge=edge,boundary=boundary,config=g.prior.npc_configuration())
      for name,locale,edge,boundary in [
       ('accept_intact','en','intact',0),('accept_ko','ko','intact',0),
       ('already_burned','en','burned',0),('unrelated_state','en','unrelated',0),
       ('faded_sword','en','faded',0),
       ('replace300_keep_world','en','replace',300),('replace500_keep_world','en','replace',500),
       ('teardown300','en','teardown',300),('teardown500','en','teardown',500)]]

RUN=r"""func state(label: String, npc: Node, route: Node) -> Dictionary:
    var s: Dictionary = snapshot("field")
    s.label = label
    s.talk_cache = npc._talked_keys.duplicate()
    s.requested = field.requested.duplicate()
    s.invocations = field.invocations.duplicate()
    s.sword_intact = MemoryManager.is_intact("identity_first_sword")
    s.game_state = GameManager.current_state
    s.memory_state = []
    for m in MemoryManager.memories:
        s.memory_state.append({"id":m.id,"burned":m.is_burned,"faded":m.is_faded,"residue":m.is_residue,"erosion":m.erosion})
    return s
func replace_fixture() -> void:
    # Explicit isolated state-replacement probe, not production New Game.
    GameManager.story_flags.clear()
    MemoryManager.memories.clear()
    MemoryManager.burned_memories.clear()
    MemoryManager._init_starting_memories()
func run() -> void:
    Engine.max_fps = 60
    await get_tree().process_frame
    await get_tree().process_frame
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    var timings: Array = []
    field = get_node("/root/DialogueManager")
    field.dialogue_line.connect(on_line)
    field.dialogue_choice.connect(on_choices)
    field.dialogue_ended.connect(on_end)
    for c in cases:
        replace_fixture()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100}
        GameManager.current_locale = c.locale
        GameManager.current_state = GameManager.GameState.EXPLORATION
        GameManager.requested_map = ""
        MemoryManager.burn_memory("daily_market_food")
        GameManager.set_flag("burn_reaction_heard_malet_taste_burned")
        if c.edge == "burned": MemoryManager.burn_memory("identity_first_sword")
        if c.edge == "faded":
            for m in MemoryManager.memories:
                if m.id == "identity_first_sword": m.is_faded = true
        if c.edge == "unrelated":
            GameManager.set_flag("unrelated_fixture")
            GameManager.player_data.hp = 73
            GameManager.player_data.grains = 41
            for m in MemoryManager.memories:
                if m.id == "sense_forest_smell": m.erosion = 2
        field.is_active = false
        field.current_index = 0
        field.current_dialogue = []
        field._current_choices = []
        field.requested.clear()
        field.invocations.clear()
        var npc = load("res://npc_route.gd").new()
        for key in ["npc_name","dialogue_file","dialogue_key","repeat_dialogue_key"]: npc.set(key,c.config[key])
        npc.set_meta(c.config.meta,c.config.reaction)
        var route = load("res://malet_route.gd").new()
        route.malet_npc = npc
        add_child(route)
        route._start_ch2_free_exploration_after_vn()
        await get_tree().process_frame
        await get_tree().process_frame
        GameManager.events.clear()
        GameManager.events.append("interact:Malet")
        npc.interact()
        for i in range(9): field.advance()
        var states: Array = [state("before_accept",npc,route)]
        await get_tree().process_frame
        GameManager.events.append("select:field:0")
        field.select_choice(0)
        states.append(state("after_accept",npc,route))
        if c.boundary == 300:
            GameManager.events.append("fixture:replace")
            replace_fixture()
            if c.edge == "teardown":
                GameManager.events.append("fixture:world_teardown")
                route.free()
            await get_tree().create_timer(0.4).timeout
            states.append(state("replacement_observed",npc,null))
        else:
            await get_tree().create_timer(0.4).timeout
            assert(field.group == "malet_deal" and field.is_active)
            states.append(state("deal_first",npc,route))
            for i in range(5):
                states.append(state("deal_row_%d" % i,npc,route))
                field.advance()
            states.append(state("deal_end",npc,route))
            if c.boundary == 500:
                GameManager.events.append("fixture:replace")
                replace_fixture()
                if c.edge == "teardown":
                    GameManager.events.append("fixture:world_teardown")
                    route.free()
            await get_tree().create_timer(0.6).timeout
            states.append(state("reward_boundary",npc,null))
        if is_instance_valid(route):
            timings.append({"id":c.id,"observations":route.timings})
            route.free()
        else:
            timings.append({"id":c.id,"observations":[],"owner_freed":true})
        npc.free()
        output.append({"id":c.id,"states":states})
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    FileAccess.open("res://timings.json",FileAccess.WRITE).store_string(JSON.stringify(timings))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
"""

def setup(work):
    g.setup(work)
    originals,normal,deal,cleanup,npc=bodies()
    prefix='extends Node\nconst DIALOGUE_FILE = "res://data/chapter2_dialogue.json"\nvar malet_npc: Node\nvar timings: Array = []\nvar delay_started: int = 0\nvar delay_frame_us: int = 0\n'
    prefix+='func _on_reward_ended() -> void:\n\tassert(false,"Reward completion is outside Phase 1H")\n'
    (work/'malet_route.gd').write_text(prefix+originals['_start_ch2_free_exploration_after_vn']+normal+deal+cleanup,encoding='utf-8')

def run(args):
    base.BASE=DEST;base.inputs=inputs
    base.FIELD+=g.FIELD.replace('["malet_encounter","malet_refused","malet_taste_burned"]','["malet_encounter","malet_refused","malet_taste_burned","malet_deal"]')
    base.ORACLE=base.ORACLE[:base.ORACLE.index('func run()')]+RUN
    base.ORACLE=base.ORACLE.replace('    GameManager.events.append("end")','    GameManager.events.append("end")\n    GameManager.events.append("state:exploration")')
    if not args.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(args,project_setup=setup)
    report=json.loads((args.evidence_dir/'oracle.json').read_text(encoding='utf-8'))
    work=Path(report['commands'][-1]['command'][3])
    (args.evidence_dir/'timings.json').write_bytes(base.canonical(json.loads((work/'timings.json').read_text(encoding='utf-8'))))
    originals,normal,deal,cleanup,npc=bodies()
    paths=['scripts/core/npc.gd','scripts/systems/perception_filter.gd','scripts/systems/dialogue_manager.gd','scripts/core/game_manager.gd','scripts/systems/memory_manager.gd','scenes/maps/verdan_market.gd','scenes/maps/verdan_market.tscn','data/chapter2_dialogue.json']
    rev=subprocess.check_output(['git','-C',str(base.ROOT),'log','-1','--format=%H','--',*paths],text=True).strip()
    hashes={}
    for path in paths:
        data=base.source_bytes((base.ROOT/path).read_bytes())
        assert data==base.source_bytes(subprocess.check_output(['git','-C',str(base.ROOT),'show',rev+':'+path]))
        hashes[path]=base.sha(data)
    evidence=dict(revision=rev,sources=hashes,original_callbacks=originals,observed_callbacks=normal+deal+cleanup,observed_npc=npc,
        observer_contract='Remove tagged observer lines to recover exact callback bodies, including both actual timers. Telemetry is integer microseconds.',
        timer_tolerance='elapsed_us + start_frame_us >= nominal_us - 1000; actual source timers unchanged; frame duration and wall time both recorded.',
        replacement_probe='Explicit fixture state replacement retaining owner versus freeing route owner; no production New Game implementation.',
        hard_boundary='malet_reward request recorded and deferred before loading/starting its rows or connecting executable reward completion.')
    (args.evidence_dir/'source_attestation.json').write_bytes(base.canonical(evidence))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true')
    run(p.parse_args())
