extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const DEVELOPMENT_SCENE = preload(
	"res://scripts/tools/malet_memory_world_development.tscn")
const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.veil.exists"
const MEMORY_ID: String = "memory.malet.veil_revelation_source"
const TEST_SAVE_SLOT: int = 1

var _runner = SmokeTestRunner.new(
	"malet_memory_world_vertical_slice", "MALET_MEMORY_WORLD_VERTICAL_SLICE_SMOKE_PASS")
var _events: Array[Dictionary] = []
var _last_dialogue_text: String = ""
var _last_dialogue_speaker: String = ""


func _ready() -> void:
	Codex.suppress_recording = true
	var legacy_memory_before := MemoryManager.export_data()
	var legacy_flags_before := GameManager.story_flags.duplicate(true)
	var previous_locale := GameManager.current_locale
	var previous_game_state := GameManager.current_state
	var story_entries_before := StoryLog.entries.duplicate(true)
	var story_read_count_before := StoryLog.read_line_count()
	var story_suppression_before := StoryLog.suppress_persistence

	if not SmokeSaveSandbox.activate("malet_vertical_slice", _runner):
		_runner.finish(get_tree())
		return

	EventBus.world_event_committed.connect(_on_world_event_committed)
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	GameManager.current_locale = "ko"

	var development_scene = DEVELOPMENT_SCENE.instantiate()
	add_child(development_scene)

	_check_scene_and_initial_active_branch(development_scene)
	_check_legacy_dialogue_compatibility()
	_check_remove_branch_and_no_op(development_scene)
	_check_sandbox_save_reload(development_scene)
	_check_restore_branch_and_no_op(development_scene)
	_check_knowledge_events(development_scene)
	_check_read_only_boundaries(development_scene, legacy_memory_before, legacy_flags_before)

	if DialogueManager.is_active:
		DialogueManager.end_dialogue()
	development_scene.free()
	_runner.begin_test("development_scene_cleanup")
	_expect(StoryLog.entries == story_entries_before,
		"Development dialogue leaked synthetic lines into StoryLog")
	_expect(StoryLog.read_line_count() == story_read_count_before,
		"Development dialogue leaked synthetic read keys into StoryLog")
	_expect(StoryLog.suppress_persistence == story_suppression_before,
		"Development scene did not restore StoryLog persistence mode")
	if EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	if DialogueManager.dialogue_line.is_connected(_on_dialogue_line):
		DialogueManager.dialogue_line.disconnect(_on_dialogue_line)
	WorldState.reset_to_defaults()
	GameManager.current_locale = previous_locale
	GameManager.change_state(previous_game_state)

	_runner.finish(get_tree(),
		"branches=active_removed_restored events_once=5 no_op_events=0 " +
		"round_trip=1 consumer_refreshes=4 state_writes=0 production_slots=0")


func _check_scene_and_initial_active_branch(development_scene: Node) -> void:
	_runner.begin_test("development_scene_active_dialogue")
	_expect(development_scene.is_save_sandbox_ready(),
		"Development scene did not inherit the isolated save sandbox")
	_expect(development_scene.get_development_dialogue_lines().size() == 3,
		"Development dialogue must contain exactly three branches")
	_expect(_count_event("memory.added") == 1,
		"Initial test state must add its source memory exactly once")
	var record := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(String(record.get("status", "")) == WorldState.MEMORY_STATUS_ACTIVE,
		"Initial source memory must be active")
	_expect(String(record.get("source_actor_id", "")) == "player.arrel",
		"Initial source actor must be player.arrel")
	_expect(int(record.get("restored_revision", -1)) == 0,
		"Initial active memory must not look restored")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Malet must know that the Veil exists")
	_talk_and_expect(development_scene, "active",
		"Veil은 존재해. 아렐에게 직접 들었거든.")


func _check_remove_branch_and_no_op(development_scene: Node) -> void:
	_runner.begin_test("removed_memory_keeps_knowledge")
	var before := _events.size()
	_expect(development_scene.remove_source_memory(), "Source memory removal failed")
	_expect(_events.size() == before + 1 and _count_event("memory.removed") == 1,
		"remove_memory must emit exactly one memory.removed event")
	_expect(development_scene.get_consumer_refresh_count() == 1,
		"Malet consumer must refresh exactly once for memory.removed")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Removing source memory must not remove Veil knowledge")
	_talk_and_expect(development_scene, "removed",
		"Veil이 존재한다는 건 알아. 그런데... 누가 말해줬는지는 기억이 안 나.")

	before = _events.size()
	_expect(not development_scene.remove_source_memory(),
		"Removing the tombstone again must be a no-op")
	_expect(_events.size() == before, "No-op remove emitted an event")


