## ActorRegistry (Autoload)
## Persistent actor identity catalog. Runtime state belongs to WorldState.
extends Node

const CANONICAL_PLAYER_ACTOR_ID: String = "player.arrel"
const VALID_NAMESPACES: PackedStringArray = ["player", "npc"]
const DEFAULT_ACTORS: Array[Dictionary] = [
	{"id": CANONICAL_PLAYER_ACTOR_ID, "display_name": "Arrel"},
	{"id": "npc.malet", "display_name": "Malet"},
]

var _actors: Dictionary = {}
var _actor_ids_by_slug: Dictionary = {}


func _ready() -> void:
	reset_to_defaults()
	print("[ActorRegistry] Ready, %d actors registered" % _actors.size())


func reset_to_defaults() -> void:
	_actors.clear()
	_actor_ids_by_slug.clear()
	for actor_definition in DEFAULT_ACTORS:
		if not _register_actor(String(actor_definition.id), String(actor_definition.display_name)):
			push_error("[ActorRegistry] Invalid built-in actor: %s" % actor_definition)


func register_actor(actor_id: String, display_name: String) -> bool:
	return _register_actor(actor_id, display_name)


func has_actor(actor_id: String) -> bool:
	return _actors.has(actor_id)


func get_actor(actor_id: String) -> Dictionary:
	if not has_actor(actor_id):
		return {}
	return (_actors[actor_id] as Dictionary).duplicate(true)


func get_actor_ids() -> PackedStringArray:
	var actor_ids := PackedStringArray()
	for actor_id in _actors:
		actor_ids.append(String(actor_id))
	actor_ids.sort()
	return actor_ids


func get_display_name(actor_id: String) -> String:
	return String((_actors.get(actor_id, {}) as Dictionary).get("display_name", ""))


func is_valid_actor_id(actor_id: String) -> bool:
	var parts := actor_id.split(".")
	return parts.size() == 2 and parts[0] in VALID_NAMESPACES and is_valid_slug(parts[1])


func is_valid_slug(value: String) -> bool:
	if value == "" or value.ends_with("_") or "__" in value:
		return false
	var first_code := value.unicode_at(0)
	if first_code < 97 or first_code > 122:
		return false
	for i in range(value.length()):
		var code := value.unicode_at(i)
		var is_lower_ascii := code >= 97 and code <= 122
		var is_digit := code >= 48 and code <= 57
		if not is_lower_ascii and not is_digit and code != 95:
			return false
	return true


func _register_actor(actor_id: String, display_name: String) -> bool:
	if not is_valid_actor_id(actor_id):
		push_warning("[ActorRegistry] Rejected invalid actor ID: %s" % actor_id)
		return false
	var normalized_display_name := display_name.strip_edges()
	if normalized_display_name == "":
		push_warning("[ActorRegistry] Rejected empty display name for: %s" % actor_id)
		return false
	if _actors.has(actor_id):
		push_warning("[ActorRegistry] Rejected duplicate actor ID: %s" % actor_id)
		return false

	# Memory IDs use memory.<actor_slug>.<memory_slug>, so actor slugs must be
	# unique even across the player/npc namespaces.
	var parts := actor_id.split(".")
	var actor_slug := String(parts[1])
	if _actor_ids_by_slug.has(actor_slug):
		push_warning("[ActorRegistry] Rejected actor slug collision: %s conflicts with %s" % [
			actor_id, String(_actor_ids_by_slug[actor_slug]),
		])
		return false

	_actors[actor_id] = {
		"id": actor_id,
		"namespace": String(parts[0]),
		"slug": actor_slug,
		"display_name": normalized_display_name,
	}
	_actor_ids_by_slug[actor_slug] = actor_id
	return true
