extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const ClassifierEntry = preload("res://scenes/story/ch5_classifier_entry.gd")
const ENTRY_SCENE = preload("res://scenes/story/ch5_classifier_entry.tscn")
const MALET_ACTOR_ID := "npc.malet"
const KAIROS_ACTOR_ID := "npc.kairos"
const CANONICAL_FACT_ID := "fact.bl07.route_request_received"
const LEGACY_FACT_ID := "fact.arrel.seeks_bl07"
const MEMORY_ID := "memory.malet.bl07_request_source"
const IDENTIFIED_FACT_ID := "fact.kairos.malet_report_identified_arrel"
const UNKNOWN_FACT_ID := "fact.kairos.malet_report_requester_unknown"
const TEST_SAVE_SLOT := 2

var _runner = SmokeTestRunner.new("canon_wave2a", "CANON_WAVE2A_SMOKE_PASS")
var _events: Array[Dictionary] = []
var _achievement_stats_before: Dictionary = {}
var _achievement_unlocked_before: Dictionary = {}
var _achievement_file_before := ""


func _ready() -> void:
	Codex.suppress_recording = true
	StoryLog.suppress_persistence = true
	var story_registry_before := _file_fingerprint(StoryLog.READ_REGISTRY_PATH)
	var game_before := GameManager.export_data()
	var game_state_before := GameManager.current_state
	var world_before := WorldState.export_data()
	var flow_before := SceneFlow.export_data()
	var legacy_memory_before := MemoryManager.export_data()
	_prepare_achievement_persistence_guard()

	if not SmokeSaveSandbox.activate("canon_wave2a", _runner):
		_finish(game_before, game_state_before, world_before, flow_before,
			legacy_memory_before, story_registry_before)
		return

	EventBus.world_event_committed.connect(_on_world_event_committed)
	GameManager.current_locale = "en"
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	_check_route_and_content_contract()
	_check_legacy_fact_compatibility()
	_check_active_report()
	_check_removed_report()
	_check_restore_before_report()
	_check_restore_after_report_and_round_trip()
	await _check_kairos_presentation_load()
	_check_ch21_hook_contract()

	_finish(game_before, game_state_before, world_before, flow_before,
		legacy_memory_before, story_registry_before)


func _check_route_and_content_contract() -> void:
	_runner.begin_test("canonical_ch1_to_ch5_route")
	var ch1_exit := _read_source("res://data/vn_scenes/ch1_after_forest.json")
	var ch2_entry := _read_source("res://data/vn_scenes/ch2_market_arrival.json")
	var verdan := _read_source("res://scenes/maps/verdan_market.gd")
	var belt := _read_source("res://scenes/maps/belt_waystation.gd")
	var drift := _read_source("res://scenes/maps/drift_shelter.gd")
	var entry := _read_source("res://scenes/story/ch5_classifier_entry.gd")
	var vn_source := _read_source("res://data/vn_scenes/ch5_classifier.json")
	var pause_source := _read_source("res://scripts/ui/pause_menu.gd")
	var minimap_source := _read_source("res://scripts/ui/minimap.gd")
	_expect(ch1_exit.contains("\"id\": \"ch2_market_arrival\"") \
		and ch2_entry.contains("res://scenes/maps/verdan_market.tscn") \
		and verdan.contains("res://scenes/maps/belt_waystation.tscn") \
		and belt.contains("res://scenes/maps/drift_shelter.tscn") \
		and drift.contains("res://scenes/story/ch5_classifier_entry.tscn") \
		and entry.contains("VN_SCENE_ID: String = \"ch5_classifier\""),
		"New Game canonical route no longer reaches Chapter 5")
	_expect(not drift.contains("res://scenes/maps/crumbling_coast.tscn") \
		and not entry.contains("crumbling_coast") \
		and not vn_source.contains("crumbling_coast"),
		"Canonical Chapter 5 still exposes the superseded coast route")
	_expect(pause_source.contains("ch5_classifier_started") \
		and pause_source.contains("res://scenes/maps/crumbling_coast.tscn"),
		"Canonical Chapter 5 can still fast-travel into the legacy coast")
	_expect(ResourceLoader.exists("res://scenes/maps/crumbling_coast.tscn"),
		"Retired Chapter 5 content was deleted instead of preserved")
	_expect(not vn_source.contains("goto_battle") \
		and not entry.contains("BattleManager"),
		"The Classifier incorrectly introduces the legacy Kairos boss encounter")
	_expect(ActorRegistry.has_actor(KAIROS_ACTOR_ID) \
		and ActorRegistry.get_display_name(KAIROS_ACTOR_ID) == "Kairós",
		"Canonical Kairos actor identity is unavailable")
	_expect(vn_source.contains("Class Seven. Confirmed.") \
		and vn_source.contains("Withhold classification. Preserve the specimen.") \
		and vn_source.contains("Contact initiated. Patient approach."),
		"Chapter 5 no longer preserves the manuscript's Classifier beats")
	_expect(vn_source.contains("canon_ch6_seam_ready") \
		and vn_source.contains("res://scenes/maps/drift_shelter.tscn"),
		"Chapter 5 has no safe canonical Chapter 6 boundary")
	_expect(entry.contains("MemoryEngine.learn_fact") \
		and not entry.contains("WorldState._store_") \
		and not entry.contains("WorldState.import_data") \
		and not entry.contains("GameManager.story_flags"),
		"Chapter 5 bypasses the allowed persistent mutation boundary")
	_expect(entry.contains("SceneTransition.is_transition_in_progress()") \
		and entry.contains("await SceneTransition.transition_finished"),
		"Chapter 5 no longer waits for the Drift-to-entry transition to release")
	_expect(minimap_source.contains("layer.tree_exiting.connect") \
		and minimap_source.contains("GameManager.state_changed.disconnect"),
		"Chapter 5 scene changes can leave a freed minimap callback connected")


