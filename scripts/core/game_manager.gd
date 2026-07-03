## GameManager (Autoload)
## 전체 게임 상태 관리. 씬 간 데이터 유지.
extends Node

# --- 게임 상태 ---
enum GameState { EXPLORATION, DIALOGUE, BATTLE, CUTSCENE, MENU, PAUSED }
var current_state: GameState = GameState.EXPLORATION

# --- 챕터 진행 ---
var current_chapter: int = 1
var story_flags: Dictionary = {}  # "met_elia": true, "malet_deal": false 등

# --- New Game+ ---
var ng_plus_cycle: int = 0  # 0 = 일반, 1+ = NG+ 회차
const NG_PLUS_FILE: String = "user://ng_plus.json"

# --- S54: Ending Gallery — 본 엔딩 추적 ---
var seen_endings: Array = []  # ["zero_burn", "preservation", ...]
const ENDING_DATA: Dictionary = {
	"zero_burn": {"name": "Zero Burn", "desc": "He burned everything. Even his name.", "cg": "res://assets/cg/generated/ending_zero_burn_trying_name.png"},
	"preservation": {"name": "Preservation", "desc": "He kept his name. The search continues.", "cg": "res://assets/cg/generated/ending_preservation_building_hands.png"},
	"ash": {"name": "Ash", "desc": "The name remains. The person behind it does not.", "cg": "res://assets/cg/generated/ending_ash_sunset_shell.png"},
	"seam": {"name": "The Seam Holds", "desc": "In the cracks between loss, something green still grows.", "cg": "res://assets/cg/generated/ending_seam_impossible_garden.png"},
	"tobias": {"name": "The Record Remains", "desc": "The pen outlasts the flame.", "cg": "res://assets/cg/generated/ending_tobias_night_press.png"},
	"hollow": {"name": "Hollow", "desc": "A single name echoes in an empty room.", "cg": "res://assets/cg/generated/ending_hollow_name_room.png"},
	"weave": {"name": "The Weave", "desc": "He spent nothing and sealed everything. The price was only the promise to never set it down.", "cg": "res://assets/cg/generated/ending_weave_colors_return.png"},
}
const SEEN_ENDINGS_FILE: String = "user://seen_endings.json"

## S147: Part III 최종 엔딩 판정 (§7 "남긴 것이 결말을 정한다")
## ch23_conversion의 resolve_part3_ending 액션에서 호출.
## 우선순위: 명시적 이름 연소 > 공허 변이 > Weave 보존 > Ash 소진 > Tobias 기록 > Seam 희망 > Preservation.
func evaluate_part3_ending() -> String:
	# 1. Zero Burn — 이름을 태우는 명시적 선택 (canon 메인 라인: 변환 성공, 이름 소실)
	if get_flag("p3_name_burned") or MemoryManager.is_memory_burned("core_name_origin"):
		return "zero_burn"
	# 2. Hollow — 태운 것이 너무 많아 사람이 남지 않음
	if MemoryManager.get_burn_ratio() >= 0.75:
		return "hollow"
	# 3. The Weave — 보존 플레이를 끝까지 유지 + 변환 시도
	if MemoryManager.weave_unlocked() and get_flag("p3_conversion_attempted"):
		return "weave"
	# 4. Ash — 이름은 지켰지만 관계 기억이 재가 됨
	var rel_burned := 0
	for m in MemoryManager.memories:
		if m.is_burned and m.id.begins_with("rel_"):
			rel_burned += 1
	if rel_burned >= 3 or MemoryManager.get_burn_count() >= 8:
		return "ash"
	# 5. Tobias — 증인의 기록이 살아남음
	if get_flag("tobias_funeral") and MemoryManager.is_intact("identity_witness_record"):
		return "tobias"
	# 6. Seam — 숨겨진 희망을 발견한 자 (셀라 재회 / 숨은 정원 계열)
	if get_flag("celah_reunion") or (get_flag("hidden_ch1_stump") and get_flag("hidden_ch6_garden")):
		return "seam"
	# 7. Preservation — 기본: 이름을 지키고 계속 나아감
	return "preservation"

func record_ending(ending_id: String) -> void:
	if ending_id not in seen_endings:
		seen_endings.append(ending_id)
		_save_seen_endings()
		print("[GameManager] Ending recorded: %s" % ending_id)

func _load_seen_endings() -> void:
	if not FileAccess.file_exists(SEEN_ENDINGS_FILE):
		return
	var file = FileAccess.open(SEEN_ENDINGS_FILE, FileAccess.READ)
	var json = JSON.new()
	if json.parse(file.get_as_text()) == OK and json.data is Array:
		seen_endings = json.data
	file.close()

func _save_seen_endings() -> void:
	var file = FileAccess.open(SEEN_ENDINGS_FILE, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(seen_endings))
		file.close()

# --- Boss Rush Mode ---
var boss_rush_mode: bool = false
var boss_rush_queue: Array = []  # [{name, hp, atk, is_void, abilities, weakness, resistance}]
var boss_rush_index: int = 0
var boss_rush_start_time: float = 0.0
var boss_rush_best_time: float = 0.0  # seconds, 0 = no record
const BOSS_RUSH_FILE: String = "user://boss_rush.json"

const BOSS_RUSH_BOSSES: Array = [
	{"name": "Shade Sentinel", "hp": 300, "atk": 28, "is_void": true, "abilities": ["shield", "drain", "multi_hit", "summon"], "weakness": "void", "resistance": "physical"},
	{"name": "Kairos, The Watcher", "hp": 500, "atk": 45, "is_void": true, "abilities": ["void_pulse", "drain", "shield", "charge", "reflect", "stun"], "weakness": "fire", "resistance": "void"},
]

