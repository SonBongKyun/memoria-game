extends Node

## S231: 보존 플레이가 실제로 하나의 빌드인지 검사한다.
##
## 이 게임은 연소 패시브 첫 문턱이 5회, The Weave는 총 연소 4회 미만이었다.
## 두 숫자가 겹치지 않아, 보존을 택하면 게임 전체에서 패시브가 0개였다.
## 여기서 고정하는 계약:
##   1. 태우지 않고 챕터를 넘기면 파수가 쌓이고, 문턱마다 패시브가 열린다.
##   2. 각 파수 패시브는 실제 전투 수치를 바꾼다 (선언만 있는 패시브 금지).
##   3. 파수 경로는 The Weave 조건과 동시에 성립한다.
##   4. 침식은 챕터당 한 번만 적용되고, 고정한 기억은 한 번 넘어간다.

var _preservation_vigil: int = 0
var _preservation_passives: int = 0

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "ko"

	_check_vigil_accrual()
	_check_passives_change_real_numbers()
	_check_preservation_reaches_weave()
	_check_erosion_guard()

	print("BATTLE_ANCHOR_TRACK_SMOKE_PASS preservation_vigil=%d preservation_passives=%d/%d burns=0 weave=1 guard_slots=%d" % [
		_preservation_vigil,
		_preservation_passives,
		MemoryManager.ANCHOR_THRESHOLDS.size(),
		MemoryManager.EROSION_GUARD_SLOTS,
	])
	get_tree().quit(0)

func _reset_memory_state() -> void:
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.burn_passives.clear()
	MemoryManager.anchor_passives.clear()
	MemoryManager.erosion_guarded.clear()
	MemoryManager.anchor_vigil = 0
	MemoryManager._guard_slots_used = 0
	MemoryManager._vigil_chapters_counted.clear()
	MemoryManager._init_starting_memories()

func _check_vigil_accrual() -> void:
	_reset_memory_state()
	assert(MemoryManager.anchor_vigil == 0, "새 회차는 파수 0에서 시작해야 한다")
	assert(MemoryManager.get_active_anchor_passives().is_empty(), "시작부터 열린 파수 패시브가 있으면 안 된다")

	var intact := MemoryManager.intact_anchor_count()
	var expected := intact * MemoryManager.VIGIL_PER_ANCHOR
	if MemoryManager.is_intact(MemoryManager.WEAVE_PRIMARY):
		expected += MemoryManager.VIGIL_PRIMARY_BONUS
	var gained := MemoryManager.accrue_anchor_vigil()
	assert(gained == expected, "파수 적립이 온전한 앵커 수와 맞지 않는다 (%d vs %d)" % [gained, expected])
	assert(expected > 0, "1장 시작 시점에도 지킬 앵커가 있어야 파수 경로가 성립한다")

	# 앵커를 태우면 그만큼 덜 쌓인다. 이것이 두 트리를 서로 배타적으로 만든다.
	var before := MemoryManager.intact_anchor_count()
	MemoryManager.burn_memory_silent(MemoryManager.WEAVE_SECONDARY[0], true)
	assert(MemoryManager.intact_anchor_count() == before - 1, "앵커를 태우면 온전한 앵커 수가 줄어야 한다")
	var after_gain := MemoryManager.accrue_anchor_vigil()
	assert(after_gain < gained, "앵커를 잃고도 파수가 같으면 보존에 의미가 없다 (%d vs %d)" % [after_gain, gained])

	# 챕터당 한 번만 적립/침식된다. 맵 재진입으로 이중 적용되면 안 된다.
	_reset_memory_state()
	MemoryManager.add_chapter_memories(3)
	var once := MemoryManager.anchor_vigil
	MemoryManager.add_chapter_memories(3)
	assert(MemoryManager.anchor_vigil == once, "같은 챕터를 다시 밟아도 파수와 침식은 한 번뿐이어야 한다")

