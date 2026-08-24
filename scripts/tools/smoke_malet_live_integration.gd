extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const VERDAN_MARKET_SCENE = preload("res://scenes/maps/verdan_market.tscn")
const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.bl07.route_request_received"
const MEMORY_ID: String = "memory.malet.bl07_request_source"
const TEST_SAVE_SLOT: int = 2

var _runner = SmokeTestRunner.new(
	"malet_live_integration", "MALET_LIVE_INTEGRATION_SMOKE_PASS")
var _events: Array[Dictionary] = []
var _last_line: Dictionary = {}
var _last_choices: Array = []


func _ready() -> void:
	Codex.suppress_recording = true
	# Synthetic dialogue may update the in-process read registry, but it must
	# never be persisted to the player's StoryLog file when this process exits.
	StoryLog.suppress_persistence = true
	var story_registry_before := _file_fingerprint(StoryLog.READ_REGISTRY_PATH)
	var game_before := GameManager.export_data()
	var game_state_before := GameManager.current_state
	var legacy_memory_before := MemoryManager.export_data()

	if not SmokeSaveSandbox.activate("malet_live_integration", _runner):
		_runner.finish(get_tree())
		return

	EventBus.world_event_committed.connect(_on_world_event_committed)
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	DialogueManager.dialogue_choice.connect(_on_dialogue_choice)
	GameManager.current_locale = "en"
	GameManager.current_chapter = 3
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	WorldState.reset_to_defaults()
	GameManager.set_flag("ch2_malet_done")
	GameManager.set_flag("ch2_complete")
	GameManager.set_flag("talked_Malet_malet_encounter")
	var progression_flags := GameManager.story_flags.duplicate(true)

	var live_map = VERDAN_MARKET_SCENE.instantiate()
	var malet = live_map.get_node_or_null("Malet")
	_check_live_scene_contract(live_map, malet)
	_check_lifecycle_seed(live_map)
	if malet != null:
		live_map.remove_child(malet)
	live_map.free()
	if malet == null:
		_finish(game_before, game_state_before, legacy_memory_before,
			progression_flags, story_registry_before)
		return
	add_child(malet)
	_check_active_interaction_and_remove(malet)
	_check_removed_interaction_and_round_trip(malet)
	_check_restore_and_recovered_consequence(malet)
	_check_canon_callback_boundary()
	_check_mutation_boundaries()

	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	malet.free()
	_finish(game_before, game_state_before, legacy_memory_before,
		progression_flags, story_registry_before)


func _check_live_scene_contract(live_map: Node, malet: Node) -> void:
	_runner.begin_test("actual_verdan_malet_scene_contract")
	_expect(ActorRegistry.has_actor(ACTOR_ID),
		"actors.json does not contain the live Malet actor ID")
	_expect(malet != null, "Verdan Market scene does not contain its Malet NPC")
	if malet == null:
		return
	_expect(String(malet.get("npc_name")) == "Malet",
		"Live NPC display name changed unexpectedly")
	_expect(String(malet.get("dialogue_file")) == "res://data/chapter2_dialogue.json",
		"Live Malet must use the authored chapter 2 dialogue file")
	_expect(String(malet.get("dialogue_key")) == "malet_encounter",
		"The legacy first Malet encounter must remain configured")
	_expect(String(malet.get("repeat_dialogue_key")) == "malet_memory_world_followup",
		"Live Malet repeat interaction is not connected to the structured dialogue")
	var map_source := _read_source("res://scenes/maps/verdan_market.gd")
	_expect(map_source.count("_seed_malet_memory_world_state_if_needed()") >= 2,
		"Malet seed is not connected to both map load and deal completion")


func _check_lifecycle_seed(live_map: Node) -> void:
	_runner.begin_test("first_needed_lifecycle_seed")
	_expect(not MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Live Malet fact must not exist before the progression seed")
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"Live Malet source memory must not exist before the progression seed")
	var before := _events.size()
	live_map.call("_seed_malet_memory_world_state_if_needed")
	_expect(_events.size() == before + 2,
		"First lifecycle seed must commit one fact and one memory mutation")
	_expect(_count_event("knowledge.learned") == 1,
		"Lifecycle seed must learn the route fact exactly once")
	_expect(_count_event("memory.added") == 1,
		"Lifecycle seed must add the source memory exactly once")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Malet did not retain that a BL-07 route request occurred")
	var record := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(String(record.get("status", "")) == WorldState.MEMORY_STATUS_ACTIVE,
		"Seeded source memory is not active")
	_expect(String(record.get("source_actor_id", "")) == "player.arrel",
		"Seeded source memory must identify Arrel as the source actor")

	before = _events.size()
	live_map.call("_seed_malet_memory_world_state_if_needed")
	_expect(_events.size() == before,
		"Scene re-entry seed emitted events for existing state")


