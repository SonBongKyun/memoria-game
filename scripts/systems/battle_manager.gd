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
			InputManager.vibrate("boss_phase")
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
const ART_KAIROS_FULLBODY: String = "res://assets/cg/generated/cinematic_kairos_watcher_confrontation.png"
const ART_KAIROS_FALLBACK: String = "res://assets/cg/game_image/kairos_fullbody.png"
const ART_NERA_FULLBODY: String = "res://assets/cg/game_image/nera_fullbody.png"
const ART_TOBIAS_FULLBODY: String = "res://assets/cg/game_image/tobias_fullbody.png"
const ART_VEIL_FULLBODY: String = "res://assets/cg/game_image/veil_fullbody.png"
const ART_VOID_BEAST: String = "res://assets/cg/generated/cinematic_void_beast_memory_devour.png"
const ART_VOID_BEAST_FALLBACK: String = "res://assets/cg/game_image/void_beast_confrontation.png"
const ART_SHADE_SENTINEL: String = "res://assets/cg/generated/cinematic_shade_sentinel_phase2.png"
const ART_VOID_CREATURE_SHEET: String = "res://assets/game_image/reference/void_creature_sprite_sheet.png"
const ART_MEMORY_LOST_SOLDIER: String = "res://assets/game_image/reference/memory_lost_soldier_sprite_sheet.png"
const ART_FORGOTTEN_GUARDIAN: String = "res://assets/game_image/reference/forgotten_guardian_sheet.png"
var player_statuses: Array = []     # StatusEntry 배열
var enemy_statuses: Array = []      # StatusEntry 배열
var enemy_break_gauge: float = 0.0
var enemy_broken_turns: int = 0

# --- 콤보 시스템 ---
var combo_count: int = 0            # 연속 공격 횟수
var _last_action: String = ""       # 마지막 행동 ("attack", "burn", "defend", "item")

# --- 파티 시스템 ---
var sable_in_party: bool = false    # 세이블 동행 여부
var tobias_in_party: bool = false   # 토비아스 동행 여부
var _boss_turn_counter: int = 0     # 보스 턴 카운터 (페이즈2 분노 패턴용)
var _encounter_modifier: Dictionary = {}  # S51: 인카운터 수정자
var _total_turns: int = 0           # S51: 턴 카운터 (수정자용)
var _burn_chain: int = 0  # S53: 연속 연소 카운터
# S55: Auto Battle
var auto_battle: bool = false
signal auto_battle_changed(enabled: bool)
signal combo_changed(count: int)
signal ally_action(ally_name: String, action: String, value: int)
signal phase_changed(enemy_name: String, phase: int)
signal enemy_ability_telegraph(ability_name: String, delay: float)  # S59: telegraph before enemy special

# --- S59: Difficulty scaling ---
var difficulty_bonus: float = 0.0  # Chapter + NG+ damage multiplier

# --- Bestiary Scan ---
var scanned_enemies: Array = []  # 이번 전투에서 스캔된 적 이름 목록
signal enemy_scanned(enemy_name: String, weakness: String, resistance: String)
signal break_changed(value: float, max_value: float)
signal enemy_broken(enemy_name: String)
signal tactical_objective_changed(objective: Dictionary)
signal momentum_changed(value: float, rank: int, label: String)

# --- 아군 조작 모드 ---
var ally_command: String = ""  # 플레이어가 선택한 세이블 행동 ("", "heal", "strike", "weaken", "guard")
var ally_command_pending: bool = false  # 세이블 행동 대기 중
var tobias_command: String = ""  # 토비아스 명령 ("", "analyze", "archive", "protect")
var tobias_command_pending: bool = false  # 토비아스 행동 대기 중

# --- 신규 능력 상태 ---
var _player_stunned: bool = false   # 기절: 다음 플레이어 턴 스킵
var _enemy_reflecting: bool = false # 반사: 다음 공격 30% 반사
var _enemy_charged: bool = false    # 차지: 다음 적 턴 2배 데미지

# --- Memory Echo 시스템 (S51: 기억 연소 후 전장 잔류 효과) ---
# 기억을 태우면 등급/관련 NPC에 따라 전장에 남는 효과
var active_echoes: Array = []  # [{id, grade, npc, type, power, turns}]
signal echo_activated(echo_type: String, desc: String)

# --- Battle Stance 시스템 (S51: 전투 자세 전환) ---
enum Stance { REMNANT, PYRE, HOLLOW }
var current_stance: Stance = Stance.REMNANT
signal stance_changed(stance: int)

const STANCE_INFO: Dictionary = {
	Stance.REMNANT: {"name": "Remnant", "desc": "Balanced. Special: Cling (+30% burn, no residue)", "atk_mult": 1.0, "def_mult": 1.0, "unlock_chapter": 1},
	Stance.PYRE: {"name": "Pyre", "desc": "Aggressive. +25% ATK, -20% DEF. Special: Immolate (burn 2 at once)", "atk_mult": 1.25, "def_mult": 0.8, "unlock_chapter": 4},
	Stance.HOLLOW: {"name": "Hollow", "desc": "Tactical. -15% ATK, +30% DEF, 2x combo mult", "atk_mult": 0.85, "def_mult": 1.3, "unlock_chapter": 7},
}

# --- Battle Environment 시스템 ---
var battle_environment: String = ""  # 맵 이름 기반
signal environment_info(env_name: String, bonus_text: String)

const ENV_BONUSES: Dictionary = {
	"rim_forest": {"name": "Rim Forest", "desc": "+5% evasion", "evasion": 0.05},
	"verdan_market": {"name": "Verdan Market", "desc": "No bonus (safe zone)", "evasion": 0.0},
	"belt_waystation": {"name": "Belt Waystation", "desc": "+3% item effectiveness", "item_boost": 0.03},
	"drift_shelter": {"name": "Drift Shelter", "desc": "-10% enemy accuracy (rain)", "enemy_miss": 0.10},
	"crumbling_coast": {"name": "Crumbling Coast", "desc": "+5% physical damage", "phys_boost": 0.05},
	"the_seam": {"name": "The Seam", "desc": "+10% void damage (both sides)", "void_boost": 0.10},
	"seam_outskirts": {"name": "Seam Outskirts", "desc": "+5% status effect chance", "status_boost": 0.05},
	"forgotten_forest": {"name": "Forgotten Forest", "desc": "+8% poison damage", "poison_boost": 0.08},
	"colorless_waste": {"name": "Colorless Waste", "desc": "-5% all damage (muted)", "dmg_reduce": 0.05},
	"bl07_void": {"name": "BL-07 Void", "desc": "+15% void damage, -10% healing", "void_boost": 0.15, "heal_reduce": 0.10},
}

