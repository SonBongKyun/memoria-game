## WorldState (Autoload)
## Memory World Engine의 JSON-safe persistent snapshot.
##
## Stable ID contract:
##   Actor  = player.<slug> | npc.<slug>       (canonical player: player.arrel)
##   Memory = memory.<actor_slug>.<memory_slug>
##   Fact   = fact.<domain_slug>.<fact_slug>
##
## Slugs begin with a-z and use lowercase ASCII letters, digits, and single
## underscores between non-empty segments. Display names and scene-node names
## are never persistent IDs. Memory IDs repeat only the actor slug because the
## full owner_actor_id is also stored and validated on every mutation.
extends Node

const SCHEMA_VERSION: int = 1
const MEMORY_STATUS_ACTIVE: String = "active"
const MEMORY_STATUS_REMOVED: String = "removed"

# Canon Migration Wave 2A compatibility. The retired fact encoded Arrel's
# identity even when Malet's source memory had been removed. Old records stay
# intact; import only adds the identity-free canonical event fact beside them.
const LEGACY_MALET_ROUTE_FACT_ID: String = "fact.arrel.seeks_bl07"
const CANONICAL_MALET_ROUTE_FACT_ID: String = "fact.bl07.route_request_received"

const DEFAULT_STATE_BASE: Dictionary = {
	"schema_version": SCHEMA_VERSION,
	"revision": 0,
	"event_sequence": 0,
	"actors": {},
	"world_flags": {},
	"quest_states": {},
}

var _state: Dictionary = {}


func _ready() -> void:
	reset_to_defaults()
	print("[WorldState] Ready, schema v%d" % SCHEMA_VERSION)


func reset_to_defaults() -> void:
	_state = make_default_data()


func make_default_data() -> Dictionary:
	var default_state := DEFAULT_STATE_BASE.duplicate(true)
	for actor_id in ActorRegistry.get_actor_ids():
		default_state["actors"][actor_id] = _make_empty_actor_state()
	var malet: Dictionary = default_state["actors"].get("npc.malet", {})
	if not malet.is_empty():
		malet["knowledge"]["fact.veil.exists"] = {
			"fact_id": "fact.veil.exists",
			"value": true,
			"updated_revision": 0,
		}
	return default_state


func export_data() -> Dictionary:
	return _state.duplicate(true)


func import_data(data: Dictionary) -> bool:
	if not is_supported_snapshot(data):
		return false
	_state = _normalize_state(data)
	return true


func is_supported_snapshot(data: Dictionary) -> bool:
	return int(data.get("schema_version", -1)) == SCHEMA_VERSION and data.get("actors", null) is Dictionary


func has_actor(actor_id: String) -> bool:
	return ActorRegistry.has_actor(actor_id) and _state.get("actors", {}).has(actor_id)


func get_actor_state(actor_id: String) -> Dictionary:
	if not has_actor(actor_id):
		return {}
	return (_state["actors"][actor_id] as Dictionary).duplicate(true)


func get_memory_record(actor_id: String, memory_id: String) -> Dictionary:
	if not has_actor(actor_id) or not is_valid_memory_id(memory_id):
		return {}
	var actor: Dictionary = _state["actors"][actor_id]
	var memories: Dictionary = actor.get("memories", {})
	if not memories.has(memory_id) or not (memories[memory_id] is Dictionary):
		return {}
	return (memories[memory_id] as Dictionary).duplicate(true)


func knows_fact(actor_id: String, fact_id: String) -> bool:
	if not has_actor(actor_id) or not is_valid_fact_id(fact_id):
		return false
	var actor: Dictionary = _state["actors"][actor_id]
	var knowledge: Dictionary = actor.get("knowledge", {})
	if not knowledge.has(fact_id) or not (knowledge[fact_id] is Dictionary):
		return false
	return bool((knowledge[fact_id] as Dictionary).get("value", false))


func get_revision() -> int:
	return int(_state.get("revision", 0))


func get_event_sequence() -> int:
	return int(_state.get("event_sequence", 0))


