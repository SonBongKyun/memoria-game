## Development-only read-only consumer for the Malet dialogue vertical slice.
##
## It validates schema v1, filters to one actor and two targets, reads the
## payload, and requests a UI/dialogue preview refresh. It owns no state and has
## no mutation API.
extends RefCounted

signal dialogue_refresh_requested(event: Dictionary)

const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.veil.exists"
const MEMORY_ID: String = "memory.malet.veil_revelation_source"
const RELEVANT_EVENT_TYPES: PackedStringArray = [
	"memory.removed",
	"memory.restored",
	"knowledge.learned",
	"knowledge.forgotten",
]

var _attached: bool = false
var _refresh_count: int = 0
var _ignored_count: int = 0
var _validation_errors: PackedStringArray = []
var _last_event: Dictionary = {}
var _last_payload_value: Variant = null


func attach() -> bool:
	if _attached:
		return false
	EventBus.world_event_committed.connect(_on_world_event_committed)
	_attached = true
	return true


func detach() -> void:
	if _attached and EventBus.world_event_committed.is_connected(_on_world_event_committed):
		EventBus.world_event_committed.disconnect(_on_world_event_committed)
	_attached = false


func get_refresh_count() -> int:
	return _refresh_count


func get_ignored_count() -> int:
	return _ignored_count


func get_validation_errors() -> PackedStringArray:
	return _validation_errors.duplicate()


func get_last_event() -> Dictionary:
	return _last_event.duplicate(true)


func get_last_payload_value() -> Variant:
	return _last_payload_value


func _on_world_event_committed(event: Dictionary) -> void:
	var snapshot := event.duplicate(true)
	var validation_error := EventBus.get_validation_error(snapshot)
	if validation_error != "":
		_validation_errors.append(validation_error)
		return
	if int(snapshot.get("schema_version", 0)) != EventBus.WORLD_EVENT_SCHEMA_VERSION:
		_validation_errors.append("unsupported schema_version")
		return

	var event_type := String(snapshot.get("event_type", ""))
	var actor_id := String(snapshot.get("actor_id", ""))
	var target_id := String(snapshot.get("target_id", ""))
	if actor_id != ACTOR_ID or event_type not in RELEVANT_EVENT_TYPES:
		_ignored_count += 1
		return
	if event_type.begins_with("memory.") and target_id != MEMORY_ID:
		_ignored_count += 1
		return
	if event_type.begins_with("knowledge.") and target_id != FACT_ID:
		_ignored_count += 1
		return

	var payload: Dictionary = snapshot.get("payload", {})
	_last_payload_value = payload.get("status", payload.get("value", null))
	_last_event = snapshot
	_refresh_count += 1
	dialogue_refresh_requested.emit(snapshot.duplicate(true))