func _check_legacy_fact_compatibility() -> void:
	_runner.begin_test("old_malet_fact_additive_compatibility")
	_events.clear()
	var legacy := WorldState.make_default_data()
	legacy["revision"] = 7
	legacy["event_sequence"] = 5
	var malet: Dictionary = legacy["actors"][MALET_ACTOR_ID]
	malet["knowledge"][LEGACY_FACT_ID] = {
		"fact_id": LEGACY_FACT_ID,
		"value": true,
		"updated_revision": 4,
	}
	malet["memories"][MEMORY_ID] = {
		"id": MEMORY_ID,
		"owner_actor_id": MALET_ACTOR_ID,
		"status": WorldState.MEMORY_STATUS_REMOVED,
		"fact_ids": [LEGACY_FACT_ID],
		"source_actor_id": "player.arrel",
		"content": {"subject": "bl07_route_request"},
		"created_revision": 3,
		"removed_revision": 7,
	}
	_expect(WorldState.import_data(legacy), "Legacy WorldState snapshot was rejected")
	_expect(MemoryEngine.knows_fact(MALET_ACTOR_ID, LEGACY_FACT_ID),
		"Compatibility import deleted the legacy fact ID")
	_expect(MemoryEngine.knows_fact(MALET_ACTOR_ID, CANONICAL_FACT_ID),
		"Legacy fact did not converge to the canonical identity-free request fact")
	var record := WorldState.get_memory_record(MALET_ACTOR_ID, MEMORY_ID)
	var fact_ids: Array = record.get("fact_ids", [])
	_expect(LEGACY_FACT_ID in fact_ids and CANONICAL_FACT_ID in fact_ids,
		"Legacy memory record did not retain old and canonical fact references")
	_expect(String(record.get("status", "")) == WorldState.MEMORY_STATUS_REMOVED,
		"Compatibility import changed the historical memory tombstone")
	_expect(WorldState.get_revision() == 7 \
		and WorldState.get_event_sequence() == 5 \
		and _events.is_empty(),
		"Compatibility normalization allocated a revision or replayed an event")


func _check_active_report() -> void:
	_runner.begin_test("active_memory_identifies_report_requester")
	_reset_scenario()
	_seed_malet_source()
	var outcome := _enter_classifier_without_transition()
	_expect(outcome == ClassifierEntry.REPORT_IDENTIFIED,
		"Active source memory did not preserve Arrel in the report")
	_expect(MemoryEngine.knows_fact(KAIROS_ACTOR_ID, IDENTIFIED_FACT_ID) \
		and not MemoryEngine.knows_fact(KAIROS_ACTOR_ID, UNKNOWN_FACT_ID),
		"Active report committed the wrong historical Kairos fact")
	_expect(_events.size() == 3 \
		and _count_target_event("knowledge.learned", IDENTIFIED_FACT_ID) == 1,
		"Active report must commit exactly one historical outcome event")
	_check_boundary_consumed()


