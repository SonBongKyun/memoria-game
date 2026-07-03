## Rim Exterior Forest — 림 외곽 숲 (Chapter 1 시작 맵)
## 아렐이 공허수를 처치한 직후. 재비가 내리는 숲.
## 스토리 시퀀스: opening → elia → ash_rain → 자유탐색 → camp_night
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter1_dialogue.json"

enum Tile { GRASS, PATH, TREE, BUSH, WATER }

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

# S43: 컬러 팔레트 리뉴얼 — 더 풍부한 색상
var tile_colors: Dictionary = {
	Tile.GRASS: Color(0.15, 0.32, 0.12),  # 더 진한 초록
	Tile.PATH: Color(0.38, 0.3, 0.2),  # 더 따뜻한 흙
	Tile.TREE: Color(0.06, 0.15, 0.06),  # 더 진한 나무
	Tile.BUSH: Color(0.18, 0.36, 0.15),  # 더 선명한 덤불
	Tile.WATER: Color(0.08, 0.18, 0.32),  # 더 깊은 파랑
}

var tile_nodes: Array = []
var collision_bodies: Array = []
var ash_rain_node = null  # 재비 파티클
var fog_rects: Array[ColorRect] = []  # 안개 효과
var _time: float = 0.0
var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null
var _mushroom_lights: Array[ColorRect] = []
var _point_lights: Array[PointLight2D] = []  # S42: 2D 조명
var _grass_blades: Array[ColorRect] = []  # S43: 풀 흔들림
var _occluders: Array[LightOccluder2D] = []  # S52: 그림자 오클루더
var _pollen: Array[ColorRect] = []  # S52: 꽃가루 파티클
var _camera: Camera2D = null  # S52: 스무스 카메라
var _blend_edges: Array[ColorRect] = []  # S53: 타일 경계 블렌딩
var _drifting_flies: Array[ColorRect] = []  # S57: 사인파 반딧불
var _falling_leaves: Array[ColorRect] = []  # S57: 낙엽
var _tod_layer: CanvasLayer = null  # S57: 시간대 색조
var _fog_layer: Array[ColorRect] = []  # S59: 프로시저럴 안개

@onready var player: CharacterBody2D = $Player

