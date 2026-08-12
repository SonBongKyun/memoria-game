extends Node

## S242: 화면보다 큰 Control을 전부 찾아낸다.
##
## S241에서 튜토리얼 패널이 768x806(뷰포트 720px)이었다. 원인은 두 겹이었고
## 둘 다 이 프로젝트 어디서든 반복될 수 있는 종류다.
##   1. autowrap 라벨은 폭이 정해지기 전에 최소 크기를 물으면 "한 글자 폭으로
##      접었을 때의 높이"를 돌려준다. 한국어는 자간이 좁아 글자 수가 많으므로
##      영어보다 훨씬 크게 부푼다.
##   2. Control.position 세터는 size_cache를 보존하며 offset을 다시 쓴다.
##      낡은 크기가 offset에 굳으면 최소 크기를 고쳐도 되돌아오지 않는다.
##
## autowrap 라벨은 18개 파일에 41곳 있다. 하나씩 눈으로 볼 게 아니라, 오버레이를
## 실제로 열고 트리를 걸어 뷰포트를 넘는 Control을 전부 신고하게 한다.
##
## 판정은 크기만 본다. 화면보다 큰 Control이 항상 버그인 것은 아니지만(스크롤
## 컨테이너의 내용물 등), 어떤 것이 그런지는 목록을 받은 뒤에 판단한다.

const OVERLAYS: Array[Dictionary] = [
	{"name": "tutorial_hint", "call": ["TutorialHints", "show_hint", ["first_directive"]]},
	{"name": "memory_archive", "call": ["MemoryUI", "open_archive", []]},
	{"name": "story_journal", "call": ["StoryJournal", "open_journal", []]},
	{"name": "story_log", "call": ["StoryLog", "open_log", []]},
	{"name": "codex", "call": ["Codex", "open", []]},
	{"name": "memory_shop", "call": ["MemoryShop", "open_shop", ["Malet"]]},
	{"name": "memory_puzzle", "call": ["MemoryPuzzle", "open_puzzle", [4, 15]]},
	{"name": "options_menu", "call": ["OptionsMenu", "open", []]},
	{"name": "pause_menu", "call": ["PauseMenu", "_open", []]},
	{"name": "system_log", "call": ["SystemLog", "show_log", ["관리국 감지: 기억 연소 흔적"]]},
	{"name": "notification_toast", "call": ["NotificationToast", "show_toast", ["기억을 잃었습니다"]]},
	{"name": "memory_constellation", "call": ["MemoryConstellation", "open", []]},
]

var _viewport_size: Vector2

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 5
	await get_tree().process_frame
	_viewport_size = get_viewport().get_visible_rect().size

	var offenders := 0
	for overlay in OVERLAYS:
		offenders += await _inspect(overlay)
	print("OVERLAY_BOUNDS_TOTAL offenders=%d viewport=%.0fx%.0f" % [
		offenders, _viewport_size.x, _viewport_size.y])
	print("OVERLAY_BOUNDS_DONE")
	get_tree().quit(0)

func _inspect(overlay: Dictionary) -> int:
	var spec: Array = overlay["call"]
	var singleton := get_node_or_null("/root/" + String(spec[0]))
	if singleton == null:
		print("OVERLAY_BOUNDS %-22s MISSING" % overlay["name"])
		return 0
	if not singleton.has_method(String(spec[1])):
		print("OVERLAY_BOUNDS %-22s NO_METHOD %s" % [overlay["name"], spec[1]])
		return 0
	singleton.callv(String(spec[1]), spec[2] as Array)
	# 레이아웃과 트윈이 자리를 잡을 시간을 준다.
	await get_tree().create_timer(0.9).timeout

	var offenders: Array[String] = []
	_walk(singleton, singleton, offenders)
	for line in offenders:
		print("OVERLAY_BOUNDS %-22s %s" % [overlay["name"], line])
	if offenders.is_empty():
		print("OVERLAY_BOUNDS %-22s ok" % overlay["name"])
	return offenders.size()

func _walk(node: Node, root: Node, offenders: Array[String]) -> void:
	var control := node as Control
	if control != null and control.visible:
		var size := control.size
		var over_x := size.x > _viewport_size.x + 1.0
		var over_y := size.y > _viewport_size.y + 1.0
		if over_x or over_y:
			# 스크롤 컨테이너 안쪽은 화면보다 커도 정상이다. 조상에 ScrollContainer가
			# 있으면 표시해 두고, 판단은 목록을 본 뒤에 한다.
			var scrolled := "scrolled" if _inside_scroll(control, root) else "LOOSE"
			offenders.append("%-8s %6.0fx%-6.0f %s  %s" % [
				scrolled, size.x, size.y, control.get_class(), _path_of(control, root)])
	for child in node.get_children():
		_walk(child, root, offenders)

func _inside_scroll(control: Control, root: Node) -> bool:
	var parent := control.get_parent()
	while parent != null and parent != root.get_parent():
		if parent is ScrollContainer:
			return true
		parent = parent.get_parent()
	return false

func _path_of(control: Control, root: Node) -> String:
	var parts: Array[String] = []
	var node: Node = control
	while node != null and node != root:
		parts.push_front(node.name)
		node = node.get_parent()
	return "/".join(parts)
