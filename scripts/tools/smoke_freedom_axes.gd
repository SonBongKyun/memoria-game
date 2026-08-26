## Freedom axes regression: journey oaths (S263) + memory carry weight (S263).
## Covers oath lifecycle/breaks/rewards, the Still Hands erosion shield,
## carry-weight math, overload-driven erosion acceleration, pause-menu oath
## panel rendering, and source-level wiring at every report site.
extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")

var _runner = SmokeTestRunner.new("freedom_axes", "FREEDOM_AXES_SMOKE_PASS")


func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "en"
	_reset_oath_state()
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.erosion_guarded.clear()
	MemoryManager.active_loan = {}
	GameManager.player_data["grains"] = 100

	test_oath_lifecycle()
	test_oath_witness_multiplier()
	test_oath_still_burn_reports()
	test_still_hands_erosion_shield()
	test_oath_of_ash_payout()
	test_carry_weight_math()
	test_overload_accelerates_erosion()
	test_save_round_trip_preserves_oaths()
	await test_oaths_panel_renders_and_swear_works()
	test_source_wiring()

	await get_tree().process_frame  # 패널 정리(queue_free)가 실제로 반영된 뒤 종료한다
	_runner.finish(get_tree())


func _reset_oath_state() -> void:
	for key in GameManager.story_flags.keys():
		if String(key).begins_with("oath_"):
			GameManager.story_flags.erase(key)
	GameManager.player_data["oath_chapters_kept"] = {}


func _mk(id: String, grade: int, npc: String = "", power: int = 100) -> MemoryManager.Memory:
	return MemoryManager.Memory.new(id, id.to_pascal_case(), "Smoke memory.", grade, power, "", npc)


func test_oath_lifecycle() -> void:
	_runner.begin_test("oath_lifecycle")
	_reset_oath_state()
	_runner.expect(JourneyOath.swear(JourneyOath.ASH), "A fresh oath must be swearable")
	_runner.expect(not JourneyOath.swear(JourneyOath.ASH), "An already-sworn oath cannot be re-sworn")
	_runner.expect(not JourneyOath.swear("nonexistent"), "Unknown oath ids are rejected")
	_runner.expect(JourneyOath.is_active(JourneyOath.ASH), "A sworn oath is active")
	_runner.expect(JourneyOath.definition(JourneyOath.WITNESS).has("vow_en"), "Every oath definition carries its vow text")

	JourneyOath.on_memory_sold()
	_runner.expect(JourneyOath.is_broken(JourneyOath.ASH), "Selling a memory breaks the Oath of Ash")
	_runner.expect(not JourneyOath.is_active(JourneyOath.ASH), "A broken oath stops being active")


func test_oath_witness_multiplier() -> void:
	_runner.begin_test("oath_witness_multiplier")
	_reset_oath_state()
	_runner.expect(absf(JourneyOath.witness_flow_multiplier() - 1.0) < 0.001, "Without the oath, flow builds at the normal rate")
	_runner.expect(JourneyOath.swear(JourneyOath.WITNESS), "The Oath of Witness is swearable")
	_runner.expect(absf(JourneyOath.witness_flow_multiplier() - 1.15) < 0.001, "Keeping the oath speeds flow accumulation by 15%")
	JourneyOath.on_threat_bypassed()
	_runner.expect(JourneyOath.is_broken(JourneyOath.WITNESS), "Bypassing a witnessed threat breaks the Oath of Witness")
	_runner.expect(absf(JourneyOath.witness_flow_multiplier() - 1.0) < 0.001, "The flow bonus ends the moment the oath breaks")


func test_oath_still_burn_reports() -> void:
	_runner.begin_test("oath_still_burn_reports")
	_reset_oath_state()
	_runner.expect(JourneyOath.swear(JourneyOath.STILL), "The Oath of Still Hands is swearable")
	var plain := _mk("still_plain", MemoryManager.MemoryGrade.GRADE_3)
	var elia := _mk("still_elia", MemoryManager.MemoryGrade.GRADE_3, "Elia")
	JourneyOath.on_player_burn(plain)
	_runner.expect(JourneyOath.is_active(JourneyOath.STILL), "Burning an unrelated memory keeps the oath intact")
	JourneyOath.on_player_burn(null)
	_runner.expect(JourneyOath.is_active(JourneyOath.STILL), "Null reports never break oaths")
	JourneyOath.on_player_burn(elia)
	_runner.expect(JourneyOath.is_broken(JourneyOath.STILL), "Burning an Elia-tied memory by hand breaks the oath")


