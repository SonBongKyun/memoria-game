"""Execute original close/autosave/disk backup/recovery/load orchestration in isolated Godot."""
from pathlib import Path
import argparse,json,zipfile
import export_shop_transactions_oracle as prior
base=prior.base;g=prior.g
DEST=base.ROOT/'docs/unreal-migration/fixtures/checkpoint'
SAVE='scripts/systems/save_manager.gd'
def inputs():return [{'id':x} for x in ['close_save','second_save','recovery','load','missing_scene','guards']]
RUN=r'''extends Node
func _ready() -> void:
    call_deferred("run")
func project(data: Dictionary) -> Dictionary:
    return {"scene":data.scene,"autosave":data.is_autosave,"chapter":data.game.current_chapter,"flags":data.game.story_flags,"grains":data.game.player_data.grains,"items":data.game.player_data.items,"memory":data.memory,"position":data.player_pos}
func run() -> void:
    var args=OS.get_cmdline_user_args()
    GameManager.player_data={"elia_with_party":true,"grains":28,"items":{"potion":2,"antidote":1,"firebomb":1},"recent_items":["firebomb","antidote","potion"]}
    GameManager.story_flags={"ch2_malet_done":true}
    MemoryManager.memories.clear();MemoryManager.burned_memories.clear();MemoryManager._init_starting_memories()
    MemoryManager.burn_memory("daily_market_food")
    MemoryManager.burn_memory("identity_first_sword")
    var player=Node2D.new();player.position=Vector2(500,340);player.add_to_group("player");add_child(player)
    var owner=load("res://shop_route.gd").new();add_child(owner);owner._open_malet_shop()
    MemoryShop._current_mode="buy";MemoryShop._refresh_items()
    MemoryShop._selected_item=MemoryShop.items[0];MemoryShop._execute_buy()
    SaveManager.save_completed.connect(func(slot): GameManager.events.append("save_completed:%d" % slot))
    SaveManager.autosave_completed.connect(func(): GameManager.events.append("autosave_completed"))
    SaveManager.load_completed.connect(func(slot): GameManager.events.append("load_completed:%d" % slot))
    GameManager.events.clear();MemoryShop.close_shop()
    var path=SaveManager._get_save_path(0)
    var first=SaveManager._try_parse_json(path)
    var out=[{"id":"close_save","data":project(first),"events":GameManager.events.duplicate()}]
    GameManager.player_data.grains=9
    GameManager.events.clear();SaveManager.autosave_on_chapter_transition()
    out.append({"id":"second_save","primary":project(SaveManager._try_parse_json(path)),"backup":project(SaveManager._try_parse_json(path+".bak"))})
    FileAccess.open(path,FileAccess.WRITE).store_string("{broken")
    var recovered=SaveManager._load_json_with_recovery(path,0)
    out.append({"id":"recovery","data":project(recovered),"primary_repaired":project(SaveManager._try_parse_json(path))==project(first)})
    GameManager.player_data.grains=999;GameManager.story_flags.clear();GameManager.current_chapter=1
    GameManager.events.clear()
    var loaded=SaveManager.load_game(0)
    out.append({"id":"load","ok":loaded,"chapter":GameManager.current_chapter,"grains":GameManager.player_data.grains,"flags":GameManager.story_flags.duplicate(),"position":SaveManager.loaded_player_pos,"events":GameManager.events.duplicate(),"memory":MemoryManager.export_data()})
    var bad=first.duplicate(true);bad.scene="res://missing_scene.tscn"
    FileAccess.open(path,FileAccess.WRITE).store_string(JSON.stringify(bad))
    GameManager.events.clear();GameManager.player_data.grains=777
    out.append({"id":"missing_scene","ok":SaveManager.load_game(0),"grains":GameManager.player_data.grains,"events":GameManager.events.duplicate()})
    var bytes=FileAccess.get_file_as_bytes(path)
    GameManager.current_state=GameManager.GameState.MENU;SaveManager.autosave_on_chapter_transition()
    var menu_unchanged=FileAccess.get_file_as_bytes(path)==bytes
    GameManager.current_state=GameManager.GameState.EXPLORATION;Codex.suppress_recording=true;SaveManager.autosave_on_chapter_transition()
    out.append({"id":"guards","menu_unchanged":menu_unchanged,"synthetic_unchanged":FileAccess.get_file_as_bytes(path)==bytes,"invalid_slot":SaveManager.save_game(-1)})
    owner.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(out))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % out.size());get_tree().quit(0)
'''
def setup(work):
    prior.setup(work)
    names=['autosave_on_chapter_transition','autosave','save_game','load_game','_create_backup','_try_parse_json','_load_json_with_recovery','_get_current_scene_path']
    s='''extends Node
const MAX_SLOTS=3
const AUTOSAVE_SLOT=0
signal save_completed(slot)
signal autosave_completed()
signal save_failed(reason)
signal load_completed(slot)
var loaded_player_pos={}
var _autosave_timer=0.0
var _last_save_time=0.0
func _get_save_path(slot: int) -> String:
    DirAccess.make_dir_recursive_absolute("user://checkpoint")
    return "user://checkpoint/autosave.json" if slot==0 else "user://checkpoint/save_%d.json" % slot
func _guard_save_path(path: String,_operation: String) -> bool:
    assert(path.begins_with("user://checkpoint/"))
    return true
func _show_save_indicator() -> void:
    pass
func _export_world_state_for_save() -> Dictionary:
    return WorldState.export_data()
func _restore_world_state_from_save_data(data: Dictionary) -> bool:
    return WorldState.import_data(data.world_state)
func _migrate_save_data(data: Dictionary) -> Dictionary:
    assert(data.version==SAVE_VERSION) # only fresh source saves; old-version migration not certified
    return data
'''
    s=s.replace('    ','\t')+'\n'.join(g.source_method(SAVE,n) for n in names)
    (work/'save.gd').write_text(s,encoding='utf-8')
    p=work/'gm.gd';s=p.read_text(encoding='utf-8')
    s+='\nvar ng_plus_cycle=0\nvar equipped={}\nvar upgrade_levels={}\nvar seen_endings=[]\nvar play_stats={}\n'+g.source_method('scripts/core/game_manager.gd','export_data')
    s+='\nfunc import_data(data: Dictionary) -> void:\n\tplayer_data=data.player_data.duplicate(true)\n\tstory_flags=data.story_flags.duplicate(true)\n\tcurrent_chapter=int(data.current_chapter)\n'
    p.write_text(s,encoding='utf-8')
    (work/'empty_export.gd').write_text('extends Node\nfunc export_data() -> Dictionary:\n\treturn {}\nfunc import_data(_data: Dictionary) -> void:\n\tpass\nfunc prepare_resume_from_save(_data: Dictionary) -> void:\n\tpass\n',encoding='utf-8')
    with (work/'hints.gd').open('a',encoding='utf-8') as f:f.write('\nfunc export_data() -> Dictionary:\n\treturn {}\nfunc import_data(_data: Dictionary) -> void:\n\tpass\n')
    (work/'codex_stub.gd').write_text('extends Node\nvar suppress_recording=false\n',encoding='utf-8')
    p=work/'project.godot';s=p.read_text(encoding='utf-8')
    s=s.replace('SaveManager="*res://unused.gd"','SaveManager="*res://save.gd"')
    s=s.replace('[rendering]','SceneFlow="*res://empty_export.gd"\nEliaDiary="*res://empty_export.gd"\nCodex="*res://codex_stub.gd"\n[rendering]')
    s=s.replace('run/main_scene="res://oracle.tscn"','run/main_scene="res://scenes/maps/verdan_market.tscn"')
    p.write_text(s,encoding='utf-8')
    scene=work/'scenes/maps/verdan_market.tscn';scene.parent.mkdir(parents=True);scene.write_bytes((work/'oracle.tscn').read_bytes())
def run(a):
    base.BASE=DEST;base.inputs=inputs;base.ORACLE=RUN
    if not a.check:DEST.mkdir(parents=True,exist_ok=True)
    base.run(a,project_setup=setup)
    p=a.evidence_dir/'oracle.json';report=json.loads(p.read_text(encoding='utf-8'))
    report['harness']='Original current-version save/backup/recovery/load orchestration and game/memory export. Isolated path resolver/guard, inert profile/UI/diary/hints, current-version migration identity, game import collector; not a legacy importer.'
    report['source_raw_hashes'].update({s:base.sha((base.ROOT/s).read_bytes()) for s in [SAVE,'scenes/maps/verdan_market.gd','scripts/core/game_manager.gd']})
    p.write_bytes(base.canonical(report));work=Path(report['commands'][-1]['command'][3])
    with zipfile.ZipFile(a.evidence_dir/'executed_harness.zip','w',zipfile.ZIP_DEFLATED) as z:
        for f in work.rglob('*'):
            if f.is_file() and (f.suffix in ['.gd','.tscn','.json'] or f.name=='project.godot') and '.godot' not in f.parts:z.write(f,f.relative_to(work))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--godot',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True);p.add_argument('--check',action='store_true');run(p.parse_args())

