## OptionalMemorySite, two compact, returnable story pockets that extend the
## Rim and Verdan without interrupting the Part 1 chapter chain.
extends Node2D

const TILE_SIZE: int = 32
const MAP_WIDTH: int = 25
const MAP_HEIGHT: int = 18

enum Tile { GROUND, WALL, PATH, LANDMARK }

const SITE_THEMES := {
	"rim_root_hollow": {"ambient": Color(0.28, 0.48, 0.40), "accent": Color(0.45, 0.88, 0.72), "biome": "dead_forest"},
	"verdan_ledger_cellar": {"ambient": Color(0.52, 0.35, 0.18), "accent": Color(0.92, 0.62, 0.28), "biome": "archive"},
	"belt_signal_yard": {"ambient": Color(0.42, 0.30, 0.26), "accent": Color(0.94, 0.46, 0.25), "biome": "wasteland"},
	"drift_waymarker_shrine": {"ambient": Color(0.30, 0.38, 0.52), "accent": Color(0.58, 0.72, 0.98), "biome": "wasteland"},
	"coast_cinder_harbor": {"ambient": Color(0.40, 0.42, 0.50), "accent": Color(0.94, 0.64, 0.32), "biome": "coast"},
	"seam_lantern_ward": {"ambient": Color(0.46, 0.40, 0.28), "accent": Color(1.00, 0.78, 0.36), "biome": "archive"},
	"forest_name_hollow": {"ambient": Color(0.24, 0.40, 0.24), "accent": Color(0.56, 0.90, 0.46), "biome": "dead_forest"},
	"waste_grey_caravan": {"ambient": Color(0.38, 0.38, 0.42), "accent": Color(0.74, 0.82, 0.92), "biome": "waste"},
	"bl07_seed_vault": {"ambient": Color(0.30, 0.22, 0.44), "accent": Color(0.82, 0.62, 1.00), "biome": "void"},
}

@export var site_id: String = "rim_root_hollow"
@export_file("*.tscn") var return_scene: String = "res://scenes/maps/rim_forest.tscn"
@export_file("*.png") var canvas_path: String = "res://assets/environment/map_canvases/map_rim_root_hollow_canvas_v1.png"
@export var title_en: String = "Root Hollow"
@export var title_ko: String = "기록목 뿌리 공동"
@export var subtitle_en: String = "A place the Monolith never learned to read"
@export var subtitle_ko: String = "모노리스가 읽는 법을 배우지 못한 곳"

var map_data: Array = []
var _minimap_data: Dictionary = {}
var _time := 0.0
var _camera: Camera2D = null

@onready var player: CharacterBody2D = $Player
@onready var elia: StaticBody2D = $Elia

func _ready() -> void:
	map_data = _make_layout(site_id)
	_build_map()
	# S239: 선택 기억 장소들은 코어 맵과 달리 대기 예산을 받지 못하고 있었다.
	# 비네트와 주변광뿐이라 그레이딩도, 안개도, 깊이도, 렌즈도 없이 밋밋하게 렌더됐다.
	# 각 장소는 이미 ambient(바탕색)와 accent(빛의 색)를 선언하고 있으므로,
	# 새 숫자를 만들지 않고 그대로 정체성으로 넘긴다.
	MapEffects.apply_atmosphere(self, _atmosphere_identity())
	MapEffects.add_parallax_background(self, {"sky": _ambient_color().darkened(0.55), "far": _ambient_color().darkened(0.45), "mid": _ambient_color().darkened(0.35), "biome": String(_site_theme().get("biome", "archive")), "width": MAP_WIDTH * TILE_SIZE, "height": MAP_HEIGHT * TILE_SIZE})
	_camera = MapEffects.setup_smooth_camera(player, 1.0)
	MapEffects.add_drop_shadow(player)
	MapEffects.add_lore_glow(self, Vector2(12 * TILE_SIZE, 4 * TILE_SIZE), _accent_color())
	_position_party()
	_add_return_gateway()
	WorldPopulation.populate(self, site_id)
	AchievementManager.record_map_visit(site_id)
	var visit_flag := "visited_%s" % site_id
	if not GameManager.get_flag(visit_flag):
		GameManager.set_flag(visit_flag)
		NotificationToast.show_toast(title_ko if GameManager.current_locale == "ko" else title_en, NotificationToast.ToastType.INFO)
	print("[OptionalMemorySite] %s loaded" % site_id)

func _process(delta: float) -> void:
	_time += delta
	Minimap.update_minimap(_minimap_data, player.position, TILE_SIZE, elia.position, elia.visible)
	MapEffects.update_camera_shake(_camera, _time)
	for npc in get_tree().get_nodes_in_group("npcs"):
		MapEffects.update_npc_idle_motion(npc, _time)

