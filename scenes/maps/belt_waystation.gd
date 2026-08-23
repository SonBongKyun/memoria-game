## Belt Waystation, 벨트 중간역 (Chapter 3: Weight of Pages)
## 버려진 무역로 '벨트' 위의 빈 중간역. 아렐과 엘리아가 백서를 발견한다.
## 남쪽에서 시작 → 역참을 조사한 뒤 동쪽으로 나가 Drift Shelter로 이동.
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
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,4],
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,4],
	[3,0,0,0,0,0,0,0,3,5,5,5,5,5,5,3,0,0,0,0,0,0,0,0,4],
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

func _ready() -> void:
	_retire_legacy_tobias_progression()
	_build_map()
	# S236: 대기 예산. 이 장소가 어떤 곳인지만 선언하고,
	# 여섯 겹 오버레이의 배분은 MapEffects.apply_atmosphere가 정한다.
	var atmosphere := {
		"hue": Color(0.45, 0.40, 0.30),
		"light": Color(0.76, 0.64, 0.44, 1.0),
		"mood": 0.38,
		"saturation": 1.0,
		"brightness": -0.03,
		"fog": "none",
		"splash": "res://assets/cg/generated/chapter_splash_belt_waystation.png",
	}
	MapEffects.apply_atmosphere(self, atmosphere)
	_fog_layer = MapEffects.apply_atmosphere_fog_layer(self, atmosphere)
	MapEffects.add_burn_desaturation(self)
	MapEffects.add_parallax_background(self, {"sky": Color(0.18, 0.17, 0.16), "far": Color(0.2, 0.19, 0.18), "mid": Color(0.22, 0.2, 0.18), "biome": "wasteland", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	# S236: 주변광은 대기 예산이 소유한다 (apply_atmosphere).
	# S52: 그래픽 업그레이드
	_s52_particles = MapEffects.add_pollen_particles(self, 15, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.45, 0.4, 0.35, 0.2))
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	# S59: 분위기 강화, 황무지 안개 + 깊이 그라디언트
	_position_player()
	if GameManager.get_flag("ch3_complete"):
		_setup_battle_triggers()
	_setup_exit_trigger()
	if GameManager.get_flag("ch3_complete"):
		_setup_interactive_objects()
	_setup_exploration_events()
	_setup_map_decorations()
	_setup_random_encounters()
	if GameManager.get_flag("ch3_complete"):
		WorldPopulation.populate(self, "belt_waystation")
	WorldAtlas.add_gateways(self, "belt_waystation")
	AchievementManager.record_map_visit("belt_waystation")
	elia.repeat_line = "The pages still feel warm. Let me carry them."
	print("[BeltWaystation] Map loaded, %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if not GameManager.get_flag("ch3_arrived"):
		await MapEffects.show_chapter_title(self, 3, "The Belt", "Weight of Pages")
		await get_tree().create_timer(0.3).timeout
		_start_ch3_sequence()
	elif not GameManager.get_flag("ch3_blank_book"):
		_start_blank_book_sequence()
	elif not GameManager.get_flag("ch3_waystation_night"):
		_start_waystation_night_sequence()
	elif not GameManager.get_flag("ch3_class_seven_message"):
		_start_class_seven_message_sequence()

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
		MapEffects.update_npc_idle_motion(npc, effect_time)

## ===================== 스토리 시퀀스 =====================

func _start_ch3_sequence() -> void:
	GameManager.set_flag("ch3_arrived")
	MemoryManager.add_chapter_memories(3)
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waystation_arrival")

func _on_arrival_ended() -> void:
	await get_tree().create_timer(1.0).timeout
	_start_blank_book_sequence()

func _start_blank_book_sequence() -> void:
	if GameManager.get_flag("ch3_blank_book"):
		return
	GameManager.set_flag("ch3_blank_book")
	DialogueManager.dialogue_ended.connect(_on_blank_book_found, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "blank_book_discovery")

func _on_blank_book_found() -> void:
	GameManager.set_flag("has_blank_book")
	NotificationToast.show_toast("Obtained: Blank Book", NotificationToast.ToastType.SUCCESS)
	await get_tree().create_timer(1.0).timeout
	_start_waystation_night_sequence()

func _start_waystation_night_sequence() -> void:
	if GameManager.get_flag("ch3_waystation_night"):
		return
	GameManager.set_flag("ch3_waystation_night")
	DialogueManager.dialogue_ended.connect(_on_waystation_night_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waystation_night")

func _on_waystation_night_ended() -> void:
	await get_tree().create_timer(1.0).timeout
	_start_class_seven_message_sequence()

func _start_class_seven_message_sequence() -> void:
	if GameManager.get_flag("ch3_class_seven_message"):
		return
	GameManager.set_flag("ch3_class_seven_message")
	DialogueManager.load_and_start(DIALOGUE_FILE, "class_seven_wall_message")

## ===================== 출구 트리거 (동쪽 → Drift Shelter) =====================

func _setup_exit_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(23.5 * TILE_SIZE, 9 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE, TILE_SIZE * 3)
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("ch3_class_seven_message") and not GameManager.get_flag("ch3_complete"):
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
	print("[BeltWaystation] Chapter 3 complete, heading to Drift Shelter")
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
		"A faded Bureau sign: 'RELAY STATION 14, All combustion events must be reported within 72 hours.'"
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
	# S209: 색깔 사각형 대신 절차적 픽셀아트 마커.
	# Clean Gameplay View에서 알파 0으로 완전히 사라지던 문제도 함께 해결한다.
	var indicator := MapEffects.make_discovery_marker("chest")
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
	# S209: 색깔 사각형 대신 절차적 픽셀아트 마커.
	# Clean Gameplay View에서 알파 0으로 완전히 사라지던 문제도 함께 해결한다.
	var indicator := MapEffects.make_discovery_marker("clue")
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
	if GameManager.get_flag("ch3_complete"):
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
	# 물탱크 (기울어진 원통, 중간역 상징)
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
	# The first canonical visit is abandoned. Revisit ambience stays available
	# only after Chapter 3 has been completed.
	if GameManager.get_flag("ch3_complete"):
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
	MapEffects.add_map_canvas(self, tilemap, "res://assets/environment/map_canvases/map_belt_waystation_canvas_v1.png", Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), {"terrain_alpha": 0.0})
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

## Compatibility for development saves created by the retired ten-chapter route.
## This touches only legacy story/party flags when the save enters canonical Ch3.
func _retire_legacy_tobias_progression() -> void:
	for flag_name in ["ch3_tobias_met", "ch3_tobias_records", "tobias_in_party", "tobias_joined"]:
		if GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name, false)
	BattleManager.tobias_in_party = false
	if GameManager.get_flag("ch3_kairos_writing") and not GameManager.get_flag("ch3_class_seven_message"):
		GameManager.set_flag("ch3_class_seven_message")
	if GameManager.get_flag("ch3_complete"):
		if not GameManager.get_flag("ch3_waystation_night"):
			GameManager.set_flag("ch3_waystation_night")
		if not GameManager.get_flag("ch3_class_seven_message"):
			GameManager.set_flag("ch3_class_seven_message")
