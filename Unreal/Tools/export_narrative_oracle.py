"""Source-authentic bounded oracle, inert presentation, isolated app-data.

Actual DialogueManager, SceneFlow, MemoryManager and JourneyOath are copied
byte-for-byte. Observer subclasses call super; they do not replace rules.
The VN renderer's complete filtering prefix is extracted verbatim, ending at
Button construction; only button creation is replaced with index collection.
"""
from pathlib import Path
import argparse, datetime, os, shutil, subprocess, re
from narrative_ir import ROOT, BASE, CASES, canonical, sha, source_bytes, parse_source

def inputs():
    field=parse_source((ROOT/CASES['field'][0]).read_bytes())['dialogues']['verdan_arrival']
    vn=parse_source((ROOT/CASES['vn'][0]).read_bytes())['steps']
    def case(id,d,rows,commands,**kw):
        return dict(id=id,dialect=d,synthetic=id.startswith('synthetic_'),rows=rows,commands=commands,locale=kw.get('locale','en'),burn_before=kw.get('burn_before',[]))
    result=[case('source_field_'+l,'field',field,[{'advance':True}]*5,locale=l) for l in ('en','ko')]
    for j in range(3): result.append(case('source_vn_choice_'+str(j),'vn',vn,[{'advance':True}]*10+[{'select':j},{'advance':True}],locale='ko' if j==2 else 'en'))
    result.append(case('source_vn_failed_cost','vn',vn,[{'advance':True}]*10+[{'select':1}],burn_before=['daily_market_food']))
    result.append(case('source_vn_filtered_original','vn',vn,[{'advance':True}]*10+[{'select':2}],burn_before=['daily_market_food']))
    # Deliberately not campaign content. Keys use exact observed source branches.
    f=[{'set_flag':'skipped_effect','requires_flag':'missing','text':'skip'},
       {'choices':[{'text':'hidden','requires_flag':'missing'}, {'text':'first'},
                   {'text':'pay missing','set_flag':'field_before_cost','cost_memory':'missing','add_grains':7,'jump_to':3}]},
       {'text':'jump skipped'}, {'text':'destination'}, {'choices':[{'text':'zero','jump_to':0}]}]
    result.append(case('synthetic_field_order_cost_jump','field',f,[{'select':1},{'advance':True},{'select':0}]))
    v=[{'set_flag':'before_gate','requires_flag':'missing','add_grains':100,'text':'skip'},
       {'choice':[{'text':'pay missing','cost_memory':'missing','set_flag':'must_not_set','add_grains':7,'goto':3},
                  {'text':'hidden','requires_flag':'missing'},
                  {'text':'explicit burn','burn_memory':'missing','set_flag':'before_explicit_burn','add_grains':9,'goto':3}]},
       {'text':'jump skipped'}, {'text':'destination'}, {'choice':[{'text':'zero','goto':0}]}]
    result.append(case('synthetic_vn_order_cost_jump','vn',v,[{'select':0},{'select':2},{'advance':True},{'select':0}]))
    # Selected asset continuation: suspended map step stays current and inactive.
    result.append(case('source_vn_resume','vn',vn,[{'advance':True}]*10+[{'resume':True},{'select':0},{'advance':True},{'consume':True},{'consume':True}]))
    return result

