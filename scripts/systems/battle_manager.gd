## BattleManager (Autoload)
## 턴제 전투 로직. 전투 시작/종료, 턴 관리, 데미지 계산.
extends Node

# --- 전투 상태 ---
enum BattleState { IDLE, PLAYER_TURN, ENEMY_TURN, VICTORY, DEFEAT, FLED }
var state: BattleState = BattleState.IDLE

# --- 적 데이터 클래스 ---
class Enemy:
	var name: String
	var hp: int
	var max_hp: int
	var attack: int
	var is_void_beast: bool  # true면 일반 공격 불가, 기억 연소만 유효
	var is_boss: bool = false  # 보스는 도주 불가 + 특수 패턴
	var phase: int = 1  # 보스 페이즈 (HP 50% 이하에서 2)
	var abilities: Array = []  # 특수 능력 목록 ("drain", "shield", "multi_hit")
	var weakness: String = ""   # 약점 속성 ("physical", "fire", "void")
	var resistance: String = "" # 저항 속성

	func _init(p_name: String, p_hp: int, p_atk: int, p_void: bool = false) -> void:
		name = p_name
		hp = p_hp
		max_hp = p_hp
		attack = p_atk
		is_void_beast = p_void
		# 기본 약점/저항 자동 설정
		# 보이드 수: is_void_beast 0.3배 감쇠가 별도 적용되므로 resistance="physical" 중복 방지
		if is_void_beast:
			weakness = "void"
			resistance = ""
		else:
			weakness = "fire"
			resistance = ""

	func is_alive() -> bool:
		return hp > 0

	var phase_changed: bool = false  # 페이즈 전환 감지용

	func take_damage(amount: int) -> int:
		var actual = mini(amount, hp)
		hp -= actual
		# 보스 페이즈 전환 체크
		if is_boss and phase == 1 and hp * 2 <= max_hp:
			phase = 2
			phase_changed = true
		return actual

# --- 속성 시스템 ---
# 공격 속성: physical(일반공격), fire(Grade 5~3 연소), void(Grade 2~1 연소)
# 약점 적중 = +50% 데미지, 저항 적중 = -30% 데미지
const ELEMENT_BONUS: float = 1.5   # 약점 보너스
const ELEMENT_RESIST: float = 0.7  # 저항 감쇠

# --- 기억 연소 스킬 ---
const BURN_SKILLS: Dictionary = {
	# grade: {name, base_damage, description, element}
	0: {"name": "Ember", "base_damage": 30, "desc": "A flicker of forgotten warmth.", "element": "fire"},
	1: {"name": "Blue Flame Slash", "base_damage": 60, "desc": "A blade edged with erased days.", "element": "fire"},
	2: {"name": "Incinerate", "base_damage": 120, "desc": "Bonds severed feed the fire.", "element": "fire"},
	3: {"name": "Identity Pyre", "base_damage": 250, "desc": "Who you were becomes what you wield.", "element": "void"},
	4: {"name": "Zero Burn", "base_damage": 999, "desc": "Everything. All of it. Gone.", "element": "void"},
}

# --- 상태이상 ---
enum StatusEffect { POISON, WEAKEN, BURN }

class StatusEntry:
	var effect: int  # StatusEffect enum 값
	var turns_left: int
	var power: int  # 독/화상: DoT 데미지, 약화: 공격력 감소%

	func _init(p_effect: int, p_turns: int, p_power: int) -> void:
		effect = p_effect
		turns_left = p_turns
		power = p_power

# --- 현재 전투 데이터 ---
var current_enemy: Enemy = null
var return_scene: String = ""  # 전투 후 돌아갈 씬
var player_defending: bool = false
var enemy_shielded: bool = false    # 적 방어 상태
var battle_bg_image: String = ""    # 전투 배경 이미지 경로
var enemy_image: String = ""        # 적 이미지 경로
var player_statuses: Array = []     # StatusEntry 배열
var enemy_statuses: Array = []      # StatusEntry 배열

# --- 콤보 시스템 ---
var combo_count: int = 0            # 연속 공격 횟수
var _last_action: String = ""       # 마지막 행동 ("attack", "burn", "defend", "item")

