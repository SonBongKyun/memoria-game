## Rim Exterior Forest — 림 외곽 숲 (Chapter 1 시작 맵)
## 아렐이 공허수를 처치한 직후. 재비가 내리는 숲.
## 스토리 시퀀스: opening → elia → ash_rain → 자유탐색 → camp_night
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter1_dialogue.json"

enum Tile { GRASS, PATH, TREE, BUSH, WATER }

var map_data: Array = [
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
	[2,2,2,0,0,0,2,2,0,0,0,0,0,0,0,0,0,2,2,0,0,0,2,2,2],
	[2,2,0,0,0,0,0,2,0,0,3,0,0,0,3,0,0,2,0,0,0,0,0,2,2],
	[2,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,0,0,3,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,3,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,2],
	[2,2,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,2],
	[2,0,0,0,3,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,3,0,0,0,2],
	[2,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,2],
	[2,2,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,2],
	[2,0,0,3,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,3,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,2,0,0,0,0,0,2,0,0,0,0,1,0,0,0,0,2,0,0,0,0,2,2,2],
	[2,2,2,0,0,0,2,2,0,0,0,0,1,0,0,0,0,2,2,0,0,0,2,2,2],
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
]

var tile_colors: Dictionary = {
	Tile.GRASS: Color(0.18, 0.28, 0.15),
	Tile.PATH: Color(0.35, 0.28, 0.2),
	Tile.TREE: Color(0.08, 0.12, 0.08),
	Tile.BUSH: Color(0.22, 0.32, 0.18),
	Tile.WATER: Color(0.12, 0.15, 0.25),
}

var tile_nodes: Array = []
var collision_bodies: Array = []
var ash_rain_node = null  # 재비 파티클

@onready var player: CharacterBody2D = $Player

func _ready() -> void:
	_build_map()
	_position_player()
	_setup_battle_triggers()
	_setup_camp_trigger()
	print("[RimForest] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	# 스토리 시퀀스 시작 (첫 진입 시만)
	if not GameManager.get_flag("ch1_opening_done"):
		# 짧은 딜레이 후 오프닝 대화 시작
		await get_tree().create_timer(0.5).timeout
		_start_story_sequence()

## ===================== 스토리 시퀀스 =====================

func _start_story_sequence() -> void:
	# 1단계: 오프닝 대화
	DialogueManager.dialogue_ended.connect(_on_opening_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "opening_void_beast")

func _on_opening_ended() -> void:
	GameManager.set_flag("ch1_opening_done")
	# 2단계: 엘리아 등장
	await get_tree().create_timer(0.8).timeout
	DialogueManager.dialogue_ended.connect(_on_elia_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "elia_appears")

func _on_elia_ended() -> void:
	GameManager.set_flag("ch1_elia_appeared")
	# 3단계: 재비 시작 + 대화
	await get_tree().create_timer(0.5).timeout
	_start_ash_rain()
	DialogueManager.dialogue_ended.connect(_on_ash_rain_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "ash_rain")

func _on_ash_rain_ended() -> void:
	GameManager.set_flag("ch1_ash_rain_seen")
	# 자유 탐색 모드 — 엘리아 재대화 가능, 전투 트리거 활성
	print("[RimForest] Free exploration — head south for camp")

## ===================== 재비 파티클 =====================

func _start_ash_rain() -> void:
	if ash_rain_node:
		return
	var AshRainScript = load("res://scripts/effects/ash_rain.gd")
	ash_rain_node = GPUParticles2D.new()
	ash_rain_node.set_script(AshRainScript)
	player.add_child(ash_rain_node)

## ===================== 야영 트리거 (남쪽 끝) =====================

func _setup_camp_trigger() -> void:
	# 남쪽 길 끝 (타일 12,16 근처)
	var area = Area2D.new()
	area.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 16 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("ch1_ash_rain_seen") and not GameManager.get_flag("ch1_camp_done"):
			_start_camp_scene()
	)

	add_child(area)

func _start_camp_scene() -> void:
	GameManager.set_flag("ch1_camp_done")
	DialogueManager.dialogue_ended.connect(_on_camp_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "camp_night")

func _on_camp_ended() -> void:
	GameManager.set_flag("ch1_complete")
	GameManager.current_chapter = 2
	print("[RimForest] Chapter 1 complete")
	# 히든 엔딩 CG — 녹색 나무 (짧게 보여주고 전환)
	await get_tree().create_timer(1.0).timeout
	CgViewer.show_cg("res://assets/cg/ch1_green_tree.jpg", "", 3.0, func():
		SceneTransition.change_scene("res://scenes/maps/verdan_market.tscn")
	)

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
			tile_nodes.append(rect)

			if tile_type == Tile.TREE:
				_add_tree_detail(rect, pos)
			elif tile_type == Tile.BUSH:
				_add_bush_detail(rect, pos)

			if tile_type == Tile.TREE or tile_type == Tile.WATER:
				_add_collision(pos)

func _add_tree_detail(_parent_rect: ColorRect, pos: Vector2) -> void:
	var trunk = ColorRect.new()
	trunk.size = Vector2(6, 10)
	trunk.position = pos + Vector2(13, 20)
	trunk.color = Color(0.25, 0.18, 0.1)
	trunk.z_index = 0
	add_child(trunk)

	var canopy = ColorRect.new()
	canopy.size = Vector2(22, 18)
	canopy.position = pos + Vector2(5, 2)
	canopy.color = Color(0.12, 0.2, 0.1)
	canopy.z_index = 1
	add_child(canopy)

func _add_bush_detail(_parent_rect: ColorRect, pos: Vector2) -> void:
	var detail = ColorRect.new()
	detail.size = Vector2(18, 12)
	detail.position = pos + Vector2(7, 10)
	detail.color = Color(0.15, 0.25, 0.12)
	detail.z_index = 0
	add_child(detail)

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
	collision_bodies.append(body)

func _position_player() -> void:
	player.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 9 * TILE_SIZE + TILE_SIZE / 2.0)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(
		Vector2(8 * TILE_SIZE, 5 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Ash Crawler", 40, 8, false,
		"res://assets/cg/ch1_forest.jpg", "res://assets/cg/ash_crawler.jpg"
	)

	_add_battle_area(
		Vector2(16 * TILE_SIZE, 7 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Beast", 80, 15, true,
		"res://assets/cg/ch1_forest.jpg", "res://assets/cg/void_beast.jpg"
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
	indicator.color = Color(0.5, 0.1, 0.1, 0.15) if not is_void else Color(0.3, 0.05, 0.3, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			_trigger_battle(enemy_name, hp, atk, is_void, bg_img, e_img)
	)

	add_child(area)

func _trigger_battle(enemy_name: String, hp: int, atk: int, is_void: bool, bg_img: String = "", e_img: String = "") -> void:
	var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
	BattleManager.start_battle(enemy, "res://scenes/maps/rim_forest.tscn", bg_img, e_img)
	SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn")
