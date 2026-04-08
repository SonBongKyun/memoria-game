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

var tile_colors: Dictionary = {
	Tile.STONE: Color(0.28, 0.26, 0.25),
	Tile.WALL: Color(0.18, 0.15, 0.13),
	Tile.STALL: Color(0.35, 0.25, 0.18),
	Tile.DOOR: Color(0.4, 0.32, 0.22),
	Tile.ALLEY: Color(0.15, 0.13, 0.12),
}

@onready var player: CharacterBody2D = $Player
@onready var elia: CharacterBody2D = $Elia

var _minimap_data: Dictionary = {}
var _tile_defs: Array = []
var _encounter_data: RandomEncounter.EncounterData = null

func _ready() -> void:
	_build_map()
	MapEffects.add_vignette(self)
	_position_player()
	_setup_random_encounters()
	_setup_puzzle_trigger()
	_setup_interactive_objects()
	AchievementManager.record_map_visit("verdan_market")
	print("[VerdenMarket] Map loaded — %dx%d tiles" % [MAP_WIDTH, MAP_HEIGHT])

	# Ch2 도착 대화 (첫 진입)
	if not GameManager.get_flag("ch2_arrived"):
		# 챕터 타이틀 카드 표시 후 대화 시작
		await MapEffects.show_chapter_title(self, 2, "Verdan Market", "Where memories are currency")
		await get_tree().create_timer(0.3).timeout
		_start_ch2_sequence()

func _process(_delta: float) -> void:
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia.position, elia.visible)
	if _encounter_data:
		RandomEncounter.update(_encounter_data, player.position, TILE_SIZE)

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