const ENEMY_PRESETS: Dictionary = {
	"ash_crawler": {"name": "Ash Crawler", "hp": 45, "atk": 10, "is_void": false, "abilities": ["poison"], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": "res://assets/cg/game_image/void_beast_confrontation.png"},
	"forest_shade": {"name": "Forest Shade", "hp": 55, "atk": 12, "is_void": false, "abilities": ["poison"], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": ""},
	"void_beast": {"name": "Void Beast", "hp": 80, "atk": 15, "is_void": true, "abilities": ["drain"], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": "res://assets/cg/generated/cinematic_void_beast_memory_devour.png"},
	"threshold_shade": {"name": "Threshold Shade", "hp": 120, "atk": 20, "is_void": true, "abilities": ["drain", "stun", "reflect"], "weakness": "fire"},
	"shade_sentinel": {"name": "Shade Sentinel", "hp": 180, "atk": 24, "is_void": true, "is_boss": true, "abilities": ["drain", "shield", "multi_hit", "summon"], "weakness": "void", "resistance": "fire", "bg": "res://assets/cg/generated/chapter_splash_the_seam.png", "img": "res://assets/cg/generated/cinematic_shade_sentinel_phase2.png"},
	"kairos": {"name": "Kairos, Authority Editor", "hp": 450, "atk": 38, "is_void": true, "is_boss": true, "abilities": ["void_pulse", "drain", "stun", "reflect", "charge", "despair"], "weakness": "physical", "resistance": "void", "bg": "res://assets/cg/generated/memory_compass_resonance_cinematic.png", "img": "res://assets/cg/generated/cinematic_kairos_watcher_confrontation.png"},
}

## 맵 씬 경로에서 환경 이름 추출
func _detect_environment(scene_path: String) -> String:
	for env_key in ENV_BONUSES:
		if scene_path.contains(env_key):
			return env_key
	return ""

## 환경 속성 배율 계산 (공격 속성에 따라)
func _get_env_element_mult(attack_element: String) -> float:
	var env = ENV_BONUSES.get(battle_environment, {})
	var mult = 1.0
	# Physical boost
	if attack_element == "physical" and env.has("phys_boost"):
		mult += env["phys_boost"]
	# Void boost (the_seam, bl07_void)
	if attack_element == "void" and env.has("void_boost"):
		mult += env["void_boost"]
	# Damage reduction (colorless_waste)
	if env.has("dmg_reduce"):
		mult -= env["dmg_reduce"]
	return mult

## 환경 회피 확률 (플레이어 피격 시 미스)
func _check_env_evasion() -> bool:
	var env = ENV_BONUSES.get(battle_environment, {})
	var evasion = env.get("evasion", 0.0)
	if evasion > 0.0 and randf() < evasion:
		return true
	return false

## 환경 적 미스 확률 (drift_shelter 비)
func _check_env_enemy_miss() -> bool:
	var env = ENV_BONUSES.get(battle_environment, {})
	var miss_chance = env.get("enemy_miss", 0.0)
	if miss_chance > 0.0 and randf() < miss_chance:
		return true
	return false

## 환경 힐 감소 (bl07_void)
func get_env_heal_mult() -> float:
	var env = ENV_BONUSES.get(battle_environment, {})
	if env.has("heal_reduce"):
		return 1.0 - env["heal_reduce"]
	return 1.0

var tactical_objective: Dictionary = {}
var _objective_completed: bool = false
var _objective_failed: bool = false
var _memory_burns_this_battle: int = 0
var _max_combo_this_battle: int = 0
var _breaks_this_battle: int = 0
var _stance_switches_this_battle: int = 0
var _limit_breaks_this_battle: int = 0
var _items_used_this_battle: int = 0
var _echoes_activated_this_battle: int = 0
var _ally_actions_this_battle: int = 0
var _player_actions_this_battle: int = 0

# --- Combat Resonance / Momentum ---
const MOMENTUM_MAX: float = 100.0
const MOMENTUM_RANK_THRESHOLDS: Array[float] = [25.0, 50.0, 75.0, 100.0]
const MOMENTUM_RANK_LABELS: Array[String] = ["Cold", "Kindled", "Burning", "Resonant", "Overbright"]
const MOMENTUM_RANK_LABELS_KO: Array[String] = ["냉각", "점화", "연소", "공명", "과휘"]
var momentum: float = 0.0
var momentum_rank: int = 0
var _best_momentum_rank: int = 0
var field_focus_opening: bool = false
var _last_stand_triggered_this_battle: bool = false

# --- Limit Break 시스템 ---
var limit_gauge: float = 0.0        # 0.0 ~ 100.0
const LIMIT_MAX: float = 100.0
const LIMIT_GAIN_ATTACK: float = 8.0    # 공격 시
const LIMIT_GAIN_BURN: float = 12.0     # 연소 시
const LIMIT_GAIN_HIT: float = 15.0      # 피격 시
const LIMIT_GAIN_DEFEND: float = 5.0    # 방어 시
const BREAK_MAX: float = 100.0
const BREAK_WEAKNESS_GAIN: float = 42.0
const BREAK_NEUTRAL_GAIN: float = 10.0
const BREAK_BOSS_MULT: float = 0.72
const BREAK_DAMAGE_BONUS: float = 1.35
signal limit_changed(value: float)

## Limit 게이지 증가 헬퍼
func _add_limit(amount: float) -> void:
	# Burn Passive: Memory Cascade (+20% limit gain) — applied globally
	if MemoryManager.has_passive("memory_cascade"):
		amount *= 1.2
	limit_gauge = minf(limit_gauge + amount, LIMIT_MAX)
	limit_changed.emit(limit_gauge)

func _reset_momentum() -> void:
	momentum = 0.0
	momentum_rank = 0
	_best_momentum_rank = 0
	momentum_changed.emit(momentum, momentum_rank, _get_momentum_label())

func _get_momentum_rank(value: float) -> int:
	var rank := 0
	for threshold in MOMENTUM_RANK_THRESHOLDS:
		if value >= threshold:
			rank += 1
	return clampi(rank, 0, MOMENTUM_RANK_LABELS.size() - 1)

func _get_momentum_label(rank_override: int = -1) -> String:
	var rank := momentum_rank if rank_override < 0 else rank_override
	rank = clampi(rank, 0, MOMENTUM_RANK_LABELS.size() - 1)
	if GameManager.current_locale == "ko":
		return MOMENTUM_RANK_LABELS_KO[rank]
	return MOMENTUM_RANK_LABELS[rank]

func _get_momentum_damage_mult() -> float:
	match momentum_rank:
		1:
			return 1.04
		2:
			return 1.08
		3:
			return 1.13
		4:
			return 1.20
	return 1.0

func _apply_momentum_damage_bonus(damage: int) -> int:
	var mult := _get_momentum_damage_mult()
	if mult <= 1.0:
		return damage
	return maxi(1, int(damage * mult))

func _add_momentum(amount: float, reason: String = "") -> void:
	if amount <= 0.0:
		return
	var old_rank := momentum_rank
	momentum = clampf(momentum + amount, 0.0, MOMENTUM_MAX)
	momentum_rank = _get_momentum_rank(momentum)
	_best_momentum_rank = maxi(_best_momentum_rank, momentum_rank)
	momentum_changed.emit(momentum, momentum_rank, _get_momentum_label())
	if reason != "":
		battle_log.emit("[RESONANCE] %s +%d" % [reason, int(amount)])
	if momentum_rank > old_rank:
		battle_log.emit("[RESONANCE] %s state reached. Damage momentum rises." % _get_momentum_label())
		GameManager.max_stat("highest_momentum_rank", momentum_rank)
		if momentum_rank >= 3:
			GameManager.add_stat("momentum_surges")
			if AchievementManager:
				AchievementManager.unlock("resonance_master")
			TutorialHints.show_hint("first_resonance")
	_check_tactical_objective("momentum")

func _get_momentum_grains_bonus() -> int:
	if _best_momentum_rank <= 0:
		return 0
	var bonus := _best_momentum_rank * 2
	if _best_momentum_rank >= 4:
		bonus += 4
	return bonus

# --- 시그널 ---
signal battle_started(enemy: Enemy)
signal player_turn_started()
signal enemy_turn_started()
signal damage_dealt(target: String, amount: int, skill_name: String)
signal pre_attack(attacker: String, target: String, skill_name: String)  # S58: anticipation signal
signal battle_ended(result: BattleState)
signal battle_cleanup_finished(result: BattleState)
signal battle_log(message: String)
signal status_changed()
signal guard_focus(trigger: String, value: int)
signal last_stand_resonance(lethal: bool)
signal victory_rewards_ready(rewards: Dictionary)  # S58: structured reward data
var _victory_dismissed: bool = false  # S58: wait for player to dismiss rewards
var _battle_started_as_boss_rush: bool = false

## 난이도별 적 스케일링 (Easy=0.7, Normal=1.0, Hard=1.4)
func _get_difficulty_scale() -> float:
	var diff = OptionsMenu.settings.get("difficulty", 1)
	match diff:
		0: return 0.7
		2: return 1.4
	return 1.0

func _ready() -> void:
	print("[BattleManager] Ready")

## S58: Called by battle_scene when player dismisses the rewards screen
func dismiss_victory() -> void:
	_victory_dismissed = true

## 전투 시작
func start_battle(enemy_ref: Variant, from_scene: String = "", bg_image: String = "", e_image: String = "") -> void:
	var enemy: Enemy = _coerce_enemy(enemy_ref)
	if enemy == null:
		push_error("[BattleManager] Invalid battle enemy: %s" % str(enemy_ref))
		return
	current_enemy = enemy
	_battle_started_as_boss_rush = GameManager.boss_rush_mode
	return_scene = from_scene
	var preset_art := _get_enemy_preset_art(enemy_ref)
	battle_bg_image = bg_image if bg_image != "" else String(preset_art.get("bg", ""))
	var requested_enemy_image := e_image if e_image != "" else String(preset_art.get("img", ""))
	enemy_image = requested_enemy_image if requested_enemy_image != "" and ResourceLoader.exists(requested_enemy_image) else resolve_enemy_image_by_name(enemy.name)
	player_defending = false
	enemy_shielded = false
	player_statuses.clear()
	enemy_statuses.clear()
	enemy_break_gauge = 0.0
	enemy_broken_turns = 0
	combo_count = 0
	_last_action = ""
	_burn_chain = 0  # S53
	_boss_turn_counter = 0
	_player_stunned = false
	_enemy_reflecting = false
	_enemy_charged = false
	active_echoes.clear()
	current_stance = Stance.REMNANT
	_encounter_modifier = {}
	_total_turns = 0
	scanned_enemies.clear()
	_memory_burns_this_battle = 0
	_max_combo_this_battle = 0
	_breaks_this_battle = 0
	_stance_switches_this_battle = 0
	_limit_breaks_this_battle = 0
	_items_used_this_battle = 0
	_echoes_activated_this_battle = 0
	_ally_actions_this_battle = 0
	_player_actions_this_battle = 0
	_last_stand_triggered_this_battle = false
	_objective_completed = false
	_objective_failed = false
	tactical_objective.clear()
	_reset_momentum()
	field_focus_opening = false
	# Detect battle environment from return scene
	battle_environment = _detect_environment(from_scene)
	sable_in_party = GameManager.get_flag("sable_joined") and GameManager.current_chapter >= 4
	tobias_in_party = GameManager.get_flag("tobias_joined") and GameManager.current_chapter >= 3 and GameManager.current_chapter < 7
	# S51: 엘리아 기술 쿨다운 리셋
	if GameManager.player_data.elia_with_party:
		EliaDiary.reset_cooldowns()
	limit_gauge = 0.0
	limit_changed.emit(0.0)
	if not _battle_started_as_boss_rush and GameManager.consume_field_focus():
		field_focus_opening = true
		_add_momentum(25.0)
		_add_limit(20.0)
	auto_battle = false
	auto_battle_changed.emit(false)
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

	# S59: Calculate difficulty bonus from chapter + NG+
	difficulty_bonus = 0.0
	if GameManager.current_chapter >= 7:
		difficulty_bonus += 0.15  # Ch7-10: +15% enemy damage
	if GameManager.ng_plus_cycle >= 1:
		difficulty_bonus += 0.20  # NG+: additional +20%

	GameManager.change_state(GameManager.GameState.BATTLE)

	# S58: 보스전 — 드라마틱 침묵 후 강렬한 BGM 진입
	if enemy.is_boss:
		AudioManager.dramatic_silence(1.0)

	battle_started.emit(enemy)
	battle_log.emit("A %s appears!" % enemy.name)
	battle_log.emit(_get_opening_tactical_hint(enemy))
	_setup_tactical_objective(enemy)
	# S55: Tutorial hint
	TutorialHints.show_hint("first_battle")

	# Battle Environment info
	if battle_environment != "":
		var env = ENV_BONUSES.get(battle_environment, {})
		battle_log.emit("[TERRAIN] %s — %s" % [env.get("name", ""), env.get("desc", "")])
		environment_info.emit(env.get("name", ""), env.get("desc", ""))

	if enemy.is_void_beast:
		battle_log.emit("It's a Void Beast — normal attacks are weakened.")

	_apply_opening_choice_battle_trait()

	# S51: 보이드 부패 수정자 적용
	_encounter_modifier = EncounterModifier.apply(enemy)
	if not _encounter_modifier.is_empty():
		battle_log.emit("[VOID CORRUPTION] %s" % _encounter_modifier.get("name", ""))
		battle_log.emit(_encounter_modifier.get("desc", ""))

	# S53: NG++ (cycle 3+) — 보스 변형
	if GameManager.ng_plus_cycle >= 3 and enemy.is_boss:
		enemy.abilities.append("despair")
		enemy.abilities.append("charge")
		if not enemy.abilities.has("reflect"):
			enemy.abilities.append("reflect")
		battle_log.emit("[NG+++] %s radiates with accumulated void energy!" % enemy.name)

	player_turn_started.emit()

func _coerce_enemy(enemy_ref: Variant) -> Enemy:
	if enemy_ref is Enemy:
		return enemy_ref
	if enemy_ref is Dictionary:
		return _enemy_from_dictionary(enemy_ref)
	if enemy_ref is String:
		var key := _normalize_enemy_key(String(enemy_ref))
		if ENEMY_PRESETS.has(key):
			return _enemy_from_dictionary(ENEMY_PRESETS[key])
		return Enemy.new(String(enemy_ref), 70, 14, String(enemy_ref).to_lower().contains("void"))
	return null

func _enemy_from_dictionary(data: Dictionary) -> Enemy:
	var enemy := Enemy.new(
		String(data.get("name", "Unknown Enemy")),
		int(data.get("hp", 70)),
		int(data.get("atk", data.get("attack", 14))),
		bool(data.get("is_void", data.get("is_void_beast", false)))
	)
	enemy.is_boss = bool(data.get("is_boss", false))
	enemy.abilities = data.get("abilities", []).duplicate(true)
	if data.has("weakness"):
		enemy.weakness = String(data.weakness)
	if data.has("resistance"):
		enemy.resistance = String(data.resistance)
	return enemy

func _normalize_enemy_key(enemy_id: String) -> String:
	return enemy_id.strip_edges().to_lower().replace(" ", "_").replace("-", "_")

func _get_enemy_preset_art(enemy_ref: Variant) -> Dictionary:
	if enemy_ref is Dictionary:
		return {"bg": String(enemy_ref.get("bg", "")), "img": String(enemy_ref.get("img", ""))}
	if enemy_ref is String:
		var key := _normalize_enemy_key(String(enemy_ref))
		if ENEMY_PRESETS.has(key):
			var data: Dictionary = ENEMY_PRESETS[key]
			return {"bg": String(data.get("bg", "")), "img": String(data.get("img", ""))}
	return {"bg": "", "img": ""}

func resolve_enemy_image_by_name(enemy_name: String) -> String:
	var lower_name: String = enemy_name.to_lower()
	if "kairos" in lower_name or "authority editor" in lower_name or "watcher" in lower_name:
		if ResourceLoader.exists(ART_KAIROS_FULLBODY):
			return ART_KAIROS_FULLBODY
		return ART_KAIROS_FALLBACK if ResourceLoader.exists(ART_KAIROS_FALLBACK) else ""
	if "nera" in lower_name:
		return ART_NERA_FULLBODY if ResourceLoader.exists(ART_NERA_FULLBODY) else ""
	if "tobias" in lower_name:
		return ART_TOBIAS_FULLBODY if ResourceLoader.exists(ART_TOBIAS_FULLBODY) else ""
	if "veil" in lower_name:
		return ART_VEIL_FULLBODY if ResourceLoader.exists(ART_VEIL_FULLBODY) else ""
	if "shade sentinel" in lower_name:
		return ART_SHADE_SENTINEL if ResourceLoader.exists(ART_SHADE_SENTINEL) else (ART_VOID_BEAST_FALLBACK if ResourceLoader.exists(ART_VOID_BEAST_FALLBACK) else "")
	if "void beast" in lower_name:
		return ART_VOID_BEAST if ResourceLoader.exists(ART_VOID_BEAST) else (ART_VOID_BEAST_FALLBACK if ResourceLoader.exists(ART_VOID_BEAST_FALLBACK) else "")
	# 일반 몬스터는 콘셉트 시트 전체를 화면에 띄우지 않고
	# PixelSprite의 이름별 128px 캐릭터 렌더러로 보낸다.
	return ""

func _get_opening_tactical_hint(enemy: Enemy) -> String:
	var focus: String = "Watch its rhythm, then force a BREAK."
	if enemy.weakness != "":
		focus = "Exploit %s to build BREAK pressure." % enemy.weakness.to_upper()
	elif enemy.is_void_beast:
		focus = "Void skills bite deeper than normal steel."
	var guard: String = ""
	if enemy.resistance != "":
		guard = " Avoid %s." % enemy.resistance.to_upper()
	elif enemy.is_boss:
		guard = " Guard during telegraphed turns."
	return "[TACTIC] %s%s" % [focus, guard]

func _setup_tactical_objective(enemy: Enemy) -> void:
	var pool: Array[Dictionary] = []
	if enemy.is_boss or enemy.max_hp >= 160:
		pool.append({
			"id": "force_break",
			"title": "Pressure Point",
			"desc": "Trigger BREAK before victory.",
			"reward_grains": 6,
			"reward_item": "firebomb",
			"reward_heal": 0,
		})
		pool.append({
			"id": "limit_release",
			"title": "Cascade Window",
			"desc": "Use Memory Cascade before victory.",
			"reward_grains": 8,
			"reward_item": "hi_potion",
			"reward_heal": 0,
		})
	if not enemy.is_boss:
		pool.append({
			"id": "keep_memory",
			"title": "Clean Hands",
			"desc": "Win without burning a memory.",
			"reward_grains": 5,
			"reward_item": "potion",
			"reward_heal": 0,
		})
		pool.append({
			"id": "swift_finish",
			"title": "Before It Learns",
			"desc": "Win within 4 player actions.",
			"reward_grains": 7,
			"reward_item": "firebomb",
			"reward_heal": 0,
		})
	pool.append({
		"id": "scan_first",
		"title": "Archivist's Eye",
		"desc": "Scan or analyze the enemy before victory.",
		"reward_grains": 4,
		"reward_item": "antidote",
		"reward_heal": 0,
	})
	pool.append({
		"id": "combo_three",
		"title": "Measured Assault",
		"desc": "Reach Combo x3 before victory.",
		"reward_grains": 5,
		"reward_item": "",
		"reward_heal": 5,
	})
	pool.append({
		"id": "kindle_momentum",
		"title": "Resonance Climb",
		"desc": "Reach Burning resonance before victory.",
		"reward_grains": 7,
		"reward_item": "",
		"reward_heal": 12,
	})
	pool.append({
		"id": "no_items",
		"title": "Bare Hands",
		"desc": "Win without using battle items.",
		"reward_grains": 4,
		"reward_item": "potion",
		"reward_heal": 0,
	})
	if GameManager.current_chapter >= 4:
		pool.append({
			"id": "stance_shift",
			"title": "Three Forms",
			"desc": "Switch stance twice before victory.",
			"reward_grains": 6,
			"reward_item": "",
			"reward_heal": 8,
		})
	if MemoryManager.get_available_memories().size() >= 2:
		pool.append({
			"id": "echo_weave",
			"title": "Echo Weave",
			"desc": "Activate 2 Memory Echoes before victory.",
			"reward_grains": 8,
			"reward_item": "hi_potion",
			"reward_heal": 0,
		})
	if sable_in_party or tobias_in_party:
		pool.append({
			"id": "ally_coordination",
			"title": "Shared Rhythm",
			"desc": "Trigger 2 companion support actions.",
			"reward_grains": 6,
			"reward_item": "antidote",
			"reward_heal": 10,
		})

	var seed_text := "%s:%s:%d" % [enemy.name, return_scene, GameManager.play_stats.get("total_battles", 0)]
	var index: int = int(abs(hash(seed_text)) % pool.size())
	tactical_objective = pool[index].duplicate(true)
	tactical_objective["status"] = "active"
	tactical_objective["complete"] = false
	tactical_objective["failed"] = false
	battle_log.emit("[OBJECTIVE] %s - %s" % [tactical_objective.title, tactical_objective.desc])
	tactical_objective_changed.emit(tactical_objective.duplicate(true))

func _complete_tactical_objective(reason: String = "") -> void:
	if tactical_objective.is_empty() or _objective_completed or _objective_failed:
		return
	_objective_completed = true
	tactical_objective["status"] = "complete"
	tactical_objective["complete"] = true
	if reason == "":
		reason = tactical_objective.get("title", "Objective")
	battle_log.emit("[OBJECTIVE COMPLETE] %s" % reason)
	tactical_objective_changed.emit(tactical_objective.duplicate(true))

func _fail_tactical_objective(reason: String = "") -> void:
	if tactical_objective.is_empty() or _objective_completed or _objective_failed:
		return
	_objective_failed = true
	tactical_objective["status"] = "failed"
	tactical_objective["failed"] = true
	if reason != "":
		battle_log.emit("[OBJECTIVE LOST] %s" % reason)
	tactical_objective_changed.emit(tactical_objective.duplicate(true))

func _check_tactical_objective(event_id: String = "") -> void:
	if tactical_objective.is_empty() or _objective_completed or _objective_failed:
		return
	var objective_id: String = tactical_objective.get("id", "")
	match objective_id:
		"force_break":
			if _breaks_this_battle > 0:
				_complete_tactical_objective("BREAK achieved.")
		"scan_first":
			if current_enemy and scanned_enemies.has(current_enemy.name):
				_complete_tactical_objective("Enemy recorded.")
		"combo_three":
			if _max_combo_this_battle >= 3:
				_complete_tactical_objective("Combo x3 reached.")
		"keep_memory":
			if event_id == "memory_burned":
				_fail_tactical_objective("A memory was burned.")
		"stance_shift":
			if _stance_switches_this_battle >= 2:
				_complete_tactical_objective("Two stance shifts completed.")
		"kindle_momentum":
			if _best_momentum_rank >= 3:
				_complete_tactical_objective("Burning resonance reached.")
		"echo_weave":
			if _echoes_activated_this_battle >= 2:
				_complete_tactical_objective("Two echoes are active.")
		"no_items":
			if event_id == "item_used":
				_fail_tactical_objective("A battle item was used.")
		"swift_finish":
			if _player_actions_this_battle > 4:
				_fail_tactical_objective("The fight dragged on too long.")
		"limit_release":
			if _limit_breaks_this_battle > 0:
				_complete_tactical_objective("Memory Cascade released.")
		"ally_coordination":
			if _ally_actions_this_battle >= 2:
				_complete_tactical_objective("Companion rhythm established.")

func _finalize_tactical_objective() -> Dictionary:
	if tactical_objective.is_empty():
		return {"grains": 0, "item": "", "title": "", "heal": 0}
	if tactical_objective.get("id", "") == "keep_memory" and not _objective_failed and _memory_burns_this_battle == 0:
		_complete_tactical_objective("No memories burned.")
	if tactical_objective.get("id", "") == "no_items" and not _objective_failed and _items_used_this_battle == 0:
		_complete_tactical_objective("No items used.")
	if tactical_objective.get("id", "") == "swift_finish" and not _objective_failed and _player_actions_this_battle <= 4:
		_complete_tactical_objective("Finished before it adapted.")
	if tactical_objective.get("id", "") == "kindle_momentum" and not _objective_failed and _best_momentum_rank >= 3:
		_complete_tactical_objective("Burning resonance reached.")
	if not _objective_completed:
		return {"grains": 0, "item": "", "title": tactical_objective.get("title", ""), "heal": 0}

	var bonus_grains: int = int(tactical_objective.get("reward_grains", 0))
	var bonus_item: String = tactical_objective.get("reward_item", "")
	var bonus_heal: int = int(tactical_objective.get("reward_heal", 0))
	if bonus_item != "" and GameManager.ITEMS.has(bonus_item):
		GameManager.add_item(bonus_item, 1)
	GameManager.add_stat("objectives_completed")
	if AchievementManager:
		AchievementManager.unlock("perfect_tactics")
	return {
		"grains": bonus_grains,
		"item": bonus_item,
		"title": tactical_objective.get("title", ""),
		"heal": bonus_heal,
	}

func _apply_opening_choice_battle_trait() -> void:
	if current_enemy == null:
		return

	if not GameManager.get_flag("ch1_opening_trait_spent"):
		if GameManager.get_flag("burned_for_passage"):
			_add_limit(18.0)
			enemy_break_gauge = minf(enemy_break_gauge + 22.0, BREAK_MAX)
			break_changed.emit(enemy_break_gauge, BREAK_MAX)
			battle_log.emit("[CHOICE ECHO] The burned song scatters through the ash. Limit +18, BREAK pressure +22.")
			GameManager.set_flag("ch1_opening_trait_spent")
		elif GameManager.get_flag("refused_to_burn"):
			player_defending = true
			_add_limit(8.0)
			battle_log.emit("[CHOICE ECHO] Arrel keeps the song intact. The first enemy blow is guarded.")
			GameManager.set_flag("ch1_opening_trait_spent")

	if GameManager.get_flag("listened_to_humming") and not GameManager.get_flag("ch1_humming_focus_spent"):
		_add_limit(10.0)
		battle_log.emit("[ANCHOR] Elia's melody steadies Arrel. Limit +10.")
		GameManager.set_flag("ch1_humming_focus_spent")

## 플레이어 행동: 일반 공격
func player_attack() -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	_player_actions_this_battle += 1
	_check_tactical_objective("action")
	_burn_chain = 0  # S53: 체인 리셋
	# 콤보 빌드
	if _last_action == "attack":
		combo_count += 1
	else:
		combo_count = 1
	_last_action = "attack"
	combo_changed.emit(combo_count)

	# S51: Memory Fog 수정자 — 미스 확률
	if has_modifier("player_miss") and randi_range(0, 99) < _encounter_modifier.get("value", 0):
		battle_log.emit("The fog of burned memories clouds your strike... MISS!")
		_end_player_turn()
		return

	var base_dmg = _get_player_attack() + randi_range(0, 10)
	# 스탠스 공격 보정
	base_dmg = int(base_dmg * get_stance_atk_mult())
	# 콤보 보너스 (2연속: +15%, 3연속: +30%, 4+: +50%)
	var combo_mult = _get_combo_multiplier()
	# Hollow 스탠스: 콤보 배율 2배
	if current_stance == Stance.HOLLOW:
		combo_mult = 1.0 + (combo_mult - 1.0) * 2.0
	base_dmg = int(base_dmg * combo_mult)
	# 약화 적용
	base_dmg = int(base_dmg * _get_weaken_multiplier("player"))
	# Total Erasure 에코: 2배 데미지
	if has_echo("total_erasure"):
		base_dmg *= 2
		consume_echo_charge("total_erasure")
		battle_log.emit("[ECHO] Total Erasure surges through the blade!")

	# 속성 상성 (물리 — Identity Fracture 에코 시 void로 변환)
	var atk_element = "void" if has_echo("identity_fracture") else "physical"
	var elem_mult = _get_element_multiplier(atk_element)
	base_dmg = int(base_dmg * elem_mult)

	if current_enemy.is_void_beast:
		base_dmg = maxi(1, int(base_dmg * 0.3))
		battle_log.emit("Your blade struggles against the void...")

	if enemy_shielded:
		base_dmg = maxi(1, base_dmg / 2)
		enemy_shielded = false
		AudioManager.play_combat_sfx("shield_break")  # S58: 방패 파괴 레이어드 SFX
		battle_log.emit("The barrier absorbs some damage!")
	var hit_broken_enemy := enemy_broken_turns > 0
	base_dmg = _apply_break_damage_bonus(base_dmg)
	base_dmg = _apply_momentum_damage_bonus(base_dmg)
	# S58: Anticipation — signal before damage, await wind-up
	pre_attack.emit("Arrel", current_enemy.name, "Attack")
	await get_tree().create_timer(0.23).timeout  # anticipation + strike duration
	var actual = current_enemy.take_damage(base_dmg)
	AudioManager.play_combat_sfx("sword_slash")  # S58: 레이어드 전투 SFX
	InputManager.vibrate("battle_hit")
	var combo_text = " (Combo x%d!)" % combo_count if combo_count >= 2 else ""
	battle_log.emit("Arrel strikes! %d damage.%s" % [actual, combo_text])
	_log_element_effect(atk_element)
	_register_break_pressure(atk_element)
	if hit_broken_enemy:
		_add_momentum(6.0, "Punished BREAK")
	elif combo_count >= 2:
		_add_momentum(5.0, "Combo maintained")
	else:
		_add_momentum(3.0, "Clean strike")
	damage_dealt.emit(current_enemy.name, actual, "Attack")
	_add_limit(LIMIT_GAIN_ATTACK)
	_check_combo_milestone()
	# 반사 배리어 처리
	if _enemy_reflecting:
		_enemy_reflecting = false
		var reflect_dmg = maxi(1, int(actual * 0.3))
		GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - reflect_dmg)
		battle_log.emit("The mirror reflects %d damage back!" % reflect_dmg)
		damage_dealt.emit("Arrel", reflect_dmg, "Reflect")
	_check_enemy_defeated()

## 속성 상성 배율 계산
func _get_element_multiplier(attack_element: String) -> float:
	if current_enemy == null or attack_element == "":
		return 1.0
	var mult = 1.0
	if current_enemy.weakness == attack_element:
		mult = ELEMENT_BONUS
	elif current_enemy.resistance == attack_element:
		mult = ELEMENT_RESIST
	# Burn Passive: Void Touch (+15% void damage)
	if attack_element == "void" and MemoryManager.has_passive("void_touch"):
		mult *= 1.15
	# Battle Environment: element bonuses
	mult *= _get_env_element_mult(attack_element)
	return mult

## 속성 상성 로그 메시지
func _log_element_effect(attack_element: String) -> void:
	if current_enemy == null or attack_element == "":
		return
	if current_enemy.weakness == attack_element:
		battle_log.emit("It's super effective!")
	elif current_enemy.resistance == attack_element:
		battle_log.emit("It's not very effective...")

## 콤보 보너스 계수 (S46: 스케일링 강화 + 마일스톤 보상)
func _apply_break_damage_bonus(damage: int) -> int:
	if enemy_broken_turns <= 0:
		return damage
	return maxi(1, int(damage * BREAK_DAMAGE_BONUS))

func _register_break_pressure(attack_element: String) -> void:
	if current_enemy == null or attack_element == "" or enemy_broken_turns > 0:
		return
	var gain := BREAK_NEUTRAL_GAIN
	if current_enemy.weakness == attack_element:
		gain = BREAK_WEAKNESS_GAIN
	elif current_enemy.resistance == attack_element:
		gain = 0.0
	if current_enemy.is_boss:
		gain *= BREAK_BOSS_MULT
	if gain <= 0.0:
		return

	enemy_break_gauge = clampf(enemy_break_gauge + gain, 0.0, BREAK_MAX)
	break_changed.emit(enemy_break_gauge, BREAK_MAX)
	if current_enemy.weakness == attack_element:
		battle_log.emit("[BREAK] Weakness pressure +%d" % int(gain))
		_add_momentum(8.0, "Weakness pressure")
	if enemy_break_gauge >= BREAK_MAX:
		enemy_break_gauge = 0.0
		enemy_broken_turns = 1
		_breaks_this_battle += 1
		battle_log.emit("[BREAK] %s is staggered!" % current_enemy.name)
		_add_momentum(18.0, "BREAK triggered")
		TutorialHints.show_hint("first_break")
		break_changed.emit(enemy_break_gauge, BREAK_MAX)
		enemy_broken.emit(current_enemy.name)
		_check_tactical_objective("break")

func _get_combo_multiplier() -> float:
	var base: float
	match combo_count:
		2: base = 1.15
		3: base = 1.30
		4: base = 1.50
		5: base = 1.70
		_:
			base = 2.0 if combo_count >= 6 else 1.0
	# S51: Lingering Habit 에코 — 콤보 배율 +20%
	if has_echo("lingering_habit") and base > 1.0:
		base += 0.20
	return base

## S46: 콤보 마일스톤 보상 (3, 5, 7+ 콤보에서 리밋 보너스)
func _check_combo_milestone() -> void:
	_max_combo_this_battle = maxi(_max_combo_this_battle, combo_count)
	_check_tactical_objective("combo")
	if combo_count == 3:
		_add_limit(5.0)
		battle_log.emit("Combo x3! Limit gauge rising!")
	elif combo_count == 5:
		_add_limit(10.0)
		battle_log.emit("Combo x5! Momentum surges!")
	elif combo_count == 7:
		_add_limit(15.0)
		battle_log.emit("COMBO x7! Unstoppable!")

## 콤보 리셋 (비공격 행동 시)
func _reset_combo(action: String) -> void:
	_last_action = action
	combo_count = 0
	combo_changed.emit(combo_count)

## 챕터에 따른 플레이어 기본 공격력
func _get_player_attack() -> int:
	var base = 15
	var chapter_bonus = (GameManager.current_chapter - 1) * 3
	var equip_bonus = GameManager.get_equip_bonus("atk")  # S41: 장비 보너스
	return base + chapter_bonus + equip_bonus

## S41: 장비 방어력 적용 (적 공격 시 피해 감소)
func _get_player_defense() -> int:
	return GameManager.get_equip_bonus("def")

## 플레이어 행동: 기억 연소 스킬
func player_burn(memory_id: String) -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return

	# Faded 기억은 전투 연소 불가
	var pre_check = MemoryManager._get_memory(memory_id)
	if pre_check and pre_check.is_faded:
		battle_log.emit("That memory has faded beyond use...")
		return

	var memory = MemoryManager.burn_memory(memory_id)
	if memory == null:
		battle_log.emit("That memory is already gone.")
		return

	_player_actions_this_battle += 1
	_check_tactical_objective("action")
	_reset_combo("burn")
	_memory_burns_this_battle += 1
	_check_tactical_objective("memory_burned")
	# S55: Tutorial hint
	TutorialHints.show_hint("first_burn")
	GameManager.add_stat("total_burns")
	# S53: 연속 연소 체인
	_burn_chain += 1
	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	AudioManager.play_combat_sfx("burn_ignite")  # S58: 레이어드 연소 SFX
	InputManager.vibrate("memory_burn")
	# 침식 반영 — 유효 연소력
	var effective_power = MemoryManager.get_effective_burn_power(memory)
	var dmg = skill.base_damage + effective_power
	# S41: 장비 효과 — 연소 부스트
	if GameManager.has_equip_effect("burn_boost"):
		dmg = int(dmg * 1.2)
	# Burn Passive: Ember Affinity (+10% burn damage)
	if MemoryManager.has_passive("ember_affinity"):
		dmg = int(dmg * 1.1)
	# S53: 체인 보너스 (+20% per consecutive burn)
	if _burn_chain >= 2:
		var chain_bonus = 1.0 + (_burn_chain - 1) * 0.2
		dmg = int(dmg * chain_bonus)
		battle_log.emit("[CHAIN x%d] Memory resonance amplifies the flames!" % _burn_chain)
	# 속성 상성 (연소 속성)
	var burn_element = skill.get("element", "fire")
	var elem_mult = _get_element_multiplier(burn_element)
	dmg = int(dmg * elem_mult)
	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.7))
		enemy_shielded = false
		battle_log.emit("The barrier weakens the flames!")
	dmg = _apply_break_damage_bonus(dmg)
	dmg = _apply_momentum_damage_bonus(dmg)
	# S58: Anticipation — signal before burn damage
	pre_attack.emit("Arrel", current_enemy.name, skill.name)
	await get_tree().create_timer(0.23).timeout
	var actual = current_enemy.take_damage(dmg)

	battle_log.emit("[BURN] %s — %s" % [skill.name, skill.desc])
	battle_log.emit("%d damage to %s!" % [actual, current_enemy.name])
	_log_element_effect(burn_element)
	_register_break_pressure(burn_element)
	var burn_momentum := 10.0 + float(memory.grade) * 3.0
	if _burn_chain >= 2:
		burn_momentum += 6.0
	_add_momentum(burn_momentum, "Memory burn")
	damage_dealt.emit(current_enemy.name, actual, skill.name)

	# 고등급 기억 연소 시 적에게 화상 DoT 부여 (Grade 2=Identity 이상)
	if memory.grade >= MemoryManager.MemoryGrade.GRADE_2:  # Grade 2(=3), Grade 1(=4)
		var burn_dot = int(memory.burn_power * 0.3) + 5
		apply_status("enemy", StatusEffect.BURN, 2, burn_dot)

	# Burn Passive: Residual Warmth (+5 HP heal after burn)
	if MemoryManager.has_passive("residual_warmth"):
		GameManager.player_data.hp = mini(GameManager.player_data.hp + 5, GameManager.player_data.max_hp)
		battle_log.emit("[PASSIVE] Residual Warmth restores 5 HP.")
		damage_dealt.emit("Arrel", -5, "Residual Warmth")

	_add_limit(LIMIT_GAIN_BURN)
	# Memory Echo — 연소 후 전장 잔류 효과
	_apply_memory_echo(memory)
	_check_enemy_defeated()

