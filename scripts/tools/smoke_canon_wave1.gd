extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const VERDAN_SCENE = preload("res://scenes/maps/verdan_market.tscn")
const BELT_SCENE = preload("res://scenes/maps/belt_waystation.tscn")
const DRIFT_SCENE = preload("res://scenes/maps/drift_shelter.tscn")
const ACTOR_ID := "npc.malet"
const FACT_ID := "fact.arrel.seeks_bl07"
const MEMORY_ID := "memory.malet.bl07_request_source"
const TEST_SAVE_SLOT := 2

var _runner = SmokeTestRunner.new("canon_wave1", "CANON_WAVE1_SMOKE_PASS")
var _events: Array[Dictionary] = []
var _loaded_maps: Array[Node] = []
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
	var legacy_memory_before := MemoryManager.export_data()
	_prepare_achievement_persistence_guard()

	if not SmokeSaveSandbox.activate("canon_wave1", _runner):
		_finish(game_before, game_state_before, world_before,
			legacy_memory_before, story_registry_before)
		return

	EventBus.world_event_committed.connect(_on_world_event_committed)
	GameManager.current_locale = "en"
	GameManager.current_chapter = 3
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	WorldState.reset_to_defaults()

	_check_authored_route_contract()
	_seed_and_check_malet_world_state()
	await _check_verdan_scene_and_malet_state()
	await _check_belt_scene_and_legacy_save_compatibility()
	await _check_drift_scene_and_boundary()
	_check_sandbox_round_trip()
	_check_runtime_assets_and_data()

	_finish(game_before, game_state_before, world_before,
		legacy_memory_before, story_registry_before)


func _check_authored_route_contract() -> void:
	_runner.begin_test("new_game_chapter_route")
	var title_source := _read_source("res://scenes/main/main.gd")
	var chapter1_exit := _read_source("res://data/vn_scenes/ch1_after_forest.json")
	var chapter2_entry := _read_source("res://data/vn_scenes/ch2_market_arrival.json")
	var verdan_source := _read_source("res://scenes/maps/verdan_market.gd")
	var belt_source := _read_source("res://scenes/maps/belt_waystation.gd")
	var drift_source := _read_source("res://scenes/maps/drift_shelter.gd")
	_expect(title_source.contains("SceneFlow.pending_scene_id = \"ch1_cold_open\"") \
		and title_source.contains("res://scenes/main/vn_host.tscn"),
		"New Game no longer enters the canonical Chapter 1 VN route")
	_expect(chapter1_exit.contains("\"set_chapter\": 2") \
		and chapter1_exit.contains("\"id\": \"ch2_market_arrival\""),
		"Canonical Chapter 1 no longer transitions into Chapter 2")
	_expect(chapter2_entry.contains("\"action\": \"goto_map\"") \
		and chapter2_entry.contains("res://scenes/maps/verdan_market.tscn"),
		"Chapter 2 arrival VN no longer opens the playable Verdan map")
	_expect(verdan_source.contains("res://scenes/maps/belt_waystation.tscn"),
		"Chapter 2 no longer routes Malet completion to canonical Chapter 3")
	_expect(belt_source.contains("res://scenes/maps/drift_shelter.tscn"),
		"Canonical Chapter 3 no longer routes to Chapter 4")
	_expect(belt_source.contains("Vector2(23.5 * TILE_SIZE, 9 * TILE_SIZE)") \
		and drift_source.contains("Vector2(23.5 * TILE_SIZE, 9 * TILE_SIZE)"),
		"Canonical Chapter 3/4 exits no longer follow the eastbound route")
	_expect(not drift_source.contains("res://scenes/maps/crumbling_coast.tscn"),
		"Chapter 4 still exposes the superseded Crumbling Coast transition")
	_expect(drift_source.contains("canon_ch5_classifier_ready") \
		and drift_source.contains("Chapter 5: The Classifier"),
		"Chapter 4 has no explicit saveable Classifier boundary")
	_expect(not belt_source.contains("WorldState.") \
		and not drift_source.contains("WorldState."),
		"Wave 1 map code writes or reads WorldState directly")


