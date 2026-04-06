## BL-07 Void Interior — 보이드 홀 내부 (Chapter 5)
## 현실이 무너져 내리는 공간. The Seal 결정이 이루어지는 곳.
## 보이드의 심장부까지 진행 → Grade 1 기억 연소 결정.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 20
const MAP_HEIGHT: int = 20
const DIALOGUE_FILE: String = "res://data/chapter5_dialogue.json"

enum Tile { VOID, FRAGMENT, PATH, CRACK, CORE }

# 0=허공, 1=부유 파편, 2=길, 3=균열(벽), 4=핵심부
var map_data: Array = [
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
	[3,0,0,0,0,0,3,0,0,0,0,0,0,3,0,0,0,0,0,3],
	[3,0,0,1,0,0,3,0,0,1,1,0,0,3,0,0,1,0,0,3],
	[3,0,1,1,2,2,0,0,1,1,1,1,0,0,2,2,1,1,0,3],
	[3,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,0,3],
	[3,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,0,3],
	[3,3,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,3,3],
	[3,0,0,0,0,0,2,0,0,1,1,0,0,2,0,0,0,0,0,3],
	[3,0,1,0,0,0,2,0,1,0,0,1,0,2,0,0,0,1,0,3],
	[3,0,0,0,0,0,2,2,0,0,0,0,2,2,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,2,0,0,0,0,2,0,0,0,0,0,0,3],
	[3,0,0,1,0,0,0,2,0,0,0,0,2,0,0,0,1,0,0,3],
	[3,3,0,0,0,0,0,2,2,0,0,2,2,0,0,0,0,0,3,3],
	[3,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,3],
	[3,0,0,0,1,0,0,0,2,0,0,2,0,0,0,1,0,0,0,3],
	[3,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,3],
	[3,0,1,0,0,0,0,0,0,4,4,0,0,0,0,0,0,1,0,3],
	[3,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
]

var tile_colors: Dictionary = {
	Tile.VOID: Color(0.02, 0.02, 0.05),
	Tile.FRAGMENT: Color(0.12, 0.1, 0.15),
	Tile.PATH: Color(0.08, 0.06, 0.12),
	Tile.CRACK: Color(0.05, 0.03, 0.08),
	Tile.CORE: Color(0.2, 0.05, 0.3),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia

var pulse_time: float = 0.0
var core_rects: Array = []

var void_particles: GPUParticles2D

func _ready() -> void:
	_build_map()
	_position_player()
	_setup_core_trigger()
	_setup_battle_triggers()
	void_particles = MapEffects.add_void_particles(self)
	void_particles.position = Vector2(MAP_WIDTH * TILE_SIZE / 2.0, MAP_HEIGHT * TILE_SIZE / 2.0)
	print("[BL07Void] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	MemoryManager.add_chapter_memories(5)

	if not GameManager.get_flag("ch5_void_entered"):
		await get_tree().create_timer(0.5).timeout
		_start_ch5_sequence()

func _process(delta: float) -> void:
	# 코어 맥동 효과
	pulse_time += delta
	var pulse = (sin(pulse_time * 2.0) + 1.0) * 0.5  # 0~1
	for rect in core_rects:
		if is_instance_valid(rect):
			rect.color = Color(
				0.2 + pulse * 0.15,
				0.02 + pulse * 0.03,
				0.3 + pulse * 0.1,
			)

## ===================== 스토리 시퀀스 =====================

func _start_ch5_sequence() -> void:
	GameManager.set_flag("ch5_void_entered")
	DialogueManager.dialogue_ended.connect(_on_entry_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "void_entry")

func _on_entry_ended() -> void:
	print("[BL07Void] Free exploration — reach the core")

## ===================== 핵심부 트리거 =====================

func _setup_core_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(9.5 * TILE_SIZE, 17 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("ch5_void_entered") and not GameManager.get_flag("ch5_core_reached"):
			_reach_core()
	)
	add_child(area)

func _reach_core() -> void:
	GameManager.set_flag("ch5_core_reached")
	DialogueManager.dialogue_ended.connect(_on_core_dialogue_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "void_core")

func _on_core_dialogue_ended() -> void:
	# The Seal 결정 — 선택지 대화
	DialogueManager.dialogue_ended.connect(_on_seal_decision_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "seal_decision")

func _on_seal_decision_ended() -> void:
	if GameManager.get_flag("seal_accepted"):
		# Grade 1 기억 연소 — The Seal
		_execute_seal()
	else:
		# 거부 — 퇴각
		_refuse_seal()

func _execute_seal() -> void:
	# 씬 암전 연출 — 이름을 태우는 순간
	AudioManager.play_sfx("void_pulse")
	await get_tree().create_timer(0.5).timeout

	# core_name_origin(Grade 1) 연소
	var memory = MemoryManager.burn_memory("core_name_origin")
	if memory:
		GameManager.set_flag("zero_burn_path")
		print("[BL07Void] ZERO BURN — Grade 1 memory burned")

	# 화면 플래시 — 백색 → 복귀
	SceneTransition.transition_rect.color = Color.WHITE
	SceneTransition.transition_rect.modulate.a = 1.0
	await get_tree().create_timer(1.2).timeout
	SceneTransition.transition_rect.color = Color.BLACK
	var flash_tween = create_tween()
	flash_tween.tween_property(SceneTransition.transition_rect, "modulate:a", 0.0, 1.5)
	await flash_tween.finished

	DialogueManager.dialogue_ended.connect(_on_seal_complete, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "seal_complete")

func _refuse_seal() -> void:
	GameManager.set_flag("seal_refused")
	AudioManager.play_sfx("void_pulse")
	await get_tree().create_timer(0.3).timeout
	DialogueManager.dialogue_ended.connect(_on_refuse_complete, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "seal_refused")

func _on_seal_complete() -> void:
	GameManager.set_flag("ch5_complete")
	GameManager.current_chapter = 6
	print("[BL07Void] Chapter 5 complete — The Seal executed (Zero Burn path)")
	await get_tree().create_timer(2.0).timeout
	SceneTransition.change_scene("res://scenes/maps/the_seam.tscn")

func _on_refuse_complete() -> void:
	GameManager.set_flag("ch5_complete")
	GameManager.current_chapter = 6
	print("[BL07Void] Chapter 5 complete — The Seal refused (Preservation path)")
	await get_tree().create_timer(2.0).timeout
	SceneTransition.change_scene("res://scenes/maps/the_seam.tscn")

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	# Void Fragment — 보이드 내부 떠다니는 파편 적
	_add_battle_area(
		Vector2(3 * TILE_SIZE, 8 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Fragment", 70, 16, true
	)
	# Memory Eater — 기억을 먹는 존재
	_add_battle_area(
		Vector2(15 * TILE_SIZE, 11 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Memory Eater", 90, 20, true
	)

var _battle_counter: int = 0

func _add_battle_area(pos: Vector2, size: Vector2, enemy_name: String, hp: int, atk: int, is_void: bool) -> void:
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
	indicator.color = Color(0.25, 0.0, 0.35, 0.25)
	indicator.z_index = -1
	area.add_child(indicator)

	_battle_counter += 1
	var flag_name = "battle_bl07_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			if enemy_name == "Memory Eater":
				enemy.abilities = ["drain", "multi_hit"]
			BattleManager.start_battle(enemy, "res://scenes/maps/bl07_void.tscn", "res://assets/cg/bl07_interior.jpg", "res://assets/cg/void_portal.jpg")
			SceneTransition.change_scene("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	var tile_defs = [
		{"color": Color(0.02, 0.02, 0.05), "detail": "void"},       # 0: VOID
		{"color": Color(0.12, 0.1, 0.15), "detail": "fragment"},    # 1: FRAGMENT
		{"color": Color(0.08, 0.06, 0.12), "detail": "path"},       # 2: PATH
		{"color": Color(0.05, 0.03, 0.08), "detail": "crack"},      # 3: CRACK
		{"color": Color(0.2, 0.05, 0.3), "detail": "core"},         # 4: CORE
	]
	var tilemap = TilePainter.create_tilemap(tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	# 충돌 (균열만)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.CRACK])
	for body in bodies:
		add_child(body)

	# 핵심부 맥동용 — TileMap 위에 ColorRect 오버레이
	for y in range(MAP_HEIGHT):
		for x in range(MAP_WIDTH):
			if map_data[y][x] == Tile.CORE:
				var rect = ColorRect.new()
				rect.size = Vector2(TILE_SIZE, TILE_SIZE)
				rect.position = Vector2(x * TILE_SIZE, y * TILE_SIZE)
				rect.color = Color(0.2, 0.05, 0.3)
				rect.z_index = 0
				add_child(rect)
				core_rects.append(rect)

func _position_player() -> void:
	# 북쪽 입구
	player.position = Vector2(4 * TILE_SIZE, 3 * TILE_SIZE)
	elia.position = Vector2(4 * TILE_SIZE - 25, 3 * TILE_SIZE + 15)
