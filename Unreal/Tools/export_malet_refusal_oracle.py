"""Phase1G: real NPC, DialogueManager and instrumented source callback bodies.

Observer insertions retain every original callback line, including create_timer.
Accept observation ends at the requested malet_deal boundary, without importing
or executing its downstream dialogue. Wall timing lives outside golden output.
"""
from pathlib import Path
import argparse,re,json,subprocess
import export_narrative_oracle as base
import export_malet_oracle as prior

DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_refusal'
GROUPS=('malet_encounter','malet_refused')
MARK=' # phase1g observer'

def source_method(path,name):
    s=(base.ROOT/path).read_text(encoding='utf-8')
    match=re.search(r'(?ms)^func '+re.escape(name)+r'\(.*?(?=^func |\Z)',s)
    if not match: raise ValueError('Missing source method '+name)
    return match.group(0).rstrip()+'\n'

def instrument(original,after=None,entry=None):
    result=[]
    for line in original.splitlines():
        result.append(line)
        if line.startswith('func ') and entry:
            result.append('\tGameManager.events.append("'+entry+'")'+MARK)
        for needle,extra in (after or {}).items():
            if line.strip()==needle:
                indent=line[:len(line)-len(line.lstrip())]
                result.extend(indent+x+MARK for x in extra)
    observed='\n'.join(result)+'\n'
    assert '\n'.join(x for x in observed.splitlines() if not x.endswith(MARK))+'\n'==original
    return observed

def bodies():
    path='scenes/maps/verdan_market.gd'
    originals={n:source_method(path,n) for n in ('_on_any_dialogue_ended','_on_refused_ended','_start_ch2_free_exploration_after_vn')}
    callbacks=instrument(originals['_on_any_dialogue_ended'],{
      'DialogueManager.dialogue_ended.disconnect(_on_any_dialogue_ended)':['GameManager.events.append("callback:normal:disconnect")','GameManager.events.append("delay:scheduled:300")','delay_started = Time.get_ticks_usec()'],
      'await get_tree().create_timer(0.3).timeout':['timings.append((Time.get_ticks_usec()-delay_started)/1000.0)','print("MEMORIA_CALLBACK_DELAY_MS=",timings.back())','assert(timings.back() >= 280.0)','GameManager.events.append("delay:elapsed:300")'],
      'DialogueManager.dialogue_ended.connect(_on_deal_ended, CONNECT_ONE_SHOT)':['GameManager.events.append("callback:deal:connect")'],
      'DialogueManager.dialogue_ended.connect(_on_refused_ended, CONNECT_ONE_SHOT)':['GameManager.events.append("callback:refused:connect")']})
    cleanup=instrument(originals['_on_refused_ended'],{
      'GameManager.story_flags.erase("malet_deal_refused")':['GameManager.events.append("erase:flag:malet_deal_refused")'],
      'GameManager.story_flags.erase("talked_Malet_malet_encounter")':['GameManager.events.append("erase:flag:talked_Malet_malet_encounter")'],
      'malet_npc._talked_keys.erase("malet_encounter")':['GameManager.events.append("erase:cache:malet_encounter")'],
      'DialogueManager.dialogue_ended.connect(_on_any_dialogue_ended)':['GameManager.events.append("callback:normal:connect")']},'callback:refused:enter')
    npc=instrument(prior.npc_methods().rstrip()+'\n',{
      '_talked_keys[dialogue_key] = true':['GameManager.events.append("cache:set:" + dialogue_key)'],
      'DialogueManager.dialogue_ended.connect(_on_first_talk_ended.bind(talk_flag), CONNECT_ONE_SHOT)':['GameManager.events.append("callback:npc:connect")']})
    # First-talk observer is inserted at the exact callback entry, not a replacement.
    npc=npc.replace('func _on_first_talk_ended(talk_flag: String) -> void:\n',
        'func _on_first_talk_ended(talk_flag: String) -> void:\n\tGameManager.events.append("callback:npc:first_talk")'+MARK+'\n')
    assert '\n'.join(x for x in npc.splitlines() if not x.endswith(MARK))+'\n'==prior.npc_methods().rstrip()+'\n'
    return originals,callbacks,cleanup,npc

