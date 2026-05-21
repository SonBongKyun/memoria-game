## The Seam — 더 씸 (Chapter 6: Thread That Holds)
## 절벽 사이 숨겨진 컬러풀한 정착촌. 세이블의 거점.
## 세이블과의 대화 → BL-07 탐사 계획 → 보이드 홀 조사.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18
const DIALOGUE_FILE: String = "res://data/chapter6_dialogue.json"
const EPILOGUE_FILE: String = "res://data/epilogue_dialogue.json"

enum Tile { STONE, CLIFF, HUT, GARDEN, PATH, WATER, LANTERN }

# 0=돌, 1=절벽(벽), 2=오두막, 3=정원, 4=길, 5=물(작은 개울), 6=랜턴(장식)
var map_data: Array = [
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
	[1,1,0,0,4,4,4,0,0,1,1,0,0,0,0,1,1,0,0,4,4,0,0,1,1],
	[1,0,0,3,3,4,0,0,0,1,0,0,3,3,0,0,1,0,0,4,0,0,0,0,1],
	[1,0,3,3,6,4,0,0,0,0,0,3,3,3,3,0,0,0,0,4,0,2,2,0,1],
	[1,0,0,3,0,4,0,2,2,0,0,0,6,0,0,0,0,0,0,4,0,2,2,0,1],
	[1,0,0,0,0,4,0,2,2,0,0,0,0,0,0,0,2,2,0,4,0,0,0,0,1],
	[1,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,2,2,0,4,4,0,0,0,1],
	[1,1,0,0,0,0,4,0,0,0,5,5,0,0,0,0,0,0,0,0,4,0,0,1,1],
	[1,0,0,2,2,0,4,0,0,5,5,5,5,0,0,0,0,0,0,0,4,0,0,0,1],
	[1,0,0,2,2,0,4,0,0,0,5,5,0,0,0,6,0,0,0,0,4,0,0,0,1],
	[1,0,0,0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0,4,4,0,0,0,1],
	[1,0,0,0,6,0,0,4,0,0,0,0,2,2,0,0,0,0,4,4,0,0,0,0,1],
	[1,0,0,0,0,0,0,4,0,0,0,0,2,2,0,0,3,3,4,0,0,0,0,1,1],
	[1,1,0,0,0,0,0,4,4,0,0,0,0,0,0,3,3,6,4,0,0,0,0,0,1],
	[1,0,0,0,2,2,0,0,4,4,0,0,0,0,0,0,3,0,4,0,2,2,0,0,1],
	[1,0,0,0,2,2,0,0,0,4,4,0,0,0,0,0,0,0,4,0,2,2,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,4,4,0,0,0,0,0,1],
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
]

