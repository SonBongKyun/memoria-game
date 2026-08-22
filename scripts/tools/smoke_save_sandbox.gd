## Shared save sandbox for Godot smoke scenes.
##
## The sandbox never reads, fingerprints, backs up, or writes the production
## save root. A process-specific directory below SaveManager's fixed test
## prefix is injected before any synthetic save operation.
extends RefCounted


static func activate(suite_name: String, runner: RefCounted) -> bool:
	runner.begin_test("save_sandbox_activation")
	if not runner.expect(SaveManager.is_smoke_test_mode(),
			"Save smoke must run with the production path guard enabled"):
		return false
	if SaveManager.is_test_save_root_configured():
		return runner.expect(_active_root_is_safe(),
			"Existing test save root is outside the permitted temp prefix")

	var safe_suite_name := _slugify(suite_name)
	var root := "%s/%s_%d" % [
		SaveManager.TEST_SAVE_ROOT_PREFIX,
		safe_suite_name,
		OS.get_process_id(),
	]
	if not SaveManager.configure_test_save_root(root):
		return runner.expect(false, "SaveManager rejected the isolated temp root: %s" % root)
	return runner.expect(_active_root_is_safe(),
		"Configured test save root is outside the permitted temp prefix")


static func get_slot_path(slot: int, runner: RefCounted) -> String:
	var path := SaveManager.get_save_path_for_test(slot)
	if not runner.expect(path != "", "SaveManager did not resolve an isolated slot path"):
		return ""
	if not runner.expect(SaveManager.guard_test_write_target(path),
			"Resolved slot path failed the test write guard: %s" % path):
		return ""
	var production_path := _production_slot_path(slot)
	runner.expect(_absolute(path) != _absolute(production_path),
		"Isolated slot resolved to the production save path: %s" % path)
	return path


static func write_json(target_path: String, data: Dictionary, runner: RefCounted) -> bool:
	if target_path == "":
		return false
	if not SaveManager.guard_test_write_target(target_path):
		return runner.expect(false, "Unsafe smoke write target: %s" % target_path)
	var file := FileAccess.open(target_path, FileAccess.WRITE)
	if not runner.expect(file != null, "Could not open isolated smoke target: %s" % target_path):
		return false
	file.store_string(JSON.stringify(data))
	file.close()
	return true


static func _active_root_is_safe() -> bool:
	var active_root := _absolute(SaveManager.get_active_save_root())
	var allowed_prefix := _absolute(SaveManager.TEST_SAVE_ROOT_PREFIX)
	var production_root := _absolute(SaveManager.PRODUCTION_SAVE_ROOT)
	return active_root.begins_with(allowed_prefix + "/") \
		and active_root != production_root \
		and not active_root.begins_with(production_root + "/")


static func _production_slot_path(slot: int) -> String:
	var filename := "autosave.json" if slot == SaveManager.AUTOSAVE_SLOT else "save_%d.json" % slot
	return SaveManager.PRODUCTION_SAVE_ROOT + "/" + filename


static func _absolute(path: String) -> String:
	return ProjectSettings.globalize_path(path).replace("\\", "/").simplify_path().trim_suffix("/").to_lower()


static func _slugify(value: String) -> String:
	var result := ""
	var normalized := value.to_lower()
	for index in range(normalized.length()):
		var character := normalized.substr(index, 1)
		var code := normalized.unicode_at(index)
		var valid := (code >= 97 and code <= 122) or (code >= 48 and code <= 57)
		result += character if valid else "_"
	result = result.strip_edges().trim_prefix("_").trim_suffix("_")
	return result if result != "" else "smoke"
