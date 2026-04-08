## AchievementManager (Autoload)
## 업적 시스템. 도전과제 추적 + 달성 시 토스트 알림.
## 영구 저장 (user://achievements.json)
extends Node

const SAVE_PATH: String = "user://achievements.json"

# ── 업적 정의 ──
const ACHIEVEMENTS: Dictionary = {
	# 전투 관련
	"first_blood": {"title": "First Blood", "desc": "Win your first battle.", "icon": "sword"},
	"void_slayer": {"title": "Void Slayer", "desc": "Defeat a Void Beast.", "icon": "skull"},
	"boss_hunter": {"title": "Boss Hunter", "desc": "Defeat a boss enemy.", "icon": "crown"},
	"battle_veteran": {"title": "Battle Veteran", "desc": "Win 10 battles.", "icon": "shield"},
	"survivor": {"title": "Survivor", "desc": "Win a battle with 10 HP or less.", "icon": "heart"},
	"item_master": {"title": "Item Master", "desc": "Use 10 items in battle.", "icon": "potion"},

	# 기억 관련
	"first_burn": {"title": "First Burn", "desc": "Burn your first memory.", "icon": "flame"},
	"pyromaniac": {"title": "Pyromaniac", "desc": "Burn 5 memories.", "icon": "flame"},
	"identity_crisis": {"title": "Identity Crisis", "desc": "Burn a Grade 2 (Identity) memory.", "icon": "flame"},
	"zero_burn": {"title": "Zero Burn", "desc": "Burn the Core memory — your name.", "icon": "skull"},

	# 탐색 관련
	"hidden_stump": {"title": "Old Growth", "desc": "Find the hidden stump in Rim Forest.", "icon": "eye"},
	"hidden_garden": {"title": "Secret Garden", "desc": "Find the hidden garden in The Seam.", "icon": "eye"},
	"explorer": {"title": "Explorer", "desc": "Visit all 5 maps.", "icon": "map"},

	# 스토리 관련
	"chapter_complete_1": {"title": "Rim Forest", "desc": "Complete Chapter 1.", "icon": "book"},
	"chapter_complete_2": {"title": "Verdan Market", "desc": "Complete Chapter 2.", "icon": "book"},
	"chapter_complete_3": {"title": "Crumbling Coast", "desc": "Complete Chapter 3.", "icon": "book"},
	"chapter_complete_5": {"title": "Into the Void", "desc": "Complete Chapter 5.", "icon": "book"},
	"ending_seal": {"title": "The Seal Holds", "desc": "Reach the Seal ending.", "icon": "star"},
	"ending_zero": {"title": "Nothing Remains", "desc": "Reach the Zero Burn ending.", "icon": "star"},
	"ending_ash": {"title": "Ash Ending", "desc": "Reach the Ash ending.", "icon": "star"},
	"ending_seam": {"title": "The Seam Holds", "desc": "Reach the Seam ending.", "icon": "star"},
	"all_endings": {"title": "Every Path", "desc": "See all 4 endings.", "icon": "crown"},

	# 경제 관련
	"merchant": {"title": "Merchant", "desc": "Complete a trade with Malet.", "icon": "coin"},
	"wealthy": {"title": "Wealthy", "desc": "Accumulate 100 Grains.", "icon": "coin"},

	# 사이드 퀘스트
	"all_quests": {"title": "Memory Hunter", "desc": "Complete all side quests.", "icon": "star"},

	# NG+
	"new_game_plus": {"title": "New Game+", "desc": "Start a New Game+ run.", "icon": "cycle"},
}

# ── 달성 데이터 ──
var unlocked: Dictionary = {}  # {id: true}
var stats: Dictionary = {      # 통계 카운터
	"battles_won": 0,
	"items_used": 0,
	"maps_visited": [],
}

signal achievement_unlocked(id: String)

func _ready() -> void:
	_load_data()
	_connect_signals()
	print("[AchievementManager] Ready — %d/%d unlocked" % [unlocked.size(), ACHIEVEMENTS.size()])

func _connect_signals() -> void:
	BattleManager.battle_ended.connect(_on_battle_ended)
	MemoryManager.memory_burned.connect(_on_memory_burned)