func test_still_hands_erosion_shield() -> void:
	_runner.begin_test("still_hands_erosion_shield")
	_reset_oath_state()
	MemoryManager.memories.clear()
	MemoryManager.erosion_guarded.clear()
	GameManager.current_chapter = 3
	_runner.expect(JourneyOath.swear(JourneyOath.STILL), "Shield test requires the oath")
	var elia := _mk("shield_elia", MemoryManager.MemoryGrade.GRADE_4, "Elia", 60)
	var plain_a := _mk("shield_plain_a", MemoryManager.MemoryGrade.GRADE_5, "", 60)
	var plain_b := _mk("shield_plain_b", MemoryManager.MemoryGrade.GRADE_5, "", 60)
	MemoryManager.memories.append(elia)
	MemoryManager.memories.append(plain_a)
	MemoryManager.memories.append(plain_b)

	MemoryManager.apply_erosion(3)
	_runner.expect(elia.erosion == 0, "Still Hands stops Elia-tied memories from eroding")
	_runner.expect(plain_a.erosion > 0 and plain_b.erosion > 0, "Unrelated memories still erode normally under the shield")

	JourneyOath.on_player_burn(elia)
	MemoryManager.erosion_guarded.clear()
	MemoryManager.apply_erosion(4)
	_runner.expect(elia.erosion > 0, "Once the oath is broken, Elia's memories erode like any other")


func test_oath_of_ash_payout() -> void:
	_runner.begin_test("oath_of_ash_payout")
	_reset_oath_state()
	_runner.expect(JourneyOath.swear(JourneyOath.ASH), "Payout test requires the oath")
	var before := int(GameManager.player_data.get("grains", 0))
	JourneyOath.on_chapter_advanced(5)
	var gained := int(GameManager.player_data.get("grains", 0)) - before
	_runner.expect(gained == 40, "Each kept chapter pays 40 Grains")
	_runner.expect(int(GameManager.player_data.get("oath_chapters_kept", {}).get(JourneyOath.ASH, 0)) == 1, "Kept chapters are counted per oath")
	JourneyOath.on_memory_sold()
	var before_broken := int(GameManager.player_data.get("grains", 0))
	JourneyOath.on_chapter_advanced(6)
	_runner.expect(int(GameManager.player_data.get("grains", 0)) == before_broken, "A broken oath pays nothing")


func test_carry_weight_math() -> void:
	_runner.begin_test("carry_weight_math")
	_reset_oath_state()
	MemoryManager.memories.clear()
	MemoryManager.active_loan = {}

	GameManager.current_chapter = 1
	_runner.expect(MemoryManager.get_carry_capacity() == 14, "Chapter 1 capacity is the base 14")
	GameManager.current_chapter = 9
	_runner.expect(MemoryManager.get_carry_capacity() == 30, "Capacity grows two points per chapter")
	_runner.expect(MemoryManager.get_carry_weight() == 0, "An empty archive weighs nothing")

	GameManager.current_chapter = 1
	MemoryManager.memories.append(_mk("carry_g5", MemoryManager.MemoryGrade.GRADE_5))
	MemoryManager.memories.append(_mk("carry_g3", MemoryManager.MemoryGrade.GRADE_3))
	MemoryManager.memories.append(_mk("carry_g1", MemoryManager.MemoryGrade.GRADE_1))
	_runner.expect(MemoryManager.get_carry_weight() == 10, "Grades weigh 1/2/3/4/6 — this archive holds 10")

	MemoryManager.active_loan = {"memory_id": "carry_g3"}
	_runner.expect(MemoryManager.get_carry_weight() == 7, "Collateral pledged away is no longer Arrel's burden")
	MemoryManager.active_loan = {}

	var g1 := MemoryManager.find_memory("carry_g1")
	g1.is_burned = true
	_runner.expect(MemoryManager.get_carry_weight() == 4, "Burned memories stop weighing anything")
	_runner.expect(not MemoryManager.is_carry_overloaded(), "4 of 14 is not overloaded")