var tile_colors: Dictionary = {
	Tile.STONE: Color(0.3, 0.28, 0.26),
	Tile.CLIFF: Color(0.15, 0.13, 0.12),
	Tile.HUT: Color(0.42, 0.3, 0.22),
	Tile.GARDEN: Color(0.15, 0.4, 0.2),
	Tile.PATH: Color(0.45, 0.38, 0.3),
	Tile.WATER: Color(0.12, 0.25, 0.45),
	Tile.LANTERN: Color(0.7, 0.55, 0.2),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia
@onready var sable_npc: StaticBody2D = $Sable

var water_shimmers: Array[ColorRect] = []
var lantern_lights: Array[ColorRect] = []
var effect_time: float = 0.0
var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null
var _crystal_lights: Array[ColorRect] = []
var _point_lights: Array[PointLight2D] = []  # S42
var _occluders: Array[LightOccluder2D] = []  # S52
var _s52_particles: Array[ColorRect] = []  # S52
var _camera: Camera2D = null  # S52
var _fog_layer: Array[ColorRect] = []  # S59

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	MapEffects.add_burn_desaturation(self)  # S46: 기억 연소 월드 탈색
	MapEffects.add_fireflies(self, 20, Color(0.85, 0.65, 0.3, 0.5))  # S46: Seam 앰버 반딧불
	# S42: 패럴랙스 + 조명
	MapEffects.add_parallax_background(self, {"sky": Color(0.06, 0.06, 0.1), "far": Color(0.1, 0.1, 0.15), "mid": Color(0.15, 0.12, 0.1), "biome": "forest", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.4, 0.38, 0.45))
	_point_lights = MapEffects.add_tile_lights(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.LANTERN, Color(1.0, 0.85, 0.5))
	# S43: 횃불 불꽃 파티클
	MapEffects.add_fire_particles(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.LANTERN)
	# S52: 그래픽 업그레이드
	MapEffects.enable_shadows_on_lights(_point_lights)
	_occluders = MapEffects.add_tile_occluders(self, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.CLIFF])
	MapEffects.add_color_grading(self, {"tint": Color(0.4, 0.3, 0.45), "brightness": -0.03})
	MapEffects.add_illustration_atmosphere(self, "res://assets/cg/game_image/tobias_memory_corridor.png", 0.11, Color(0.9, 0.78, 1.0))
	_s52_particles = MapEffects.add_pollen_particles(self, 8, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), Color(0.7, 0.5, 0.3, 0.2))
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	# S59: 분위기 강화 — 은은한 안개 + 깊이 그라디언트
	_fog_layer = MapEffects.add_fog_layer(self, 0.3, Color(0.25, 0.25, 0.35, 0.04), 1.5)
	MapEffects.add_depth_gradient(self, 0.06)
	_position_player()
	_setup_effects()
	_setup_hidden_events()
	_setup_exploration_events()
	MemoryManager.add_chapter_memories(6)
	_setup_random_encounters()
	_setup_puzzle_trigger()
	_setup_interactive_objects()
	_setup_side_quests()
	_setup_map_decorations()
	AchievementManager.record_map_visit("the_seam")
	elia.repeat_line = "I'm here. I'm not going anywhere."
	sable_npc.repeat_line = "The Seam holds. For now."
	print("[TheSeam] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	if GameManager.current_chapter >= 11 and GameManager.get_flag("ch10_complete"):
		# 에필로그 — Ch10 완료 후 복귀
		AudioManager.play_bgm("res://assets/audio/bgm/epilogue.mp3")
		await get_tree().create_timer(1.0).timeout
		_start_epilogue()
	elif GameManager.get_flag("ch6_bl07_entered") and not GameManager.get_flag("ch6_complete"):
		# 보스전 후 복귀
		_setup_battle_triggers()
		if GameManager.player_data.hp > int(GameManager.player_data.max_hp * 0.3):
			# 승리로 복귀 — 에필로그 시작
			await get_tree().create_timer(1.0).timeout
			_start_ch6_epilogue()
		else:
			# 패배로 복귀 — 보스전 재도전 가능하도록 플래그 리셋
			GameManager.set_flag("ch6_bl07_entered", false)
			_setup_bl07_trigger()
	elif not GameManager.get_flag("ch6_arrived"):
		_setup_battle_triggers()
		_setup_bl07_trigger()
		# 엘리아 분리 상태 체크 — 분리 시 Seam 도착 전 재합류
		if GameManager.get_flag("elia_separates") and not GameManager.get_flag("elia_reunited"):
			elia.visible = false
			elia.set_physics_process(false)
		# 챕터 타이틀 카드 표시 후 대화 시작
		await MapEffects.show_chapter_title(self, 6, "The Seam", "Between what was and what will be")
		await get_tree().create_timer(0.3).timeout
		# 재합류 이벤트
		if GameManager.get_flag("elia_separates") and not GameManager.get_flag("elia_reunited"):
			_start_reunion()
		else:
			_start_ch6_sequence()
	else:
		_setup_battle_triggers()
		_setup_bl07_trigger()

func _process(delta: float) -> void:
	effect_time += delta
	MapEffects.update_water_shimmer(water_shimmers, effect_time)
	MapEffects.update_lantern_lights(lantern_lights, effect_time)
	MapEffects.update_point_lights(_point_lights, effect_time)
	MapEffects.update_pollen(_s52_particles, effect_time, delta)
	MapEffects.update_camera_shake(_camera, effect_time)
	# S59: 안개 + 트리거 글로우
	MapEffects.update_fog_layer(_fog_layer, effect_time)
	MapEffects.update_trigger_approach_glow(self, player.position, effect_time)
	var elia_vis = elia.visible if elia else false
	var elia_pos = elia.position if elia else Vector2.ZERO
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia_pos, elia_vis)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	# 크리스탈 반짝임
	for c in _crystal_lights:
		var phase = c.get_meta("phase", 0.0)
		c.color.a = 0.15 + sin(effect_time * 2.5 + phase) * 0.1 + (0.3 if fmod(effect_time + phase, 4.0) < 0.15 else 0.0)
	# S53: NPC 아이들 모션
	for npc in get_tree().get_nodes_in_group("npcs"):
		if npc.has_node("AnimatedSprite2D"):
			var spr = npc.get_node("AnimatedSprite2D")
			spr.scale = Vector2(1.0 + sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.008, 1.0 - sin(effect_time * 1.5 + npc.position.x * 0.1) * 0.006)

