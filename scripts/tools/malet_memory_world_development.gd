## Development-only Malet Memory World Engine vertical slice.
##
## This scene is not referenced by gameplay progression. All world mutations
## go through MemoryEngine (apart from the explicit WorldState reset API), and
## save access is disabled unless SaveManager's smoke guard is active.
extends Control

signal branch_preview_changed(branch_id: String)

const MaletDialogueEventConsumer = preload(
	"res://scripts/tools/malet_dialogue_event_consumer.gd")
const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.veil.exists"
const MEMORY_ID: String = "memory.malet.veil_revelation_source"
const SOURCE_ACTOR_ID: String = "player.arrel"
const DIALOGUE_PATH: String = "res://data/development/malet_memory_world_dialogue.json"
const DIALOGUE_KEY: String = "malet_memory_world_probe"
const TEST_SAVE_SLOT: int = 1

@onready var state_label: Label = %StateLabel
@onready var branch_label: Label = %BranchLabel
@onready var event_label: Label = %EventLabel
@onready var feedback_label: Label = %FeedbackLabel
@onready var save_button: Button = %SaveButton
@onready var reload_button: Button = %ReloadButton

var _consumer = MaletDialogueEventConsumer.new()
var _sandbox_ready: bool = false
var _last_branch_id: String = "none"
var _story_log_snapshot: Dictionary = {}


func _ready() -> void:
	_begin_story_log_isolation()
	_consumer.dialogue_refresh_requested.connect(_on_dialogue_refresh_requested)
	_consumer.attach()
	_bind_buttons()
	_sandbox_ready = _ensure_test_save_root()
	save_button.disabled = not _sandbox_ready
	reload_button.disabled = not _sandbox_ready
	reset_test_state()
	if not _sandbox_ready:
		_set_feedback("Save disabled: launch this development scene with --smoke-test.")


func _exit_tree() -> void:
	_consumer.detach()
	_restore_story_log_isolation()


func reset_test_state() -> bool:
	_end_open_dialogue()
	WorldState.reset_to_defaults()
	# Defaults currently contain this knowledge; learn_fact keeps the reset
	# robust if that default changes and remains a deterministic no-op today.
	MemoryEngine.learn_fact(ACTOR_ID, FACT_ID)
	var added := MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": SOURCE_ACTOR_ID,
		"content": {"kind": "revelation_source"},
	})
	_refresh_from_world_state("reset")
	_set_feedback("Test state reset." if added else "Test state reset failed.")
	return added


func remove_source_memory() -> bool:
	var changed := MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID)
	_set_feedback("Source memory removed." if changed else "Remove was a no-op.")
	return changed


func restore_source_memory() -> bool:
	var changed := MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID)
	_set_feedback("Source memory restored." if changed else "Restore was a no-op.")
	return changed


func talk_to_malet() -> bool:
	_end_open_dialogue()
	DialogueManager.load_and_start(DIALOGUE_PATH, DIALOGUE_KEY)
	var started := DialogueManager.is_active
	_set_feedback("Dialogue branch: %s" % get_current_branch_id() if started \
		else "No Malet dialogue branch matched.")
	return started


func save_test_state() -> bool:
	if not _sandbox_ready:
		_set_feedback("Save rejected: isolated test root is not active.")
		return false
	var saved := SaveManager.save_game(TEST_SAVE_SLOT)
	_set_feedback("Sandbox WorldState saved." if saved else "Sandbox save failed.")
	return saved


func reload_test_state() -> bool:
	if not _sandbox_ready:
		_set_feedback("Reload rejected: isolated test root is not active.")
		return false
	_end_open_dialogue()
	var loaded := SaveManager.reload_test_world_state(TEST_SAVE_SLOT)
	if loaded:
		_refresh_from_world_state("load")
	_set_feedback("Sandbox WorldState reloaded." if loaded else "Sandbox reload failed.")
	return loaded


func get_current_branch_id() -> String:
	var lines := get_development_dialogue_lines()
	for value in lines:
		if not (value is Dictionary):
			continue
		var line: Dictionary = value
		var condition: Variant = line.get("condition", null)
		if condition is Dictionary and DialogueConditionSystem.evaluate(condition, {
				"consumer": "MaletDevelopmentPreview",
			}):
			return String(line.get("branch_id", "unknown"))
	return "none"