func _ready() -> void:
	_build_map()
	_blend_edges = TilePainter.auto_blend_edges(self, map_data, MAP_WIDTH, MAP_HEIGHT, tile_colors, TILE_SIZE)
	# S131: Opening readability pass — keep the mood, but avoid hiding the first playable screen.
	MapEffects.add_vignette(self, 0.14)
	MapEffects.add_burn_desaturation(self)  # S46: 기억 연소 월드 탈색
	MapEffects.add_fireflies(self, 12, Color(0.5, 0.85, 0.3, 0.5))  # S46: 숲 반딧불
	fog_rects = MapEffects.add_fog(self, Color(0.2, 0.22, 0.18, 0.018))
	# S42: 패럴랙스 배경 + 2D 조명
	MapEffects.add_parallax_background(self, {"sky": Color(0.05, 0.08, 0.12), "far": Color(0.08, 0.12, 0.08), "mid": Color(0.12, 0.18, 0.1), "biome": "forest", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.66, 0.66, 0.68))
	# S43: 풀 흔들림 애니메이션
	_grass_blades = MapEffects.add_grass_sway(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.GRASS)
	# 버섯 위치에 은은한 초록 라이트
	for m in _mushroom_lights:
		_point_lights.append(MapEffects.add_point_light(self, m.position + Vector2(3, 3), Color(0.3, 0.9, 0.4), 0.4, 48.0))
	# S52: 그래픽 업그레이드
	MapEffects.enable_shadows_on_lights(_point_lights)
	_occluders = MapEffects.add_tile_occluders(self, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.TREE, Tile.BUSH])
	MapEffects.add_color_grading(self, {"tint": Color(0.34, 0.48, 0.28), "brightness": 0.02})
	MapEffects.add_illustration_atmosphere(self, "res://assets/cg/generated/story_ch1_twisted_forest_path.png", 0.08, Color(0.84, 0.98, 0.76))
	_pollen = MapEffects.add_pollen_particles(self, 6, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.6, 0.8, 0.4, 0.16))
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	# S57: 앰비언트 와일드라이프 — 반딧불 + 낙엽
	var map_area = Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE)
	_drifting_flies = MapEffects.add_drifting_fireflies(self, 8, map_area, Color(0.5, 0.9, 0.35, 0.6))
	_falling_leaves = MapEffects.add_falling_leaves(self, 6, map_area, Color(0.4, 0.5, 0.18, 0.45))
	# S57: 시간대 색조 변화
	_tod_layer = MapEffects.add_time_of_day(self)
	# S57: 플레이어 포그 오브 워 라이트
	MapEffects.add_player_fog_light(player, 320.0, 1.15, Color(0.98, 0.94, 0.84))
	# S57: 로어 글로우 포인트 (그루터기/숨겨진 메모)
	MapEffects.add_lore_glow(self, Vector2(4 * TILE_SIZE, 9 * TILE_SIZE), Color(0.7, 0.85, 0.3, 0.5))
	MapEffects.add_lore_glow(self, Vector2(20 * TILE_SIZE, 4 * TILE_SIZE), Color(0.85, 0.7, 0.3, 0.5))
	# S57: 맵 진입 테마 파티클
	MapEffects.spawn_transition_particles(self, "forest")
	# S59: 인터랙티브 프롭 + 분위기 강화
	_setup_interactive_props()
	_fog_layer = MapEffects.add_fog_layer(self, 0.16, Color(0.2, 0.22, 0.18, 0.018), 2.0)
	MapEffects.add_wind_sway(self, 2.0)
	MapEffects.add_depth_gradient(self, 0.045)
	MapEffects.add_premium_map_lens(self, {"tint": Color(0.62, 0.76, 0.42, 1.0), "vignette": 0.16, "tint_strength": 0.028, "shafts": 0.035, "glints": 1, "grain": 0.0, "letterbox": 0.08})
	_position_player()
	# S66 (A안 — Act I 데모 슬림화):
	# 보스(Void Beast) 트리거 + 캠프 + 핵심 히든 이벤트만 유지.
	# 비활성: 랜덤 인카운터, 사이드 퀘스트 — 코드 보존, 호출만 차단.
	_setup_battle_triggers()
	_setup_camp_trigger()
	_setup_hidden_events()
	_setup_interactive_objects()
	# _setup_random_encounters()  # S66: VN 정체성에 맞지 않음 — 비활성
	# _setup_side_quests()        # S66: 사이드 분기 제거, 본 스토리에 집중
	_setup_map_decorations()
	AchievementManager.record_map_visit("rim_forest")
	# S64: Perception Drift — daily_campfire_song 태웠으면 엘리아를 창백하게, "Echo of the Song" 오브젝트 표시
	_setup_perception_nodes()
	PerceptionFilter.apply(self)
	print("[RimForest] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	# S60: VN 하이브리드 모드 — VN이 프롤로그를 이미 재생했으면 스토리 건너뛰고
	# 자유 탐색 + 남쪽 끝에서 VN 재개(ch1_after_forest).
	if SceneFlow.resume_queue.size() > 0:
		GameManager.set_flag("ch1_opening_done")
		GameManager.set_flag("ch1_elia_appeared")
		GameManager.set_flag("ch1_ash_rain_seen")
		_start_ash_rain()
		print("[RimForest] VN hybrid mode — free exploration, exit south to resume VN")
	# 레거시 스토리 시퀀스 (VN 없이 직접 진입 시)
	elif not GameManager.get_flag("ch1_opening_done"):
		await MapEffects.show_chapter_title(self, 1, "Rim Forest", "The edge of what remains")
		await get_tree().create_timer(0.3).timeout
		_start_story_sequence()

func _process(delta: float) -> void:
	_time += delta
	MapEffects.update_fog(fog_rects, _time)
	MapEffects.update_point_lights(_point_lights, _time)
	MapEffects.update_grass_sway(_grass_blades, _time)
	MapEffects.update_pollen(_pollen, _time, delta)
	MapEffects.update_camera_shake(_camera, _time)
	# S59: 안개 + 바람 + 트리거 글로우 + 캠프파이어
	MapEffects.update_fog_layer(_fog_layer, _time)
	MapEffects.update_wind_sway(self, _time)
	MapEffects.update_trigger_approach_glow(self, player.position, _time)
	MapEffects.update_campfire_glows(self, _time)
	# S57: 와일드라이프 + 시간대 + 로어 글로우 업데이트
	MapEffects.update_drifting_fireflies(_drifting_flies, _time)
	MapEffects.update_falling_leaves(_falling_leaves, _time, delta)
	MapEffects.update_time_of_day(_tod_layer, _time)
	MapEffects.update_lore_glows(self, _time)
	# S53: 뷰포트 외 파티클 컬링
	var vp_rect = Rect2(player.position - Vector2(700, 400), Vector2(1400, 800))
	MapEffects.cull_offscreen_particles(_pollen, vp_rect)
	MapEffects.cull_offscreen_particles(_grass_blades, vp_rect)
	# S66: 미니맵·랜덤 인카운터 비활성화 (Act I VN 정체성)
	# Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE)
	# if _encounter_data:
	# 	RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# S53: NPC 아이들 모션
	for npc in get_tree().get_nodes_in_group("npcs"):
		if npc.has_node("AnimatedSprite2D"):
			var spr = npc.get_node("AnimatedSprite2D")
			spr.scale = Vector2(1.0 + sin(_time * 1.5 + npc.position.x * 0.1) * 0.008, 1.0 - sin(_time * 1.5 + npc.position.x * 0.1) * 0.006)
	# 버섯 빛 애니메이션
	for m in _mushroom_lights:
		var phase = m.get_meta("phase", 0.0)
		m.color.a = 0.2 + sin(_time * 2.0 + phase) * 0.12

## ===================== 스토리 시퀀스 =====================

func _start_story_sequence() -> void:
	# 1단계: 오프닝 대화
	DialogueManager.dialogue_ended.connect(_on_opening_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "opening_void_beast")

func _on_opening_ended() -> void:
	GameManager.set_flag("ch1_opening_done")
	# 2단계: 엘리아 등장
	await get_tree().create_timer(0.8).timeout
	DialogueManager.dialogue_ended.connect(_on_elia_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "elia_appears")

func _on_elia_ended() -> void:
	GameManager.set_flag("ch1_elia_appeared")
	# 3단계: 재비 시작 + 대화
	await get_tree().create_timer(0.5).timeout
	_start_ash_rain()
	DialogueManager.dialogue_ended.connect(_on_ash_rain_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "ash_rain")

func _on_ash_rain_ended() -> void:
	GameManager.set_flag("ch1_ash_rain_seen")
	# 자유 탐색 모드 — 엘리아 재대화 가능, 전투 트리거 활성
	print("[RimForest] Free exploration — head south for camp")
	# S53: 튜토리얼 프롬프트 표시
	_show_tutorial()

## ===================== S53: 인터랙티브 튜토리얼 =====================

func _show_tutorial() -> void:
	if GameManager.get_flag("tutorial_complete"):
		return
	GameManager.set_flag("tutorial_complete")
	# 이동 안내
	await _show_tutorial_toast("Move with WASD or Arrow Keys", 3.0)
	# 상호작용 안내
	await _show_tutorial_toast("Press SPACE to interact with NPCs and objects", 3.0)
	# 기억 안내
	await _show_tutorial_toast("Press TAB or M to open your Memory Archive", 3.0)
	# 메뉴 안내
	await _show_tutorial_toast("Press ESC to open the Pause Menu", 2.0)

func _show_tutorial_toast(text: String, duration: float) -> void:
	NotificationToast.show_toast(text)
	await get_tree().create_timer(duration).timeout

## ===================== 재비 파티클 =====================

func _start_ash_rain() -> void:
	if ash_rain_node:
		return
	var AshRainScript = load("res://scripts/effects/ash_rain.gd")
	ash_rain_node = GPUParticles2D.new()
	ash_rain_node.set_script(AshRainScript)
	player.add_child(ash_rain_node)

## ===================== 야영 트리거 (남쪽 끝) =====================

func _setup_camp_trigger() -> void:
	# 남쪽 길 끝 (타일 12,16 근처)
	var area = Area2D.new()
	area.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 16 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name != "Player" or GameManager.get_flag("ch1_camp_done"):
			return
		# S66 (Act I 데모): 보스(Void Beast) 처치 전에는 진행 불가 — 클라이맥스 필수화
		if not GameManager.get_flag("ch1_void_beast_defeated"):
			NotificationToast.show_toast("Something blocks the path. Find what hunts these woods.")
			return
		# S60: VN 하이브리드 — 복귀 큐가 있으면 VN으로 돌아감 (vn_host._ready가 자동 resume)
		if SceneFlow.resume_queue.size() > 0:
			GameManager.set_flag("ch1_camp_done")
			SceneTransition.change_scene_styled("res://scenes/main/vn_host.tscn")
			return
		if GameManager.get_flag("ch1_ash_rain_seen"):
			_start_camp_scene()
	)

	add_child(area)

