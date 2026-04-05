## GameManager (Autoload)
## 전체 게임 상태 관리. 씬 간 데이터 유지.
extends Node

# --- 게임 상태 ---
enum GameState { EXPLORATION, DIALOGUE, BATTLE, CUTSCENE, MENU, PAUSED }
var current_state: GameState = GameState.EXPLORATION

# --- 챕터 진행 ---
var current_chapter: int = 1
var story_flags: Dictionary = {}  # "met_elia": true, "malet_deal": false 등

# --- 플레이어 데이터 ---
var player_data: Dictionary = {
	"name": "Arrel",
	"hp": 100,
	"max_hp": 100,
	"grains": 0,  # 화폐
	"elia_with_party": true,  # 엘리아 동행 여부
}

signal state_changed(new_state: GameState)

func _ready() -> void:
	print("[GameManager] Initialized — MEMORIA v0.1.0")

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
	}

## 세이브 데이터 불러오기
func import_data(data: Dictionary) -> void:
	if data.has("player_data"):
		player_data = data.player_data
	if data.has("story_flags"):
		story_flags = data.story_flags
	if data.has("current_chapter"):
		current_chapter = data.current_chapter