## MemoryEngine 전용 mutation boundary. 검증을 마친 완전한 레코드만 교체한다.
func _store_memory_record(actor_id: String, memory_id: String, record: Dictionary) -> int:
	if not has_actor(actor_id) or not memory_belongs_to_actor(memory_id, actor_id):
		return -1
	var actor: Dictionary = _state["actors"][actor_id]
	var memories: Dictionary = actor.get("memories", {})
	memories[memory_id] = record.duplicate(true)
	actor["memories"] = memories
	_state["revision"] = get_revision() + 1
	return get_revision()


## MemoryEngine 전용 knowledge mutation boundary. Missing knowledge and an
## explicit false value remain distinguishable in the serialized snapshot.
func _store_knowledge_value(actor_id: String, fact_id: String, value: bool) -> int:
	if not has_actor(actor_id) or not is_valid_fact_id(fact_id):
		return -1
	var actor: Dictionary = _state["actors"][actor_id]
	var knowledge: Dictionary = actor.get("knowledge", {})
	var revision := get_revision() + 1
	knowledge[fact_id] = {
		"fact_id": fact_id,
		"value": value,
		"updated_revision": revision,
	}
	actor["knowledge"] = knowledge
	_state["revision"] = revision
	return revision


## MemoryEngine 전용 deterministic sequence allocator.
func _next_event_sequence() -> int:
	_state["event_sequence"] = get_event_sequence() + 1
	return get_event_sequence()


func is_valid_actor_id(actor_id: String) -> bool:
	return ActorRegistry.is_valid_actor_id(actor_id)


func is_valid_memory_id(memory_id: String) -> bool:
	var parts := memory_id.split(".")
	return parts.size() == 3 and parts[0] == "memory" and ActorRegistry.is_valid_slug(parts[1]) and ActorRegistry.is_valid_slug(parts[2])


func is_valid_fact_id(fact_id: String) -> bool:
	var parts := fact_id.split(".")
	return parts.size() == 3 and parts[0] == "fact" and ActorRegistry.is_valid_slug(parts[1]) and ActorRegistry.is_valid_slug(parts[2])


func memory_belongs_to_actor(memory_id: String, actor_id: String) -> bool:
	if not is_valid_memory_id(memory_id) or not is_valid_actor_id(actor_id):
		return false
	return memory_id.split(".")[1] == actor_id.split(".")[1]

func _normalize_state(data: Dictionary) -> Dictionary:
	var normalized := make_default_data()
	normalized["schema_version"] = SCHEMA_VERSION
	normalized["revision"] = maxi(0, int(data.get("revision", 0)))
	normalized["event_sequence"] = maxi(0, int(data.get("event_sequence", 0)))
	normalized["world_flags"] = _dictionary_copy(data.get("world_flags", {}))
	normalized["quest_states"] = _dictionary_copy(data.get("quest_states", {}))

	var imported_actors: Variant = data.get("actors", {})
	if imported_actors is Dictionary:
		for actor_key in imported_actors:
			var actor_id := String(actor_key)
			var actor_data: Variant = imported_actors[actor_key]
			if not ActorRegistry.has_actor(actor_id) or not (actor_data is Dictionary):
				continue
			normalized["actors"][actor_id] = _normalize_actor(actor_id, actor_data)
	return normalized


