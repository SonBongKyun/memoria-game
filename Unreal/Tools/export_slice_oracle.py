"""Phase 1E route observer using the accepted isolated source-engine harness.

Only project setup is reused. Outputs are separate route fixtures, never new
narrative IR/imports. Verdan guard and all three arrival methods execute verbatim.
"""
from pathlib import Path
import argparse, textwrap, json
import export_narrative_oracle as base

DEST = base.ROOT / 'docs/unreal-migration/fixtures/campaign'

def route_source():
    source = (base.ROOT/'scenes/maps/verdan_market.gd').read_text(encoding='utf-8')
    guard = source[source.index('\tif not GameManager.get_flag("ch2_arrived"):'):source.index('\nfunc _process(')]
    methods = source[source.index('func _start_ch2_sequence()'):source.index('## 대화 종료 감지')]
    return guard, methods

def inputs():
    old = base.inputs_original()
    cases = [dict(c) for c in old if c['id'] in ('source_vn_choice_0','source_vn_choice_1','source_field_en')]
    filtered = next(dict(c) for c in old if c['id']=='source_vn_filtered_original')
    filtered['commands'] = filtered['commands'] + [{'advance':True}]
    cases.append(filtered)
    # Fresh-run API's actual player defaults, shared by both engine hosts.
    return cases

RUN = '''func run() -> void:
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    filter = load("res://filter.gd").new()
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":100,"max_hp":100}
        GameManager.current_locale = c.locale
        GameManager.requested_map = ""
        MemoryManager.memories.clear()
        MemoryManager.burned_memories.clear()
        MemoryManager._init_starting_memories()
        for mid in c.burn_before: MemoryManager.burn_memory(mid)
        GameManager.events.clear()
        field = load("res://field.gd").new()
        field.dialogue_line.connect(on_line)
        field.dialogue_choice.connect(on_choices)
        field.dialogue_ended.connect(on_end)
        var route = ObservedVerdan.new()
        route.DialogueManager = field
        add_child(route)
        var selections: Array = []
        if c.dialect == "vn":
            flow = load("res://vn.gd").new()
            add_child(flow)
            flow.step_changed.connect(on_step)
            flow.scene_ended.connect(on_end)
            flow._cache["ch2_market_arrival"] = {"steps":c.rows}
            GameManager.events.append("vn:start:ch2_market_arrival")
            flow.play("ch2_market_arrival")
            for command in c.commands:
                if command.has("advance"): flow.advance()
                if command.has("select"):
                    selections.append(int(command.select))
                    GameManager.events.append("select:vn:%d" % int(command.select))
                    flow.select_choice(int(command.select))
            assert(not flow.is_active)
            assert(GameManager.requested_map == "res://scenes/maps/verdan_market.tscn")
            GameManager.events.append("travel:verdan")
            GameManager.events.append("verdan:enter")
            await route.enter()
        else:
            field.loaded_dialogues[route.DIALOGUE_FILE] = {"verdan_arrival":c.rows}
            GameManager.events.append("fixture:vn_unseen")
            GameManager.events.append("verdan:enter")
            await route.enter()
            for command in c.commands: field.advance()
        var result = snapshot(c.dialect)
        result.field_invocations = field.invocations
        result.selections = selections
        result.exploration = GameManager.current_state == GameManager.GameState.EXPLORATION
        result.owned_count = MemoryManager.memories.size()
        output.append({"id":c.id,"states":[result]})
        if c.dialect == "vn": flow.free()
        field.free()
        route.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
'''

def run(args):
    guard, methods = route_source()
    # Inert title only; real timer, guard, calls, signal connections and source
    # DialogueManager load/start/end execute. No branch decision is stubbed.
    classes = '''
class MapEffects:
    static func show_chapter_title(_host, _chapter, _name, _subtitle):
        pass
class SourceVerdan extends Node:
    var DialogueManager: Node
    const DIALOGUE_FILE = "res://data/chapter2_dialogue.json"
    func _on_any_dialogue_ended() -> void:
        pass # next NPC chain is outside this entry fixture
    func enter() -> void:
'''.replace('    ','\t') + textwrap.indent(guard, '\t') + '\n' + textwrap.indent(methods, '\t')
    classes += '''
class ObservedVerdan extends SourceVerdan:
    func _start_ch2_free_exploration_after_vn() -> void:
        super._start_ch2_free_exploration_after_vn()
        GameManager.events.append("field:skip:verdan_arrival")
        GameManager.events.append("exploration:ready")
    func _on_arrival_ended() -> void:
        super._on_arrival_ended()
        GameManager.events.append("exploration:ready")
'''.replace('    ','\t')
    base.ORACLE = (base.ORACLE[:base.ORACLE.index('func run()')] + RUN).replace('    ', '\t') + classes
    base.FIELD += '''
var invocations: int = 0
func load_and_start(file_path: String, dialogue_key: String) -> void:
    invocations += 1
    GameManager.events.append("field:start:" + dialogue_key)
    super.load_and_start(file_path, dialogue_key)
'''
    base.inputs_original = base.inputs
    base.inputs = inputs
    base.BASE = DEST
    if not args.check: DEST.mkdir(parents=True, exist_ok=True)
    base.run(args)
    evidence = args.evidence_dir/'route_source.json'
    evidence.write_bytes(base.canonical({'guard_sha256':base.sha(guard.encode()),'methods_sha256':base.sha(methods.encode()),
        'guard':guard,'methods':methods,'source_sha256':base.sha((base.ROOT/'scenes/maps/verdan_market.gd').read_bytes()),
        'observers':'super calls only; exact source guard/methods; inert title and out-of-scope NPC callback',
        'case_count':4}))

if __name__ == '__main__':
    p=argparse.ArgumentParser(); p.add_argument('--godot',type=Path,required=True)
    p.add_argument('--evidence-dir',type=Path,required=True); p.add_argument('--check',action='store_true')
    run(p.parse_args())
