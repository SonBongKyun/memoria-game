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

signal state_changed(new_state: GameState)

func _ready() -> void:
	print("[GameManager] Initialized — MEMORIA v0.1.0")

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

	AchievementManager.unlock("new_game_plus")
	print("[GameManager] New Game+ Cycle %d started (Grains: %d, Items: %d)" % [ng_plus_cycle, kept_grains, kept_items.size()])

## 상태 전환
func change_state(new_state: GameState) -> void:
	var old_state = current_state
	current_state = new_state
	state_changed.emit(new_state)
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

## 세이브용 데이터 내보내기
func export_data() -> Dictionary:
	return {
		"player_data": player_data.duplicate(),
		"story_flags": story_flags.duplicate(),
		"current_chapter": current_chapter,
		"ng_plus_cycle": ng_plus_cycle,
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