def inputs():
    return [dict(id=name,choice=choice,locale=locale,heard=heard,talked=talked,cached=cached,config=prior.npc_configuration())
      for name,choice,locale,heard,talked,cached in [
       ('heard_first',-1,'en',True,False,False),
       ('refusal_retry',1,'en',True,False,False),
       ('refusal_ko',1,'ko',True,False,False),
       ('accept_source',0,'en',True,False,False),
       ('persisted_repeat',-1,'en',True,True,False),
       ('cached_repeat',-1,'en',True,False,True),
       ('unheard_priority',-1,'en',False,True,True)]]

FIELD=r"""
var requested: Array = []
var group: String = ""
var invocations: Array = []
func load_and_start(file_path: String, dialogue_key: String) -> void:
    requested.append(dialogue_key)
    GameManager.events.append("request:" + file_path + "::" + dialogue_key)
    if dialogue_key not in ["malet_encounter","malet_refused","malet_taste_burned"]:
        GameManager.events.append("development:deferred:" + dialogue_key)
        return
    group = dialogue_key
    invocations.append(group)
    GameManager.events.append("field:start:" + group)
    super.load_and_start(file_path, dialogue_key)
"""
RUN=r"""func state(label: String, npc: Node, route: Node) -> Dictionary:
    var s: Dictionary = snapshot("field")
    s.label = label
    s.talk_cache = npc._talked_keys.duplicate()
    s.requested = field.requested.duplicate()
    s.invocations = field.invocations.duplicate()
    s.normal_callback = field.dialogue_ended.is_connected(route._on_any_dialogue_ended)
    s.sword_intact = MemoryManager.is_intact("identity_first_sword")
    return s
func run() -> void:
    Engine.max_fps = 60
    await get_tree().process_frame
    await get_tree().process_frame
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    var wall_timings: Array = []
    field = get_node("/root/DialogueManager")
    field.dialogue_line.connect(on_line)
    field.dialogue_choice.connect(on_choices)
    field.dialogue_ended.connect(on_end)
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100}
        GameManager.current_locale = c.locale
        GameManager.current_state = GameManager.GameState.EXPLORATION
        GameManager.requested_map = ""
        MemoryManager.memories.clear()
        MemoryManager.burned_memories.clear()
        MemoryManager._init_starting_memories()
        MemoryManager.burn_memory("daily_market_food")
        if c.heard: GameManager.set_flag("burn_reaction_heard_malet_taste_burned")
        if c.talked: GameManager.set_flag("talked_Malet_malet_encounter")
        field.is_active = false
        field.current_index = 0
        field.current_dialogue = []
        field._current_choices = []
        field.requested.clear()
        field.invocations.clear()
        var npc = load("res://npc_route.gd").new()
        for key in ["npc_name","dialogue_file","dialogue_key","repeat_dialogue_key"]: npc.set(key,c.config[key])
        npc.set_meta(c.config.meta,c.config.reaction)
        if c.cached: npc._talked_keys["malet_encounter"] = true
        var route = load("res://malet_route.gd").new()
        route.malet_npc = npc
        add_child(route)
        route._start_ch2_free_exploration_after_vn()
        await get_tree().process_frame
        await get_tree().process_frame
        GameManager.events.clear()
        GameManager.events.append("interact:Malet")
        npc.interact()
        var states: Array = [state("first_request",npc,route)]
        if field.group == "malet_encounter" and field.is_active:
            for i in range(9): field.advance()
            states.append(state("choices",npc,route))
            if c.choice >= 0:
                await get_tree().process_frame
                GameManager.events.append("select:field:%d" % c.choice)
                field.select_choice(c.choice)
                states.append(state("choice_return",npc,route))
                await get_tree().create_timer(0.4).timeout
                states.append(state("callback_request",npc,route))
                if c.choice == 1:
                    assert(field.group == "malet_refused" and field.is_active)
                    for i in range(3): field.advance()
                    assert(GameManager.current_state == GameManager.GameState.EXPLORATION)
                    GameManager.events.append("exploration:ready")
                    states.append(state("cleanup",npc,route))
                    GameManager.events.append("interact:Malet")
                    npc.interact()
                    states.append(state("retry",npc,route))
                else:
                    assert(field.requested.back() == "malet_deal")
                    assert(not field.is_active)
        elif not c.heard:
            assert(field.group == "malet_taste_burned" and field.is_active)
            for i in range(3): field.advance()
            GameManager.events.append("exploration:ready")
            states.append(state("reaction_complete",npc,route))
        output.append({"id":c.id,"states":states})
        wall_timings.append({"id":c.id,"actual_delay_ms":route.timings})
        route.free()
        npc.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    FileAccess.open("res://timings.json",FileAccess.WRITE).store_string(JSON.stringify(wall_timings))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
"""

