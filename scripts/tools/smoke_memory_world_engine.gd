extends Node

const ACTOR_ID: String = "npc.malet"
const FACT_ID: String = "fact.veil.exists"
const MEMORY_ID: String = "memory.malet.veil_revelation_source"

var _committed_events: Array[Dictionary] = []
var _failures: PackedStringArray = []


func _ready() -> void:
	Codex.suppress_recording = true
	var legacy_memory_before := MemoryManager.export_data()
	var legacy_flags_before := GameManager.story_flags.duplicate(true)
	var dialogue_active_before := DialogueManager.is_active
	var rewrite_afterglow_before := WorldRewriteDirector.get_active_afterglow_count()

	_check_id_contract()
	_check_memory_and_knowledge_separation()
	_check_save_reset_load_round_trip()
	_check_legacy_save_compatibility()

	_require(MemoryManager.export_data() == legacy_memory_before,
		"Memory World Engine smoke changed the existing MemoryManager")
	_require(GameManager.story_flags == legacy_flags_before,
		"Memory World Engine smoke changed legacy story_flags")
	_require(DialogueManager.is_active == dialogue_active_before,
		"Memory World Engine smoke changed DialogueManager state")
	_require(WorldRewriteDirector.get_active_afterglow_count() == rewrite_afterglow_before,
		"Memory World Engine smoke triggered WorldRewriteDirector presentation")

	if not _failures.is_empty():
		push_error("MEMORY_WORLD_ENGINE_SMOKE_FAIL count=%d failures=%s" % [_failures.size(), _failures])
		get_tree().quit(1)
		return
	print("MEMORY_WORLD_ENGINE_SMOKE_PASS actor=%s fact_kept=1 tombstone=1 committed_once=1 save_round_trip=1 legacy_save=1" % ACTOR_ID)
	get_tree().quit(0)


func _check_id_contract() -> void:
	_require(WorldState.CANONICAL_PLAYER_ACTOR_ID == "player.arrel",
		"Canonical player actor ID must be player.arrel")
	_require(WorldState.is_valid_actor_id(WorldState.CANONICAL_PLAYER_ACTOR_ID),
		"Canonical player actor ID must satisfy the actor ID grammar")
	_require(WorldState.has_actor(WorldState.CANONICAL_PLAYER_ACTOR_ID),
		"Canonical player actor must exist in the default WorldState")
	_require(not WorldState.has_actor("player.arell"),
		"The misspelled player.arell ID must not be registered as the canonical actor")
	_require(WorldState.is_valid_actor_id(ACTOR_ID), "Test actor ID is invalid")
	_require(WorldState.is_valid_memory_id(MEMORY_ID), "Test memory ID is invalid")
	_require(WorldState.is_valid_fact_id(FACT_ID), "Test fact ID is invalid")
	_require(WorldState.memory_belongs_to_actor(MEMORY_ID, ACTOR_ID), "Memory owner slug does not match actor")
	_require(not WorldState.is_valid_actor_id("npc.Malet"), "Persistent IDs must be lowercase")
	_require(not WorldState.is_valid_actor_id("npc.2malet"), "Persistent ID slugs must begin with a letter")
	_require(not WorldState.is_valid_fact_id("fact.veil.double__gap"),
		"Persistent ID slugs must use single underscores")


func _check_memory_and_knowledge_separation() -> void:
	WorldState.reset_to_defaults()
	_require(WorldState.knows_fact(ACTOR_ID, FACT_ID), "Malet must know that the Veil exists")
	_require(MemoryEngine.add_memory(ACTOR_ID, MEMORY_ID, {
		"fact_ids": [FACT_ID],
		"source_actor_id": "player.arrel",
		"content": {"kind": "revelation_source"},
	}), "Test memory could not be added")
	_require(MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID), "Added memory must be active")

	EventBus.world_event_committed.connect(_on_world_event_committed)
	_committed_events.clear()
	_require(MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID), "Active memory could not be removed")
	_require(_committed_events.size() == 1, "remove_memory must commit exactly one world event")
	if _committed_events.size() == 1:
		_require(String(_committed_events[0].get("type", "")) == "memory.removed",
			"Committed event must describe the removal")
	_require(not MemoryEngine.check_memory(ACTOR_ID, MEMORY_ID), "Removed memory must not pass check_memory")
	_require(not MemoryEngine.remove_memory(ACTOR_ID, MEMORY_ID),
		"Removing an existing tombstone must be a no-op")
	_require(_committed_events.size() == 1,
		"A repeated remove_memory call must not commit another world event")

	var tombstone := WorldState.get_memory_record(ACTOR_ID, MEMORY_ID)
	_require(not tombstone.is_empty(), "remove_memory deleted the memory instead of retaining a tombstone")
	_require(String(tombstone.get("status", "")) == WorldState.MEMORY_STATUS_REMOVED,
		"Removed memory tombstone has the wrong status")
	_require(WorldState.knows_fact(ACTOR_ID, FACT_ID), "Removing source memory also removed knowledge")

	var memory_condition := {
		"type": "memory", "actor": ACTOR_ID, "memory": MEMORY_ID, "status": "active",
	}
	var knowledge_condition := {
		"type": "knowledge", "actor": ACTOR_ID, "fact": FACT_ID, "equals": true,
	}
	_require(not DialogueConditionSystem.evaluate(memory_condition), "Removed memory condition must be false")
	_require(DialogueConditionSystem.evaluate(knowledge_condition), "Knowledge condition must remain true")


func _check_save_reset_load_round_trip() -> void:
	var expected := SaveManager._export_world_state_for_save()
	var encoded := JSON.stringify({"world_state": expected})
	var parsed: Variant = JSON.parse_string(encoded)
	if not _require(parsed is Dictionary, "Serialized WorldState did not parse as a Dictionary"):
		return

	WorldState.reset_to_defaults()
	_require(WorldState.get_memory_record(ACTOR_ID, MEMORY_ID).is_empty(),
		"WorldState reset did not clear the runtime test memory")
	_require(SaveManager._restore_world_state_from_save_data(parsed),
		"SaveManager WorldState restore failed")
	_require(WorldState.export_data() == expected,
		"Save -> reset -> load did not restore an identical WorldState")
	_require(_committed_events.size() == 1,
		"Save/reset/load must not replay world_event_committed")


func _check_legacy_save_compatibility() -> void:
	var legacy_save := {
		"version": "0.3.0",
		"scene": "res://scenes/main/main.tscn",
		"game": {},
		"memory": {},
		"scene_flow": {},
		"elia_diary": {},
		"tutorial_hints": {},
		"player_pos": {},
	}
	var migrated := SaveManager._migrate_save_data(legacy_save)
	_require(String(migrated.get("version", "")) == SaveManager.SAVE_VERSION,
		"Legacy save version was not migrated")
	_require(migrated.get("world_state", null) is Dictionary,
		"Legacy save migration did not add a WorldState snapshot")
	WorldState.reset_to_defaults()
	_require(SaveManager._restore_world_state_from_save_data(migrated),
		"Migrated legacy save could not restore WorldState")
	_require(WorldState.knows_fact(ACTOR_ID, FACT_ID),
		"Legacy save default WorldState lost Malet's test knowledge")
	_require(_committed_events.size() == 1,
		"Legacy save migration/restore must not commit a world event")


func _on_world_event_committed(event: Dictionary) -> void:
	_committed_events.append(event.duplicate(true))


func _require(condition: bool, message: String) -> bool:
	if condition:
		return true
	_failures.append(message)
	push_error("[MemoryWorldEngineSmoke] %s" % message)
	return false
