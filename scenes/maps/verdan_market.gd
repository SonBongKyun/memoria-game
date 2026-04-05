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
	# 거절해도 재대화로 다시 시도 가능
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
	for y in range(MAP_HEIGHT):
		for x in range(MAP_WIDTH):
			var tile_type = map_data[y][x] as Tile
			var pos = Vector2(x * TILE_SIZE, y * TILE_SIZE)

			var rect = ColorRect.new()
			rect.size = Vector2(TILE_SIZE, TILE_SIZE)
			rect.position = pos
			rect.color = tile_colors[tile_type]
			rect.z_index = -1
			add_child(rect)

			if tile_type == Tile.STALL:
				_add_stall_detail(pos)

			if tile_type == Tile.WALL:
				_add_collision(pos)

func _add_stall_detail(pos: Vector2) -> void:
	# 노점 천막
	var canopy = ColorRect.new()
	canopy.size = Vector2(28, 10)
	canopy.position = pos + Vector2(2, 0)
	canopy.color = Color(0.45, 0.3, 0.2)
	canopy.z_index = 1
	add_child(canopy)

func _add_collision(pos: Vector2) -> void:
	var body = StaticBody2D.new()
	body.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	body.collision_layer = 1

	var shape = CollisionShape2D.new()
	var rect_shape = RectangleShape2D.new()
	rect_shape.size = Vector2(TILE_SIZE, TILE_SIZE)
	shape.shape = rect_shape
	body.add_child(shape)
	add_child(body)

func _position_player() -> void:
	player.position = Vector2(4 * TILE_SIZE, 9 * TILE_SIZE)
	elia.position = Vector2(4 * TILE_SIZE - 30, 9 * TILE_SIZE + 20)