## 플레이어 행동: 엘리아 기술 (S51: EliaDiary 연동)
func player_use_elia_skill(skill_id: String) -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return
	if not GameManager.player_data.elia_with_party:
		battle_log.emit("Elia is not with you.")
		return
	var result = EliaDiary.use_skill(skill_id)
	if not result["success"]:
		battle_log.emit(result["msg"])
		return

	_player_actions_this_battle += 1
	_check_tactical_objective("action")
	AudioManager.play_combat_sfx("heal_layered")  # S58: 레이어드 힐 SFX
	battle_log.emit("[ELIA] %s" % result["name"])
	battle_log.emit(result["msg"])
	ally_action.emit("Elia", skill_id, int(result["power"]))

	match result["effect"]:
		"defend":
			# 다음 적 턴 데미지 50% 감소
			player_defending = true
		"stun_enemy":
			_player_stunned = false  # 적 기절 (다음 적 턴 스킵)
			# enemy stun: 적에게 stun 상태 부여 (1턴)
			apply_status("enemy", StatusEffect.WEAKEN, 1, 0)
			battle_log.emit("%s is stunned!" % current_enemy.name)
		"damage":
			var dmg = result["power"]
			var elem_mult = _get_element_multiplier("void")
			dmg = int(dmg * elem_mult)
			if enemy_shielded:
				dmg = maxi(1, int(dmg * 0.7))
				enemy_shielded = false
			dmg = _apply_break_damage_bonus(dmg)
			dmg = _apply_momentum_damage_bonus(dmg)
			var actual = current_enemy.take_damage(dmg)
			_register_break_pressure("void")
			damage_dealt.emit(current_enemy.name, actual, result["name"])
		"heal_cure":
			var heal = result["power"]
			GameManager.player_data.hp = mini(GameManager.player_data.hp + heal, GameManager.player_data.max_hp)
			# 상태이상 해제
			player_statuses.clear()
			status_changed.emit()
			battle_log.emit("Healed %d HP and cured all ailments." % heal)

	_add_limit(5.0)
	_add_momentum(7.0, "Anchor technique")
	_check_enemy_defeated()

