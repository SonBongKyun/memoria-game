## ActorRegistry (Autoload)
## Persistent actor identity catalog. Runtime state belongs to WorldState.
extends Node

const CATALOG_PATH: String = "res://data/world_state/actors.json"
const CATALOG_SCHEMA_VERSION: int = 1
const CANONICAL_PLAYER_ACTOR_ID: String = "player.arrel"
const VALID_NAMESPACES: PackedStringArray = ["player", "npc"]

var _actors: Dictionary = {}
var _actor_ids_by_slug: Dictionary = {}
var _catalog_loaded: bool = false
var _last_load_error: String = ""


func _ready() -> void:
	if not reset_to_defaults():
		push_error("[ActorRegistry] Catalog bootstrap failed: %s" % _last_load_error)
		get_tree().quit(1)
		return
	print("[ActorRegistry] Ready, %d actors loaded from %s" % [_actors.size(), CATALOG_PATH])


func reset_to_defaults() -> bool:
	return load_catalog(CATALOG_PATH)


func load_catalog(path: String = CATALOG_PATH) -> bool:
	if not FileAccess.file_exists(path):
		return _reject_catalog("Catalog file does not exist: %s" % path)
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return _reject_catalog("Catalog file could not be opened: %s" % path)
	var json := JSON.new()
	var parse_error := json.parse(file.get_as_text())
	file.close()
	if parse_error != OK or not (json.data is Dictionary):
		return _reject_catalog("Catalog is not a valid JSON object: %s" % path)
	return load_catalog_data(json.data, path)


## Validates into temporary dictionaries and swaps atomically only on success.
## Tests can pass synthetic data without writing a catalog fixture to disk.
func load_catalog_data(data: Dictionary, source: String = "<memory>") -> bool:
	if int(data.get("schema_version", -1)) != CATALOG_SCHEMA_VERSION:
		return _reject_catalog("Unsupported actor catalog schema in %s" % source)
	var definitions: Variant = data.get("actors", null)
	if not (definitions is Array) or definitions.is_empty():
		return _reject_catalog("Actor catalog must contain a non-empty actors array: %s" % source)

	var candidate_actors: Dictionary = {}
	var candidate_slugs: Dictionary = {}
	for index in range(definitions.size()):
		var definition: Variant = definitions[index]
		if not (definition is Dictionary):
			return _reject_catalog("Actor entry %d is not an object: %s" % [index, source])
		var actor_id := String(definition.get("actor_id", ""))
		var display_name := String(definition.get("display_name", "")).strip_edges()
		var validation_error := _validate_registration(
			actor_id, display_name, candidate_actors, candidate_slugs)
		if validation_error != "":
			return _reject_catalog("Actor entry %d rejected in %s: %s" % [
				index, source, validation_error,
			])
		_insert_actor(actor_id, display_name, candidate_actors, candidate_slugs)

	_actors = candidate_actors
	_actor_ids_by_slug = candidate_slugs
	_catalog_loaded = true
	_last_load_error = ""
	return true


func register_actor(actor_id: String, display_name: String) -> bool:
	var normalized_display_name := display_name.strip_edges()
	var validation_error := _validate_registration(
		actor_id, normalized_display_name, _actors, _actor_ids_by_slug)
	if validation_error != "":
		push_warning("[ActorRegistry] Registration rejected: %s" % validation_error)
		return false
	_insert_actor(actor_id, normalized_display_name, _actors, _actor_ids_by_slug)
	return true


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


func is_catalog_loaded() -> bool:
	return _catalog_loaded


func get_last_load_error() -> String:
	return _last_load_error


func is_valid_actor_id(actor_id: String) -> bool:
	var parts := actor_id.split(".")
	return parts.size() == 2 and parts[0] in VALID_NAMESPACES and is_valid_slug(parts[1])


func is_valid_slug(value: String) -> bool:
	if value == "" or value.ends_with("_") or "__" in value:
		return false
	var first_code := value.unicode_at(0)
	if first_code < 97 or first_code > 122:
		return false
	for index in range(value.length()):
		var code := value.unicode_at(index)
		var is_lower_ascii := code >= 97 and code <= 122
		var is_digit := code >= 48 and code <= 57
		if not is_lower_ascii and not is_digit and code != 95:
			return false
	return true


func _validate_registration(actor_id: String, display_name: String,
		actors: Dictionary, actor_ids_by_slug: Dictionary) -> String:
	if not is_valid_actor_id(actor_id):
		return "invalid actor_id=%s" % actor_id
	if display_name == "":
		return "empty display_name for actor_id=%s" % actor_id
	if actors.has(actor_id):
		return "duplicate actor_id=%s" % actor_id

	# Memory IDs use memory.<actor_slug>.<memory_slug>, so actor slugs must be
	# unique even across the player/npc namespaces.
	var actor_slug := String(actor_id.split(".")[1])
	if actor_ids_by_slug.has(actor_slug):
		return "actor slug collision: %s conflicts with %s" % [
			actor_id, String(actor_ids_by_slug[actor_slug]),
		]
	return ""


func _insert_actor(actor_id: String, display_name: String,
		actors: Dictionary, actor_ids_by_slug: Dictionary) -> void:
	var parts := actor_id.split(".")
	var actor_slug := String(parts[1])
	actors[actor_id] = {
		"actor_id": actor_id,
		"namespace": String(parts[0]),
		"slug": actor_slug,
		"display_name": display_name,
	}
	actor_ids_by_slug[actor_slug] = actor_id


func _reject_catalog(reason: String) -> bool:
	_last_load_error = reason
	push_warning("[ActorRegistry] Catalog rejected: %s" % reason)
	return false
