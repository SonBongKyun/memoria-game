extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const VERDAN_MARKET_SCENE = preload("res://scenes/maps/verdan_market.tscn")
const SEAM_OUTSKIRTS_SCENE = preload("res://scenes/maps/seam_outskirts.tscn")
const CHAPTER7_DIALOGUE_PATH := "res://data/chapter7_dialogue.json"
const MALET_ACTOR_ID := "npc.malet"
const MALET_FACT_ID := "fact.arrel.seeks_bl07"
const MALET_MEMORY_ID := "memory.malet.bl07_request_source"
const SABLE_ACTOR_ID := "npc.sable"
const SABLE_FACT_ID := "fact.authority.child_memory_erasure_order"
const SABLE_MEMORY_ID := "memory.sable.child_memory_erasure_target"
const TEST_SAVE_SLOT := 1

var _runner = SmokeTestRunner.new(
	"sable_memory_gameplay", "SABLE_MEMORY_GAMEPLAY_SMOKE_PASS")
var _events: Array[Dictionary] = []
var _last_line: Dictionary = {}
var _last_choices: Array = []


func _ready() -> void:
	Codex.suppress_recording = true
	StoryLog.suppress_persistence = true
	var story_registry_before := _file_fingerprint(StoryLog.READ_REGISTRY_PATH)
	var game_before := GameManager.export_data()
	var game_state_before := GameManager.current_state
	var legacy_memory_before := MemoryManager.export_data()

	if not SmokeSaveSandbox.activate("sable_memory_gameplay", _runner):
		_runner.finish(get_tree())
		return

	EventBus.world_event_committed.connect(_on_world_event_committed)
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	DialogueManager.dialogue_choice.connect(_on_dialogue_choice)
	GameManager.current_locale = "en"
	GameManager.current_chapter = 7
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	WorldState.reset_to_defaults()
	GameManager.set_flag("ch2_malet_done")
	GameManager.set_flag("ch2_complete")
	GameManager.set_flag("talked_Malet_malet_encounter")
	GameManager.set_flag("ch7_sable_truth")
	GameManager.set_flag("talked_Sable_sable_confession")
	var progression_flags := GameManager.story_flags.duplicate(true)

	var verdan_map = VERDAN_MARKET_SCENE.instantiate()
	_check_malet_seed(verdan_map)
	verdan_map.free()

	var outskirts_map = SEAM_OUTSKIRTS_SCENE.instantiate()
	var sable = outskirts_map.get_node_or_null("Sable")
	_check_chapter7_scene_contract(outskirts_map, sable)
	_check_sable_seed(outskirts_map)
	if sable != null:
		outskirts_map.remove_child(sable)
	outskirts_map.free()
	if sable != null:
		add_child(sable)

	_check_combination(sable, "a", "sable_combo_a", "monument_combo_a",
		"ask_cleaner_target", "ask_malet_provenance")
	_remove_sable_through_live_dialogue(sable)
	_check_combination(sable, "c", "sable_combo_c", "monument_combo_c",
		"ask_cleaner_gap", "ask_malet_provenance")
	_check_round_trip_keeps_combination_c(sable)

	_runner.begin_test("remove_malet_source_reaches_combination_d")
	var before := _events.size()
	_expect(MemoryEngine.remove_memory(MALET_ACTOR_ID, MALET_MEMORY_ID),
		"Malet source removal failed after the round trip")
	_expect(_events.size() == before + 1,
		"Malet source removal did not emit exactly one event")
	_expect(MemoryEngine.knows_fact(MALET_ACTOR_ID, MALET_FACT_ID),
		"Removing Malet's source memory also removed route knowledge")
	_check_combination(sable, "d", "sable_combo_d", "monument_combo_d",
		"ask_cleaner_gap", "ask_malet_gap")

	_restore_sable_through_live_dialogue(sable)
	_check_combination(sable, "b", "sable_combo_b", "monument_combo_b",
		"ask_cleaner_target", "ask_malet_gap")

	_runner.begin_test("restore_both_returns_combination_a")
	before = _events.size()
	_expect(MemoryEngine.restore_memory(MALET_ACTOR_ID, MALET_MEMORY_ID),
		"Malet source restoration failed")
	_expect(_events.size() == before + 1,
		"Malet source restoration did not emit exactly one event")
	_check_combination(sable, "a_restored", "sable_combo_a", "monument_combo_a",
		"ask_cleaner_target", "ask_malet_provenance")
	before = _events.size()
	_expect(not MemoryEngine.restore_memory(MALET_ACTOR_ID, MALET_MEMORY_ID),
		"Restoring an active Malet memory must be a no-op")
	_expect(not MemoryEngine.restore_memory(SABLE_ACTOR_ID, SABLE_MEMORY_ID),
		"Restoring an active Sable memory must be a no-op")
	_expect(_events.size() == before, "No-op restorations emitted events")

	_check_mutation_boundaries()
	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	if sable != null:
		sable.free()
	_finish(game_before, game_state_before, legacy_memory_before,
		progression_flags, story_registry_before)