func get_development_dialogue_lines() -> Array:
	if not DialogueManager.load_dialogue_file(DIALOGUE_PATH):
		return []
	var dialogues: Variant = DialogueManager.loaded_dialogues.get(DIALOGUE_PATH, {})
	if not (dialogues is Dictionary):
		return []
	var lines: Variant = dialogues.get(DIALOGUE_KEY, [])
	return (lines as Array).duplicate(true) if lines is Array else []


func get_consumer_refresh_count() -> int:
	return _consumer.get_refresh_count()


func get_consumer_validation_errors() -> PackedStringArray:
	return _consumer.get_validation_errors()


func get_last_consumed_event() -> Dictionary:
	return _consumer.get_last_event()


func is_save_sandbox_ready() -> bool:
	return _sandbox_ready


func refresh_development_view() -> void:
	_refresh_from_world_state("manual")


func _bind_buttons() -> void:
	%ResetButton.pressed.connect(reset_test_state)
	%RemoveButton.pressed.connect(remove_source_memory)
	%RestoreButton.pressed.connect(restore_source_memory)
	%TalkButton.pressed.connect(talk_to_malet)
	save_button.pressed.connect(save_test_state)
	reload_button.pressed.connect(reload_test_state)


func _ensure_test_save_root() -> bool:
	if not SaveManager.is_smoke_test_mode():
		return false
	if SaveManager.is_test_save_root_configured():
		return true
	var root := "%s/malet_vertical_slice_%d" % [
		SaveManager.TEST_SAVE_ROOT_PREFIX,
		OS.get_process_id(),
	]
	return SaveManager.configure_test_save_root(root)


func _on_dialogue_refresh_requested(event: Dictionary) -> void:
	_refresh_from_world_state("event")
	event_label.text = "Last committed event: %s / %s" % [
		String(event.get("event_id", "?")),
		String(event.get("event_type", "?")),
	]


func _refresh_from_world_state(_reason: String) -> void:
	var record := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	var memory_status := String(record.get("status", "missing"))
	var source_actor := String(record.get("source_actor_id", "none"))
	var restored_revision := int(record.get("restored_revision", 0))
	state_label.text = "Knowledge: %s | Memory: %s | Source: %s | Restored revision: %d" % [
		str(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID)),
		memory_status,
		source_actor,
		restored_revision,
	]
	_last_branch_id = get_current_branch_id()
	branch_label.text = "Resolved dialogue branch: %s" % _last_branch_id
	branch_preview_changed.emit(_last_branch_id)


func _end_open_dialogue() -> void:
	if DialogueManager.is_active:
		DialogueManager.end_dialogue()


func _set_feedback(message: String) -> void:
	feedback_label.text = message


func _begin_story_log_isolation() -> void:
	if not StoryLog:
		return
	_story_log_snapshot = {
		"entries": StoryLog.entries.duplicate(true),
		"read_keys": (StoryLog.get("_read_keys") as Dictionary).duplicate(true),
		"read_dirty": bool(StoryLog.get("_read_dirty")),
		"last_line_was_new": StoryLog.last_line_was_new,
		"suppress_persistence": StoryLog.suppress_persistence,
	}
	StoryLog.suppress_persistence = true


func _restore_story_log_isolation() -> void:
	if not StoryLog or _story_log_snapshot.is_empty():
		return
	StoryLog.entries = (_story_log_snapshot.get("entries", []) as Array).duplicate(true)
	StoryLog.set("_read_keys",
		(_story_log_snapshot.get("read_keys", {}) as Dictionary).duplicate(true))
	StoryLog.set("_read_dirty", bool(_story_log_snapshot.get("read_dirty", false)))
	StoryLog.last_line_was_new = bool(_story_log_snapshot.get("last_line_was_new", false))
	StoryLog.suppress_persistence = bool(
		_story_log_snapshot.get("suppress_persistence", false))
	_story_log_snapshot.clear()