func is_boss_rush_unlocked() -> bool:
	return seen_endings.size() > 0

func start_boss_rush() -> void:
	boss_rush_mode = true
	boss_rush_index = 0
	boss_rush_queue = BOSS_RUSH_BOSSES.duplicate(true)
	boss_rush_start_time = Time.get_unix_time_from_system()
	_load_boss_rush_record()
	# Reset player for boss rush
	player_data.hp = player_data.max_hp
	player_data.items = {"potion": 3, "hi_potion": 2, "antidote": 2}
	print("[GameManager] Boss Rush started — %d bosses" % boss_rush_queue.size())
	_start_next_boss()

func _start_next_boss() -> void:
	if boss_rush_index >= boss_rush_queue.size():
		_boss_rush_complete()
		return
	var data = boss_rush_queue[boss_rush_index]
	var enemy = BattleManager.Enemy.new(data["name"], data["hp"], data["atk"], data["is_void"])
	enemy.is_boss = true
	enemy.abilities = data.get("abilities", [])
	enemy.weakness = data.get("weakness", "")
	enemy.resistance = data.get("resistance", "")
	BattleManager.start_battle(enemy, "", "", "")
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")

func on_boss_rush_battle_ended(result: int) -> void:
	if not boss_rush_mode:
		return
	if result == BattleManager.BattleState.VICTORY:
		boss_rush_index += 1
		if boss_rush_index < boss_rush_queue.size():
			# Heal 50% between bosses
			player_data.hp = mini(player_data.hp + int(player_data.max_hp * 0.5), player_data.max_hp)
			NotificationToast.show_toast("Boss %d/%d defeated! 50%% HP restored." % [boss_rush_index, boss_rush_queue.size()], NotificationToast.ToastType.SUCCESS)
			# Small delay before next boss
			get_tree().create_timer(2.0).timeout.connect(_start_next_boss)
		else:
			_boss_rush_complete()
	elif result == BattleManager.BattleState.DEFEAT:
		boss_rush_mode = false
		NotificationToast.show_toast("Boss Rush failed at boss %d/%d." % [boss_rush_index + 1, boss_rush_queue.size()], NotificationToast.ToastType.WARNING)
		change_state(GameState.MENU)
		SceneTransition.change_scene("res://scenes/main/main.tscn")

func _boss_rush_complete() -> void:
	var elapsed = Time.get_unix_time_from_system() - boss_rush_start_time
	boss_rush_mode = false
	var is_record = false
	if boss_rush_best_time <= 0.0 or elapsed < boss_rush_best_time:
		boss_rush_best_time = elapsed
		is_record = true
		_save_boss_rush_record()
	var mins = int(elapsed) / 60
	var secs = int(elapsed) % 60
	var record_text = " NEW RECORD!" if is_record else ""
	NotificationToast.show_toast("Boss Rush Complete! Time: %d:%02d%s" % [mins, secs, record_text], NotificationToast.ToastType.SUCCESS)
	print("[GameManager] Boss Rush complete — %.1fs%s" % [elapsed, record_text])
	change_state(GameState.MENU)
	SceneTransition.change_scene("res://scenes/main/main.tscn")

func _load_boss_rush_record() -> void:
	if not FileAccess.file_exists(BOSS_RUSH_FILE):
		return
	var file = FileAccess.open(BOSS_RUSH_FILE, FileAccess.READ)
	var json = JSON.new()
	if json.parse(file.get_as_text()) == OK and json.data is Dictionary:
		boss_rush_best_time = json.data.get("best_time", 0.0)
	file.close()

func _save_boss_rush_record() -> void:
	var file = FileAccess.open(BOSS_RUSH_FILE, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify({"best_time": boss_rush_best_time}))
		file.close()

# --- S54: NPC Schedule System ---
# Maps npc_name -> {chapter_min: {position: Vector2, dialogue_key: str, visible: bool}}
var NPC_SCHEDULES: Dictionary = {
	"malet": {
		# Ch2: present at market (default position)
		2: {"pos": Vector2(14, 12), "dialogue": "malet_encounter", "visible": true},
		# Ch3-5: malet still at market but moved to different corner
		3: {"pos": Vector2(22, 4), "dialogue": "malet_revisit_ch3", "visible": true},
		# Ch6+: malet is in the Sump (barely visible)
		6: {"pos": Vector2(12, 17), "dialogue": "malet_late_game", "visible": true},
	},
	"tobias": {
		# Ch3: at waystation (default)
		3: {"pos": Vector2(11, 9), "dialogue": "tobias_encounter", "visible": true},
		# Ch4-6: tobias gone (traveling with party)
		4: {"pos": Vector2(11, 9), "dialogue": "", "visible": false},
		# Ch7+: tobias returns to waystation for research
		7: {"pos": Vector2(11, 9), "dialogue": "tobias_research", "visible": true},
	},
}

## Get NPC schedule for given chapter. Returns the most recent schedule entry at or before the chapter.
func get_npc_schedule(npc_name: String, chapter: int) -> Dictionary:
	if not NPC_SCHEDULES.has(npc_name):
		return {}
	var schedule = NPC_SCHEDULES[npc_name]
	var best_ch: int = -1
	for ch_key in schedule:
		var ch_int = int(ch_key)
		if ch_int <= chapter and ch_int > best_ch:
			best_ch = ch_int
	if best_ch >= 0:
		return schedule[best_ch]
	return {}

# --- 플레이어 데이터 ---
var player_data: Dictionary = {
	"name": "Arrel",
	"hp": 100,
	"max_hp": 100,
	"grains": 0,  # 화폐
	"elia_with_party": true,  # 엘리아 동행 여부
	"items": {},  # 아이템 인벤토리 {"potion": 2, "antidote": 1, ...}
}

