extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.veil.exists"
const MEMORY_ID: String = "memory.malet.veil_revelation_source"

var _runner = SmokeTestRunner.new("memory_world_engine", "MEMORY_WORLD_ENGINE_SMOKE_PASS")
var _committed_events: Array[Dictionary] = []


func _ready() -> void:
	Codex.suppress_recording = true
	var legacy_memory_before := MemoryManager.export_data()
	var legacy_flags_before := GameManager.story_flags.duplicate(true)
	var dialogue_active_before := DialogueManager.is_active
	var rewrite_afterglow_before := WorldRewriteDirector.get_active_afterglow_count()

	EventBus.world_event_committed.connect(_on_world_event_committed)
	_check_id_contract()
	_check_knowledge_lifecycle()
	_check_memory_remove_restore_lifecycle()
	_check_conditions_and_independence()
	_check_save_reset_load_round_trip()

	_runner.begin_test("legacy_system_isolation")
	_expect(MemoryManager.export_data() == legacy_memory_before,
		"Memory World Engine smoke changed the existing MemoryManager")
	_expect(GameManager.story_flags == legacy_flags_before,
		"Memory World Engine smoke changed legacy story_flags")
	_expect(DialogueManager.is_active == dialogue_active_before,
		"Memory World Engine smoke changed DialogueManager state")
	_expect(WorldRewriteDirector.get_active_afterglow_count() == rewrite_afterglow_before,
		"Memory World Engine smoke triggered WorldRewriteDirector presentation")

	_runner.finish(get_tree(),
		"actor=%s knowledge_cycle=1 restore_once=1 tombstone=1 round_trip=1" % ACTOR_ID)


func _check_id_contract() -> void:
	_runner.begin_test("id_contract")
	_expect(ActorRegistry.CANONICAL_PLAYER_ACTOR_ID == "player.arrel",
		"Canonical player actor ID must be player.arrel")
	_expect(ActorRegistry.has_actor(ActorRegistry.CANONICAL_PLAYER_ACTOR_ID),
		"Canonical player actor must exist in ActorRegistry")
	_expect(ActorRegistry.has_actor(ACTOR_ID), "Test actor must exist in ActorRegistry")
	_expect(WorldState.has_actor(ACTOR_ID), "Test actor must have a WorldState container")
	_expect(WorldState.is_valid_memory_id(MEMORY_ID), "Test memory ID is invalid")
	_expect(WorldState.is_valid_fact_id(FACT_ID), "Test fact ID is invalid")
	_expect(WorldState.memory_belongs_to_actor(MEMORY_ID, ACTOR_ID),
		"Memory owner slug does not match actor")
	_expect(not ActorRegistry.is_valid_actor_id("npc.Malet"), "Persistent IDs must be lowercase")
	_expect(not ActorRegistry.is_valid_actor_id("npc.2malet"),
		"Persistent ID slugs must begin with a letter")
	_expect(not WorldState.is_valid_fact_id("fact.veil.double__gap"),
		"Persistent ID slugs must use single underscores")


func _check_knowledge_lifecycle() -> void:
	_runner.begin_test("knowledge_learn_forget")
	WorldState.reset_to_defaults()
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Malet must initially know that the Veil exists")

	_committed_events.clear()
	_expect(MemoryEngine.forget_fact(ACTOR_ID, FACT_ID), "Known fact could not be forgotten")
	_expect(not MemoryEngine.knows_fact(ACTOR_ID, FACT_ID), "Forgotten fact must evaluate false")
	_expect(_has_single_event("knowledge.forgotten"),
		"forget_fact must commit exactly one knowledge.forgotten event")
	_expect(not MemoryEngine.forget_fact(ACTOR_ID, FACT_ID),
		"Forgetting an already false fact must be a no-op")
	_expect(_committed_events.size() == 1,
		"Repeated forget_fact must not commit another event")

	_committed_events.clear()
	_expect(MemoryEngine.learn_fact(ACTOR_ID, FACT_ID), "Forgotten fact could not be learned again")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID), "Learned fact must evaluate true")
	_expect(_has_single_event("knowledge.learned"),
		"learn_fact must commit exactly one knowledge.learned event")
	_expect(not MemoryEngine.learn_fact(ACTOR_ID, FACT_ID),
		"Learning an already true fact must be a no-op")
	_expect(_committed_events.size() == 1,
		"Repeated learn_fact must not commit another event")