func _check_canon_callback_boundary() -> void:
	_runner.begin_test("malet_callback_matches_latest_chapter21_canon")
	var chapter6_source := _read_source("res://data/chapter6_dialogue.json")
	var seam_scene_source := _read_source("res://scenes/maps/the_seam.tscn")
	_expect(not chapter6_source.contains("sable_malet_route_followup") \
		and not seam_scene_source.contains("sable_malet_route_followup"),
		"The non-canon Malet note read by Sable is still live")
	var verdan_return_source := _read_source("res://data/vn_scenes/ch12_reader.json")
	_expect(verdan_return_source.contains("Malet's desk was gone") \
		and verdan_return_source.contains("Malet knew the exits"),
		"The existing Verdan return no longer preserves Malet's later callback")
	# The latest manuscript places the full Malet-record reveal in Chapter 21.
	# The current compressed VN has only the setup; a WorldState-conditioned
	# payoff belongs to the eventual Chapter 21 migration, not to Sable.


func _check_active_interaction_and_remove(malet: Node) -> void:
	_runner.begin_test("active_dialogue_and_information_option")
	_start_live_interaction(malet, "active")
	_advance_to_root_choices()
	var source_index := _choice_index("source_detail")
	_expect(source_index >= 0,
		"Active source memory must unlock Malet's extra information option")
	_expect(_choice_index("restore_source_memory") < 0,
		"Restore option must not appear while the source memory is active")
	if source_index >= 0:
		DialogueManager.select_choice(source_index)
		_expect(String(_last_line.get("branch_id", "")) == "source_detail",
			"Unlocked information option did not reach its authored detail")
		DialogueManager.advance()
		var back_index := _choice_index("back_from_source_detail")
		_expect(back_index >= 0, "Source detail has no return choice")
		if back_index >= 0:
			DialogueManager.select_choice(back_index)

	_runner.begin_test("player_choice_removes_source_memory")
	var remove_index := _choice_index("remove_source_memory")
	_expect(remove_index >= 0, "Active interaction has no source-memory removal action")
	var before := _events.size()
	if remove_index >= 0:
		DialogueManager.select_choice(remove_index)
	_expect(_events.size() == before + 1 and _count_event("memory.removed") == 1,
		"Player removal choice must emit exactly one memory.removed event")
	_expect(String(_last_line.get("branch_id", "")) == "removed_consequence",
		"Removal choice did not display its immediate gameplay feedback")
	_expect(not MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Removal choice left the source memory active")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Removing the source memory also removed Malet's route knowledge")
	before = _events.size()
	_expect(not MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID),
		"Removing an existing tombstone must be a no-op")
	_expect(_events.size() == before, "No-op removal emitted a duplicate event")


func _check_removed_interaction_and_round_trip(malet: Node) -> void:
	_runner.begin_test("removed_dialogue_changes_gameplay_option")
	_start_live_interaction(malet, "removed")
	_advance_to_root_choices()
	_expect(_choice_index("source_detail") < 0,
		"Removed source memory must hide the source-specific information option")
	_expect(_choice_index("remove_source_memory") < 0,
		"Removed source memory must hide the removal action")
	_expect(_choice_index("restore_source_memory") >= 0,
		"Removed source memory must expose the restoration action")
	DialogueManager.end_dialogue()

	_runner.begin_test("sandbox_save_reset_load_keeps_removed_branch")
	var save_path := SmokeSaveSandbox.get_slot_path(TEST_SAVE_SLOT, _runner)
	_expect(save_path != "", "Live integration save slot did not resolve in the sandbox")
	var expected_world_state := WorldState.export_data()
	var event_count_before := _events.size()
	_expect(SaveManager.save_game(TEST_SAVE_SLOT),
		"Live integration could not save through the real SaveManager contract")
	WorldState.reset_to_defaults()
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"WorldState reset did not clear the live test record")
	_expect(SaveManager.reload_test_world_state(TEST_SAVE_SLOT),
		"Sandbox load could not restore the live WorldState")
	_expect(WorldState.export_data() == expected_world_state,
		"Save -> reset -> load did not restore identical live WorldState")
	_expect(_events.size() == event_count_before,
		"Save/load replayed committed world events")
	_start_live_interaction(malet, "removed")
	_advance_to_root_choices()
	_expect(_choice_index("source_detail") < 0,
		"Loaded removed state incorrectly restored the source information option")
	_expect(_choice_index("restore_source_memory") >= 0,
		"Loaded removed state lost the restoration option")


func _check_restore_and_recovered_consequence(malet: Node) -> void:
	_runner.begin_test("player_choice_restores_source_memory")
	var restore_index := _choice_index("restore_source_memory")
	_expect(restore_index >= 0, "Removed interaction has no restoration action")
	var before := _events.size()
	if restore_index >= 0:
		DialogueManager.select_choice(restore_index)
	_expect(_events.size() == before + 1 and _count_event("memory.restored") == 1,
		"Player restoration choice must emit exactly one memory.restored event")
	_expect(String(_last_line.get("branch_id", "")) == "restored_consequence",
		"Restoration choice did not display its immediate gameplay feedback")
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Restoration choice did not reactivate the source memory")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Route knowledge changed during source-memory restoration")
	before = _events.size()
	_expect(not MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID),
		"Restoring an active memory must be a no-op")
	_expect(_events.size() == before, "No-op restoration emitted a duplicate event")

	_runner.begin_test("restored_dialogue_recovers_information_option")
	_start_live_interaction(malet, "restored")
	_advance_to_root_choices()
	_expect(_choice_index("source_detail") >= 0,
		"Restored source memory did not recover the extra information option")
	_expect(_choice_index("remove_source_memory") >= 0,
		"Restored source memory did not recover the removal action")
	_expect(_choice_index("restore_source_memory") < 0,
		"Restore option remained visible after restoration")


