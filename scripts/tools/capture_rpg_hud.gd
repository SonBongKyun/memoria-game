## S216: 미니맵 POI와 퀘스트 트래커를 실제 화면으로 확인한다.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/rpg_hud.png"

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 2
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	# 진행 중인 의뢰 하나를 켠다.
	GameManager.set_flag("sq_echoes_ash_started")

	var map: Node2D = load("res://scenes/maps/verdan_market.tscn").instantiate()
	add_child(map)
	for _f in range(20):
		await get_tree().process_frame

	# 플레이어를 발견물 근처로 옮겨 POI가 드러나게 한다.
	var points: Array = Minimap._collect_points_of_interest(map)
	assert(not points.is_empty(), "맵에 관심 지점이 있어야 한다")
	var target: Vector2 = points[0].get("pos", Vector2.ZERO)
	var player := map.get_node_or_null("Player") as Node2D
	if player != null:
		player.position = target
	for _f in range(20):
		await get_tree().process_frame

	ExplorationHUD.call("_update_quest_tracker")
	await get_tree().process_frame

	var revealed := 0
	var data: Dictionary = map.get("_minimap_data")
	for m: ColorRect in data.get("poi_markers", []):
		if m.visible:
			revealed += 1

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("RPG_HUD_CAPTURE_PASS path=%s poi=%d revealed=%d" % [OUTPUT_PATH, points.size(), revealed])
	get_tree().quit(0)