func _check_removed_report() -> void:
	_runner.begin_test("removed_memory_keeps_event_but_loses_identity")
	_reset_scenario()
	_seed_malet_source()
	_expect(MemoryEngine.remove_memory(MALET_ACTOR_ID, MEMORY_ID),
		"Could not remove Malet source memory before the report")
	var outcome := _enter_classifier_without_transition()
	_expect(outcome == ClassifierEntry.REPORT_UNKNOWN,
		"Removed source memory regenerated requester identity")
	_expect(MemoryEngine.knows_fact(MALET_ACTOR_ID, CANONICAL_FACT_ID),
		"Removing requester identity also removed the BL-07 request event")
	_expect(MemoryEngine.knows_fact(KAIROS_ACTOR_ID, UNKNOWN_FACT_ID) \
		and not MemoryEngine.knows_fact(KAIROS_ACTOR_ID, IDENTIFIED_FACT_ID),
		"Removed report committed the wrong historical Kairos fact")
	_expect(_events.size() == 4 \
		and _count_target_event("knowledge.learned", UNKNOWN_FACT_ID) == 1,
		"Removed report must commit exactly one historical outcome event")


func _check_restore_before_report() -> void:
	_runner.begin_test("restore_before_report_recovers_identity")
	_reset_scenario()
	_seed_malet_source()
	_expect(MemoryEngine.remove_memory(MALET_ACTOR_ID, MEMORY_ID),
		"Could not remove Malet source memory")
	_expect(MemoryEngine.restore_memory(MALET_ACTOR_ID, MEMORY_ID),
		"Could not restore Malet source memory before the report")
	var outcome := _enter_classifier_without_transition()
	_expect(outcome == ClassifierEntry.REPORT_IDENTIFIED,
		"Restoring before Chapter 5 did not recover the report identity")
	_expect(_events.size() == 5 \
		and _count_target_event("knowledge.learned", IDENTIFIED_FACT_ID) == 1,
		"Restore-before-report emitted a missing or duplicate outcome event")


func _check_restore_after_report_and_round_trip() -> void:
	_runner.begin_test("restore_after_report_is_not_retroactive")
	_reset_scenario()
	_seed_malet_source()
	MemoryEngine.remove_memory(MALET_ACTOR_ID, MEMORY_ID)
	var outcome := _enter_classifier_without_transition()
	_expect(outcome == ClassifierEntry.REPORT_UNKNOWN,
		"Removed report did not start from the anonymous outcome")
	var before_restore := _events.size()
	_expect(MemoryEngine.restore_memory(MALET_ACTOR_ID, MEMORY_ID),
		"Could not restore Malet source memory after the report")
	_expect(_events.size() == before_restore + 1 \
		and _count_target_event("memory.restored", MEMORY_ID) == 1,
		"Restore-after-report did not emit exactly one memory event")
	var before_resolve := _events.size()
	_expect(ClassifierEntry.resolve_malet_report_outcome() == ClassifierEntry.REPORT_UNKNOWN,
		"Restoring Malet's current memory rewrote the historical report")
	_expect(_events.size() == before_resolve,
		"Re-reading the decided historical report emitted a duplicate event")
	_expect(MemoryEngine.check_memory(MALET_ACTOR_ID, MEMORY_ID) \
		and MemoryEngine.knows_fact(KAIROS_ACTOR_ID, UNKNOWN_FACT_ID) \
		and not MemoryEngine.knows_fact(KAIROS_ACTOR_ID, IDENTIFIED_FACT_ID),
		"Current restored memory and historical anonymous report cannot coexist")

	_runner.begin_test("ch5_report_save_reset_load_round_trip")
	var save_path := SmokeSaveSandbox.get_slot_path(TEST_SAVE_SLOT, _runner)
	_expect(save_path != "", "Wave 2A save slot did not resolve in the sandbox")
	var expected_world := WorldState.export_data()
	var event_count_before := _events.size()
	_expect(SaveManager.save_game(TEST_SAVE_SLOT),
		"Wave 2A state could not save through SaveManager")
	WorldState.reset_to_defaults()
	_expect(SaveManager.reload_test_world_state(TEST_SAVE_SLOT),
		"Wave 2A sandbox load did not restore WorldState")
	_expect(WorldState.export_data() == expected_world,
		"Chapter 5 report outcome changed across save -> reset -> load")
	_expect(_events.size() == event_count_before,
		"Chapter 5 save/load replayed world events")
	_expect(ClassifierEntry.get_persistent_report_outcome() == ClassifierEntry.REPORT_UNKNOWN \
		and MemoryEngine.check_memory(MALET_ACTOR_ID, MEMORY_ID),
		"Loaded historical outcome did not remain independent from current memory")