func _soften_player_statuses() -> bool:
	if player_statuses.is_empty():
		return false

	for entry in player_statuses:
		entry.turns_left = maxi(entry.turns_left - 1, 0)

	var expired: Array = []
	for entry in player_statuses:
		if entry.turns_left <= 0:
			expired.append(entry)
	for entry in expired:
		player_statuses.erase(entry)

	status_changed.emit()
	return true

## 플레이어 행동: 방어
func player_defend() -> void:
	if state != BattleState.PLAYER_TURN:
		return

	_player_actions_this_battle += 1
	_check_tactical_objective("action")
	_burn_chain = 0  # S53: 체인 리셋

	player_defending = true
	_reset_combo("defend")
	var focus_gain: float = LIMIT_GAIN_DEFEND + 7.0
	var momentum_gain: float = 6.0
	if _soften_player_statuses():
		focus_gain += 4.0
		momentum_gain += 4.0
		guard_focus.emit("status", 1)
		battle_log.emit("Guard Focus steadies Arrel. Status pressure weakens.")
	elif GameManager.player_data.hp < GameManager.player_data.max_hp:
		var heal_amount: int = maxi(3, int(GameManager.player_data.max_hp * 0.05))
		var actual_heal: int = mini(heal_amount, GameManager.player_data.max_hp - GameManager.player_data.hp)
		GameManager.player_data.hp += actual_heal
		momentum_gain += 2.0
		damage_dealt.emit("Arrel", -actual_heal, "Guard Focus")
		guard_focus.emit("heal", actual_heal)
		battle_log.emit("Guard Focus restores %d HP." % actual_heal)
	else:
		focus_gain += 4.0
		momentum_gain += 2.0
		guard_focus.emit("limit", int(focus_gain))
		battle_log.emit("Guard Focus primes the Limit gauge.")
	_add_limit(focus_gain)
	_add_momentum(momentum_gain, "Guard Focus")
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

	_player_actions_this_battle += 1
	_items_used_this_battle += 1
	_check_tactical_objective("action")
	_check_tactical_objective("item_used")
	AudioManager.play_sfx("ui_select")
	AchievementManager.record_item_used()
	GameManager.add_stat("items_used")
	_reset_combo("item")

	match item_def["type"]:
		"heal":
			var heal_amount = item_def["power"]
			# Environment: item effectiveness bonus (belt_waystation)
			var env = ENV_BONUSES.get(battle_environment, {})
			if env.has("item_boost"):
				heal_amount = int(heal_amount * (1.0 + env["item_boost"]))
			# Environment: heal reduction (bl07_void)
			heal_amount = int(heal_amount * get_env_heal_mult())
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

	if enemy_broken_turns > 0:
		enemy_broken_turns -= 1
		battle_log.emit("%s is broken and loses the turn!" % current_enemy.name)
		break_changed.emit(enemy_break_gauge, BREAK_MAX)
		await get_tree().create_timer(0.35).timeout
		if state == BattleState.ENEMY_TURN:
			state = BattleState.PLAYER_TURN
			player_turn_started.emit()
		return

	# 특수 능력 선택 (보스 페이즈 2 또는 확률적)
	var used_ability = await _try_enemy_ability()
	if used_ability:
		_check_player_defeated()
		return

	# Environment evasion/miss check
	if _check_env_evasion():
		battle_log.emit("The forest's cover grants evasion — DODGE!")
		_check_player_defeated()
		return
	if _check_env_enemy_miss():
		battle_log.emit("Rain obscures the enemy's aim — MISS!")
		_check_player_defeated()
		return

	var base_dmg = current_enemy.attack + randi_range(0, 5)
	# S59: Difficulty scaling — chapters 7+ and NG+ bonus
	if difficulty_bonus > 0.0:
		base_dmg = int(base_dmg * (1.0 + difficulty_bonus))
	# 차지 공격: 이전 턴에 차지했으면 2배 데미지
	if _enemy_charged:
		_enemy_charged = false
		base_dmg = int(base_dmg * 2.0)
		battle_log.emit("%s unleashes charged energy!" % current_enemy.name)
	# 적 약화 적용
	base_dmg = int(base_dmg * _get_weaken_multiplier("enemy"))
	# S41: 장비 방어력 적용
	var def = _get_player_defense()
	if def > 0:
		base_dmg = maxi(1, base_dmg - def)
	# 보이드 내성 (액세서리 효과)
	if current_enemy.is_void_beast and GameManager.has_equip_effect("void_resist"):
		base_dmg = maxi(1, int(base_dmg * 0.75))
	# Environment: enemy damage modifiers (void boost for void beasts, colorless reduction)
	var enemy_env = ENV_BONUSES.get(battle_environment, {})
	if current_enemy.is_void_beast and enemy_env.has("void_boost"):
		base_dmg = int(base_dmg * (1.0 + enemy_env["void_boost"]))
	if enemy_env.has("dmg_reduce"):
		base_dmg = maxi(1, int(base_dmg * (1.0 - enemy_env["dmg_reduce"])))
	# 스탠스 방어 보정
	base_dmg = int(base_dmg / get_stance_def_mult())
	if player_defending:
		base_dmg = maxi(1, base_dmg / 2)
		battle_log.emit("Defended! Reduced damage.")
	# Elia Anchor 에코: 25% 확률로 절반 데미지
	if has_echo("elia_anchor") and randf() < 0.25:
		base_dmg = maxi(1, base_dmg / 2)
		battle_log.emit("[ECHO] Elia's Anchor softens the blow!")

	player_defending = false

	# S58: Enemy anticipation — signal before enemy damage
	pre_attack.emit(current_enemy.name, "Arrel", "")
	await get_tree().create_timer(0.2).timeout
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
	# S59: Difficulty scaling applied to ability damage
	if difficulty_bonus > 0.0:
		rage_bonus *= (1.0 + difficulty_bonus)

	# S59: Telegraph — warn player before special ability (0.5s delay)
	enemy_ability_telegraph.emit(ability, 0.5)
	await get_tree().create_timer(0.5).timeout

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
		# S41: 새로운 보스 전용 능력
		"void_pulse":
			# 보이드 펄스: 전체 데미지 + 콤보 초기화
			var dmg = int((current_enemy.attack * 0.8 + 10) * rage_bonus)
			if player_defending:
				dmg = maxi(1, dmg / 2)
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
			combo_count = 0
			combo_changed.emit(0)
			battle_log.emit("Reality distorts around %s! %d damage." % [current_enemy.name, dmg])
			battle_log.emit("Your momentum shatters... combo broken!")
			damage_dealt.emit("Arrel", dmg, "Void Pulse")
			_add_limit(LIMIT_GAIN_HIT)
		"despair":
			# 절망: 독 + 약화 동시 부여
			apply_status("player", StatusEffect.POISON, 3, int(current_enemy.attack * 0.2) + 3)
			apply_status("player", StatusEffect.WEAKEN, 2, 25)
			AudioManager.play_sfx("drain")
			battle_log.emit("%s floods your mind with despair!" % current_enemy.name)
			battle_log.emit("Poison and weakness seize your body!")
		"stun":
			# 기절: 플레이어 다음 턴 스킵
			var dmg = int((current_enemy.attack * 0.4 + randi_range(0, 5)) * rage_bonus)
			if player_defending:
				dmg = maxi(1, dmg / 2)
			player_defending = false
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - dmg)
			_player_stunned = true
			AudioManager.play_combat_sfx("sword_slash")  # S58: 레이어드 전투 SFX
			InputManager.vibrate("battle_hit")
			battle_log.emit("%s delivers a stunning blow! %d damage." % [current_enemy.name, dmg])
			battle_log.emit("Arrel is stunned! Next turn will be lost.")
			damage_dealt.emit("Arrel", dmg, "Stun")
			_add_limit(LIMIT_GAIN_HIT)
		"reflect":
			# 반사 배리어: 다음 플레이어 공격의 30% 반사
			_enemy_reflecting = true
			enemy_shielded = true
			AudioManager.play_sfx("shield")
			battle_log.emit("%s conjures a mirror of void energy!" % current_enemy.name)
			battle_log.emit("Attacks will be partially reflected!")
		"charge":
			# 차지: 1턴 대기 후 다음 턴 강타 (현재 턴은 차지만)
			_enemy_charged = true
			AudioManager.play_sfx("shield")
			battle_log.emit("%s gathers dark energy..." % current_enemy.name)
			battle_log.emit("A devastating attack is coming!")
	return true

