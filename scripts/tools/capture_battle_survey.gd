extends Node

## S241: 게임의 모든 정규 교전을 같은 조건으로 렌더해 한 장에 모은다.
##
## 지금까지 전투 화면은 The Seam 한 장면만 보고 판단해 왔다. 맵은 열 곳을 나란히
## 놓고 나서야 아홉 곳이 단색 안개에 덮여 있다는 걸 알았다(S236). 전투도 같은
## 방식으로 보지 않으면, 잘 나온 한 장면을 전체라고 착각하게 된다.

const OUTPUT_PATH := "res://tmp/visual_audit/battle_survey.png"
const SHOT_SIZE := Vector2i(640, 360)

## 실제 게임에서 만나는 순서대로. 챕터는 적이 등장하는 지점에 맞춘다.
const ENCOUNTERS: Array[Dictionary] = [
	{"preset": "ash_crawler", "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"preset": "forest_shade", "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
	{"preset": "void_beast", "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"preset": "threshold_shade", "chapter": 7, "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"preset": "shade_sentinel", "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"preset": "kairos", "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "ko"

	var shots: Array[Image] = []
	var labels: Array[String] = []
	for encounter in ENCOUNTERS:
		var image := await _shoot(encounter)
		if image != null:
			shots.append(image)
			labels.append(String(encounter["preset"]))
	if shots.is_empty():
		print("BATTLE_SURVEY_CAPTURE_FAIL")
		get_tree().quit(1)
		return

	var columns := 2
	var rows := int(ceil(float(shots.size()) / float(columns)))
	var sheet := Image.create(SHOT_SIZE.x * columns, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, SHOT_SIZE), Vector2i((i % columns) * SHOT_SIZE.x, (i / columns) * SHOT_SIZE.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("BATTLE_SURVEY_CAPTURE_PASS path=%s encounters=%s" % [OUTPUT_PATH, ", ".join(labels)])
	get_tree().quit(0)

func _shoot(encounter: Dictionary) -> Image:
	GameManager.current_chapter = int(encounter["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.hp = 82
	GameManager.player_data.max_hp = 100
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = GameManager.current_chapter >= 4
	BattleManager.tobias_in_party = GameManager.current_chapter >= 3 and GameManager.current_chapter < 7
	# 실제 경로를 그대로 쓴다. 프리셋이 배경과 적 삽화를 스스로 고르게 둔다.
	BattleManager.start_battle(String(encounter["preset"]), String(encounter["scene"]))

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.5).timeout
	battle.call("_on_player_turn")
	await get_tree().create_timer(0.7).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	# 시트는 전체를 비교하기 위한 것이고, 글자를 읽으려면 원본 크기가 필요하다.
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/battle_%s.png" % encounter["preset"]))
	print("BATTLE_SURVEY shot %s (%s)" % [encounter["preset"], BattleManager.enemy_image])
	battle.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return image