func _check_malet_seed(verdan_map: Node) -> void:
	_runner.begin_test("existing_malet_state_reused")
	_expect(verdan_map != null, "Verdan Market scene could not be instantiated")
	if verdan_map == null:
		return
	var before := _events.size()
	verdan_map.call("_seed_malet_memory_world_state_if_needed")
	_expect(_events.size() == before + 2,
		"Existing Malet lifecycle seed must add one fact and one memory")
	_expect(MemoryEngine.knows_fact(MALET_ACTOR_ID, MALET_FACT_ID),
		"Malet route knowledge was not seeded")
	_expect(MemoryEngine.check_memory(MALET_ACTOR_ID, MALET_MEMORY_ID),
		"Malet source memory was not seeded")


func _check_chapter7_scene_contract(outskirts_map: Node, sable: Node) -> void:
	_runner.begin_test("actual_chapter7_sable_and_monument_contract")
	_expect(ActorRegistry.has_actor(SABLE_ACTOR_ID),
		"actors.json does not contain npc.sable")
	_expect(outskirts_map != null, "Seam Outskirts scene could not be instantiated")
	_expect(sable != null, "Seam Outskirts scene does not contain Sable")
	if sable != null:
		_expect(String(sable.get("dialogue_file")) == CHAPTER7_DIALOGUE_PATH,
			"Chapter 7 Sable uses the wrong dialogue file")
		_expect(String(sable.get("dialogue_key")) == "sable_confession",
			"Sable's optional Cleaner confession is not live")
		_expect(String(sable.get("repeat_dialogue_key")) \
			== "sable_cleaner_memory_followup",
			"Sable's persistent follow-up is not live")
	var map_source := _read_source("res://scenes/maps/seam_outskirts.gd")
	_expect(map_source.contains('"outskirts_monument", "ch7_monument"'),
		"The authored twelve-name memorial is not reachable on the live map")
	_expect(map_source.contains('load_and_start(DIALOGUE_FILE, "sable_truth")') \
		and map_source.contains('load_and_start(DIALOGUE_FILE, "sable_trial")') \
		and map_source.contains('GameManager.set_flag("ch7_trial_complete")'),
		"Chapter 7 main progression contract changed")
	_expect(DialogueManager.load_dialogue_file(CHAPTER7_DIALOGUE_PATH),
		"Chapter 7 dialogue file could not be loaded")
	var dialogues: Dictionary = DialogueManager.loaded_dialogues.get(
		CHAPTER7_DIALOGUE_PATH, {})
	for key in ["outskirts_arrival", "sable_truth", "sable_trial",
			"trial_complete", "sable_confession",
			"sable_cleaner_memory_followup", "outskirts_monument"]:
		_expect(dialogues.has(key), "Required Chapter 7 dialogue missing: %s" % key)


func _check_sable_seed(outskirts_map: Node) -> void:
	_runner.begin_test("sable_fact_and_personal_memory_seed")
	_expect(not MemoryEngine.knows_fact(SABLE_ACTOR_ID, SABLE_FACT_ID),
		"Sable fact existed before its lifecycle seed")
	_expect(WorldState.get_memory_record(SABLE_ACTOR_ID, SABLE_MEMORY_ID).is_empty(),
		"Sable personal memory existed before its lifecycle seed")
	var before := _events.size()
	outskirts_map.call("_seed_sable_cleaner_state_if_needed")
	_expect(_events.size() == before + 2,
		"First Sable seed must commit one fact and one memory")
	_expect(MemoryEngine.knows_fact(SABLE_ACTOR_ID, SABLE_FACT_ID),
		"Sable did not retain knowledge of the Cleaner order")
	_expect(MemoryEngine.check_memory(SABLE_ACTOR_ID, SABLE_MEMORY_ID),
		"Sable's target memory was not active after seeding")
	var record := WorldState.get_memory_record(SABLE_ACTOR_ID, SABLE_MEMORY_ID)
	_expect(String(record.get("source_actor_id", "")) == SABLE_ACTOR_ID,
		"Sable's personal memory must identify Sable as its witness")
	before = _events.size()
	outskirts_map.call("_seed_sable_cleaner_state_if_needed")
	_expect(_events.size() == before, "Sable scene re-entry seed was not a no-op")


