## S213: 기억 연소에 무대가 반응하는 순간을 실측 캡처한다.
## 위: 평상시 / 아래: 연소 직후 (격자 발화 + 잔불)
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/burn_arena_compare.png"

func _ready() -> void:
	OptionsMenu.settings["reduce_motion"] = false
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	GameManager.current_locale = "ko"
	BattleManager.current_enemy = BattleManager.Enemy.new("Ash Crawler", 60, 9, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/rim_forest.tscn"
	BattleManager.enemy_image = ""
	BattleManager.battle_bg_image = ""

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	for _f in range(60):
		await get_tree().process_frame
	var stage: HybridDepthStage = battle.get("_hybrid_depth_stage")
	var calm := get_viewport().get_texture().get_image()

	stage.play_memory_burn(3)
	for _f in range(18):
		await get_tree().process_frame
	var burning := get_viewport().get_texture().get_image()

	var crop := Rect2i(0, 200, 1280, 260)
	var sheet := Image.create(crop.size.x, crop.size.y * 2, false, Image.FORMAT_RGBA8)
	sheet.blit_rect(calm, crop, Vector2i(0, 0))
	sheet.blit_rect(burning, crop, Vector2i(0, crop.size.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("BURN_ARENA_CAPTURE_PASS path=%s flare=%.2f" % [OUTPUT_PATH, stage._burn_flare])
	get_tree().quit(0)
