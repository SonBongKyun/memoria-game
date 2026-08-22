extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const ACTOR_ID := "npc.malet"
const FACT_ID := "fact.veil.exists"
const MEMORY_ID := "memory.malet.veil_revelation_source"

var _runner = SmokeTestRunner.new("world_event_schema", "WORLD_EVENT_SCHEMA_SMOKE_PASS")
var _events: Array[Dictionary] = []


func _ready() -> void:
	Codex.suppress_recording = true
	WorldState.reset_to_defaults()
	EventBus.world_event_committed.connect(_on_world_event_committed)

	_runner.begin_test("five_committed_event_types")
	_expect(MemoryEngine.forget_fact(ACTOR_ID, FACT_ID), "knowledge.forgotten mutation failed")
	_expect(_events.size() == 1, "knowledge.forgotten must emit exactly one event")
	_expect(MemoryEngine.learn_fact(ACTOR_ID, FACT_ID), "knowledge.learned mutation failed")
	_expect(_events.size() == 2, "knowledge.learned must emit exactly one event")
	_expect(MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": "player.arrel",
	}), "memory.added mutation failed")
	_expect(_events.size() == 3, "memory.added must emit exactly one event")
	_expect(MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID), "memory.removed mutation failed")
	_expect(_events.size() == 4, "memory.removed must emit exactly one event")
	_expect(MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID), "memory.restored mutation failed")
	_expect(_events.size() == 5, "memory.restored must emit exactly one event")

	_validate_event_contract()
	_validate_nondeterministic_field_rejection()
	_validate_no_op_and_load_contract()
	WorldState.reset_to_defaults()
	_runner.finish(get_tree(), "types=5 deterministic_ids=true replay_on_load=0")


func _validate_event_contract() -> void:
	_runner.begin_test("stable_payload_schema")
	var expected := [
		{"event_type": "knowledge.forgotten", "target_id": FACT_ID, "value": false},
		{"event_type": "knowledge.learned", "target_id": FACT_ID, "value": true},
		{"event_type": "memory.added", "target_id": MEMORY_ID, "status": "active"},
		{"event_type": "memory.removed", "target_id": MEMORY_ID, "status": "removed"},
		{"event_type": "memory.restored", "target_id": MEMORY_ID, "status": "active"},
	]
	for index in range(expected.size()):
		if not _expect(index < _events.size(), "Missing event at index %d" % index):
			continue
		var event := _events[index]
		var contract: Dictionary = expected[index]
		var sequence := index + 1
		_expect(EventBus.validate_world_event(event),
			"EventBus rejected emitted event %d: %s" % [
				sequence, EventBus.get_validation_error(event),
			])
		_expect(event.size() == EventBus.COMMON_FIELDS.size(),
			"Event must contain only the stable common fields")
		_expect(String(event.get("event_id", "")) == "world.%08d" % sequence,
			"event_id must derive only from event_sequence")
		_expect(int(event.get("event_sequence", 0)) == sequence,
			"event_sequence must increase by one")
		_expect(int(event.get("revision", 0)) == sequence,
			"Each tested mutation must advance revision once")
		_expect(String(event.get("actor_id", "")) == ACTOR_ID,
			"actor_id must identify the state owner")
		_expect(String(event.get("event_type", "")) == String(contract.event_type),
			"event_type mismatch at sequence %d" % sequence)
		_expect(String(event.get("target_id", "")) == String(contract.target_id),
			"target_id mismatch at sequence %d" % sequence)
		var payload: Dictionary = event.get("payload", {})
		if contract.has("value"):
			_expect(payload.get("value") == contract.value,
				"Knowledge payload value mismatch at sequence %d" % sequence)
		if contract.has("status"):
			_expect(String(payload.get("status", "")) == String(contract.status),
				"Memory payload status mismatch at sequence %d" % sequence)


func _validate_nondeterministic_field_rejection() -> void:
	_runner.begin_test("nondeterministic_fields_rejected")
	if not _expect(not _events.is_empty(), "Schema rejection test needs a valid baseline event"):
		return
	var extra_common := _events[0].duplicate(true)
	extra_common["timestamp"] = 1
	_expect(not EventBus.validate_world_event(extra_common),
		"Event schema must reject wall-clock fields")

	var extra_payload := _events[0].duplicate(true)
	var payload: Dictionary = extra_payload.get("payload", {}).duplicate(true)
	payload["nonce"] = 7
	extra_payload["payload"] = payload
	_expect(not EventBus.validate_world_event(extra_payload),
		"Event schema must reject random/nonce payload fields")

	var wrong_id := _events[0].duplicate(true)
	wrong_id["event_id"] = "world.99999999"
	_expect(not EventBus.validate_world_event(wrong_id),
		"event_id must be derived from event_sequence")


func _validate_no_op_and_load_contract() -> void:
	_runner.begin_test("no_op_and_load_do_not_emit")
	var event_count := _events.size()
	_expect(not MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID),
		"Restoring an active memory must be a no-op")
	_expect(not MemoryEngine.learn_fact(ACTOR_ID, FACT_ID),
		"Learning an already-known fact must be a no-op")
	_expect(_events.size() == event_count, "No-op mutations must not emit events")

	var snapshot := SaveManager._export_world_state_for_save()
	var encoded := JSON.stringify({"world_state": snapshot})
	var parsed: Variant = JSON.parse_string(encoded)
	WorldState.reset_to_defaults()
	_expect(parsed is Dictionary, "Serialized event-schema snapshot did not parse")
	if parsed is Dictionary:
		_expect(SaveManager._restore_world_state_from_save_data(parsed),
			"WorldState snapshot could not be restored")
	_expect(_events.size() == event_count, "Save/load must not replay committed events")


func _on_world_event_committed(event: Dictionary) -> void:
	_events.append(event.duplicate(true))


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