func _check_combination(sable: Node, label: String, expected_branch: String,
		expected_monument: String, expected_sable_choice: String,
		expected_malet_choice: String) -> void:
	_runner.begin_test("combination_%s_live_sable_interaction" % label)
	if sable == null:
		_expect(false, "Cannot test a live Sable interaction without the NPC")
		return
	var before := _events.size()
	_start_sable_interaction(sable, expected_branch)
	_advance_to_root_choices()
	_expect(_choice_index(expected_sable_choice) >= 0,
		"Sable option missing in combination %s" % label)
	_expect(_choice_index(expected_malet_choice) >= 0,
		"Malet provenance option missing in combination %s" % label)
	var contradictory_sable := "ask_cleaner_gap" \
		if expected_sable_choice == "ask_cleaner_target" else "ask_cleaner_target"
	var contradictory_malet := "ask_malet_gap" \
		if expected_malet_choice == "ask_malet_provenance" else "ask_malet_provenance"
	_expect(_choice_index(contradictory_sable) < 0,
		"Contradictory Sable option visible in combination %s" % label)
	_expect(_choice_index(contradictory_malet) < 0,
		"Contradictory Malet option visible in combination %s" % label)
	DialogueManager.end_dialogue()
	_expect(_select_monument_branch() == expected_monument,
		"The twelve-name memorial selected the wrong branch in combination %s" % label)
	_expect(_events.size() == before,
		"Read-only Sable/monument interactions emitted mutation events")


func _remove_sable_through_live_dialogue(sable: Node) -> void:
	_runner.begin_test("live_sable_choice_removes_target_memory")
	_start_sable_interaction(sable, "sable_combo_a")
	_advance_to_root_choices()
	var choice_index := _choice_index("remove_sable_target_memory")
	_expect(choice_index >= 0, "Live Sable dialogue has no memory removal action")
	var before := _events.size()
	if choice_index >= 0:
		DialogueManager.select_choice(choice_index)
	_expect(_events.size() == before + 1,
		"Sable removal choice must emit exactly one event")
	_expect(_count_event("memory.removed", SABLE_ACTOR_ID, SABLE_MEMORY_ID) == 1,
		"Sable removal event was missing or duplicated")
	_expect(String(_last_line.get("branch_id", "")) == "sable_target_removed_feedback",
		"Sable removal did not show its authored feedback")
	_expect(not MemoryEngine.check_memory(SABLE_ACTOR_ID, SABLE_MEMORY_ID),
		"Sable target memory remained active")
	_expect(MemoryEngine.knows_fact(SABLE_ACTOR_ID, SABLE_FACT_ID),
		"Removing Sable's target memory also removed event knowledge")
	before = _events.size()
	_expect(not MemoryEngine.remove_memory(SABLE_ACTOR_ID, SABLE_MEMORY_ID),
		"Removing a Sable tombstone must be a no-op")
	_expect(_events.size() == before, "No-op Sable removal emitted an event")
	DialogueManager.end_dialogue()


func _restore_sable_through_live_dialogue(sable: Node) -> void:
	_runner.begin_test("live_sable_choice_restores_target_memory")
	_start_sable_interaction(sable, "sable_combo_d")
	_advance_to_root_choices()
	var choice_index := _choice_index("restore_sable_target_memory")
	_expect(choice_index >= 0, "Live Sable dialogue has no memory restoration action")
	var before := _events.size()
	if choice_index >= 0:
		DialogueManager.select_choice(choice_index)
	_expect(_events.size() == before + 1,
		"Sable restoration choice must emit exactly one event")
	_expect(_count_event("memory.restored", SABLE_ACTOR_ID, SABLE_MEMORY_ID) == 1,
		"Sable restoration event was missing or duplicated")
	_expect(String(_last_line.get("branch_id", "")) == "sable_target_restored_feedback",
		"Sable restoration did not show its authored feedback")
	_expect(MemoryEngine.check_memory(SABLE_ACTOR_ID, SABLE_MEMORY_ID),
		"Sable target memory was not restored")
	_expect(MemoryEngine.knows_fact(SABLE_ACTOR_ID, SABLE_FACT_ID),
		"Sable knowledge changed during restoration")
	DialogueManager.end_dialogue()