# --- 아이템 정의 ---
const ITEMS: Dictionary = {
	"potion": {"name": "Potion", "desc": "Restores 40 HP.", "type": "heal", "power": 40, "price": 12},
	"hi_potion": {"name": "Hi-Potion", "desc": "Restores 80 HP.", "type": "heal", "power": 80, "price": 25},
	"antidote": {"name": "Antidote", "desc": "Cures poison and burn.", "type": "cure", "power": 0, "price": 10},
	"firebomb": {"name": "Firebomb", "desc": "Burns the enemy for 2 turns.", "type": "burn", "power": 15, "price": 18},
	"smoke_bomb": {"name": "Smoke Bomb", "desc": "Guaranteed escape from battle.", "type": "flee", "power": 0, "price": 15},
}

# --- S41: 장비 시스템 ---
const EQUIPMENT: Dictionary = {
	# 무기
	"rusty_blade": {"name": "Rusty Blade", "slot": "weapon", "atk": 3, "def": 0, "desc": "A dull, corroded sword.", "price": 15},
	"iron_sword": {"name": "Iron Sword", "slot": "weapon", "atk": 8, "def": 0, "desc": "Sturdy and reliable.", "price": 35},
	"void_edge": {"name": "Void Edge", "slot": "weapon", "atk": 15, "def": 0, "desc": "A blade that hums with void energy.", "price": 80, "element": "void"},
	"ember_brand": {"name": "Ember Brand", "slot": "weapon", "atk": 12, "def": 0, "desc": "Warm to the touch. Burns on contact.", "price": 60, "element": "fire"},
	# 방어구
	"worn_coat": {"name": "Worn Coat", "slot": "armor", "atk": 0, "def": 3, "desc": "Threadbare but better than nothing.", "price": 12},
	"leather_vest": {"name": "Leather Vest", "slot": "armor", "atk": 0, "def": 7, "desc": "Basic protection.", "price": 30},
	"memory_weave": {"name": "Memory Weave", "slot": "armor", "atk": 2, "def": 12, "desc": "Woven from residue threads. Resists void.", "price": 70},
	# 액세서리
	"ash_pendant": {"name": "Ash Pendant", "slot": "accessory", "atk": 0, "def": 0, "desc": "Increases burn damage by 20%.", "price": 50, "effect": "burn_boost"},
	"iron_ring": {"name": "Iron Ring", "slot": "accessory", "atk": 3, "def": 3, "desc": "Simple but effective.", "price": 40},
	"void_charm": {"name": "Void Charm", "slot": "accessory", "atk": 0, "def": 0, "desc": "Reduces void damage taken by 25%.", "price": 65, "effect": "void_resist"},
	# S53: NG++ 전용 장비
	"void_edge_plus": {"name": "Void Edge", "slot": "weapon", "atk": 18, "def": 0, "price": 200, "desc": "Forged from collapsed void. NG++ only.", "element": "void", "effect": "burn_boost"},
	"memory_plate": {"name": "Memory Plate", "slot": "armor", "atk": 0, "def": 16, "price": 200, "desc": "Woven from residual memories. NG++ only.", "element": "", "effect": "erosion_resist"},
}

## S53: 장비 강화
var upgrade_levels: Dictionary = {}  # {"equip_id": level}

var equipped: Dictionary = {"weapon": "", "armor": "", "accessory": ""}

func equip_item(equip_id: String) -> String:
	if not EQUIPMENT.has(equip_id):
		return ""
	var slot: String = EQUIPMENT[equip_id].slot
	# S58: Track old stats for delta popup
	var old_atk = get_equip_bonus("atk")
	var old_def = get_equip_bonus("def")
	var old = equipped[slot]
	equipped[slot] = equip_id
	# S58: Emit stat change popups
	var new_atk = get_equip_bonus("atk")
	var new_def = get_equip_bonus("def")
	if new_atk != old_atk:
		stat_gained.emit("ATK", new_atk - old_atk)
	if new_def != old_def:
		stat_gained.emit("DEF", new_def - old_def)
	# S55: Tutorial hint
	TutorialHints.show_hint("first_equipment")
	return old

func upgrade_equipment(equip_id: String) -> bool:
	if not EQUIPMENT.has(equip_id):
		return false
	var level = upgrade_levels.get(equip_id, 0)
	var cost = (level + 1) * 30  # 30, 60, 90...
	if player_data.grains < cost or level >= 3:
		return false
	player_data.grains -= cost
	upgrade_levels[equip_id] = level + 1
	# S58: Stat gain popup for upgrade (+3 per level to relevant stat)
	var eq = EQUIPMENT[equip_id]
	if eq.get("atk", 0) > 0:
		stat_gained.emit("ATK", 3)
	if eq.get("def", 0) > 0:
		stat_gained.emit("DEF", 3)
	return true

func get_upgrade_level(equip_id: String) -> int:
	return upgrade_levels.get(equip_id, 0)

func get_upgraded_bonus(equip_id: String, stat: String) -> int:
	if not EQUIPMENT.has(equip_id):
		return 0
	var base = EQUIPMENT[equip_id].get(stat, 0)
	var level = get_upgrade_level(equip_id)
	return base + level * 3  # +3 per upgrade level

func get_equip_bonus(stat: String) -> int:
	var total: int = 0
	for slot in equipped:
		var eid = equipped[slot]
		if eid != "" and EQUIPMENT.has(eid):
			total += get_upgraded_bonus(eid, stat)
	return total

func has_equip_effect(effect_name: String) -> bool:
	for slot in equipped:
		var eid = equipped[slot]
		if eid != "" and EQUIPMENT.has(eid):
			if EQUIPMENT[eid].get("effect", "") == effect_name:
				return true
	return false

