## EventBus (Autoload)
## Memory World Engine이 확정한 deterministic world event만 전달한다.
## 상태를 소유하거나 변경하지 않으며, 소비자는 전달받은 복사본만 읽는다.
extends Node

signal world_event_committed(event: Dictionary)

const WORLD_EVENT_SCHEMA_VERSION: int = 1
const EVENT_TYPES: PackedStringArray = [
	"memory.added",
	"memory.removed",
	"memory.restored",
	"knowledge.learned",
	"knowledge.forgotten",
]
const COMMON_FIELDS: PackedStringArray = [
	"schema_version",
	"event_id",
	"event_type",
	"event_sequence",
	"revision",
	"actor_id",
	"target_id",
	"payload",
]


func emit_world_event_committed(event: Dictionary) -> bool:
	var validation_error := get_validation_error(event)
	if validation_error != "":
		push_warning("[EventBus] Rejected world event: %s" % validation_error)
		return false
	world_event_committed.emit(event.duplicate(true))
	return true


func validate_world_event(event: Dictionary) -> bool:
	return get_validation_error(event) == ""


func get_validation_error(event: Dictionary) -> String:
	if event.size() != COMMON_FIELDS.size():
		return "common field count mismatch"
	for key in event:
		if String(key) not in COMMON_FIELDS:
			return "unexpected common field=%s" % key
	for field in COMMON_FIELDS:
		if not event.has(field):
			return "missing common field=%s" % field
	if not (event.get("schema_version") is int) \
			or int(event.schema_version) != WORLD_EVENT_SCHEMA_VERSION:
		return "unsupported schema_version=%s" % event.get("schema_version", "<missing>")

	var event_type := String(event.get("event_type", ""))
	if event_type not in EVENT_TYPES:
		return "unsupported event_type=%s" % event_type
	if not (event.get("event_sequence") is int) or int(event.event_sequence) <= 0:
		return "event_sequence must be a positive integer"
	if not (event.get("revision") is int) or int(event.revision) <= 0:
		return "revision must be a positive integer"
	if String(event.get("event_id", "")) != "world.%08d" % int(event.event_sequence):
		return "event_id is not deterministic for event_sequence"
	if String(event.get("actor_id", "")) == "":
		return "actor_id is empty"
	var target_id := String(event.get("target_id", ""))
	if target_id == "":
		return "target_id is empty"
	if event_type.begins_with("memory.") and not target_id.begins_with("memory."):
		return "memory event target_id must use memory namespace"
	if event_type.begins_with("knowledge.") and not target_id.begins_with("fact."):
		return "knowledge event target_id must use fact namespace"
	if not (event.get("payload") is Dictionary):
		return "payload must be a dictionary"
	return _validate_payload(event_type, event.payload)


func _validate_payload(event_type: String, payload: Dictionary) -> String:
	match event_type:
		"memory.added":
			var field_error := _require_exact_payload_fields(
				payload, PackedStringArray(["status", "fact_ids", "source_actor_id"]))
			if field_error != "":
				return field_error
			if String(payload.status) != "active":
				return "memory.added payload status must be active"
			if not (payload.fact_ids is Array):
				return "memory.added payload fact_ids must be an array"
			for fact_id in payload.fact_ids:
				if not (fact_id is String):
					return "memory.added payload fact_ids must contain strings"
			if not (payload.source_actor_id is String):
				return "memory.added payload source_actor_id must be a string"
		"memory.removed":
			var field_error := _require_exact_payload_fields(
				payload, PackedStringArray(["previous_status", "status"]))
			if field_error != "":
				return field_error
			if String(payload.previous_status) != "active" or String(payload.status) != "removed":
				return "memory.removed payload must describe active to removed"
		"memory.restored":
			var field_error := _require_exact_payload_fields(
				payload, PackedStringArray(["previous_status", "status", "last_removed_revision"]))
			if field_error != "":
				return field_error
			if String(payload.previous_status) != "removed" or String(payload.status) != "active":
				return "memory.restored payload must describe removed to active"
			if not (payload.last_removed_revision is int) or int(payload.last_removed_revision) <= 0:
				return "memory.restored last_removed_revision must be a positive integer"
		"knowledge.learned":
			var field_error := _require_exact_payload_fields(payload, PackedStringArray(["value"]))
			if field_error != "":
				return field_error
			if not (payload.value is bool) or not bool(payload.value):
				return "knowledge.learned payload value must be true"
		"knowledge.forgotten":
			var field_error := _require_exact_payload_fields(payload, PackedStringArray(["value"]))
			if field_error != "":
				return field_error
			if not (payload.value is bool) or bool(payload.value):
				return "knowledge.forgotten payload value must be false"
	return ""


func _require_exact_payload_fields(payload: Dictionary, fields: PackedStringArray) -> String:
	if payload.size() != fields.size():
		return "payload field count mismatch"
	for key in payload:
		if String(key) not in fields:
			return "unexpected payload field=%s" % key
	for field in fields:
		if not payload.has(field):
			return "missing payload field=%s" % field
	return ""
