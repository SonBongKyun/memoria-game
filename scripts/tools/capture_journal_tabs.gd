extends Node

## S244: 저널의 다섯 탭을 각각 렌더해 한 장에 모은다.
##
## 오버레이 로케일 프로브는 열려 있는 탭만 본다. 저널 항목 65개를 한국어로
## 옮긴 뒤 프로브는 0을 돌려주지만, 그건 기본 탭(사건)만 확인한 값이다.
## 특히 세계 탭은 본문이 100자를 넘는 문단이라 한국어로 바꾸면 줄바꿈과
## 상세 패널 높이가 달라진다. 숫자로는 안 보이고 봐야 아는 종류다.
##
## 항목이 채워지려면 진행 플래그가 필요하다. 표에 있는 플래그를 전부 세운다.

const OUTPUT_PATH := "res://tmp/visual_audit/journal_tabs.png"
const SHOT_SIZE := Vector2i(640, 360)
const TABS: Array[String] = ["events", "npcs", "world", "choices", "losses"]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 10
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	# 표가 참조하는 플래그를 전부 세워서 모든 항목이 목록에 뜨게 한다.
	for table in [StoryJournal.EVENT_ENTRIES, StoryJournal.NPC_ENTRIES, StoryJournal.WORLD_ENTRIES]:
		for entry in table:
			GameManager.set_flag(String(entry.get("flag", "")))
	await get_tree().process_frame

	StoryJournal.open_journal()
	await get_tree().create_timer(0.9).timeout

	var shots: Array[Image] = []
	for tab in TABS:
		StoryJournal.set("_current_tab", tab)
		StoryJournal.call("_refresh_list")
		await get_tree().create_timer(0.5).timeout
		# 목록만 보면 상세 패널이 "항목을 고르세요..." 상태로 남는다. 긴 한국어
		# 문단이 실제로 어떻게 흐르는지가 가장 위험한 자리이므로 하나를 눌러 둔다.
		_press_longest_entry()
		await get_tree().create_timer(0.4).timeout
		await RenderingServer.frame_post_draw
		var image := get_viewport().get_texture().get_image()
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
		image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/journal_%s.png" % tab))
		shots.append(image)
		print("JOURNAL_TABS shot %s" % tab)

	var columns := 3
	var rows := int(ceil(float(shots.size()) / float(columns)))
	var sheet := Image.create(SHOT_SIZE.x * columns, SHOT_SIZE.y * rows, false, shots[0].get_format())
	sheet.fill(Color(0.02, 0.02, 0.03))
	for i in range(shots.size()):
		var shot := shots[i]
		shot.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, SHOT_SIZE),
			Vector2i((i % columns) * SHOT_SIZE.x, (i / columns) * SHOT_SIZE.y))
	assert(sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "콘택트 시트를 저장하지 못했다")
	print("JOURNAL_TABS_CAPTURE_PASS path=%s tabs=%d" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)

## 목록에서 이름이 가장 긴 버튼을 누른다. 본문도 대체로 그쪽이 길다.
func _press_longest_entry() -> void:
	var list := StoryJournal.get("item_list") as Node
	if list == null:
		return
	var best: Button = null
	for child in list.get_children():
		var btn := child as Button
		if btn != null and (best == null or btn.text.length() > best.text.length()):
			best = btn
	if best != null:
		best.emit_signal("pressed")