## 전술적 능력 선택 — S59: 가중치 기반 전술 AI
func _select_ability() -> String:
	var abilities = current_enemy.abilities
	if abilities.is_empty():
		return ""

	var enemy_hp_ratio = float(current_enemy.hp) / max(current_enemy.max_hp, 1)
	var player_hp_ratio = float(GameManager.player_data.hp) / max(GameManager.player_data.max_hp, 1)

	# Step 1: Filter out redundant/impossible abilities
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
			"reflect":
				if not _enemy_reflecting:
					filtered.append(a)
			"charge":
				if not _enemy_charged:
					filtered.append(a)
			"stun":
				if not _player_stunned:
					filtered.append(a)
			_:
				filtered.append(a)

	if filtered.is_empty():
		filtered = abilities  # 모두 중복이면 그냥 아무거나

	# Step 2: Weighted scoring for each ability
	var weights: Dictionary = {}  # ability_name -> float weight
	for a in filtered:
		weights[a] = 1.0  # base weight

	# Offensive abilities list
	var offensive = ["drain", "multi_hit", "burn_attack", "void_pulse", "stun", "despair"]
	# Defensive abilities list
	var defensive = ["shield", "reflect", "summon", "charge"]

	# S59: Player low HP — prefer offensive to finish them off (2x weight)
	if player_hp_ratio < 0.3:
		for a in filtered:
			if a in offensive:
				weights[a] *= 2.0

	# S59: Enemy low HP — prefer defensive/healing (2x weight)
	if enemy_hp_ratio < 0.4:
		for a in filtered:
			if a in defensive or a == "drain":
				weights[a] *= 2.0

	# S59: Player has high combo — prefer stun to interrupt (3x weight)
	if combo_count >= 3:
		if weights.has("stun"):
			weights["stun"] *= 3.0
		if weights.has("shield"):
			weights["shield"] *= 1.5

	# S59: Player already poisoned — avoid redundant poison (0.1x weight)
	if has_status("player", StatusEffect.POISON) and weights.has("poison"):
		weights["poison"] *= 0.1

	# Boss phase 2: summon preference
	if current_enemy.is_boss and current_enemy.phase == 2 and weights.has("summon"):
		weights["summon"] *= 1.5

	# Player defending: avoid multi_hit (less effective), prefer status
	if player_defending:
		if weights.has("multi_hit"):
			weights["multi_hit"] *= 0.5
		if weights.has("poison"):
			weights["poison"] *= 1.5
		if weights.has("weaken"):
			weights["weaken"] *= 1.5

	# Step 3: Weighted random selection
	var total_weight: float = 0.0
	for a in filtered:
		total_weight += weights.get(a, 1.0)

	var roll = randf() * total_weight
	var cumulative: float = 0.0
	for a in filtered:
		cumulative += weights.get(a, 1.0)
		if roll <= cumulative:
			return a

	return filtered[randi_range(0, filtered.size() - 1)]

