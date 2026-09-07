"""Bounded NPC dispatch with the actual PerceptionFilter and verbatim npc.interact.

Only visual facing and post-boundary normal-dialogue execution are inert.
The paid case also executes the source VN and exact Verdan entry guard.
"""
from pathlib import Path
import argparse, re, textwrap, subprocess
import export_narrative_oracle as base
from export_slice_oracle import route_source

DEST = base.ROOT/'docs/unreal-migration/fixtures/malet'
GROUP = 'malet_taste_burned'
HEARD = 'burn_reaction_heard_' + GROUP

def npc_methods():
    s=(base.ROOT/'scripts/core/npc.gd').read_text(encoding='utf-8')
    return s[s.index('func interact()'):s.index('## PixelSprite')]

def npc_configuration():
    s=(base.ROOT/'scenes/maps/verdan_market.tscn').read_text(encoding='utf-8')
    section=s[s.index('[node name="Malet"'):]
    keys=('npc_name','dialogue_file','dialogue_key','repeat_dialogue_key')
    result={k:re.search(r'^'+k+r' = "([^"]*)"$',section,re.M).group(1) for k in keys}
    world=(base.ROOT/'scenes/maps/verdan_market.gd').read_text(encoding='utf-8')
    match=re.search(r'malet_npc.set_meta\("([^"]*)", "([^"]*)"\)',world)
    result.update(meta=match.group(1),reaction=match.group(2))
    return result

def inputs():
    config=npc_configuration()
    cases=[]
    for name,burned,heard,talked,missing,paid in [
        ('burned_unheard',True,False,False,False,False),
        ('burned_heard',True,True,False,False,False),
        ('intact_unheard',False,False,False,False,False),
        ('burned_unheard_talked',True,False,True,False,False),
        ('burned_heard_talked',True,True,True,False,False),
        ('missing_group',True,False,False,True,False),
        ('canonical_paid',True,False,False,False,True)]:
        cases.append(dict(id=name,burned=burned,heard=heard,talked=talked,missing=missing,paid=paid,config=config))
    return cases

OBSERVER = """extends RefCounted
static func take_burn_reaction(node: Node, file: String) -> Dictionary:
    GameManager.events.append("resolver:begin:burned=%s:heard=%s" % ["true" if MemoryManager.is_memory_burned("daily_market_food") else "false", "true" if GameManager.get_flag("burn_reaction_heard_malet_taste_burned") else "false"])
    var result: Dictionary = load("res://perception_source.gd").take_burn_reaction(node, file)
    GameManager.events.append("resolver:none" if result.is_empty() else "resolver:reaction:" + String(result.key))
    return result
"""

FIELD = """
var invocations: int = 0
var requested: Array = []
func load_and_start(file_path: String, dialogue_key: String) -> void:
    requested.append(dialogue_key)
    GameManager.events.append("request:" + file_path + "::" + dialogue_key)
    if dialogue_key != "malet_taste_burned":
        GameManager.events.append("development:deferred:" + dialogue_key)
        return
    invocations += 1
    GameManager.events.append("field:start:%s:heard=%s" % [dialogue_key, "true" if GameManager.get_flag("burn_reaction_heard_malet_taste_burned") else "false"])
    super.load_and_start(file_path, dialogue_key)
"""

RUN = """func run() -> void:
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    filter = load("res://filter.gd").new()
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100}
        GameManager.current_locale = "en"
        GameManager.current_state = GameManager.GameState.EXPLORATION
        GameManager.requested_map = ""
        MemoryManager.memories.clear()
        MemoryManager.burned_memories.clear()
        MemoryManager._init_starting_memories()
        field = get_node("/root/DialogueManager")
        field.is_active = false
        field.current_index = 0
        field.invocations = 0
        field.requested.clear()
        field.dialogue_line.connect(on_line)
        field.dialogue_ended.connect(on_end)
        GameManager.events.clear()
        var route: Node = null
        if c.paid:
            flow = load("res://vn.gd").new()
            add_child(flow)
            flow.step_changed.connect(on_step)
            var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string("res://data/vn_scenes/ch2_market_arrival.json"))
            flow._cache["ch2_market_arrival"] = data
            GameManager.events.append("vn:start:ch2_market_arrival")
            flow.play("ch2_market_arrival")
            for i in range(10): flow.advance()
            GameManager.events.append("select:vn:1")
            flow.select_choice(1)
            flow.advance()
            assert(not flow.is_active)
            assert(GameManager.requested_map == "res://scenes/maps/verdan_market.tscn")
            GameManager.events.append("travel:verdan")
            GameManager.events.append("verdan:enter")
            route = ObservedVerdan.new()
            route.DialogueManager = field
            add_child(route)
            await route.enter()
        else:
            if c.burned: MemoryManager.burn_memory("daily_market_food")
            if c.heard: GameManager.set_flag("burn_reaction_heard_malet_taste_burned")
            if c.talked: GameManager.set_flag("talked_Malet_malet_encounter")
            GameManager.events.clear()
        var npc = load("res://npc_route.gd").new()
        for key in ["npc_name","dialogue_file","dialogue_key","repeat_dialogue_key"]: npc.set(key,c.config[key])
        npc.set_meta(c.config.meta, "missing_phase1f_group" if c.missing else c.config.reaction)
        var before_burned: Array = []
        for memory in MemoryManager.burned_memories: before_burned.append(memory.id)
        GameManager.events.append("interact:Malet")
        npc.interact()
        var at_start: Dictionary = snapshot("field")
        for i in range(3):
            if field.is_active: field.advance()
        if field.invocations > 0:
            assert(GameManager.current_state == GameManager.GameState.EXPLORATION)
            GameManager.events.append("exploration:ready")
        var result: Dictionary = snapshot("field")
        result.requested = field.requested.duplicate()
        result.field_invocations = field.invocations
        result.memory_unchanged = before_burned == result.burned
        result.at_start = at_start
        output.append({"id":c.id,"states":[result]})
        field.dialogue_line.disconnect(on_line)
        field.dialogue_ended.disconnect(on_end)
        if field.dialogue_ended.is_connected(npc._on_first_talk_ended.bind("talked_Malet_malet_encounter")):
            field.dialogue_ended.disconnect(npc._on_first_talk_ended.bind("talked_Malet_malet_encounter"))
        npc.free()
        if route != null: route.free()
        if c.paid: flow.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
"""