func _check_memory_remove_restore_lifecycle() -> void:
	_runner.begin_test("memory_remove_restore")
	_committed_events.clear()
	_expect(MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": "player.arrel",
		"content": {"kind": "revelation_source"},
	}), "Test memory could not be added")
	var original := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID), "Added memory must be active")

	_committed_events.clear()
	_expect(MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID), "Active memory could not be removed")
	_expect(_has_single_event("memory.removed"),
		"remove_memory must commit exactly one memory.removed event")
	var tombstone := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(not tombstone.is_empty(), "remove_memory deleted the memory instead of retaining a tombstone")
	_expect(String(tombstone.get("status", "")) == WorldState.MEMORY_STATUS_REMOVED,
		"Removed memory tombstone has the wrong status")
	_expect(not MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID),
		"Removed memory must not pass check_memory")

	_committed_events.clear()
	_expect(MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID), "Removed memory could not be restored")
	_expect(_has_single_event("memory.restored"),
		"restore_memory must commit exactly one memory.restored event")
	var restored := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_expect(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID), "Restored memory must be active")
	_expect(restored.get("content", {}) == original.get("content", {}),
		"restore_memory must preserve the pre-removal content")
	_expect(restored.get("fact_ids", []) == original.get("fact_ids", []),
		"restore_memory must preserve the pre-removal fact links")
	_expect(String(restored.get("source_actor_id", "")) == String(original.get("source_actor_id", "")),
		"restore_memory must preserve the pre-removal source actor")
	_expect(int(restored.get("created_revision", -1)) == int(original.get("created_revision", -2)),
		"restore_memory must preserve created_revision")
	_expect(int(restored.get("removed_revision", -1)) == 0,
		"An active restored memory must clear removed_revision")
	_expect(int(restored.get("last_removed_revision", 0)) == int(tombstone.get("removed_revision", -1)),
		"restore_memory must retain the latest tombstone revision as audit metadata")
	_expect(int(restored.get("restored_revision", 0)) > 0,
		"restore_memory must record restored_revision")
	_expect(not MemoryEngine.restore_memory(ACTOR_ID, MEMORY_ID),
		"Restoring an active memory must be a no-op")
	_expect(_committed_events.size() == 1,
		"Repeated restore_memory must not commit another event")

	# Leave a tombstone for condition and save/load coverage.
	_committed_events.clear()
	_expect(MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID),
		"Restored memory could not be removed for final tombstone coverage")
	_expect(_has_single_event("memory.removed"),
		"Second valid removal must still commit exactly one event")


func _check_conditions_and_independence() -> void:
	_runner.begin_test("memory_knowledge_independence")
	_expect(MemoryEngine.knows_fact(ACTOR_ID, FACT_ID),
		"Removing source memory must not delete related knowledge")
	var memory_condition := {
		"type": "memory", "actor": ACTOR_ID, "memory": MEMORY_ID, "status": "active",
	}
	var knowledge_condition := {
		"type": "knowledge", "actor": ACTOR_ID, "fact": FACT_ID, "equals": true,
	}
	_expect(not DialogueConditionSystem.evaluate(memory_condition),
		"Removed memory condition must be false")
	_expect(DialogueConditionSystem.evaluate(knowledge_condition),
		"Knowledge condition must remain true")


func _check_save_reset_load_round_trip() -> void:
	_runner.begin_test("save_reset_load_round_trip")
	var expected := SaveManager._export_world_state_for_save()
	var event_count_before := _committed_events.size()
	var encoded := JSON.stringify({"world_state": expected})
	var parsed: Variant = JSON.parse_string(encoded)
	if not _expect(parsed is Dictionary, "Serialized WorldState did not parse as a Dictionary"):
		return

	WorldState.reset_to_defaults()
	_expect(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"WorldState reset did not clear the runtime test memory")
	_expect(SaveManager._restore_world_state_from_save_data(parsed),
		"SaveManager WorldState restore failed")
	_expect(WorldState.export_data() == expected,
		"Save -> reset -> load did not restore an identical WorldState")
	_expect(_committed_events.size() == event_count_before,
		"Save/reset/load must not replay world_event_committed")


func _has_single_event(event_type: String) -> bool:
	return _committed_events.size() == 1 and String(_committed_events[0].get("type", "")) == event_type


func _on_world_event_committed(event: Dictionary) -> void:
	_committed_events.append(event.duplicate(true))


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
