## Drift Shelter, 드리프트 쉘터 (Chapter 4: Drift)
## 무너진 고가 아래 임시 야영지. 아렐의 읽기 능력 저하 + 엘리아의 앵커링 세션.
## Canon migration Wave 1 ends here, immediately before Chapter 5: The Classifier.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter4_dialogue.json"

enum Tile { MUD, RUBBLE, CONCRETE, WALL, PATH, SHELTER }

# 0=진흙, 1=잔해, 2=콘크리트(고가 잔해), 3=벽, 4=길, 5=셸터(지붕)
var map_data: Array = [
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
	[3,0,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,1,1,0,0,0,0,0,3],
	[3,0,0,1,0,0,0,0,0,0,4,0,0,0,0,0,1,1,1,0,0,0,0,0,3],
	[3,0,0,1,1,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,2,2,2,2,4,2,2,2,2,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,2,5,5,5,5,5,5,5,5,5,2,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,2,5,5,5,5,5,5,5,5,5,2,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,2,5,5,5,5,5,5,5,5,5,2,0,0,0,0,0,0,0,0,4],
	[3,0,0,0,0,2,5,5,5,5,5,5,5,5,5,2,0,0,0,1,0,0,0,0,4],
	[3,0,0,0,0,0,2,2,4,4,4,4,2,2,0,0,0,0,1,1,0,0,0,0,4],
	[3,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,1,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,1,0,0,3],
	[3,0,1,1,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,1,1,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,3],
	[3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3],
]

