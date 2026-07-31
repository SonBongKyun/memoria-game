extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_battle_stage.png"

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.hp = 82
	GameManager.player_data.max_hp = 100
	BattleManager.current_enemy = BattleManager.Enemy.new("Shade Sentinel", 160, 18, true)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/the_seam.tscn"
	BattleManager.enemy_image = "res://assets/cg/character_shots/shade_sentinel_guard_v3.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/story_ch5_seam_first_light.png"
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(3.2).timeout
	if battle.objective_briefing_overlay != null and battle.objective_briefing_overlay.visible and battle.objective_briefing_buttons.get_child_count() > 0:
		battle.call("_choose_tactical_objective", 0)
		await get_tree().create_timer(0.45).timeout
	battle.call("_on_player_turn")
	var relief := battle.get_node_or_null("HybridDepthStage") as HybridDepthStage
	assert(relief != null and relief.profile_id == "the_seam", "Battle must resolve the returning map into the correct 3D biome")
	relief.pulse_impact(1.0, 1.2)
	await get_tree().create_timer(0.65).timeout
	print("HYBRID_BATTLE_LAYOUT bg=%s relief=%s viewport=%s" % [battle.bg.size, relief.size, get_viewport().get_visible_rect().size])
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("HYBRID_BATTLE_CAPTURE_PASS path=%s profile=%s" % [OUTPUT_PATH, relief.profile_id])
	get_tree().quit(0)