# --- 파티 시스템 ---
var sable_in_party: bool = false    # 세이블 동행 여부
var _boss_turn_counter: int = 0     # 보스 턴 카운터 (페이즈2 분노 패턴용)
signal combo_changed(count: int)
signal ally_action(ally_name: String, action: String, value: int)

# --- Limit Break 시스템 ---
var limit_gauge: float = 0.0        # 0.0 ~ 100.0
const LIMIT_MAX: float = 100.0
const LIMIT_GAIN_ATTACK: float = 8.0    # 공격 시
const LIMIT_GAIN_BURN: float = 12.0     # 연소 시
const LIMIT_GAIN_HIT: float = 15.0      # 피격 시
const LIMIT_GAIN_DEFEND: float = 5.0    # 방어 시
signal limit_changed(value: float)

## Limit 게이지 증가 헬퍼
func _add_limit(amount: float) -> void:
	limit_gauge = minf(limit_gauge + amount, LIMIT_MAX)
	limit_changed.emit(limit_gauge)

# --- 시그널 ---
signal battle_started(enemy: Enemy)
signal player_turn_started()
signal enemy_turn_started()
signal damage_dealt(target: String, amount: int, skill_name: String)
signal battle_ended(result: BattleState)
signal battle_log(message: String)
signal status_changed()

## 난이도별 적 스케일링 (Easy=0.7, Normal=1.0, Hard=1.4)
func _get_difficulty_scale() -> float:
	var diff = OptionsMenu.settings.get("difficulty", 1)
	match diff:
		0: return 0.7
		2: return 1.4
	return 1.0

func _ready() -> void:
	print("[BattleManager] Ready")

## 전투 시작
func start_battle(enemy: Enemy, from_scene: String = "", bg_image: String = "", e_image: String = "") -> void:
	current_enemy = enemy
	return_scene = from_scene
	battle_bg_image = bg_image
	enemy_image = e_image
	player_defending = false
	enemy_shielded = false
	player_statuses.clear()
	enemy_statuses.clear()
	combo_count = 0
	_last_action = ""
	_boss_turn_counter = 0
	sable_in_party = GameManager.get_flag("sable_joined") and GameManager.current_chapter >= 4
	limit_gauge = 0.0
	limit_changed.emit(0.0)
	state = BattleState.PLAYER_TURN

	# NG+ 적 스케일링
	var ng_scale = GameManager.get_ng_scale()
	if ng_scale > 1.0:
		enemy.hp = int(enemy.hp * ng_scale)
		enemy.max_hp = int(enemy.max_hp * ng_scale)
		enemy.attack = int(enemy.attack * ng_scale)

	# 난이도 스케일링 (Easy: 적 약화, Hard: 적 강화)
	var diff_scale = _get_difficulty_scale()
	if diff_scale != 1.0:
		enemy.hp = int(enemy.hp * diff_scale)
		enemy.max_hp = int(enemy.max_hp * diff_scale)
		enemy.attack = int(enemy.attack * diff_scale)

	# 챕터별 최대 HP 성장
	var chapter_hp = 100 + (GameManager.current_chapter - 1) * 15
	if GameManager.player_data.max_hp < chapter_hp:
		GameManager.player_data.max_hp = chapter_hp
		GameManager.player_data.hp = mini(GameManager.player_data.hp + 15, chapter_hp)

	GameManager.change_state(GameManager.GameState.BATTLE)
	battle_started.emit(enemy)
	battle_log.emit("A %s appears!" % enemy.name)

	if enemy.is_void_beast:
		battle_log.emit("It's a Void Beast — normal attacks are weakened.")

	player_turn_started.emit()