func _start_camp_scene() -> void:
	GameManager.set_flag("ch1_camp_done")
	DialogueManager.dialogue_ended.connect(_on_camp_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "camp_night")

func _on_camp_ended() -> void:
	GameManager.set_flag("ch1_complete")
	GameManager.current_chapter = 2
	SaveManager.autosave_on_chapter_transition()
	AchievementManager.record_chapter_complete(1)
	GameManager.add_item("potion", 1)
	print("[RimForest] Chapter 1 complete")
	# 히든 엔딩 CG — 녹색 나무 (짧게 보여주고 전환)
	await get_tree().create_timer(1.0).timeout
	CgViewer.show_cg("res://assets/cg/generated/story_ch1_green_tree_dawn.png", "", 3.0, func():
		# S58: Chapter completion screen with stats summary
		SceneTransition.change_scene_chapter_complete("res://scenes/maps/verdan_market.tscn", 1)
	)

## ===================== S64: Perception Drift =====================

## daily_campfire_song을 태운 플레이어에게만 보이는 "노래의 잔향"
## 태우지 않았으면 일반 인카운터 구역, 태웠으면 흐릿한 빛이 캠프파이어 주변을 맴돈다.
func _setup_perception_nodes() -> void:
	# 태운 기억의 잔향 — 캠프 구역 근처 포인트라이트 + 파티클
	var echo = Node2D.new()
	echo.name = "SongEcho"
	echo.position = Vector2(11 * TILE_SIZE, 15 * TILE_SIZE)
	echo.add_to_group("perception_burned_daily_campfire_song")
	add_child(echo)

	var light = PointLight2D.new()
	var tex = GradientTexture2D.new()
	var gradient = Gradient.new()
	gradient.add_point(0.0, Color(1, 0.85, 0.55, 1))
	gradient.add_point(1.0, Color(1, 0.85, 0.55, 0))
	tex.gradient = gradient
	tex.width = 128
	tex.height = 128
	tex.fill = GradientTexture2D.FILL_RADIAL
	tex.fill_from = Vector2(0.5, 0.5)
	tex.fill_to = Vector2(1.0, 0.5)
	light.texture = tex
	light.energy = 0.8
	light.color = Color(1, 0.7, 0.4)
	echo.add_child(light)

	# 부유하는 재색 파티클 (잔향의 기억)
	var particles = GPUParticles2D.new()
	particles.amount = 12
	particles.lifetime = 4.0
	particles.preprocess = 2.0
	var pmat = ParticleProcessMaterial.new()
	pmat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	pmat.emission_sphere_radius = 40.0
	pmat.direction = Vector3(0, -1, 0)
	pmat.initial_velocity_min = 8.0
	pmat.initial_velocity_max = 16.0
	pmat.gravity = Vector3(0, -3, 0)
	pmat.scale_min = 1.5
	pmat.scale_max = 3.0
	pmat.color = Color(1, 0.85, 0.6, 0.5)
	particles.process_material = pmat
	echo.add_child(particles)

	# 상호작용 Area — 다가가면 짧은 나레이션 (이미 태운 기억의 흔적)
	var area = Area2D.new()
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and not GameManager.get_flag("ch1_song_echo_heard"):
			GameManager.set_flag("ch1_song_echo_heard")
			NotificationToast.show_toast("A faint warmth. A song you no longer know.")
	)
	echo.add_child(area)

	# 엘리아 창백 틴트 — companions 그룹의 Elia 찾아 modulate 지정
	for npc in get_tree().get_nodes_in_group("companions"):
		if "npc_name" in npc and String(npc.npc_name) == "Elia":
			npc.set_meta("on_burned_tint_memory", "daily_campfire_song")
			npc.set_meta("on_burned_tint", Color(0.75, 0.8, 0.85, 0.85))
			break

