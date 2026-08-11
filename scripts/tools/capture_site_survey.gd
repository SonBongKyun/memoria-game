extends Node

## S239: 선택 기억 장소 아홉 곳을 같은 조건으로 렌더해 한 장에 모은다.
## 코어 맵과 같은 대기 예산을 받았는지 눈과 수치로 함께 확인하기 위한 도구.

const OUTPUT_PATH := "res://tmp/visual_audit/site_survey.png"
const SHOT_SIZE := Vector2i(640, 360)
const SITES: Array[String] = [
	"rim_root_hollow", "verdan_ledger_cellar", "belt_signal_yard",
	"drift_waymarker_shrine", "coast_cinder_harbor", "seam_lantern_ward",
	"forest_name_hollow", "waste_grey_caravan", "bl07_seed_vault",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "ko"
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var shots: Array[Image] = []
	for site_id in SITES:
		var image := await _shoot(site_id)
		if image != null:
			shots.append(image)
	if shots.is_empty():
		print("SITE_SURVEY_CAPTURE_FAIL")
		get_tree().quit(1)
		return

	var columns := 3
	var rows := int(ceil(float(shots.size()) / float(columns)))
	var sheet := Image.create(SHOT_SIZE.x * columns, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, SHOT_SIZE), Vector2i((i % columns) * SHOT_SIZE.x, (i / columns) * SHOT_SIZE.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("SITE_SURVEY_CAPTURE_PASS path=%s sites=%d" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)

func _shoot(site_id: String) -> Image:
	var path := "res://scenes/maps/%s.tscn" % site_id
	if not ResourceLoader.exists(path):
		return null
	var site: Node = load(path).instantiate()
	add_child(site)
	await get_tree().create_timer(1.3).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	print("SITE_SURVEY shot %s" % site_id)
	site.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return image
