## Crumbling Coast — 크럼블링 코스트 (Chapter 3)
## 땅이 무너져 내리는 해안 절벽. 카이로스의 추적이 시작되는 곳.
## 남쪽에서 시작 → 북쪽 The Seam으로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter3_dialogue.json"

enum Tile { ROCK, SAND, CLIFF, WATER, PATH }

# 0=바위, 1=모래, 2=절벽(벽), 3=물, 4=길
var map_data: Array = [
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
	[2,0,0,0,4,4,4,0,0,2,3,3,3,3,3,2,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,4,0,0,0,2,3,3,3,3,3,2,0,0,1,1,0,0,0,0,2],
	[2,0,0,0,0,4,0,0,0,0,2,3,3,3,2,0,0,1,1,1,1,0,0,0,2],
	[2,2,0,0,0,4,0,0,0,0,0,2,2,2,0,0,0,0,1,1,0,0,0,2,2],
	[2,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,0,0,1,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,2],
	[2,0,0,1,1,0,0,4,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,2],
	[2,2,0,0,0,0,0,4,0,0,2,2,0,0,2,2,0,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,4,0,0,2,0,0,0,0,2,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,2],
	[2,0,0,1,1,0,0,0,4,0,0,0,0,0,0,0,4,4,0,0,1,1,0,0,2],
	[2,2,0,0,0,0,0,0,4,4,0,0,0,0,0,4,4,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,0,0,4,4,0,0,0,4,4,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,2,2,0,0,0,0,4,0,0,0,0,2,2,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
]

var tile_colors: Dictionary = {
	Tile.ROCK: Color(0.32, 0.3, 0.28),
	Tile.SAND: Color(0.45, 0.4, 0.32),
	Tile.CLIFF: Color(0.15, 0.13, 0.12),
	Tile.WATER: Color(0.1, 0.18, 0.3),
	Tile.PATH: Color(0.38, 0.35, 0.3),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia

func _ready() -> void:
	_build_map()
	_position_player()
	_setup_battle_triggers()
	_setup_seam_trigger()
	print("[CrumblingCoast] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	if not GameManager.get_flag("ch3_arrived"):
		await get_tree().create_timer(0.5).timeout
		_start_ch3_sequence()

## ===================== 스토리 시퀀스 =====================

func _start_ch3_sequence() -> void:
	GameManager.set_flag("ch3_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "coast_arrival")

func _on_arrival_ended() -> void:
	# 카이로스 목격 이벤트 (짧은 딜레이 후)
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch3_kairos_seen"):
		GameManager.set_flag("ch3_kairos_seen")
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_sighting")

## ===================== The Seam 도착 트리거 (북쪽) =====================

func _setup_seam_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(5 * TILE_SIZE, 1.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name == "Player" and not GameManager.get_flag("ch3_seam_arrived"):
			_arrive_at_seam()
	)
	add_child(area)

func _arrive_at_seam() -> void:
	GameManager.set_flag("ch3_seam_arrived")
	GameManager.set_flag("ch3_complete")
	DialogueManager.dialogue_ended.connect(_on_seam_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "seam_arrival")

func _on_seam_ended() -> void:
	GameManager.current_chapter = 4
	print("[CrumblingCoast] Chapter 3 complete — The Seam reached")

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

			# 물에 파도 디테일
			if tile_type == Tile.WATER:
				var wave = ColorRect.new()
				wave.size = Vector2(TILE_SIZE, 4)
				wave.position = pos + Vector2(0, randi_range(8, 24))
				wave.color = Color(0.15, 0.25, 0.4, 0.4)
				wave.z_index = 0
				add_child(wave)

			if tile_type == Tile.CLIFF or tile_type == Tile.WATER:
				_add_collision(pos)

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
	player.position = Vector2(12 * TILE_SIZE, 16 * TILE_SIZE)
	elia.position = Vector2(12 * TILE_SIZE - 30, 16 * TILE_SIZE + 20)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(
		Vector2(18 * TILE_SIZE, 7 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Coastal Void Beast", 100, 18, true,
		"res://assets/cg/crumbling_coast.jpg", "res://assets/cg/void_beast.jpg"
	)

func _add_battle_area(pos: Vector2, size: Vector2, enemy_name: String, hp: int, atk: int, is_void: bool, bg_img: String = "", e_img: String = "") -> void:
	var area = Area2D.new()
	area.position = pos + size / 2.0
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = size
	shape.shape = rect
	area.add_child(shape)

	var indicator = ColorRect.new()
	indicator.size = size
	indicator.position = -size / 2.0
	indicator.color = Color(0.3, 0.05, 0.3, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			BattleManager.start_battle(enemy, "res://scenes/maps/crumbling_coast.tscn", bg_img, e_img)
			SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)
