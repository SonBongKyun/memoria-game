extends Node

## S243: VN 20개 장면을 같은 조건으로 렌더해 한 장에 모은다.
##
## 이 방식은 세 번 값을 했다. 맵 열 곳(S236)에서 아홉 곳이 단색 안개에 덮여
## 있었고, 전투 여섯(S241)에서 첫 잡몹이 남의 삽화를 달고 있었고, 오버레이
## 열둘(S242)에서 UI 크롬이 통째로 영어였다. VN은 504스텝으로 이 게임에서
## 분량이 가장 큰 화면인데 한 번도 나란히 놓고 본 적이 없다.
##
## 스텝은 504개지만 전부 찍을 필요는 없다. 각 파일에서 "CG가 깔리고 인물이
## 말하는" 첫 스텝까지 진행시켜 찍는다. 그 한 장에 배경, 포트레이트, 이름표,
## 대사 상자, 오버레이가 동시에 나오므로 레이아웃 문제는 거기서 드러난다.
##
## 인덱스를 바로 지정하지 않고 0부터 진행시키는 이유: CG는 앞선 스텝에서
## 설정되고 이후 스텝은 그것을 물려받는다. 중간부터 시작하면 배경이 빈다.

const OUTPUT_PATH := "res://tmp/visual_audit/vn_survey.png"
const SHOT_SIZE := Vector2i(480, 270)
const COLUMNS := 4

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["text_speed"] = 4
	GameManager.current_locale = "ko"
	await get_tree().process_frame

	var ids := _scene_ids()
	var shots: Array[Image] = []
	var labels: Array[String] = []
	for scene_id in ids:
		var image := await _shoot(scene_id)
		if image != null:
			shots.append(image)
			labels.append(scene_id)
	if shots.is_empty():
		print("VN_SURVEY_CAPTURE_FAIL")
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
	print("VN_SURVEY_CAPTURE_PASS path=%s scenes=%d" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)

func _scene_ids() -> Array[String]:
	var ids: Array[String] = []
	var dir := DirAccess.open("res://data/vn_scenes")
	if dir == null:
		return ids
	for file_name in dir.get_files():
		if file_name.ends_with(".json"):
			ids.append(file_name.get_basename())
	ids.sort()
	return ids

func _shoot(scene_id: String) -> Image:
	var data := _load(scene_id)
	var steps: Array = data.get("steps", [])
	if steps.is_empty():
		print("VN_SURVEY skip %s (steps=0)" % scene_id)
		return null
	var target := _target_index(steps)

	SceneFlow.play(scene_id, 0)
	await get_tree().create_timer(0.5).timeout
	var guard := 0
	while SceneFlow.is_active and SceneFlow.current_index < target and guard < 40:
		SceneFlow.advance()
		await get_tree().create_timer(0.12).timeout
		guard += 1
	# 타자기 효과가 끝나야 대사가 다 나온다. 고정 시간(1.1초)으로 기다렸더니
	# 긴 대사가 문장 중간에서 잘린 채 찍혔다. VN이 스스로 끝났다고 말할 때까지
	# 기다린다.
	var ui: Node = SceneFlow.get("_vn_ui")
	var typing_guard := 0
	while ui != null and not bool(ui.get("_typing_done")) and typing_guard < 60:
		await get_tree().create_timer(0.1).timeout
		typing_guard += 1
	await get_tree().create_timer(0.3).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/vn_%s.png" % scene_id))
	print("VN_SURVEY shot %-28s step=%d/%d" % [scene_id, SceneFlow.current_index, steps.size()])

	SceneFlow.call("_end_scene")
	await get_tree().create_timer(0.4).timeout
	return image

## CG가 이미 깔린 뒤 인물이 말하는 첫 스텝. 선택지/전투/전환 앞에서 멈춘다.
func _target_index(steps: Array) -> int:
	for i in range(steps.size()):
		var step: Dictionary = steps[i]
		if step.has("choices") or step.has("action") or step.has("battle"):
			break
		if step.has("speaker") and (step.has("text") or step.has("text_ko")):
			return i
	return mini(2, steps.size() - 1)

func _load(scene_id: String) -> Dictionary:
	var path := "res://data/vn_scenes/%s.json" % scene_id
	if not FileAccess.file_exists(path):
		return {}
	var file := FileAccess.open(path, FileAccess.READ)
	var json := JSON.new()
	if json.parse(file.get_as_text()) != OK or not (json.data is Dictionary):
		return {}
	return json.data