def setup(work):
    prior.setup(work)
    _,callbacks,cleanup,npc=bodies()
    p=work/'npc_route.gd'
    s=p.read_text(encoding='utf-8')
    assert prior.npc_methods() in s
    p.write_text(s.replace(prior.npc_methods(),npc),encoding='utf-8')
    # The normal route callbacks themselves are extracted from the source.
    prefix='extends Node\nconst DIALOGUE_FILE = "res://data/chapter2_dialogue.json"\nvar malet_npc: Node\nvar timings: Array = []\nvar delay_started: int = 0\n'
    prefix+='func _on_deal_ended() -> void:\n\tassert(false,"Downstream completion is beyond the oracle request boundary")\n'
    (work/'malet_route.gd').write_text(prefix+bodies()[0]['_start_ch2_free_exploration_after_vn']+callbacks+cleanup,encoding='utf-8')

def run(args):
    originals,callbacks,cleanup,npc=bodies()
    base.BASE=DEST; base.inputs=inputs
    base.FIELD+=FIELD
    base.ORACLE=base.ORACLE[:base.ORACLE.index('func run()')]+RUN
    base.ORACLE=base.ORACLE.replace('    GameManager.events.append("end")', '    GameManager.events.append("end")\n    if field.group in ["malet_encounter","malet_refused"]:\n        GameManager.events.append("state:exploration")')
    if not args.check: DEST.mkdir(parents=True,exist_ok=True)
    base.run(args,project_setup=setup)
    report=json.loads((args.evidence_dir/'oracle.json').read_text(encoding='utf-8'))
    work=Path(report['commands'][-1]['command'][3])
    timings=json.loads((work/'timings.json').read_text(encoding='utf-8'))
    (args.evidence_dir/'timings.json').write_bytes(base.canonical(timings))
    paths=['scripts/core/npc.gd','scripts/systems/perception_filter.gd','scripts/systems/dialogue_manager.gd','scripts/core/game_manager.gd','scripts/systems/memory_manager.gd','scenes/maps/verdan_market.gd','scenes/maps/verdan_market.tscn','data/chapter2_dialogue.json']
    rev=subprocess.check_output(['git','-C',str(base.ROOT),'log','-1','--format=%H','--',*paths],text=True).strip()
    hashes={}
    for path in paths:
        data=base.source_bytes((base.ROOT/path).read_bytes())
        assert data==base.source_bytes(subprocess.check_output(['git','-C',str(base.ROOT),'show',rev+':'+path]))
        hashes[path]=base.sha(data)
    evidence=dict(revision=rev,sources=hashes,original_callbacks=originals,observed_callbacks=callbacks+cleanup,observed_npc=npc,
        observer_contract='Removing observer-tagged lines recovers exact source bodies; real create_timer(0.3) remains. Actual elapsed timing is outside deterministic golden output.',
        accept_boundary='Complete original choice effects and normal callback until malet_deal request; no downstream group executes.')
    (args.evidence_dir/'source_attestation.json').write_bytes(base.canonical(evidence))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true')
    run(p.parse_args())
