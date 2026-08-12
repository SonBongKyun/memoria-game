extends Node

## S242: 한국어 로케일에서 화면에 남아 있는 영어 UI 문자열을 전부 찾아낸다.
##
## 오버레이 서베이(capture_overlay_survey)에서 시트 전체를 관통하는 패턴이 나왔다.
## 로케일은 ko인데 저널 탭, 도감 항목, 상점 탭, 퍼즐 안내, 일시정지 정보 블록,
## 메모리 나침반이 전부 영어다. 대화는 한국어인데 UI 크롬만 영어로 남아 있다.
##
## 기존 validate_korean_localization.py는 대화 JSON만 본다(31파일 1592필드).
## UI 크롬은 어떤 검사도 받은 적이 없어서 지금까지 드러나지 않았다.
##
## 판정: 보이는 Label/Button/RichTextLabel의 텍스트에 ASCII 알파벳이 있고 한글이
## 하나도 없으면 신고한다. 고유명사(Kairos, BL-07)나 키 이름([ESC], Tab)처럼
## 영어로 두는 게 맞는 것도 걸리지만, 그 판단은 목록을 받은 뒤에 한다.

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
]

## 오버레이가 아니라 항상 떠 있는 것들. 여는 호출이 없으므로 따로 훑는다.
## 첫 계측에서 이것들을 빼먹었고, 콘택트 시트 아래 세 칸에 MEMORY COMPASS /
## THREADS HUM / FIELD FLOW가 그대로 남아 있는 걸 눈으로 보고서야 알았다.
const PERSISTENT: Array[String] = ["ExplorationHUD", "MemoryCompass"]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 5
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	# 저널 항목은 진행 플래그가 있어야 채워진다. 비어 있으면 계측기가 아무것도
	# 못 보고 0을 돌려주므로, 콘택트 시트와 같은 상태로 맞춘다.
	for flag in ["ch1_camp_done", "ch2_arrival_vn_seen", "ch2_arrived", "ch2_malet_done", "ch2_complete"]:
		GameManager.set_flag(flag)
	var backdrop: Node = load("res://scenes/maps/verdan_market.tscn").instantiate()
	add_child(backdrop)
	await get_tree().create_timer(1.4).timeout

	var total := 0
	for overlay in OVERLAYS:
		total += await _inspect(overlay)
	for singleton_name in PERSISTENT:
		var node := get_node_or_null("/root/" + singleton_name)
		if node == null:
			print("OVERLAY_LOCALE %-22s MISSING" % singleton_name)
			continue
		var persistent_found: Array[String] = []
		_walk(node, persistent_found)
		for line in persistent_found:
			print("OVERLAY_LOCALE %-22s %s" % [singleton_name, line])
		print("OVERLAY_LOCALE_COUNT %-18s %d" % [singleton_name, persistent_found.size()])
		total += persistent_found.size()
	print("OVERLAY_LOCALE_TOTAL english_strings=%d" % total)
	print("OVERLAY_LOCALE_DONE")
	get_tree().quit(0)

func _inspect(overlay: Dictionary) -> int:
	var spec: Array = overlay["call"]
	var singleton := get_node_or_null("/root/" + String(spec[0]))
	if singleton == null or not singleton.has_method(String(spec[1])):
		print("OVERLAY_LOCALE %-22s MISSING" % overlay["name"])
		return 0
	singleton.callv(String(spec[1]), spec[2] as Array)
	await get_tree().create_timer(0.9).timeout

	var found: Array[String] = []
	_walk(singleton, found)
	for line in found:
		print("OVERLAY_LOCALE %-22s %s" % [overlay["name"], line])
	print("OVERLAY_LOCALE_COUNT %-18s %d" % [overlay["name"], found.size()])

	var closer := String(overlay.get("close", ""))
	if closer != "" and singleton.has_method(closer):
		singleton.call(closer)
	await get_tree().create_timer(0.5).timeout
	return found.size()

func _walk(node: Node, found: Array[String]) -> void:
	var control := node as Control
	if control != null and control.visible:
		var text := ""
		if control is Label:
			text = (control as Label).text
		elif control is Button:
			text = (control as Button).text
		elif control is RichTextLabel:
			text = (control as RichTextLabel).get_parsed_text()
		if _is_english_only(text):
			found.append('"%s"' % text.replace("\n", " / ").substr(0, 90))
	for child in node.get_children():
		_walk(child, found)

## ASCII 알파벳이 있고 한글 음절/자모가 하나도 없으면 영어로 본다.
func _is_english_only(text: String) -> bool:
	var stripped := text.strip_edges()
	if stripped.length() < 2:
		return false
	var has_latin := false
	for i in range(stripped.length()):
		var c := stripped.unicode_at(i)
		if (c >= 0xAC00 and c <= 0xD7A3) or (c >= 0x3130 and c <= 0x318F):
			return false
		if (c >= 0x41 and c <= 0x5A) or (c >= 0x61 and c <= 0x7A):
			has_latin = true
	return has_latin