func _check_kairos_presentation_load() -> void:
	_runner.begin_test("kairos_classifier_vn_load")
	# This smoke itself starts inside _ready(). Let the root finish attaching its
	# children before SceneFlow adds the existing CanvasLayer-based VN UI.
	await get_tree().process_frame
	SceneFlow.play("ch5_classifier")
	await get_tree().process_frame
	_expect(SceneFlow.is_active and SceneFlow.current_id == "ch5_classifier",
		"Existing SceneFlow could not load the canonical Kairos presentation")
	var has_authority_edit := false
	var has_identified_branch := false
	var has_unknown_branch := false
	var has_ch6_boundary := false
	for step_value in SceneFlow.current_steps:
		if not (step_value is Dictionary):
			continue
		var step: Dictionary = step_value
		has_authority_edit = has_authority_edit \
			or String(step.get("cg", "")).ends_with("cinematic_kairos_authority_edit.png")
		has_identified_branch = has_identified_branch \
			or String(step.get("requires_flag", "")) == "ch5_malet_report_identified_arrel"
		has_unknown_branch = has_unknown_branch \
			or String(step.get("requires_flag", "")) == "ch5_malet_report_requester_unknown"
		has_ch6_boundary = has_ch6_boundary \
			or String(step.get("set_flag", "")) == "canon_ch6_seam_ready"
	_expect(has_authority_edit and has_identified_branch and has_unknown_branch,
		"Kairos VN did not load its canonical art or both Malet report variants")
	_expect(has_ch6_boundary, "Kairos VN did not load its Chapter 6 handoff")

	_runner.begin_test("classifier_completion_autosave_and_ch6_boundary")
	# Execute the authored final action through SceneFlow. The test-only copy
	# clears its destination so no scene replacement can remove this smoke node;
	# the shipped JSON path was already asserted in the route contract.
	var test_steps := SceneFlow.current_steps.duplicate(true)
	var final_index := test_steps.size() - 1
	var final_step: Dictionary = test_steps[final_index].duplicate(true)
	final_step["path"] = ""
	test_steps[final_index] = final_step
	SceneFlow.current_steps = test_steps
	SceneFlow.current_index = final_index
	var expected_world := WorldState.export_data()
	var event_count_before := _events.size()
	# SaveManager intentionally suppresses every autosave while Codex marks a
	# scene synthetic. The injected root is already guarded, so open only this
	# synchronous call and restore suppression before another frame can run.
	Codex.suppress_recording = false
	SceneFlow.call("_run_step")
	Codex.suppress_recording = true
	await get_tree().process_frame
	_expect(GameManager.get_flag("canon_ch6_seam_ready") \
		and GameManager.current_chapter == 5 \
		and not SceneFlow.is_active,
		"Chapter 5 completion did not reach the saveable Chapter 6 boundary")
	var autosave_path := SmokeSaveSandbox.get_slot_path(
		SaveManager.AUTOSAVE_SLOT, _runner)
	_expect(autosave_path != "" and FileAccess.file_exists(autosave_path),
		"Chapter 5 completion did not create its sandbox autosave")
	WorldState.reset_to_defaults()
	_expect(SaveManager.reload_test_world_state(SaveManager.AUTOSAVE_SLOT),
		"Chapter 5 completion autosave could not restore WorldState")
	_expect(WorldState.export_data() == expected_world,
		"Chapter 5 completion autosave lost the historical report outcome")
	_expect(_events.size() == event_count_before,
		"Chapter 5 completion autosave/load replayed world events")
	SceneFlow.import_data({})


func _check_ch21_hook_contract() -> void:
	_runner.begin_test("chapter21_future_hook")
	var memory_map := _read_source("res://docs/SEASON1_MEMORY_MAP.md")
	var progression := _read_source("res://docs/SEASON1_GAME_PROGRESSION.md")
	_expect(memory_map.contains(IDENTIFIED_FACT_ID) \
		and memory_map.contains(UNKNOWN_FACT_ID) \
		and (memory_map.contains("Chapter 21") or memory_map.contains("Ch21")),
		"Memory map does not expose the historical report facts to Chapter 21")
	_expect(progression.contains("canon_ch6_seam_ready") \
		and progression.contains("ch5_classifier"),
		"Progression contract does not document the live Chapter 5 and Chapter 6 boundary")
	_expect(not _read_source("res://data/vn_scenes/ch21_editors_turn.json").contains(
		"ch5_malet_report_identified_arrel"),
		"Wave 2A added placeholder Chapter 21 dialogue instead of a future hook")


