extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const SEAM_OUTSKIRTS_SCENE = preload("res://scenes/maps/seam_outskirts.tscn")
const CHAPTER7_DIALOGUE_PATH := "res://data/chapter7_dialogue.json"
const ACTOR_ID := "npc.sable"
const FACT_ID := "fact.bl07.seventeen_seekers_never_returned"
const MEMORY_ID := "memory.sable.vessa_daughter_bond"
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
	GameManager.set_flag("ch7_sable_truth")
	GameManager.set_flag("talked_Sable_sable_confession")
	var progression_flags := GameManager.story_flags.duplicate(true)

	var outskirts_map := SEAM_OUTSKIRTS_SCENE.instantiate()
	var sable := outskirts_map.get_node_or_null("Sable")
	_check_live_scene_contract(outskirts_map, sable)
	_check_sable_seed(outskirts_map)
	if sable != null:
		outskirts_map.remove_child(sable)
	outskirts_map.free()
	if sable == null:
		_finish(game_before, game_state_before, legacy_memory_before,
			progression_flags, story_registry_before)
		return
	add_child(sable)

	_check_active_consequence(sable)
	_remove_vessa_through_live_dialogue(sable)
	_check_removed_consequence(sable)
	_check_round_trip_keeps_removed(sable)
	_restore_vessa_through_live_dialogue(sable)
	_check_restored_consequence(sable)
	_check_no_op_mutations()
	_check_mutation_boundaries()

	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	sable.free()
	_finish(game_before, game_state_before, legacy_memory_before,
		progression_flags, story_registry_before)


func _check_live_scene_contract(outskirts_map: Node, sable: Node) -> void:
	_runner.begin_test("actual_chapter7_sable_canon_contract")
	_expect(ActorRegistry.has_actor(ACTOR_ID),
		"actors.json does not contain npc.sable")
	_expect(outskirts_map != null, "Seam Outskirts scene could not be instantiated")
	_expect(sable != null, "Seam Outskirts scene does not contain Sable")
	if sable != null:
		_expect(String(sable.get("dialogue_file")) == CHAPTER7_DIALOGUE_PATH,
			"Live Sable does not use the Chapter 7 dialogue file")
		_expect(String(sable.get("dialogue_key")) == "sable_confession",
			"Live Sable's first interaction no longer points to the canon scene")
		_expect(String(sable.get("repeat_dialogue_key")) \
			== "sable_seeker_memory_followup",
			"Live Sable repeat interaction is not connected to the seeker memory")

	_expect(DialogueManager.load_dialogue_file(CHAPTER7_DIALOGUE_PATH),
		"Chapter 7 dialogue file could not be loaded")
	var dialogues: Dictionary = DialogueManager.loaded_dialogues.get(
		CHAPTER7_DIALOGUE_PATH, {})
	for key in ["outskirts_arrival", "sable_truth", "sable_trial",
			"trial_complete", "sable_confession", "sable_seeker_memory_followup"]:
		_expect(dialogues.has(key), "Chapter 7 dialogue is missing %s" % key)
	_expect(not dialogues.has("sable_cleaner_memory_followup") \
		and not dialogues.has("outskirts_monument"),
		"Non-canon Cleaner or twelve-name memorial content is still reachable")


func _check_sable_seed(outskirts_map: Node) -> void:
	_runner.begin_test("sable_seventeen_fact_and_vessa_memory_seed")
	_expect(not MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Sable fact existed before the Chapter 7 lifecycle seed")
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"Sable Vessa memory existed before the Chapter 7 lifecycle seed")
	var before := _events.size()
	outskirts_map.call("_seed_sable_seeker_state_if_needed")
	_expect(_events.size() == before + 2,
		"Sable seed must commit one fact and one memory event")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Sable did not learn that the seventeen seekers never returned")
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Sable's Vessa relationship memory is not active")
	var record := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	var content: Dictionary = record.get("content", {})
	_expect(String(record.get("source_actor_id", "")) == ACTOR_ID,
		"Sable must be the source actor for her relationship memory")
	_expect(String(content.get("kind", "")) == "personal_relationship" \
		and String(content.get("subject", "")) == "vessa" \
		and String(content.get("relationship", "")) == "daughter",
		"Vessa memory record does not encode the canon personal relationship")
	before = _events.size()
	outskirts_map.call("_seed_sable_seeker_state_if_needed")
	_expect(_events.size() == before,
		"Scene re-entry seed emitted duplicate events")