func _check_round_trip_keeps_combination_c(sable: Node) -> void:
	_runner.begin_test("sandbox_round_trip_keeps_combination_c")
	var save_path := SmokeSaveSandbox.get_slot_path(TEST_SAVE_SLOT, _runner)
	_expect(save_path != "", "Sable smoke save slot did not resolve in sandbox")
	var expected_state := WorldState.export_data()
	var before := _events.size()
	_expect(SaveManager.save_game(TEST_SAVE_SLOT),
		"Sable state could not save through the real SaveManager")
	WorldState.reset_to_defaults()
	_expect(WorldState.get_memory_record(SABLE_ACTOR_ID, SABLE_MEMORY_ID).is_empty(),
		"WorldState reset did not clear Sable's record")
	_expect(SaveManager.reload_test_world_state(TEST_SAVE_SLOT),
		"Sandbox load could not restore Sable's state")
	_expect(WorldState.export_data() == expected_state,
		"Save -> reset -> load changed the combined world state")
	_expect(_events.size() == before, "Save/load replayed world events")
	_check_combination(sable, "c_loaded", "sable_combo_c", "monument_combo_c",
		"ask_cleaner_gap", "ask_malet_provenance")


func _select_monument_branch() -> String:
	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	_last_line = {}
	DialogueManager.load_and_start(CHAPTER7_DIALOGUE_PATH, "outskirts_monument")
	while DialogueManager.is_active:
		var branch_id := String(_last_line.get("branch_id", ""))
		if branch_id.begins_with("monument_combo_"):
			DialogueManager.end_dialogue()
			return branch_id
		DialogueManager.advance()
	return ""


func _check_mutation_boundaries() -> void:
	_runner.begin_test("sable_mutation_api_boundaries")
	var map_source := _read_source("res://scenes/maps/seam_outskirts.gd")
	var seed_body := _function_body(map_source,
		"func _seed_sable_cleaner_state_if_needed() -> void:")
	_expect(seed_body.contains("MemoryEngine.learn_fact") \
		and seed_body.contains("MemoryEngine.add_memory"),
		"Sable lifecycle seed bypasses MemoryEngine")
	_expect(not seed_body.contains("WorldState._store_") \
		and not seed_body.contains("WorldState.import_data") \
		and not seed_body.contains("GameManager.story_flags"),
		"Sable lifecycle seed directly writes persistent state")
	var dialogue_source := _read_source("res://scripts/systems/dialogue_manager.gd")
	var effects_body := _function_body(dialogue_source,
		"func _apply_memory_world_choice_effects(choice: Dictionary) -> void:")
	_expect(effects_body.contains("MemoryEngine.remove_memory") \
		and effects_body.contains("MemoryEngine.restore_memory"),
		"Dialogue memory choices do not use MemoryEngine")
	_expect(not effects_body.contains("WorldState.") \
		and not effects_body.contains("GameManager.story_flags") \
		and not effects_body.contains("MemoryManager."),
		"Dialogue memory choices contain forbidden direct writes")


func _start_sable_interaction(sable: Node, expected_branch: String) -> void:
	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	_last_line = {}
	_last_choices = []
	sable.call("interact")
	_expect(DialogueManager.is_active, "Actual Sable NPC did not start dialogue")
	_expect(String(_last_line.get("speaker", "")) == "Sable",
		"Actual Sable NPC did not emit a Sable line")
	_expect(String(_last_line.get("branch_id", "")) == expected_branch,
		"Actual Sable NPC selected %s instead of %s" % [
			String(_last_line.get("branch_id", "<none>")), expected_branch])


func _advance_to_root_choices() -> void:
	_last_choices = []
	DialogueManager.advance()
	_expect(not _last_choices.is_empty(),
		"Sable follow-up did not reach its conditional options")