## 플레이어 행동: 일반 공격
func player_attack() -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	# 콤보 빌드
	if _last_action == "attack":
		combo_count += 1
	else:
		combo_count = 1
	_last_action = "attack"
	combo_changed.emit(combo_count)

	var base_dmg = _get_player_attack() + randi_range(0, 10)
	# 콤보 보너스 (2연속: +15%, 3연속: +30%, 4+: +50%)
	var combo_mult = _get_combo_multiplier()
	base_dmg = int(base_dmg * combo_mult)
	# 약화 적용
	base_dmg = int(base_dmg * _get_weaken_multiplier("player"))

	# 속성 상성 (물리)
	var elem_mult = _get_element_multiplier("physical")
	base_dmg = int(base_dmg * elem_mult)

	if current_enemy.is_void_beast:
		base_dmg = maxi(1, int(base_dmg * 0.3))
		battle_log.emit("Your blade struggles against the void...")

	if enemy_shielded:
		base_dmg = maxi(1, base_dmg / 2)
		enemy_shielded = false
		battle_log.emit("The barrier absorbs some damage!")
	var actual = current_enemy.take_damage(base_dmg)
	AudioManager.play_sfx("hit")
	var combo_text = " (Combo x%d!)" % combo_count if combo_count >= 2 else ""
	battle_log.emit("Arrel strikes! %d damage.%s" % [actual, combo_text])
	_log_element_effect("physical")
	damage_dealt.emit(current_enemy.name, actual, "Attack")
	_add_limit(LIMIT_GAIN_ATTACK)
	_check_enemy_defeated()

## 속성 상성 배율 계산
func _get_element_multiplier(attack_element: String) -> float:
	if current_enemy == null or attack_element == "":
		return 1.0
	if current_enemy.weakness == attack_element:
		return ELEMENT_BONUS
	if current_enemy.resistance == attack_element:
		return ELEMENT_RESIST
	return 1.0

## 속성 상성 로그 메시지
func _log_element_effect(attack_element: String) -> void:
	if current_enemy == null or attack_element == "":
		return
	if current_enemy.weakness == attack_element:
		battle_log.emit("It's super effective!")
	elif current_enemy.resistance == attack_element:
		battle_log.emit("It's not very effective...")

## 콤보 보너스 계수
func _get_combo_multiplier() -> float:
	match combo_count:
		2: return 1.15
		3: return 1.30
	if combo_count >= 4:
		return 1.50
	return 1.0

## 콤보 리셋 (비공격 행동 시)
func _reset_combo(action: String) -> void:
	_last_action = action
	combo_count = 0
	combo_changed.emit(combo_count)

## 챕터에 따른 플레이어 기본 공격력
func _get_player_attack() -> int:
	var base = 15
	var chapter_bonus = (GameManager.current_chapter - 1) * 3
	return base + chapter_bonus

## 플레이어 행동: 기억 연소 스킬
func player_burn(memory_id: String) -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	var memory = MemoryManager.burn_memory(memory_id)
	if memory == null:
		battle_log.emit("That memory is already gone.")
		return

	_reset_combo("burn")
	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	AudioManager.play_sfx("burn")
	var dmg = skill.base_damage + memory.burn_power
	# 속성 상성 (연소 속성)
	var burn_element = skill.get("element", "fire")
	var elem_mult = _get_element_multiplier(burn_element)
	dmg = int(dmg * elem_mult)
	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.7))
		enemy_shielded = false
		battle_log.emit("The barrier weakens the flames!")
	var actual = current_enemy.take_damage(dmg)

	battle_log.emit("[BURN] %s — %s" % [skill.name, skill.desc])
	battle_log.emit("%d damage to %s!" % [actual, current_enemy.name])
	_log_element_effect(burn_element)
	damage_dealt.emit(current_enemy.name, actual, skill.name)

	# 고등급 기억 연소 시 적에게 화상 DoT 부여 (Grade 2=Identity 이상)
	if memory.grade >= MemoryManager.MemoryGrade.GRADE_2:  # Grade 2(=3), Grade 1(=4)
		var burn_dot = int(memory.burn_power * 0.3) + 5
		apply_status("enemy", StatusEffect.BURN, 2, burn_dot)

	_add_limit(LIMIT_GAIN_BURN)
	_check_enemy_defeated()

## 플레이어 행동: 방어
func player_defend() -> void:
	if state != BattleState.PLAYER_TURN:
		return

	player_defending = true
	_reset_combo("defend")
	_add_limit(LIMIT_GAIN_DEFEND)
	battle_log.emit("Arrel braces for impact.")
	_end_player_turn()