func _check_active_consequence(sable: Node) -> void:
	_runner.begin_test("active_vessa_dialogue_and_personal_question")
	var before := _events.size()
	_start_sable_interaction(sable, "sable_vessa_active")
	_advance_to_root_choices()
	_expect(_choice_index("ask_vessa_detail") >= 0,
		"Active Vessa memory did not unlock the personal question")
	_expect(_choice_index("ask_seventeen_record") < 0,
		"Active Vessa memory exposed the provenance-gap question")
	var detail_index := _choice_index("ask_vessa_detail")
	if detail_index >= 0:
		DialogueManager.select_choice(detail_index)
		_expect(String(_last_line.get("branch_id", "")) == "vessa_personal_detail",
			"Active personal question selected the wrong branch")
	_expect(_events.size() == before,
		"Read-only active consequence emitted a world event")
	DialogueManager.end_dialogue()


func _remove_vessa_through_live_dialogue(sable: Node) -> void:
	_runner.begin_test("live_sable_choice_removes_vessa_bond")
	_start_sable_interaction(sable, "sable_vessa_active")
	_advance_to_root_choices()
	var choice_index := _choice_index("remove_sable_vessa_memory")
	_expect(choice_index >= 0, "Live Sable dialogue has no Vessa removal action")
	var before := _events.size()
	if choice_index >= 0:
		DialogueManager.select_choice(choice_index)
	_expect(_events.size() == before + 1,
		"Vessa removal must emit exactly one event")
	_expect(_count_event("memory.removed", MEMORY_ID) == 1,
		"Vessa removal event was missing or duplicated")
	_expect(String(_last_line.get("branch_id", "")) \
		== "sable_vessa_removed_feedback",
		"Vessa removal did not show its authored feedback")
	_expect(not MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Vessa relationship memory remains active after removal")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Removing Vessa's relationship erased the seventeen-seeker fact")
	DialogueManager.end_dialogue()


func _check_removed_consequence(sable: Node) -> void:
	_runner.begin_test("removed_vessa_dialogue_and_record_question")
	var before := _events.size()
	_start_sable_interaction(sable, "sable_vessa_removed")
	_advance_to_root_choices()
	_expect(_choice_index("ask_seventeen_record") >= 0,
		"Removed Vessa memory did not unlock the retained-fact question")
	_expect(_choice_index("ask_vessa_detail") < 0,
		"Removed Vessa memory still exposed the personal question")
	var record_index := _choice_index("ask_seventeen_record")
	if record_index >= 0:
		DialogueManager.select_choice(record_index)
		_expect(String(_last_line.get("branch_id", "")) \
			== "seventeen_record_detail",
			"Retained-fact question selected the wrong branch")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Sable lost the seeker knowledge while the memory was removed")
	_expect(_events.size() == before,
		"Read-only removed consequence emitted a world event")
	DialogueManager.end_dialogue()


func _check_round_trip_keeps_removed(sable: Node) -> void:
	_runner.begin_test("sandbox_round_trip_keeps_removed_vessa_branch")
	var save_path := SmokeSaveSandbox.get_slot_path(TEST_SAVE_SLOT, _runner)
	_expect(save_path != "", "Sable smoke slot did not resolve in the sandbox")
	var expected_state := WorldState.export_data()
	var before := _events.size()
	_expect(SaveManager.save_game(TEST_SAVE_SLOT),
		"Sable state could not save through SaveManager")
	WorldState.reset_to_defaults()
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"WorldState reset did not clear Sable's memory")
	_expect(SaveManager.reload_test_world_state(TEST_SAVE_SLOT),
		"Sandbox load could not restore Sable's state")
	_expect(WorldState.export_data() == expected_state,
		"Save -> reset -> load changed Sable's world state")
	_expect(_events.size() == before, "Save/load replayed world events")
	_check_removed_consequence(sable)


func _restore_vessa_through_live_dialogue(sable: Node) -> void:
	_runner.begin_test("live_sable_choice_restores_vessa_bond")
	_start_sable_interaction(sable, "sable_vessa_removed")
	_advance_to_root_choices()
	var choice_index := _choice_index("restore_sable_vessa_memory")
	_expect(choice_index >= 0, "Live Sable dialogue has no Vessa restoration action")
	var before := _events.size()
	if choice_index >= 0:
		DialogueManager.select_choice(choice_index)
	_expect(_events.size() == before + 1,
		"Vessa restoration must emit exactly one event")
	_expect(_count_event("memory.restored", MEMORY_ID) == 1,
		"Vessa restoration event was missing or duplicated")
	_expect(String(_last_line.get("branch_id", "")) \
		== "sable_vessa_restored_feedback",
		"Vessa restoration did not show its authored feedback")
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Vessa relationship memory was not restored")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Sable knowledge changed during restoration")
	DialogueManager.end_dialogue()