func _seed_and_check_malet_world_state() -> void:
	_runner.begin_test("malet_world_state_survives_route")
	_expect(MemoryEngine.learn_fact(ACTOR_ID, FACT_ID),
		"Could not seed the canonical Malet BL-07 knowledge")
	_expect(MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": "player.arrel",
		"content": {"topic": "bl07_request"},
	}), "Could not seed the canonical Malet request-source memory")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Malet did not retain BL-07 route knowledge")
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Malet request-source memory was not active")
	_expect(_events.size() == 2,
		"Initial Malet seed must emit exactly two deterministic events")


func _check_verdan_scene_and_malet_state() -> void:
	_runner.begin_test("canonical_chapter2_scene_load")
	GameManager.current_chapter = 2
	GameManager.set_flag("ch2_arrival_vn_seen")
	GameManager.set_flag("ch2_arrived")
	var verdan := VERDAN_SCENE.instantiate()
	_expect(verdan.get_node_or_null("Malet") != null,
		"Canonical Chapter 2 scene no longer contains the live Malet interaction")
	add_child(verdan)
	_loaded_maps.append(verdan)
	await get_tree().process_frame
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID) \
		and MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Loading playable Chapter 2 reset Malet WorldState")
	_expect(_events.size() == 2,
		"Chapter 2 scene entry duplicated the existing Malet seed events")


func _check_belt_scene_and_legacy_save_compatibility() -> void:
	_runner.begin_test("canonical_chapter3_scene_load")
	GameManager.current_chapter = 3
	for flag_name in ["ch3_arrived", "ch3_blank_book", "ch3_waystation_night", "ch3_class_seven_message"]:
		GameManager.set_flag(flag_name)
	for legacy_flag in ["ch3_tobias_met", "ch3_tobias_records", "tobias_in_party", "tobias_joined"]:
		GameManager.set_flag(legacy_flag)
	BattleManager.tobias_in_party = true

	var belt := BELT_SCENE.instantiate()
	_expect(belt.get_node_or_null("Tobias") == null,
		"Canonical Belt scene still instantiates early Tobias")
	add_child(belt)
	_loaded_maps.append(belt)
	await get_tree().process_frame
	for legacy_flag in ["ch3_tobias_met", "ch3_tobias_records", "tobias_in_party", "tobias_joined"]:
		_expect(not GameManager.get_flag(legacy_flag),
			"Legacy Chapter 3 save flag remains active: %s" % legacy_flag)
	_expect(not BattleManager.tobias_in_party,
		"Legacy early Tobias battle-party state remains active")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID) \
		and MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Loading canonical Chapter 3 changed Malet WorldState")


func _check_drift_scene_and_boundary() -> void:
	_runner.begin_test("canonical_chapter4_scene_load")
	GameManager.current_chapter = 4
	for flag_name in ["ch4_arrived", "ch4_reading_loss", "ch4_anchoring", "ch4_night_watch"]:
		GameManager.set_flag(flag_name)
	GameManager.set_flag("ch4_complete", false)
	GameManager.set_flag("canon_ch5_classifier_ready", false)

	var drift := DRIFT_SCENE.instantiate()
	_expect(drift.get_node_or_null("Tobias") == null,
		"Canonical Drift scene unexpectedly instantiates Tobias")
	add_child(drift)
	_loaded_maps.append(drift)
	await get_tree().process_frame
	drift.call("_depart_shelter")
	var dialogue_steps := 0
	while DialogueManager.is_active and dialogue_steps < 8:
		DialogueManager.advance()
		dialogue_steps += 1
	_expect(not DialogueManager.is_active,
		"Canonical Chapter 4 departure dialogue did not terminate")
	_expect(GameManager.current_chapter == 4,
		"Chapter 4 boundary unlocked the superseded Chapter 5 runtime")
	_expect(GameManager.get_flag("ch4_complete") \
		and GameManager.get_flag("canon_ch5_classifier_ready"),
		"Chapter 4 did not reach its explicit Classifier boundary")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID) \
		and MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Chapter 4 boundary changed Malet WorldState")