## S59: Turn order hint — predict what enemy might do next turn
func get_next_turn_hint() -> String:
	if current_enemy == null or not current_enemy.is_alive():
		return ""
	if current_enemy.abilities.is_empty():
		return "The enemy readies a basic attack."

	var enemy_hp_ratio = float(current_enemy.hp) / max(current_enemy.max_hp, 1)
	var player_hp_ratio = float(GameManager.player_data.hp) / max(GameManager.player_data.max_hp, 1)

	# Predict based on AI scoring tendencies
	if enemy_hp_ratio < 0.3 and ("drain" in current_enemy.abilities or "summon" in current_enemy.abilities):
		return "The enemy looks desperate... it may try to heal."
	if player_hp_ratio < 0.3 and current_enemy.abilities.size() > 0:
		return "The enemy senses weakness — brace for a fierce attack!"
	if combo_count >= 3 and "stun" in current_enemy.abilities:
		return "Your combo draws attention — watch for a stunning blow!"
	if _enemy_charged:
		return "Charged energy surges — a devastating strike is imminent!"
	if current_enemy.is_boss and current_enemy.phase == 2:
		return "Dark fury builds... expect a powerful ability."
	# Generic hints
	var hints = [
		"The enemy shifts its stance...",
		"Something stirs in the darkness...",
		"The air grows tense...",
	]
	return hints[randi_range(0, hints.size() - 1)]

## 플레이어 사망 체크
func _check_player_defeated() -> void:
	if GameManager.player_data.hp <= 0:
		if _try_last_stand_resonance(true):
			return
		state = BattleState.DEFEAT
		AudioManager.play_sfx("defeat")
		InputManager.vibrate("game_over")
		battle_log.emit("Arrel falls...")
		battle_ended.emit(BattleState.DEFEAT)
		_cleanup()
		return

	# 플레이어 상태이상 처리 (독/화상 DoT)
	if not player_statuses.is_empty():
		_process_statuses("player")
		if GameManager.player_data.hp <= 0:
			if _try_last_stand_resonance(true):
				return
			state = BattleState.DEFEAT
			AudioManager.play_sfx("defeat")
			InputManager.vibrate("game_over")
			battle_log.emit("Arrel succumbs...")
			battle_ended.emit(BattleState.DEFEAT)
			_cleanup()
			return

	_try_last_stand_resonance(false)

	# 스턴 체크: 플레이어 턴 스킵
	if _player_stunned:
		_player_stunned = false
		battle_log.emit("Arrel shakes off the stun...")
		state = BattleState.PLAYER_TURN
		await get_tree().create_timer(0.6).timeout
		_end_player_turn()
		return

	# S58: Low HP 오디오 필터 업데이트 (HP < 25% → 로우패스 + 하트비트)
	var _hp_ratio = float(GameManager.player_data.hp) / float(maxi(1, GameManager.player_data.max_hp))
	AudioManager.update_low_hp_audio(_hp_ratio)
	AudioManager.update_battle_intensity(_hp_ratio)

	# 다시 플레이어 턴
	state = BattleState.PLAYER_TURN
	player_turn_started.emit()

func _try_last_stand_resonance(lethal: bool) -> bool:
	if _last_stand_triggered_this_battle:
		return false
	var max_hp: int = maxi(int(GameManager.player_data.get("max_hp", 100)), 1)
	var hp: int = int(GameManager.player_data.get("hp", 0))
	var low_hp := float(hp) / float(max_hp) <= 0.25
	if not lethal and not low_hp:
		return false

	_last_stand_triggered_this_battle = true
	if hp <= 0:
		GameManager.player_data.hp = 1
	player_defending = true
	_add_limit(22.0)
	_add_momentum(16.0, "Last Stand")
	last_stand_resonance.emit(lethal)
	battle_log.emit("[LAST STAND] Arrel refuses to vanish. HP holds at %d, next blow guarded." % int(GameManager.player_data.hp))
	if has_node("/root/NotificationToast"):
		NotificationToast.show_toast("Last Stand Resonance", NotificationToast.ToastType.WARNING)
	if AchievementManager:
		AchievementManager.unlock("survivor")
	if lethal:
		state = BattleState.PLAYER_TURN
		player_turn_started.emit()
	return true

func _check_enemy_defeated() -> void:
	if current_enemy and not current_enemy.is_alive():
		state = BattleState.VICTORY
		battle_log.emit("%s is defeated!" % current_enemy.name)
		battle_ended.emit(BattleState.VICTORY)
		_cleanup()
	else:
		# S46: 보스 페이즈 전환 — 드라마틱 연출
		if current_enemy and current_enemy.phase_changed:
			current_enemy.phase_changed = false
			AudioManager.play_sfx("phase_change")
			battle_log.emit("%s staggers... then surges with renewed fury!" % current_enemy.name)
			phase_changed.emit(current_enemy.name, 2)
		_end_player_turn()

