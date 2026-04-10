## Crumbling Coast — 크럼블링 코스트 (Chapter 3)
## 땅이 무너져 내리는 해안 절벽. 카이로스의 추적이 시작되는 곳.
## 남쪽에서 시작 → 북쪽 The Seam으로 이동.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter3_dialogue.json"

enum Tile { ROCK, SAND, CLIFF, WATER, PATH }

# 0=바위, 1=모래, 2=절벽(벽), 3=물, 4=길
var map_data: Array = [
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
	[2,0,0,0,4,4,4,0,0,2,3,3,3,3,3,2,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,4,0,0,0,2,3,3,3,3,3,2,0,0,1,1,0,0,0,0,2],
	[2,0,0,0,0,4,0,0,0,0,2,3,3,3,2,0,0,1,1,1,1,0,0,0,2],
	[2,2,0,0,0,4,0,0,0,0,0,2,2,2,0,0,0,0,1,1,0,0,0,2,2],
	[2,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,0,0,1,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,2],
	[2,0,0,1,1,0,0,4,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,2],
	[2,2,0,0,0,0,0,4,0,0,2,2,0,0,2,2,0,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,4,0,0,2,0,0,0,0,2,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,4,4,0,0,0,0,0,2],
	[2,0,0,1,1,0,0,0,4,0,0,0,0,0,0,0,4,4,0,0,1,1,0,0,2],
	[2,2,0,0,0,0,0,0,4,4,0,0,0,0,0,4,4,0,0,0,0,0,2,2,2],
	[2,0,0,0,0,0,0,0,0,4,4,0,0,0,4,4,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,2],
	[2,0,0,0,0,0,2,2,0,0,0,0,4,0,0,0,0,2,2,0,0,0,0,0,2],
	[2,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,2],
	[2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2],
]

var tile_colors: Dictionary = {
	Tile.ROCK: Color(0.32, 0.3, 0.28),
	Tile.SAND: Color(0.45, 0.4, 0.32),
	Tile.CLIFF: Color(0.15, 0.13, 0.12),
	Tile.WATER: Color(0.1, 0.18, 0.3),
	Tile.PATH: Color(0.38, 0.35, 0.3),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia

var water_shimmers: Array[ColorRect] = []
var effect_time: float = 0.0
var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null
var _tide_pools: Array[ColorRect] = []
var _point_lights: Array[PointLight2D] = []  # S42

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	MapEffects.add_burn_desaturation(self)  # S46: 기억 연소 월드 탈색
	MapEffects.add_heat_haze(self, 0.002)  # S46: 해안 열기 왜곡
	# S42: 패럴랙스 + 조명
	MapEffects.add_parallax_background(self, {"sky": Color(0.12, 0.12, 0.18), "far": Color(0.15, 0.15, 0.2), "mid": Color(0.2, 0.18, 0.16), "biome": "coast", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.5, 0.5, 0.55))
	_position_player()
	_setup_battle_triggers()
	_setup_seam_trigger()
	water_shimmers = MapEffects.add_water_shimmer(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.WATER)
	MapEffects.add_rain(self, 0.7, Color(0.5, 0.55, 0.7, 0.25))
	_setup_random_encounters()
	_setup_interactive_objects()
	_setup_map_decorations()
	AchievementManager.record_map_visit("crumbling_coast")
	elia.repeat_line = "The ground shifts. Stay close."
	print("[CrumblingCoast] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])
	_ready_sequence()

func _ready_sequence() -> void:
	# Ch3 도착 시퀀스 (한 번만 실행)
	if not GameManager.get_flag("ch3_arrived"):
		# 챕터 타이틀 카드 표시 후 대화 시작
		await MapEffects.show_chapter_title(self, 3, "Crumbling Coast", "The ground gives way")
		await get_tree().create_timer(0.3).timeout
		_start_ch3_sequence()

func _process(delta: float) -> void:
	effect_time += delta
	MapEffects.update_water_shimmer(water_shimmers, effect_time)
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# 조수 웅덩이 맥동
	for tp in _tide_pools:
		var phase = tp.get_meta("phase", 0.0)
		tp.color.a = 0.15 + sin(effect_time * 1.8 + phase) * 0.08

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 조수 웅덩이 (모래 근처 물가)
	var pool_positions = [Vector2(5, 12), Vector2(10, 15), Vector2(18, 13)]
	for i in range(pool_positions.size()):
		var pos = pool_positions[i]
		var tp = ColorRect.new()
		tp.size = Vector2(10, 6)
		tp.position = pos * TILE_SIZE + Vector2(8, 12)
		tp.color = Color(0.3, 0.5, 0.7, 0.18)
		tp.z_index = -1
		tp.set_meta("phase", float(i) * 2.1)
		add_child(tp)
		_tide_pools.append(tp)
	# 유목 (해변 위)
	for dw_pos in [Vector2(8, 14), Vector2(15, 16)]:
		var dw = ColorRect.new()
		dw.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 0.3)
		dw.position = dw_pos * TILE_SIZE + Vector2(4, 20)
		dw.color = Color(0.3, 0.25, 0.2, 0.3)
		dw.z_index = -1
		add_child(dw)

## ===================== 스토리 시퀀스 =====================

func _start_ch3_sequence() -> void:
	GameManager.set_flag("ch3_arrived")
	MemoryManager.add_chapter_memories(3)
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "coast_arrival")

