## Shared assertion harness for focused Godot smoke scenes.
## Unlike Godot assert(), a recorded failure always finishes with exit code 1.
extends RefCounted

var _suite_name: String
var _pass_marker: String
var _current_test: String = "unscoped"
var _failures: Array[Dictionary] = []


func _init(suite_name: String, pass_marker: String) -> void:
	_suite_name = suite_name
	_pass_marker = pass_marker


func begin_test(test_name: String) -> void:
	_current_test = test_name if test_name != "" else "unscoped"


func expect(condition: bool, reason: String) -> bool:
	if condition:
		return true
	var failure := {"test": _current_test, "reason": reason}
	_failures.append(failure)
	push_error("[SMOKE][FAIL][%s][%s] %s" % [_suite_name, _current_test, reason])
	return false


func failure_count() -> int:
	return _failures.size()


func finish(tree: SceneTree, success_details: String = "") -> bool:
	if not _failures.is_empty():
		push_error("[SMOKE][SUITE_FAIL][%s] failures=%d" % [_suite_name, _failures.size()])
		tree.quit(1)
		return false
	var details := " " + success_details if success_details != "" else ""
	print("%s suite=%s%s" % [_pass_marker, _suite_name, details])
	tree.quit(0)
	return true
