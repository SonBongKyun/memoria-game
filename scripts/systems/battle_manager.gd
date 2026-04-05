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

	func _init(p_name: String, p_hp: int, p_atk: int, p_void: bool = false) -> void:
		name = p_name
		hp = p_hp
		max_hp = p_hp
		attack = p_atk
		is_void_beast = p_void

	func is_alive() -> bool:
		return hp > 0

	var phase_changed: bool = false  # 페이즈 전환 감지용

	func take_damage(amount: int) -> int:
		var actual = mini(amount, hp)
		hp -= actual
		# 보스 페이즈 전환 체크
		if is_boss and phase == 1 and hp <= max_hp / 2.0:
			phase = 2
			phase_changed = true
		return actual

# --- 기억 연소 스킬 ---
const BURN_SKILLS: Dictionary = {
	# grade: {name, base_damage, description}
	0: {"name": "Ember", "base_damage": 30, "desc": "A flicker of forgotten warmth."},
	1: {"name": "Blue Flame Slash", "base_damage": 60, "desc": "A blade edged with erased days."},
	2: {"name": "Incinerate", "base_damage": 120, "desc": "Bonds severed feed the fire."},
	3: {"name": "Identity Pyre", "base_damage": 250, "desc": "Who you were becomes what you wield."},
	4: {"name": "Zero Burn", "base_damage": 999, "desc": "Everything. All of it. Gone."},
}

# --- 현재 전투 데이터 ---
var current_enemy: Enemy = null
var return_scene: String = ""  # 전투 후 돌아갈 씬
var player_defending: bool = false
var enemy_shielded: bool = false    # 적 방어 상태
var battle_bg_image: String = ""    # 전투 배경 이미지 경로
var enemy_image: String = ""        # 적 이미지 경로

# --- 시그널 ---
signal battle_started(enemy: Enemy)
signal player_turn_started()
signal enemy_turn_started()
signal damage_dealt(target: String, amount: int, skill_name: String)
signal battle_ended(result: BattleState)
signal battle_log(message: String)

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
	state = BattleState.PLAYER_TURN

	# 챕터별 최대 HP 성장
	var chapter_hp = 100 + (GameManager.current_chapter - 1) * 15
	if GameManager.player_data.max_hp < chapter_hp:
		GameManager.player_data.max_hp = chapter_hp
		GameManager.player_data.hp = mini(GameManager.player_data.hp + 15, chapter_hp)

	GameManager.change_state(GameManager.GameState.BATTLE)
	battle_started.emit(enemy)
	battle_log.emit("A %s appears!" % enemy.name)

	if enemy.is_void_beast:
		battle_log.emit("It's a Void Beast — normal attacks won't work.")

	player_turn_started.emit()

## 플레이어 행동: 일반 공격
func player_attack() -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	var base_dmg = _get_player_attack() + randi_range(0, 10)

	if current_enemy.is_void_beast:
		# 보이드 적에게 30% 감쇠 (완전 무효 아님)
		base_dmg = maxi(1, int(base_dmg * 0.3))
		battle_log.emit("Your blade struggles against the void...")

	if enemy_shielded:
		base_dmg = maxi(1, base_dmg / 2.0)
		enemy_shielded = false
		battle_log.emit("The barrier absorbs some damage!")
	var actual = current_enemy.take_damage(base_dmg)
	AudioManager.play_sfx("hit")
	battle_log.emit("Arrel strikes! %d damage." % actual)
	damage_dealt.emit(current_enemy.name, actual, "Attack")
	_check_enemy_defeated()

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

	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	AudioManager.play_sfx("burn")
	var dmg = skill.base_damage + memory.burn_power
	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.7))
		enemy_shielded = false
		battle_log.emit("The barrier weakens the flames!")
	var actual = current_enemy.take_damage(dmg)

	battle_log.emit("[BURN] %s — %s" % [skill.name, skill.desc])
	battle_log.emit("%d damage to %s!" % [actual, current_enemy.name])
	damage_dealt.emit(current_enemy.name, actual, skill.name)
	_check_enemy_defeated()

## 플레이어 행동: 방어
func player_defend() -> void:
	if state != BattleState.PLAYER_TURN:
		return

	player_defending = true
	battle_log.emit("Arrel braces for impact.")
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

	state = BattleState.ENEMY_TURN
	enemy_turn_started.emit()

	# 특수 능력 선택 (보스 페이즈 2 또는 확률적)
	var used_ability = _try_enemy_ability()
	if used_ability:
		_check_player_defeated()
		return

	var base_dmg = current_enemy.attack + randi_range(0, 5)
	if player_defending:
		base_dmg = maxi(1, base_dmg / 2.0)
		battle_log.emit("Defended! Reduced damage.")

	player_defending = false

	# 플레이어 HP 감소
	GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - base_dmg)
	battle_log.emit("%s attacks! %d damage to Arrel." % [current_enemy.name, base_dmg])
	damage_dealt.emit("Arrel", base_dmg, current_enemy.name)

	_check_player_defeated()

## 적 특수 능력 시도
func _try_enemy_ability() -> bool:
	if current_enemy.abilities.is_empty():
		return false

	# 페이즈 2에서는 50% 확률, 페이즈 1에서는 25% 확률
	var chance = 0.5 if current_enemy.phase == 2 else 0.25
	if randf() > chance:
		return false

	var ability = current_enemy.abilities[randi_range(0, current_enemy.abilities.size() - 1)]
	match ability:
		"drain":  # HP 흡수 공격
			var dmg = current_enemy.attack + randi_range(5, 10)
			if player_defending:
				dmg = maxi(1, dmg / 2.0)
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
			var heal = int(dmg / 2.0)
			current_enemy.hp = mini(current_enemy.hp + heal, current_enemy.max_hp)
			AudioManager.play_sfx("drain")
			battle_log.emit("%s drains your life! %d damage, heals %d." % [current_enemy.name, dmg, heal])
			damage_dealt.emit("Arrel", dmg, "Drain")
		"shield":  # 다음 턴 방어력 상승 (50% 데미지 감소 효과)
			enemy_shielded = true
			AudioManager.play_sfx("shield")
			battle_log.emit("%s raises a dark barrier." % current_enemy.name)
		"multi_hit":  # 연속 공격 (약한 2타)
			var total_dmg = 0
			for i in range(2):
				var hit = int(current_enemy.attack * 0.6) + randi_range(0, 3)
				if player_defending:
					hit = maxi(1, hit / 2.0)
				total_dmg += hit
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - total_dmg)
			battle_log.emit("%s strikes twice! %d total damage." % [current_enemy.name, total_dmg])
			damage_dealt.emit("Arrel", total_dmg, "Multi Hit")
	return true

## 플레이어 사망 체크
func _check_player_defeated() -> void:
	if GameManager.player_data.hp <= 0:
		state = BattleState.DEFEAT
		AudioManager.play_sfx("defeat")
		battle_log.emit("Arrel falls...")
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
	# 짧은 딜레이 후 적 턴 (UI 갱신 시간)
	await get_tree().create_timer(0.8).timeout
	_enemy_turn()

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
	elif state == BattleState.DEFEAT:
		# 패배 시 HP 30%로 회복 + 맵 복귀 (관대 처리)
		GameManager.player_data.hp = int(GameManager.player_data.max_hp * 0.3)
		battle_log.emit("Something pulls you back from the edge...")
		battle_log.emit("You stagger to your feet. HP partially restored.")

	await get_tree().create_timer(1.5).timeout
	current_enemy = null
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	if return_scene != "":
		SceneTransition.change_scene(return_scene)