func add_item(item_id: String, count: int = 1) -> void:
	if not ITEMS.has(item_id):
		return
	var current = player_data.items.get(item_id, 0)
	player_data.items[item_id] = current + count
	NotificationToast.show_toast("+%d %s" % [count, ITEMS[item_id]["name"]], NotificationToast.ToastType.SUCCESS)

func remove_item(item_id: String, count: int = 1) -> bool:
	var current = player_data.items.get(item_id, 0)
	if current < count:
		return false
	player_data.items[item_id] = current - count
	if player_data.items[item_id] <= 0:
		player_data.items.erase(item_id)
	return true

func get_item_count(item_id: String) -> int:
	return player_data.items.get(item_id, 0)

# --- S55: Play Statistics ---
var play_stats: Dictionary = {
	"play_time_seconds": 0.0,
	"total_battles": 0,
	"total_burns": 0,
	"total_grains_earned": 0,
	"enemies_defeated": 0,
	"memories_collected": 0,
	"steps_taken": 0,
	"highest_combo": 0,
	"highest_momentum_rank": 0,
	"objectives_completed": 0,
	"momentum_surges": 0,
	"bosses_defeated": 0,
	"items_used": 0,
}

## 통계 증가 헬퍼
func add_stat(stat_name: String, amount = 1) -> void:
	if play_stats.has(stat_name):
		play_stats[stat_name] += amount

## 통계 최대값 갱신 헬퍼
func max_stat(stat_name: String, value) -> void:
	if play_stats.has(stat_name):
		if value > play_stats[stat_name]:
			play_stats[stat_name] = value

## 플레이 타임 포맷 (HH:MM:SS)
func format_play_time() -> String:
	var total = int(play_stats.play_time_seconds)
	var hours = total / 3600
	var mins = (total % 3600) / 60
	var secs = total % 60
	return "%02d:%02d:%02d" % [hours, mins, secs]

# --- S55/S133: Localization Foundation ---
var current_locale: String = "ko"

const LOCALIZED_STRINGS: Dictionary = {
	"attack": {"en": "ATTACK", "ko": "공격"},
	"burn": {"en": "BURN", "ko": "연소"},
	"defend": {"en": "DEFEND", "ko": "방어"},
	"item": {"en": "ITEM", "ko": "아이템"},
	"auto": {"en": "AUTO", "ko": "자동"},
	"limit": {"en": "LIMIT", "ko": "리밋"},
	"flee": {"en": "FLEE", "ko": "도주"},
	"victory": {"en": "VICTORY", "ko": "승리"},
	"defeat": {"en": "DEFEAT", "ko": "패배"},
	"save": {"en": "Save (Slot 1)", "ko": "저장 (슬롯 1)"},
	"load": {"en": "Load (Slot 1)", "ko": "불러오기 (슬롯 1)"},
	"options": {"en": "Options", "ko": "옵션"},
	"resume": {"en": "Resume", "ko": "계속"},
	"quit": {"en": "Quit Game", "ko": "게임 종료"},
	"stats": {"en": "Stats", "ko": "통계"},
	"journal": {"en": "Journal", "ko": "저널"},
	"travel": {"en": "Travel", "ko": "이동"},
	"codex": {"en": "Codex", "ko": "도감"},
	"achievements": {"en": "Achievements", "ko": "업적"},
	"endings": {"en": "Endings", "ko": "엔딩"},
	"title_return": {"en": "Return to Title", "ko": "타이틀로"},
	"paused": {"en": "PAUSED", "ko": "일시정지"},
	"hp": {"en": "HP", "ko": "HP"},
	"grains": {"en": "Grains", "ko": "그레인"},
	"language": {"en": "Language", "ko": "언어"},
	"back": {"en": "Back", "ko": "뒤로"},
	"new_game": {"en": "New Game", "ko": "새 게임"},
	"continue": {"en": "Continue", "ko": "이어하기"},
	"aftermath": {"en": "Part II: Aftermath", "ko": "2부: 여파"},
	"title_subtitle": {"en": "The Price of Oblivion", "ko": "망각의 대가"},
	"title_tagline": {"en": "Burn what you remember. Carry what remains.", "ko": "기억을 태워라. 남은 것을 짊어져라."},
	"game_over_title": {"en": "You fell.", "ko": "당신은 쓰러졌다."},
	"game_over_subtitle": {"en": "Something pulls you back from the edge...", "ko": "무언가가 끝자락에서 당신을 끌어당긴다..."},
	"retry": {"en": "Stagger On (HP 30%)", "ko": "비틀거리며 계속한다 (HP 30%)"},
	"load_save": {"en": "Load Save", "ko": "저장 불러오기"},
	"audio": {"en": "AUDIO", "ko": "오디오"},
	"gameplay": {"en": "GAMEPLAY", "ko": "게임플레이"},
	"display": {"en": "DISPLAY", "ko": "화면"},
	"performance": {"en": "PERFORMANCE", "ko": "성능"},
	"accessibility": {"en": "ACCESSIBILITY", "ko": "접근성"},
	"master_volume": {"en": "Master Volume", "ko": "전체 음량"},
	"bgm_volume": {"en": "BGM Volume", "ko": "배경음악 음량"},
	"sfx_volume": {"en": "SFX Volume", "ko": "효과음 음량"},
	"text_speed": {"en": "Text Speed", "ko": "텍스트 속도"},
	"difficulty": {"en": "Difficulty", "ko": "난이도"},
	"auto_narration": {"en": "Auto-Advance Narration", "ko": "나레이션 자동 진행"},
	"fullscreen": {"en": "Fullscreen", "ko": "전체 화면"},
	"resolution": {"en": "Resolution", "ko": "해상도"},
	"quality": {"en": "Quality", "ko": "그래픽 품질"},
	"show_fps": {"en": "Show FPS", "ko": "FPS 표시"},
	"dialogue_font_size": {"en": "Dialogue Font Size", "ko": "대화 글자 크기"},
	"high_contrast": {"en": "High Contrast", "ko": "고대비"},
	"screen_shake": {"en": "Screen Shake", "ko": "화면 흔들림"},
	"colorblind_mode": {"en": "Colorblind Mode", "ko": "색각 보정"},
	"reduce_motion": {"en": "Reduce Motion", "ko": "움직임 줄이기"},
	"options_help": {"en": "Font Size affects dialogue text. High Contrast increases text outlines and UI borders. Reduce Motion disables particles and simplifies animations.", "ko": "글자 크기는 대화문에 적용됩니다. 고대비는 글자 윤곽과 UI 테두리를 강화합니다. 움직임 줄이기는 파티클을 끄고 애니메이션을 단순화합니다."},
}