func _make_layout(id: String) -> Array:
	var grid: Array = []
	for y in range(MAP_HEIGHT):
		var row: Array = []
		for x in range(MAP_WIDTH):
			var tile := Tile.GROUND
			if x == 0 or y == 0 or x == MAP_WIDTH - 1 or y == MAP_HEIGHT - 1:
				tile = Tile.WALL
			if id == "rim_root_hollow":
				if (x < 3 and y < 7) or (x > 21 and y < 7) or (x < 3 and y > 12) or (x > 21 and y > 12):
					tile = Tile.WALL
				if x == 12 or (y == 8 and x > 5 and x < 20):
					tile = Tile.PATH
				if (x == 12 and y == 4) or (x == 6 and y == 8) or (x == 18 and y == 8):
					tile = Tile.LANDMARK
			elif id == "verdan_ledger_cellar":
				if (x < 5 and y > 3 and y < 15) or (x > 19 and y > 3 and y < 15):
					tile = Tile.WALL
				if x > 8 and x < 17:
					tile = Tile.PATH
				if (x == 12 and y == 4) or (x == 7 and y == 10) or (x == 17 and y == 10):
					tile = Tile.LANDMARK
			else:
				# The atlas sites share a compact, readable loop: a central approach,
				# two optional danger lanes, and open social corners for the new NPCs.
				if (x < 3 and y < 5) or (x > 21 and y < 5):
					tile = Tile.WALL
				if x == 12 or (y == 9 and x > 4 and x < 21):
					tile = Tile.PATH
				if (x == 12 and y == 4) or (x == 8 and y == 9) or (x == 16 and y == 9) or (x == 12 and y == 12):
					tile = Tile.LANDMARK
			row.append(tile)
		grid.append(row)
	return grid

func _build_map() -> void:
	var tile_defs := [
		{"color": Color(0.14, 0.16, 0.15), "detail": "stone"},
		{"color": Color(0.04, 0.05, 0.05), "detail": "wall"},
		{"color": Color(0.25, 0.27, 0.23), "detail": "path"},
		{"color": _accent_color().darkened(0.35), "detail": "landmark"},
	]
	var tilemap := TilePainter.create_tilemap(tile_defs, map_data, MAP_WIDTH, MAP_HEIGHT)
	add_child(tilemap)
	if canvas_path != "" and ResourceLoader.exists(canvas_path):
		MapEffects.add_map_canvas(self, tilemap, canvas_path, Vector2(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE), {"terrain_alpha": 0.0})
	var bodies := TilePainter.add_collisions(tilemap, map_data, MAP_WIDTH, MAP_HEIGHT, [Tile.WALL])
	for body in bodies:
		add_child(body)
	_minimap_data = Minimap.create_minimap(self, map_data, tile_defs, MAP_WIDTH, MAP_HEIGHT)

func _position_party() -> void:
	player.position = Vector2(12.5 * TILE_SIZE, 14.5 * TILE_SIZE)
	elia.position = player.position + Vector2(-25, 15)
	if not SaveManager.loaded_player_pos.is_empty():
		player.position = Vector2(SaveManager.loaded_player_pos.x, SaveManager.loaded_player_pos.y)
		elia.position = player.position + Vector2(-25, 15)
		SaveManager.loaded_player_pos = {}

func _add_return_gateway() -> void:
	var gateway := MapGateway.new()
	gateway.name = "ReturnGateway"
	gateway.position = Vector2(12.5 * TILE_SIZE, 16.0 * TILE_SIZE)
	gateway.destination_scene = return_scene
	gateway.label_en = "Return to the road"
	gateway.label_ko = "길로 돌아가기"
	gateway.accent = _accent_color()
	add_child(gateway)

func _ambient_color() -> Color:
	return _site_theme().get("ambient", Color(0.40, 0.40, 0.40))

func _accent_color() -> Color:
	return _site_theme().get("accent", Color(0.82, 0.62, 0.34))

## 장소가 이미 들고 있는 선언(ambient / accent / biome)에서 대기 정체성을 만든다.
## mood만 바이옴별로 정한다. 씨앗 금고(BL-07)가 가장 짓눌린 곳이다.
const BIOME_MOOD := {
	"dead_forest": 0.52,
	"archive": 0.34,
	"wasteland": 0.46,
	"shrine": 0.40,
	"harbor": 0.42,
	"ward": 0.36,
	"hollow": 0.55,
	"grey": 0.50,
	"vault": 0.88,
}

func _atmosphere_identity() -> Dictionary:
	var theme := _site_theme()
	var biome := String(theme.get("biome", "archive"))
	# 무색 지대의 회색 카라반은 채도 자체가 정체성이다.
	var saturation := 0.15 if site_id == "waste_grey_caravan" else 1.0
	return {
		"hue": _ambient_color(),
		"light": _accent_color(),
		"mood": float(BIOME_MOOD.get(biome, 0.42)),
		"saturation": saturation,
		"brightness": 0.0,
		"fog": "none",
		"splash": "",
	}

func _site_theme() -> Dictionary:
	return SITE_THEMES.get(site_id, SITE_THEMES.verdan_ledger_cellar)