## 플레이어 행동: 아이템 사용
func player_use_item(item_id: String) -> void:
	if state != BattleState.PLAYER_TURN:
		return

	var item_def = GameManager.ITEMS.get(item_id)
	if item_def == null:
		battle_log.emit("Unknown item.")
		return

	if not GameManager.remove_item(item_id):
		battle_log.emit("No %s left." % item_def["name"])
		return

	AudioManager.play_sfx("ui_select")
	AchievementManager.record_item_used()
	_reset_combo("item")

	match item_def["type"]:
		"heal":
			var heal_amount = item_def["power"]
			GameManager.player_data.hp = mini(
				GameManager.player_data.hp + heal_amount,
				GameManager.player_data.max_hp
			)
			AudioManager.play_sfx("heal")
			battle_log.emit("Used %s — restored %d HP." % [item_def["name"], heal_amount])
			damage_dealt.emit("Arrel", -heal_amount, item_def["name"])
		"cure":
			var cured = false
			var to_remove: Array = []
			for entry in player_statuses:
				if entry.effect == StatusEffect.POISON or entry.effect == StatusEffect.BURN:
					to_remove.append(entry)
					cured = true
			for e in to_remove:
				player_statuses.erase(e)
			if cured:
				status_changed.emit()
				battle_log.emit("Used %s — status effects cured!" % item_def["name"])
			else:
				battle_log.emit("Used %s — but nothing to cure." % item_def["name"])
		"burn":
			if current_enemy:
				apply_status("enemy", StatusEffect.BURN, 2, item_def["power"])
				battle_log.emit("Threw %s — enemy is burning!" % item_def["name"])
		"flee":
			battle_log.emit("Used %s — vanished in smoke!" % item_def["name"])
			AudioManager.play_sfx("flee")
			state = BattleState.FLED
			battle_ended.emit(BattleState.FLED)
			_cleanup()
			return

	_end_player_turn()

## 플레이어 행동: 도주
func player_flee() -> void:
	if state != BattleState.PLAYER_TURN:
		return

	# 공허수/보스는 도주 불가
	if current_enemy and (current_enemy.is_void_beast or current_enemy.is_boss):
		if current_enemy.is_boss:
			battle_log.emit("There's no running from this.")
		else:
			battle_log.emit("You can't run from a Void Beast.")
		return

	var chance = randf()
	if chance > 0.3:
		AudioManager.play_sfx("flee")
		battle_log.emit("Arrel escapes!")
		state = BattleState.FLED
		battle_ended.emit(BattleState.FLED)
		_cleanup()
	else:
		battle_log.emit("Couldn't get away!")
		_end_player_turn()

## 적 턴 처리
func _enemy_turn() -> void:
	if current_enemy == null or not current_enemy.is_alive():
		return
	if state == BattleState.VICTORY or state == BattleState.DEFEAT or state == BattleState.FLED:
		return

	state = BattleState.ENEMY_TURN
	enemy_turn_started.emit()

	# 특수 능력 선택 (보스 페이즈 2 또는 확률적)
	var used_ability = _try_enemy_ability()
	if used_ability:
		_check_player_defeated()
		return

	var base_dmg = current_enemy.attack + randi_range(0, 5)
	# 적 약화 적용
	base_dmg = int(base_dmg * _get_weaken_multiplier("enemy"))
	if player_defending:
		base_dmg = maxi(1, base_dmg / 2)
		battle_log.emit("Defended! Reduced damage.")

	player_defending = false

	# 플레이어 HP 감소
	GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - base_dmg)
	battle_log.emit("%s attacks! %d damage to Arrel." % [current_enemy.name, base_dmg])
	damage_dealt.emit("Arrel", base_dmg, current_enemy.name)
	_add_limit(LIMIT_GAIN_HIT)

	_check_player_defeated()