GM='''extends Node
enum GameState { EXPLORATION, DIALOGUE, BATTLE, CUTSCENE, MENU, PAUSED }
var current_state: int = 0
var current_chapter: int = 1
var current_locale: String = "en"
var player_data: Dictionary = {"elia_with_party": true, "grains": 0, "hp": 50, "max_hp": 100}
var story_flags: Dictionary = {}
var events: Array = []
var endings: Array = []
var requested_map: String = ""
func change_state(s: int) -> void:
    current_state = s
func add_stat(_key: String, _amount: int = 1) -> void:
    if _key == "total_grains_earned": events.append("stat:%s:%d" % [_key, _amount])
func get_flag(k: String) -> bool:
    return bool(story_flags.get(k, false))
func set_flag(k: String, v: bool = true) -> void:
    story_flags[k] = v
    events.append("flag:" + k)
func record_ending(k: String) -> void:
    endings.append(k)
    events.append("ending:" + k)
func add_item(k: String, n: int = 1) -> void:
    events.append("item:%s:%d" % [k,n])
func evaluate_part3_ending() -> String:
    return ""
'''
FIELD='''extends "res://dialogue_source.gd"
func _show_next_line() -> void:
    GameManager.events.append("visit:%d" % current_index)
    super._show_next_line()
func _line_condition_met(line: Dictionary) -> bool:
    var result: bool = super._line_condition_met(line)
    GameManager.events.append("gate:%d:%s" % [current_index, "pass" if result else "skip"])
    return result
func _apply_line_effects(line: Dictionary) -> void:
    GameManager.events.append("effects:%d" % current_index)
    super._apply_line_effects(line)
'''
VN='''extends "res://scene_flow_source.gd"
func _run_step() -> void:
    GameManager.events.append("visit:%d" % current_index)
    super._run_step()
func _ensure_vn_ui() -> void:
    pass
func _close_vn_ui() -> void:
    pass
'''
MEMORY='''extends "res://memory_source.gd"
func burn_memory(id: String, allow_faded: bool = false) -> Memory:
    var result: Memory = super.burn_memory(id, allow_faded)
    GameManager.events.append("burn:%s:%s" % [id,"ok" if result != null else "fail"])
    return result
'''
ORACLE='''extends Node
var flow: Node
var field: Node
var filter: RefCounted
func _ready() -> void:
    call_deferred("run")
func snapshot(d: String) -> Dictionary:
    var state: Dictionary = {"events":GameManager.events.duplicate(),"flags":GameManager.story_flags.duplicate(),"grains":GameManager.player_data.grains,"hp":GameManager.player_data.hp,"map":GameManager.requested_map,"burned":[],"active":false,"index":0}
    for m in MemoryManager.burned_memories: state.burned.append(m.id)
    if d == "vn":
        state.active = flow.is_active
        state.index = flow.current_index
        state.continuation = flow.export_data()
    else:
        state.active = field.is_active
        state.index = field.current_index
    return state
func on_line(_speaker: String, text: String, _portrait: String) -> void:
    GameManager.events.append("line:%d:%s" % [field.current_index,text])
func on_choices(_choices: Array) -> void:
    var ids: PackedStringArray = []
    for c in field._current_choices: ids.append(str(field.current_dialogue[field.current_index].choices.find(c)))
    GameManager.events.append("choices:" + ",".join(ids))
func on_step(step: Dictionary) -> void:
    GameManager.events.append("step:%d" % flow.current_index)
    if step.has("choice"):
        var ids: PackedStringArray = []
        for i in filter.exposed(step.choice): ids.append(str(i))
        GameManager.events.append("choices:" + ",".join(ids))
func on_end(_id: String = "") -> void:
    GameManager.events.append("end")
func run() -> void:
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    filter = load("res://filter.gd").new()
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":50,"max_hp":100}
        GameManager.current_locale = c.locale
        GameManager.requested_map = ""
        MemoryManager.memories.clear()
        MemoryManager.burned_memories.clear()
        MemoryManager._init_starting_memories()
        for mid in c.burn_before: MemoryManager.burn_memory(mid)
        GameManager.events.clear()
        if c.dialect == "field":
            field = load("res://field.gd").new()
            field.dialogue_line.connect(on_line)
            field.dialogue_choice.connect(on_choices)
            field.dialogue_ended.connect(on_end)
            field.start_dialogue(c.rows)
        else:
            flow = load("res://vn.gd").new()
            add_child(flow)
            flow.step_changed.connect(on_step)
            flow.scene_ended.connect(on_end)
            flow._cache["ch2_market_arrival"] = {"steps":c.rows}
            flow.play("ch2_market_arrival")
        var states: Array = [snapshot(c.dialect)]
        for command in c.commands:
            var target: Node = flow if c.dialect == "vn" else field
            if command.has("advance"): target.advance()
            if command.has("select"): target.select_choice(int(command.select))
            if command.has("resume"):
                var save: Dictionary = flow.export_data()
                save.pending_scene_id = "ch2_market_arrival"
                save.pending_start_index = 1
                save.resume_queue = [{"scene_id":"ch2_market_arrival","index":3},{"scene_id":"ch2_market_arrival","index":4}]
                flow.prepare_resume_from_save(save)
                states.append(snapshot(c.dialect))
                var sid: String = flow.pending_scene_id
                var idx: int = flow.pending_start_index
                flow.pending_scene_id = ""
                flow.pending_start_index = 0
                flow.play(sid,idx)
            if command.has("consume"): flow.resume_if_queued()
            states.append(snapshot(c.dialect))
        output.append({"id":c.id,"states":states})
        if c.dialect == "vn": flow.free()
        else: field.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
'''

