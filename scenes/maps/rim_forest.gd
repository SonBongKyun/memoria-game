## Rim Exterior Forest — 림 외곽 숲 (Chapter 1 시작 맵)
## 아렐이 공허수를 처치한 직후. 재비가 내리는 숲.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25   # 타일 수
const MAP_HEIGHT: int = 18

# 타일 타입
enum Tile { GRASS, PATH, TREE, BUSH, WATER }

# 맵 레이아웃 (0=풀, 1=길, 2=나무, 3=덤불, 4=물)
# 둘레는 나무로 막고, 가운데에 길과 열린 공간
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

# 타일 색상
var tile_colors: Dictionary = {
	Tile.GRASS: Color(0.18, 0.28, 0.15),   # 어두운 초록 (회색 톤 숲)
	Tile.PATH: Color(0.35, 0.28, 0.2),     # 갈색 길
	Tile.TREE: Color(0.08, 0.12, 0.08),    # 진한 초록 (나무/벽)
	Tile.BUSH: Color(0.22, 0.32, 0.18),    # 중간 초록 (덤불)
	Tile.WATER: Color(0.12, 0.15, 0.25),   # 어두운 파랑 (물)
}

var tile_nodes: Array = []  # 타일 ColorRect 노드 저장
var collision_bodies: Array = []  # 충돌 StaticBody2D

@onready var player: CharacterBody2D = $Player

func _ready() -> void:
	_build_map()
	_position_player()
	_setup_battle_triggers()
	print("[RimForest] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

## 맵 구축 — 타일을 ColorRect로 배치하고, 나무/물에 충돌 추가
func _build_map() -> void:
	for y in range(MAP_HEIGHT):
		for x in range(MAP_WIDTH):
			var tile_type = map_data[y][x] as Tile
			var pos = Vector2(x * TILE_SIZE, y * TILE_SIZE)

			# 타일 시각 (ColorRect)
			var rect = ColorRect.new()
			rect.size = Vector2(TILE_SIZE, TILE_SIZE)
			rect.position = pos
			rect.color = tile_colors[tile_type]
			rect.z_index = -1
			add_child(rect)
			tile_nodes.append(rect)

			# 나무 타일에 약간의 시각적 변화
			if tile_type == Tile.TREE:
				_add_tree_detail(rect, pos)
			elif tile_type == Tile.BUSH:
				_add_bush_detail(rect, pos)

			# 충돌 (나무, 물)
			if tile_type == Tile.TREE or tile_type == Tile.WATER:
				_add_collision(pos)

## 나무 타일에 시각적 디테일 추가
func _add_tree_detail(parent_rect: ColorRect, pos: Vector2) -> void:
	# 나무 줄기 (갈색 작은 사각형)
	var trunk = ColorRect.new()
	trunk.size = Vector2(6, 10)
	trunk.position = pos + Vector2(13, 20)
	trunk.color = Color(0.25, 0.18, 0.1)
	trunk.z_index = 0
	add_child(trunk)

	# 나무 관 (더 밝은 초록 원형 느낌)
	var canopy = ColorRect.new()
	canopy.size = Vector2(22, 18)
	canopy.position = pos + Vector2(5, 2)
	canopy.color = Color(0.12, 0.2, 0.1)
	canopy.z_index = 1
	add_child(canopy)

## 덤불에 시각적 디테일
func _add_bush_detail(parent_rect: ColorRect, pos: Vector2) -> void:
	var detail = ColorRect.new()
	detail.size = Vector2(18, 12)
	detail.position = pos + Vector2(7, 10)
	detail.color = Color(0.15, 0.25, 0.12)
	detail.z_index = 0
	add_child(detail)

## 충돌 바디 추가
func _add_collision(pos: Vector2) -> void:
	var body = StaticBody2D.new()
	body.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	body.collision_layer = 1  # World 레이어

	var shape = CollisionShape2D.new()
	var rect_shape = RectangleShape2D.new()
	rect_shape.size = Vector2(TILE_SIZE, TILE_SIZE)
	shape.shape = rect_shape
	body.add_child(shape)

	add_child(body)
	collision_bodies.append(body)

## 플레이어를 맵 중앙 길 위에 배치
func _position_player() -> void:
	# 맵 중앙 근처 (길 위)
	player.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 9 * TILE_SIZE + TILE_SIZE / 2.0)

## 전투 트리거 영역 — 맵 남쪽 길에 일반 몬스터, 북쪽에 공허수
func _setup_battle_triggers() -> void:
	# 일반 몬스터 (남쪽 길 — 타일 12,14 근처)
	_add_battle_area(
		Vector2(12 * TILE_SIZE, 14 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Ash Crawler", 40, 8, false
	)

	# 공허수 (북쪽 숲 — 타일 12,4 근처)
	_add_battle_area(
		Vector2(12 * TILE_SIZE, 4 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Beast", 80, 15, true
	)

func _add_battle_area(pos: Vector2, size: Vector2, enemy_name: String, hp: int, atk: int, is_void: bool) -> void:
	var area = Area2D.new()
	area.position = pos + size / 2.0
	area.collision_layer = 0
	area.collision_mask = 2  # Player 레이어

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = size
	shape.shape = rect
	area.add_child(shape)

	# 시각적 표시 (위험 지역 — 붉은 반투명)
	var indicator = ColorRect.new()
	indicator.size = size
	indicator.position = -size / 2.0
	indicator.color = Color(0.5, 0.1, 0.1, 0.15) if not is_void else Color(0.3, 0.05, 0.3, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			_trigger_battle(enemy_name, hp, atk, is_void)
	)

	add_child(area)

func _trigger_battle(enemy_name: String, hp: int, atk: int, is_void: bool) -> void:
	var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
	BattleManager.start_battle(enemy, "res://scenes/maps/rim_forest.tscn")
	SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn")
