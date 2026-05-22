## Belt Waystation — 벨트 중간역 (Chapter 3: Weight of Pages)
## 버려진 무역로 '벨트' 위의 관리국 중간역. 토비아스와의 만남 + 백서 획득.
## 남쪽에서 시작 → 북쪽으로 진행하면 Drift Shelter로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter3_dialogue.json"

enum Tile { DEAD_SOIL, CRACKED_ROAD, RUIN, WALL, PATH, INTERIOR }

# 0=죽은 토양(회색), 1=갈라진 도로, 2=폐허(잔해), 3=벽, 4=길, 5=건물 내부
var map_data: Array = [
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
	[3,0,0,0,0,0,0,0,0,0,0,4,4,4,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,2,2,0,0,0,0,0,0,0,4,0,0,0,0,0,2,2,0,0,0,0,3],
	[3,0,0,2,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,2,0,0,0,0,3],
	[3,0,0,0,0,0,1,1,1,1,1,1,4,1,1,1,1,1,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,1,0,0,0,0,0,4,0,0,0,0,1,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,3,3,3,4,4,3,3,3,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,1,1,1,1,1,4,4,1,1,1,1,1,0,0,0,0,0,0,3],
	[3,0,0,2,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,2,0,0,0,3],
	[3,0,0,2,2,0,0,0,0,0,0,0,4,0,0,0,0,0,0,2,2,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
]

var _tile_defs: Array = []
var _minimap_data: Dictionary = {}
var _encounter_data: RandomEncounter.EncounterData = null
var _point_lights: Array[PointLight2D] = []
var effect_time: float = 0.0
var _occluders: Array[LightOccluder2D] = []  # S52
var _s52_particles: Array[ColorRect] = []  # S52
var _camera: Camera2D = null  # S52
var _fog_layer: Array[ColorRect] = []  # S59

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia
@onready var tobias_npc: StaticBody2D = $Tobias

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	MapEffects.add_burn_desaturation(self)
	MapEffects.add_parallax_background(self, {"sky": Color(0.18, 0.17, 0.16), "far": Color(0.2, 0.19, 0.18), "mid": Color(0.22, 0.2, 0.18), "biome": "wasteland", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.45, 0.42, 0.4))
	# S52: 그래픽 업그레이드
	MapEffects.add_color_grading(self, {"tint": Color(0.45, 0.4, 0.3), "brightness": -0.03})
	MapEffects.add_illustration_atmosphere(self, "res://assets/cg/game_image/env_wasteland_city.png", 0.13, Color(0.92, 0.84, 0.68))
	_s52_particles = MapEffects.add_pollen_particles(self, 15, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.45, 0.4, 0.35, 0.2))
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	# S59: 분위기 강화 — 황무지 안개 + 깊이 그라디언트
	_fog_layer = MapEffects.add_fog_layer(self, 0.4, Color(0.35, 0.32, 0.3, 0.05), 2.0)
	MapEffects.add_depth_gradient(self, 0.06)
	_position_player()
	_setup_battle_triggers()
	_setup_exit_trigger()
	_setup_interactive_objects()
	_setup_exploration_events()
	_setup_map_decorations()
	_setup_random_encounters()
	AchievementManager.record_map_visit("belt_waystation")
	elia.repeat_line = "This place... it's like the land itself forgot how to live."
	tobias_npc.repeat_line = "Fascinating. Absolutely fascinating. Let me write that down."
	# 토비아스는 만나기 전에는 숨김
	if not GameManager.get_flag("ch3_tobias_met"):
		tobias_npc.visible = false
		tobias_npc.set_physics_process(false)
	# S54: NPC Schedule — adjust tobias based on chapter
	_apply_npc_schedules()
	print("[BeltWaystation] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if not GameManager.get_flag("ch3_arrived"):
		await MapEffects.show_chapter_title(self, 3, "The Belt", "Weight of Pages")
		await get_tree().create_timer(0.3).timeout
		_start_ch3_sequence()

func _process(delta: float) -> void:
	effect_time += delta
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
	MapEffects.update_pollen(_s52_particles, effect_time, delta)
	MapEffects.update_camera_shake(_camera, effect_time)
	# S59: 안개 + 트리거 글로우
	MapEffects.update_fog_layer(_fog_layer, effect_time)
	MapEffects.update_trigger_approach_glow(self, player.position, effect_time)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# S53: NPC 아이들 모션
	for npc in get_tree().get_nodes_in_group("npcs"):
		if npc.has_node("AnimatedSprite2D"):
			var spr = npc.get_node("AnimatedSprite2D")
			spr.scale = Vector2(1.0 + sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.008, 1.0 - sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.006)

## ===================== 스토리 시퀀스 =====================

func _start_ch3_sequence() -> void:
	GameManager.set_flag("ch3_arrived")
	MemoryManager.add_chapter_memories(3)
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waystation_arrival")

func _on_arrival_ended() -> void:
	# 웨이스테이션 도착 후 → 토비아스 만남
	await get_tree().create_timer(1.5).timeout
	if not GameManager.get_flag("ch3_tobias_met"):
		GameManager.set_flag("ch3_tobias_met")
		tobias_npc.visible = true
		tobias_npc.set_physics_process(true)
		DialogueManager.dialogue_ended.connect(_on_tobias_met, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "tobias_encounter")

func _on_tobias_met() -> void:
	# 토비아스 만남 후 → 백서 발견
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch3_blank_book"):
		GameManager.set_flag("ch3_blank_book")
		DialogueManager.dialogue_ended.connect(_on_blank_book_found, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "blank_book_discovery")

func _on_blank_book_found() -> void:
	# 백서 획득 → 카이로스 벽 낙서 발견
	GameManager.set_flag("has_blank_book")
	NotificationToast.show_toast("Obtained: Blank Book", NotificationToast.ToastType.SUCCESS)
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch3_kairos_writing"):
		GameManager.set_flag("ch3_kairos_writing")
		DialogueManager.dialogue_ended.connect(_on_writing_found, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_wall_writing")

func _on_writing_found() -> void:
	# 낙서 발견 → 토비아스 합류
	await get_tree().create_timer(1.5).timeout
	DialogueManager.dialogue_ended.connect(_on_tobias_joined, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "tobias_joins")

func _on_tobias_joined() -> void:
	GameManager.set_flag("tobias_in_party")
	GameManager.set_flag("tobias_joined", true)
	NotificationToast.show_toast("Tobias joined the party", NotificationToast.ToastType.SUCCESS)
	# 저널 등록
	StoryJournal.add_event("tobias_joined", "Met Tobias Crane, a Bureau Recorder, at the Belt waystation. He carries twenty years of memory transaction records.")
	StoryJournal.add_npc("tobias", "Tobias Crane — Bureau Recorder, Class C. Meticulous, curious, and surprisingly brave for a bureaucrat.")

## ===================== 출구 트리거 (북쪽 → Drift Shelter) =====================

func _setup_exit_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(12 * TILE_SIZE, 1.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("tobias_in_party") and not GameManager.get_flag("ch3_complete"):
			_depart_waystation()
	)
	add_child(area)

func _depart_waystation() -> void:
	GameManager.set_flag("ch3_complete")
	AchievementManager.record_chapter_complete(3)
	DialogueManager.dialogue_ended.connect(_on_departure_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waystation_departure")

func _on_departure_ended() -> void:
	GameManager.current_chapter = 4
	SaveManager.autosave_on_chapter_transition()
	print("[BeltWaystation] Chapter 3 complete — heading to Drift Shelter")
	await get_tree().create_timer(1.5).timeout
	# S58: Chapter completion screen with stats summary
	SceneTransition.change_scene_chapter_complete("res://scenes/maps/drift_shelter.tscn", 3)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(
		Vector2(4 * TILE_SIZE, 13 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Belt Scavenger", 55, 12, false
	)
	_add_battle_area(
		Vector2(18 * TILE_SIZE, 5 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Wisp", 45, 14, true
	)

var _battle_counter: int = 0

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
	_battle_counter += 1
	var flag_name = "battle_belt_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			if is_void:
				enemy.abilities = ["drain"]
			else:
				enemy.abilities = ["weaken"]
			BattleManager.start_battle(enemy, "res://scenes/maps/belt_waystation.tscn", bg_img, e_img)
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 랜덤 인카운터 =====================

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch3_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Belt Scavenger", "hp": 55, "atk": 12, "is_void": false, "abilities": ["weaken"]},
			{"name": "Void Wisp", "hp": 45, "atk": 14, "is_void": true, "abilities": ["drain"]},
			{"name": "Dust Crawler", "hp": 40, "atk": 10, "is_void": false, "abilities": ["poison"]},
		],
		"res://scenes/maps/belt_waystation.tscn", "", "", 50, 90
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	_add_chest(
		Vector2(4 * TILE_SIZE, 4 * TILE_SIZE),
		"chest_belt_ruin1",
		{"items": {"potion": 2}, "grains": 10}
	)
	_add_chest(
		Vector2(20 * TILE_SIZE, 3 * TILE_SIZE),
		"chest_belt_ruin2",
		{"items": {"antidote": 1, "firebomb": 1}, "grains": 8}
	)
	_add_clue(
		Vector2(14 * TILE_SIZE, 6 * TILE_SIZE),
		"clue_belt_sign",
		"A faded Bureau sign: 'RELAY STATION 14 — All combustion events must be reported within 72 hours.'"
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
	_add_story_trigger(Vector2(3 * TILE_SIZE, 12 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "belt_atmosphere", "ch3_belt_walk")
	_add_story_trigger(Vector2(19 * TILE_SIZE, 13 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "tobias_records", "ch3_tobias_records")
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "belt_waystation")

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

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 물탱크 (기울어진 원통 — 중간역 상징)
	var tank = ColorRect.new()
	tank.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 3)
	tank.position = Vector2(20 * TILE_SIZE, 7 * TILE_SIZE)
	tank.color = Color(0.25, 0.22, 0.2, 0.4)
	tank.rotation = 0.1
	tank.z_index = -1
	add_child(tank)
	# 갈라진 도로 표시 (벨트)
	for i in range(3):
		var crack = ColorRect.new()
		crack.size = Vector2(2, TILE_SIZE * 0.6)
		crack.position = Vector2((11 + i) * TILE_SIZE + 14, 5 * TILE_SIZE + 8)
		crack.color = Color(0.1, 0.08, 0.07, 0.3)
		crack.z_index = -1
		add_child(crack)
	# S55: 중간역 배경 NPC (여행자/관리국 요원)
	var ambient_npcs = [
		{"pos": Vector2(8, 6), "preset": "traveler"},
		{"pos": Vector2(14, 8), "preset": "bureau_agent"},
		{"pos": Vector2(18, 5), "preset": "guard"},
	]
	for npc_data in ambient_npcs:
		var npc_sprite = PixelSprite.create_npc_sprite(npc_data["preset"])
		npc_sprite.position = npc_data["pos"] * TILE_SIZE + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
		npc_sprite.z_index = 1
		add_child(npc_sprite)

	# 먼지 파티클
	MapEffects.add_void_particles(self, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, Color(0.4, 0.38, 0.35, 0.15), 15)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.25, 0.23, 0.21), "detail": "dead_soil"},     # 0: DEAD_SOIL
		{"color": Color(0.3, 0.28, 0.25), "detail": "road"},           # 1: CRACKED_ROAD
		{"color": Color(0.18, 0.16, 0.14), "detail": "rock"},          # 2: RUIN
		{"color": Color(0.12, 0.1, 0.09), "detail": "cliff"},          # 3: WALL
		{"color": Color(0.35, 0.32, 0.28), "detail": "path"},          # 4: PATH
		{"color": Color(0.2, 0.18, 0.16), "detail": "stone_floor"},    # 5: INTERIOR
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL, Tile.RUIN])
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

## S54: NPC Schedule System — adjust tobias based on current chapter
func _apply_npc_schedules() -> void:
	var ch = GameManager.current_chapter
	var tobias_sched = GameManager.get_npc_schedule("tobias", ch)
	if not tobias_sched.is_empty():
		var is_vis: bool = tobias_sched.get("visible", true)
		# Ch4-6: tobias is traveling with party, not at waystation
		if not is_vis and GameManager.get_flag("ch3_tobias_met"):
			tobias_npc.visible = false
			tobias_npc.set_physics_process(false)
		elif is_vis and GameManager.get_flag("ch3_tobias_met"):
			tobias_npc.visible = true
			tobias_npc.set_physics_process(true)
			var tile_pos: Vector2 = tobias_sched.get("pos", Vector2(11, 9))
			tobias_npc.position = Vector2(tile_pos.x * TILE_SIZE, tile_pos.y * TILE_SIZE)
		# Update repeat line based on chapter
		if ch >= 7:
			tobias_npc.repeat_line = "I've been cross-referencing the Bureau records. The patterns are... troubling."
		elif ch >= 4:
			tobias_npc.repeat_line = ""  # not visible anyway