func _end_player_turn() -> void:
	# S51: 턴 카운터 + 수정자 처리
	_total_turns += 1
	_process_modifier_effects()
	if state == BattleState.DEFEAT or state == BattleState.VICTORY or state == BattleState.FLED:
		return
	if GameManager.player_data.hp <= 0:
		_check_player_defeated()
		return
	if current_enemy and not current_enemy.is_alive():
		state = BattleState.VICTORY
		battle_log.emit("%s is defeated!" % current_enemy.name)
		battle_ended.emit(BattleState.VICTORY)
		_cleanup()
		return
	# Memory Echo 턴 처리 (힐 틱 등)
	_process_echoes_turn()
	# S46: 세이블 행동 — Sable Shadow 에코 시 100%, 아니면 40%/명령
	if sable_in_party and current_enemy and current_enemy.is_alive():
		if ally_command_pending and ally_command != "":
			_sable_support_action(ally_command)
			ally_command = ""
			ally_command_pending = false
		elif has_echo("sable_shadow"):
			_sable_support_action()
		elif randf() < 0.4:
			_sable_support_action()
		# 세이블이 적을 처치했는지 확인
		if current_enemy and not current_enemy.is_alive():
			state = BattleState.VICTORY
			battle_log.emit("%s is defeated!" % current_enemy.name)
			battle_ended.emit(BattleState.VICTORY)
			_cleanup()
			return

	# 토비아스 지원 행동 (Ch3~6)
	if tobias_in_party and current_enemy and current_enemy.is_alive():
		if tobias_command_pending and tobias_command != "":
			_tobias_support_action(tobias_command)
			tobias_command = ""
			tobias_command_pending = false
		elif randf() < 0.3:
			_tobias_support_action()

	# 적 상태이상 처리 (독/화상 DoT)
	if current_enemy and not enemy_statuses.is_empty():
		_process_statuses("enemy")
		if current_enemy and not current_enemy.is_alive():
			state = BattleState.VICTORY
			battle_log.emit("%s is defeated!" % current_enemy.name)
			battle_ended.emit(BattleState.VICTORY)
			_cleanup()
			return
	# S51: 엘리아 기술 쿨다운 틱
	if GameManager.player_data.elia_with_party:
		EliaDiary.tick_cooldowns()
	# 짧은 딜레이 후 적 턴 (UI 갱신 시간) — S55: 자동전투 시 0.5x 대기
	var wait_time = 0.4 if auto_battle else 0.8
	await get_tree().create_timer(wait_time).timeout
	_enemy_turn()

## S46: 플레이어 세이블 명령 설정
func set_ally_command(command: String) -> void:
	ally_command = command
	ally_command_pending = true

## 세이블 지원 행동 (S46: 명령 지정 가능)
func _sable_support_action(forced_action: String = "") -> void:
	_ally_actions_this_battle += 1
	_add_momentum(4.0, "Sable support")
	_check_tactical_objective("ally")
	var actions = ["heal", "strike", "weaken"]
	var action = forced_action if forced_action != "" else actions[randi_range(0, actions.size() - 1)]
	match action:
		"heal":
			var heal = randi_range(10, 20)
			GameManager.player_data.hp = mini(GameManager.player_data.hp + heal, GameManager.player_data.max_hp)
			battle_log.emit("Sable mends your wounds. +%d HP." % heal)
			ally_action.emit("Sable", "heal", heal)
			damage_dealt.emit("Arrel", -heal, "Sable Heal")
		"strike":
			var dmg = randi_range(12, 22)  # S53: 세이블 타격 데미지 증가
			if current_enemy:
				var actual = current_enemy.take_damage(dmg)
				battle_log.emit("Sable strikes from the shadows! %d damage." % actual)
				ally_action.emit("Sable", "strike", actual)
				damage_dealt.emit(current_enemy.name, actual, "Sable Strike")
		"weaken":
			apply_status("enemy", StatusEffect.WEAKEN, 2, 20)
			battle_log.emit("Sable disrupts the enemy's stance!")
			ally_action.emit("Sable", "weaken", 20)
		"guard":
			player_defending = true
			battle_log.emit("Sable shields Arrel! Damage halved this turn.")
			ally_action.emit("Sable", "guard", 0)

## S53: 토비아스 명령 설정
func set_tobias_command(command: String) -> void:
	tobias_command = command
	tobias_command_pending = true

## 토비아스 지원 행동 (S53: 분석/기록/방어)
func _tobias_support_action(forced_action: String = "") -> void:
	_ally_actions_this_battle += 1
	_add_momentum(4.0, "Tobias support")
	_check_tactical_objective("ally")
	var actions = ["analyze", "archive", "protect"]
	var action = forced_action if forced_action != "" else actions[randi_range(0, actions.size() - 1)]
	match action:
		"analyze":
			if current_enemy:
				var weakness_text = current_enemy.weakness if current_enemy.weakness != "" else "none"
				var resist_text = current_enemy.resistance if current_enemy.resistance != "" else "none"
				battle_log.emit("Tobias analyzes the enemy...")
				battle_log.emit("  Weakness: %s  |  Resistance: %s" % [weakness_text.to_upper(), resist_text.to_upper()])
				ally_action.emit("Tobias", "analyze", 0)
				# Mark as scanned
				if current_enemy.name not in scanned_enemies:
					scanned_enemies.append(current_enemy.name)
				enemy_scanned.emit(current_enemy.name, weakness_text, resist_text)
				_add_momentum(8.0, "Enemy analyzed")
				_check_tactical_objective("scan")
				# Record scan data permanently in Codex bestiary
				if Codex.enemy_entries.has(current_enemy.name):
					Codex.enemy_entries[current_enemy.name]["weakness"] = current_enemy.weakness
					Codex.enemy_entries[current_enemy.name]["resistance"] = current_enemy.resistance
					Codex.enemy_entries[current_enemy.name]["scanned"] = true
					Codex._save_data()
		"archive":
			battle_log.emit("Tobias opens his records — burn power boosted by 15%%!")
			ally_action.emit("Tobias", "archive", 15)
			# Boost implemented via a temporary echo-like effect
			active_echoes.append({"id": "tobias_archive", "grade": 0, "npc": "Tobias", "type": "burn_boost", "power": 15, "turns": 1})
		"protect":
			player_defending = true
			battle_log.emit("Tobias raises a ward from his ledger! Damage reduced by 30%%.")
			ally_action.emit("Tobias", "protect", 30)

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

## ===================== Encounter Modifier 처리 (S51) =====================

func _process_modifier_effects() -> void:
	if _encounter_modifier.is_empty():
		return
	var effect = _encounter_modifier.get("effect", "")
	var value = _encounter_modifier.get("value", 0)
	match effect:
		"dot_both":
			# 양쪽 모두 DoT
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - value)
			if current_enemy and current_enemy.is_alive():
				current_enemy.take_damage(value)
			battle_log.emit("[CORRUPTION] The ground burns — %d damage to both sides." % value)
		"player_dot":
			GameManager.player_data.hp = maxi(0, GameManager.player_data.hp - value)
			battle_log.emit("[CORRUPTION] The void gnaws — %d damage." % value)
		"turn_limit":
			if _total_turns >= value:
				battle_log.emit("[CORRUPTION] The Watcher's patience ends. You are recalled.")
				if _try_last_stand_resonance(true):
					_total_turns = 0
					return
				state = BattleState.DEFEAT
				battle_ended.emit(BattleState.DEFEAT)
				_cleanup()
		"enemy_double_turn":
			if _total_turns > 0 and _total_turns % value == 0:
				# 추가 적 턴
				if current_enemy and current_enemy.is_alive():
					battle_log.emit("[CORRUPTION] Time fractures — the enemy moves again!")
					_enemy_turn()

func has_modifier(effect_name: String) -> bool:
	return _encounter_modifier.get("effect", "") == effect_name

## ===================== Memory Echo (S51) =====================

func _apply_memory_echo(memory: MemoryManager.Memory) -> void:
	var echo := {}
	echo["id"] = memory.id
	echo["grade"] = memory.grade
	echo["npc"] = memory.related_npc
	echo["turns"] = 3
	match memory.grade:
		MemoryManager.MemoryGrade.GRADE_5:
			echo["type"] = "fading_warmth"
			echo["power"] = 5
			echo["turns"] = 4  # S53: 힐 에코 4턴으로 증가
			battle_log.emit("[ECHO] Fading Warmth — heal 5 HP/turn for 4 turns.")
		MemoryManager.MemoryGrade.GRADE_4:
			echo["type"] = "lingering_habit"
			echo["power"] = 10
			echo["turns"] = 3  # S53: 콤보 에코 3턴 유지
			battle_log.emit("[ECHO] Lingering Habit — combo multiplier boosted.")
		MemoryManager.MemoryGrade.GRADE_3:
			if memory.related_npc == "Elia":
				echo["type"] = "elia_anchor"
				echo["power"] = 25
				echo["turns"] = 5  # S53: 엘리아 앵커 5턴으로 증가
				battle_log.emit("[ECHO] Elia's Anchor — 25%% chance to halve next hit.")
			elif memory.related_npc == "Sable":
				echo["type"] = "sable_shadow"
				echo["power"] = 0
				battle_log.emit("[ECHO] Sable's Shadow — Sable attacks every turn.")
			else:
				echo["type"] = "bond_fracture"
				echo["power"] = 15
				battle_log.emit("[ECHO] Bond Fracture — +15%% critical chance.")
		MemoryManager.MemoryGrade.GRADE_2:
			echo["type"] = "identity_fracture"
			echo["power"] = 0
			echo["turns"] = 99  # 전투 종료까지
			battle_log.emit("[ECHO] Identity Fracture — all attacks deal void damage.")
		MemoryManager.MemoryGrade.GRADE_1:
			echo["type"] = "total_erasure"
			echo["power"] = 2  # 2배 데미지 횟수
			echo["turns"] = 2
			battle_log.emit("[ECHO] Total Erasure — next 2 attacks deal double damage!")
	active_echoes.append(echo)
	_echoes_activated_this_battle += 1
	_check_tactical_objective("echo")
	echo_activated.emit(echo["type"], "")

func _process_echoes_turn() -> void:
	# 턴 종료 시 에코 틱 처리
	var to_remove: Array = []
	for echo in active_echoes:
		match echo["type"]:
			"fading_warmth":
				var heal = echo["power"]
				GameManager.player_data.hp = mini(GameManager.player_data.hp + heal, GameManager.player_data.max_hp)
				battle_log.emit("[ECHO] Warmth restores %d HP." % heal)
				damage_dealt.emit("Arrel", -heal, "Echo Heal")
		echo["turns"] -= 1
		if echo["turns"] <= 0:
			to_remove.append(echo)
	for e in to_remove:
		active_echoes.erase(e)

func has_echo(echo_type: String) -> bool:
	for echo in active_echoes:
		if echo["type"] == echo_type:
			return true
	return false

func consume_echo_charge(echo_type: String) -> bool:
	for echo in active_echoes:
		if echo["type"] == echo_type and echo.get("power", 0) > 0:
			echo["power"] -= 1
			if echo["power"] <= 0:
				active_echoes.erase(echo)
			return true
	return false

## ===================== Battle Stance (S51) =====================

func switch_stance(new_stance: Stance) -> void:
	# 해금 체크
	var info = STANCE_INFO[new_stance]
	if GameManager.current_chapter < info["unlock_chapter"]:
		battle_log.emit("Stance not yet unlocked.")
		return
	if new_stance == current_stance:
		return
	current_stance = new_stance
	_stance_switches_this_battle += 1
	stance_changed.emit(new_stance)
	battle_log.emit("Switched to %s stance." % info["name"])
	_add_momentum(7.0, "%s stance" % info["name"])
	_check_tactical_objective("stance")

