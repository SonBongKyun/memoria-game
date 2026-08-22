## MemoryEngine (Autoload)
## NPC/world memory mutations. Existing player-card MemoryManager is separate
## and must not be called or mirrored from this MVP.
extends Node


func add_memory(actor_id: String, memory_id: String, payload: Dictionary = {}) -> bool:
	if not _valid_memory_target(actor_id, memory_id):
		return false
	if not WorldState.get_memory_record(actor_id, memory_id).is_empty():
		return false

	var fact_ids: Array = []
	var supplied_fact_ids: Variant = payload.get("fact_ids", [])
	if supplied_fact_ids is Array:
		for fact_value in supplied_fact_ids:
			var fact_id := String(fact_value)
			if not WorldState.is_valid_fact_id(fact_id):
				push_warning("[MemoryEngine] Invalid fact ID: %s" % fact_id)
				return false
			fact_ids.append(fact_id)

	var source_actor_id := String(payload.get("source_actor_id", ""))
	if source_actor_id != "" and not ActorRegistry.has_actor(source_actor_id):
		push_warning("[MemoryEngine] Invalid source actor ID: %s" % source_actor_id)
		return false

	var content: Dictionary = {}
	if payload.get("content", {}) is Dictionary:
		content = (payload.get("content", {}) as Dictionary).duplicate(true)
	var record := {
		"id": memory_id,
		"owner_actor_id": actor_id,
		"status": WorldState.MEMORY_STATUS_ACTIVE,
		"fact_ids": fact_ids,
		"source_actor_id": source_actor_id,
		"content": content,
		"created_revision": WorldState.get_revision() + 1,
		"removed_revision": 0,
		"last_removed_revision": 0,
		"restored_revision": 0,
	}
	var revision := WorldState._store_memory_record(actor_id, memory_id, record)
	if revision < 0:
		return false
	return _commit_event("memory.added", actor_id, memory_id, revision)


func remove_memory(actor_id: String, memory_id: String) -> bool:
	if not _valid_memory_target(actor_id, memory_id):
		return false
	var record := WorldState.get_memory_record(actor_id, memory_id)
	if record.is_empty() or String(record.get("status", "")) != WorldState.MEMORY_STATUS_ACTIVE:
		return false

	record["status"] = WorldState.MEMORY_STATUS_REMOVED
	record["removed_revision"] = WorldState.get_revision() + 1
	var revision := WorldState._store_memory_record(actor_id, memory_id, record)
	if revision < 0:
		return false
	return _commit_event("memory.removed", actor_id, memory_id, revision)


## Restores the latest valid pre-removal record kept inside the tombstone.
## Identity, content, facts, source, and created_revision are preserved. The
## current removed_revision moves to last_removed_revision, status becomes
## active, and restored_revision records this deterministic mutation.
func restore_memory(actor_id: String, memory_id: String) -> bool:
	if not _valid_memory_target(actor_id, memory_id):
		return false
	var record := WorldState.get_memory_record(actor_id, memory_id)
	if record.is_empty() or String(record.get("status", "")) != WorldState.MEMORY_STATUS_REMOVED:
		return false

	record["status"] = WorldState.MEMORY_STATUS_ACTIVE
	record["last_removed_revision"] = maxi(0, int(record.get("removed_revision", 0)))
	record["removed_revision"] = 0
	record["restored_revision"] = WorldState.get_revision() + 1
	var revision := WorldState._store_memory_record(actor_id, memory_id, record)
	if revision < 0:
		return false
	return _commit_event("memory.restored", actor_id, memory_id, revision)


func check_memory(actor_id: String, memory_id: String) -> bool:
	if not _valid_memory_target(actor_id, memory_id):
		return false
	var record := WorldState.get_memory_record(actor_id, memory_id)
	return not record.is_empty() and String(record.get("status", "")) == WorldState.MEMORY_STATUS_ACTIVE


func learn_fact(actor_id: String, fact_id: String) -> bool:
	if not _valid_knowledge_target(actor_id, fact_id) or WorldState.knows_fact(actor_id, fact_id):
		return false
	var revision := WorldState._store_knowledge_value(actor_id, fact_id, true)
	if revision < 0:
		return false
	return _commit_event("knowledge.learned", actor_id, fact_id, revision, "fact_id")


func forget_fact(actor_id: String, fact_id: String) -> bool:
	if not _valid_knowledge_target(actor_id, fact_id) or not WorldState.knows_fact(actor_id, fact_id):
		return false
	var revision := WorldState._store_knowledge_value(actor_id, fact_id, false)
	if revision < 0:
		return false
	return _commit_event("knowledge.forgotten", actor_id, fact_id, revision, "fact_id")


func knows_fact(actor_id: String, fact_id: String) -> bool:
	if not ActorRegistry.has_actor(actor_id) or not WorldState.is_valid_fact_id(fact_id):
		return false
	return WorldState.knows_fact(actor_id, fact_id)


func _valid_memory_target(actor_id: String, memory_id: String) -> bool:
	if not ActorRegistry.has_actor(actor_id) or not WorldState.has_actor(actor_id):
		push_warning("[MemoryEngine] Unknown actor ID: %s" % actor_id)
		return false
	if not WorldState.memory_belongs_to_actor(memory_id, actor_id):
		push_warning("[MemoryEngine] Memory ID does not belong to actor: %s -> %s" % [memory_id, actor_id])
		return false
	return true


func _valid_knowledge_target(actor_id: String, fact_id: String) -> bool:
	if not ActorRegistry.has_actor(actor_id) or not WorldState.has_actor(actor_id):
		push_warning("[MemoryEngine] Unknown actor ID: %s" % actor_id)
		return false
	if not WorldState.is_valid_fact_id(fact_id):
		push_warning("[MemoryEngine] Invalid fact ID: %s" % fact_id)
		return false
	return true


func _commit_event(event_type: String, actor_id: String, subject_id: String, revision: int,
		subject_key: String = "memory_id") -> bool:
	var sequence := WorldState._next_event_sequence()
	var event := {
		"event_id": "world.%08d" % sequence,
		"sequence": sequence,
		"type": event_type,
		"actor_id": actor_id,
		"world_revision": revision,
	}
	event[subject_key] = subject_id
	return EventBus.emit_world_event_committed(event)
