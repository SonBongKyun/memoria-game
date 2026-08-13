extends Node

## S243: 게임의 처음과 끝 화면을 한 장에 모은다.
##
## 타이틀, 게임 오버, 데모 종료, 그리고 엔딩 분기별 크레딧. 플레이어가 반드시
## 보는 화면인데 맵·전투·오버레이·VN과 달리 한 번도 나란히 놓고 본 적이 없다.
##
## 크레딧은 한 씬이지만 플래그에 따라 네 갈래(zero/ash/seam/seal)로 갈린다.
## 분기마다 따로 세워서 찍는다. 어느 갈래가 잡혔는지는 AchievementManager가
## 기록하는 값으로 확인한다.

const OUTPUT_PATH := "res://tmp/visual_audit/endpoint_survey.png"
const SHOT_SIZE := Vector2i(640, 360)
const COLUMNS := 3

const SHOTS: Array[Dictionary] = [
	{"name": "title", "scene": "res://scenes/main/main.tscn", "flags": []},
	{"name": "game_over", "scene": "res://scenes/ui/game_over.tscn", "flags": []},
	{"name": "demo_end", "scene": "res://scenes/ui/demo_end.tscn", "flags": []},
	{"name": "credits_zero", "scene": "res://scenes/ui/credits.tscn", "flags": ["zero_burn_path"]},
	{"name": "credits_seam", "scene": "res://scenes/ui/credits.tscn",
		"flags": ["seal_refused", "hidden_ch1_stump", "hidden_ch6_garden"]},
	{"name": "credits_seal", "scene": "res://scenes/ui/credits.tscn", "flags": []},
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	await get_tree().process_frame

	var shots: Array[Image] = []
	var labels: Array[String] = []
	for spec in SHOTS:
		var image := await _shoot(spec)
		if image != null:
			shots.append(image)
			labels.append(String(spec["name"]))
	if shots.is_empty():
		print("ENDPOINT_SURVEY_CAPTURE_FAIL")
		get_tree().quit(1)
		return

	var rows := int(ceil(float(shots.size()) / float(COLUMNS)))
	var sheet := Image.create(SHOT_SIZE.x * COLUMNS, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, SHOT_SIZE),
			Vector2i((i % COLUMNS) * SHOT_SIZE.x, (i / COLUMNS) * SHOT_SIZE.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("ENDPOINT_SURVEY_CAPTURE_PASS path=%s shots=%s" % [OUTPUT_PATH, ", ".join(labels)])
	get_tree().quit(0)

func _shoot(spec: Dictionary) -> Image:
	var path := String(spec["scene"])
	if not ResourceLoader.exists(path):
		print("ENDPOINT_SURVEY skip %s (%s 없음)" % [spec["name"], path])
		return null
	GameManager.story_flags.clear()
	for flag in spec["flags"] as Array:
		GameManager.set_flag(String(flag))

	var node: Node = load(path).instantiate()
	add_child(node)
	await get_tree().create_timer(1.6).timeout
	# 크레딧은 화면 아래에서 위로 흐른다. 1.6초 시점에는 아직 본문이 화면 밖이라
	# 첫 캡처가 세 장 모두 빈 화면이었다. 두루마리를 본문 한가운데로 옮겨 놓는다.
	var scroll := node.get("scroll_container") as Control
	if scroll != null:
		scroll.position.y = -maxf(float(node.get("_total_height")) * 0.42, 0.0)
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/endpoint_%s.png" % spec["name"]))
	print("ENDPOINT_SURVEY shot %s" % spec["name"])

	node.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return image
