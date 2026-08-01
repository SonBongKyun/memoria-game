extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/tactical_objective_card.png"

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings.clean_gameplay_visuals = true
	OptionsMenu.settings.reduce_motion = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 5
	GameManager.player_data.hp = 84
	GameManager.player_data.max_hp = 115
	GameManager.player_data["field_focus"] = 0
	GameManager.player_data["directive_streak"] = 2
	if "first_battle" not in TutorialHints.shown_hints:
		TutorialHints.shown_hints.append("first_battle")
	if "first_directive" not in TutorialHints.shown_hints:
		TutorialHints.shown_hints.append("first_directive")

	var enemy := BattleManager.Enemy.new("Shade Sentinel", 180, 20, true)
	enemy.is_boss = true
	enemy.weakness = "void"
	enemy.resistance = "fire"
	BattleManager.start_battle(
		enemy,
		"res://scenes/maps/the_seam.tscn",
		"res://assets/cg/generated/chapter_splash_the_seam.png",
		"res://assets/cg/character_shots/shade_sentinel_shot_v2.png"
	)
	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle: Node = battle_packed.instantiate()
	add_child(battle)
	await get_tree().create_timer(4.2).timeout

	var briefing := battle.find_child("DirectiveBriefingOverlay", true, false) as Control
	var choices := battle.find_child("DirectiveChoiceButtons", true, false) as VBoxContainer
	var objective_panel := battle.get("objective_panel") as PanelContainer
	assert(briefing != null and not briefing.visible, "The obsolete directive briefing must remain hidden")
	assert(choices != null and choices.get_child_count() == 0, "Normal battle capture must not show directive choices")
	assert(objective_panel != null and objective_panel.visible, "The compact objective card must stay visible in combat")

	if DisplayServer.get_name() == "headless":
		print("TACTICAL_DIRECTIVE_CAPTURE_SKIP renderer=headless objective=1 modal=0")
		get_tree().quit(0)
		return
	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var result := image.save_png(OUTPUT_PATH)
	assert(result == OK, "Tactical directive capture must save")
	print("TACTICAL_DIRECTIVE_CAPTURE_PASS path=%s objective=1 modal=0 chain=2" % OUTPUT_PATH)
	get_tree().quit(0)
