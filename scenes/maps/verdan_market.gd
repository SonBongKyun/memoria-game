## Verdan Market — 베르단 시장 (Chapter 2)
## 회색 벨트 최대 도시. 말렛과의 거래가 이루어지는 곳.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 30
const MAP_HEIGHT: int = 20
const DIALOGUE_FILE: String = "res://data/chapter2_dialogue.json"

enum Tile { STONE, WALL, STALL, DOOR, ALLEY }

# 0=돌바닥, 1=건물벽, 2=노점, 3=문/입구, 4=골목
var map_data: Array = [
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
	[1,0,0,0,0,0,1,0,0,0,2,0,0,0,0,2,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,1,1,1,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,4,4,0,0,0,0,4,4,0,0,0,0,0,0,2,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,1,0,0,4,4,0,0,0,0,4,4,0,0,1,1,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,1,1,0,0,0,4,4,3,3,4,4,0,0,0,1,1,0,0,0,0,0,0,0,1],
	[1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
	[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
]

# S43: 컬러 팔레트 리뉴얼
var tile_colors: Dictionary = {
	Tile.STONE: Color(0.32, 0.3, 0.28),  # 더 밝은 돌바닥
	Tile.WALL: Color(0.2, 0.17, 0.14),
	Tile.STALL: Color(0.42, 0.3, 0.18),  # 더 따뜻한 노점
	Tile.DOOR: Color(0.45, 0.35, 0.22),
	Tile.ALLEY: Color(0.12, 0.1, 0.1),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia
@onready var malet_npc: StaticBody2D = $Malet

var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null
var _lantern_lights: Array[ColorRect] = []
var _smoke_wisps: Array[Dictionary] = []
var _time: float = 0.0
var _point_lights: Array[PointLight2D] = []  # S42: 2D 조명

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	MapEffects.add_burn_desaturation(self)  # S46: 기억 연소 월드 탈색
	MapEffects.add_heat_haze(self, 0.0015)  # S46: 시장 훈기
	# S42: 패럴랙스 + 조명
	MapEffects.add_parallax_background(self, {"sky": Color(0.1, 0.08, 0.12), "far": Color(0.15, 0.12, 0.1), "mid": Color(0.2, 0.16, 0.14), "biome": "market", "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	MapEffects.add_ambient_lighting(self, Color(0.5, 0.45, 0.4))
	# 노점에 따뜻한 라이트
	for y in range(MAP_HEIGHT):
		for x in range(MAP_WIDTH):
			if y < map_data.size() and x < map_data[y].size() and map_data[y][x] == Tile.STALL:
				_point_lights.append(MapEffects.add_point_light(self, Vector2(x * TILE_SIZE + 16, y * TILE_SIZE + 16), Color(1.0, 0.8, 0.5), 0.6, 80.0))
	_position_player()
	_setup_random_encounters()
	_setup_puzzle_trigger()
	_setup_interactive_objects()
	_setup_side_quests()
	_setup_map_decorations()
	AchievementManager.record_map_visit("verdan_market")
	elia.repeat_line = "This market smells like rust and regret."
	malet_npc.repeat_line = "You know where to find me."
	print("[VerdenMarket] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	# Ch2 도착 대화 (첫 진입)
	if not GameManager.get_flag("ch2_arrived"):
		# 챕터 타이틀 카드 표시 후 대화 시작
		await MapEffects.show_chapter_title(self, 2, "Verdan Market", "Where memories are currency")
		await get_tree().create_timer(0.3).timeout
		_start_ch2_sequence()

func _process(delta: float) -> void:
	_time += delta
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia.position, elia.visible)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)
	MapEffects.update_point_lights(_point_lights, _time)
	# 랜턴 깜빡임
	for l in _lantern_lights:
		l.color.a = 0.3 + randf_range(-0.05, 0.05) + sin(_time * 3.0) * 0.05
	# 연기 상승
	for w in _smoke_wisps:
		var rect = w["rect"] as ColorRect
		rect.position.y -= delta * 8.0
		rect.color.a = maxf(0.0, rect.color.a - delta * 0.02)
		if rect.position.y < w["start_y"] - 60:
			rect.position.y = w["start_y"]
			rect.color.a = 0.08

## ===================== 스토리 시퀀스 =====================

func _start_ch2_sequence() -> void:
	GameManager.set_flag("ch2_arrived")
	DialogueManager.dialogue_ended.connect(_on_arrival_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "verdan_arrival")

func _on_arrival_ended() -> void:
	# 자유 탐색 — 말렛 대화 종료 감지
	DialogueManager.dialogue_ended.connect(_on_any_dialogue_ended)
	print("[VerdenMarket] Free exploration — talk to Malet in the Sump")

## 대화 종료 감지 — 말렛 거래 흐름 자동 연결
func _on_any_dialogue_ended() -> void:
	# malet_encounter 끝나면 → malet_deal 시작
	if GameManager.get_flag("malet_deal_accepted") or GameManager.get_flag("malet_deal_refused"):
		DialogueManager.dialogue_ended.disconnect(_on_any_dialogue_ended)
		await get_tree().create_timer(0.3).timeout
		if GameManager.get_flag("malet_deal_accepted"):
			# 거래 수락 → 추출 대화 → 보상
			DialogueManager.dialogue_ended.connect(_on_deal_ended, CONNECT_ONE_SHOT)
			DialogueManager.load_and_start(DIALOGUE_FILE, "malet_deal")
		else:
			# 거절
			DialogueManager.dialogue_ended.connect(_on_refused_ended, CONNECT_ONE_SHOT)
			DialogueManager.load_and_start(DIALOGUE_FILE, "malet_refused")

func _on_deal_ended() -> void:
	await get_tree().create_timer(0.5).timeout
	DialogueManager.dialogue_ended.connect(_on_reward_ended, CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(DIALOGUE_FILE, "malet_reward")

func _on_refused_ended() -> void:
	# 거절해도 재대화로 다시 시도 가능 — 플래그 초기화
	GameManager.story_flags.erase("malet_deal_refused")
	GameManager.story_flags.erase("talked_Malet_malet_encounter")
	malet_npc._talked_keys.erase("malet_encounter")
	DialogueManager.dialogue_ended.connect(_on_any_dialogue_ended)

func _on_reward_ended() -> void:
	GameManager.set_flag("ch2_malet_done")
	# 말렛 거래 보상: 아이템 지급
	GameManager.add_item("potion", 2)
	GameManager.add_item("antidote", 1)
	GameManager.add_item("firebomb", 1)
	# 보상 대화 후 상점 오픈 (Grains 거래 기회)
	_open_malet_shop()

## 말렛 상점 — 기억 매매 + 완료 시 Ch3 전환
func _open_malet_shop() -> void:
	# 말렛 상점 재고: 구매 가능한 기억 (정보 기억)
	var shop_items: Array[Dictionary] = [
		{
			"id": "sense_copper_taste",
			"title": "The Taste of Copper",
			"description": "The Sump's air. Old paper and metal. A taste you shouldn't remember but do.",
			"grade": MemoryManager.MemoryGrade.GRADE_5,
			"burn_power": 12,
			"price": 8,
		},
		{
			"id": "daily_malet_deal",
			"title": "A Deal in the Dark",
			"description": "Amber eyes in a dim cellar. A transaction that felt like surgery.",
			"grade": MemoryManager.MemoryGrade.GRADE_4,
			"burn_power": 28,
			"story_effect": "Malet's trust decreases.",
			"related_npc": "Malet",
			"price": 20,
		},
	]
	MemoryShop.open_shop("Malet", shop_items)
	MemoryShop.shop_closed.connect(_on_shop_closed, CONNECT_ONE_SHOT)

func _on_shop_closed() -> void:
	GameManager.set_flag("ch2_complete")
	GameManager.current_chapter = 3
	AchievementManager.record_chapter_complete(2)
	AchievementManager.unlock("merchant")
	print("[VerdenMarket] Chapter 2 complete — transitioning to Crumbling Coast")
	await get_tree().create_timer(1.5).timeout
	SceneTransition.change_scene("res://scenes/maps/crumbling_coast.tscn")

func _setup_random_encounters() -> void:
	if not GameManager.get_flag("ch2_complete"):
		return
	_encounter_data = RandomEncounter.setup(
		[
			{"name": "Alley Rat", "hp": 35, "atk": 8, "is_void": false, "abilities": ["poison"]},
			{"name": "Market Thief", "hp": 50, "atk": 12, "is_void": false, "abilities": ["weaken"]},
		],
		"res://scenes/maps/verdan_market.tscn", "", "", 60, 100
	)

## ===================== 퍼즐 미니게임 트리거 =====================

func _setup_puzzle_trigger() -> void:
	# Ch2 완료 후 재방문 시 노점 근처에서 퍼즐 플레이 가능
	if not GameManager.get_flag("ch2_complete"):
		return
	var area = Area2D.new()
	area.position = Vector2(24 * TILE_SIZE + TILE_SIZE / 2.0, 10 * TILE_SIZE + TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	shape.shape = rect
	area.add_child(shape)
	# 인디케이터
	var indicator = ColorRect.new()
	indicator.size = Vector2(TILE_SIZE * 2, TILE_SIZE * 2)
	indicator.position = -Vector2(TILE_SIZE, TILE_SIZE)
	indicator.color = Color(0.3, 0.5, 0.3, 0.15)
	indicator.z_index = -1
	area.add_child(indicator)
	area.body_entered.connect(func(body):
		if body.name == "Player" and GameManager.current_state == GameManager.GameState.EXPLORATION:
			MemoryPuzzle.open_puzzle(4, 15)
	)
	add_child(area)

## ===================== 인터랙티브 오브젝트 =====================

func _setup_interactive_objects() -> void:
	# 골목 숨겨진 상자 — 좌하단 노점 뒤
	_add_chest(
		Vector2(3 * TILE_SIZE, 15 * TILE_SIZE),
		"chest_verdan_alley",
		{"items": {"firebomb": 1}, "grains": 15}
	)
	# 단서 — 문 근처 (썸프 입구)
	_add_clue(
		Vector2(14 * TILE_SIZE, 14 * TILE_SIZE),
		"clue_verdan_sump",
		"Scratch marks near the door. Someone was dragged through here."
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
	if not SideQuest.is_available("sump_ledger") and not SideQuest.is_active("sump_ledger"):
		return
	if not SideQuest.is_complete("sump_ledger"):
		# NPC: Nervous Trader (골목 근처)
		var area = Area2D.new()
		area.position = Vector2(2 * TILE_SIZE + TILE_SIZE / 2.0, 16 * TILE_SIZE + TILE_SIZE / 2.0)
		area.collision_layer = 0
		area.collision_mask = 2
		var shape = CollisionShape2D.new()
		var rect = RectangleShape2D.new()
		rect.size = Vector2(TILE_SIZE * 1.5, TILE_SIZE * 1.5)
		shape.shape = rect
		area.add_child(shape)
		var sprite = ColorRect.new()
		sprite.size = Vector2(TILE_SIZE * 0.7, TILE_SIZE * 1.0)
		sprite.position = -Vector2(TILE_SIZE * 0.35, TILE_SIZE * 0.5)
		sprite.color = Color(0.4, 0.35, 0.3, 0.6)
		area.add_child(sprite)
		var marker = Label.new()
		marker.text = "!" if not SideQuest.is_active("sump_ledger") else "?"
		marker.add_theme_font_size_override("font_size", 16)
		marker.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
		marker.position = Vector2(-4, -TILE_SIZE)
		area.add_child(marker)

		area.body_entered.connect(func(body):
			if body.name != "Player" or GameManager.current_state != GameManager.GameState.EXPLORATION:
				return
			if SideQuest.is_available("sump_ledger"):
				SideQuest.advance_step("sump_ledger", "sq_sump_ledger_started")
				DialogueManager.load_and_start("res://data/chapter2_dialogue.json", "sq_sump_ledger_start")
			elif SideQuest.is_active("sump_ledger") and GameManager.get_flag("sq_sump_ledger_found"):
				# 반납 선택 (간단히: Grains 보상)
				SideQuest.advance_step("sump_ledger", "sq_sump_ledger_done")
				DialogueManager.load_and_start("res://data/chapter2_dialogue.json", "sq_sump_ledger_return")
			elif SideQuest.is_active("sump_ledger"):
				NotificationToast.show_toast("Find the ledger in the Sump.", NotificationToast.ToastType.INFO)
		)
		add_child(area)

	# 장부 위치 (숨겨진 골목)
	if SideQuest.is_active("sump_ledger") and not GameManager.get_flag("sq_sump_ledger_found"):
		var ledger_area = Area2D.new()
		ledger_area.position = Vector2(12 * TILE_SIZE + TILE_SIZE / 2.0, 12 * TILE_SIZE + TILE_SIZE / 2.0)
		ledger_area.collision_layer = 0
		ledger_area.collision_mask = 2
		var ls = CollisionShape2D.new()
		var lr = RectangleShape2D.new()
		lr.size = Vector2(TILE_SIZE, TILE_SIZE)
		ls.shape = lr
		ledger_area.add_child(ls)
		var ind = ColorRect.new()
		ind.size = Vector2(TILE_SIZE * 0.5, TILE_SIZE * 0.5)
		ind.position = -Vector2(TILE_SIZE * 0.25, TILE_SIZE * 0.25)
		ind.color = Color(0.7, 0.6, 0.3, 0.3)
		ledger_area.add_child(ind)
		ledger_area.body_entered.connect(func(body):
			if body.name == "Player" and not GameManager.get_flag("sq_sump_ledger_found"):
				SideQuest.advance_step("sump_ledger", "sq_sump_ledger_found")
				AudioManager.play_sfx("ui_select")
				DialogueManager.load_and_start("res://data/chapter2_dialogue.json", "sq_sump_ledger_found")
				ind.queue_free()
		)
		add_child(ledger_area)

## ===================== 맵 데코레이션 =====================

func _setup_map_decorations() -> void:
	# 매달린 랜턴 (노점 위)
	var lantern_positions = [
		Vector2(6, 4), Vector2(10, 3), Vector2(14, 5), Vector2(18, 4), Vector2(8, 8),
	]
	for pos in lantern_positions:
		var l = ColorRect.new()
		l.size = Vector2(5, 5)
		l.position = pos * TILE_SIZE + Vector2(12, 4)
		l.color = Color(0.8, 0.6, 0.2, 0.3)
		l.z_index = -1
		add_child(l)
		_lantern_lights.append(l)

	# 연기 (골목에서 올라오는 연기)
	for smoke_pos in [Vector2(3, 14), Vector2(15, 11), Vector2(9, 16)]:
		var s = ColorRect.new()
		s.size = Vector2(8, 4)
		var start_y = smoke_pos.y * TILE_SIZE
		s.position = Vector2(smoke_pos.x * TILE_SIZE + 10, start_y)
		s.color = Color(0.5, 0.5, 0.5, 0.08)
		s.z_index = -1
		add_child(s)
		_smoke_wisps.append({"rect": s, "start_y": start_y})

## ===================== 맵 빌드 =====================

func _build_map() -> void:
	_tile_defs = [
		{"color": Color(0.28, 0.26, 0.25), "detail": "stone"},   # 0: STONE
		{"color": Color(0.18, 0.15, 0.13), "detail": "wall"},    # 1: WALL
		{"color": Color(0.35, 0.25, 0.18), "detail": "stall"},   # 2: STALL
		{"color": Color(0.4, 0.32, 0.22), "detail": "door"},     # 3: DOOR
		{"color": Color(0.15, 0.13, 0.12), "detail": "alley"},   # 4: ALLEY
	]
	var tilemap = TilePainter.create_tilemap(_tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)

	var bodies = TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL])
	for body in bodies:
		add_child(body)

	_minimap_data = Minimap.create_minimap(self, map_data, _tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_player() -> void:
	player.position = Vector2(4 * TILE_SIZE, 9 * TILE_SIZE)
	elia.position = Vector2(4 * TILE_SIZE - 30, 9 * TILE_SIZE + 20)
	# 세이브 로드 시 위치 복원
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-30, 20)
		SaveManager.loaded_player_pos = {}