func _on_arrival_ended() -> void:
	# 카이로스 목격 이벤트 (짧은 딜레이 후)
	await get_tree().create_timer(2.0).timeout
	if not GameManager.get_flag("ch3_kairos_seen"):
		GameManager.set_flag("ch3_kairos_seen")
		DialogueManager.dialogue_ended.connect(_on_kairos_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "kairos_sighting")

func _on_kairos_ended() -> void:
	# 카이로스 목격 후 → 엘리아 분리 선택 (한 번만)
	if not GameManager.get_flag("elia_separation_done"):
		await get_tree().create_timer(1.5).timeout
		GameManager.set_flag("elia_separation_done")
		DialogueManager.dialogue_ended.connect(_on_separation_choice_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "elia_separation_choice")

func _on_separation_choice_ended() -> void:
	await get_tree().create_timer(0.3).timeout
	if GameManager.get_flag("elia_separates"):
		# 엘리아 분리
		GameManager.player_data.elia_with_party = false
		DialogueManager.dialogue_ended.connect(_on_separation_response_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "elia_separates_response")
	else:
		# 엘리아 동행 유지
		DialogueManager.dialogue_ended.connect(_on_separation_response_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "elia_stays_response")

func _on_separation_response_ended() -> void:
	if GameManager.get_flag("elia_separates"):
		# 엘리아 씬에서 제거
		if elia:
			elia.visible = false
			elia.set_physics_process(false)
		print("[CrumblingCoast] Elia separated — memories burn without residue")

## ===================== The Seam 도착 트리거 (북쪽) =====================

func _setup_seam_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(5 * TILE_SIZE, 1.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name == "Player" and not GameManager.get_flag("ch3_seam_arrived"):
			_arrive_at_seam()
	)
	add_child(area)

func _arrive_at_seam() -> void:
	GameManager.set_flag("ch3_seam_arrived")
	GameManager.set_flag("ch3_complete")
	AchievementManager.record_chapter_complete(3)
	DialogueManager.dialogue_ended.connect(_on_seam_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "seam_arrival")

func _on_seam_ended() -> void:
	GameManager.current_chapter = 4
	print("[CrumblingCoast] Chapter 3 complete — The Seam reached")
	await get_tree().create_timer(1.5).timeout
	SceneTransition.change_scene("res://scenes/maps/the_seam.tscn")

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch3_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Coastal Void Beast", "hp": 100, "atk": 18, "is_void": true, "abilities": ["drain"], "bg": "res://assets/cg/ch3_seam_coast.jpg", "img": "res://assets/cg/void_beast.jpg"},
			{"name": "Cliff Stalker", "hp": 70, "atk": 16, "is_void": false, "abilities": ["poison", "multi_hit"], "bg": "res://assets/cg/ch3_seam_coast.jpg"},
			{"name": "Shore Wraith", "hp": 85, "atk": 14, "is_void": true, "abilities": ["burn_attack", "weaken"], "bg": "res://assets/cg/ch3_seam_coast.jpg"},
		],
		"res://scenes/maps/crumbling_coast.tscn", "", "", 40, 70
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	# 상자 — 모래밭 사이 (우측 중단)
	_add_chest(
		Vector2(19 * TILE_SIZE, 6 * TILE_SIZE),
		"chest_coast_sand",
		{"items": {"hi_potion": 1}, "grains": 12}
	)
	# 단서 — 절벽 근처 (카이로스 관련)
	_add_clue(
		Vector2(7 * TILE_SIZE, 11 * TILE_SIZE),
		"clue_coast_tracks",
		"Footprints that end abruptly. Whoever walked here... stopped existing."
	)
	# 상자 — 남쪽 길 옆
	_add_chest(
		Vector2(14 * TILE_SIZE, 14 * TILE_SIZE),
		"chest_coast_path",
		{"items": {"antidote": 2}, "grains": 8}
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

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.32, 0.3, 0.28), "detail": "rock"},     # 0: ROCK
		{"color": Color(0.45, 0.4, 0.32), "detail": "sand"},     # 1: SAND
		{"color": Color(0.15, 0.13, 0.12), "detail": "cliff"},   # 2: CLIFF
		{"color": Color(0.1, 0.18, 0.3), "detail": "water"},     # 3: WATER
		{"color": Color(0.38, 0.35, 0.3), "detail": "path"},     # 4: PATH
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.CLIFF, Tile.WATER])
	for body in bodies:
		add_child(body)

	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(12 * TILE_SIZE, 16 * TILE_SIZE)
	elia.position = Vector2(12 * TILE_SIZE - 30, 16 * TILE_SIZE + 20)
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}
	# 엘리아 분리 상태 반영
	if GameManager.get_flag("elia_separates") and not GameManager.get_flag("elia_reunited"):
		elia.visible = false
		elia.set_physics_process(false)

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	_add_battle_area(
		Vector2(18 * TILE_SIZE, 7 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Coastal Void Beast", 100, 18, true,
		"res://assets/cg/ch3_seam_coast.jpg", "res://assets/cg/void_beast3.jpg"
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
	var flag_name = "battle_coast_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			if enemy_name == "Coastal Void Beast":
				enemy.abilities = ["drain"]
			BattleManager.start_battle(enemy, "res://scenes/maps/crumbling_coast.tscn", bg_img, e_img)
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)