def run(args, project_setup=None):
    ev=args.evidence_dir.resolve()
    if ev.exists(): raise ValueError('Use fresh evidence path')
    ev.mkdir(parents=True)
    work=ROOT/'Unreal/Memoria/Saved/Validation'/('narrative-oracle-'+datetime.datetime.now().strftime('%Y%m%dT%H%M%S%f'))
    work.mkdir(parents=True)
    hashes={}
    for src,target in [('scripts/systems/dialogue_manager.gd','dialogue_source.gd'),('scripts/systems/scene_flow.gd','scene_flow_source.gd'),('scripts/systems/memory_manager.gd','memory_source.gd'),('scripts/utils/journey_oath.gd','journey_oath.gd')]:
        data=(ROOT/src).read_bytes(); (work/target).write_bytes(data); hashes[src]=sha(data)
    gm=(ROOT/'scripts/core/game_manager.gd').read_text(encoding='utf-8')
    localization=gm[gm.index('func localized_value('):gm.index('\nsignal state_changed',gm.index('func localized_value('))]
    ui=(ROOT/'scripts/ui/vn_scene.gd').read_text(encoding='utf-8')
    start=ui.index('\tfor i in range(choices.size()):',ui.index('func _show_choices'))
    end=ui.index('\t\tvar btn = Button.new()',start)
    filtering=ui[start:end]
    stubs={'gm.gd':GM.replace('    ','\t')+'\n'+localization,'field.gd':FIELD,'vn.gd':VN,'memory.gd':MEMORY,'oracle.gd':ORACLE,
      'filter.gd':'extends RefCounted\nfunc exposed(choices: Array) -> Array:\n\tvar result: Array = []\n'+filtering+'\t\tresult.append(i)\n\treturn result\n',
      'story.gd':'extends Node\nfunc record_choice(t: String) -> void:\n    GameManager.events.append("choice:" + t)\n',
      'transition.gd':'extends Node\nfunc change_scene_styled(p: String) -> void:\n    GameManager.requested_map = p\n    GameManager.events.append("map:" + p)\n',
      'condition.gd':'class_name DialogueConditionSystem\nstatic func evaluate(_c: Dictionary, _ctx: Dictionary) -> bool:\n    assert(false, "Structured world conditions are outside bounded fixtures")\n    return false\n',
      'engine.gd':'extends Node\nfunc remove_memory(_a: String, _m: String) -> void:\n    assert(false)\nfunc restore_memory(_a: String, _m: String) -> void:\n    assert(false)\n',
      'unused.gd':'extends Node\nfunc record_chapter_complete(_c: int) -> void:\n    assert(false)\nfunc autosave_on_chapter_transition() -> void:\n    assert(false)\n'}
    for p in ('notification_stub.gd','system_log_stub.gd','audio_manager_stub.gd'): shutil.copyfile(ROOT/'Unreal/Tools/GodotOracle'/p,work/p)
    with (work/'audio_manager_stub.gd').open('a',encoding='utf-8') as f: f.write('\nfunc play_bgm(_path: String) -> void:\n\tpass\n')
    for p,s in stubs.items(): (work/p).write_text(s,encoding='utf-8',newline='\n')
    (work/'project.godot').write_text('''config_version=5
[application]
config/name="MEMORIA_Narrative_Offline_Oracle"
run/main_scene="res://oracle.tscn"
config/use_custom_user_dir=true
config/custom_user_dir_name="MEMORIA_Narrative_Offline_Oracle"
[autoload]
GameManager="*res://gm.gd"
AudioManager="*res://audio_manager_stub.gd"
NotificationToast="*res://notification_stub.gd"
SystemLog="*res://system_log_stub.gd"
MemoryManager="*res://memory.gd"
StoryLog="*res://story.gd"
SceneTransition="*res://transition.gd"
MemoryEngine="*res://engine.gd"
AchievementManager="*res://unused.gd"
SaveManager="*res://unused.gd"
[rendering]
renderer/rendering_method="gl_compatibility"
''',encoding='utf-8')
    (work/'oracle.tscn').write_text('[gd_scene load_steps=2 format=3]\n[ext_resource type="Script" path="res://oracle.gd" id="1"]\n[node name="Oracle" type="Node"]\nscript = ExtResource("1")\n',encoding='utf-8')
    if project_setup is not None: project_setup(work)
    cases=inputs(); (work/'inputs.json').write_bytes(canonical(cases))
    env=os.environ.copy(); env['APPDATA']=str(work/'Roaming'); env['LOCALAPPDATA']=str(work/'Local')
    for k in ('APPDATA','LOCALAPPDATA'): Path(env[k]).mkdir()
    report={'source_raw_hashes':hashes,'vn_filter_prefix_sha256':sha(filtering.encode()),'harness':'unmodified engines, super-calling observers, extracted exact renderer filter prefix; inert presentation/world adapters','commands':[]}
    version=subprocess.check_output([str(args.godot),'--version'],text=True).strip()
    if not version.startswith('4.6.2.'): raise ValueError('Requires Godot 4.6.2')
    report['godot_version']=version
    for name,command in [('import',[str(args.godot),'--headless','--path',str(work),'--editor','--import','--quit']),('execute',[str(args.godot),'--headless','--path',str(work),'--',str(work/'inputs.json'),str(work/'outputs.json')])]:
        try:
            r=subprocess.run(command,capture_output=True,encoding='utf-8',errors='replace',env=env,timeout=90)
        except subprocess.TimeoutExpired as error:
            (ev/(name+'.log')).write_bytes((error.stdout or b'')+(error.stderr or b''))
            raise
        log=r.stdout+r.stderr
        (ev/(name+'.log')).write_text(log,encoding='utf-8')
        report['commands'].append(dict(name=name,command=command,exit_code=r.returncode))
        (ev/'oracle.json').write_bytes(canonical(report))
        if r.returncode or re.search(r'SCRIPT ERROR|Parse Error|FATAL|CRASH|ERROR:',log,re.I): raise ValueError('Oracle '+name+' failed: '+log[-6000:])
    if 'MEMORIA_NARRATIVE_ORACLE_PASS' not in log: raise ValueError('Missing oracle marker')
    expected=parse_source((work/'outputs.json').read_bytes())
    for filename,value in [('contract_inputs.v1.json',cases),('contract_expected.v1.json',expected)]:
        data=canonical(value); target=BASE/filename
        if args.check:
            if target.read_bytes()!=data: raise ValueError('Oracle/check changed '+filename)
        else: target.write_bytes(data)
        report[filename]=sha(data)
    report.update(status='PASS',cases=len(cases)); (ev/'oracle.json').write_bytes(canonical(report)); print('MEMORIA_NARRATIVE_ORACLE_PASS cases='+str(len(cases)))

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--godot',type=Path,required=True); p.add_argument('--evidence-dir',type=Path,required=True); p.add_argument('--check',action='store_true'); run(p.parse_args())
