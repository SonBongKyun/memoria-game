## S216 회귀 테스트 — 미니맵 POI, 퀘스트 트래커, 사이드 퀘스트 한국어
extends Node

func _ready() -> void:
	var prev_flags := GameManager.story_flags.duplicate(true)
	var prev_chapter := GameManager.current_chapter
	var prev_locale := GameManager.current_locale
	GameManager.current_locale = "ko"

	_check_quest_localization()
	await _check_quest_tracker()
	await _check_minimap_poi()

	GameManager.story_flags = prev_flags
	GameManager.current_chapter = prev_chapter
	GameManager.current_locale = prev_locale
	print("RPG_SYSTEMS_SMOKE_PASS quests=%d" % SideQuest.get_all_quests().size())
	get_tree().quit(0)

## 사이드 퀘스트는 한국어가 기본인 게임에서 영문 제목을 그대로 띄우고 있었다.
func _check_quest_localization() -> void:
	GameManager.current_locale = "ko"
	var quests := SideQuest.get_all_quests()
	assert(quests.size() >= 6, "사이드 퀘스트가 등록되어 있어야 한다")
	for q: Dictionary in quests:
		var title := SideQuest.loc(q, "title")
		assert(title != "", "%s 에 제목이 있어야 한다" % q.get("id", "?"))
		assert(_has_hangul(title), "한국어 로케일에서 '%s' 는 번역되어야 한다" % title)
		for step: Dictionary in q.get("steps", []):
			var text := SideQuest.loc(step, "desc")
			assert(_has_hangul(text), "단계 설명 '%s' 이 번역되지 않았다" % text)
	# 영어 로케일에서는 원문이 유지되어야 한다.
	GameManager.current_locale = "en"
	assert(not _has_hangul(SideQuest.loc(quests[0], "title")), "영어 로케일은 원문을 써야 한다")
	GameManager.current_locale = "ko"

func _has_hangul(text: String) -> bool:
	for i in range(text.length()):
		var c := text.unicode_at(i)
		if c >= 0xAC00 and c <= 0xD7A3:
			return true
	return false

## 사이드 퀘스트를 수락해도 이야기 목표가 사라지면 안 된다.
func _check_quest_tracker() -> void:
	GameManager.current_chapter = 1
	GameManager.story_flags = {}
	ExplorationHUD.call("_update_quest_tracker")
	var story_line: String = ExplorationHUD.quest_label.text
	assert(story_line != "" and ExplorationHUD.quest_label.visible, "이야기 목표가 보여야 한다")
	assert(not ExplorationHUD.quest_side_label.visible, "의뢰가 없으면 의뢰 줄은 숨어야 한다")

	# 의뢰 하나를 진행 중으로 만든다.
	GameManager.current_chapter = 2
	GameManager.set_flag("sq_echoes_ash_started")
	ExplorationHUD.call("_update_quest_tracker")
	assert(ExplorationHUD.quest_label.visible,
		"의뢰를 수락해도 이야기 목표는 남아 있어야 한다")
	assert(ExplorationHUD.quest_side_label.visible, "진행 중인 의뢰가 표시되어야 한다")
	var side_text: String = ExplorationHUD.quest_side_label.text
	assert("재 속의 메아리" in side_text, "의뢰 제목이 한국어로 나와야 한다: %s" % side_text)
	assert("(1/4)" in side_text, "의뢰 진행도가 보여야 한다: %s" % side_text)
	assert("이끼" in side_text, "다음에 할 일이 보여야 한다: %s" % side_text)
	assert(ExplorationHUD.quest_tag_label.text.contains("이야기"),
		"이야기 목표가 있을 때 태그는 이야기 흐름이어야 한다")
	await get_tree().process_frame

## 미니맵은 맵에 실제로 놓인 발견물을 근처에서 알려 줘야 한다.
func _check_minimap_poi() -> void:
	GameManager.story_flags = {}
	GameManager.current_chapter = 2
	var map: Node2D = load("res://scenes/maps/verdan_market.tscn").instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame

	var points: Array = Minimap._collect_points_of_interest(map)
	assert(points.size() >= 3,
		"맵에 놓인 발견물/유물/주민이 미니맵 후보로 잡혀야 한다 (현재 %d)" % points.size())
	var kinds := {}
	for p: Dictionary in points:
		kinds[String(p.get("kind", ""))] = true
	assert(kinds.has("curio"), "지역 유물이 POI로 잡혀야 한다")

	var data: Dictionary = map.get("_minimap_data")
	assert(not data.is_empty(), "미니맵이 만들어져야 한다")
	var markers: Array = data.get("poi_markers", [])
	assert(markers.size() == points.size(), "POI마다 마커가 있어야 한다")

	# 멀리 있으면 감춰지고, 가까이 가면 드러난다.
	var far := Vector2(-4000, -4000)
	Minimap.update_minimap(data, far, 32)
	var visible_far := 0
	for m: ColorRect in markers:
		if m.visible:
			visible_far += 1
	assert(visible_far == 0, "멀리 있는 발견물은 미리 드러나면 안 된다 (탐색이 사라진다)")

	var near_point: Vector2 = points[0].get("pos", Vector2.ZERO)
	Minimap.update_minimap(data, near_point, 32)
	var visible_near := 0
	for m: ColorRect in markers:
		if m.visible:
			visible_near += 1
	assert(visible_near >= 1, "가까이 가면 발견물이 미니맵에 떠야 한다")
	var legend := data.get("legend") as Control
	assert(legend != null and legend.visible, "마커가 뜨면 범례도 함께 보여야 한다")

	map.queue_free()
	await get_tree().process_frame