func _reset_scenario() -> void:
	WorldState.reset_to_defaults()
	_events.clear()
	GameManager.current_chapter = 4
	for flag_name in [
		"canon_ch5_classifier_ready",
		"ch5_classifier_started",
		"ch5_kairos_seen",
		"ch5_malet_report_identified_arrel",
		"ch5_malet_report_requester_unknown",
		"canon_ch6_seam_ready",
	]:
		GameManager.set_flag(flag_name, false)


func _seed_malet_source() -> void:
	_expect(MemoryEngine.learn_fact(MALET_ACTOR_ID, CANONICAL_FACT_ID),
		"Could not seed canonical Malet request knowledge")
	_expect(MemoryEngine.add_memory(MALET_ACTOR_ID, MEMORY_ID, {
		"fact_ids": [CANONICAL_FACT_ID],
		"source_actor_id": "player.arrel",
		"content": {"subject": "bl07_route_request"},
	}), "Could not seed Malet requester-source memory")


func _enter_classifier_without_transition() -> String:
	GameManager.set_flag("canon_ch5_classifier_ready")
	var entry := ENTRY_SCENE.instantiate()
	entry.set("launch_vn_on_ready", false)
	entry.set("autosave_on_ready", false)
	add_child(entry)
	var outcome := ClassifierEntry.get_persistent_report_outcome()
	entry.free()
	return outcome


func _check_boundary_consumed() -> void:
	_expect(not GameManager.get_flag("canon_ch5_classifier_ready") \
		and GameManager.get_flag("ch5_classifier_started") \
		and GameManager.current_chapter == 5,
		"Classifier entry did not consume the Chapter 4 boundary")


func _count_target_event(event_type: String, target_id: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type \
				and String(event.get("target_id", "")) == target_id:
			count += 1
	return count


func _finish(game_before: Dictionary, game_state_before: int,
		world_before: Dictionary, flow_before: Dictionary,
		legacy_memory_before: Dictionary, story_registry_before: String) -> void:
	_runner.begin_test("persistent_and_legacy_system_isolation")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Chapter 5 migration changed the legacy MemoryManager")
	_expect(_file_fingerprint(StoryLog.READ_REGISTRY_PATH) == story_registry_before,
		"Chapter 5 smoke changed the player's StoryLog/read registry")
	_expect(_file_fingerprint(AchievementManager.SAVE_PATH) == _achievement_file_before,
		"Chapter 5 smoke changed the player's persistent achievement data")
	_expect(SaveManager.is_smoke_test_mode() \
		and SaveManager.is_test_save_root_configured(),
		"Chapter 5 smoke lost the production save-path guard")

	if EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	AchievementManager.stats = _achievement_stats_before.duplicate(true)
	AchievementManager.unlocked = _achievement_unlocked_before.duplicate(true)
	SceneFlow.import_data(flow_before)
	WorldState.import_data(world_before)
	GameManager.import_data(game_before)
	GameManager.change_state(game_state_before)
	StoryLog.suppress_persistence = true
	_runner.finish(get_tree(),
		"route=canon_ch1_to_ch5 presentation=kairos_classifier " +
		"reports=active_removed_restore_before_restore_after historical=stable " +
		"old_fact=compatible ch21_hook=ready ch6_boundary=saveable " +
		"event_replay=0 story_registry_delta=0 memory_manager_delta=0 " +
		"production_slots=0")


func _on_world_event_committed(event: Dictionary) -> void:
	_events.append(event.duplicate(true))


func _prepare_achievement_persistence_guard() -> void:
	_achievement_stats_before = AchievementManager.stats.duplicate(true)
	_achievement_unlocked_before = AchievementManager.unlocked.duplicate(true)
	_achievement_file_before = _file_fingerprint(AchievementManager.SAVE_PATH)
	# record_chapter_complete() becomes a no-op and therefore cannot write the
	# real achievement file while the final VN action is exercised.
	AchievementManager.unlocked["chapter_complete_5"] = true


func _read_source(path: String) -> String:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return ""
	var content := file.get_as_text()
	file.close()
	return content


func _file_fingerprint(path: String) -> String:
	if not FileAccess.file_exists(path):
		return "missing"
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return "unreadable"
	var content := file.get_as_text()
	file.close()
	return str(hash(content))


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