func _check_mutation_boundaries() -> void:
	_runner.begin_test("production_mutation_boundaries")
	var map_source := _read_source("res://scenes/maps/verdan_market.gd")
	var seed_body := _function_body(map_source,
		"func _seed_malet_memory_world_state_if_needed() -> void:")
	_expect(seed_body.contains("MemoryEngine.learn_fact") \
		and seed_body.contains("MemoryEngine.add_memory"),
		"Live seed does not use the MemoryEngine API")
	_expect(not seed_body.contains("WorldState._store_") \
		and not seed_body.contains("WorldState.import_data") \
		and not seed_body.contains("GameManager.story_flags"),
		"Live seed contains a forbidden direct persistent-state write")

	var manager_source := _read_source("res://scripts/systems/dialogue_manager.gd")
	var mutation_body := _function_body(manager_source,
		"func _apply_memory_world_choice_effects(choice: Dictionary) -> void:")
	_expect(mutation_body.contains("MemoryEngine.remove_memory") \
		and mutation_body.contains("MemoryEngine.restore_memory"),
		"Dialogue choices are not routed through MemoryEngine")
	_expect(not mutation_body.contains("WorldState.") \
		and not mutation_body.contains("GameManager.story_flags") \
		and not mutation_body.contains("MemoryManager."),
		"Live dialogue mutation bypasses its permitted API boundary")


func _start_live_interaction(malet: Node, expected_branch: String) -> void:
	_start_npc_interaction(malet, "Malet", expected_branch)


func _start_npc_interaction(npc: Node, expected_speaker: String,
		expected_branch: String) -> void:
	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	_last_line = {}
	_last_choices = []
	npc.call("interact")
	_expect(DialogueManager.is_active,
		"Live %s interaction did not start DialogueManager" % expected_speaker)
	_expect(String(_last_line.get("speaker", "")) == expected_speaker,
		"Live interaction did not emit a %s dialogue line" % expected_speaker)
	_expect(String(_last_line.get("branch_id", "")) == expected_branch,
		"Live interaction selected %s instead of %s" % [
			String(_last_line.get("branch_id", "<none>")), expected_branch,
		])


func _advance_to_root_choices() -> void:
	_last_choices = []
	DialogueManager.advance()
	_expect(not _last_choices.is_empty(),
		"Live Malet dialogue did not reach its conditional interaction options")


func _choice_index(choice_id: String) -> int:
	for index in range(_last_choices.size()):
		var choice: Variant = _last_choices[index]
		if choice is Dictionary and String(choice.get("choice_id", "")) == choice_id:
			return index
	return -1


func _count_event(event_type: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type:
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
	_runner.begin_test("legacy_and_persistent_data_isolation")
	_expect(GameManager.story_flags == progression_flags,
		"Live Memory World interaction changed legacy story_flags")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Live Memory World interaction changed the legacy MemoryManager")
	_expect(_file_fingerprint(StoryLog.READ_REGISTRY_PATH) == story_registry_before,
		"Live smoke changed the player's persistent StoryLog/read_lines file")
	_expect(SaveManager.is_smoke_test_mode() \
		and SaveManager.is_test_save_root_configured(),
		"Live smoke did not retain the production save-path guard")
	_expect(_count_event("knowledge.learned") == 1 \
		and _count_event("memory.added") == 1 \
		and _count_event("memory.removed") == 1 \
		and _count_event("memory.restored") == 1,
		"Live integration emitted missing or duplicate mutation events")

	if EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	if DialogueManager.dialogue_line.is_connected(_on_dialogue_line):
		DialogueManager.dialogue_line.disconnect(_on_dialogue_line)
	if DialogueManager.dialogue_choice.is_connected(_on_dialogue_choice):
		DialogueManager.dialogue_choice.disconnect(_on_dialogue_choice)
	WorldState.reset_to_defaults()
	GameManager.import_data(game_before)
	GameManager.change_state(game_state_before)
	# Keep suppression enabled through SceneTree shutdown; StoryLog._exit_tree()
	# therefore cannot flush synthetic read keys after the PASS marker.
	StoryLog.suppress_persistence = true
	_runner.finish(get_tree(),
		"scene=verdan_market actor=npc.malet branches=active_removed_restored " +
		"consequence=source_detail_toggle canon=malet_record_ch21 round_trip=1 events_once=4 " +
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
