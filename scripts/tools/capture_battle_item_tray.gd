extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/battle_item_tray_v3.png"

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings.clean_gameplay_visuals = true
	OptionsMenu.settings.reduce_motion = true
	GameManager.current_locale = "en"
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.current_chapter = 5
	GameManager.player_data.hp = 72
	GameManager.player_data.max_hp = 100
	GameManager.player_data.items = {
		"lantern_salve": 2,
		"root_balm": 1,
		"compass_shard": 1,
		"cinder_vial": 2,
		"signal_jammer": 1,
	}
	BattleManager.current_enemy = BattleManager.Enemy.new("Shade Sentinel", 90, 13, true)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.enemy_image = "res://assets/cg/character_shots/shade_sentinel_shot_v2.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/chapter_splash_the_seam.png"
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(3.2).timeout
	battle.call("_on_player_turn")
	await get_tree().create_timer(0.35).timeout
	battle.call("_toggle_item_list")
	await get_tree().create_timer(0.4).timeout

	var item_picker := battle.get("item_list_container") as VBoxContainer
	assert(item_picker != null, "Battle item picker did not build")
	var tray := item_picker.get_meta("tray_backdrop") as TextureRect
	assert(tray != null and tray.visible and tray.texture != null, "Transparent battle supply tray is not visible")
	assert(tray.texture.resource_path.ends_with("ui_battle_item_tray_v3.png"), "Battle must use the transparent v3 supply tray")
	var tray_image := tray.texture.get_image()
	assert(tray_image.get_pixel(0, 0).a < 0.05, "Battle supply tray corners must stay transparent")

	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var result := image.save_png(OUTPUT_PATH)
	assert(result == OK, "Battle item tray capture must save")
	print("BATTLE_ITEM_TRAY_CAPTURE_PASS path=%s" % OUTPUT_PATH)
	get_tree().quit(0)
