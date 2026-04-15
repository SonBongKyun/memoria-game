## Colorless Waste — 무색 황무지 (Chapter 9: Where Colors Stop)
## 색이 사라진 BL-07 경계. 카이로스 대면 + 메모리 나침반 획득.
## 북쪽으로 진행하면 BL-07 내부(Ch10)로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter9_dialogue.json"

enum Tile { FLAT, RIDGE, PILLAR, WALL, PATH, MARKER }

# 0=평지, 1=능선(벽), 2=잔해 기둥(벽), 3=벽, 4=길, 5=깊이 표지(장식)
var map_data: Array = [
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
	[3,0,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,1,1,0,0,0,0,0,4,0,0,0,0,1,1,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,5,0,0,0,4,0,0,5,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,2,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,2,2,0,0,0,0,3],
	[3,0,0,2,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,2,2,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,4,0,0,0,0,2,2,0,0,0,0,0,0,3],
	[3,0,0,0,0,5,0,0,0,0,4,0,0,0,0,0,2,0,0,0,5,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,1,1,0,0,0,0,4,0,0,0,0,1,1,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
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
	# 완전 탈색 — 무색 황무지
	MapEffects.add_parallax_background(self, {"sky": Color(0.12, 0.12, 0.12), "far": Color(0.15, 0.15, 0.15), "mid": Color(0.18, 0.18, 0.18), "biome": "waste", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.35, 0.35, 0.35))
	MapEffects.add_void_particles(self, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, Color(0.3, 0.3, 0.3, 0.08), 30)
	# S52: 그래픽 업그레이드
	MapEffects.add_color_grading(self, {"tint": Color(0.35, 0.35, 0.35), "brightness": -0.05})
	_s52_particles = MapEffects.add_pollen_particles(self, 20, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.35, 0.35, 0.35, 0.2))
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	_position_player()
	_setup_battle_triggers()
	_setup_exit_trigger()
	_setup_interactive_objects()
	_setup_exploration_events()
	_setup_random_encounters()
	MemoryManager.add_chapter_memories(9)
	AchievementManager.record_map_visit("colorless_waste")
	elia.repeat_line = "Hold my hand. Don't let go."
	sable_npc.repeat_line = "Follow the pull. It's the only direction left."
	print("[ColorlessWaste] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if not GameManager.get_flag("ch9_arrived"):
		await MapEffects.show_chapter_title(self, 9, "Where Colors Stop", "The achromatic zone")
		await get_tree().create_timer(0.3).timeout
		_start_ch9_sequence()

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

func _start_ch9_sequence() -> void:
	GameManager.set_flag("ch9_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waste_arrival")

func _on_arrival_ended() -> void:
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch9_compass"):
		GameManager.set_flag("ch9_compass")
		NotificationToast.show_toast("Obtained: Memory Compass", NotificationToast.ToastType.SUCCESS)
		DialogueManager.dialogue_ended.connect(_on_compass_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "compass_discovery")

func _on_compass_ended() -> void:
	await get_tree().create_timer(2.5).timeout
	# 카이로스 대면
	if not GameManager.get_flag("ch9_kairos"):
		GameManager.set_flag("ch9_kairos")
		DialogueManager.dialogue_ended.connect(_on_kairos_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_confrontation")

func _on_kairos_ended() -> void:
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch9_kairos_truth"):
		GameManager.set_flag("ch9_kairos_truth")
		DialogueManager.dialogue_ended.connect(_on_kairos_truth_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_truth")

func _on_kairos_truth_ended() -> void:
	if not GameManager.get_flag("ch9_kairos_battle"):
		GameManager.set_flag("ch9_kairos_battle", true)
		# Kairos Boss Fight
		var kairos = BattleManager.Enemy.new(
			"Kairos, Authority Editor", 450, 38, true, true,
			["void_pulse", "drain", "stun", "reflect", "charge", "despair"]
		)
		kairos.weakness = "physical"
		kairos.resistance = "void"
		BattleManager.start_battle(kairos, "res://scenes/maps/colorless_waste.tscn",
			"res://assets/cg/ch9_kairos_battle.jpg", "res://assets/cg/ch9_kairos_battle.jpg")
		BattleManager.battle_ended.connect(_on_kairos_battle_ended, CONNECT_ONE_SHOT)
		SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")

func _on_kairos_battle_ended(victory: bool) -> void:
	if victory:
		await get_tree().create_timer(1.0).timeout
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_defeated")

## ===================== 출구 트리거 (북쪽 → BL-07) =====================

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
		if body.name == "Player" and GameManager.get_flag("ch9_kairos") and not GameManager.get_flag("ch9_complete"):
			_depart_waste()
	)
	add_child(area)

func _depart_waste() -> void:
	GameManager.set_flag("ch9_complete")
	AchievementManager.record_chapter_complete(9)
	DialogueManager.dialogue_ended.connect(_on_departure_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "waste_departure")

func _on_departure_ended() -> void:
	GameManager.current_chapter = 10
	SaveManager.autosave_on_chapter_transition()
	print("[ColorlessWaste] Chapter 9 complete — entering BL-07")
	await get_tree().create_timer(1.5).timeout
	# S58: Chapter completion screen with stats summary
	SceneTransition.change_scene_chapter_complete("res://scenes/maps/bl07_void.tscn", 9)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(Vector2(4 * TILE_SIZE, 5 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Colorless Wraith", 100, 24, true, ["drain", "stun"])
	_add_battle_area(Vector2(18 * TILE_SIZE, 6 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Depth Crawler", 85, 20, false, ["charge", "reflect"])
	_add_battle_area(Vector2(8 * TILE_SIZE, 11 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Void Fragment", 110, 26, true, ["drain", "shield", "reflect"])

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
	indicator.color = Color(0.2, 0.2, 0.2, 0.15)
	indicator.z_index = -1
	area.add_child(indicator)
	_battle_counter += 1
	var flag_name = "battle_waste_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			enemy.abilities = abilities
			BattleManager.start_battle(enemy, "res://scenes/maps/colorless_waste.tscn", "", "")
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 랜덤 인카운터 =====================

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch9_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Colorless Wraith", "hp": 100, "atk": 24, "is_void": true, "abilities": ["drain", "stun"]},
			{"name": "Depth Crawler", "hp": 85, "atk": 20, "is_void": false, "abilities": ["charge", "reflect"]},
			{"name": "Void Fragment", "hp": 110, "atk": 26, "is_void": true, "abilities": ["drain", "shield", "reflect"]},
		],
		"res://scenes/maps/colorless_waste.tscn", "", "", 25, 55
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	_add_chest(Vector2(6 * TILE_SIZE, 4 * TILE_SIZE), "chest_waste_ridge", {"items": {"hi_potion": 3, "antidote": 1}, "grains": 25})
	_add_clue(Vector2(13 * TILE_SIZE, 4 * TILE_SIZE), "clue_waste_pillar", "A compressed pillar of residue. If you press your ear against it, you hear a century of conversation compressed into a single hum.")
	_add_clue(Vector2(5 * TILE_SIZE, 11 * TILE_SIZE), "clue_waste_footprints", "Footprints that go in circles. The walker forgot they were walking. The prints get shallower with each loop until they vanish.")

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
	indicator.color = Color(0.4, 0.4, 0.4, 0.2)
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
	indicator.color = Color(0.3, 0.3, 0.5, 0.2)
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
	_add_story_trigger(Vector2(15 * TILE_SIZE, 9 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "waste_atmosphere", "ch9_atmosphere")
	_add_story_trigger(Vector2(8 * TILE_SIZE, 7 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "arrel_compass_pull", "ch9_pull")
	_add_story_trigger(Vector2(16 * TILE_SIZE, 11 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "depth_markers", "ch9_markers")
	# Side quest: Calibrating the Compass
	if SideQuest.is_active("colorless_compass"):
		_add_story_trigger(Vector2(20 * TILE_SIZE, 3 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "arrel_compass_pull", "sq_compass_anchor1")
		_add_story_trigger(Vector2(4 * TILE_SIZE, 10 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "arrel_compass_pull", "sq_compass_anchor2")
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "colorless_waste")

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
		{"color": Color(0.2, 0.2, 0.2), "detail": "dead_soil"},       # 0: FLAT
		{"color": Color(0.12, 0.12, 0.12), "detail": "cliff"},         # 1: RIDGE
		{"color": Color(0.15, 0.15, 0.15), "detail": "cliff"},         # 2: PILLAR
		{"color": Color(0.08, 0.08, 0.08), "detail": "cliff"},         # 3: WALL
		{"color": Color(0.25, 0.25, 0.25), "detail": "path"},          # 4: PATH
		{"color": Color(0.3, 0.3, 0.3), "detail": "rock"},             # 5: MARKER
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.RIDGE, Tile.PILLAR, Tile.WALL])
	for body in bodies:
		add_child(body)
	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(8 * TILE_SIZE, 16 * TILE_SIZE)
	elia.position = Vector2(8 * TILE_SIZE - 30, 16 * TILE_SIZE + 20)
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}
