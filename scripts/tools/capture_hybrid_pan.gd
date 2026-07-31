## S212: 카메라 패닝 전후를 나란히 붙여, 2D 전투원이 3D 무대와 함께 움직이는지 눈으로 확인한다.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_pan_compare.png"

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	GameManager.player_data.elia_with_party = true
	BattleManager.current_enemy = BattleManager.Enemy.new("Ash Crawler", 60, 9, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/rim_forest.tscn"
	BattleManager.enemy_image = ""
	BattleManager.battle_bg_image = ""

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	for _f in range(4):
		await get_tree().process_frame
	var stage: HybridDepthStage = battle.get("_hybrid_depth_stage")

	stage.set_battle_focus("player")
	for _f in range(90):
		await get_tree().process_frame
	var left := get_viewport().get_texture().get_image()

	stage.set_battle_focus("enemy")
	for _f in range(90):
		await get_tree().process_frame
	var right := get_viewport().get_texture().get_image()

	# 위: 플레이어 턴 / 아래: 적 턴. 전투원 주변만 잘라 비교한다.
	# 전경은 화면 양 끝에 있으므로 전체 폭을 그대로 담는다.
	var crop := Rect2i(0, 210, 1280, 250)
	var sheet := Image.create(crop.size.x, crop.size.y * 2, false, Image.FORMAT_RGBA8)
	sheet.blit_rect(left, crop, Vector2i(0, 0))
	sheet.blit_rect(right, crop, Vector2i(0, crop.size.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("HYBRID_PAN_CAPTURE_PASS path=%s" % OUTPUT_PATH)
	get_tree().quit(0)