func _choice_index(choice_id: String) -> int:
	for index in range(_last_choices.size()):
		var choice: Variant = _last_choices[index]
		if choice is Dictionary and String(choice.get("choice_id", "")) == choice_id:
			return index
	return -1


func _count_event(event_type: String, actor_id: String, target_id: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type \
				and String(event.get("actor_id", "")) == actor_id \
				and String(event.get("target_id", "")) == target_id:
			count += 1
	return count


func _on_world_event_committed(event: Dictionary) -> void:
	_events.append(event.duplicate(true))


func _on_dialogue_line(speaker: String, text: String, portrait: String) -> void:
	var branch_id := ""
	if DialogueManager.current_index >= 0 \
			and DialogueManager.current_index < DialogueManager.current_dialogue.size():
		branch_id = String(DialogueManager.current_dialogue[
			DialogueManager.current_index].get("branch_id", ""))
	_last_line = {
		"speaker": speaker,
		"text": text,
		"portrait": portrait,
		"branch_id": branch_id,
	}


func _on_dialogue_choice(choices: Array) -> void:
	_last_choices = choices.duplicate(true)


func _finish(game_before: Dictionary, game_state_before: int,
		legacy_memory_before: Dictionary, progression_flags: Dictionary,
		story_registry_before: String) -> void:
	_runner.begin_test("regression_and_persistent_data_isolation")
	_expect(GameManager.story_flags == progression_flags,
		"Sable Memory World interactions changed legacy story_flags")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Sable Memory World interactions changed MemoryManager")
	_expect(_file_fingerprint(StoryLog.READ_REGISTRY_PATH) == story_registry_before,
		"Sable smoke changed the persistent StoryLog/read registry")
	_expect(SaveManager.is_smoke_test_mode() \
		and SaveManager.is_test_save_root_configured(),
		"Sable smoke lost the production save-path guard")
	_expect(_count_event("knowledge.learned", MALET_ACTOR_ID, MALET_FACT_ID) == 1 \
		and _count_event("memory.added", MALET_ACTOR_ID, MALET_MEMORY_ID) == 1 \
		and _count_event("knowledge.learned", SABLE_ACTOR_ID, SABLE_FACT_ID) == 1 \
		and _count_event("memory.added", SABLE_ACTOR_ID, SABLE_MEMORY_ID) == 1 \
		and _count_event("memory.removed", SABLE_ACTOR_ID, SABLE_MEMORY_ID) == 1 \
		and _count_event("memory.removed", MALET_ACTOR_ID, MALET_MEMORY_ID) == 1 \
		and _count_event("memory.restored", SABLE_ACTOR_ID, SABLE_MEMORY_ID) == 1 \
		and _count_event("memory.restored", MALET_ACTOR_ID, MALET_MEMORY_ID) == 1,
		"Combined gameplay emitted missing or duplicate mutation events")
	_expect(_events.size() == 8,
		"Expected eight deterministic events, got %d" % _events.size())

	if EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	if DialogueManager.dialogue_line.is_connected(_on_dialogue_line):
		DialogueManager.dialogue_line.disconnect(_on_dialogue_line)
	if DialogueManager.dialogue_choice.is_connected(_on_dialogue_choice):
		DialogueManager.dialogue_choice.disconnect(_on_dialogue_choice)
	WorldState.reset_to_defaults()
	GameManager.import_data(game_before)
	GameManager.change_state(game_state_before)
	StoryLog.suppress_persistence = true
	_runner.finish(get_tree(),
		"actor=npc.sable combinations=4 consequence=choices_and_twelve_name_monument " +
		"round_trip=1 events_once=8 story_flags_delta=0 memory_manager_delta=0 " +
		"production_slots=0")


func _function_body(source: String, signature: String) -> String:
	var start := source.find(signature)
	if start < 0:
		return ""
	var next_function := source.find("\nfunc ", start + signature.length())
	return source.substr(start) if next_function < 0 \
		else source.substr(start, next_function - start)


func _read_source(path: String) -> String:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return ""
	var content := file.get_as_text()
	file.close()
	return content


func _file_fingerprint(path: String) -> String:
	if not FileAccess.file_exists(path):
		return "<missing>"
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return "<unreadable>"
	var content := file.get_as_text()
	file.close()
	return str(hash(content))


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
