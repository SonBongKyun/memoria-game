## Forgotten Forest — 잊힌 숲 (Chapter 8: The Forest That Forgets)
## 기억을 먹는 숲. 유령 NPC, 기억 침식 환경.
## 북쪽으로 진행하면 무색 황무지로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter8_dialogue.json"

enum Tile { DEAD_SOIL, TRUNK, CANOPY, HOLLOW, PATH, ROOT, CAIRN }

# 0=땅, 1=나무줄기(벽), 2=수관(벽, 어두움), 3=빈 공간(공터), 4=길, 5=뿌리(장애), 6=돌무더기(안전지대)
var map_data: Array = [
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
	[2,1,0,0,0,1,0,4,4,4,0,0,1,0,0,0,0,4,4,0,0,1,0,0,2],
	[2,0,0,1,0,0,0,0,4,0,0,1,1,0,0,0,0,4,0,0,1,0,0,0,2],
	[2,0,1,1,0,0,5,0,4,0,0,0,0,0,5,0,0,4,0,0,0,0,1,0,2],
	[2,0,0,0,0,3,3,0,4,0,0,0,0,3,3,0,0,4,0,0,0,0,0,0,2],
	[2,1,0,0,3,3,6,3,4,0,0,1,0,3,0,0,0,4,0,0,5,0,1,0,2],
	[2,0,0,0,0,3,3,0,4,4,0,0,0,0,0,0,0,4,4,0,0,0,0,0,2],
	[2,0,1,0,0,0,0,0,0,4,0,0,0,0,0,1,0,0,4,0,0,1,0,0,2],
	[2,0,0,0,5,0,0,0,0,4,0,0,1,0,0,0,0,0,4,0,0,0,0,0,2],
	[2,0,0,0,0,0,1,0,0,4,4,0,0,0,0,0,1,0,4,0,0,0,0,0,2],
	[2,0,0,1,0,0,0,0,0,0,4,0,0,0,0,0,0,0,4,4,0,0,1,0,2],
	[2,0,0,0,0,0,0,5,0,0,4,0,0,1,0,5,0,0,0,4,0,0,0,0,2],
	[2,1,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,4,0,0,0,1,2],
	[2,0,0,0,1,0,0,0,0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0,2],
	[2,0,0,0,0,0,5,0,0,0,0,4,4,0,0,0,5,0,4,4,0,0,0,0,2],
	[2,0,0,1,0,0,0,0,0,0,0,0,4,4,0,0,0,4,4,0,0,0,1,0,2],
	[2,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,2],
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
]