## ===================== 히든 이벤트 =====================

func _setup_hidden_events() -> void:
	# S66 (Act I 데모): 핵심 3개만 — 기억 사당(테마 강화), 그루터기(A.E. 모멘트), 엘리아 기억 대화
	# 제거: dead_burner(잡 분위기), forest_walk(중복), anchor_talk(중복), MemoryResonance(미니게임 잡요소)

	# 그루터기 — 'A. E.' 각인. Act I 정체성 직결.
	_add_hidden_trigger(
		Vector2(20 * TILE_SIZE, 13 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		DIALOGUE_FILE, "hidden_stump", "hidden_ch1_stump"
	)
	# 기억 사당 — 세계관 확립
	_add_hidden_trigger(
		Vector2(3 * TILE_SIZE, 3 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		DIALOGUE_FILE, "forest_shrine", "ch1_shrine_found"
	)
	# 엘리아 — 기억 이야기. 관계성 핵심 비트.
	_add_hidden_trigger(
		Vector2(10 * TILE_SIZE, 12 * TILE_SIZE),
		Vector2(TILE_SIZE * 3, TILE_SIZE * 2),
		DIALOGUE_FILE, "elia_memory_talk", "ch1_elia_memory"
	)
	# Memory Resonance, dead_burner, forest_walk, anchor_talk 비활성 — Act I 흐름 짧고 강하게.

func _add_hidden_trigger(pos: Vector2, size: Vector2, dialogue_file: String, dialogue_key: String, flag_name: String) -> void:
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
			DialogueManager.load_and_start(dialogue_file, dialogue_key)
			# 히든 이벤트 업적
			if flag_name == "hidden_ch1_stump":
				AchievementManager.unlock("hidden_stump")
	)
	add_child(area)

## ===================== S59: 인터랙티브 프롭 =====================

func _setup_interactive_props() -> void:
	# 숲 길가 나무통 (Grains)
	MapEffects.add_interactive_prop(self, Vector2(6 * TILE_SIZE, 8 * TILE_SIZE), "barrel")
	# 쓰러진 나무 옆 상자 (아이템)
	MapEffects.add_interactive_prop(self, Vector2(16 * TILE_SIZE, 5 * TILE_SIZE), "crate")
	# 캠프파이어 — 남쪽 길 근처 (HP 회복)
	MapEffects.add_interactive_prop(self, Vector2(12 * TILE_SIZE, 13 * TILE_SIZE), "campfire", {"heal": 5})

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	# 숨겨진 상자 — 덤불 속 (좌측 하단 덤불 근처)
	_add_chest(
		Vector2(4 * TILE_SIZE, 13 * TILE_SIZE),
		"chest_rim_bush",
		{"items": {"potion": 2}, "grains": 10}
	)
	# 기억 단서 — 길 옆 돌무더기 (중앙 하단)
	_add_clue(
		Vector2(10 * TILE_SIZE, 14 * TILE_SIZE),
		"clue_rim_stone",
		"A worn stone with scratches. Someone counted days here."
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
	# 금색 인디케이터
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
	# 청색 인디케이터
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

## ===================== 사이드 퀘스트 =====================

func _setup_side_quests() -> void:
	if not SideQuest.is_available("echoes_ash") and not SideQuest.is_active("echoes_ash"):
		return
	# NPC: Ashen Figure (나무 그루터기 근처)
	if not SideQuest.is_complete("echoes_ash"):
		_add_quest_npc(
			Vector2(20 * TILE_SIZE, 10 * TILE_SIZE),
			"Ashen Figure",
			Color(0.5, 0.5, 0.5, 0.4),
			"echoes_ash"
		)
	# 기억 조각 1 (이끼 돌 근처)
	if SideQuest.is_active("echoes_ash") and not GameManager.get_flag("sq_echoes_ash_frag1"):
		_add_quest_clue(
			Vector2(6 * TILE_SIZE, 5 * TILE_SIZE),
			"sq_echoes_ash_frag1",
			"echoes_ash"
		)
	# 기억 조각 2 (쓰러진 나무 근처)
	if SideQuest.is_active("echoes_ash") and not GameManager.get_flag("sq_echoes_ash_frag2"):
		_add_quest_clue(
			Vector2(18 * TILE_SIZE, 13 * TILE_SIZE),
			"sq_echoes_ash_frag2",
			"echoes_ash"
		)

func _add_quest_npc(pos: Vector2, npc_name: String, _color: Color, quest_id: String) -> void:
	var area = Area2D.new()
	area.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 1.5, TILE_SIZE * 1.5)
	shape.shape = rect
	area.add_child(shape)
	# S55: NPC 비주얼 — PixelSprite 프리셋 사용
	var sprite = PixelSprite.create_npc_sprite("traveler")
	area.add_child(sprite)
	# "!" 퀘스트 마커
	var marker = Label.new()
	marker.text = "!" if not SideQuest.is_active(quest_id) else "?"
	marker.add_theme_font_size_override("font_size", 16)
	marker.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
	marker.position = Vector2(-4, -TILE_SIZE)
	area.add_child(marker)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			if not SideQuest.is_active(quest_id) and SideQuest.is_available(quest_id):
				# 퀘스트 시작
				SideQuest.advance_step(quest_id, "sq_%s_started" % quest_id)
				DialogueManager.load_and_start(DIALOGUE_FILE, "sq_%s_start" % quest_id)
			elif SideQuest.is_active(quest_id):
				var step = SideQuest.get_current_step(quest_id)
				var quest = SideQuest._find_quest(quest_id)
				var steps = quest["steps"] as Array
				if step >= steps.size() - 1:
					# 마지막 단계: 완료
					SideQuest.advance_step(quest_id, steps[steps.size() - 1]["flag"])
					DialogueManager.load_and_start(DIALOGUE_FILE, "sq_%s_complete" % quest_id)
					sprite.queue_free()
					marker.queue_free()
				else:
					NotificationToast.show_toast("Find the memory fragments.", NotificationToast.ToastType.INFO)
	)
	add_child(area)