func _check_restored_consequence(sable: Node) -> void:
	_runner.begin_test("restored_vessa_dialogue_and_personal_question")
	var before := _events.size()
	_start_sable_interaction(sable, "sable_vessa_restored")
	_advance_to_root_choices()
	_expect(_choice_index("ask_vessa_detail") >= 0,
		"Restored Vessa memory did not recover the personal question")
	_expect(_choice_index("ask_seventeen_record") < 0,
		"Restored Vessa memory still exposed the provenance-gap question")
	_expect(_events.size() == before,
		"Read-only restored consequence emitted a world event")
	DialogueManager.end_dialogue()


func _check_no_op_mutations() -> void:
	_runner.begin_test("sable_no_op_mutations_emit_no_event")
	var before := _events.size()
	_expect(not MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID),
		"Restoring an already-active Vessa memory must be a no-op")
	_expect(_events.size() == before,
		"No-op restoration emitted a world event")


func _check_mutation_boundaries() -> void:
	_runner.begin_test("sable_canon_and_mutation_api_boundaries")
	var map_source := _read_source("res://scenes/maps/seam_outskirts.gd")
	var seed_body := _function_body(map_source,
		"func _seed_sable_seeker_state_if_needed() -> void:")
	_expect(seed_body.contains("MemoryEngine.learn_fact") \
		and seed_body.contains("MemoryEngine.add_memory"),
		"Sable lifecycle seed bypasses MemoryEngine")
	_expect(not seed_body.contains("WorldState._store_") \
		and not seed_body.contains("WorldState.import_data") \
		and not seed_body.contains("GameManager.story_flags"),
		"Sable lifecycle seed directly writes persistent state")
	var dialogue_source := _read_source(CHAPTER7_DIALOGUE_PATH)
	_expect(not dialogue_source.contains("child_memory_erasure_order") \
		and not dialogue_source.contains("child_memory_erasure_target") \
		and not dialogue_source.contains("sable_cleaner_memory_followup") \
		and not dialogue_source.contains("outskirts_monument") \
		and not dialogue_source.contains("twelve-name memorial"),
		"Chapter 7 still contains the superseded Cleaner/twelve-person canon")
	_expect(dialogue_source.contains("seventeen_seekers_never_returned") \
		and dialogue_source.contains("vessa_daughter_bond"),
		"Chapter 7 does not reference the canon fact/memory pair")
	var chapter6_source := _read_source("res://data/chapter6_dialogue.json")
	var seam_scene_source := _read_source("res://scenes/maps/the_seam.tscn")
	_expect(not chapter6_source.contains("sable_malet_route_followup") \
		and not seam_scene_source.contains("sable_malet_route_followup"),
		"The non-canon Malet -> Sable note ripple is still live")
	var effects_source := _read_source("res://scripts/systems/dialogue_manager.gd")
	var effects_body := _function_body(effects_source,
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
		"Sable follow-up did not reach its conditional choices")


func _choice_index(choice_id: String) -> int:
	for index in range(_last_choices.size()):
		var choice: Variant = _last_choices[index]
		if choice is Dictionary and String(choice.get("choice_id", "")) == choice_id:
			return index
	return -1


func _count_event(event_type: String, target_id: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type \
				and String(event.get("actor_id", "")) == ACTOR_ID \
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
	_expect(_count_event("knowledge.learned", FACT_ID) == 1 \
		and _count_event("memory.added", MEMORY_ID) == 1 \
		and _count_event("memory.removed", MEMORY_ID) == 1 \
		and _count_event("memory.restored", MEMORY_ID) == 1,
		"Sable gameplay emitted missing or duplicate mutation events")
	_expect(_events.size() == 4,
		"Expected four deterministic Sable events, got %d" % _events.size())

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
		"actor=npc.sable canon=vessa_seventeen branches=active_removed_restored " +
		"consequence=choice_and_lore_toggle round_trip=1 events_once=4 " +
		"story_flags_delta=0 memory_manager_delta=0 production_slots=0")


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
