extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const WorldEventConsumerProbe = preload("res://scripts/tools/world_event_consumer_probe.gd")
const ACTOR_ID := "npc.malet"
const FACT_ID := "fact.veil.exists"
const MEMORY_ID := "memory.malet.veil_revelation_source"

var _runner = SmokeTestRunner.new(
	"world_event_consumer_probe", "WORLD_EVENT_CONSUMER_PROBE_SMOKE_PASS")


func _ready() -> void:
	Codex.suppress_recording = true
	var legacy_memory_before := MemoryManager.export_data()
	var legacy_flags_before := GameManager.story_flags.duplicate(true)

	_runner.begin_test("baseline_without_probe")
	WorldState.reset_to_defaults()
	_run_five_mutations()
	var expected_state := WorldState.export_data()

	_runner.begin_test("read_only_probe_receives_five_types")
	WorldState.reset_to_defaults()
	var probe = WorldEventConsumerProbe.new()
	_expect(probe.attach(), "Read-only consumer probe could not subscribe")
	_run_five_mutations()
	probe.detach()

	var received: Array[Dictionary] = probe.get_events()
	_expect(received.size() == 5, "Probe must receive exactly five committed events")
	for event_type in EventBus.EVENT_TYPES:
		_expect(probe.count_event_type(event_type) == 1,
			"Probe must receive exactly one %s event" % event_type)
	_expect(probe.get_validation_errors().is_empty(),
		"Probe reported schema/order/identity errors: %s" % probe.get_validation_errors())
	_expect(probe.get_last_sequence() == 5,
		"Probe must observe event_sequence 1 through 5 in order")
	_expect(probe.get_payload_fields_read() == 10,
		"Probe must read all ten v1 payload fields across five events")
	_expect(WorldState.export_data() == expected_state,
		"Attaching the probe changed the deterministic WorldState result")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Read-only probe changed the existing MemoryManager")
	_expect(GameManager.story_flags == legacy_flags_before,
		"Read-only probe changed GameManager.story_flags")

	WorldState.reset_to_defaults()
	_runner.finish(get_tree(),
		"schema_v1=true events=5 types_once=5 payload_fields_read=10 state_writes=0")


func _run_five_mutations() -> void:
	_expect(MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": "player.arrel",
	}), "memory.added mutation failed")
	_expect(MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID), "memory.removed mutation failed")
	_expect(MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID), "memory.restored mutation failed")
	_expect(MemoryEngine.forget_fact(ACTOR_ID, FACT_ID), "knowledge.forgotten mutation failed")
	_expect(MemoryEngine.learn_fact(ACTOR_ID, FACT_ID), "knowledge.learned mutation failed")


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
