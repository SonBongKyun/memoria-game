extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")
const SmokeSaveSandbox = preload("res://scripts/tools/smoke_save_sandbox.gd")
const SELF_SCENE := "res://scripts/tools/smoke_crash_guards.tscn"
const PHASE_META := "crash_guard_smoke_phase"

var _runner = SmokeTestRunner.new("crash_guards_isolated", "CRASH_GUARDS_ISOLATED_SMOKE_PASS")
var _save_failed_emitted := false
var _load_completed_emitted := false


func _ready() -> void:
	Codex.suppress_recording = true
	if not SmokeSaveSandbox.activate("crash_guards", _runner):
		_runner.finish(get_tree())
		return

	await get_tree().process_frame
	if not SceneTransition.has_meta(PHASE_META):
		_test_transition_mutex_helpers()
		if _runner.failure_count() > 0:
			_runner.finish(get_tree())
			return
		SceneTransition.set_meta(PHASE_META, "public_transition")
		SceneTransition.change_scene(SELF_SCENE, 0.05)
		SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn", 0.05)
		return

	await get_tree().create_timer(0.25).timeout
	_runner.begin_test("public_transition_cleanup")
	_expect(not bool(SceneTransition.get("_transition_in_progress")),
		"Completed public transition must release the guard")
	_expect(String(SceneTransition.get("_transition_target")).is_empty(),
		"Completed public transition must clear its destination")
	_expect(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_IGNORE,
		"Completed transition must restore normal input routing")
	SceneTransition.remove_meta(PHASE_META)
	_test_invalid_save_is_transactional()
	_runner.finish(get_tree(),
		"transition_mutex=true transactional_load=true isolated_save_root=true")


func _test_transition_mutex_helpers() -> void:
	_runner.begin_test("transition_mutex_helpers")
	var first_target := "res://scenes/main/main.tscn"
	var competing_target := "res://scenes/battle/battle_scene.tscn"
	_expect(bool(SceneTransition.call("_begin_scene_transition", first_target)),
		"First scene transition should acquire the guard")
	_expect(bool(SceneTransition.get("_transition_in_progress")),
		"Transition guard should report an active owner")
	_expect(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_STOP,
		"Active transition must block duplicate pointer input")
	_expect(not bool(SceneTransition.call("_begin_scene_transition", competing_target)),
		"Overlapping scene transition must be rejected")
	_expect(String(SceneTransition.get("_transition_target")) == first_target,
		"Competing request must not replace the active destination")
	SceneTransition.call("_abort_scene_transition")
	_expect(not bool(SceneTransition.get("_transition_in_progress")),
		"Aborting must release the transition guard")
	_expect(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_IGNORE,
		"Aborting must restore normal input routing")
	_expect(bool(SceneTransition.call("_begin_scene_transition", competing_target)),
		"Transition guard must be reusable after cleanup")
	SceneTransition.call("_abort_scene_transition")


func _test_invalid_save_is_transactional() -> void:
	_runner.begin_test("isolated_invalid_save_transaction")
	var slot := SaveManager.MAX_SLOTS
	var path := SmokeSaveSandbox.get_slot_path(slot, _runner)
	_expect(SaveManager.save_game(slot),
		"SaveManager could not write a normal save inside the isolated test root")
	_expect(SaveManager.has_save(slot),
		"SaveManager could not read its isolated slot after writing it")
	var invalid_data := {
		"version": "0.3.0",
		"timestamp": "crash-guard-smoke",
		"scene": "res://scenes/maps/missing_crash_guard.tscn",
		"game": {"current_chapter": 99},
	}
	if not SmokeSaveSandbox.write_json(path, invalid_data, _runner):
		return

	var previous_chapter := GameManager.current_chapter
	_save_failed_emitted = false
	_load_completed_emitted = false
	SaveManager.save_failed.connect(_on_save_failed, CONNECT_ONE_SHOT)
	SaveManager.load_completed.connect(_on_load_completed, CONNECT_ONE_SHOT)
	_expect(not SaveManager.load_game(slot), "A save pointing to a missing scene must fail")
	_expect(_save_failed_emitted, "Invalid scene save must report save_failed")
	_expect(not _load_completed_emitted, "Invalid scene save must not report load_completed")
	_expect(GameManager.current_chapter == previous_chapter,
		"Invalid scene save must not partially import game state")
	_expect(not SaveManager.has_save_path_guard_failed(),
		"Isolated crash guard smoke triggered the production save path guard")


func _on_save_failed(_message: String) -> void:
	_save_failed_emitted = true


func _on_load_completed(_slot: int) -> void:
	_load_completed_emitted = true


func _expect(condition: bool, reason: String) -> bool:
	return _runner.expect(condition, reason)
