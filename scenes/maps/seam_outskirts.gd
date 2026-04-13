## Seam Outskirts — 이음새 외곽 (Chapter 7: The Other Side of the Flame)
## BL-07 경계의 황폐한 고원. 세이블의 진실 + 에코 셸 획득 + 시련.
## 북쪽으로 진행하면 잊힌 숲으로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter7_dialogue.json"

enum Tile { ASH_GROUND, CRACK, VOID_ROCK, WALL, PATH, LEDGE }

# 0=재 땅, 1=균열, 2=보이드 바위, 3=벽, 4=길, 5=절벽 가장자리
var map_data: Array = [
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
	[3,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,1,0,0,0,0,4,0,0,0,0,0,0,1,0,0,0,0,0,0,0,3],
	[3,0,0,1,1,0,0,0,0,4,0,0,0,0,0,1,1,0,0,2,2,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,2,0,0,0,0,3],
	[3,5,5,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,5,5,3],
	[3,5,0,0,0,0,2,2,0,4,0,2,2,0,0,0,0,0,0,0,0,0,0,5,3],
	[3,0,0,0,0,0,2,0,0,4,0,0,2,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,2,2,0,0,0,0,0,0,3],
	[3,0,0,2,0,0,0,0,0,0,4,0,0,0,0,2,0,2,0,0,0,0,0,0,3],
	[3,5,0,2,2,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,5,5,3],
	[3,5,5,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,5,5,5,3],
	[3,0,0,0,0,0,0,1,0,0,0,0,4,0,0,0,1,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,1,1,0,0,0,0,4,0,0,1,1,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,3],
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
	MapEffects.add_parallax_background(self, {"sky": Color(0.1, 0.09, 0.12), "far": Color(0.12, 0.1, 0.14), "mid": Color(0.14, 0.12, 0.1), "biome": "void_edge", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.3, 0.28, 0.35))
	MapEffects.add_void_particles(self, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, Color(0.4, 0.2, 0.5, 0.12), 25)
	# S52: 그래픽 업그레이드
	MapEffects.add_color_grading(self, {"tint": Color(0.25, 0.2, 0.4), "brightness": -0.05})
	_s52_particles = MapEffects.add_void_tendrils(self, 4, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE))
	_camera = MapEffects.setup_smooth_camera(player, 1.0, 0.4)
	MapEffects.add_drop_shadow(player)
	_position_player()
	_setup_battle_triggers()
	_setup_exit_trigger()
	_setup_interactive_objects()
	_setup_exploration_events()
	_setup_random_encounters()
	AchievementManager.record_map_visit("seam_outskirts")
	elia.repeat_line = "The air feels wrong. Like static before a storm."
	sable_npc.repeat_line = "Stay focused. Don't let the Threshold get into your head."
	print("[SeamOutskirts] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if not GameManager.get_flag("ch7_arrived"):
		await MapEffects.show_chapter_title(self, 7, "The Threshold", "The other side of the flame")
		await get_tree().create_timer(0.3).timeout
		_start_ch7_sequence()

func _process(delta: float) -> void:
	effect_time += delta
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
	MapEffects.update_void_tendrils(_s52_particles, effect_time, delta)
	MapEffects.update_camera_shake(_camera, effect_time)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# S53: NPC 아이들 모션
	for npc in get_tree().get_nodes_in_group("npcs"):
		if npc.has_node("AnimatedSprite2D"):
			var spr = npc.get_node("AnimatedSprite2D")
			spr.scale = Vector2(1.0 + sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.008, 1.0 - sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.006)

## ===================== 스토리 시퀀스 =====================

func _start_ch7_sequence() -> void:
	GameManager.set_flag("ch7_arrived")
	MemoryManager.add_chapter_memories(7)
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "outskirts_arrival")

func _on_arrival_ended() -> void:
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch7_sable_truth"):
		GameManager.set_flag("ch7_sable_truth")
		DialogueManager.dialogue_ended.connect(_on_truth_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "sable_truth")

func _on_truth_ended() -> void:
	GameManager.set_flag("has_echo_shell")
	NotificationToast.show_toast("Obtained: Echo Shell", NotificationToast.ToastType.SUCCESS)
	StoryJournal.add_event("echo_shell", "Sable gave Arrel the Echo Shell — a spiraling shell that holds the last echoes of those consumed by BL-07.")
	await get_tree().create_timer(2.0).timeout
	# 시련 시작
	if not GameManager.get_flag("ch7_trial"):
		GameManager.set_flag("ch7_trial")
		DialogueManager.dialogue_ended.connect(_on_trial_dialogue_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "sable_trial")

func _on_trial_dialogue_ended() -> void:
	# 시련 전투 — Threshold Shade (자기 자신의 그림자)
	var enemy = BattleManager.Enemy.new("Threshold Shade", 120, 20, true)
	enemy.abilities = ["drain", "stun", "reflect"]
	enemy.weakness = "fire"
	BattleManager.start_battle(enemy, "res://scenes/maps/seam_outskirts.tscn", "", "")
	BattleManager.battle_ended.connect(_on_trial_battle_ended, CONNECT_ONE_SHOT)
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")

