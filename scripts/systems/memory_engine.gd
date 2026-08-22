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
	if source_actor_id != "" and not WorldState.is_valid_actor_id(source_actor_id):
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


func check_memory(actor_id: String, memory_id: String) -> bool:
	if not _valid_memory_target(actor_id, memory_id):
		return false
	var record := WorldState.get_memory_record(actor_id, memory_id)
	return not record.is_empty() and String(record.get("status", "")) == WorldState.MEMORY_STATUS_ACTIVE


func _valid_memory_target(actor_id: String, memory_id: String) -> bool:
	if not WorldState.has_actor(actor_id):
		push_warning("[MemoryEngine] Unknown actor ID: %s" % actor_id)
		return false
	if not WorldState.memory_belongs_to_actor(memory_id, actor_id):
		push_warning("[MemoryEngine] Memory ID does not belong to actor: %s -> %s" % [memory_id, actor_id])
		return false
	return true


func _commit_event(event_type: String, actor_id: String, memory_id: String, revision: int) -> bool:
	var sequence := WorldState._next_event_sequence()
	var event := {
		"event_id": "world.%08d" % sequence,
		"sequence": sequence,
		"type": event_type,
		"actor_id": actor_id,
		"memory_id": memory_id,
		"world_revision": revision,
	}
	return EventBus.emit_world_event_committed(event)
