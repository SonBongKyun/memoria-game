extends Node

const SELF_SCENE := "res://scripts/tools/smoke_crash_guards.tscn"
const PHASE_META := "crash_guard_smoke_phase"

var _save_failed_emitted := false
var _load_completed_emitted := false

func _ready() -> void:
	await get_tree().process_frame
	if not SceneTransition.has_meta(PHASE_META):
		_test_transition_mutex_helpers()
		SceneTransition.set_meta(PHASE_META, "public_transition")
		SceneTransition.change_scene(SELF_SCENE, 0.05)
		SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn", 0.05)
		return

	await get_tree().create_timer(0.25).timeout
	assert(not bool(SceneTransition.get("_transition_in_progress")), "Completed public transition must release the guard")
	assert(String(SceneTransition.get("_transition_target")).is_empty(), "Completed public transition must clear its destination")
	assert(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_IGNORE, "Completed transition must restore normal input routing")
	SceneTransition.remove_meta(PHASE_META)
	_test_invalid_save_is_transactional()
	print("CRASH_GUARDS_SMOKE_PASS transition_mutex=true public_transition=true transactional_load=true")
	get_tree().quit(0)

func _test_transition_mutex_helpers() -> void:
	var first_target := "res://scenes/main/main.tscn"
	var competing_target := "res://scenes/battle/battle_scene.tscn"
	assert(bool(SceneTransition.call("_begin_scene_transition", first_target)), "First scene transition should acquire the guard")
	assert(bool(SceneTransition.get("_transition_in_progress")), "Transition guard should report an active owner")
	assert(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_STOP, "Active transition must block duplicate pointer input")
	assert(not bool(SceneTransition.call("_begin_scene_transition", competing_target)), "Overlapping scene transition must be rejected")
	assert(String(SceneTransition.get("_transition_target")) == first_target, "Competing request must not replace the active destination")
	SceneTransition.call("_abort_scene_transition")
	assert(not bool(SceneTransition.get("_transition_in_progress")), "Aborting must release the transition guard")
	assert(SceneTransition.transition_rect.mouse_filter == Control.MOUSE_FILTER_IGNORE, "Aborting must restore normal input routing")
	assert(bool(SceneTransition.call("_begin_scene_transition", competing_target)), "Transition guard must be reusable after cleanup")
	SceneTransition.call("_abort_scene_transition")

func _test_invalid_save_is_transactional() -> void:
	var slot := SaveManager.MAX_SLOTS
	var path := "user://saves/save_%d.json" % slot
	var invalid_data := {
		"version": "0.3.0",
		"timestamp": "crash-guard-smoke",
		"scene": "res://scenes/maps/missing_crash_guard.tscn",
		"game": {"current_chapter": 99},
	}
	var file := FileAccess.open(path, FileAccess.WRITE)
	assert(file != null, "Crash guard smoke could not create its isolated save")
	file.store_string(JSON.stringify(invalid_data))
	file.close()

	var previous_chapter := GameManager.current_chapter
	_save_failed_emitted = false
	_load_completed_emitted = false
	SaveManager.save_failed.connect(_on_save_failed, CONNECT_ONE_SHOT)
	SaveManager.load_completed.connect(_on_load_completed, CONNECT_ONE_SHOT)
	assert(not SaveManager.load_game(slot), "A save pointing to a missing scene must fail")
	assert(_save_failed_emitted, "Invalid scene save must report save_failed")
	assert(not _load_completed_emitted, "Invalid scene save must not report load_completed")
	assert(GameManager.current_chapter == previous_chapter, "Invalid scene save must not partially import game state")

func _on_save_failed(_message: String) -> void:
	_save_failed_emitted = true

func _on_load_completed(_slot: int) -> void:
	_load_completed_emitted = true
