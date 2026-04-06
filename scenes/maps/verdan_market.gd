## Verdan Market — 베르단 시장 (Chapter 2)
## 회색 벨트 최대 도시. 말렛과의 거래가 이루어지는 곳.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 30
const MAP_HEIGHT: int = 20
const DIALOGUE_FILE: String = "res://data/chapter2_dialogue.json"

enum Tile { STONE, WALL, STALL, DOOR, ALLEY }

# 0=돌바닥, 1=건물벽, 2=노점, 3=문/입구, 4=골목
var map_data: Array = [
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
	[1,0,0,0,0,0,1,0,0,0,2,0,0,0,0,2,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,1,1,1,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,4,4,0,0,0,0,4,4,0,0,0,0,0,0,2,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,1,0,0,4,4,0,0,0,0,4,4,0,0,1,1,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,1,0,0,0,4,4,3,3,4,4,0,0,0,1,1,0,0,0,0,0,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
]

var tile_colors: Dictionary = {
	Tile.STONE: Color(0.28, 0.26, 0.25),
	Tile.WALL: Color(0.18, 0.15, 0.13),
	Tile.STALL: Color(0.35, 0.25, 0.18),
	Tile.DOOR: Color(0.4, 0.32, 0.22),
	Tile.ALLEY: Color(0.15, 0.13, 0.12),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	_position_player()
	print("[VerdenMarket] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	# Ch2 도착 대화 (첫 진입)
	if not GameManager.get_flag("ch2_arrived"):
		await get_tree().create_timer(0.5).timeout
		_start_ch2_sequence()

## ===================== 스토리 시퀀스 =====================

func _start_ch2_sequence() -> void:
	GameManager.set_flag("ch2_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "verdan_arrival")

func _on_arrival_ended() -> void:
	# 자유 탐색 — 말렛 대화 종료 감지
	DialogueManager.dialogue_ended.connect(_on_any_dialogue_ended)
	print("[VerdenMarket] Free exploration — talk to Malet in the Sump")

## 대화 종료 감지 — 말렛 거래 흐름 자동 연결
func _on_any_dialogue_ended() -> void:
	# malet_encounter 끝나면 → malet_deal 시작
	if GameManager.get_flag("malet_deal_accepted") or GameManager.get_flag("malet_deal_refused"):
		DialogueManager.dialogue_ended.disconnect(_on_any_dialogue_ended)
		await get_tree().create_timer(0.3).timeout
		if GameManager.get_flag("malet_deal_accepted"):
			# 거래 수락 → 추출 대화 → 보상
			DialogueManager.dialogue_ended.connect(_on_deal_ended, CONNECT_ONE_SHOT)
			DialogueManager.load_and_start(DIALOGUE_FILE, "malet_deal")
		else:
			# 거절
			DialogueManager.dialogue_ended.connect(_on_refused_ended, CONNECT_ONE_SHOT)
			DialogueManager.load_and_start(DIALOGUE_FILE, "malet_refused")

func _on_deal_ended() -> void:
	await get_tree().create_timer(0.5).timeout
	DialogueManager.dialogue_ended.connect(_on_reward_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "malet_reward")

func _on_refused_ended() -> void:
	# 거절해도 재대화로 다시 시도 가능 — 플래그 초기화
	GameManager.story_flags.erase("malet_deal_refused")
	DialogueManager.dialogue_ended.connect(_on_any_dialogue_ended)

func _on_reward_ended() -> void:
	GameManager.set_flag("ch2_malet_done")
	GameManager.set_flag("ch2_complete")
	GameManager.current_chapter = 3
	print("[VerdenMarket] Chapter 2 complete — transitioning to Crumbling Coast")
	await get_tree().create_timer(1.5).timeout
	SceneTransition.change_scene("res://scenes/maps/crumbling_coast.tscn")

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	var tile_defs = [
		{"color": Color(0.28, 0.26, 0.25), "detail": "stone"},   # 0: STONE
		{"color": Color(0.18, 0.15, 0.13), "detail": "wall"},    # 1: WALL
		{"color": Color(0.35, 0.25, 0.18), "detail": "stall"},   # 2: STALL
		{"color": Color(0.4, 0.32, 0.22), "detail": "door"},     # 3: DOOR
		{"color": Color(0.15, 0.13, 0.12), "detail": "alley"},   # 4: ALLEY
	]
	var tilemap = TilePainter.create_tilemap(tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL])
	for body in bodies:
		add_child(body)

func _position_player() -> void:
	player.position = Vector2(4 * TILE_SIZE, 9 * TILE_SIZE)
	elia.position = Vector2(4 * TILE_SIZE - 30, 9 * TILE_SIZE + 20)
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}
