extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")

var _runner = SmokeTestRunner.new("actor_catalog", "ACTOR_CATALOG_SMOKE_PASS")


func _ready() -> void:
	Codex.suppress_recording = true
	_runner.begin_test("catalog_file_load")
	_expect(SaveManager.is_smoke_test_mode(),
		"Actor catalog smoke must run with --smoke-test save guards active")
	_expect(ActorRegistry.CATALOG_PATH == "res://data/world_state/actors.json",
		"ActorRegistry catalog path contract changed")
	_expect(FileAccess.file_exists(ActorRegistry.CATALOG_PATH),
		"Actor catalog is unavailable through the runtime res:// filesystem")
	_expect(ActorRegistry.reset_to_defaults(), "Actor catalog file failed to load")
	_expect(ActorRegistry.is_catalog_loaded(), "ActorRegistry did not mark the catalog as loaded")
	_expect(ActorRegistry.get_actor_ids() == PackedStringArray([
		"npc.malet", "npc.sable", "player.arrel"]),
		"Catalog must contain player.arrel, npc.malet, and npc.sable")
	_expect(String(ActorRegistry.get_actor("player.arrel").get("actor_id", "")) == "player.arrel",
		"Persistent identity must come from actor_id")
	_expect(ActorRegistry.get_display_name("npc.malet") == "Malet",
		"Display metadata did not load from the catalog")
	_expect(ActorRegistry.get_display_name("npc.sable") == "Sable",
		"Sable display metadata did not load from the catalog")

	var baseline := ActorRegistry.get_actor_ids()
	_runner.begin_test("catalog_failure_is_visible_and_atomic")
	_expect(not ActorRegistry.load_catalog_data({
		"schema_version": 1,
		"actors": [
			{"actor_id": "npc.malet", "display_name": "Malet"},
			{"actor_id": "npc.malet", "display_name": "Duplicate"},
		],
	}, "duplicate-fixture"), "Duplicate actor catalog must be rejected")
	_expect("duplicate actor_id" in ActorRegistry.get_last_load_error(),
		"Duplicate catalog failure must expose a precise error")
	_expect(ActorRegistry.get_actor_ids() == baseline,
		"Rejected catalog must not partially replace the live registry")

	_expect(not ActorRegistry.load_catalog_data({
		"schema_version": 1,
		"actors": [
			{"actor_id": "npc.malet", "display_name": "Malet"},
			{"actor_id": "player.malet", "display_name": "Other Malet"},
		],
	}, "namespace-collision-fixture"),
		"Actor slug collision across namespaces must reject the catalog")
	_expect("actor slug collision" in ActorRegistry.get_last_load_error(),
		"Namespace collision failure must expose a precise error")
	_expect(ActorRegistry.get_actor_ids() == baseline,
		"Namespace collision catalog must leave the live registry unchanged")

	_expect(not ActorRegistry.load_catalog_data({
		"schema_version": 1,
		"actors": [{"actor_id": "npc.Bad_ID", "display_name": "Invalid"}],
	}, "invalid-id-fixture"), "Invalid actor ID catalog must be rejected")
	_expect("invalid actor_id" in ActorRegistry.get_last_load_error(),
		"Invalid ID failure must expose a precise error")
	_expect(ActorRegistry.get_actor_ids() == baseline,
		"Invalid ID catalog must leave the live registry unchanged")

	_expect(not ActorRegistry.load_catalog(
		"res://data/world_state/__missing_actor_catalog_fixture__.json"),
		"Missing catalog must be reported as a failure")
	_expect("does not exist" in ActorRegistry.get_last_load_error(),
		"Missing catalog failure must remain observable")
	_expect(ActorRegistry.get_actor_ids() == baseline,
		"Missing catalog must not silently replace the live registry")

	_expect(ActorRegistry.reset_to_defaults(), "Actor catalog could not be restored after rejection tests")
	WorldState.reset_to_defaults()
	_runner.finish(get_tree(),
		"actors=3 schema=1 runtime_path=res://data/world_state/actors.json atomic_rejection=true")


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
