extends Node

## S233: 기억 연쇄.
##
## 기억 그래프는 S62부터 있었지만 쓰임은 연쇄 연소 +20%뿐이었고,
## "무엇을 태울까"는 사실상 "제일 싼 걸 태운다"였다.
## 여기서 고정하는 계약:
##   1. 태우면 이어진 기억이 실제로 침식된다.
##   2. 이어진 기억이 많은 쪽(허브)을 태우는 값이 더 비싸다.
##   3. 연쇄는 확인창에서 커밋 전에 이름으로 예보된다. 숨은 대가가 아니다.
##   4. 고정한 기억은 연쇄도 한 번 막고, 그 보호를 소모한다.
##   5. 핵심 기억은 연쇄로 상하지 않는다 (침식 면역과 같은 규칙).
##   6. 강제 추출은 스스로 태울 때보다 세게 번진다.

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "ko"

	_check_cascade_actually_erodes()
	_check_hub_costs_more_than_isolated()
	_check_preview_matches_effect()
	_check_guard_absorbs_cascade()
	_check_core_memory_immune()
	_check_extraction_is_harsher()

	print("MEMORY_CASCADE_SMOKE_PASS grades=%d extraction_mult=%.1f preview=named guard=absorbs core=immune" % [
		MemoryManager.CASCADE_EROSION.size(), MemoryManager.CASCADE_EXTRACTION_MULT,
	])
	get_tree().quit(0)

func _reset() -> void:
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.burn_passives.clear()
	MemoryManager.anchor_passives.clear()
	MemoryManager.erosion_guarded.clear()
	MemoryManager.extracted_memories.clear()
	MemoryManager.active_loan = {}
	MemoryManager.anchor_vigil = 0
	MemoryManager._guard_slots_used = 0
	MemoryManager._vigil_chapters_counted.clear()
	MemoryManager._init_starting_memories()
	GameManager.current_chapter = 1
	GameManager.player_data["grains"] = 999
	GameManager.player_data.elia_with_party = false

func _linked_memory():
	# 이웃이 하나라도 있는, 태울 수 있는 기억.
	for m in MemoryManager.memories:
		if m.is_burned or m.grade >= MemoryManager.MemoryGrade.GRADE_1:
			continue
		if not MemoryManager.cascade_targets(m).is_empty():
			return m
	return null

func _check_cascade_actually_erodes() -> void:
	_reset()
	var source = _linked_memory()
	assert(source != null, "이웃이 있는 기억이 있어야 연쇄를 검사할 수 있다")
	var targets := MemoryManager.cascade_targets(source)
	assert(not targets.is_empty(), "연쇄 대상이 있어야 한다")
	var before: Dictionary = {}
	for t in targets:
		before[t.id] = t.erosion
	var amount := MemoryManager.cascade_amount(source)
	assert(amount > 0, "연쇄량이 0이면 그래프는 여전히 장식이다")

	MemoryManager.burn_memory_silent(source.id)
	var moved := 0
	for t in targets:
		if t.erosion == int(before[t.id]) + amount:
			moved += 1
	assert(moved > 0, "태웠는데 이어진 기억이 하나도 상하지 않았다")

func _check_hub_costs_more_than_isolated() -> void:
	_reset()
	var most = null
	var fewest = null
	for m in MemoryManager.memories:
		if m.is_burned or m.grade >= MemoryManager.MemoryGrade.GRADE_1:
			continue
		var links := MemoryManager.cascade_targets(m).size()
		if most == null or links > MemoryManager.cascade_targets(most).size():
			most = m
		if fewest == null or links < MemoryManager.cascade_targets(fewest).size():
			fewest = m
	assert(most != null and fewest != null, "비교할 기억이 두 개는 있어야 한다")
	var hub_links := MemoryManager.cascade_targets(most).size()
	var lone_links := MemoryManager.cascade_targets(fewest).size()
	assert(hub_links > lone_links,
		"허브와 외딴 기억의 연결 수가 같으면 '어느 것을 태울까'가 성립하지 않는다 (%d vs %d)" % [hub_links, lone_links])

func _check_preview_matches_effect() -> void:
	_reset()
	var source = _linked_memory()
	var preview: Dictionary = MemoryManager.cascade_preview(source)
	assert(int(preview.get("count", 0)) > 0, "예보가 대상 수를 알려 줘야 한다")
	assert(int(preview.get("amount", 0)) == MemoryManager.cascade_amount(source), "예보 침식량이 실제와 달랐다")
	var titles: Array = preview.get("titles", [])
	assert(titles.size() == int(preview.get("count", 0)), "예보는 이름을 그대로 돌려줘야 UI가 적을 수 있다")
	for title in titles:
		assert(String(title).strip_edges() != "", "예보 이름이 비어 있다")
	# 예보한 대상과 실제로 상하는 대상이 같아야 한다.
	# S234 이후 예보는 표시용(로케일 반영) 제목을 돌려준다. UI가 그대로 적을 수 있어야 한다.
	var predicted: Array[String] = []
	for t in MemoryManager.cascade_targets(source):
		predicted.append(MemoryManager.localized_memory_title(t))
	for title in titles:
		assert(predicted.has(title), "예보에 없는 기억이 상한다: %s" % title)

func _check_guard_absorbs_cascade() -> void:
	_reset()
	var source = _linked_memory()
	var targets := MemoryManager.cascade_targets(source)
	var protected = targets[0]
	assert(MemoryManager.guard_memory(protected.id), "연쇄 대상을 고정할 수 있어야 한다")
	var preview: Dictionary = MemoryManager.cascade_preview(source)
	assert(Array(preview.get("guarded", [])).has(MemoryManager.localized_memory_title(protected)),
		"예보가 고정된 대상을 따로 알려 줘야 한다")

	var before: int = protected.erosion
	MemoryManager.burn_memory_silent(source.id)
	assert(protected.erosion == before, "고정한 기억이 연쇄로 상했다")
	assert(not MemoryManager.is_guarded(protected.id), "연쇄를 막았으면 보호가 소모되어야 한다")

func _check_core_memory_immune() -> void:
	_reset()
	var core = MemoryManager.find_memory(MemoryManager.WEAVE_PRIMARY)
	assert(core != null, "핵심 기억이 있어야 한다")
	for m in MemoryManager.memories:
		for target in MemoryManager.cascade_targets(m):
			assert(target.grade < MemoryManager.MemoryGrade.GRADE_1, "핵심 기억이 연쇄 대상에 들어갔다")

func _check_extraction_is_harsher() -> void:
	_reset()
	var source = _linked_memory()
	var voluntary := MemoryManager.cascade_amount(source, false)
	var forced := MemoryManager.cascade_amount(source, true)
	assert(forced > voluntary, "빼앗기는 쪽이 더 세게 번져야 한다 (%d vs %d)" % [forced, voluntary])

	# 실제 추출 경로에서도 그 값이 쓰이는지 확인.
	var targets := MemoryManager.cascade_targets(source)
	var watched = targets[0]
	var before: int = watched.erosion
	MemoryManager.take_loan(source.id)
	MemoryManager.check_loan_maturity(int(MemoryManager.active_loan["due_chapter"]) + 1)
	assert(source.is_burned, "강제 추출이 일어나야 한다")
	assert(watched.erosion == before + forced,
		"추출 연쇄가 강화된 값으로 적용되지 않았다 (%d -> %d, 기대 +%d)" % [before, watched.erosion, forced])
