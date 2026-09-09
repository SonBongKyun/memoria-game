"""Phase 1I source oracle. Executes original reward callback in isolated Godot only.
Unreal authorization ends before its first gameplay effect. No production source edits.
"""
from pathlib import Path
import argparse,json,re,shutil,subprocess
import export_malet_deal_oracle as h
g=h.g;base=h.base
DEST=base.ROOT/'docs/unreal-migration/fixtures/malet_reward'
MAP='scenes/maps/verdan_market.gd'
EXTRA=('scripts/systems/world_state.gd','scripts/systems/actor_registry.gd','scripts/systems/event_bus.gd','scripts/systems/memory_engine.gd','data/world_state/actors.json','scripts/ui/memory_shop.gd','scripts/systems/save_manager.gd','scripts/ui/achievement_manager.gd')

def bodies():
    originals,normal,deal,cleanup,npc=h.bodies()
    for name in ('_on_reward_ended','_seed_malet_memory_world_state_if_needed','_open_malet_shop','_on_shop_closed'):
        originals[name]=g.source_method(MAP,name)
    reward=g.instrument(originals['_on_reward_ended'],entry='callback:reward:enter')
    seed=g.instrument(originals['_seed_malet_memory_world_state_if_needed'],entry='world:seed:enter')
    shop=g.instrument(originals['_open_malet_shop'],{'MemoryShop.shop_closed.connect(_on_shop_closed, CONNECT_ONE_SHOT)':['GameManager.events.append("callback:shop:connect")']},'shop:request')
    closed=g.instrument(originals['_on_shop_closed'],{
        'GameManager.current_chapter = 3':['GameManager.events.append("chapter:3")'],
        'await get_tree().create_timer(1.5).timeout':['GameManager.events.append("delay:elapsed:1500")']},'callback:shop:enter')
    return originals,normal+deal+cleanup+reward+seed+shop+closed,npc

def inputs():
    return [dict(id=n,locale=l,probe=p,config=g.prior.npc_configuration()) for n,l,p in (
        ('reward_en','en','none'),('reward_ko','ko','none'),
        ('replace_during_reward','en','replace_during'),('replace_at_completion','en','replace_completion'),
        ('teardown_during_reward','en','teardown_during'),('teardown_at_completion','en','teardown_completion'),
        ('preseeded_world','en','preseeded'),('full_shop_continuation','en','close_shop'))]