func _check_sandbox_round_trip() -> void:
	_runner.begin_test("wave1_world_state_round_trip")
	var save_path := SmokeSaveSandbox.get_slot_path(TEST_SAVE_SLOT, _runner)
	_expect(save_path != "", "Wave 1 test slot did not resolve inside the sandbox")
	var expected_world := WorldState.export_data()
	var event_count_before := _events.size()
	_expect(SaveManager.save_game(TEST_SAVE_SLOT),
		"Wave 1 state could not save through the real SaveManager contract")
	WorldState.reset_to_defaults()
	_expect(not MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"WorldState reset did not clear Malet knowledge")
	_expect(SaveManager.reload_test_world_state(TEST_SAVE_SLOT),
		"Wave 1 sandbox load did not restore WorldState")
	_expect(WorldState.export_data() == expected_world,
		"Wave 1 save -> reset -> load did not restore identical WorldState")
	_expect(_events.size() == event_count_before,
		"Wave 1 save/load replayed committed world events")


func _check_runtime_assets_and_data() -> void:
	_runner.begin_test("canon_dialogue_and_asset_contract")
	var chapter3 := _read_source("res://data/chapter3_dialogue.json")
	var chapter4 := _read_source("res://data/chapter4_dialogue.json")
	var belt_scene := _read_source("res://scenes/maps/belt_waystation.tscn")
	var drift_source := _read_source("res://scenes/maps/drift_shelter.gd")
	_expect(chapter3.contains("waystation_night") \
		and chapter3.contains("class_seven_wall_message") \
		and chapter3.contains("Subject demonstrates Class Seven combustion efficiency."),
		"Chapter 3 canon beats are missing")
	_expect(chapter4.contains("reading_deterioration") \
		and chapter4.contains("anchoring_session") \
		and chapter4.contains("night_watch"),
		"Chapter 4 canon beats are missing")
	_expect(not chapter3.to_lower().contains("tobias") \
		and not chapter4.to_lower().contains("tobias") \
		and not belt_scene.contains("name=\"Tobias\""),
		"Early Tobias dialogue or scene content remains reachable")
	_expect(not drift_source.contains("MapEffects.add_rain") \
		and not drift_source.contains("MapEffects.add_lightning"),
		"Chapter 4 still stages an active memory-rain storm")
	_expect(ResourceLoader.exists("res://assets/cg/generated/story_ch3_tobias_waystation.png"),
		"Retired Tobias art was deleted instead of retained for later canon use")


func _finish(game_before: Dictionary, game_state_before: int,
		world_before: Dictionary, legacy_memory_before: Dictionary,
		story_registry_before: String) -> void:
	_runner.begin_test("persistent_data_and_legacy_system_isolation")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Wave 1 route changed the legacy MemoryManager")
	_expect(_file_fingerprint(StoryLog.READ_REGISTRY_PATH) == story_registry_before,
		"Wave 1 smoke changed the player's persistent StoryLog/read registry")
	_expect(_file_fingerprint(AchievementManager.SAVE_PATH) == _achievement_file_before,
		"Wave 1 scene load changed the player's persistent achievement data")
	_expect(SaveManager.is_smoke_test_mode() \
		and SaveManager.is_test_save_root_configured(),
		"Wave 1 smoke lost the production save-path guard")
	_expect(_events.size() == 2,
		"Scene loads or save/load emitted unexpected WorldState events")

	if EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	AchievementManager.stats = _achievement_stats_before.duplicate(true)
	AchievementManager.unlocked = _achievement_unlocked_before.duplicate(true)
	WorldState.import_data(world_before)
	GameManager.import_data(game_before)
	GameManager.change_state(game_state_before)
	StoryLog.suppress_persistence = true
	_runner.finish(get_tree(),
		"route=new_game_ch1_to_ch2_malet_to_ch3_to_ch4 boundary=ch5_classifier " +
		"tobias_reachable=0 malet_world_state=preserved round_trip=1 " +
		"memory_manager_delta=0 production_slots=0")


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)


func _on_world_event_committed(event: Dictionary) -> void:
	_events.append(event.duplicate(true))


func _prepare_achievement_persistence_guard() -> void:
	_achievement_stats_before = AchievementManager.stats.duplicate(true)
	_achievement_unlocked_before = AchievementManager.unlocked.duplicate(true)
	_achievement_file_before = _file_fingerprint(AchievementManager.SAVE_PATH)
	var visited: Array = AchievementManager.stats.get("maps_visited", []).duplicate()
	for map_id in ["verdan_market", "belt_waystation", "drift_shelter"]:
		if map_id not in visited:
			visited.append(map_id)
	AchievementManager.stats["maps_visited"] = visited
	AchievementManager.unlocked["explorer"] = true
	AchievementManager.unlocked["chapter_complete_4"] = true


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