func _on_trial_battle_ended(victory: bool) -> void:
	if victory:
		await get_tree().create_timer(1.0).timeout
		GameManager.set_flag("ch7_trial_complete")
		DialogueManager.dialogue_ended.connect(_on_trial_complete_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "trial_complete")

func _on_trial_complete_ended() -> void:
	# 준비 대화
	await get_tree().create_timer(1.5).timeout
	DialogueManager.load_and_start(DIALOGUE_FILE, "party_preparation")

## ===================== 출구 트리거 (북쪽 → Forgotten Forest) =====================

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
		if body.name == "Player" and GameManager.get_flag("ch7_trial_complete") and not GameManager.get_flag("ch7_complete"):
			_depart_outskirts()
	)
	add_child(area)

func _depart_outskirts() -> void:
	GameManager.set_flag("ch7_complete")
	AchievementManager.record_chapter_complete(7)
	DialogueManager.dialogue_ended.connect(_on_departure_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "outskirts_departure")

func _on_departure_ended() -> void:
	GameManager.current_chapter = 8
	SaveManager.autosave_on_chapter_transition()
	print("[SeamOutskirts] Chapter 7 complete — entering Forgotten Forest")
	await get_tree().create_timer(1.5).timeout
	SceneTransition.change_scene_styled("res://scenes/maps/forgotten_forest.tscn")

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(Vector2(4 * TILE_SIZE, 3 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Void Sentinel", 90, 20, true, ["drain", "shield"])
	_add_battle_area(Vector2(20 * TILE_SIZE, 10 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Ash Phantom", 75, 18, false, ["stun", "weaken"])
	_add_battle_area(Vector2(10 * TILE_SIZE, 12 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "Threshold Crawler", 65, 22, true, ["charge", "drain"])

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
	indicator.color = Color(0.3, 0.05, 0.3, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)
	_battle_counter += 1
	var flag_name = "battle_outskirts_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			enemy.abilities = abilities
			BattleManager.start_battle(enemy, "res://scenes/maps/seam_outskirts.tscn", "", "")
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 랜덤 인카운터 =====================

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch7_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Void Sentinel", "hp": 90, "atk": 20, "is_void": true, "abilities": ["drain", "shield"]},
			{"name": "Ash Phantom", "hp": 75, "atk": 18, "is_void": false, "abilities": ["stun", "weaken"]},
			{"name": "Threshold Crawler", "hp": 65, "atk": 22, "is_void": true, "abilities": ["charge", "drain"]},
		],
		"res://scenes/maps/seam_outskirts.tscn", "", "", 35, 65
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	_add_chest(Vector2(21 * TILE_SIZE, 4 * TILE_SIZE), "chest_outskirts_ledge", {"items": {"hi_potion": 2, "firebomb": 1}, "grains": 15})
	_add_clue(Vector2(7 * TILE_SIZE, 14 * TILE_SIZE), "clue_outskirts_bones", "Scattered equipment. Bureau-issue. The name tags have been scoured clean — not by weather, but by absence.")
	_add_clue(Vector2(16 * TILE_SIZE, 11 * TILE_SIZE), "clue_outskirts_mark", "A carved symbol on the void rock — Sable's mark. She left it here to remember the path back.")

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
	_add_story_trigger(Vector2(15 * TILE_SIZE, 5 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "threshold_atmosphere", "ch7_atmosphere")
	_add_story_trigger(Vector2(5 * TILE_SIZE, 9 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "echo_shell_discovery", "ch7_echo_listen")
	# Side quest: Echoes of the Threshold
	if SideQuest.is_active("echo_fragments"):
		_add_story_trigger(Vector2(3 * TILE_SIZE, 11 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "echo_shell_discovery", "sq_echo_frag1")
		_add_story_trigger(Vector2(19 * TILE_SIZE, 4 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "echo_shell_discovery", "sq_echo_frag2")
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "seam_outskirts")

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
		{"color": Color(0.18, 0.16, 0.15), "detail": "dead_soil"},    # 0: ASH_GROUND
		{"color": Color(0.1, 0.08, 0.12), "detail": "rock"},          # 1: CRACK
		{"color": Color(0.15, 0.1, 0.18), "detail": "cliff"},         # 2: VOID_ROCK
		{"color": Color(0.08, 0.06, 0.08), "detail": "cliff"},        # 3: WALL
		{"color": Color(0.22, 0.2, 0.18), "detail": "path"},          # 4: PATH
		{"color": Color(0.12, 0.1, 0.1), "detail": "rock"},           # 5: LEDGE
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL, Tile.VOID_ROCK, Tile.CRACK, Tile.LEDGE])
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