func _normalize_actor(actor_id: String, actor_data: Dictionary) -> Dictionary:
	var actor := _make_empty_actor_state()
	actor["location"] = _dictionary_copy(actor_data.get("location", {}))
	actor["relationships"] = _dictionary_copy(actor_data.get("relationships", {}))
	actor["emotions"] = _dictionary_copy(actor_data.get("emotions", {}))
	actor["quest_state"] = _dictionary_copy(actor_data.get("quest_state", {}))
	actor["flags"] = _dictionary_copy(actor_data.get("flags", {}))

	var imported_memories: Variant = actor_data.get("memories", {})
	if imported_memories is Dictionary:
		for memory_key in imported_memories:
			var memory_id := String(memory_key)
			var record: Variant = imported_memories[memory_key]
			if not memory_belongs_to_actor(memory_id, actor_id) or not (record is Dictionary):
				continue
			actor["memories"][memory_id] = _normalize_memory_record(actor_id, memory_id, record)

	var imported_knowledge: Variant = actor_data.get("knowledge", {})
	if imported_knowledge is Dictionary:
		for fact_key in imported_knowledge:
			var fact_id := String(fact_key)
			var knowledge_record: Variant = imported_knowledge[fact_key]
			if not is_valid_fact_id(fact_id) or not (knowledge_record is Dictionary):
				continue
			actor["knowledge"][fact_id] = {
				"fact_id": fact_id,
				"value": bool(knowledge_record.get("value", false)),
				"updated_revision": maxi(0, int(knowledge_record.get("updated_revision", 0))),
			}
	_apply_malet_route_fact_compatibility(actor_id, actor)
	return actor


func _normalize_memory_record(actor_id: String, memory_id: String, record: Dictionary) -> Dictionary:
	var status := String(record.get("status", MEMORY_STATUS_ACTIVE))
	if status not in [MEMORY_STATUS_ACTIVE, MEMORY_STATUS_REMOVED]:
		status = MEMORY_STATUS_ACTIVE

	var fact_ids: Array = []
	var imported_fact_ids: Variant = record.get("fact_ids", [])
	if imported_fact_ids is Array:
		for fact_value in imported_fact_ids:
			var fact_id := String(fact_value)
			if is_valid_fact_id(fact_id) and fact_id not in fact_ids:
				fact_ids.append(fact_id)
	if actor_id == "npc.malet" \
			and LEGACY_MALET_ROUTE_FACT_ID in fact_ids \
			and CANONICAL_MALET_ROUTE_FACT_ID not in fact_ids:
		fact_ids.append(CANONICAL_MALET_ROUTE_FACT_ID)

	var source_actor_id := String(record.get("source_actor_id", ""))
	if source_actor_id != "" and not ActorRegistry.has_actor(source_actor_id):
		source_actor_id = ""

	return {
		"id": memory_id,
		"owner_actor_id": actor_id,
		"status": status,
		"fact_ids": fact_ids,
		"source_actor_id": source_actor_id,
		"content": _dictionary_copy(record.get("content", {})),
		"created_revision": maxi(0, int(record.get("created_revision", 0))),
		"removed_revision": maxi(0, int(record.get("removed_revision", 0))),
		"last_removed_revision": maxi(0, int(record.get("last_removed_revision", 0))),
		"restored_revision": maxi(0, int(record.get("restored_revision", 0))),
	}


func _make_empty_actor_state() -> Dictionary:
	return {
		"location": {},
		"memories": {},
		"knowledge": {},
		"relationships": {},
		"emotions": {},
		"quest_state": {},
		"flags": {},
	}


## Additive compatibility for saves written before the BL-07 request event and
## requester identity were separated. No revision/event is allocated and the
## legacy key remains available to older code or development saves.
func _apply_malet_route_fact_compatibility(actor_id: String, actor: Dictionary) -> void:
	if actor_id != "npc.malet":
		return
	var knowledge: Dictionary = actor.get("knowledge", {})
	if not knowledge.has(LEGACY_MALET_ROUTE_FACT_ID) \
			or knowledge.has(CANONICAL_MALET_ROUTE_FACT_ID):
		return
	var legacy_record: Variant = knowledge[LEGACY_MALET_ROUTE_FACT_ID]
	if not (legacy_record is Dictionary):
		return
	knowledge[CANONICAL_MALET_ROUTE_FACT_ID] = {
		"fact_id": CANONICAL_MALET_ROUTE_FACT_ID,
		"value": bool(legacy_record.get("value", false)),
		"updated_revision": maxi(0, int(legacy_record.get("updated_revision", 0))),
	}
	actor["knowledge"] = knowledge


func _dictionary_copy(value: Variant) -> Dictionary:
	return (value as Dictionary).duplicate(true) if value is Dictionary else {}
