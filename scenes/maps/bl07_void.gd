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
var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null

var void_particles: GPUParticles2D
var heavy_fog: Array[ColorRect] = []
var _memory_shards: Array[ColorRect] = []
var _point_lights: Array[PointLight2D] = []  # S42

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self, 0.6)  # 보이드: 강한 비네트
	MapEffects.add_burn_desaturation(self)  # S46: 기억 연소 월드 탈색
	# S42: 패럴랙스 + 조명
	MapEffects.add_parallax_background(self, {"sky": Color(0.01, 0.01, 0.03), "far": Color(0.05, 0.02, 0.08), "mid": Color(0.08, 0.04, 0.12), "biome": "void", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.3, 0.25, 0.4))
	# 코어와 파편에 보이드 라이트
	for y in range(MAP_HEIGHT):
		for x in range(MAP_WIDTH):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == Tile.CORE:
					_point_lights.append(MapEffects.add_point_light(self, Vector2(x * TILE_SIZE + 16, y * TILE_SIZE + 16), Color(0.6, 0.2, 0.8), 1.2, 128.0))
				elif map_data[y][x] == Tile.FRAGMENT and randi() % 3 == 0:
					_point_lights.append(MapEffects.add_point_light(self, Vector2(x * TILE_SIZE + 16, y * TILE_SIZE + 16), Color(0.4, 0.15, 0.6), 0.3, 48.0))
	_position_player()
	_setup_core_trigger()
	_setup_battle_triggers()
	void_particles = MapEffects.add_void_particles(self)
	void_particles.position = Vector2(MAP_WIDTH * TILE_SIZE / 2.0, MAP_HEIGHT * TILE_SIZE / 2.0)
	heavy_fog = MapEffects.add_heavy_fog(self, Color(0.15, 0.08, 0.2, 0.1))
	_setup_random_encounters()
	_setup_interactive_objects()
	_setup_map_decorations()
	AchievementManager.record_map_visit("bl07_void")
	print("[BL07Void] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	MemoryManager.add_chapter_memories(5)

	if not GameManager.get_flag("ch5_void_entered"):
		# 챕터 타이틀 카드 표시 후 대화 시작
		await MapEffects.show_chapter_title(self, 5, "BL-07", "The Void stares back")
		await get_tree().create_timer(0.3).timeout
		_start_ch5_sequence()

func _process(delta: float) -> void:
	MapEffects.update_heavy_fog(heavy_fog, pulse_time)
	MapEffects.update_point_lights(_point_lights, pulse_time)
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
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia.position, elia.visible)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# 기억 파편 부유
	for s in _memory_shards:
		var phase = s.get_meta("phase", 0.0)
		s.position.y = s.get_meta("base_y") + sin(pulse_time * 1.2 + phase) * 6.0
		s.color.a = 0.1 + sin(pulse_time * 2.0 + phase) * 0.06

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 떠다니는 기억 파편 (보이드 타일 위)
	var shard_positions = [
		Vector2(6, 5), Vector2(12, 3), Vector2(18, 7), Vector2(8, 12), Vector2(16, 14),
	]
	var shard_colors = [
		Color(0.9, 0.8, 0.5, 0.12),
		Color(0.7, 0.6, 0.9, 0.1),
		Color(0.5, 0.8, 0.7, 0.1),
		Color(0.9, 0.5, 0.6, 0.1),
		Color(0.8, 0.9, 0.5, 0.12),
	]
	for i in range(shard_positions.size()):
		var pos = shard_positions[i]
		var shard = ColorRect.new()
		shard.size = Vector2(4, 4)
		var base_y = pos.y * TILE_SIZE + randf_range(4, 20)
		shard.position = Vector2(pos.x * TILE_SIZE + randf_range(4, 20), base_y)
		shard.color = shard_colors[i]
		shard.z_index = -1
		shard.set_meta("phase", float(i) * 1.7)
		shard.set_meta("base_y", base_y)
		add_child(shard)
		_memory_shards.append(shard)
	# 보이드 균열 (바닥에 가느다란 보라색 선)
	for crack_pos in [Vector2(9, 8), Vector2(15, 11), Vector2(5, 15)]:
		var crack = ColorRect.new()
		crack.size = Vector2(TILE_SIZE * 2.5, 2)
		crack.position = crack_pos * TILE_SIZE + Vector2(0, TILE_SIZE / 2.0)
		crack.color = Color(0.3, 0.1, 0.4, 0.25)
		crack.rotation = randf_range(-0.2, 0.2)
		crack.z_index = -1
		add_child(crack)

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
	AchievementManager.record_chapter_complete(5)
	print("[BL07Void] Chapter 5 complete — The Seal executed (Zero Burn path)")
	await get_tree().create_timer(2.0).timeout
	SceneTransition.change_scene("res://scenes/maps/the_seam.tscn")