const SPEAKER_NAMES_KO: Dictionary = {
	"": "",
	"system_log": "시스템",
	"System": "시스템",
	"Narration": "나레이션",
	"Arrel": "아렐",
	"Elia": "엘리아",
	"Malet": "말렛",
	"Mallet": "말렛",
	"Kairos": "카이로스",
	"Sable": "세이블",
	"Nera": "네라",
	"Seric": "세릭",
	"Tobias": "토비아스",
	"Veil": "베일",
	"Ashen Figure": "잿빛 형상",
	"Old Man": "노인",
	"Nervous Trader": "불안한 상인",
	"Gardener": "정원사",
	"Handler": "관리관",
	"Prisoner": "수감자",
	"Guard": "경비병",
	"Han": "한",
	"Mira": "미라",
	"Vael": "바엘",
	"Belor": "벨로르",
	"Chief Archivist": "수석 기록관",
	"???": "???",
}

const ENEMY_NAMES_KO: Dictionary = {
	"Alley Rat": "골목쥐",
	"Ash Crawler": "잿빛 크롤러",
	"Ash Phantom": "재의 환영",
	"Ash Walker": "재의 방랑자",
	"Belt Scavenger": "벨트 약탈자",
	"Cliff Stalker": "절벽 추적자",
	"Coastal Void Beast": "해안의 보이드 비스트",
	"Colorless Wraith": "무색의 망령",
	"Depth Crawler": "심연 크롤러",
	"Dust Crawler": "먼지 크롤러",
	"Forest Shade": "숲의 그림자",
	"Hollow": "공허체",
	"Hollow Walker": "빈껍데기 방랑자",
	"Kairos, Authority Editor": "카이로스 · 관리국 편집관",
	"Market Thief": "시장 도적",
	"Memory Eater": "기억 포식자",
	"Memory Leech": "기억 거머리",
	"Null Wisp": "무의 도깨비불",
	"Remnant": "잔존체",
	"Root Shade": "뿌리 그림자",
	"Rubble Rat": "잔해쥐",
	"Seam Lurker": "심 틈새의 잠복자",
	"Shade Sentinel": "그림자 파수꾼",
	"Shore Wraith": "해안 망령",
	"Threshold Crawler": "경계 크롤러",
	"Threshold Shade": "경계의 그림자",
	"Void Beast": "보이드 비스트",
	"Void Fragment": "보이드 파편",
	"Void Sentinel": "보이드 파수꾼",
	"Void Watcher": "보이드 감시자",
	"Void Wisp": "보이드 도깨비불",
	"Void Wraith": "보이드 망령",
}

const RUNTIME_TEXT_KO: Dictionary = {
	"Rim Forest": "림 숲",
	"The edge of what remains": "남겨진 세계의 끝자락",
	"Verdan Market": "베르단 시장",
	"Where memories are currency": "기억이 화폐가 되는 곳",
	"The Belt": "벨트",
	"Weight of Pages": "페이지의 무게",
	"Drift": "드리프트",
	"The architecture crumbles": "무너져 내리는 구조",
	"Crumbling Coast": "무너지는 해안",
	"The ground gives way": "발밑이 무너지는 곳",
	"The Seam": "심",
	"Between what was and what will be": "과거와 미래의 사이",
	"The Threshold": "경계",
	"The other side of the flame": "불꽃 너머",
	"The Forest That Forgets": "망각하는 숲",
	"Memory-parasitic ecosystem": "기억을 기생하는 생태계",
	"Where Colors Stop": "색이 멎는 곳",
	"The achromatic zone": "무채색 지대",
	"The Void stares back": "보이드가 마주 바라본다",
	"This market smells like rust and regret.": "이 시장에선 녹과 후회의 냄새가 나.",
	"You know where to find me.": "날 어디서 찾을지는 알겠지.",
	"Still here? Business never sleeps, even when everything else does.": "아직도 여기 있나? 다른 모든 것이 잠들어도 거래는 잠들지 않아.",
	"Come back when you have something worth trading.": "거래할 가치가 있는 걸 가져오면 다시 오게.",
	"This place... it's like the land itself forgot how to live.": "이곳은... 땅 자체가 살아가는 법을 잊은 것 같아.",
	"Fascinating. Absolutely fascinating. Let me write that down.": "흥미롭군. 정말 흥미로워. 기록해 두겠네.",
	"I've been cross-referencing the Bureau records. The patterns are... troubling.": "관리국 기록을 교차 검토 중이네. 이 패턴은... 심상치 않아.",
	"Rest. Please.": "쉬어. 제발.",
	"The ground shifts. Stay close.": "땅이 움직여. 가까이 있어.",
	"I'm here. I'm not going anywhere.": "나 여기 있어. 어디에도 가지 않아.",
	"The Seam holds. For now.": "심은 버티고 있어. 지금은.",
	"The air feels wrong. Like static before a storm.": "공기가 이상해. 폭풍 전의 정전기 같아.",
	"Stay focused. Don't let the Threshold get into your head.": "집중해. 경계가 머릿속까지 파고들게 두지 마.",
	"Say your name. Don't forget it.": "네 이름을 말해. 잊지 마.",
	"Keep moving. Don't look at the shapes between the trees.": "계속 움직여. 나무 사이의 형체는 보지 마.",
	"Hold my hand. Don't let go.": "내 손 잡아. 놓지 마.",
	"Follow the pull. It's the only direction left.": "이끌림을 따라가. 남은 방향은 그것뿐이야.",
	"I can feel it pulling. Don't let go.": "끌어당기는 게 느껴져. 놓지 마.",
	"Something blocks the path. Find what hunts these woods.": "무언가 길을 막고 있다. 이 숲에서 사냥하는 존재를 찾아라.",
	"A faint warmth. A song you no longer know.": "희미한 온기. 이제는 기억나지 않는 노래.",
	"Find the memory fragments.": "기억 파편을 찾아라.",
	"Find the ledger in the Sump.": "섬프에서 장부를 찾아라.",
	"Found a Potion!": "포션을 발견했다!",
	"The crate is empty.": "상자는 비어 있다.",
	"Elia wrote in her diary...": "엘리아가 일지에 글을 남겼다...",
	"No autosave found": "자동 저장을 찾지 못했다",
}

