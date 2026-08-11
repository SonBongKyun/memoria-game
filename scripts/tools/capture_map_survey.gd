extends Node

## S236 개발용. 모든 코어 맵을 같은 조건으로 렌더해 한 장에 모은다.
## "그래픽을 개선하자"를 기능 개수가 아니라 실제 화면으로 판단하기 위한 도구.

const OUTPUT_PATH := "res://tmp/visual_audit/map_survey.png"
const SHOT_SIZE := Vector2i(640, 360)

const MAPS: Array[String] = [
	"rim_forest",
	"verdan_market",
	"belt_waystation",
	"drift_shelter",
	"crumbling_coast",
	"the_seam",
	"seam_outskirts",
	"forgotten_forest",
	"colorless_waste",
	"bl07_void",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "ko"
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var shots: Array[Image] = []
	var labels: Array[String] = []
	for map_id in MAPS:
		var image := await _shoot(map_id)
		if image != null:
			shots.append(image)
			labels.append(map_id)

	if shots.is_empty():
		print("MAP_SURVEY_CAPTURE_FAIL no shots")
		get_tree().quit(1)
		return

	var columns := 2
	var rows := int(ceil(float(shots.size()) / float(columns)))
	var sheet := Image.create(SHOT_SIZE.x * columns, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(
			shot,
			Rect2i(Vector2i.ZERO, SHOT_SIZE),
			Vector2i((i % columns) * SHOT_SIZE.x, (i / columns) * SHOT_SIZE.y)
		)
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("MAP_SURVEY_CAPTURE_PASS path=%s maps=%s" % [OUTPUT_PATH, ", ".join(labels)])
	get_tree().quit(0)

func _shoot(map_id: String) -> Image:
	var path := "res://scenes/maps/%s.tscn" % map_id
	if not ResourceLoader.exists(path):
		print("MAP_SURVEY skip %s (missing)" % map_id)
		return null
	var map: Node = load(path).instantiate()
	add_child(map)
	# 맵 스크립트가 지형/조명/파티클을 모두 세우고 카메라가 정착할 시간을 준다.
	await get_tree().create_timer(1.4).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	print("MAP_SURVEY shot %s" % map_id)
	map.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return image