RUN=r'''var route_owner: Node
var probe: String
var completion_states: Array = []
func replace_fixture() -> void:
    GameManager.story_flags.clear()
    GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100,"items":{},"recent_items":[]}
    MemoryManager.memories.clear()
    MemoryManager.burned_memories.clear()
    MemoryManager._init_starting_memories()
    WorldState.reset_to_defaults()
func on_world(event: Dictionary) -> void:
    GameManager.events.append("world:" + event.event_type + ":" + event.target_id)
func state(label: String, npc: Node) -> Dictionary:
    var s: Dictionary = snapshot("field")
    s.label = label
    s.requested = field.requested.duplicate()
    s.invocations = field.invocations.duplicate()
    s.game_state = GameManager.current_state
    s.chapter = GameManager.current_chapter
    s.items = GameManager.player_data.items.duplicate(true)
    s.recent_items = GameManager.player_data.recent_items.duplicate()
    s.world = WorldState.export_data()
    s.shop_open = MemoryShop.is_open
    s.shop_inventory = MemoryShop._shop_inventory.duplicate(true)
    s.reward_callback_connected = is_instance_valid(route_owner) and field.dialogue_ended.is_connected(route_owner._on_reward_ended)
    s.memory_state = []
    for m in MemoryManager.memories:
        s.memory_state.append({"id":m.id,"burned":m.is_burned,"faded":m.is_faded,"residue":m.is_residue,"erosion":m.erosion})
    if is_instance_valid(npc): s.talk_cache = npc._talked_keys.duplicate()
    return s
func completion_probe() -> void:
    if field.group != "malet_reward": return
    completion_states.append(state("completion_before_callback",null))
    if probe == "replace_completion":
        GameManager.events.append("fixture:replace:completion")
        replace_fixture()
    elif probe == "teardown_completion":
        GameManager.events.append("fixture:teardown:completion")
        route_owner.free()
func run() -> void:
    Engine.max_fps = 60
    await get_tree().process_frame
    await get_tree().process_frame
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    field = get_node("/root/DialogueManager")
    field.dialogue_line.connect(on_line)
    field.dialogue_choice.connect(on_choices)
    field.dialogue_ended.connect(on_end)
    field.dialogue_ended.connect(completion_probe)
    EventBus.world_event_committed.connect(on_world)
    for c in cases:
        replace_fixture()
        probe = c.probe
        completion_states.clear()
        GameManager.current_locale = c.locale
        GameManager.current_chapter = 1
        GameManager.current_state = GameManager.GameState.EXPLORATION
        GameManager.requested_map = ""
        MemoryShop.is_open = false
        MemoryShop._shop_inventory = []
        MemoryManager.burn_memory("daily_market_food")
        GameManager.set_flag("burn_reaction_heard_malet_taste_burned")
        field.is_active = false
        field.current_index = 0
        field.current_dialogue = []
        field._current_choices = []
        field.requested.clear()
        field.invocations.clear()
        var npc = load("res://npc_route.gd").new()
        for key in ["npc_name","dialogue_file","dialogue_key","repeat_dialogue_key"]: npc.set(key,c.config[key])
        npc.set_meta(c.config.meta,c.config.reaction)
        route_owner = load("res://malet_route.gd").new()
        route_owner.malet_npc = npc
        add_child(route_owner)
        route_owner._start_ch2_free_exploration_after_vn()
        if probe == "preseeded":
            GameManager.set_flag("ch2_malet_done")
            route_owner._seed_malet_memory_world_state_if_needed()
            MemoryEngine.remove_memory("npc.malet","memory.malet.bl07_request_source")
            MemoryEngine.forget_fact("npc.malet","fact.bl07.route_request_received")
        await get_tree().process_frame
        await get_tree().process_frame
        GameManager.events.clear()
        GameManager.events.append("interact:Malet")
        npc.interact()
        for i in range(9): field.advance()
        var states: Array = [state("before_accept",npc)]
        await get_tree().process_frame
        GameManager.events.append("select:field:0")
        field.select_choice(0)
        states.append(state("after_accept",npc))
        await get_tree().create_timer(0.4).timeout
        assert(field.group == "malet_deal" and field.is_active)
        for i in range(5): field.advance()
        states.append(state("deal_end",npc))
        await get_tree().create_timer(0.6).timeout
        assert(field.group == "malet_reward" and field.is_active)
        states.append(state("reward_first",npc))
        for i in range(8):
            states.append(state("reward_row_%d" % i,npc))
            if i == 3:
                if probe == "replace_during":
                    GameManager.events.append("fixture:replace:during")
                    replace_fixture()
                elif probe == "teardown_during":
                    GameManager.events.append("fixture:teardown:during")
                    route_owner.free()
            field.advance()
        states.append_array(completion_states)
        states.append(state("after_reward_callback",npc))
        assert(not field.is_active)
        if probe.begins_with("teardown"):
            assert(not GameManager.get_flag("ch2_malet_done") and not MemoryShop.is_open)
        else:
            assert(GameManager.get_flag("ch2_malet_done") and MemoryShop.is_open)
            assert(not field.dialogue_ended.is_connected(route_owner._on_reward_ended))
        if probe == "close_shop":
            MemoryShop.close_shop()
            states.append(state("shop_closed_immediate",npc))
            await get_tree().create_timer(1.65).timeout
            states.append(state("chapter_request",npc))
        output.append({"id":c.id,"states":states,"timer_observations":route_owner.timings if is_instance_valid(route_owner) else []})
        if is_instance_valid(route_owner): route_owner.free()
        npc.free()
    var timings: Array = []
    for c in output:
        timings.append({"id":c.id,"observations":c.timer_observations})
        c.erase("timer_observations")
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    FileAccess.open("res://timings.json",FileAccess.WRITE).store_string(JSON.stringify(timings))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
'''

