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
	assert(not BattleManager.tactical_objective.is_empty(), "Each encounter must begin with one readable objective")
	assert(BattleManager.tactical_objective_options.is_empty(), "The fight must not stop for a directive choice menu")
	assert(String(BattleManager.tactical_objective.get("status", "")) == "active", "The encounter objective must be active immediately")
	assert(String(BattleManager.tactical_objective.get("id", "")) != "echo_weave", "An automatic objective must never prescribe irreversible burns")

	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle_scene: Node = battle_packed.instantiate()
	add_child(battle_scene)
	await get_tree().process_frame
	await get_tree().process_frame
	var briefing := battle_scene.find_child("DirectiveBriefingOverlay", true, false) as Control
	var choices := battle_scene.find_child("DirectiveChoiceButtons", true, false) as VBoxContainer
	var objective_panel := battle_scene.get("objective_panel") as PanelContainer
	var objective_desc := battle_scene.get("objective_desc_label") as Label
	assert(briefing != null and not briefing.visible, "The directive briefing must stay out of the combat opening")
	assert(choices != null and choices.get_child_count() == 0, "No unused directive buttons should be rendered")
	assert(objective_panel != null and objective_panel.visible, "The compact objective card must carry the battle thread")
	assert(objective_desc != null and "PAYOFF" in objective_desc.text, "The objective card must expose its payoff")

	GameManager.player_data["elia_with_party"] = true
	assert(int(BattleManager.get_burn_aftershock_preview(MemoryManager.MemoryGrade.GRADE_4).get("turns", -1)) == 0, "Low-grade burns must remain the safe resource option")
	assert(int(BattleManager.get_burn_aftershock_preview(MemoryManager.MemoryGrade.GRADE_3).get("turns", 0)) == 1, "Relationship memories must create one unreadable response")
	var core_preview := BattleManager.get_burn_aftershock_preview(MemoryManager.MemoryGrade.GRADE_1)
	assert(int(core_preview.get("turns", 0)) == 2 and bool(core_preview.get("elia_anchored", false)), "Elia must shorten, not erase, a core-memory aftershock")
	var core_memory := MemoryManager.Memory.new(
		"smoke_core",
		"Smoke Core Memory",
		"A temporary regression memory.",
		MemoryManager.MemoryGrade.GRADE_1,
		999
	)
	BattleManager._apply_burn_aftershock(core_memory)
	await get_tree().process_frame
	var aftershock_state := BattleManager.get_burn_aftershock_state()
	var objective_meta := battle_scene.get("objective_meta_label") as Label
	assert(int(aftershock_state.get("turns", 0)) == 2, "A core burn must hide two enemy responses after Elia's anchor")
	assert("afterimage" in BattleManager.get_next_turn_hint().to_lower(), "Burn aftershock must replace actionable enemy intent")
	assert(objective_meta != null and "afterimage" in objective_meta.text.to_lower(), "The compact card must expose the active burn cost")
	BattleManager._advance_burn_aftershock()
	BattleManager._advance_burn_aftershock()
	assert(not bool(BattleManager.get_burn_aftershock_state().get("active", true)), "Burn aftershock must clear after its enemy responses")

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

	print("TACTICAL_DIRECTIVES_SMOKE_PASS objective=1 modal=0 payoff=1 aftershock=2 elia_anchor=1 grade=S score=100 chain=3 focus=1 save=1 victory_ui=1")
	get_tree().quit(0)
