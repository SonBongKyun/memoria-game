extends Node

const SmokeTestRunner = preload("res://scripts/tools/smoke_test_runner.gd")


func _ready() -> void:
	Codex.suppress_recording = true
	var runner = SmokeTestRunner.new("smoke_test_runner_contract", "SMOKE_TEST_RUNNER_CONTRACT_PASS")
	runner.begin_test("forced_failure_exit_contract")
	var force_failure := "--force-smoke-failure" in OS.get_cmdline_user_args()
	runner.expect(not force_failure,
		"Forced failure proves that the shared runner returns process exit code 1")
	runner.finish(get_tree(), "failure_mode=available")