var _tile_defs: Array = []
var _minimap_data: Dictionary = {}
var _encounter_data: RandomEncounter.EncounterData = null
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
		"hue": Color(0.30, 0.30, 0.45),
		"light": Color(0.58, 0.64, 0.88, 1.0),
		"mood": 0.48,
		"saturation": 1.0,
		"brightness": -0.05,
		"fog": "none",
		"splash": "res://assets/cg/generated/chapter_splash_drift_shelter.png",
	}
	MapEffects.apply_atmosphere(self, atmosphere)
	_fog_layer = MapEffects.apply_atmosphere_fog_layer(self, atmosphere)
	MapEffects.add_burn_desaturation(self)
	MapEffects.add_parallax_background(self, {"sky": Color(0.14, 0.13, 0.15), "far": Color(0.16, 0.15, 0.17), "mid": Color(0.18, 0.16, 0.15), "biome": "wasteland", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	# S236: 주변광은 대기 예산이 소유한다 (apply_atmosphere).
	# S52: 그래픽 업그레이드
	_camera = MapEffects.setup_smooth_camera(player, 1.0, 0.3)
	MapEffects.add_drop_shadow(player)
	# S59: 비에 젖은 안개 + 깊이 그라디언트
	_position_player()
	if GameManager.get_flag("ch4_complete") and not GameManager.get_flag("canon_ch5_classifier_ready"):
		_setup_battle_triggers()
	_setup_exit_trigger()
	if GameManager.get_flag("ch4_complete") and not GameManager.get_flag("canon_ch5_classifier_ready"):
		_setup_interactive_objects()
		_setup_exploration_events()
	_setup_map_decorations()
	if GameManager.get_flag("ch4_complete") and not GameManager.get_flag("canon_ch5_classifier_ready"):
		_setup_random_encounters()
		WorldPopulation.populate(self, "drift_shelter")
		WorldAtlas.add_gateways(self, "drift_shelter")
	AchievementManager.record_map_visit("drift_shelter")
	elia.repeat_line = "Rest. Please."
	print("[DriftShelter] Map loaded, %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	if GameManager.get_flag("canon_ch5_classifier_ready"):
		return
	if not GameManager.get_flag("ch4_arrived"):
		await MapEffects.show_chapter_title(self, 4, "Drift", "The architecture crumbles")
		await get_tree().create_timer(0.3).timeout
		_start_ch4_sequence()
	elif not GameManager.get_flag("ch4_reading_loss"):
		_start_reading_loss_sequence()
	elif not GameManager.get_flag("ch4_anchoring"):
		_start_anchoring_sequence()
	elif not GameManager.get_flag("ch4_night_watch"):
		_start_night_watch_sequence()

func _process(delta: float) -> void:
	effect_time += delta
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
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

func _start_ch4_sequence() -> void:
	GameManager.set_flag("ch4_arrived")
	MemoryManager.add_chapter_memories(4)
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "drift_arrival")

func _on_arrival_ended() -> void:
	await get_tree().create_timer(1.0).timeout
	_start_reading_loss_sequence()

func _start_reading_loss_sequence() -> void:
	if GameManager.get_flag("ch4_reading_loss"):
		return
	GameManager.set_flag("ch4_reading_loss")
	DialogueManager.dialogue_ended.connect(_on_deterioration_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "reading_deterioration")

func _on_deterioration_ended() -> void:
	await get_tree().create_timer(1.0).timeout
	_start_anchoring_sequence()

func _start_anchoring_sequence() -> void:
	if GameManager.get_flag("ch4_anchoring"):
		return
	GameManager.set_flag("ch4_anchoring")
	DialogueManager.dialogue_ended.connect(_on_anchoring_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "anchoring_session")

func _on_anchoring_ended() -> void:
	StoryJournal.add_event("anchoring_session", "Elia performed an anchoring session, stabilizing Arrel's memory architecture with small, unforgettable sensations.")
	await get_tree().create_timer(1.0).timeout
	_start_night_watch_sequence()

func _start_night_watch_sequence() -> void:
	if GameManager.get_flag("ch4_night_watch"):
		return
	GameManager.set_flag("ch4_night_watch")
	DialogueManager.load_and_start(DIALOGUE_FILE, "night_watch")

## ===================== 출구 트리거 (Chapter 5 migration boundary) =====================

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
		if body.name == "Player" and GameManager.get_flag("ch4_night_watch") and not GameManager.get_flag("ch4_complete"):
			_depart_shelter()
	)
	add_child(area)

func _depart_shelter() -> void:
	GameManager.set_flag("ch4_complete")
	AchievementManager.record_chapter_complete(4)
	DialogueManager.dialogue_ended.connect(_on_departure_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "drift_departure")

func _on_departure_ended() -> void:
	GameManager.current_chapter = 4
	GameManager.set_flag("canon_ch5_classifier_ready")
	SaveManager.autosave_on_chapter_transition()
	var boundary_text := "Season 1 migration boundary reached. Chapter 5: The Classifier is next."
	if GameManager.current_locale == "ko":
		boundary_text = "시즌 1 마이그레이션 경계에 도달했습니다. 다음은 5장: 분류자입니다."
	NotificationToast.show_toast(boundary_text, NotificationToast.ToastType.INFO)
	print("[DriftShelter] Canon Wave 1 complete; Chapter 5: The Classifier is not live yet")

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(
		Vector2(3 * TILE_SIZE, 3 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Memory Leech", 50, 13, true
	)
	_add_battle_area(
		Vector2(19 * TILE_SIZE, 9 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Rubble Rat", 35, 9, false
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
	var flag_name = "battle_drift_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			if is_void:
				enemy.abilities = ["drain"]
			else:
				enemy.abilities = ["poison"]
			BattleManager.start_battle(enemy, "res://scenes/maps/drift_shelter.tscn", bg_img, e_img)
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)

## ===================== 랜덤 인카운터 =====================

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch4_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Memory Leech", "hp": 50, "atk": 13, "is_void": true, "abilities": ["drain"]},
			{"name": "Rubble Rat", "hp": 35, "atk": 9, "is_void": false, "abilities": ["poison"]},
			{"name": "Ash Walker", "hp": 60, "atk": 11, "is_void": false, "abilities": ["weaken", "burn_attack"]},
		],
		"res://scenes/maps/drift_shelter.tscn", "", "", 50, 90
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	_add_chest(
		Vector2(17 * TILE_SIZE, 3 * TILE_SIZE),
		"chest_drift_rubble",
		{"items": {"potion": 1, "antidote": 1}, "grains": 6}
	)
	_add_clue(
		Vector2(10 * TILE_SIZE, 7 * TILE_SIZE),
		"clue_drift_campfire",
		"Warm ashes. Someone camped here recently, the fire pit is lined with Bureau-issue kindling."
	)
	_add_clue(
		Vector2(3 * TILE_SIZE, 13 * TILE_SIZE),
		"clue_drift_graffiti",
		"Scratched into the concrete: 'WE REMEMBER.' Below it, in different handwriting: 'For how long?'"
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
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "drift_shelter")

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
	# 모닥불 (셸터 중앙)
	var fire = ColorRect.new()
	fire.size = Vector2(8, 8)
	fire.position = Vector2(10 * TILE_SIZE + 12, 7 * TILE_SIZE + 12)
	fire.color = Color(0.8, 0.4, 0.1, 0.6)
	fire.z_index = 1
	add_child(fire)
	# 모닥불 빛
	var light = PointLight2D.new()
	light.position = fire.position + Vector2(4, 4)
	light.color = Color(0.9, 0.6, 0.3)
	light.energy = 0.8
	light.texture = PlaceholderTexture2D.new()
	light.texture_scale = 3.0
	light.shadow_enabled = false
	add_child(light)
	# 잔해 더미
	for pos in [Vector2(3, 3), Vector2(4, 4), Vector2(17, 2), Vector2(18, 3), Vector2(19, 9), Vector2(20, 10), Vector2(3, 12), Vector2(21, 12)]:
		var rubble = ColorRect.new()
		rubble.size = Vector2(TILE_SIZE * 0.6, TILE_SIZE * 0.4)
		rubble.position = pos * TILE_SIZE + Vector2(6, 10)
		rubble.color = Color(0.2, 0.18, 0.16, 0.25)
		rubble.z_index = -1
		add_child(rubble)
	if GameManager.get_flag("ch4_complete") and not GameManager.get_flag("canon_ch5_classifier_ready"):
		var ambient_npcs = [
			{"pos": Vector2(8, 7), "preset": "villager_f"},
			{"pos": Vector2(13, 6), "preset": "scholar"},
			{"pos": Vector2(16, 9), "preset": "villager_m"},
		]
		for npc_data in ambient_npcs:
			var npc_sprite = PixelSprite.create_npc_sprite(npc_data["preset"])
			npc_sprite.position = npc_data["pos"] * TILE_SIZE + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
			npc_sprite.z_index = 1
			add_child(npc_sprite)

	# 재비 파티클
	MapEffects.add_void_particles(self, MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, Color(0.35, 0.33, 0.38, 0.12), 20)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.2, 0.18, 0.17), "detail": "mud"},           # 0: MUD
		{"color": Color(0.22, 0.2, 0.18), "detail": "rock"},          # 1: RUBBLE
		{"color": Color(0.28, 0.26, 0.24), "detail": "stone_floor"},  # 2: CONCRETE
		{"color": Color(0.12, 0.1, 0.09), "detail": "cliff"},         # 3: WALL
		{"color": Color(0.3, 0.27, 0.24), "detail": "path"},          # 4: PATH
		{"color": Color(0.16, 0.14, 0.13), "detail": "stone_floor"},  # 5: SHELTER
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	MapEffects.add_map_canvas(self, tilemap, "res://assets/environment/map_canvases/map_drift_shelter_canvas_v2.png", Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), {"terrain_alpha": 0.0})
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL, Tile.RUBBLE, Tile.CONCRETE])
	for body in bodies:
		add_child(body)
	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(10 * TILE_SIZE, 16 * TILE_SIZE)
	elia.position = Vector2(10 * TILE_SIZE - 30, 16 * TILE_SIZE + 20)
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}

## Compatibility for development saves created by the retired ten-chapter route.
func _retire_legacy_tobias_progression() -> void:
	for flag_name in ["ch3_tobias_met", "ch3_tobias_records", "tobias_in_party", "tobias_joined", "ch4_classification"]:
		if GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name, false)
	BattleManager.tobias_in_party = false
	if GameManager.get_flag("ch4_complete") and not GameManager.get_flag("canon_ch5_classifier_ready"):
		GameManager.set_flag("canon_ch5_classifier_ready")
		GameManager.current_chapter = 4