## 게임 시작 시 설정 파일에서 locale 조기 로드 (다른 autoload의 _ready보다 먼저)
func _load_locale_early() -> void:
	var settings_path = "user://settings.json"
	if not FileAccess.file_exists(settings_path):
		return
	var file = FileAccess.open(settings_path, FileAccess.READ)
	if not file:
		return
	var json = JSON.new()
	if json.parse(file.get_as_text()) == OK and json.data is Dictionary:
		if json.data.has("locale"):
			current_locale = json.data["locale"]
	file.close()

## 번역 함수
func loc(key: String) -> String:
	if LOCALIZED_STRINGS.has(key):
		var entry = LOCALIZED_STRINGS[key]
		if entry.has(current_locale):
			return entry[current_locale]
		if entry.has("en"):
			return entry["en"]
	return key

func localized_speaker(speaker: String) -> String:
	if current_locale == "ko":
		return String(SPEAKER_NAMES_KO.get(speaker, speaker))
	return speaker

func localized_enemy_name(enemy_name: String) -> String:
	if current_locale == "ko":
		return String(ENEMY_NAMES_KO.get(enemy_name, enemy_name))
	return enemy_name

func localized_runtime_text(text: String) -> String:
	if current_locale != "ko" or text == "":
		return text
	if RUNTIME_TEXT_KO.has(text):
		return String(RUNTIME_TEXT_KO[text])
	var result := text
	result = result.replace("Obtained: ", "획득: ")
	result = result.replace("Memory acquired: ", "기억 획득: ")
	result = result.replace("Memory burned: ", "기억 연소: ")
	result = result.replace("Memory spent: ", "기억 소모: ")
	result = result.replace("Memory fading: ", "기억 소실 중: ")
	result = result.replace("Quest Complete: ", "퀘스트 완료: ")
	result = result.replace("Passive Unlocked: ", "패시브 해금: ")
	result = result.replace("Elia Technique unlocked: ", "엘리아 기술 해금: ")
	result = result.replace("Synthesized: ", "합성 완료: ")
	result = result.replace("Sold: ", "판매: ")
	result = result.replace("Bought: ", "구매: ")
	result = result.replace("Equipped: ", "장착: ")
	result = result.replace("Upgraded: ", "강화: ")
	result = result.replace(" Grains", " 그레인")
	result = result.replace("Autosaved", "자동 저장 완료")
	result = result.replace("Autosave loaded", "자동 저장 불러오기 완료")
	result = result.replace("Game saved — Slot ", "게임 저장 완료 — 슬롯 ")
	result = result.replace("Game loaded — Slot ", "게임 불러오기 완료 — 슬롯 ")
	return result

func localized_value(source: Dictionary, key: String, fallback: String = "") -> String:
	if source.is_empty():
		return fallback
	var locale_key := "%s_%s" % [key, current_locale]
	if source.has(locale_key):
		return String(source[locale_key])
	if current_locale == "ko" and source.has("ko") and (key == "text" or key == "narrate"):
		return String(source["ko"])
	if source.has(key):
		return String(source[key])
	return fallback

signal state_changed(new_state: GameState)
signal stat_gained(stat_name: String, amount: int)  # S58: Progression feedback

# --- S58: Chapter completion tracking ---
var _chapter_start_battles: int = 0
var _chapter_start_burns: int = 0
var _chapter_start_time: float = 0.0

# --- S58: Rich Presence ---
## Map chapter numbers to human-readable names for Steam Rich Presence.
const RICH_PRESENCE_CHAPTERS: Dictionary = {
	1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation",
	4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam",
	7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste",
	10: "BL-07 Void", 11: "Departure",
	# S147: Part II/III (VN 확장)
	12: "The Reader", 13: "The Third Person", 14: "The Confessor's Hall",
	15: "The Singer", 16: "Nera", 17: "The Forgetting Storm",
	18: "Living Funeral", 19: "The Approach", 20: "Into the Monolith",
	21: "The Editor's Turn", 22: "The Core", 23: "Conversion", 24: "Testimony",
}

## Current rich presence string (for debugging / Steam API)
var _rich_presence_status: String = "In Menu"