func _add_quest_clue(pos: Vector2, flag_name: String, quest_id: String) -> void:
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
	indicator.size = Vector2(TILE_SIZE * 0.6, TILE_SIZE * 0.6)
	indicator.position = -Vector2(TILE_SIZE * 0.3, TILE_SIZE * 0.3)
	indicator.color = Color(0.8, 0.7, 0.4, 0.35)
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			SideQuest.advance_step(quest_id, flag_name)
			AudioManager.play_sfx("memory_add")
			DialogueManager.load_and_start(DIALOGUE_FILE, "sq_echoes_ash_frag")
			indicator.queue_free()
	)
	add_child(area)

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 빛나는 버섯 (나무 가장자리 풀밭)
	var mushroom_positions = [
		Vector2(3, 5), Vector2(19, 3), Vector2(5, 11), Vector2(21, 9),
	]
	for i in range(mushroom_positions.size()):
		var pos = mushroom_positions[i]
		var m = ColorRect.new()
		m.size = Vector2(6, 6)
		m.position = pos * TILE_SIZE + Vector2(randf_range(4, 20), randf_range(4, 20))
		m.color = Color(0.3, 0.7, 0.5, 0.25)
		m.z_index = -1
		m.set_meta("phase", float(i) * 1.5)
		add_child(m)
		_mushroom_lights.append(m)

	# 쓰러진 나무줄기 (장식)
	for log_pos in [Vector2(7, 12), Vector2(16, 4)]:
		var log = ColorRect.new()
		log.size = Vector2(TILE_SIZE * 2.5, TILE_SIZE * 0.4)
		log.position = log_pos * TILE_SIZE + Vector2(4, 16)
		log.color = Color(0.15, 0.1, 0.06, 0.35)
		log.z_index = -1
		add_child(log)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.18, 0.28, 0.15), "detail": "grass"},   # 0: GRASS
		{"color": Color(0.35, 0.28, 0.2), "detail": "path"},     # 1: PATH
		{"color": Color(0.08, 0.12, 0.08), "detail": "tree"},    # 2: TREE
		{"color": Color(0.22, 0.32, 0.18), "detail": "bush"},    # 3: BUSH
		{"color": Color(0.12, 0.15, 0.25), "detail": "water"},   # 4: WATER
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	# 충돌 (나무, 물)
	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.TREE, Tile.WATER])
	for body in bodies:
		add_child(body)
		collision_bodies.append(body)

	# 미니맵
	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 9 * TILE_SIZE + TILE_SIZE / 2.0)
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		SaveManager.loaded_player_pos = {}