## 업적 해금
func unlock(id: String) -> void:
	if unlocked.has(id):
		return
	if not ACHIEVEMENTS.has(id):
		return
	unlocked[id] = true
	var ach = ACHIEVEMENTS[id]
	NotificationToast.show_toast("Achievement: %s" % ach["title"], NotificationToast.ToastType.SUCCESS)
	achievement_unlocked.emit(id)
	_save_data()
	print("[Achievement] Unlocked: %s" % ach["title"])

	# all_endings 체크
	if id.begins_with("ending_"):
		_check_all_endings()

func is_unlocked(id: String) -> bool:
	return unlocked.has(id)

## ===================== 이벤트 핸들러 =====================

func _on_battle_ended(result: BattleManager.BattleState) -> void:
	if result == BattleManager.BattleState.VICTORY:
		stats["battles_won"] += 1
		unlock("first_blood")
		if stats["battles_won"] >= 10:
			unlock("battle_veteran")
		# Void Beast / Boss 체크
		if BattleManager.current_enemy:
			if BattleManager.current_enemy.is_void_beast:
				unlock("void_slayer")
			if BattleManager.current_enemy.is_boss:
				unlock("boss_hunter")
		# 생존자 체크
		if GameManager.player_data.get("hp", 100) <= 10:
			unlock("survivor")
		_save_data()

func _on_memory_burned(memory) -> void:
	unlock("first_burn")
	if MemoryManager.get_burn_count() >= 5:
		unlock("pyromaniac")
	if memory.grade == MemoryManager.MemoryGrade.GRADE_2:
		unlock("identity_crisis")
	if memory.id == "core_name_origin":
		unlock("zero_burn")

## 맵 방문 기록 (맵 스크립트에서 호출)
func record_map_visit(map_name: String) -> void:
	if map_name not in stats["maps_visited"]:
		stats["maps_visited"].append(map_name)
		_save_data()
	if stats["maps_visited"].size() >= 5:
		unlock("explorer")

## 아이템 사용 기록 (BattleManager에서 호출)
func record_item_used() -> void:
	stats["items_used"] += 1
	if stats["items_used"] >= 10:
		unlock("item_master")
	_save_data()

## 챕터 완료 기록
func record_chapter_complete(chapter: int) -> void:
	var id = "chapter_complete_%d" % chapter
	if ACHIEVEMENTS.has(id):
		unlock(id)

## 엔딩 기록
func record_ending(ending_id: String) -> void:
	unlock(ending_id)

## Grains 체크
func check_grains() -> void:
	if GameManager.player_data.get("grains", 0) >= 100:
		unlock("wealthy")

func check_quest_complete() -> void:
	# 모든 사이드 퀘스트 완료 체크
	var all_done = true
	for q in SideQuest.QUESTS:
		if not SideQuest.is_complete(q["id"]):
			all_done = false
			break
	if all_done:
		unlock("all_quests")

func _check_all_endings() -> void:
	var endings = ["ending_seal", "ending_zero", "ending_ash", "ending_seam"]
	var all = true
	for e in endings:
		if not unlocked.has(e):
			all = false
			break
	if all:
		unlock("all_endings")

## ===================== 영구 저장 =====================

func _save_data() -> void:
	var data = {
		"unlocked": unlocked.duplicate(),
		"stats": stats.duplicate(true),
	}
	var file = FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(data, "\t"))
		file.close()

func _load_data() -> void:
	if not FileAccess.file_exists(SAVE_PATH):
		return
	var file = FileAccess.open(SAVE_PATH, FileAccess.READ)
	if file == null:
		return
	var json = JSON.new()
	if json.parse(file.get_as_text()) != OK:
		file.close()
		return
	file.close()
	var data = json.data
	if data is Dictionary:
		unlocked = data.get("unlocked", {})
		var loaded_stats = data.get("stats", {})
		for key in loaded_stats:
			stats[key] = loaded_stats[key]

## 업적 목록 반환 (UI용)
func get_all_achievements() -> Array[Dictionary]:
	var result: Array[Dictionary] = []
	for id in ACHIEVEMENTS:
		var ach = ACHIEVEMENTS[id].duplicate()
		ach["id"] = id
		ach["unlocked"] = unlocked.has(id)
		result.append(ach)
	return result