func _check_legacy_dialogue_compatibility() -> void:
	_runner.begin_test("legacy_dialogue_condition_compatibility")
	var legacy_path := "res://data/chapter2_dialogue.json"
	_expect(DialogueManager.load_dialogue_file(legacy_path),
		"Existing chapter 2 dialogue could not be loaded")
	var dialogues: Dictionary = DialogueManager.loaded_dialogues.get(legacy_path, {})
	var legacy_lines: Array = dialogues.get("elia_ch2_talk", [])
	if not _expect(not legacy_lines.is_empty(), "Legacy dialogue probe line is missing"):
		return
	var legacy_line: Dictionary = (legacy_lines[0] as Dictionary).duplicate(true)
	_expect(not legacy_line.has("condition"),
		"Legacy probe must not opt into structured Memory World conditions")
	var expected_text := GameManager.localized_value(legacy_line, "text", "")
	if MemoryManager.is_memory_burned(String(legacy_line.get("requires_memory", ""))):
		expected_text = GameManager.localized_value(legacy_line, "burned_text", expected_text)
	_last_dialogue_text = ""
	_last_dialogue_speaker = ""
	DialogueManager.start_dialogue([legacy_line])
	_expect(DialogueManager.is_active,
		"Legacy requires_memory + burned_text line was incorrectly skipped")
	_expect(_last_dialogue_speaker == String(legacy_line.get("speaker", "")) \
		and _last_dialogue_text == expected_text,
		"Legacy dialogue text replacement semantics changed")
	DialogueManager.end_dialogue()


func _check_sandbox_save_reload(development_scene: Node) -> void:
	_runner.begin_test("sandbox_save_reset_reload")
	var isolated_path := SmokeSaveSandbox.get_slot_path(
		TEST_SAVE_SLOT, _runner)
	_expect(isolated_path != "", "Development save slot did not resolve in the sandbox")
	var expected_removed_state := WorldState.export_data()
	var events_before_save := _events.size()
	_expect(development_scene.save_test_state(), "Development sandbox save failed")

	WorldState.reset_to_defaults()
	development_scene.refresh_development_view()
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"WorldState reset did not clear the source memory")
	_expect(development_scene.reload_test_state(), "Development sandbox reload failed")
	_expect(WorldState.export_data() == expected_removed_state,
		"Sandbox save/reset/reload did not restore identical WorldState")
	_expect(_events.size() == events_before_save,
		"Save or reload replayed committed events")
	_talk_and_expect(development_scene, "removed",
		"Veil이 존재한다는 건 알아. 그런데... 누가 말해줬는지는 기억이 안 나.")


func _check_restore_branch_and_no_op(development_scene: Node) -> void:
	_runner.begin_test("restored_dialogue_branch")
	var before := _events.size()
	_expect(development_scene.restore_source_memory(), "Source memory restore failed")
	_expect(_events.size() == before + 1 and _count_event("memory.restored") == 1,
		"restore_memory must emit exactly one memory.restored event")
	_expect(development_scene.get_consumer_refresh_count() == 2,
		"Malet consumer must refresh exactly once for memory.restored")
	var record := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(int(record.get("restored_revision", 0)) > 0,
		"Restored branch requires existing deterministic restored_revision metadata")
	_talk_and_expect(development_scene, "restored", "맞아. 아렐이었어. 이제 기억나.")

	before = _events.size()
	_expect(not development_scene.restore_source_memory(),
		"Restoring an active memory again must be a no-op")
	_expect(_events.size() == before, "No-op restore emitted an event")


