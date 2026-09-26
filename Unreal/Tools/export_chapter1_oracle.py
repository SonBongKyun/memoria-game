"""Execute the pinned Godot SceneFlow over the whole current Chapter 1 route.

Reuses export_narrative_oracle's isolated harness (byte-copied dialogue_manager,
scene_flow, memory_manager, journey_oath; inert presentation). The six route
scenes are copied to res://data/vn_scenes so SceneFlow loads them itself and
follows goto_scene/goto_map as authored. A driver advances every step and
selects the given original choice index at each choice step, from
ch1_cold_open to the Verdan goto_map. Each case records the event trace
(visits, steps with the final displayed text after distortion, choices, flags,
burns, items, chapter effects) plus choice-point and final snapshots.
"""
from pathlib import Path
import argparse, shutil
import export_narrative_oracle as base
from narrative_ir import ROOT, VN_CASES

DEST = ROOT / 'docs/unreal-migration/fixtures/chapter1'
ROUTE = ['ch1_cold_open', 'ch1_prologue', 'ch1_forest_walk', 'ch1_void_beast', 'ch1_after_forest', 'ch2_market_arrival']

def inputs():
    # Picks are original choice indices, consumed at each choice step in route order:
    # cold open, prologue 26 (song), prologue 33 (rest), forest hub (repeats), void 4, void 30, arrival.
    paths = {
        'burn_song_strike': [0, 0, 0, 0, 3, 0, 0, 0],
        'refuse_humming_burns': [1, 1, 1, 1, 3, 1, 1, 1],
        'read_rhythm_full_forest': [2, 1, 2, 2, 0, 1, 3, 2, 2, 2],
        'burn_song_elia_counter': [0, 0, 2, 2, 3, 2, 2, 0],
    }
    cases = [dict(id=k + '_en', locale='en', picks=v) for k, v in paths.items()]
    cases.append(dict(id='burn_song_strike_ko', locale='ko', picks=paths['burn_song_strike']))
    return cases

VN = '''extends "res://scene_flow_source.gd"
func _run_step() -> void:
    GameManager.events.append("visit:%s:%d" % [current_id, current_index])
    super._run_step()
func _ensure_vn_ui() -> void:
    pass
func _close_vn_ui() -> void:
    pass
func _show_chapter_ledger(chapter: int) -> void:
    # Presentation overlay replaced by its data: the memories burned since set_chapter.
    var ids: PackedStringArray = []
    for i in range(_ledger_burn_snapshot, MemoryManager.burned_memories.size()): ids.append(MemoryManager.burned_memories[i].id)
    GameManager.events.append("ledger:%d:%s" % [chapter, ",".join(ids)])
'''
UNUSED = '''extends Node
func record_chapter_complete(c: int) -> void:
    GameManager.events.append("chapter_complete:%d" % c)
func autosave_on_chapter_transition() -> void:
    GameManager.events.append("autosave:%d" % GameManager.current_chapter)
'''
ORACLE = '''extends Node
var flow: Node
var filter: RefCounted
func _ready() -> void:
    call_deferred("run")
func snapshot(kind: String) -> Dictionary:
    var burned: Array = []
    for m in MemoryManager.burned_memories: burned.append(m.id)
    var passives: Array = MemoryManager.anchor_passives.keys()
    passives.sort()
    return {"kind":kind,"scene":flow.current_id,"index":flow.current_index,"active":flow.is_active,
        "flags":GameManager.story_flags.duplicate(),"grains":GameManager.player_data.grains,"hp":GameManager.player_data.hp,
        "chapter":GameManager.current_chapter,"burned":burned,"map":GameManager.requested_map,
        "anchor_vigil":MemoryManager.anchor_vigil,"anchor_passives":passives,"guard_slots_used":MemoryManager._guard_slots_used}
func shown(step: Dictionary, key: String) -> String:
    return GameManager.localized_value(step, key, String(step.get(key, "")))
func on_step(step: Dictionary) -> void:
    GameManager.events.append("step:%s:%d" % [flow.current_id, flow.current_index])
    GameManager.events.append("text:%s:%s|%s|%s|%s|%s" % ["distorted" if step.get("_distorted", false) else "plain",
        step.get("speaker", ""), shown(step, "text"), shown(step, "narrate"), step.get("portrait", ""), step.get("cg", "")])
    if step.has("choice"):
        var ids: PackedStringArray = []
        for i in filter.exposed(step.choice): ids.append(str(i))
        GameManager.events.append("choices:" + ",".join(ids))
func run() -> void:
    var args := OS.get_cmdline_user_args()
    var cases: Array = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
    var output: Array = []
    filter = load("res://filter.gd").new()
    for c in cases:
        GameManager.story_flags.clear()
        GameManager.player_data = {"elia_with_party":true,"grains":0,"hp":50,"max_hp":100}
        GameManager.current_locale = c.locale
        GameManager.current_chapter = 1
        GameManager.requested_map = ""
        MemoryManager.memories.clear()
        MemoryManager.burned_memories.clear()
        MemoryManager.anchor_vigil = 0
        MemoryManager.anchor_passives.clear()
        MemoryManager._vigil_chapters_counted.clear()
        MemoryManager._guard_slots_used = 0
        MemoryManager._init_starting_memories()
        GameManager.events.clear()
        flow = load("res://vn.gd").new()
        add_child(flow)
        flow.step_changed.connect(on_step)
        var states: Array = [snapshot("start")]
        flow.play("ch1_cold_open")
        var picks: Array = c.picks
        var used := 0
        var guard := 0
        while flow.is_active and guard < 4000:
            guard += 1
            var step: Dictionary = flow.current_steps[flow.current_index] if flow.current_index < flow.current_steps.size() else {}
            if step.has("choice"):
                states.append(snapshot("choice"))
                assert(used < picks.size(), "Ran out of picks")
                flow.select_choice(int(picks[used]))
                used += 1
            else:
                flow.advance()
        assert(used == picks.size(), "Unused picks")
        assert(GameManager.requested_map == "res://scenes/maps/verdan_market.tscn", "Route did not reach Verdan")
        states.append(snapshot("final"))
        output.append({"id":c.id,"events":GameManager.events.duplicate(),"states":states})
        flow.free()
    FileAccess.open(args[1],FileAccess.WRITE).store_string(JSON.stringify(output))
    print("MEMORIA_NARRATIVE_ORACLE_PASS cases=%d" % output.size())
    get_tree().quit(0)
'''

def setup(work):
    (work / 'unused.gd').write_text(UNUSED.replace('    ', '\t'), encoding='utf-8', newline='\n')
    scenes = work / 'data/vn_scenes'
    scenes.mkdir(parents=True)
    for sid in ROUTE:
        shutil.copyfile(ROOT / VN_CASES[sid][0], scenes / (sid + '.json'))

def main():
    p = argparse.ArgumentParser(); p.add_argument('--godot', type=Path, required=True)
    p.add_argument('--evidence-dir', type=Path, required=True); p.add_argument('--check', action='store_true')
    a = p.parse_args()
    DEST.mkdir(parents=True, exist_ok=True)
    base.BASE = DEST; base.inputs = inputs; base.VN = VN.replace('    ', '\t'); base.ORACLE = ORACLE.replace('    ', '\t')
    base.run(a, project_setup=setup)

if __name__ == '__main__':
    main()
