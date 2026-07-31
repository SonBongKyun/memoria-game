extends Node

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	GameManager.current_locale = "en"
	GameManager.current_chapter = 4
	GameManager.player_data["field_focus"] = 0
	GameManager.player_data["directive_streak"] = 0
	GameManager.play_stats["best_directive_streak"] = 0
	GameManager.play_stats["highest_battle_grade"] = 0
	if "first_battle" not in TutorialHints.shown_hints:
		TutorialHints.shown_hints.append("first_battle")
	if "first_directive" not in TutorialHints.shown_hints:
		TutorialHints.shown_hints.append("first_directive")

	var enemy := BattleManager.Enemy.new("Directive Probe", 140, 8, false)
	enemy.weakness = "physical"
	BattleManager.start_battle(enemy, "")
	assert(BattleManager.tactical_objective.is_empty(), "Directives must not be assigned without player agency")
	assert(BattleManager.tactical_objective_options.size() == 2, "A normal encounter must offer two readable directives")
	assert(
		String(BattleManager.tactical_objective_options[0].get("id", "")) != String(BattleManager.tactical_objective_options[1].get("id", "")),
		"Directive choices must be unique"
	)

	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle_scene: Node = battle_packed.instantiate()
	add_child(battle_scene)
	await get_tree().process_frame
	await get_tree().process_frame
	var briefing := battle_scene.find_child("DirectiveBriefingOverlay", true, false) as Control
	var choices := battle_scene.find_child("DirectiveChoiceButtons", true, false) as VBoxContainer
	assert(briefing != null and briefing.visible, "Battle UI must block on the directive briefing")
	assert(choices != null and choices.get_child_count() == 2, "Battle UI must render every offered directive")

	assert(BattleManager.select_tactical_objective(1), "A displayed directive must be selectable")
	await get_tree().process_frame
	assert(not BattleManager.tactical_objective.is_empty(), "The selected directive must become active")
	assert(BattleManager.tactical_objective_options.is_empty(), "Unused directive choices must clear after selection")
	assert(not briefing.visible, "The briefing must release the battlefield after selection")

	BattleManager._complete_tactical_objective("Smoke objective completed.")
	BattleManager._best_momentum_rank = 3
	BattleManager._witness_completed_this_battle = true
	BattleManager._breaks_this_battle = 1
	BattleManager._max_combo_this_battle = 4
	BattleManager._player_actions_this_battle = 4
	var grade := BattleManager._calculate_battle_grade()
	assert(String(grade.get("label", "")) == "S", "Layered tactical play must earn an S grade")
	assert(int(grade.get("score", 0)) == 100, "Battle grade must clamp to a readable 100-point score")
	assert(int(grade.get("grains", 0)) == 15, "S grade must have a meaningful reward")

	GameManager.set_directive_streak(2)
	var streak_reward := BattleManager._advance_directive_streak()
	assert(int(streak_reward.get("streak", 0)) == 3, "A completed directive must extend the saved chain")
	assert(int(streak_reward.get("focus", 0)) == 1, "Every third directive must return Field Focus to exploration")
	assert(GameManager.get_directive_streak() == 3, "Directive chain must persist in player data")
	assert(int(GameManager.play_stats.get("best_directive_streak", 0)) == 3, "Best directive chain must be tracked")
	var save_snapshot := GameManager.export_data()
	GameManager.set_directive_streak(0)
	GameManager.import_data(save_snapshot)
	assert(GameManager.get_directive_streak() == 3, "Directive chain must survive the normal save-data round trip")

	battle_scene.call("_on_victory_rewards_ready", {
		"grains": 42,
		"battle_grade": "S",
		"battle_score": 100,
		"grade_bonus": 15,
		"directive_streak": 3,
		"streak_bonus": 2,
		"streak_focus": 1,
		"streak_item": "",
		"objective_bonus": 8,
		"objective_title": "Hold the Name",
		"momentum_bonus": 6,
		"momentum_rank": 3,
		"momentum_label": "Resonant",
		"enemy_name": "Directive Probe",
		"heal": 20,
	})
	await get_tree().process_frame
	assert(battle_scene.get("_victory_panel") != null, "Victory UI must render the tactical grade and directive chain")

	print("TACTICAL_DIRECTIVES_SMOKE_PASS options=2 choice=1 grade=S score=100 chain=3 focus=1 save=1 victory_ui=1")
	get_tree().quit(0)