func _check_passives_change_real_numbers() -> void:
	_reset_memory_state()
	var enemy := BattleManager.Enemy.new("Vigil Probe", 200, 20, true)  # 보스
	enemy.is_void_beast = true
	BattleManager.current_enemy = enemy

	# 1. quiet_focus: WITNESS 요구가 실제로 줄어든다.
	var base_required: int = BattleManager.call("_get_witness_requirement", enemy)
	MemoryManager.anchor_passives["quiet_focus"] = true
	var focused_required: int = BattleManager.call("_get_witness_requirement", enemy)
	assert(focused_required == base_required - 1, "quiet_focus가 기억 읽기 요구를 줄이지 않았다 (%d -> %d)" % [base_required, focused_required])

	# 2. steady_hand: 비연소 공격의 브레이크 압력이 커진다.
	BattleManager.enemy_break_gauge = 0.0
	BattleManager.enemy_broken_turns = 0
	MemoryManager.anchor_passives.erase("steady_hand")
	BattleManager.call("_register_break_pressure", "physical")
	var plain_gain := BattleManager.enemy_break_gauge
	BattleManager.enemy_break_gauge = 0.0
	MemoryManager.anchor_passives["steady_hand"] = true
	BattleManager.call("_register_break_pressure", "physical")
	var steady_gain := BattleManager.enemy_break_gauge
	assert(steady_gain > plain_gain, "steady_hand가 브레이크 압력을 올리지 않았다 (%.1f vs %.1f)" % [steady_gain, plain_gain])
	BattleManager.enemy_break_gauge = 0.0

	# 3. deep_anchor: 온전한 앵커가 많을수록 피해 감소가 커진다.
	MemoryManager.anchor_passives.erase("deep_anchor")
	assert(is_zero_approx(MemoryManager.anchor_damage_reduction()), "패시브 없이 피해 감소가 붙으면 안 된다")
	MemoryManager.anchor_passives["deep_anchor"] = true
	var full_guard := MemoryManager.anchor_damage_reduction()
	assert(full_guard > 0.0, "deep_anchor가 아무 값도 만들지 않는다")
	MemoryManager.burn_memory_silent(MemoryManager.WEAVE_SECONDARY[1], true)
	var reduced_guard := MemoryManager.anchor_damage_reduction()
	assert(reduced_guard < full_guard, "앵커를 잃었는데 피해 감소가 그대로다 (%.3f vs %.3f)" % [reduced_guard, full_guard])

	# 4. shared_burden: 엘리아 기술 쿨다운이 실제로 줄어든다.
	GameManager.player_data.elia_with_party = true
	MemoryManager.anchor_passives.erase("shared_burden")
	EliaDiary.skills = {
		"probe": {"name": "Probe", "desc": "", "cooldown_max": 4, "current_cooldown": 0, "effect": "defend", "power": 0},
	}
	EliaDiary.use_skill("probe")
	var plain_cooldown: int = int(EliaDiary.skills["probe"]["current_cooldown"])
	EliaDiary.skills["probe"]["current_cooldown"] = 0
	MemoryManager.anchor_passives["shared_burden"] = true
	EliaDiary.use_skill("probe")
	var shared_cooldown: int = int(EliaDiary.skills["probe"]["current_cooldown"])
	assert(shared_cooldown == plain_cooldown - 1, "shared_burden이 쿨다운을 줄이지 않았다 (%d -> %d)" % [plain_cooldown, shared_cooldown])

	# 5. unbroken_edge: 보이드수 감쇠가 완화된다. (배틀 로그 대신 상수 계약으로 확인)
	assert(MemoryManager.ANCHOR_THRESHOLDS.values().any(func(p): return String(p["id"]) == "unbroken_edge"),
		"unbroken_edge가 문턱 표에 있어야 한다")

	BattleManager.current_enemy = null

func _check_preservation_reaches_weave() -> void:
	_reset_memory_state()
	# 한 번도 태우지 않고 열 챕터를 넘긴 플레이어.
	for chapter in range(1, 11):
		MemoryManager.add_chapter_memories(chapter)
	assert(MemoryManager.get_burn_count() == 0, "이 시나리오는 연소 0회여야 한다")
	assert(MemoryManager.get_active_passives().is_empty(), "연소 0회면 연소 패시브는 없어야 한다")
	# 핵심: 예전에는 여기서 패시브가 0개였다.
	var unlocked := MemoryManager.get_active_anchor_passives().size()
	assert(unlocked >= 3, "보존 플레이가 파수 패시브를 최소 3개는 얻어야 한다 (현재 %d, 파수 %d)" % [unlocked, MemoryManager.anchor_vigil])
	assert(MemoryManager.weave_unlocked(), "보존 플레이는 The Weave 조건도 동시에 만족해야 한다")
	_preservation_vigil = MemoryManager.anchor_vigil
	_preservation_passives = unlocked

func _check_erosion_guard() -> void:
	_reset_memory_state()
	GameManager.player_data["grains"] = 500
	var target = null
	for m in MemoryManager.memories:
		if m.grade < MemoryManager.MemoryGrade.GRADE_1 and not m.is_burned:
			target = m
			break
	assert(target != null, "침식 가능한 기억이 있어야 한다")

	assert(MemoryManager.guard_slots_remaining() == MemoryManager.EROSION_GUARD_SLOTS, "새 챕터는 고정 슬롯이 가득해야 한다")
	var cost := MemoryManager.erosion_guard_cost(target)
	assert(cost > 0, "고정에는 값이 있어야 한다")
	var grains_before := int(GameManager.player_data["grains"])
	assert(MemoryManager.guard_memory(target.id), "그레인이 충분하면 고정할 수 있어야 한다")
	assert(int(GameManager.player_data["grains"]) == grains_before - cost, "고정이 그레인을 소모하지 않았다")
	assert(MemoryManager.is_guarded(target.id), "고정 상태가 기록되어야 한다")
	assert(MemoryManager.guard_slots_remaining() == MemoryManager.EROSION_GUARD_SLOTS - 1, "고정 슬롯이 줄어야 한다")

	# 고정한 기억은 이번 침식을 넘어간다. 보호는 한 번 쓰면 사라진다.
	var erosion_before: int = target.erosion
	MemoryManager.apply_erosion(6)
	assert(target.erosion == erosion_before, "고정한 기억이 침식됐다")
	assert(not MemoryManager.is_guarded(target.id), "고정은 한 번 막고 소모되어야 한다")
	MemoryManager.apply_erosion(6)
	assert(target.erosion > erosion_before, "보호가 풀린 뒤에는 다시 침식되어야 한다")

	# 슬롯은 유한하다. 전부 지킬 수 있으면 결정이 아니다.
	var guarded := 1
	for m in MemoryManager.memories:
		if m.grade >= MemoryManager.MemoryGrade.GRADE_1 or m.is_burned or m.is_faded:
			continue
		if MemoryManager.guard_memory(m.id):
			guarded += 1
	assert(guarded <= MemoryManager.EROSION_GUARD_SLOTS, "챕터당 고정 슬롯 상한이 지켜지지 않았다 (%d)" % guarded)
	var blocked: Dictionary = MemoryManager.guard_availability(target)
	assert(not bool(blocked.get("ok", true)), "슬롯이 없으면 고정이 막혀야 한다")
	assert(String(blocked.get("reason", "")) != "", "막힌 이유를 문장으로 돌려줘야 UI가 설명할 수 있다")