## Update Steam Rich Presence status. Called on every state/map transition.
## Shows "In Menu", "Exploring: Rim Forest", "In Battle: Void Beast", "Chapter 3: Weight of Pages" etc.
func update_rich_presence(status: String) -> void:
	_rich_presence_status = status
	# --- GodotSteam Integration Point ---
	# When GodotSteam is installed, uncomment:
	#   if Steam.isSteamRunning():
	#       Steam.setRichPresence("steam_display", "#Status")
	#       Steam.setRichPresence("status", status)
	#       Steam.setRichPresence("chapter", str(current_chapter))
	# Reference: https://godotsteam.com/classes/friends/#setrichpresence
	print("[RichPresence] %s" % status)

## Build rich presence string for current exploration state.
func _get_exploration_presence() -> String:
	var ch_name = RICH_PRESENCE_CHAPTERS.get(current_chapter, "Unknown")
	return "Exploring: %s (Ch.%d)" % [ch_name, current_chapter]

## S58: Store page metadata — print game stats to console for store description copy.
## Call from debug console or add to a debug menu. Counts dialogue lines, endings, etc.
func print_store_stats() -> void:
	# Count dialogue lines across all chapter JSON files
	var total_lines: int = 0
	var dialogue_dir = "res://data/"
	for i in range(1, 12):
		var filename = "chapter%d_dialogue.json" if i <= 10 else "epilogue_dialogue.json"
		var path = dialogue_dir + (filename % i if i <= 10 else "epilogue_dialogue.json")
		if FileAccess.file_exists(path):
			var file = FileAccess.open(path, FileAccess.READ)
			if file:
				var json = JSON.new()
				if json.parse(file.get_as_text()) == OK and json.data is Dictionary:
					var dialogue_groups: Dictionary = json.data.get("dialogues", {})
					for dialogue in dialogue_groups.values():
						if dialogue is Array:
							total_lines += dialogue.size()
				file.close()

	var num_endings = ENDING_DATA.size()
	var num_achievements = AchievementManager.ACHIEVEMENTS.size() if AchievementManager.has_method("get") else 28
	var num_maps = 10  # 10 explorable maps
	var est_hours = "6-10"  # estimated play time range

	print("========== MEMORIA — Steam Store Stats ==========")
	print("  Dialogue lines:    ~%d" % total_lines)
	print("  Endings:           %d unique endings" % num_endings)
	print("  Achievements:      %d" % num_achievements)
	print("  Maps:              %d explorable areas" % num_maps)
	print("  Chapters:          10 story chapters + epilogue")
	print("  Estimated time:    %s hours (first playthrough)" % est_hours)
	print("  Side quests:       6")
	print("  Boss battles:      2+ (with Boss Rush mode)")
	print("  NG+ cycles:        Unlimited (NG++ with extra content)")
	print("  Languages:         English, Korean")
	print("=================================================")

func _ready() -> void:
	_load_locale_early()  # S55: Load locale before other autoloads build UI
	_load_seen_endings()
	_load_boss_rush_record()
	BattleManager.battle_cleanup_finished.connect(_on_battle_ended_for_boss_rush)
	# S55: 통계 — 전투/연소/적 격파 추적
	BattleManager.battle_started.connect(func(_e): add_stat("total_battles"))
	BattleManager.battle_ended.connect(_on_battle_ended_stats)
	BattleManager.combo_changed.connect(func(c): max_stat("highest_combo", c))
	# S58: Track rich presence on battle start (enemy name)
	BattleManager.battle_started.connect(func(e):
		if e:
			update_rich_presence("In Battle: %s" % e.name)
	)
	# S58: Initial rich presence + chapter tracking
	update_rich_presence("In Menu")
	mark_chapter_start()
	print("[GameManager] Initialized — MEMORIA v0.1.0 (seen endings: %d)" % seen_endings.size())

## S59: Screenshot mode — F12 to capture
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_F12 or event.physical_keycode == KEY_F12:
			_take_screenshot()
			get_viewport().set_input_as_handled()

func _take_screenshot() -> void:
	# Ensure screenshots directory exists
	var dir = DirAccess.open("user://")
	if dir and not dir.dir_exists("screenshots"):
		dir.make_dir("screenshots")

	# Capture the viewport
	var image = get_viewport().get_texture().get_image()
	if not image:
		return

	# Generate filename with timestamp
	var datetime = Time.get_datetime_dict_from_system()
	var filename = "screenshot_%04d%02d%02d_%02d%02d%02d.png" % [
		datetime.year, datetime.month, datetime.day,
		datetime.hour, datetime.minute, datetime.second
	]
	var path = "user://screenshots/" + filename
	image.save_png(path)

	# Show toast notification
	NotificationToast.show_toast("Screenshot saved: %s" % filename, NotificationToast.ToastType.SUCCESS)
	print("[GameManager] Screenshot saved: %s" % path)

func _process(delta: float) -> void:
	# S55: 플레이 타임 추적 (일시정지 아닐 때만)
	if current_state != GameState.PAUSED and current_state != GameState.MENU:
		play_stats.play_time_seconds += delta

func _on_battle_ended_stats(result: BattleManager.BattleState) -> void:
	if result == BattleManager.BattleState.VICTORY:
		add_stat("enemies_defeated")
		if BattleManager.current_enemy and BattleManager.current_enemy.is_boss:
			add_stat("bosses_defeated")

func _on_battle_ended_for_boss_rush(result: BattleManager.BattleState) -> void:
	if boss_rush_mode:
		on_boss_rush_battle_ended(result)

## NG+ 적 스케일링 계수 (HP/ATK에 곱함)
func get_ng_scale() -> float:
	return 1.0 + ng_plus_cycle * 0.3

## NG+ 해금 여부 (영구 파일 체크)
func is_ng_plus_unlocked() -> bool:
	return FileAccess.file_exists(NG_PLUS_FILE)