## 적 특수 능력 시도 — 전술적 AI
func _try_enemy_ability() -> bool:
	if current_enemy.abilities.is_empty():
		return false

	# 페이즈 2에서는 60% 확률, 페이즈 1에서는 30% 확률
	var chance = 0.6 if current_enemy.phase == 2 else 0.3
	if randf() > chance:
		return false

	_boss_turn_counter += 1

	var ability = _select_ability()
	if ability == "":
		return false

	# 보스 페이즈2 분노 패턴: 매 3턴 강화 공격
	var rage_bonus: float = 1.0
	if current_enemy.is_boss and current_enemy.phase == 2 and _boss_turn_counter % 3 == 0:
		rage_bonus = 1.3
		battle_log.emit("%s surges with dark fury!" % current_enemy.name)

	match ability:
		"drain":
			var dmg = int((current_enemy.attack + randi_range(5, 10)) * rage_bonus)
			if player_defending:
				dmg = maxi(1, dmg / 2)
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
			var heal = dmg / 2
			current_enemy.hp = mini(current_enemy.hp + heal, current_enemy.max_hp)
			AudioManager.play_sfx("drain")
			battle_log.emit("%s drains your life! %d damage, heals %d." % [current_enemy.name, dmg, heal])
			damage_dealt.emit("Arrel", dmg, "Drain")
			_add_limit(LIMIT_GAIN_HIT)
		"shield":
			enemy_shielded = true
			AudioManager.play_sfx("shield")
			battle_log.emit("%s raises a dark barrier." % current_enemy.name)
		"multi_hit":
			var total_dmg = 0
			var hits = 3 if rage_bonus > 1.0 else 2
			for i in range(hits):
				var hit = int(current_enemy.attack * 0.6) + randi_range(0, 3)
				if player_defending:
					hit = maxi(1, hit / 2)
				total_dmg += hit
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - total_dmg)
			battle_log.emit("%s strikes %d times! %d total damage." % [current_enemy.name, hits, total_dmg])
			damage_dealt.emit("Arrel", total_dmg, "Multi Hit")
			_add_limit(LIMIT_GAIN_HIT)
		"poison":
			var dot = int(current_enemy.attack * 0.3) + randi_range(2, 5)
			apply_status("player", StatusEffect.POISON, 3, dot)
			battle_log.emit("%s releases a toxic cloud!" % current_enemy.name)
		"burn_attack":
			var dmg_val = int((current_enemy.attack * 0.7 + randi_range(0, 5)) * rage_bonus)
			if player_defending:
				dmg_val = maxi(1, dmg_val / 2)
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg_val)
			battle_log.emit("%s scorches Arrel! %d damage." % [current_enemy.name, dmg_val])
			damage_dealt.emit("Arrel", dmg_val, "Scorch")
			_add_limit(LIMIT_GAIN_HIT)
			apply_status("player", StatusEffect.BURN, 2, int(current_enemy.attack * 0.2) + 3)
		"weaken":
			apply_status("player", StatusEffect.WEAKEN, 3, 30)
			battle_log.emit("%s curses Arrel's strength!" % current_enemy.name)
		"summon":
			var heal = int(current_enemy.max_hp * 0.15)
			current_enemy.hp = mini(current_enemy.hp + heal, current_enemy.max_hp)
			apply_status("player", StatusEffect.WEAKEN, 2, 20)
			AudioManager.play_sfx("shield")
			battle_log.emit("Shadows coalesce around %s. +%d HP." % [current_enemy.name, heal])
			battle_log.emit("The darkness saps your strength!")
			damage_dealt.emit(current_enemy.name, -heal, "Shadow Summon")
	return true

## 전술적 능력 선택 — 상황 분석 기반
func _select_ability() -> String:
	var abilities = current_enemy.abilities
	if abilities.is_empty():
		return ""

	var hp_ratio = float(current_enemy.hp) / max(current_enemy.max_hp, 1)

	# 1. 위기 시 자가 치유 우선 (HP < 30%)
	if hp_ratio < 0.3 and "drain" in abilities and randf() < 0.7:
		return "drain"
	if hp_ratio < 0.3 and "summon" in abilities and randf() < 0.6:
		return "summon"

	# 2. 플레이어 콤보 방어 (combo >= 3 → shield 우선)
	if combo_count >= 3 and "shield" in abilities and not enemy_shielded and randf() < 0.6:
		return "shield"

	# 3. 방어 미사용 시 multi_hit 활용
	if not player_defending and "multi_hit" in abilities and randf() < 0.5:
		return "multi_hit"

	# 4. 중복 상태이상 회피 — 이미 걸려있으면 다른 능력 선택
	var filtered: Array = []
	for a in abilities:
		match a:
			"poison":
				if not has_status("player", StatusEffect.POISON):
					filtered.append(a)
			"burn_attack":
				if not has_status("player", StatusEffect.BURN):
					filtered.append(a)
			"weaken":
				if not has_status("player", StatusEffect.WEAKEN):
					filtered.append(a)
			_:
				filtered.append(a)

	# 5. 보스 페이즈2 전용: summon 우선
	if current_enemy.is_boss and current_enemy.phase == 2 and "summon" in filtered and randf() < 0.4:
		return "summon"

	if filtered.is_empty():
		filtered = abilities  # 모두 중복이면 그냥 아무거나

	return filtered[randi_range(0, filtered.size() - 1)]

