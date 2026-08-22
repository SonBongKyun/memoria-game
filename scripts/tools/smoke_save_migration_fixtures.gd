extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const FIXTURE_ROOT: String = "res://data/test_fixtures/save_migrations/"
const DEFAULT_FIXTURES: PackedStringArray = [
	"legacy_0_3_0.json",
	"missing_world_state_0_4_0.json",
	"corrupt_world_state_0_4_0.json",
	"unsupported_world_schema_0_4_0.json",
]

var _runner = SmokeTestRunner.new("save_migration_fixtures", "SAVE_MIGRATION_FIXTURES_SMOKE_PASS")
var _committed_event_count: int = 0


func _ready() -> void:
	Codex.suppress_recording = true
	EventBus.world_event_committed.connect(_on_world_event_committed)

	for fixture_name in DEFAULT_FIXTURES:
		_check_default_recovery_fixture(fixture_name)
	_check_current_fixture()
	_check_direct_schema_rejection()

	_runner.begin_test("no_user_save_side_effects")
	_expect(_committed_event_count == 0,
		"Fixture migration and restore must not emit world_event_committed")
	WorldState.reset_to_defaults()
	_runner.finish(get_tree(), "fixtures=5 user_slots_touched=0")


func _check_default_recovery_fixture(fixture_name: String) -> void:
	_runner.begin_test("fixture:%s" % fixture_name)
	var fixture := _load_fixture(fixture_name)
	if fixture.is_empty():
		return
	var migrated := SaveManager._migrate_save_data(fixture)
	_expect(String(migrated.get("version", "")) == SaveManager.SAVE_VERSION,
		"Migrated save must use the current save version")
	var snapshot: Variant = migrated.get("world_state", null)
	if not _expect(snapshot is Dictionary, "Migration must produce a WorldState dictionary"):
		return
	_expect(WorldState.is_supported_snapshot(snapshot),
		"Recovered WorldState must use the supported schema")
	_expect(SaveManager._restore_world_state_from_save_data(migrated),
		"Recovered WorldState must import without an error")
	_expect(WorldState.export_data() == WorldState.make_default_data(),
		"Missing, corrupt, or unsupported WorldState must recover to deterministic defaults")


func _check_current_fixture() -> void:
	const FIXTURE_NAME := "current_0_4_0.json"
	_runner.begin_test("fixture:%s" % FIXTURE_NAME)
	var fixture := _load_fixture(FIXTURE_NAME)
	if fixture.is_empty():
		return
	var original_snapshot: Dictionary = fixture.get("world_state", {}).duplicate(true)
	var migrated := SaveManager._migrate_save_data(fixture)
	_expect(migrated.get("world_state", {}) == original_snapshot,
		"A valid current WorldState snapshot must not be replaced during migration")
	_expect(SaveManager._restore_world_state_from_save_data(migrated),
		"Current WorldState fixture must restore")
	_expect(WorldState.get_revision() == 7, "Current fixture revision must be preserved")
	_expect(WorldState.get_event_sequence() == 3, "Current fixture event sequence must be preserved")
	_expect(not WorldState.knows_fact("npc.malet", "fact.veil.exists"),
		"Current fixture's explicit false knowledge value must be preserved")
	var restored := WorldState.export_data()
	_expect(bool((restored.get("world_flags", {}) as Dictionary).get("fixture.current_0_4_0", false)),
		"Current fixture world flags must be preserved")


func _check_direct_schema_rejection() -> void:
	_runner.begin_test("direct_unsupported_schema_rejection")
	var before := WorldState.export_data()
	_expect(not WorldState.import_data({"schema_version": 999, "actors": {}}),
		"WorldState must reject an unsupported schema when called directly")
	_expect(WorldState.export_data() == before,
		"A rejected direct import must leave the current WorldState unchanged")


func _load_fixture(fixture_name: String) -> Dictionary:
	var path := FIXTURE_ROOT + fixture_name
	if not _expect(FileAccess.file_exists(path), "Fixture file does not exist: %s" % path):
		return {}
	var file := FileAccess.open(path, FileAccess.READ)
	if not _expect(file != null, "Fixture file could not be opened: %s" % path):
		return {}
	var parsed: Variant = JSON.parse_string(file.get_as_text())
	if not _expect(parsed is Dictionary, "Fixture is not a valid JSON object: %s" % path):
		return {}
	return parsed


func _on_world_event_committed(_event: Dictionary) -> void:
	_committed_event_count += 1


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