## NG+ 해금 기록 (크레딧 도달 시 호출)
func mark_game_completed() -> void:
	if FileAccess.file_exists(NG_PLUS_FILE):
		# 기존 데이터에 클리어 횟수 추가
		var file = FileAccess.open(NG_PLUS_FILE, FileAccess.READ)
		var json = JSON.new()
		if json.parse(file.get_as_text()) == OK and json.data is Dictionary:
			file.close()
			var data = json.data
			data["completions"] = data.get("completions", 0) + 1
			var wf = FileAccess.open(NG_PLUS_FILE, FileAccess.WRITE)
			if wf:
				wf.store_string(JSON.stringify(data, "\t"))
				wf.close()
			return
		file.close()
	var file = FileAccess.open(NG_PLUS_FILE, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify({"completions": 1}, "\t"))
		file.close()

## NG+ 시작 (아이템/Grains/회차 유지, 나머지 초기화)
func start_new_game_plus() -> void:
	var kept_grains = player_data.get("grains", 0)
	var kept_items = player_data.get("items", {}).duplicate()
	var prev_cycle = ng_plus_cycle

	# 스토리/기억 초기화
	story_flags.clear()
	current_chapter = 1
	ng_plus_cycle = prev_cycle + 1

	player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": kept_grains,
		"elia_with_party": true,
		"items": kept_items,
	}

	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._init_starting_memories()

	# S53: NG++ (cycle 2+) 보너스
	if ng_plus_cycle >= 2:
		player_data["max_hp"] = 120  # 시작 HP 증가
		player_data["hp"] = 120
		# NG++ 전용 장비 해금
		set_flag("ng_plus_equipment", true)
	if ng_plus_cycle >= 3:
		set_flag("ng_triple_plus", true)  # 최종 보스 변형 플래그

	AchievementManager.unlock("new_game_plus")
	print("[GameManager] New Game+ Cycle %d started (Grains: %d, Items: %d)" % [ng_plus_cycle, kept_grains, kept_items.size()])

## 상태 전환
func change_state(new_state: GameState) -> void:
	var old_state = current_state
	current_state = new_state
	state_changed.emit(new_state)
	# S58: Update rich presence on every state change
	match new_state:
		GameState.EXPLORATION:
			update_rich_presence(_get_exploration_presence())
		GameState.MENU:
			update_rich_presence("In Menu")
		GameState.DIALOGUE:
			update_rich_presence("In Dialogue (Ch.%d)" % current_chapter)
		GameState.CUTSCENE:
			update_rich_presence("Watching Cutscene (Ch.%d)" % current_chapter)
		GameState.PAUSED:
			pass  # Keep previous status during pause
	print("[GameManager] State: %s -> %s" % [GameState.keys()[old_state], GameState.keys()[new_state]])

## 스토리 플래그
func set_flag(flag_name: String, value: bool = true) -> void:
	story_flags[flag_name] = value
	print("[GameManager] Flag set: %s = %s" % [flag_name, value])

func get_flag(flag_name: String) -> bool:
	return story_flags.get(flag_name, false)

## 게임 일시정지
func pause_game() -> void:
	get_tree().paused = true
	change_state(GameState.PAUSED)

func unpause_game() -> void:
	get_tree().paused = false
	change_state(GameState.EXPLORATION)

## S58: Track chapter start stats for completion screen
func mark_chapter_start() -> void:
	_chapter_start_battles = play_stats.get("total_battles", 0)
	_chapter_start_burns = play_stats.get("total_burns", 0)
	_chapter_start_time = play_stats.get("play_time_seconds", 0.0)

## S58: Get chapter stats delta for completion screen
func get_chapter_stats() -> Dictionary:
	return {
		"battles": play_stats.get("total_battles", 0) - _chapter_start_battles,
		"burns": play_stats.get("total_burns", 0) - _chapter_start_burns,
		"time_seconds": play_stats.get("play_time_seconds", 0.0) - _chapter_start_time,
	}

## S58: Notify HP increase (called when max_hp changes)
func set_max_hp(new_max: int) -> void:
	var old_max = player_data.max_hp
	player_data.max_hp = new_max
	if new_max > old_max:
		stat_gained.emit("MAX HP", new_max - old_max)
	player_data.hp = mini(player_data.hp, player_data.max_hp)

## 세이브용 데이터 내보내기
func export_data() -> Dictionary:
	return {
		"player_data": player_data.duplicate(),
		"story_flags": story_flags.duplicate(),
		"current_chapter": current_chapter,
		"ng_plus_cycle": ng_plus_cycle,
		"equipped": equipped.duplicate(),  # S41
		"upgrade_levels": upgrade_levels.duplicate(),  # S53
		"seen_endings": seen_endings.duplicate(),  # S54
		"play_stats": play_stats.duplicate(),  # S55
		"current_locale": current_locale,  # S55
	}

## 세이브 데이터 불러오기
func import_data(data: Dictionary) -> void:
	if data.has("player_data"):
		player_data = data.player_data
	if data.has("story_flags"):
		story_flags = data.story_flags
	if data.has("current_chapter"):
		current_chapter = data.current_chapter
	if data.has("ng_plus_cycle"):
		ng_plus_cycle = int(data["ng_plus_cycle"])
	if data.has("equipped"):
		equipped = data.equipped  # S41
	if data.has("upgrade_levels"):
		upgrade_levels = data.upgrade_levels  # S53
	if data.has("seen_endings"):
		for e in data.seen_endings:
			if e not in seen_endings:
				seen_endings.append(e)
		_save_seen_endings()  # S54: persist endings across saves
	if data.has("play_stats") and data.play_stats is Dictionary:
		for key in play_stats.keys():
			if data.play_stats.has(key):
				play_stats[key] = data.play_stats[key]
	if data.has("current_locale"):
		current_locale = data.current_locale