func _check_knowledge_events(development_scene: Node) -> void:
	_runner.begin_test("knowledge_consumer_refresh")
	var before := _events.size()
	_expect(MemoryEngine.forget_fact(ACTOR_ID, FACT_ID), "Knowledge forget failed")
	_expect(_events.size() == before + 1 and _count_event("knowledge.forgotten") == 1,
		"forget_fact must emit exactly one knowledge.forgotten event")
	_expect(development_scene.get_consumer_refresh_count() == 3,
		"Malet consumer did not refresh for knowledge.forgotten")
	_expect(development_scene.get_current_branch_id() == "none",
		"No branch should match while Veil knowledge is false")

	before = _events.size()
	_expect(not MemoryEngine.forget_fact(ACTOR_ID, FACT_ID),
		"Repeated forget_fact must be a no-op")
	_expect(_events.size() == before, "No-op forget emitted an event")

	_expect(MemoryEngine.learn_fact(ACTOR_ID, FACT_ID), "Knowledge relearn failed")
	_expect(_count_event("knowledge.learned") == 1,
		"learn_fact must emit exactly one knowledge.learned event")
	_expect(development_scene.get_consumer_refresh_count() == 4,
		"Malet consumer did not refresh for knowledge.learned")
	_talk_and_expect(development_scene, "restored", "맞아. 아렐이었어. 이제 기억나.")

	before = _events.size()
	_expect(not MemoryEngine.learn_fact(ACTOR_ID, FACT_ID),
		"Repeated learn_fact must be a no-op")
	_expect(_events.size() == before, "No-op learn emitted an event")


func _check_read_only_boundaries(development_scene: Node,
		legacy_memory_before: Dictionary, legacy_flags_before: Dictionary) -> void:
	_runner.begin_test("read_only_consumer_and_legacy_isolation")
	_expect(development_scene.get_consumer_validation_errors().is_empty(),
		"Malet consumer rejected a committed schema v1 event")
	var last_event: Dictionary = development_scene.get_last_consumed_event()
	_expect(int(last_event.get("schema_version", 0)) == 1,
		"Consumer did not validate schema v1")
	for event in _events:
		_expect(EventBus.validate_world_event(event),
			"Vertical slice observed an invalid event: %s" % EventBus.get_validation_error(event))
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Malet vertical slice changed the legacy MemoryManager")
	_expect(GameManager.story_flags == legacy_flags_before,
		"Malet vertical slice changed GameManager.story_flags")

	var controller_source := _read_source(
		"res://scripts/tools/malet_memory_world_development.gd")
	var consumer_source := _read_source(
		"res://scripts/tools/malet_dialogue_event_consumer.gd")
	_expect(not controller_source.contains("WorldState._store_") \
		and not controller_source.contains("WorldState.import_data") \
		and not controller_source.contains("story_flags"),
		"Development controller contains a forbidden direct state write")
	_expect(not consumer_source.contains("MemoryEngine.") \
		and not consumer_source.contains("WorldState._") \
		and not consumer_source.contains("GameManager."),
		"Read-only consumer contains a gameplay mutation dependency")


func _talk_and_expect(development_scene: Node, branch_id: String, text: String) -> void:
	_last_dialogue_text = ""
	_last_dialogue_speaker = ""
	_expect(development_scene.get_current_branch_id() == branch_id,
		"Preview resolved the wrong branch; expected %s" % branch_id)
	_expect(development_scene.talk_to_malet(),
		"DialogueManager did not start branch %s" % branch_id)
	_expect(_last_dialogue_speaker == "Malet", "Dialogue speaker must be Malet")
	_expect(_last_dialogue_text == text,
		"DialogueManager emitted the wrong text for branch %s: %s" % [
			branch_id, _last_dialogue_text,
		])
	_expect(DialogueManager.current_index >= 0 \
		and DialogueManager.current_index < DialogueManager.current_dialogue.size() \
		and String(DialogueManager.current_dialogue[DialogueManager.current_index].get(
			"branch_id", "")) == branch_id,
		"Actual DialogueManager pipeline selected the wrong source line")
	DialogueManager.end_dialogue()


func _read_source(path: String) -> String:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return ""
	var content := file.get_as_text()
	file.close()
	return content


func _count_event(event_type: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type:
			count += 1
	return count


func _on_world_event_committed(event: Dictionary) -> void:
	_events.append(event.duplicate(true))


func _on_dialogue_line(speaker: String, text: String, _portrait: String) -> void:
	_last_dialogue_speaker = speaker
	_last_dialogue_text = text


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