var _tile_defs: Array = []
var _minimap_data: Dictionary = {}
var _encounter_data: RandomEncounter.EncounterData = null
var effect_time: float = 0.0
var _occluders: Array[LightOccluder2D] = []  # S52
var _s52_particles: Array[ColorRect] = []  # S52
var _camera: Camera2D = null  # S52

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia
@onready var sable_npc: StaticBody2D = $Sable

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	MapEffects.add_burn_desaturation(self)
	MapEffects.add_parallax_background(self, {"sky": Color(0.05, 0.06, 0.04), "far": Color(0.08, 0.1, 0.06), "mid": Color(0.1, 0.12, 0.08), "biome": "dead_forest", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.25, 0.28, 0.22))
	MapEffects.add_fog(self, 0.7, Color(0.15, 0.18, 0.12, 0.3))
	# S52: 그래픽 업그레이드
	MapEffects.add_color_grading(self, {"tint": Color(0.15, 0.2, 0.12), "brightness": -0.08})
	_s52_particles = MapEffects.add_pollen_particles(self, 18, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.4, 0.5, 0.2, 0.2))
	_camera = MapEffects.setup_smooth_camera(player, 1.0, 0.3)
	MapEffects.add_drop_shadow(player)
	_position_player()
	_setup_battle_triggers()
	_setup_exit_trigger()
	_setup_interactive_objects()
	_setup_exploration_events()
	_setup_random_encounters()
	MemoryManager.add_chapter_memories(8)
	AchievementManager.record_map_visit("forgotten_forest")
	elia.repeat_line = "Say your name. Don't forget it."
	sable_npc.repeat_line = "Keep moving. Don't look at the shapes between the trees."
	print("[ForgottenForest] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if not GameManager.get_flag("ch8_arrived"):
		await MapEffects.show_chapter_title(self, 8, "The Forest That Forgets", "Memory-parasitic ecosystem")
		await get_tree().create_timer(0.3).timeout
		_start_ch8_sequence()

func _process(delta: float) -> void:
	effect_time += delta
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
	MapEffects.update_pollen(_s52_particles, effect_time, delta)
	MapEffects.update_camera_shake(_camera, effect_time)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# S53: NPC 아이들 모션
	for npc in get_tree().get_nodes_in_group("npcs"):
		if npc.has_node("AnimatedSprite2D"):
			var spr = npc.get_node("AnimatedSprite2D")
			spr.scale = Vector2(1.0 + sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.008, 1.0 - sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.006)

## ===================== 스토리 시퀀스 =====================

func _start_ch8_sequence() -> void:
	GameManager.set_flag("ch8_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "forest_arrival")

func _on_arrival_ended() -> void:
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch8_ghost"):
		GameManager.set_flag("ch8_ghost")
		DialogueManager.dialogue_ended.connect(_on_ghost_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "ghost_encounter")

func _on_ghost_ended() -> void:
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch8_tobias_theory"):
		GameManager.set_flag("ch8_tobias_theory")
		DialogueManager.load_and_start(DIALOGUE_FILE, "tobias_theory")

## ===================== 출구 트리거 (북쪽 → Colorless Waste) =====================

func _setup_exit_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(10 * TILE_SIZE, 1.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 4, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("ch8_arrived") and not GameManager.get_flag("ch8_complete"):
			_depart_forest()
	)
	add_child(area)

func _depart_forest() -> void:
	GameManager.set_flag("ch8_complete")
	AchievementManager.record_chapter_complete(8)
	DialogueManager.dialogue_ended.connect(_on_departure_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "forest_departure")

func _on_departure_ended() -> void:
	GameManager.current_chapter = 9
	SaveManager.autosave_on_chapter_transition()
	print("[ForgottenForest] Chapter 8 complete — entering Colorless Waste")
	await get_tree().create_timer(1.5).timeout
	# S58: Chapter completion screen with stats summary
	SceneTransition.change_scene_chapter_complete("res://scenes/maps/colorless_waste.tscn", 8)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(Vector2(3 * TILE_SIZE, 4 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Memory Leech", 80, 18, true, ["drain", "poison"])
	_add_battle_area(Vector2(18 * TILE_SIZE, 8 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Hollow Walker", 95, 22, false, ["stun", "charge"])
	_add_battle_area(Vector2(8 * TILE_SIZE, 12 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Root Shade", 70, 16, true, ["reflect", "weaken"])

var _battle_counter: int = 0

func _add_battle_area(pos: Vector2, size: Vector2, enemy_name: String, hp: int, atk: int, is_void: bool, abilities: Array = []) -> void:
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
	indicator.color = Color(0.15, 0.2, 0.1, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)
	_battle_counter += 1
	var flag_name = "battle_forest_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			enemy.abilities = abilities
			BattleManager.start_battle(enemy, "res://scenes/maps/forgotten_forest.tscn", "", "")
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 랜덤 인카운터 =====================

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch8_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Memory Leech", "hp": 80, "atk": 18, "is_void": true, "abilities": ["drain", "poison"]},
			{"name": "Hollow Walker", "hp": 95, "atk": 22, "is_void": false, "abilities": ["stun", "charge"]},
			{"name": "Root Shade", "hp": 70, "atk": 16, "is_void": true, "abilities": ["reflect", "weaken"]},
		],
		"res://scenes/maps/forgotten_forest.tscn", "", "", 30, 60
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	_add_chest(Vector2(5 * TILE_SIZE, 5 * TILE_SIZE), "chest_forest_cairn", {"items": {"hi_potion": 2, "antidote": 2}, "grains": 20})
	_add_chest(Vector2(20 * TILE_SIZE, 3 * TILE_SIZE), "chest_forest_canopy", {"items": {"firebomb": 2, "smoke_bomb": 1}, "grains": 10})
	_add_clue(Vector2(14 * TILE_SIZE, 6 * TILE_SIZE), "clue_forest_roots", "The roots pulse faintly. Not with sap — with residue. These trees are digesting someone's childhood.")
	_add_clue(Vector2(6 * TILE_SIZE, 13 * TILE_SIZE), "clue_forest_carving", "Scratches in the bark. Someone tried to carve their name. The letters stop halfway — they forgot the rest.")

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
	indicator.color = Color(0.6, 0.5, 0.2, 0.2)
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
	indicator.color = Color(0.2, 0.3, 0.6, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			NotificationToast.show_toast(clue_text, NotificationToast.ToastType.INFO)
			indicator.queue_free()
	)
	add_child(area)

## ===================== 탐색 이벤트 =====================

func _setup_exploration_events() -> void:
	_add_story_trigger(Vector2(12 * TILE_SIZE, 4 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "forest_atmosphere", "ch8_atmosphere")
	_add_story_trigger(Vector2(4 * TILE_SIZE, 10 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "elia_anchor_strain", "ch8_anchor")
	_add_story_trigger(Vector2(16 * TILE_SIZE, 12 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "ghost_child", "ch8_ghost_child")
	_add_story_trigger(Vector2(10 * TILE_SIZE, 8 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "forest_whispers", "ch8_whispers")
	# 플래시백: 엘리아와의 첫 만남
	_add_story_trigger(Vector2(14 * TILE_SIZE, 6 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "arrel_flashback_elia", "ch8_flashback_elia")
	# Side quest: The Parasite's Root
	if SideQuest.is_active("forest_parasite"):
		_add_story_trigger(Vector2(14 * TILE_SIZE, 10 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "forest_whispers", "sq_parasite_found")
		_add_story_trigger(Vector2(6 * TILE_SIZE, 8 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "forest_whispers", "sq_parasite_burned")
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "forgotten_forest")

func _add_story_trigger(pos: Vector2, size: Vector2, dialogue_key: String, flag_name: String) -> void:
	if GameManager.get_flag(flag_name):
		return
	var area = Area2D.new()
	area.position = pos + size / 2.0
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = size
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			DialogueManager.load_and_start(DIALOGUE_FILE, dialogue_key)
	)
	add_child(area)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.12, 0.14, 0.1), "detail": "dead_soil"},    # 0: DEAD_SOIL
		{"color": Color(0.08, 0.06, 0.05), "detail": "cliff"},        # 1: TRUNK
		{"color": Color(0.05, 0.06, 0.04), "detail": "cliff"},        # 2: CANOPY
		{"color": Color(0.16, 0.18, 0.14), "detail": "path"},         # 3: HOLLOW
		{"color": Color(0.2, 0.18, 0.15), "detail": "path"},          # 4: PATH
		{"color": Color(0.1, 0.08, 0.06), "detail": "rock"},          # 5: ROOT
		{"color": Color(0.4, 0.38, 0.35), "detail": "rock"},          # 6: CAIRN
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.TRUNK, Tile.CANOPY, Tile.ROOT])
	for body in bodies:
		add_child(body)
	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(12 * TILE_SIZE, 16 * TILE_SIZE)
	elia.position = Vector2(12 * TILE_SIZE - 30, 16 * TILE_SIZE + 20)
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}