def setup(work):
    for source,dest in [('scripts/systems/perception_filter.gd','perception_source.gd'),
                        ('data/chapter2_dialogue.json','data/chapter2_dialogue.json'),
                        ('data/vn_scenes/ch2_market_arrival.json','data/vn_scenes/ch2_market_arrival.json')]:
        p=work/dest; p.parent.mkdir(parents=True,exist_ok=True); p.write_bytes((base.ROOT/source).read_bytes())
    (work/'observed_perception.gd').write_text(OBSERVER.replace('    ','\t'),encoding='utf-8')
    prefix="""extends Node
const PerceptionFilter = preload("res://observed_perception.gd")
var npc_name: String
var dialogue_file: String
var dialogue_key: String
var repeat_dialogue_key: String
var repeat_line: String = ""
var repeat_line_ko: String = ""
var display_name_ko: String = ""
var _talked_keys: Dictionary = {}
func _face_toward_player() -> void:
    pass
"""
    (work/'npc_route.gd').write_text(prefix.replace('    ','\t') + npc_methods(),encoding='utf-8')
    p=work/'project.godot'; s=p.read_text(encoding='utf-8')
    p.write_text(s.replace('[autoload]','[autoload]\nDialogueManager="*res://field.gd"'),encoding='utf-8')

def run(args):
    guard,methods=route_source()
    classes="""
class MapEffects:
    static func show_chapter_title(_host,_chapter,_name,_subtitle):
        pass
class SourceVerdan extends Node:
    var DialogueManager: Node
    const DIALOGUE_FILE = "res://data/chapter2_dialogue.json"
    func _on_any_dialogue_ended() -> void:
        assert(not GameManager.get_flag("malet_deal_accepted"))
        assert(not GameManager.get_flag("malet_deal_refused"))
    func enter() -> void:
""".replace('    ','\t')+textwrap.indent(guard,'\t')+'\n'+textwrap.indent(methods,'\t')
    classes+="""
class ObservedVerdan extends SourceVerdan:
    func _start_ch2_free_exploration_after_vn() -> void:
        super._start_ch2_free_exploration_after_vn()
        GameManager.events.append("field:skip:verdan_arrival")
        GameManager.events.append("exploration:ready")
""".replace('    ','\t')
    base.FIELD += FIELD
    base.ORACLE=(base.ORACLE[:base.ORACLE.index('func run()')]+RUN).replace('    ','\t')+classes
    base.BASE=DEST; base.inputs=inputs
    if not args.check: DEST.mkdir(parents=True,exist_ok=True)
    base.run(args, project_setup=setup)
    paths=('scripts/core/npc.gd','scripts/systems/perception_filter.gd','scenes/maps/verdan_market.gd','scenes/maps/verdan_market.tscn','data/chapter2_dialogue.json')
    revision=subprocess.check_output(['git','-C',str(base.ROOT),'log','-1','--format=%H','--',*paths],text=True).strip()
    sources={}
    for path in paths:
        data=base.source_bytes((base.ROOT/path).read_bytes())
        git=subprocess.check_output(['git','-C',str(base.ROOT),'show',revision+':'+path])
        if base.source_bytes(git)!=data: raise ValueError('Source differs from revision '+path)
        sources[path]=base.sha(data)
    evidence=dict(revision=revision,sources=sources,npc_methods=npc_methods(),npc_configuration=npc_configuration(),
        npc_methods_sha256=base.sha(npc_methods().encode()),verdan_guard_sha256=base.sha(guard.encode()),case_count=7,
        adapters='Exact resolver source and verbatim npc dispatch; facing inert. Nonselected load_and_start is recorded and not executed. Normal-chain completion callback asserts no deal flags.')
    (args.evidence_dir/'source_attestation.json').write_bytes(base.canonical(evidence))

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--godot',type=Path,required=True); p.add_argument('--evidence-dir',type=Path,required=True); p.add_argument('--check',action='store_true')
    run(p.parse_args())
