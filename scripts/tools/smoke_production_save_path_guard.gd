extends Node


func _ready() -> void:
	Codex.suppress_recording = true
	print("[SMOKE][EXPECTED_FAILURE][production_save_path_guard] requesting forbidden target")
	var target := SaveManager.PRODUCTION_SAVE_ROOT
	if "--guard-outside-temp" in OS.get_cmdline_user_args():
		target = "user://not_allowed_smoke_saves"
	SaveManager.configure_test_save_root(target)
	# The guard schedules process exit 1. Reaching the next frame without its
	# fatal marker means the contract is broken, which is also a failed process.
	await get_tree().process_frame
	push_error("[SMOKE][FAIL][production_save_path_guard] guard did not terminate the process")
	get_tree().quit(1)
