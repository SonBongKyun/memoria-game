extends Node

const PREVIEW_REPEATS := 6
var _battle_scene: Node = null

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "en"
	GameManager.current_chapter = 2
	GameManager.player_data["elia_with_party"] = true
	for hint in ["first_battle", "first_directive", "first_burn"]:
		if hint not in TutorialHints.shown_hints:
			TutorialHints.shown_hints.append(hint)

	_check_swift_finish_boundaries()
	await _check_preview_reselection_lifetime()
	await _check_single_afterglow_from_real_burns()

	print("BURN_DIRECTIVE_STABILIZATION_SMOKE_PASS previews=%d swift=advance-risk-fail actual_burns=2 afterglow=1" % PREVIEW_REPEATS)
	get_tree().quit(0)

func _check_swift_finish_boundaries() -> void:
	BattleManager.tactical_objective = {
		"id": "swift_finish",
		"title": "Before It Learns",
		"desc": "Win within 4 player actions.",
		"status": "active",
		"complete": false,
		"failed": false,
	}
	BattleManager._objective_completed = false
	BattleManager._objective_failed = false

	BattleManager._player_actions_this_battle = 2
	var advance := BattleManager.get_objective_action_relation("attack")
	assert(String(advance.get("kind", "")) == "advance", "The third action must still advance Swift Finish")
	assert("remain after this" in String(advance.get("text", "")), "Advance copy must describe the post-action budget")

	BattleManager._player_actions_this_battle = 3
	var risk := BattleManager.get_objective_action_relation("attack")
	assert(String(risk.get("kind", "")) == "risk", "The fourth action must be the final-action risk boundary")
	assert("finish now" in String(risk.get("text", "")), "Risk copy must say that the current action has to finish")

	BattleManager._player_actions_this_battle = 4
	var fail := BattleManager.get_objective_action_relation("attack")
	assert(String(fail.get("kind", "")) == "fail", "A fifth action must preview immediate Swift Finish failure")
	assert("exceeded" in String(fail.get("text", "")), "Failure copy must name the exceeded action limit")

func _check_preview_reselection_lifetime() -> void:
	var enemy := BattleManager.Enemy.new("Burn Lifetime Probe", 180, 8, false)
	BattleManager.start_battle(enemy, "")
	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle: Node = battle_packed.instantiate()
	_battle_scene = battle
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame

	var first = MemoryManager._get_memory("daily_campfire_song")
	var second = MemoryManager._get_memory("sense_warm_light")
	assert(first != null and second != null, "Preview lifetime smoke needs two selectable memories")

	for index in range(PREVIEW_REPEATS):
		battle.call("_show_burn_preview", first)
		await get_tree().process_frame
		var old_pulse: Tween = battle.get("_burn_preview_pulse_tween")
		assert(old_pulse != null and old_pulse.is_valid(), "Each open preview must own one live pulse tween")

		battle.call("_on_burn_preview_cancelled")
		battle.call("_hide_burn_list")
		battle.call("_show_burn_preview", second)
		await get_tree().process_frame
		await get_tree().process_frame
		assert(not old_pulse.is_valid(), "Cancelling must kill the previous button's looping tween")
		assert(String(battle.get("_pending_burn_id")) == second.id, "Immediate reselection must keep the newest memory")
		var panel := battle.get("_burn_preview_panel") as PanelContainer
		assert(panel != null and panel.visible, "A stale hide callback must not close the reselected preview")
		assert(panel.get_child_count() == 1, "Preview rebuilds must leave one current content tree")

		if index < PREVIEW_REPEATS - 1:
			battle.call("_hide_burn_preview")

	await get_tree().create_timer(BattleManager.paced(0.6)).timeout
	var current_button := battle.get("_burn_preview_confirm_btn") as Button
	assert(current_button != null and not current_button.disabled, "Only the current preview timer may unlock its burn button")

	battle.call("_on_burn_preview_cancelled")
	battle.call("_hide_burn_list")
	await get_tree().create_timer(0.25).timeout
	assert(not (battle.get("_burn_preview_panel") as PanelContainer).visible, "The final cancel must settle closed")
	assert(battle.get("_burn_preview_pulse_tween") == null, "No looping preview tween may survive cancellation")

func _check_single_afterglow_from_real_burns() -> void:
	WorldRewriteDirector._clear_absence_afterglow()
	var first_probe := MemoryManager.Memory.new(
		"s227_afterglow_first",
		"First Stabilization Burn",
		"Synthetic memory for the real burn path.",
		MemoryManager.MemoryGrade.GRADE_3,
		40
	)
	var second_probe := MemoryManager.Memory.new(
		"s227_afterglow_second",
		"Second Stabilization Burn",
		"Synthetic memory that replaces the first wash.",
		MemoryManager.MemoryGrade.GRADE_2,
		80
	)
	MemoryManager.memories.append(first_probe)
	MemoryManager.memories.append(second_probe)

	assert(MemoryManager.burn_memory(first_probe.id) == first_probe, "The first probe must use MemoryManager.burn_memory()")
	await get_tree().process_frame
	await get_tree().process_frame
	assert(WorldRewriteDirector.get_active_afterglow_count() == 1, "A real memory burn must create exactly one owned afterglow")
	var first_rect := WorldRewriteDirector.get_active_afterglow_rect()
	assert(first_rect != null and first_rect.mouse_filter == Control.MOUSE_FILTER_IGNORE, "The shared afterglow must stay non-blocking")

	assert(MemoryManager.burn_memory(second_probe.id) == second_probe, "The replacement probe must use the same real burn path")
	await get_tree().process_frame
	await get_tree().process_frame
	assert(WorldRewriteDirector.get_active_afterglow_count() == 1, "A second burn must replace, never stack, the shared afterglow")
	var second_rect := WorldRewriteDirector.get_active_afterglow_rect()
	assert(second_rect != null and second_rect != first_rect, "The single owner must replace the prior wash with the latest grade")
	assert(second_rect.get_parent().get_parent() == WorldRewriteDirector, "Battle and field afterglow must share WorldRewriteDirector ownership")
	assert(_battle_scene != null and _battle_scene.find_child("MemoryBurnAfterglow", true, false) == null, "The battle scene must not create a competing afterglow")
	_battle_scene.queue_free()
	await get_tree().process_frame