## 플레이어 사망 체크
func _check_player_defeated() -> void:
	if GameManager.player_data.hp <= 0:
		state = BattleState.DEFEAT
		AudioManager.play_sfx("defeat")
		battle_log.emit("Arrel falls...")
		battle_ended.emit(BattleState.DEFEAT)
		_cleanup()
		return

	# 플레이어 상태이상 처리 (독/화상 DoT)
	if not player_statuses.is_empty():
		_process_statuses("player")
		if GameManager.player_data.hp <= 0:
			state = BattleState.DEFEAT
			AudioManager.play_sfx("defeat")
			battle_log.emit("Arrel succumbs...")
			battle_ended.emit(BattleState.DEFEAT)
			_cleanup()
			return

	# 다시 플레이어 턴
	state = BattleState.PLAYER_TURN
	player_turn_started.emit()

func _check_enemy_defeated() -> void:
	if current_enemy and not current_enemy.is_alive():
		state = BattleState.VICTORY
		battle_log.emit("%s is defeated!" % current_enemy.name)
		battle_ended.emit(BattleState.VICTORY)
		_cleanup()
	else:
		# 보스 페이즈 전환 알림
		if current_enemy and current_enemy.phase_changed:
			current_enemy.phase_changed = false
			AudioManager.play_sfx("phase_change")
			battle_log.emit("%s staggers... then surges with renewed fury!" % current_enemy.name)
		_end_player_turn()

func _end_player_turn() -> void:
	# 세이블 지원 행동 (파티에 있을 때, 40% 확률)
	if sable_in_party and current_enemy and current_enemy.is_alive() and randf() < 0.4:
		_sable_support_action()

	# 적 상태이상 처리 (독/화상 DoT)
	if current_enemy and not enemy_statuses.is_empty():
		_process_statuses("enemy")
		if current_enemy and not current_enemy.is_alive():
			state = BattleState.VICTORY
			battle_log.emit("%s is defeated!" % current_enemy.name)
			battle_ended.emit(BattleState.VICTORY)
			_cleanup()
			return
	# 짧은 딜레이 후 적 턴 (UI 갱신 시간)
	await get_tree().create_timer(0.8).timeout
	_enemy_turn()

## 세이블 지원 행동 (랜덤)
func _sable_support_action() -> void:
	var actions = ["heal", "strike", "weaken"]
	var action = actions[randi_range(0, actions.size() - 1)]
	match action:
		"heal":
			var heal = randi_range(10, 20)
			GameManager.player_data.hp = mini(GameManager.player_data.hp + heal, GameManager.player_data.max_hp)
			battle_log.emit("Sable mends your wounds. +%d HP." % heal)
			ally_action.emit("Sable", "heal", heal)
			damage_dealt.emit("Arrel", -heal, "Sable Heal")
		"strike":
			var dmg = randi_range(8, 18)
			if current_enemy:
				var actual = current_enemy.take_damage(dmg)
				battle_log.emit("Sable strikes from the shadows! %d damage." % actual)
				ally_action.emit("Sable", "strike", actual)
				damage_dealt.emit(current_enemy.name, actual, "Sable Strike")
		"weaken":
			apply_status("enemy", StatusEffect.WEAKEN, 2, 20)
			battle_log.emit("Sable disrupts the enemy's stance!")
			ally_action.emit("Sable", "weaken", 20)

