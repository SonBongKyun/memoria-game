## Development-only read-only EventBus consumer.
##
## The probe validates and records immutable copies of committed events. It has
## no reference to MemoryEngine mutation APIs and never writes WorldState,
## GameManager, story_flags, or gameplay systems.
extends RefCounted

var _events: Array[Dictionary] = []
var _validation_errors: PackedStringArray = []
var _last_sequence: int = 0
var _payload_fields_read: int = 0
var _attached: bool = false


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


func get_events() -> Array[Dictionary]:
	return _events.duplicate(true)


func get_validation_errors() -> PackedStringArray:
	return _validation_errors.duplicate()


func get_last_sequence() -> int:
	return _last_sequence


func get_payload_fields_read() -> int:
	return _payload_fields_read


func count_event_type(event_type: String) -> int:
	var count := 0
	for event in _events:
		if String(event.get("event_type", "")) == event_type:
			count += 1
	return count


func _on_world_event_committed(event: Dictionary) -> void:
	var snapshot := event.duplicate(true)
	var validation_error := EventBus.get_validation_error(snapshot)
	if validation_error != "":
		_validation_errors.append(validation_error)

	var sequence := int(snapshot.get("event_sequence", 0))
	if sequence <= 0:
		_validation_errors.append("event_sequence must be positive")
	elif _last_sequence > 0 and sequence != _last_sequence + 1:
		_validation_errors.append("event_sequence out of order: %d after %d" % [
			sequence, _last_sequence,
		])
	_last_sequence = sequence

	var actor_id := String(snapshot.get("actor_id", ""))
	if not ActorRegistry.has_actor(actor_id):
		_validation_errors.append("unknown actor_id=%s" % actor_id)
	var target_id := String(snapshot.get("target_id", ""))
	var event_type := String(snapshot.get("event_type", ""))
	if event_type.begins_with("memory."):
		if not WorldState.memory_belongs_to_actor(target_id, actor_id):
			_validation_errors.append("invalid memory target_id=%s" % target_id)
	elif event_type.begins_with("knowledge."):
		if not WorldState.is_valid_fact_id(target_id):
			_validation_errors.append("invalid fact target_id=%s" % target_id)

	var payload: Dictionary = snapshot.get("payload", {})
	for key in payload:
		# Reading every field proves consumers can inspect the payload without
		# retaining a mutable reference to the emitted Dictionary.
		var _read_value: Variant = payload[key]
		_payload_fields_read += 1
	_events.append(snapshot)