func get_stance_atk_mult() -> float:
	return STANCE_INFO[current_stance]["atk_mult"]

func get_stance_def_mult() -> float:
	return STANCE_INFO[current_stance]["def_mult"]

func _cleanup() -> void:
	var completed_state := state
	active_echoes.clear()
	if state == BattleState.VICTORY:
		# 승리 시 HP 20% 회복
		var heal = int(GameManager.player_data.max_hp * 0.2)
		# Environment heal reduction (bl07_void)
		heal = int(heal * get_env_heal_mult())
		GameManager.player_data.hp = mini(
			GameManager.player_data.hp + heal,
			GameManager.player_data.max_hp
		)
		AudioManager.play_sfx("heal")
		battle_log.emit("Recovered %d HP." % heal)

		# Grains 보상
		var grains = _get_grains_reward()
		var tactical_bonus: int = _get_tactical_bonus()
		var objective_reward := _finalize_tactical_objective()
		var objective_bonus: int = int(objective_reward.get("grains", 0))
		var objective_heal: int = int(objective_reward.get("heal", 0))
		if objective_heal > 0:
			var actual_objective_heal: int = mini(objective_heal, GameManager.player_data.max_hp - GameManager.player_data.hp)
			GameManager.player_data.hp += actual_objective_heal
			heal += actual_objective_heal
			battle_log.emit("[OBJECTIVE BONUS] Restored %d extra HP." % actual_objective_heal)
			damage_dealt.emit("Arrel", -actual_objective_heal, "Objective Heal")
		var momentum_bonus: int = _get_momentum_grains_bonus()
		grains += tactical_bonus + objective_bonus + momentum_bonus
		GameManager.player_data.grains += grains
		GameManager.add_stat("total_grains_earned", grains)  # S55
		battle_log.emit("Gained %d Grains." % grains)
		if tactical_bonus > 0:
			battle_log.emit("[CODEX BONUS] Tactical record +%d Grains." % tactical_bonus)
		if objective_bonus > 0:
			battle_log.emit("[OBJECTIVE BONUS] %s +%d Grains." % [objective_reward.get("title", "Objective"), objective_bonus])
		if momentum_bonus > 0:
			battle_log.emit("[RESONANCE BONUS] %s +%d Grains." % [_get_momentum_label(), momentum_bonus])
		NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)
		AchievementManager.check_grains()

		# 아이템 드롭 (30% 확률)
		var dropped_item = _try_item_drop_return()

		# S58: Emit structured reward data for animated rewards screen
		var reward_data: Dictionary = {
			"grains": grains,
			"tactical_bonus": tactical_bonus,
			"objective_bonus": objective_bonus,
			"objective_title": objective_reward.get("title", ""),
			"objective_item": objective_reward.get("item", ""),
			"objective_heal": objective_heal,
			"momentum_bonus": momentum_bonus,
			"momentum_rank": _best_momentum_rank,
			"momentum_label": _get_momentum_label(_best_momentum_rank),
			"heal": heal,
			"item": dropped_item,
			"enemy_name": current_enemy.name if current_enemy else "Unknown",
			"is_boss": current_enemy.is_boss if current_enemy else false,
			"battles_total": GameManager.play_stats.get("total_battles", 0),
		}
		_victory_dismissed = false
		victory_rewards_ready.emit(reward_data)

		# S58: Wait for player to dismiss the rewards screen (max 15s safety)
		var wait_time: float = 0.0
		while not _victory_dismissed and wait_time < 15.0:
			await get_tree().create_timer(0.1).timeout
			wait_time += 0.1

	elif state == BattleState.DEFEAT:
		battle_log.emit("Darkness closes in...")
		await get_tree().create_timer(1.5).timeout
		if _battle_started_as_boss_rush:
			current_enemy = null
			player_statuses.clear()
			enemy_statuses.clear()
			_battle_started_as_boss_rush = false
			battle_cleanup_finished.emit(completed_state)
			return
		# 게임 오버 화면으로 전환 (current_enemy/return_scene 유지)
		await SceneTransition.change_scene("res://scenes/ui/game_over.tscn")
		return

	await get_tree().create_timer(0.3).timeout
	current_enemy = null
	player_statuses.clear()
	enemy_statuses.clear()
	# Boss Rush: don't transition scene. GameManager advances after cleanup.
	if _battle_started_as_boss_rush:
		_battle_started_as_boss_rush = false
		battle_cleanup_finished.emit(completed_state)
		return
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	if return_scene != "":
		await SceneTransition.change_scene_styled(return_scene)

## 전투 승리 시 아이템 드롭
func _try_item_drop() -> void:
	_try_item_drop_return()

func _get_tactical_bonus() -> int:
	if current_enemy == null:
		return 0
	if scanned_enemies.has(current_enemy.name):
		return 3 if current_enemy.is_boss else 1
	return 0

## S58: 아이템 드롭 + 드롭된 아이템 이름 반환
func _try_item_drop_return() -> String:
	if randf() > 0.30:  # 30% 확률
		return ""
	var drop_table: Array = ["potion", "potion", "potion", "antidote", "antidote", "firebomb"]
	if current_enemy and current_enemy.is_void_beast:
		drop_table.append_array(["firebomb", "hi_potion"])
	if current_enemy and current_enemy.is_boss:
		drop_table.append_array(["hi_potion", "hi_potion", "smoke_bomb"])
	var drop = drop_table[randi_range(0, drop_table.size() - 1)]
	GameManager.add_item(drop)
	battle_log.emit("Found: %s" % GameManager.ITEMS[drop]["name"])
	return GameManager.ITEMS[drop]["name"]

## ===================== 상태이상 시스템 =====================

## 상태이상 적용 (대상: "player" 또는 "enemy")
func apply_status(target: String, effect: StatusEffect, turns: int, power: int) -> void:
	# Environment: status effect boost (seam_outskirts — +1 turn duration)
	var env = ENV_BONUSES.get(battle_environment, {})
	if env.has("status_boost"):
		turns += 1
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
	# S55: Tutorial hint for player status effects
	if target == "player":
		TutorialHints.show_hint("first_status_effect")

## 턴 시작 시 상태이상 처리 (DoT 등)
func _process_statuses(target: String) -> void:
	var list = player_statuses if target == "player" else enemy_statuses
	var expired: Array = []

	for entry in list:
		match entry.effect:
			StatusEffect.POISON:
				var dmg = entry.power
				# Environment: poison damage boost (forgotten_forest)
				var env = ENV_BONUSES.get(battle_environment, {})
				if env.has("poison_boost"):
					dmg = int(dmg * (1.0 + env["poison_boost"]))
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

	_player_actions_this_battle += 1
	_limit_breaks_this_battle += 1
	_check_tactical_objective("action")
	_check_tactical_objective("limit")
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
	dmg = _apply_momentum_damage_bonus(dmg)

	var actual = current_enemy.take_damage(dmg)
	AudioManager.play_combat_sfx("burn_ignite")  # S58: 레이어드 연소 SFX
	InputManager.vibrate("memory_burn")
	battle_log.emit("[LIMIT BREAK] Memory Cascade!")
	battle_log.emit("All remembered pain converges — %d damage!" % actual)
	_log_element_effect("void")
	damage_dealt.emit(current_enemy.name, actual, "Memory Cascade")
	_add_momentum(15.0, "Limit released")

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

	_player_actions_this_battle += 1
	_check_tactical_objective("action")
	_reset_combo("burn")
	var skill = BURN_SKILLS.get(memory.grade, BURN_SKILLS[0])
	AudioManager.play_combat_sfx("burn_ignite")  # S58: 레이어드 연소 SFX
	InputManager.vibrate("memory_burn")
	var dmg = int((skill.base_damage + memory.burn_power) * 0.5)
	var burn_element = skill.get("element", "fire")
	var elem_mult = _get_element_multiplier(burn_element)
	dmg = int(dmg * elem_mult)

	if enemy_shielded:
		dmg = maxi(1, int(dmg * 0.7))
		enemy_shielded = false
		battle_log.emit("The barrier weakens the residue flames!")
	dmg = _apply_momentum_damage_bonus(dmg)
	var actual = current_enemy.take_damage(dmg)

	battle_log.emit("[RESIDUE] %s — a faded echo of %s" % [skill.name, memory.title])
	battle_log.emit("%d damage to %s. (50%% power)" % [actual, current_enemy.name])
	_log_element_effect(burn_element)
	damage_dealt.emit(current_enemy.name, actual, "Residue: " + skill.name)
	_add_limit(LIMIT_GAIN_BURN * 0.5)
	_add_momentum(6.0, "Residue burn")

	_check_enemy_defeated()

## ===================== Auto Battle (S55) =====================

## 자동 전투 토글
func toggle_auto_battle() -> void:
	auto_battle = not auto_battle
	auto_battle_changed.emit(auto_battle)
	if auto_battle:
		battle_log.emit("[AUTO] Auto-battle engaged.")
	else:
		battle_log.emit("[AUTO] Auto-battle disengaged.")

## 자동 전투 AI — 플레이어 턴에 자동 행동 선택
func auto_battle_action() -> void:
	if state != BattleState.PLAYER_TURN or current_enemy == null:
		return
	var hp_pct = float(GameManager.player_data.hp) / float(GameManager.player_data.max_hp)
	# 1. HP < 30%: 포션 사용
	if hp_pct < 0.3:
		if GameManager.get_item_count("hi_potion") > 0:
			player_use_item("hi_potion")
			return
		elif GameManager.get_item_count("potion") > 0:
			player_use_item("potion")
			return
	# 2. 상태이상: 해독제 사용
	if not player_statuses.is_empty():
		var has_curable = false
		for s in player_statuses:
			if s.effect == StatusEffect.POISON or s.effect == StatusEffect.BURN:
				has_curable = true
				break
		if has_curable and GameManager.get_item_count("antidote") > 0:
			player_use_item("antidote")
			return
	# 3. 적 HP 50% 이상 & 고등급 기억 보유: 연소
	if current_enemy and float(current_enemy.hp) / float(current_enemy.max_hp) > 0.5:
		var available = MemoryManager.get_available_memories().filter(func(m): return not m.is_faded)
		# 고등급부터 사용 (Grade 2=Identity 이상)
		var best_mem = null
		for m in available:
			if m.grade >= 2:
				if best_mem == null or m.grade > best_mem.grade:
					best_mem = m
		if best_mem:
			player_burn(best_mem.id)
			return
	# 4. 기본 공격
	player_attack()
