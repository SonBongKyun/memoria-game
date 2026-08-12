extends Node

## S242: 게임의 모든 오버레이를 같은 조건으로 렌더해 한 장에 모은다.
##
## 이 방식은 두 번 값을 했다. 맵 열 곳을 나란히 놓고 나서야 아홉 곳이 단색 안개에
## 덮여 있다는 걸 알았고(S236), 전투 여섯을 나란히 놓고 나서야 첫 잡몹이 남의
## 삽화를 달고 있다는 걸 알았다(S241). 오버레이는 아직 한 번도 그렇게 본 적이 없다.
##
## 크기 검사(probe_overlay_bounds)는 이미 통과했다. 화면을 넘는 Control은 없다.
## 하지만 크기가 맞다고 읽히는 건 아니다. 대비, 겹침, 잘린 글자, 빈 대역은 봐야만
## 안다. 그래서 숫자가 아니라 그림을 남긴다.
##
## 배경은 실제 맵을 깐다. 오버레이 상당수가 반투명 패널이라 회색 바탕 위에서는
## 실제 게임과 다르게 보인다.

const OUTPUT_PATH := "res://tmp/visual_audit/overlay_survey.png"
const BACKDROP := "res://scenes/maps/verdan_market.tscn"
const SHOT_SIZE := Vector2i(640, 360)

## 닫기 이름은 오버레이마다 다르다(close_log, close_shop, close_puzzle...).
## 처음에는 흔한 이름을 순서대로 시도했는데, StoryLog는 그중 아무것도 없어서
## 닫히지 않은 채 이후 아홉 장 위에 그대로 남았다. 표에 명시한다.
const OVERLAYS: Array[Dictionary] = [
	{"name": "memory_archive", "call": ["MemoryUI", "open_archive", []], "close": "close_archive"},
	{"name": "story_journal", "call": ["StoryJournal", "open_journal", []], "close": "close_journal"},
	{"name": "story_log", "call": ["StoryLog", "open_log", []], "close": "close_log"},
	{"name": "codex", "call": ["Codex", "open", []], "close": "close"},
	{"name": "memory_shop", "call": ["MemoryShop", "open_shop", ["Malet"]], "close": "close_shop"},
	{"name": "memory_puzzle", "call": ["MemoryPuzzle", "open_puzzle", [4, 15]], "close": "close_puzzle"},
	{"name": "options_menu", "call": ["OptionsMenu", "open", []], "close": "close"},
	{"name": "pause_menu", "call": ["PauseMenu", "_open", []], "close": "_close"},
	{"name": "memory_constellation", "call": ["MemoryConstellation", "open", []], "close": "close"},
	{"name": "system_log", "call": ["SystemLog", "show_log", ["관리국 감지: 기억 연소 흔적"]], "close": ""},
	{"name": "notification_toast", "call": ["NotificationToast", "show_toast", ["기억을 잃었습니다"]], "close": ""},
	{"name": "tutorial_hint", "call": ["TutorialHints", "show_hint", ["first_directive"]], "close": "_dismiss"},
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 5
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	# 배경 맵이 자기 도착 시퀀스를 재생하면 VN 장면이 오버레이 위를 덮는다.
	# 첫 캡처에서 상점·퍼즐·성좌가 그렇게 가려졌다. 이미 본 것으로 표시해 둔다.
	for flag in ["ch2_arrival_vn_seen", "ch2_arrived", "ch2_malet_done", "ch2_complete"]:
		GameManager.set_flag(flag)

	var backdrop: Node = load(BACKDROP).instantiate()
	add_child(backdrop)
	await get_tree().create_timer(1.4).timeout

	var shots: Array[Image] = []
	var labels: Array[String] = []
	for overlay in OVERLAYS:
		var image := await _shoot(overlay)
		if image != null:
			shots.append(image)
			labels.append(String(overlay["name"]))
	if shots.is_empty():
		print("OVERLAY_SURVEY_CAPTURE_FAIL")
		get_tree().quit(1)
		return

	var columns := 3
	var rows := int(ceil(float(shots.size()) / float(columns)))
	var sheet := Image.create(SHOT_SIZE.x * columns, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, SHOT_SIZE),
			Vector2i((i % columns) * SHOT_SIZE.x, (i / columns) * SHOT_SIZE.y))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("OVERLAY_SURVEY_CAPTURE_PASS path=%s overlays=%s" % [OUTPUT_PATH, ", ".join(labels)])
	get_tree().quit(0)

func _shoot(overlay: Dictionary) -> Image:
	var spec: Array = overlay["call"]
	var singleton := get_node_or_null("/root/" + String(spec[0]))
	if singleton == null or not singleton.has_method(String(spec[1])):
		print("OVERLAY_SURVEY skip %s" % overlay["name"])
		return null
	singleton.callv(String(spec[1]), spec[2] as Array)
	await get_tree().create_timer(1.0).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/overlay_%s.png" % overlay["name"]))
	print("OVERLAY_SURVEY shot %s" % overlay["name"])
	var closer := String(overlay.get("close", ""))
	if closer != "":
		assert(singleton.has_method(closer), "%s에 닫기 메서드 %s가 없다" % [spec[0], closer])
		singleton.call(closer)
	# 시스템 로그는 3.5초 유지 후 0.8초에 걸쳐 사라진다. 토스트도 스스로 접힌다.
	# 닫기 메서드가 없는 것들은 다 사라질 때까지 기다려야 다음 장을 오염시키지 않는다.
	await get_tree().create_timer(1.2 if closer != "" else 5.0).timeout
	return image