func test_overload_accelerates_erosion() -> void:
	_runner.begin_test("overload_accelerates_erosion")
	_reset_oath_state()
	MemoryManager.memories.clear()
	MemoryManager.erosion_guarded.clear()
	MemoryManager.active_loan = {}
	GameManager.current_chapter = 3  # capacity 18

	for i in range(3):
		MemoryManager.memories.append(_mk("light_%d" % i, MemoryManager.MemoryGrade.GRADE_5, "", 90))
	_runner.expect(not MemoryManager.is_carry_overloaded(), "The light archive sits inside its capacity")
	MemoryManager.apply_erosion(3)
	var light := MemoryManager.find_memory("light_0")
	_runner.expect(light.erosion == 3, "Chapter 3 erodes 3 without overload")

	MemoryManager.memories.clear()
	for i in range(12):
		MemoryManager.memories.append(_mk("heavy_%d" % i, MemoryManager.MemoryGrade.GRADE_3, "", 90))
	_runner.expect(MemoryManager.is_carry_overloaded(), "Twelve relational memories overload the archive")
	_runner.expect(absf(MemoryManager.get_carry_overload_ratio() - 1.0) < 0.001, "Double the capacity clamps the overload ratio at 1.0")
	MemoryManager.apply_erosion(3)
	var heavy := MemoryManager.find_memory("heavy_0")
	_runner.expect(heavy.erosion == 6, "Fully overloaded archives erode twice as fast")


func test_save_round_trip_preserves_oaths() -> void:
	_runner.begin_test("save_round_trip")
	_reset_oath_state()
	_runner.expect(JourneyOath.swear(JourneyOath.ASH), "Round trip requires a sworn oath")
	JourneyOath.on_chapter_advanced(7)

	var snapshot := GameManager.export_data()
	_reset_oath_state()
	GameManager.import_data(snapshot)
	_runner.expect(JourneyOath.is_sworn(JourneyOath.ASH), "Sworn oaths survive the save round trip")
	_runner.expect(not JourneyOath.is_broken(JourneyOath.ASH), "An unbroken oath loads unbroken")
	_runner.expect(int(GameManager.player_data.get("oath_chapters_kept", {}).get(JourneyOath.ASH, 0)) == 1, "Kept-chapter counts survive the round trip")


func test_oaths_panel_renders_and_swear_works() -> void:
	_runner.begin_test("oaths_panel_render")
	_reset_oath_state()
	await get_tree().process_frame
	var pause_menu := get_node_or_null("/root/PauseMenu")
	if not _runner.expect(pause_menu != null, "PauseMenu autoload must exist"):
		return
	pause_menu._show_oaths_panel()
	var overlay: Control = pause_menu.get_node_or_null("OathsOverlay")
	if not _runner.expect(overlay != null, "The oaths panel must open as OathsOverlay"):
		return

	var swear_buttons: Array[Button] = []
	_collect_swear_buttons(overlay, swear_buttons)
	_runner.expect(_overlay_has_text(overlay, "JOURNEY OATHS"), "The panel names itself JOURNEY OATHS")
	_runner.expect(swear_buttons.size() == 3, "All three unsworn vows offer a swear action")

	if swear_buttons.size() > 0:
		swear_buttons[0].emit_signal("pressed")
		_runner.expect(GameManager.get_flag("oath_ash_sworn"), "Pressing SWEAR records the vow")
		_runner.expect(is_instance_valid(swear_buttons[0]) and not swear_buttons[0].visible, "A sworn row hides its swear action")

	overlay.queue_free()


func _collect_swear_buttons(node: Node, swear_buttons: Array[Button]) -> void:
	for child in node.get_children():
		if child is Button and String(child.text) == "SWEAR":
			swear_buttons.append(child)
		_collect_swear_buttons(child, swear_buttons)


func _overlay_has_text(node: Node, needle: String) -> bool:
	if node is Label and needle in String(node.text):
		return true
	for child in node.get_children():
		if _overlay_has_text(child, needle):
			return true
	return false


func test_source_wiring() -> void:
	_runner.begin_test("source_wiring")
	var expectations := {
		"res://scripts/systems/memory_manager.gd": ["JourneyOath.on_chapter_advanced", "JourneyOath.still_hands_shields", "get_carry_weight", "_notify_carry"],
		"res://scripts/ui/memory_shop.gd": ["JourneyOath.on_memory_sold"],
		"res://scripts/systems/field_threat.gd": ["JourneyOath.on_threat_bypassed"],
		"res://scripts/systems/battle_manager.gd": ["JourneyOath.on_player_burn"],
		"res://scripts/systems/dialogue_manager.gd": ["JourneyOath.on_player_burn"],
		"res://scripts/systems/scene_flow.gd": ["JourneyOath.on_player_burn"],
		"res://scripts/utils/memory_resonance.gd": ["JourneyOath.on_player_burn"],
		"res://scripts/systems/field_flow.gd": ["witness_flow_multiplier"],
	}
	for path: String in expectations.keys():
		var script = load(path)
		if not _runner.expect(script != null and script is Script, "Source readable: %s" % path):
			continue
		var src := String(script.source_code)
		for needle: String in expectations[path]:
			_runner.expect(src.contains(needle), "%s must reference %s" % [path.get_file(), needle])
