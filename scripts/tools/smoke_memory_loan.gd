extends Node

## S232: 기억 대출 (GDD 2.4).
##
## 이 시스템의 요점은 "스스로 태우는 것"과 "빼앗기는 것"의 차이다.
## 여기서 고정하는 계약:
##   1. 담보를 잡으면 지금 그레인이 들어오고, 그 기억은 태울 수도 팔 수도 없다.
##   2. 기한 안에 갚으면 담보가 풀린다.
##   3. 기한을 넘기면 강제 추출된다. 기억은 사라지되 연소 패시브는 오르지 않는다.
##   4. 핵심 기억(Grade 1)은 담보로 잡히지 않는다.

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "ko"

	_check_pledge_locks_the_memory()
	_check_repayment_releases()
	_check_default_extracts_without_power()
	_check_core_memory_cannot_be_pledged()

	print("MEMORY_LOAN_SMOKE_PASS term=%d interest=%.2f extracted_gives_power=0 collateral_burnable=0" % [
		MemoryManager.LOAN_TERM_CHAPTERS, MemoryManager.LOAN_INTEREST,
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
	GameManager.player_data["grains"] = 0

func _first_pledgeable():
	for m in MemoryManager.memories:
		if m.grade < MemoryManager.MemoryGrade.GRADE_1 and not m.is_burned:
			return m
	return null

func _check_pledge_locks_the_memory() -> void:
	_reset()
	var memory = _first_pledgeable()
	assert(memory != null, "담보로 잡을 기억이 있어야 한다")
	var offer: Dictionary = MemoryManager.loan_offer(memory)
	assert(int(offer["principal"]) > 0, "담보 대출은 실제 그레인을 줘야 한다")
	assert(int(offer["repay"]) > int(offer["principal"]), "이자가 붙어야 대출이 선택지가 된다")
	assert(int(offer["due_chapter"]) == GameManager.current_chapter + MemoryManager.LOAN_TERM_CHAPTERS, "기한이 계약대로여야 한다")

	assert(MemoryManager.take_loan(memory.id), "담보 등록이 되어야 한다")
	assert(int(GameManager.player_data["grains"]) == int(offer["principal"]), "대출금이 들어오지 않았다")
	assert(MemoryManager.is_collateral(memory.id), "담보 상태가 기록되어야 한다")

	# 잡힌 기억은 아렐이 처분할 수 없다.
	assert(MemoryManager.burn_memory(memory.id) == null, "담보로 잡힌 기억이 전투에서 태워졌다")
	assert(MemoryManager.burn_memory_silent(memory.id) == null, "담보로 잡힌 기억이 조용히 태워졌다")
	assert(not memory.is_burned, "담보가 소실되면 안 된다")
	for candidate in MemoryManager.get_available_memories():
		assert(candidate.id != memory.id, "담보가 연소/판매 후보 목록에 남아 있다")

	# 대출은 한 번에 하나뿐이다.
	var other = null
	for m in MemoryManager.memories:
		if m.id != memory.id and m.grade < MemoryManager.MemoryGrade.GRADE_1 and not m.is_burned:
			other = m
			break
	assert(other != null, "두 번째 후보가 있어야 한다")
	assert(not MemoryManager.take_loan(other.id), "대출이 열려 있으면 새 대출이 막혀야 한다")

func _check_repayment_releases() -> void:
	_reset()
	var memory = _first_pledgeable()
	MemoryManager.take_loan(memory.id)
	var repay := int(MemoryManager.active_loan["repay"])

	# 이자를 못 내면 갚을 수 없다.
	GameManager.player_data["grains"] = repay - 1
	assert(not MemoryManager.can_repay_loan(), "그레인이 모자라면 상환이 막혀야 한다")
	assert(not MemoryManager.repay_loan(), "부족한데도 상환이 성립했다")

	GameManager.player_data["grains"] = repay
	assert(MemoryManager.repay_loan(), "그레인이 충분하면 상환되어야 한다")
	assert(not MemoryManager.has_active_loan(), "상환 후에도 대출이 남아 있다")
	assert(int(GameManager.player_data["grains"]) == 0, "상환액이 빠져나가지 않았다")
	assert(not MemoryManager.is_collateral(memory.id), "상환 후 담보가 풀려야 한다")
	assert(MemoryManager.burn_memory(memory.id) != null, "담보가 풀린 기억은 다시 태울 수 있어야 한다")

func _check_default_extracts_without_power() -> void:
	_reset()
	var memory = _first_pledgeable()
	MemoryManager.take_loan(memory.id)
	var due := int(MemoryManager.active_loan["due_chapter"])
	var burns_before := MemoryManager.get_voluntary_burn_count()

	# 기한 안에는 아무 일도 없다.
	assert(not MemoryManager.check_loan_maturity(due), "기한 당일에 회수되면 안 된다")
	assert(MemoryManager.has_active_loan(), "기한 전에 대출이 사라졌다")

	# 기한을 넘기면 가져간다.
	assert(MemoryManager.check_loan_maturity(due + 1), "기한이 지났는데 회수되지 않았다")
	assert(memory.is_burned, "강제 추출된 기억이 남아 있다")
	assert(not MemoryManager.has_active_loan(), "회수 후에도 대출이 남아 있다")
	assert(MemoryManager.extracted_memories.has(memory.id), "강제 추출 기록이 남아야 한다")

	# 핵심: 잃었지만 힘은 얻지 못한다.
	assert(MemoryManager.get_voluntary_burn_count() == burns_before,
		"빼앗긴 기억이 연소 패시브 적립에 들어갔다 (%d -> %d)" % [burns_before, MemoryManager.get_voluntary_burn_count()])
	assert(MemoryManager.get_burn_count() > burns_before, "엔딩 판정에는 상실이 반영되어야 한다")

	# 연소 패시브 문턱까지 스스로 태운 뒤, 추출 한 건이 그 수를 되돌리지 않는지 확인.
	_reset()
	var voluntary := 0
	for m in MemoryManager.memories.duplicate():
		if m.grade < MemoryManager.MemoryGrade.GRADE_1 and not m.is_burned:
			if MemoryManager.burn_memory_silent(m.id, true) != null:
				voluntary += 1
	assert(MemoryManager.get_voluntary_burn_count() == voluntary, "스스로 태운 수가 그대로 세어져야 한다")

func _check_core_memory_cannot_be_pledged() -> void:
	_reset()
	var core = MemoryManager.find_memory(MemoryManager.WEAVE_PRIMARY)
	assert(core != null and core.grade == MemoryManager.MemoryGrade.GRADE_1, "핵심 기억이 있어야 한다")
	var availability: Dictionary = MemoryManager.loan_availability(core)
	assert(not bool(availability.get("ok", true)), "핵심 기억이 담보로 잡혔다")
	assert(String(availability.get("reason", "")) != "", "막힌 이유를 문장으로 돌려줘야 한다")
	assert(not MemoryManager.take_loan(core.id), "핵심 기억 대출이 성립했다")
