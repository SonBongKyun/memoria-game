## S214: 전투의 여러 순간을 한 장으로 모아 실제 연출을 판단한다.
## 정지 아이들 프레임만 보면 타격 순간의 문제를 절대 못 잡는다.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/battle_moments.png"

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
	for _f in range(70):
		await get_tree().process_frame

	var shots: Array[Image] = []
	shots.append(get_viewport().get_texture().get_image())          # 1 대기

	battle.call("_on_pre_attack", "player", "enemy", "Strike")
	for _f in range(10):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())          # 2 돌진 (연출 중)

	battle.call("_on_damage_dealt", "enemy", 34, "Strike")
	for _f in range(6):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())          # 3 타격 순간

	# 이펙트가 스스로 걷히는지 확인한다. 여기서 안 걷히면 진짜 버그다.
	for _f in range(150):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())          # 4 2.5초 후

	var crop := Rect2i(0, 150, 1280, 330)
	var sheet := Image.create(crop.size.x, crop.size.y * shots.size(), false, Image.FORMAT_RGBA8)
	for i in range(shots.size()):
		sheet.blit_rect(shots[i], crop, Vector2i(0, crop.size.y * i))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("BATTLE_MOMENTS_CAPTURE_PASS path=%s shots=%d" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)