## 전투 승리 시 Grains 보상 계산
func _get_grains_reward() -> int:
	if current_enemy == null:
		return 0
	var base = 3
	if current_enemy.is_void_beast:
		base = 8
	if current_enemy.is_boss:
		base = 20
	# 적 HP 기반 보너스
	base += current_enemy.max_hp / 20
	return base

func _cleanup() -> void:
	if state == BattleState.VICTORY:
		# 승리 시 HP 20% 회복
		var heal = int(GameManager.player_data.max_hp * 0.2)
		GameManager.player_data.hp = mini(
			GameManager.player_data.hp + heal,
			GameManager.player_data.max_hp
		)
		AudioManager.play_sfx("heal")
		battle_log.emit("Recovered %d HP." % heal)

		# Grains 보상
		var grains = _get_grains_reward()
		GameManager.player_data.grains += grains
		battle_log.emit("Gained %d Grains." % grains)
		NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)
		AchievementManager.check_grains()

		# 아이템 드롭 (30% 확률)
		_try_item_drop()
	elif state == BattleState.DEFEAT:
		battle_log.emit("Darkness closes in...")
		await get_tree().create_timer(1.5).timeout
		# 게임 오버 화면으로 전환 (current_enemy/return_scene 유지)
		SceneTransition.change_scene("res://scenes/ui/game_over.tscn")
		return

	await get_tree().create_timer(1.5).timeout
	current_enemy = null
	player_statuses.clear()
	enemy_statuses.clear()
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	if return_scene != "":
		SceneTransition.change_scene(return_scene)

## 전투 승리 시 아이템 드롭
func _try_item_drop() -> void:
	if randf() > 0.30:  # 30% 확률
		return
	var drop_table: Array = ["potion", "potion", "potion", "antidote", "antidote", "firebomb"]
	if current_enemy and current_enemy.is_void_beast:
		drop_table.append_array(["firebomb", "hi_potion"])
	if current_enemy and current_enemy.is_boss:
		drop_table.append_array(["hi_potion", "hi_potion", "smoke_bomb"])
	var drop = drop_table[randi_range(0, drop_table.size() - 1)]
	GameManager.add_item(drop)
	battle_log.emit("Found: %s" % GameManager.ITEMS[drop]["name"])

## ===================== 상태이상 시스템 =====================

## 상태이상 적용 (대상: "player" 또는 "enemy")
func apply_status(target: String, effect: StatusEffect, turns: int, power: int) -> void:
	var list = player_statuses if target == "player" else enemy_statuses
	# 같은 효과 중복 시 강한 쪽으로 갱신
	for entry in list:
		if entry.effect == effect:
			if power >= entry.power:
				entry.turns_left = turns
				entry.power = power
			status_changed.emit()
			return
	list.append(StatusEntry.new(effect, turns, power))
	status_changed.emit()

	var effect_name = _get_status_name(effect)
	var target_name = "Arrel" if target == "player" else current_enemy.name if current_enemy else "Enemy"
	battle_log.emit("%s is afflicted with %s!" % [target_name, effect_name])

## 턴 시작 시 상태이상 처리 (DoT 등)
func _process_statuses(target: String) -> void:
	var list = player_statuses if target == "player" else enemy_statuses
	var expired: Array = []

	for entry in list:
		match entry.effect:
			StatusEffect.POISON:
				var dmg = entry.power
				if target == "player":
					GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
					battle_log.emit("Poison deals %d damage to Arrel." % dmg)
					damage_dealt.emit("Arrel", dmg, "Poison")
				else:
					if current_enemy:
						current_enemy.take_damage(dmg)
						battle_log.emit("Poison deals %d damage to %s." % [dmg, current_enemy.name])
						damage_dealt.emit(current_enemy.name, dmg, "Poison")
			StatusEffect.BURN:
				var dmg = entry.power
				if target == "player":
					GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
					battle_log.emit("Burn deals %d damage to Arrel." % dmg)
					damage_dealt.emit("Arrel", dmg, "Burn")
				else:
					if current_enemy:
						current_enemy.take_damage(dmg)
						battle_log.emit("Burn sears %s for %d damage." % [current_enemy.name, dmg])
						damage_dealt.emit(current_enemy.name, dmg, "Burn")
			StatusEffect.WEAKEN:
				pass  # 약화는 공격 시 적용됨

		entry.turns_left -= 1
		if entry.turns_left <= 0:
			expired.append(entry)

	for e in expired:
		list.erase(e)
		var effect_name = _get_status_name(e.effect)
		var target_name = "Arrel" if target == "player" else current_enemy.name if current_enemy else "Enemy"
		battle_log.emit("%s's %s wears off." % [target_name, effect_name])

	if not expired.is_empty():
		status_changed.emit()