## ===================== 전투 트리거 =====================

func _setup_battle_triggers() -> void:
	# S66 (Act I 데모): 보스(Void Beast) 1전만. Ash Crawler 잡몹 제거 — 클라이맥스 집중.
	_add_battle_area(
		Vector2(16 * TILE_SIZE, 7 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Beast", 80, 15, true,
		"res://assets/cg/generated/story_ch1_twisted_forest_path.png", "res://assets/cg/generated/cinematic_void_beast_memory_devour.png"
	)
	# 보스 처치 시 ch1_void_beast_defeated 플래그 (캠프 진행 잠금 해제용)
	if not BattleManager.battle_ended.is_connected(_on_act1_battle_ended):
		BattleManager.battle_ended.connect(_on_act1_battle_ended)

func _on_act1_battle_ended(result) -> void:
	if result == BattleManager.BattleState.VICTORY:
		GameManager.set_flag("ch1_void_beast_defeated")

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
	indicator.color = Color(0.5, 0.1, 0.1, 0.15) if not is_void else Color(0.3, 0.05, 0.3, 0.2)
	indicator.z_index = -1
	area.add_child(indicator)

	_battle_counter += 1
	var flag_name = "battle_rim_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			_trigger_battle(enemy_name, hp, atk, is_void, bg_img, e_img)
	)

	add_child(area)

func _setup_random_encounters() -> void:
	# Ch1 완료 후 재방문 시에만 활성화
	if not GameManager.get_flag("ch1_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Ash Crawler", "hp": 45, "atk": 10, "is_void": false, "abilities": [], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": "res://assets/cg/game_image/void_beast_confrontation.png"},
			{"name": "Forest Shade", "hp": 55, "atk": 12, "is_void": false, "abilities": ["poison"], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": ""},
			{"name": "Void Beast", "hp": 80, "atk": 15, "is_void": true, "abilities": ["drain"], "bg": "res://assets/cg/generated/story_ch1_twisted_forest_path.png", "img": "res://assets/cg/generated/cinematic_void_beast_memory_devour.png"},
		],
		"res://scenes/maps/rim_forest.tscn", "", "", 50, 90
	)

func _trigger_battle(enemy_name: String, hp: int, atk: int, is_void: bool, bg_img: String = "", e_img: String = "") -> void:
	var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
	BattleManager.start_battle(enemy, "res://scenes/maps/rim_forest.tscn", bg_img, e_img)
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