func _on_refuse_complete() -> void:
	GameManager.set_flag("ch5_complete")
	GameManager.current_chapter = 6
	AchievementManager.record_chapter_complete(5)
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
				enemy.weakness = "fire"
				enemy.resistance = "void"
			BattleManager.start_battle(enemy, "res://scenes/maps/bl07_void.tscn", "res://assets/cg/bl07_interior.jpg", "res://assets/cg/void_portal.jpg")
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

func _setup_random_encounters() -> void:
	# Ch5 진입 후 (보이드 내부는 항상 위험)
	if not GameManager.get_flag("ch5_void_entered"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Void Fragment", "hp": 75, "atk": 16, "is_void": true, "abilities": ["burn_attack"], "bg": "res://assets/cg/bl07_interior.jpg", "img": "res://assets/cg/void_portal.jpg"},
			{"name": "Memory Eater", "hp": 95, "atk": 20, "is_void": true, "abilities": ["drain", "multi_hit", "weaken"], "bg": "res://assets/cg/bl07_interior.jpg", "img": "res://assets/cg/void_portal.jpg"},
			{"name": "Null Wisp", "hp": 60, "atk": 22, "is_void": true, "abilities": ["poison", "burn_attack"], "bg": "res://assets/cg/bl07_interior.jpg"},
		],
		"res://scenes/maps/bl07_void.tscn", "", "", 30, 55
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	# 보이드 상자 — 파편 영역 (좌측 파편 근처, 희귀 아이템)
	_add_chest(
		Vector2(16 * TILE_SIZE, 2 * TILE_SIZE),
		"chest_void_fragment",
		{"items": {"hi_potion": 2, "firebomb": 1}, "grains": 25}
	)
	# 단서 — 핵심부 근처 (보이드 본질)
	_add_clue(
		Vector2(5 * TILE_SIZE, 9 * TILE_SIZE),
		"clue_void_whisper",
		"A sound that isn't a sound. The void remembers what you've forgotten."
	)

func _add_chest(pos: Vector2, flag_name: String, rewards: Dictionary) -> void:
	if GameManager.get_flag(flag_name):
		return
	var area = Area2D.new()
	area.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)
	var indicator = ColorRect.new()
	indicator.size = Vector2(TILE_SIZE, TILE_SIZE)
	indicator.position = -Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	indicator.color = Color(0.5, 0.3, 0.6, 0.25)
	indicator.z_index = -1
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			AudioManager.play_sfx("ui_select")
			if rewards.has("grains"):
				GameManager.player_data.grains += rewards["grains"]
				NotificationToast.show_toast("+%d Grains" % rewards["grains"], NotificationToast.ToastType.SUCCESS)
			if rewards.has("items"):
				for item_id in rewards["items"]:
					GameManager.add_item(item_id, rewards["items"][item_id])
			indicator.queue_free()
	)
	add_child(area)

func _add_clue(pos: Vector2, flag_name: String, clue_text: String) -> void:
	if GameManager.get_flag(flag_name):
		return
	var area = Area2D.new()
	area.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)
	var indicator = ColorRect.new()
	indicator.size = Vector2(TILE_SIZE, TILE_SIZE)
	indicator.position = -Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	indicator.color = Color(0.2, 0.2, 0.5, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			NotificationToast.show_toast(clue_text, NotificationToast.ToastType.INFO)
			indicator.queue_free()
	)
	add_child(area)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.02, 0.02, 0.05), "detail": "void"},       # 0: VOID
		{"color": Color(0.12, 0.1, 0.15), "detail": "fragment"},    # 1: FRAGMENT
		{"color": Color(0.08, 0.06, 0.12), "detail": "path"},       # 2: PATH
		{"color": Color(0.05, 0.03, 0.08), "detail": "crack"},      # 3: CRACK
		{"color": Color(0.2, 0.05, 0.3), "detail": "core"},         # 4: CORE
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	# 충돌 (균열만)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.CRACK])
	for body in bodies:
		add_child(body)

	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

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
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-25, 15)
		SaveManager.loaded_player_pos = {}