## 약화 계수 가져오기 (1.0 = 약화 없음, 0.7 = 30% 감소)
func _get_weaken_multiplier(target: String) -> float:
	var list = player_statuses if target == "player" else enemy_statuses
	for entry in list:
		if entry.effect == StatusEffect.WEAKEN:
			return 1.0 - (entry.power / 100.0)
	return 1.0

## 상태이상 이름
func _get_status_name(effect: StatusEffect) -> String:
	match effect:
		StatusEffect.POISON: return "Poison"
		StatusEffect.WEAKEN: return "Weaken"
		StatusEffect.BURN: return "Burn"
	return "Unknown"

## 상태이상 보유 여부
func has_status(target: String, effect: StatusEffect) -> bool:
	var list = player_statuses if target == "player" else enemy_statuses
	for entry in list:
		if entry.effect == effect:
			return true
	return false

## 대상의 모든 상태이상 반환
func get_statuses(target: String) -> Array:
	return player_statuses if target == "player" else enemy_statuses

## ===================== Limit Break =====================

## 플레이어 궁극기 — 게이지 100% 시 사용 가능
func player_limit_break() -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return
	if limit_gauge < LIMIT_MAX:
		battle_log.emit("Limit gauge not full yet.")
		return

	_reset_combo("limit")
	limit_gauge = 0.0
	limit_changed.emit(0.0)

	# 데미지: 기본 300 + 챕터 보너스 + 연소 횟수 보너스
	var base = 300
	var chapter_bonus = (GameManager.current_chapter - 1) * 40
	var burn_bonus = MemoryManager.get_burn_count() * 15
	var dmg = base + chapter_bonus + burn_bonus

	# 속성: void (고등급 기술)
	var elem_mult = _get_element_multiplier("void")
	dmg = int(dmg * elem_mult)

	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.5))
		enemy_shielded = false
		battle_log.emit("The barrier cracks under the weight!")

	var actual = current_enemy.take_damage(dmg)
	AudioManager.play_sfx("burn")
	battle_log.emit("[LIMIT BREAK] Memory Cascade!")
	battle_log.emit("All remembered pain converges — %d damage!" % actual)
	_log_element_effect("void")
	damage_dealt.emit(current_enemy.name, actual, "Memory Cascade")

	# 적에게 약화 부여
	apply_status("enemy", StatusEffect.WEAKEN, 2, 25)

	_check_enemy_defeated()

## ===================== Residue Burn (잔존 기억 재사용) =====================

## 잔존 기억으로 약한 연소 (50% 데미지, 기억 소멸 없음)
func player_burn_residue(memory_id: String) -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	var memory = MemoryManager.get_residue_memory(memory_id)
	if memory == null:
		battle_log.emit("That residue is too faint to use.")
		return

	_reset_combo("burn")
	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	AudioManager.play_sfx("burn")
	var dmg = int((skill.base_damage + memory.burn_power) * 0.5)
	var burn_element = skill.get("element", "fire")
	var elem_mult = _get_element_multiplier(burn_element)
	dmg = int(dmg * elem_mult)

	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.7))
		enemy_shielded = false
		battle_log.emit("The barrier weakens the residue flames!")
	var actual = current_enemy.take_damage(dmg)

	battle_log.emit("[RESIDUE] %s — a faded echo of %s" % [skill.name, memory.title])
	battle_log.emit("%d damage to %s. (50%% power)" % [actual, current_enemy.name])
	_log_element_effect(burn_element)
	damage_dealt.emit(current_enemy.name, actual, "Residue: " + skill.name)
	_add_limit(LIMIT_GAIN_BURN * 0.5)

	_check_enemy_defeated()