func _setup_hidden_events() -> void:
	# 숨겨진 정원 — 좌상단 정원 타일 영역 (3,2 근처)
	var area = Area2D.new()
	area.position = Vector2(3.5 * TILE_SIZE, 2.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	shape.shape = rect
	area.add_child(shape)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag("hidden_ch6_garden"):
			GameManager.set_flag("hidden_ch6_garden")
			AchievementManager.unlock("hidden_garden")
			DialogueManager.load_and_start(DIALOGUE_FILE, "hidden_garden")
	)
	add_child(area)

## ===================== S48: 탐색 이벤트 =====================

func _setup_exploration_events() -> void:
	# 저녁 풍경 대화 (정원 근처)
	_add_story_trigger(Vector2(5 * TILE_SIZE, 5 * TILE_SIZE), Vector2(TILE_SIZE * 3, TILE_SIZE * 2), "seam_evening", "ch6_evening")
	# 세이블 과거 이야기 (세이블 집 근처)
	_add_story_trigger(Vector2(18 * TILE_SIZE, 4 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "sable_past", "ch6_sable_past")
	# 마을 주민 대화 (중앙 거리)
	_add_story_trigger(Vector2(12 * TILE_SIZE, 8 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "seam_residents", "ch6_residents")
	# 엘리아 밤 대화 (북쪽 절벽 위)
	_add_story_trigger(Vector2(10 * TILE_SIZE, 2 * TILE_SIZE), Vector2(TILE_SIZE * 3, TILE_SIZE * 2), "elia_night_talk", "ch6_elia_night")
	# BL-07 출발 전 세이블 준비 (남쪽 근처)
	if GameManager.get_flag("ch6_briefing_done") and not GameManager.get_flag("ch6_bl07_entered"):
		_add_story_trigger(Vector2(12 * TILE_SIZE, 14 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "sable_preparation", "ch6_sable_prep")
	# 플래시백: 상실의 날 기억
	_add_story_trigger(Vector2(16 * TILE_SIZE, 10 * TILE_SIZE), Vector2(TILE_SIZE * 2, TILE_SIZE * 2), "arrel_flashback_loss", "ch6_flashback_loss")
	# S51: 기억 공명 지점
	MemoryResonance.setup_points(self, "the_seam")

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

func _setup_effects() -> void:
	water_shimmers = MapEffects.add_water_shimmer(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.WATER)
	lantern_lights = MapEffects.add_lantern_lights(self, map_data, MAP_WIDTH, MAP_HEIGHT, Tile.LANTERN)
	MapEffects.add_snow(self, 0.6)

## ===================== 엘리아 재합류 =====================

func _start_reunion() -> void:
	GameManager.set_flag("elia_reunited")
	GameManager.player_data.elia_with_party = true
	DialogueManager.dialogue_ended.connect(_on_reunion_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start("res://data/chapter5_dialogue.json", "elia_reunion")

func _on_reunion_ended() -> void:
	# 엘리아 다시 표시
	elia.visible = true
	elia.set_physics_process(true)
	elia.position = player.position + Vector2(-30, 20)
	print("[TheSeam] Elia reunited — anchor restored")
	await get_tree().create_timer(0.5).timeout
	_start_ch6_sequence()

## ===================== 스토리 시퀀스 =====================

func _start_ch6_sequence() -> void:
	GameManager.set_flag("ch6_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	# 엘리아 분리 시 솔로 대사
	if GameManager.get_flag("elia_separates") and not GameManager.get_flag("elia_reunited"):
		DialogueManager.load_and_start(DIALOGUE_FILE, "seam_welcome_solo")
	else:
		DialogueManager.load_and_start(DIALOGUE_FILE, "seam_welcome")

## ===================== Ch6 에필로그 =====================

func _start_epilogue() -> void:
	if GameManager.get_flag("epilogue_started"):
		# 에필로그 이미 완료 — NPC 대화만 활성화
		_setup_epilogue_npcs()
		return
	GameManager.set_flag("epilogue_started")

	var epilogue_key: String
	if GameManager.get_flag("zero_burn_path"):
		# Zero Burn — 이름을 잃은 아렐
		epilogue_key = "epilogue_zero_burn"
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 4:
		# Ash — 기억을 너무 많이 태운 아렐 (이름은 지켰지만 껍데기만 남음)
		epilogue_key = "epilogue_ash"
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("hidden_ch1_stump") and GameManager.get_flag("hidden_ch6_garden"):
		# Seam — 숨겨진 아름다움을 발견한 아렐 (희망의 비밀 엔딩)
		epilogue_key = "epilogue_seam"
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("tobias_joined") and GameManager.get_flag("ch9_kairos_battle"):
		# Tobias — 토비아스가 연구를 완성하는 엔딩
		epilogue_key = "epilogue_tobias"
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 8:
		# Hollow — 아렐이 거의 텅 빈 엔딩
		epilogue_key = "epilogue_hollow"
	else:
		# Preservation — 이름을 지킨 아렐 (기본 보존 엔딩)
		epilogue_key = "epilogue_preservation"

	DialogueManager.dialogue_ended.connect(_on_epilogue_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(EPILOGUE_FILE, epilogue_key)

func _on_epilogue_ended() -> void:
	GameManager.set_flag("epilogue_complete")
	# 엔딩별 업적 기록 + S54: Ending Gallery 기록
	if GameManager.get_flag("zero_burn_path"):
		AchievementManager.unlock("ending_zero_burn")
		GameManager.record_ending("zero_burn")
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("tobias_joined") and GameManager.get_flag("ch9_kairos_battle"):
		AchievementManager.unlock("ending_tobias")
		GameManager.record_ending("tobias")
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 8:
		AchievementManager.unlock("ending_hollow")
		GameManager.record_ending("hollow")
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 4:
		AchievementManager.unlock("ending_ash")
		GameManager.record_ending("ash")
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("hidden_ch1_stump") and GameManager.get_flag("hidden_ch6_garden"):
		AchievementManager.unlock("ending_seam")
		GameManager.record_ending("seam")
	else:
		AchievementManager.unlock("ending_preservation")
		GameManager.record_ending("preservation")
	_setup_epilogue_npcs()
	print("[TheSeam] Epilogue complete — talk to Elia or Sable")

func _setup_epilogue_npcs() -> void:
	# 대화 종료 시 크레딧 체크 (매 대화마다, 중복 연결 방지)
	if not DialogueManager.dialogue_ended.is_connected(_check_credits_trigger):
		DialogueManager.dialogue_ended.connect(_check_credits_trigger)
	# 엘리아 대화 트리거
	_add_npc_talk_area(
		elia.position,
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		EPILOGUE_FILE, "elia_epilogue_talk", "epilogue_elia_talked"
	)
	# 세이블 대화 트리거
	_add_npc_talk_area(
		sable_npc.position,
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		EPILOGUE_FILE, "sable_epilogue_talk", "epilogue_sable_talked"
	)

func _add_npc_talk_area(pos: Vector2, size: Vector2, dialogue_file: String, dialogue_key: String, flag_name: String) -> void:
	var area = Area2D.new()
	area.position = pos
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = size
	shape.shape = rect
	area.add_child(shape)

	area.body_entered.connect(func(body):
		if body.name == "Player" and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			DialogueManager.load_and_start(dialogue_file, dialogue_key)
	)
	add_child(area)

## 에필로그 NPC 둘 다 대화 완료 시 크레딧 진입
func _check_credits_trigger() -> void:
	if GameManager.get_flag("epilogue_elia_talked") and GameManager.get_flag("epilogue_sable_talked"):
		DialogueManager.dialogue_ended.disconnect(_check_credits_trigger)
		await get_tree().create_timer(2.0).timeout
		SceneTransition.change_scene("res://scenes/ui/credits.tscn")

func _on_arrival_ended() -> void:
	# 짧은 딜레이 후 세이블 브리핑
	await get_tree().create_timer(1.5).timeout
	if not GameManager.get_flag("ch6_briefing_done"):
		GameManager.set_flag("ch6_briefing_done")
		DialogueManager.dialogue_ended.connect(_on_briefing_ended, CONNECT_ONE_SHOT)
		DialogueManager.load_and_start(DIALOGUE_FILE, "sable_briefing")

func _on_briefing_ended() -> void:
	# 세이블 파티 합류
	GameManager.set_flag("sable_joined")
	# 자유 탐색 — BL-07 입구 트리거 활성
	print("[TheSeam] Briefing complete — Sable joined. Explore or head to BL-07")

## ===================== BL-07 탐사 트리거 (남쪽 중앙) =====================

func _setup_bl07_trigger() -> void:
	var area = Area2D.new()
	area.position = Vector2(12 * TILE_SIZE, 16.5 * TILE_SIZE)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	shape.shape = rect
	area.add_child(shape)

	# 시각적 표시 — 어두운 보라색 (보이드 에너지)
	var indicator = ColorRect.new()
	indicator.size = Vector2(TILE_SIZE * 3, TILE_SIZE)
	indicator.position = Vector2(-TILE_SIZE * 1.5, -TILE_SIZE * 0.5)
	indicator.color = Color(0.25, 0.05, 0.35, 0.3)
	indicator.z_index = -1
	area.add_child(indicator)

	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.get_flag("ch6_briefing_done") and not GameManager.get_flag("ch6_bl07_entered"):
			_enter_bl07()
	)
	add_child(area)

func _enter_bl07() -> void:
	GameManager.set_flag("ch6_bl07_entered")
	DialogueManager.dialogue_ended.connect(_on_bl07_dialogue_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "bl07_entrance")

func _on_bl07_dialogue_ended() -> void:
	# BL-07 보스전: Shade Sentinel
	# 보스전 후 return_scene으로 돌아오면 _ready()에서 자동 감지
	var boss = BattleManager.Enemy.new("Shade Sentinel", 180, 24, true)
	boss.is_boss = true
	boss.abilities = ["drain", "shield", "multi_hit", "summon"]
	boss.weakness = "void"
	boss.resistance = "fire"
	BattleManager.start_battle(boss, "res://scenes/maps/the_seam.tscn", "res://assets/cg/game_image/nera_void_cavern.png", "res://assets/cg/game_image/void_beast_confrontation.png")
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")

func _start_ch6_epilogue() -> void:
	if GameManager.get_flag("ch6_complete"):
		return
	GameManager.set_flag("ch6_complete")
	DialogueManager.dialogue_ended.connect(_on_ch6_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "bl07_aftermath")

func _on_ch6_ended() -> void:
	GameManager.current_chapter = 7
	SaveManager.autosave_on_chapter_transition()
	print("[TheSeam] Chapter 6 complete — heading to Seam Outskirts")
	await get_tree().create_timer(1.5).timeout
	# S58: Chapter completion screen with stats summary
	SceneTransition.change_scene_chapter_complete("res://scenes/maps/seam_outskirts.tscn", 6)

## ===================== 전투 트리거 (마을 외곽) =====================

func _setup_battle_triggers() -> void:
	# Void Wraith — 마을 외곽 순찰하는 약한 적
	_add_battle_area(
		Vector2(3 * TILE_SIZE, 3 * TILE_SIZE),
		Vector2(TILE_SIZE * 2, TILE_SIZE * 2),
		"Void Wraith", 90, 18, true,
		"res://assets/cg/game_image/sealed_city_ruins.png", "res://assets/cg/game_image/void_beast_confrontation.png"
	)

func _setup_puzzle_trigger() -> void:
	if not GameManager.get_flag("ch6_complete"):
		return
	var area = Area2D.new()
	area.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 14 * TILE_SIZE + TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	shape.shape = rect
	area.add_child(shape)
	var indicator = ColorRect.new()
	indicator.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	indicator.position = -Vector2(TILE_SIZE, TILE_SIZE)
	indicator.color = Color(0.3, 0.3, 0.5, 0.15)
	indicator.z_index = -1
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			MemoryPuzzle.open_puzzle(5, 20)
	)
	add_child(area)

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch6_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Void Wraith", "hp": 90, "atk": 18, "is_void": true, "abilities": ["drain", "weaken"], "bg": "res://assets/cg/game_image/sealed_gate_plaza.png", "img": "res://assets/cg/game_image/void_beast_confrontation.png"},
			{"name": "Seam Lurker", "hp": 110, "atk": 20, "is_void": true, "abilities": ["poison", "shield"], "bg": "res://assets/cg/game_image/sealed_gate_plaza.png"},
		],
		"res://scenes/maps/the_seam.tscn", "", "", 45, 80
	)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	# 상자 — 정원 구석 (우상단 오두막 뒤)
	_add_chest(
		Vector2(21 * TILE_SIZE, 3 * TILE_SIZE),
		"chest_seam_garden",
		{"items": {"hi_potion": 1, "smoke_bomb": 1}, "grains": 20}
	)
	# 단서 — 개울 옆 (세이블 관련)
	_add_clue(
		Vector2(10 * TILE_SIZE, 8 * TILE_SIZE),
		"clue_seam_stream",
		"Water that flows upward. The Seam bends even simple things."
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

## ===================== 사이드 퀘스트 =====================

func _setup_side_quests() -> void:
	if not SideQuest.is_available("sable_vigil") and not SideQuest.is_active("sable_vigil"):
		return
	if SideQuest.is_complete("sable_vigil"):
		return
	# 세이블 NPC에 퀘스트 마커
	var q_marker = Label.new()
	q_marker.text = "!" if not SideQuest.is_active("sable_vigil") else "?"
	q_marker.add_theme_font_size_override("font_size", 16)
	q_marker.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
	q_marker.position = Vector2(-4, -TILE_SIZE * 1.5)
	if sable_npc:
		sable_npc.add_child(q_marker)

	# 세이블 대화 트리거 확장 (기존 interact 보완)
	var quest_area = Area2D.new()
	quest_area.position = sable_npc.position if sable_npc else Vector2(6 * TILE_SIZE, 5 * TILE_SIZE)
	quest_area.collision_layer = 0
	quest_area.collision_mask = 2
	var qs = CollisionShape2D.new()
	var qr = RectangleShape2D.new()
	qr.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	qs.shape = qr
	quest_area.add_child(qs)
	quest_area.body_entered.connect(func(body):
		if body.name != "Player" or GameManager.current_state != GameManager.GameState.EXPLORATION:
			return
		if SideQuest.is_available("sable_vigil"):
			SideQuest.advance_step("sable_vigil", "sq_sable_vigil_started")
			DialogueManager.load_and_start("res://data/chapter6_dialogue.json", "sq_sable_vigil_start")
		elif SideQuest.is_active("sable_vigil") and GameManager.get_flag("sq_sable_vigil_killed"):
			SideQuest.advance_step("sable_vigil", "sq_sable_vigil_complete")
			DialogueManager.load_and_start("res://data/chapter6_dialogue.json", "sq_sable_vigil_complete")
			q_marker.queue_free()
	)
	add_child(quest_area)

	# Void Watcher 전투 트리거 (퀘스트 진행 중에만)
	if SideQuest.is_active("sable_vigil") and not GameManager.get_flag("sq_sable_vigil_killed"):
		var vw_area = Area2D.new()
		vw_area.position = Vector2(22 * TILE_SIZE + TILE_SIZE / 2.0, 14 * TILE_SIZE + TILE_SIZE / 2.0)
		vw_area.collision_layer = 0
		vw_area.collision_mask = 2
		var vs = CollisionShape2D.new()
		var vr = RectangleShape2D.new()
		vr.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
		vs.shape = vr
		vw_area.add_child(vs)
		# S55: NPC 비주얼 — PixelSprite 프리셋 사용 (보이드 요원)
		var v_sprite = PixelSprite.create_npc_sprite("bureau_agent")
		v_sprite.modulate = Color(0.6, 0.3, 0.8, 0.8)  # 보이드 보라색 틴트
		vw_area.add_child(v_sprite)
		vw_area.body_entered.connect(func(body):
			if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag("sq_sable_vigil_killed"):
				GameManager.set_flag("sq_sable_vigil_killed")
				var enemy = BattleManager.Enemy.new("Void Watcher", 120, 22, true)
				enemy.abilities = ["drain", "shield"]
				enemy.weakness = "void"
				BattleManager.start_battle(enemy, "res://scenes/maps/the_seam.tscn", "res://assets/cg/game_image/nera_void_cavern.png", "res://assets/cg/game_image/void_beast_confrontation.png")
				SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
		)
		add_child(vw_area)

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 크리스탈 형성 (돌 타일 근처)
	var crystal_positions = [
		Vector2(8, 3), Vector2(15, 6), Vector2(20, 10), Vector2(5, 14),
	]
	for i in range(crystal_positions.size()):
		var pos = crystal_positions[i]
		var c = ColorRect.new()
		c.size = Vector2(5, 8)
		c.position = pos * TILE_SIZE + Vector2(randf_range(4, 20), randf_range(4, 20))
		c.color = Color(0.7, 0.4, 0.6, 0.2)
		c.z_index = -1
		c.set_meta("phase", float(i) * 2.0)
		add_child(c)
		_crystal_lights.append(c)

	# 덩굴 (절벽 가장자리에서 늘어지는)
	for vine_pos in [Vector2(3, 7), Vector2(18, 4), Vector2(12, 12)]:
		var vine = ColorRect.new()
		vine.size = Vector2(2, TILE_SIZE * 1.5)
		vine.position = vine_pos * TILE_SIZE + Vector2(14, 0)
		vine.color = Color(0.2, 0.35, 0.15, 0.25)
		vine.z_index = -1
		add_child(vine)

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.3, 0.28, 0.26), "detail": "stone"},     # 0: STONE
		{"color": Color(0.15, 0.13, 0.12), "detail": "cliff"},    # 1: CLIFF
		{"color": Color(0.42, 0.3, 0.22), "detail": "hut"},       # 2: HUT
		{"color": Color(0.15, 0.4, 0.2), "detail": "garden"},     # 3: GARDEN
		{"color": Color(0.45, 0.38, 0.3), "detail": "path"},      # 4: PATH
		{"color": Color(0.12, 0.25, 0.45), "detail": "water"},    # 5: WATER
		{"color": Color(0.7, 0.55, 0.2), "detail": "lantern"},    # 6: LANTERN
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.CLIFF, Tile.HUT])
	for body in bodies:
		add_child(body)

	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	if GameManager.current_chapter >= 11:
		# 에필로그 — The Seam 절벽 가장자리
		player.position = Vector2(10 * TILE_SIZE, 8 * TILE_SIZE)
		elia.position = Vector2(10 * TILE_SIZE - 30, 8 * TILE_SIZE + 20)
	else:
		# 북쪽 입구에서 시작 (절벽 사이 진입)
		player.position = Vector2(5 * TILE_SIZE, 1.5 * TILE_SIZE)
		elia.position = Vector2(5 * TILE_SIZE - 30, 1.5 * TILE_SIZE + 20)
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}

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
	var flag_name = "battle_seam_%d" % _battle_counter
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION and not GameManager.get_flag(flag_name):
			GameManager.set_flag(flag_name)
			var enemy = BattleManager.Enemy.new(enemy_name, hp, atk, is_void)
			BattleManager.start_battle(enemy, "res://scenes/maps/the_seam.tscn", bg_img, e_img)
			SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	)
	add_child(area)
