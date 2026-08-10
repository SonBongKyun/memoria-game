extends Node

## S234: 기억 텍스트 한국어화.
##
## 이 게임의 중심 결정은 "무엇을 태울 것인가"인데, 한국어판에서도 기억의 제목과
## 설명만 영어로 남아 있었다. 무엇을 잃는지 못 읽으면 선택의 무게도 없다.
## 여기서 고정하는 계약:
##   1. 모든 기억이 한국어 제목/설명을 갖는다. 스토리 효과가 있으면 그것도.
##   2. 로케일에 따라 표시 텍스트가 실제로 바뀐다.
##   3. 플레이어에게 보이는 경로(서고, 상점, 확인창, 관리국 로그)가 그 함수를 지난다.
##   4. 표가 없는 기억이 들어와도 영어로 떨어질 뿐, 빈 문자열이 되지 않는다.

func _ready() -> void:
	Codex.suppress_recording = true
	var previous_locale: String = GameManager.current_locale

	_check_every_memory_is_translated()
	_check_locale_switches_text()
	_check_display_paths_use_the_accessor()
	_check_unknown_memory_falls_back()

	GameManager.current_locale = previous_locale
	print("MEMORY_LOCALIZATION_SMOKE_PASS memories=%d translated=%d synthesis_ko=%d" % [
		MemoryManager.memories.size(),
		MemoryManager.MEMORY_TEXT_KO.size(),
		MemoryManager.SYNTHESIS_NAMES_KO.size(),
	])
	get_tree().quit(0)

func _check_every_memory_is_translated() -> void:
	# 게임에 등장하는 기억을 전부 만들어 놓고 검사한다.
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._vigil_chapters_counted.clear()
	MemoryManager._init_starting_memories()
	for chapter in range(2, 12):
		MemoryManager.add_chapter_memories(chapter)
	assert(MemoryManager.memories.size() >= 20, "검사 대상 기억이 충분히 만들어져야 한다 (현재 %d)" % MemoryManager.memories.size())
	# 챕터 경로로 만들어지지 않는 기억(맵/VN에서 추가되는 것들)까지 표가 덮어야 한다.
	assert(MemoryManager.MEMORY_TEXT_KO.size() >= 40, "한국어 표가 전체 기억을 덮지 못한다 (%d)" % MemoryManager.MEMORY_TEXT_KO.size())

	var missing := MemoryManager.untranslated_memory_ids()
	assert(missing.is_empty(), "한국어 텍스트가 없는 기억: %s" % ", ".join(missing))

	# 합성 결과도 같은 규칙을 따른다.
	for grade in MemoryManager.SYNTHESIS_NAMES:
		assert(MemoryManager.SYNTHESIS_NAMES_KO.has(grade), "합성 등급 %d의 한국어 이름이 없다" % grade)

func _check_locale_switches_text() -> void:
	var memory = MemoryManager.find_memory("identity_first_sword")
	assert(memory != null, "검사할 기억이 있어야 한다")

	GameManager.current_locale = "en"
	var en_title := MemoryManager.localized_memory_title(memory)
	var en_desc := MemoryManager.localized_memory_description(memory)
	var en_effect := MemoryManager.localized_memory_effect(memory)
	assert(en_title == memory.title, "영어 로케일은 원문을 그대로 써야 한다")
	assert(en_desc == memory.description, "영어 설명이 원문과 달라졌다")

	GameManager.current_locale = "ko"
	var ko_title := MemoryManager.localized_memory_title(memory)
	var ko_desc := MemoryManager.localized_memory_description(memory)
	var ko_effect := MemoryManager.localized_memory_effect(memory)
	assert(ko_title != en_title, "한국어 제목이 영어와 같다: %s" % ko_title)
	assert(ko_desc != en_desc, "한국어 설명이 영어와 같다")
	assert(ko_effect != en_effect, "한국어 스토리 효과가 영어와 같다")
	assert(_has_hangul(ko_title), "한국어 제목이 한국어로 쓰이지 않았다: %s" % ko_title)
	assert(ko_title.strip_edges() != "" and ko_desc.strip_edges() != "", "한국어 텍스트가 비었다")

	# 모든 기억의 한국어 제목에 영문이 섞이지 않았는지 훑는다.
	for m in MemoryManager.memories:
		var title := MemoryManager.localized_memory_title(m)
		assert(title.strip_edges() != "", "%s의 표시 제목이 비었다" % m.id)
		# BL-07 같은 고유 코드는 그대로 두되, 문장 자체는 한국어여야 한다.
		assert(_has_hangul(title), "%s의 제목이 번역되지 않았다: %s" % [m.id, title])
		assert(_has_hangul(MemoryManager.localized_memory_description(m)), "%s의 설명이 번역되지 않았다" % m.id)

func _check_display_paths_use_the_accessor() -> void:
	GameManager.current_locale = "ko"
	var memory = MemoryManager.find_memory("identity_first_sword")
	var ko_title := MemoryManager.localized_memory_title(memory)

	# 서고: 카드 목록과 상세가 한국어로 채워져야 한다.
	MemoryUI.call("_refresh_cards")
	MemoryUI.call("_show_detail", memory)
	assert(MemoryUI.detail_title.text == ko_title, "서고 상세 제목이 한국어가 아니다: %s" % MemoryUI.detail_title.text)
	assert(MemoryUI.detail_desc.text == MemoryManager.localized_memory_description(memory), "서고 설명이 한국어가 아니다")
	var card_titles: Array[String] = []
	for child in MemoryUI.card_list.get_children():
		if child is Button:
			card_titles.append((child as Button).text)
	assert(card_titles.any(func(t): return ko_title in t), "서고 카드 목록에 한국어 제목이 없다")

	# 상점: 판매 목록 버튼이 한국어여야 한다.
	MemoryShop._current_mode = "sell"
	MemoryShop.call("_refresh_items")
	var shop_titles: Array[String] = []
	for child in MemoryShop.item_list.get_children():
		if child is Button:
			shop_titles.append((child as Button).text)
	assert(shop_titles.any(func(t): return ko_title in t), "상점 판매 목록에 한국어 제목이 없다")

	# 관리국 감지 로그: 연소 시 표시되는 SUBJECT 줄.
	SystemLog.queue.clear()
	SystemLog.call("_on_memory_burned", memory)
	var logged := String(SystemLog.log_label.text) + " " + " ".join(SystemLog.queue)
	assert(ko_title in logged, "관리국 로그가 한국어 제목을 쓰지 않는다: %s" % logged)
	SystemLog.queue.clear()

func _check_unknown_memory_falls_back() -> void:
	GameManager.current_locale = "ko"
	# 표에 없는 기억이 나중에 추가되어도 빈 문자열이 되면 안 된다.
	var stray = MemoryManager.Memory.new(
		"zz_untranslated_probe", "Untranslated Probe", "No Korean entry exists for this one.",
		MemoryManager.MemoryGrade.GRADE_5, 10
	)
	assert(MemoryManager.localized_memory_title(stray) == "Untranslated Probe", "표가 없으면 원문으로 떨어져야 한다")
	assert(MemoryManager.localized_memory_description(stray) == stray.description, "설명도 원문으로 떨어져야 한다")
	assert(MemoryManager.localized_memory_title(null) == "", "null은 빈 문자열이어야 한다")

func _has_hangul(text: String) -> bool:
	for c in text:
		var code := c.unicode_at(0)
		if code >= 0xAC00 and code <= 0xD7A3:
			return true
	return false
