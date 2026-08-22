extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")

var _runner = SmokeTestRunner.new("actor_registry", "ACTOR_REGISTRY_SMOKE_PASS")


func _ready() -> void:
	Codex.suppress_recording = true
	ActorRegistry.reset_to_defaults()
	WorldState.reset_to_defaults()

	_runner.begin_test("default_actor_contract")
	_expect(ActorRegistry.has_actor("player.arrel"), "player.arrel must be registered")
	_expect(ActorRegistry.has_actor("npc.malet"), "npc.malet must be registered")
	_expect(ActorRegistry.get_display_name("player.arrel") == "Arrel",
		"Persistent player ID and display name must remain separate")
	_expect(String(ActorRegistry.get_actor("npc.malet").get("namespace", "")) == "npc",
		"Actor metadata must retain its namespace")

	_runner.begin_test("collision_rejection")
	_expect(not ActorRegistry.register_actor("npc.malet", "Other Malet"),
		"An exact duplicate actor ID must be rejected")
	_expect(not ActorRegistry.register_actor("player.malet", "Player Malet"),
		"The same actor slug across namespaces must be rejected because memory IDs omit namespace")
	_expect(ActorRegistry.get_actor_ids().size() == 2,
		"Rejected registrations must not mutate the registry")

	_runner.begin_test("display_name_is_not_identity")
	_expect(ActorRegistry.register_actor("npc.archive_witness", "Malet"),
		"A duplicate display name with a unique persistent ID must be allowed")
	_expect(ActorRegistry.get_display_name("npc.archive_witness") == "Malet",
		"Display name must be stored independently from persistent identity")
	WorldState.reset_to_defaults()
	_expect(WorldState.has_actor("npc.archive_witness"),
		"WorldState defaults must derive actor containers from ActorRegistry")

	_runner.begin_test("unknown_actor_access")
	_expect(not ActorRegistry.has_actor("npc.unknown"), "Unknown actors must not pass has_actor")
	_expect(ActorRegistry.get_actor("npc.unknown").is_empty(),
		"Unknown actor metadata access must return an empty record")
	_expect(ActorRegistry.get_display_name("npc.unknown") == "",
		"Unknown actor display access must return an empty string")
	_expect(not ActorRegistry.register_actor("npc.Bad_ID", "Bad"),
		"Invalid persistent IDs must be rejected")

	ActorRegistry.reset_to_defaults()
	WorldState.reset_to_defaults()
	_runner.finish(get_tree(), "defaults=2 collisions=2 unknown_access=validated")


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
