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

	func _init(p_name: String, p_hp: int, p_atk: int, p_void: bool = false) -> void:
		name = p_name
		hp = p_hp
		max_hp = p_hp
		attack = p_atk
		is_void_beast = p_void

	func is_alive() -> bool:
		return hp > 0

	func take_damage(amount: int) -> int:
		var actual = mini(amount, hp)
		hp -= actual
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
func start_battle(enemy: Enemy, from_scene: String = "") -> void:
	current_enemy = enemy
	return_scene = from_scene
	player_defending = false
	state = BattleState.PLAYER_TURN

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

	if current_enemy.is_void_beast:
		battle_log.emit("Your sword passes through. No effect.")
		damage_dealt.emit(current_enemy.name, 0, "Attack")
		_end_player_turn()
		return

	var base_dmg = 15 + randi_range(0, 10)
	var actual = current_enemy.take_damage(base_dmg)
	battle_log.emit("Arrel strikes! %d damage." % actual)
	damage_dealt.emit(current_enemy.name, actual, "Attack")
	_check_enemy_defeated()

## 플레이어 행동: 기억 연소 스킬
func player_burn(memory_id: String) -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	var memory = MemoryManager.burn_memory(memory_id)
	if memory == null:
		battle_log.emit("That memory is already gone.")
		return

	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	var dmg = skill.base_damage + memory.burn_power
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

	# 공허수는 도주 불가
	if current_enemy and current_enemy.is_void_beast:
		battle_log.emit("You can't run from a Void Beast.")
		return

	var chance = randf()
	if chance > 0.3:
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

	var base_dmg = current_enemy.attack + randi_range(0, 5)
	if player_defending:
		base_dmg = maxi(1, base_dmg / 2)
		battle_log.emit("Defended! Reduced damage.")

	player_defending = false

	# 플레이어 HP 감소
	GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - base_dmg)
	battle_log.emit("%s attacks! %d damage to Arrel." % [current_enemy.name, base_dmg])
	damage_dealt.emit("Arrel", base_dmg, current_enemy.name)

	# 사망 체크
	if GameManager.player_data.hp <= 0:
		state = BattleState.DEFEAT
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
		_end_player_turn()

func _end_player_turn() -> void:
	# 짧은 딜레이 후 적 턴 (UI 갱신 시간)
	await get_tree().create_timer(0.8).timeout
	_enemy_turn()

func _cleanup() -> void:
	# 승리 시 HP 20% 회복
	if state == BattleState.VICTORY:
		var heal = int(GameManager.player_data.max_hp * 0.2)
		GameManager.player_data.hp = mini(
			GameManager.player_data.hp + heal,
			GameManager.player_data.max_hp
		)
		battle_log.emit("Recovered %d HP." % heal)

	await get_tree().create_timer(1.5).timeout
	current_enemy = null
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	if return_scene != "":
		SceneTransition.change_scene(return_scene)