def setup(work):
    h.setup(work)
    originals,observed,_=bodies()
    constants='\n'.join(line for line in (base.ROOT/MAP).read_text(encoding='utf-8').splitlines() if line.startswith('const ') and any(k in line for k in ('DIALOGUE_FILE','MALET_','ARREL_')))+'\n'
    (work/'malet_route.gd').write_text('extends Node\n'+constants+'var malet_npc: Node\nvar timings: Array = []\nvar delay_started: int = 0\nvar delay_frame_us: int = 0\n'+originals['_start_ch2_free_exploration_after_vn']+observed,encoding='utf-8')
    for src in EXTRA[:5]:
        dest=work/src;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((base.ROOT/src).read_bytes())
    project=(work/'project.godot').read_text(encoding='utf-8')
    project=project.replace('MemoryEngine="*res://engine.gd"','\n'.join(f'{name}="*res://scripts/systems/{file}.gd"' for name,file in [('EventBus','event_bus'),('ActorRegistry','actor_registry'),('WorldState','world_state'),('MemoryEngine','memory_engine')]))
    project=project.replace('[rendering]','MemoryShop="*res://shop.gd"\nTutorialHints="*res://hints.gd"\n[rendering]')
    (work/'project.godot').write_text(project,encoding='utf-8')
    gm=(work/'gm.gd').read_text(encoding='utf-8')
    gm=re.sub(r'func add_item\([\s\S]*?(?=\nfunc )',g.instrument(g.source_method('scripts/core/game_manager.gd','add_item'),{'inventory_changed.emit(item_id)':['events.append("item:%s:%d" % [item_id,count])']}).rstrip(),gm,count=1)
    source=(base.ROOT/'scripts/core/game_manager.gd').read_text(encoding='utf-8')
    const=source[source.index('const ITEMS:'):source.index('\n}',source.index('const ITEMS:'))+2]
    gm+='\n'+const+'\nsignal inventory_changed(item_id: String)\n'+g.source_method('scripts/core/game_manager.gd','_record_recent_item')+g.source_method('scripts/core/game_manager.gd','get_recent_items')
    speakers=source[source.index('const SPEAKER_NAMES_KO:'):source.index('\n}',source.index('const SPEAKER_NAMES_KO:'))+2]
    gm+='\n'+speakers+'\n'+g.source_method('scripts/core/game_manager.gd','localized_speaker').split('\n\n')[0]+'\n'
    (work/'gm.gd').write_text(gm,encoding='utf-8')
    # Exact shop state changes and callback emission; visual-only endpoints inert.
    shop='extends Node\nsignal shop_closed()\nvar is_open: bool = false\nvar _merchant_name: String\nvar _shop_inventory: Array[Dictionary] = []\nvar _current_mode: String\nvar shop_title = Label.new()\n'
    shop+=g.instrument(g.source_method('scripts/ui/memory_shop.gd','open_shop'),{'is_open = true':['GameManager.events.append("shop:open")']})
    shop+=g.instrument(g.source_method('scripts/ui/memory_shop.gd','close_shop'),{'is_open = false':['GameManager.events.append("shop:close")']})
    shop+=g.source_method('scripts/ui/memory_shop.gd','_loc')
    for name in ('_update_grains','_refresh_items','_show_ui','_hide_ui'):shop+='func '+name+'() -> void:\n\tpass\n'
    shop+='func _update_merchant_portrait(_name: String) -> void:\n\tpass\nfunc _exit_tree() -> void:\n\tshop_title.free()\n'
    (work/'shop.gd').write_text(shop,encoding='utf-8')
    (work/'hints.gd').write_text('extends Node\nfunc show_hint(id: String) -> void:\n\tGameManager.events.append("tutorial:"+id)\n',encoding='utf-8')
    (work/'unused.gd').write_text('extends Node\nfunc record_chapter_complete(c: int) -> void:\n\tGameManager.events.append("achievement:chapter:%d" % c)\nfunc unlock(id: String) -> void:\n\tGameManager.events.append("achievement:unlock:"+id)\n'+g.source_method('scripts/systems/save_manager.gd','autosave_on_chapter_transition')+'func autosave(reason: String) -> void:\n\tGameManager.events.append("autosave:"+reason)\n',encoding='utf-8')
    (work/'transition.gd').write_text((work/'transition.gd').read_text(encoding='utf-8').replace('    ','\t'),encoding='utf-8')
    with (work/'transition.gd').open('a',encoding='utf-8') as f:f.write('\nfunc change_scene_chapter_complete(path: String, chapter: int) -> void:\n\tGameManager.requested_map = path\n\tGameManager.events.append("chapter_transition:%d:%s" % [chapter,path])\n')

def run(args):
    base.BASE=DEST;base.inputs=inputs
    base.FIELD+=g.FIELD.replace('["malet_encounter","malet_refused","malet_taste_burned"]','["malet_encounter","malet_refused","malet_taste_burned","malet_deal","malet_reward"]')
    base.ORACLE=base.ORACLE[:base.ORACLE.index('func run()')]+RUN
    base.ORACLE=base.ORACLE.replace('    GameManager.events.append("end")','    GameManager.events.append("end")\n    GameManager.events.append("state:exploration")')
    if not args.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(args,project_setup=setup)
    report=json.loads((args.evidence_dir/'oracle.json').read_text(encoding='utf-8'));work=Path(report['commands'][-1]['command'][3])
    (args.evidence_dir/'timings.json').write_bytes(base.canonical(json.loads((work/'timings.json').read_text(encoding='utf-8'))))
    paths=[MAP,'scenes/maps/verdan_market.tscn','data/chapter2_dialogue.json','scripts/core/npc.gd','scripts/systems/dialogue_manager.gd','scripts/core/game_manager.gd','scripts/systems/memory_manager.gd',*EXTRA]
    rev=subprocess.check_output(['git','-C',str(base.ROOT),'log','-1','--format=%H','--',*paths],text=True).strip()
    originals,observed,npc=bodies();hashes={}
    for path in paths:
        data=base.source_bytes((base.ROOT/path).read_bytes());assert data==base.source_bytes(subprocess.check_output(['git','-C',str(base.ROOT),'show',rev+':'+path]));hashes[path]=base.sha(data)
    att=dict(revision=rev,sources=hashes,original_callbacks=originals,observed_callbacks=observed,first_effect='GameManager.set_flag("ch2_malet_done")',completion='DialogueManager clears active/rows/index/choices; changes to exploration; synchronously emits dialogue_ended; one-shot reward listener then runs with no timer.',ownership='State-only replacement retains source listener. Freeing owner before emission, or from earlier completion listener, cancels it. UE run replacement intentionally cancels.',isolation='Actual world owners and item grant/shop state functions; inert visual, autosave storage, achievement profile and scene-travel sinks. No source edits. Downstream source observation never authorizes UE effects.')
    (args.evidence_dir/'source_attestation.json').write_bytes(base.canonical(att))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())
