## Minimap — 우상단 미니맵 오버레이
## 현재 맵 레이아웃 + 플레이어 위치 표시.
## CanvasLayer 기반, EXPLORATION에서만 표시.
class_name Minimap

const MINIMAP_SIZE := Vector2(112, 80)
const MINIMAP_MARGIN := Vector2(12, 12)
const PIXEL_SIZE := 3  # clean, compact map footprint
const PLAYER_SIZE := 5
const OBJECTIVE_SIZE := 6

# 타일 색상 (공통 매핑)
const TILE_COLORS := {
	"grass": Color(0.25, 0.4, 0.2, 0.9),
	"path": Color(0.5, 0.4, 0.3, 0.9),
	"tree": Color(0.1, 0.15, 0.1, 0.9),
	"bush": Color(0.3, 0.4, 0.25, 0.9),
	"water": Color(0.15, 0.25, 0.5, 0.9),
	"stone": Color(0.35, 0.33, 0.3, 0.9),
	"wall": Color(0.2, 0.18, 0.15, 0.9),
	"stall": Color(0.45, 0.35, 0.25, 0.9),
	"door": Color(0.5, 0.4, 0.3, 0.9),
	"alley": Color(0.18, 0.16, 0.15, 0.9),
	"rock": Color(0.4, 0.38, 0.35, 0.9),
	"sand": Color(0.55, 0.48, 0.38, 0.9),
	"cliff": Color(0.18, 0.16, 0.14, 0.9),
	"hut": Color(0.5, 0.38, 0.28, 0.9),
	"garden": Color(0.2, 0.5, 0.3, 0.9),
	"lantern": Color(0.8, 0.65, 0.3, 0.9),
	"void": Color(0.2, 0.05, 0.3, 0.9),
	"fragment": Color(0.3, 0.1, 0.4, 0.9),
	"crack": Color(0.35, 0.1, 0.45, 0.9),
	"core": Color(0.5, 0.15, 0.6, 0.9),
}

## 미니맵을 부모 맵에 추가.
## map_data: 2D 타일 배열, tile_defs: TilePainter용 타일 정의 배열
## Returns: Dictionary with {layer, player_marker, map_image, tile_size, map_width, map_height}
static func create_minimap(parent: Node, map_data: Array, tile_defs: Array, map_width: int, map_height: int) -> Dictionary:
	var layer = CanvasLayer.new()
	layer.layer = 9  # ExplorationHUD(10) 바로 아래

	# 컨테이너 — 우상단
	var container = Control.new()
	container.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	container.offset_left = -(MINIMAP_SIZE.x + MINIMAP_MARGIN.x)
	container.offset_right = -MINIMAP_MARGIN.x
	container.offset_top = MINIMAP_MARGIN.y
	container.offset_bottom = MINIMAP_SIZE.y + MINIMAP_MARGIN.y
	container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(container)

	# 배경 패널
	var bg = ColorRect.new()
	bg.size = MINIMAP_SIZE
	bg.color = Color(0.04, 0.03, 0.06, 0.58)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(bg)

	# 테두리
	var border = ReferenceRect.new()
	border.size = MINIMAP_SIZE
	border.border_color = Color(0.3, 0.25, 0.2, 0.6)
	border.border_width = 1.0
	border.editor_only = false
	border.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(border)

	# 맵 이미지 (타일 기반)
	var map_image = _create_map_texture(map_data, tile_defs, map_width, map_height)
	map_image.position = Vector2(
		(MINIMAP_SIZE.x - map_width * PIXEL_SIZE) / 2.0,
		(MINIMAP_SIZE.y - map_height * PIXEL_SIZE) / 2.0
	)
	map_image.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(map_image)

	# 플레이어 마커 (밝은 파란 점)
	var marker = ColorRect.new()
	marker.size = Vector2(PLAYER_SIZE, PLAYER_SIZE)
	marker.color = Color(0.4, 0.7, 1.0, 1.0)
	marker.z_index = 1
	marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(marker)

	# 엘리아 마커 (은빛)
	var elia_marker = ColorRect.new()
	elia_marker.size = Vector2(4, 4)
	elia_marker.color = Color(0.7, 0.75, 0.85, 0.8)
	elia_marker.z_index = 1
	elia_marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
	elia_marker.visible = false
	container.add_child(elia_marker)

	# Story objective marker. A rotated gold square stays readable against every
	# biome palette and does not rely on color alone thanks to its larger shape.
	var objective_marker := ColorRect.new()
	objective_marker.size = Vector2(OBJECTIVE_SIZE, OBJECTIVE_SIZE)
	objective_marker.color = Color(1.0, 0.72, 0.24, 1.0)
	objective_marker.rotation = PI / 4.0
	objective_marker.pivot_offset = objective_marker.size / 2.0
	objective_marker.z_index = 2
	objective_marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
	objective_marker.visible = false
	container.add_child(objective_marker)

	parent.add_child(layer)

	# 가시성 연동 — is_instance_valid 체크 (씬 전환 시 freed 방지)
	GameManager.state_changed.connect(func(state):
		if is_instance_valid(container):
			container.visible = (state == GameManager.GameState.EXPLORATION)
	)
	container.visible = (GameManager.current_state == GameManager.GameState.EXPLORATION)

	return {
		"layer": layer,
		"container": container,
		"player_marker": marker,
		"elia_marker": elia_marker,
		"objective_marker": objective_marker,
		"map_ref": weakref(parent),
		"map_offset": map_image.position,
		"map_width": map_width,
		"map_height": map_height,
	}

## 미니맵 플레이어 마커 업데이트 (맵 _process에서 호출)
static func update_minimap(data: Dictionary, player_pos: Vector2, tile_size: int, elia_pos: Vector2 = Vector2.ZERO, elia_visible: bool = false) -> void:
	if data.is_empty():
		return
	var marker: ColorRect = data.player_marker
	var offset: Vector2 = data.map_offset
	var mw: int = data.map_width
	var mh: int = data.map_height

	# 플레이어 위치를 미니맵 좌표로 변환
	var norm_x: float = player_pos.x / (mw * tile_size)
	var norm_y: float = player_pos.y / (mh * tile_size)
	marker.position = Vector2(
		offset.x + norm_x * mw * PIXEL_SIZE - PLAYER_SIZE / 2.0,
		offset.y + norm_y * mh * PIXEL_SIZE - PLAYER_SIZE / 2.0
	)

	# 엘리아 마커
	var em: ColorRect = data.elia_marker
	if elia_visible:
		em.visible = true
		var en_x: float = elia_pos.x / (mw * tile_size)
		var en_y: float = elia_pos.y / (mh * tile_size)
		em.position = Vector2(
			offset.x + en_x * mw * PIXEL_SIZE - 2.0,
			offset.y + en_y * mh * PIXEL_SIZE - 2.0
		)
	else:
		em.visible = false

	_update_objective_marker(data, offset, mw, mh, tile_size)

static func _update_objective_marker(data: Dictionary, offset: Vector2, map_width: int, map_height: int, tile_size: int) -> void:
	var marker := data.get("objective_marker") as ColorRect
	var map_ref: WeakRef = data.get("map_ref")
	if marker == null or map_ref == null:
		return
	var map_node := map_ref.get_ref() as Node2D
	if map_node == null:
		marker.visible = false
		return
	var target: Variant = _resolve_story_target(map_node)
	if target == null:
		marker.visible = false
		return
	marker.visible = true
	var world_pos: Vector2 = target
	var norm_x := clampf(world_pos.x / (map_width * tile_size), 0.0, 1.0)
	var norm_y := clampf(world_pos.y / (map_height * tile_size), 0.0, 1.0)
	marker.position = Vector2(
		offset.x + norm_x * map_width * PIXEL_SIZE - OBJECTIVE_SIZE / 2.0,
		offset.y + norm_y * map_height * PIXEL_SIZE - OBJECTIVE_SIZE / 2.0
	)
	var pulse: float = 0.88 + sin(Time.get_ticks_msec() * 0.006) * 0.12
	marker.scale = Vector2(pulse, pulse)

static func _resolve_story_target(map_node: Node2D) -> Variant:
	var map_key := map_node.scene_file_path.get_file().get_basename()
	match map_key:
		"rim_forest":
			if not GameManager.get_flag("ch1_elia_appeared"):
				return _node_position(map_node, "Elia")
			if not GameManager.get_flag("ch1_void_beast_defeated"):
				return Vector2(17 * 32, 8 * 32)
			if not GameManager.get_flag("ch1_camp_done"):
				return Vector2(12.5 * 32, 16 * 32)
		"verdan_market":
			if not GameManager.get_flag("ch2_malet_done"):
				return _node_position(map_node, "Malet")
		"belt_waystation":
			if not GameManager.get_flag("tobias_in_party"):
				return _node_position(map_node, "Tobias")
			return Vector2(12 * 32, 1.5 * 32)
		"drift_shelter":
			return Vector2(10 * 32, 1.5 * 32)
		"crumbling_coast":
			return Vector2(5 * 32, 1.5 * 32)
		"the_seam":
			if not GameManager.get_flag("ch6_briefing_done"):
				return _node_position(map_node, "Sable")
			return Vector2(12 * 32, 16.5 * 32)
		"seam_outskirts":
			if not GameManager.get_flag("ch7_trial_complete"):
				return _node_position(map_node, "Sable")
			return Vector2(10 * 32, 1.5 * 32)
		"forgotten_forest":
			return Vector2(10 * 32, 1.5 * 32)
		"colorless_waste":
			return Vector2(10 * 32, 1.5 * 32)
		"bl07_void":
			return Vector2(9.5 * 32, 17 * 32)
	return null

static func _node_position(parent: Node, node_name: String) -> Variant:
	var node := parent.get_node_or_null(node_name) as Node2D
	return node.position if node != null and node.visible else null

## 맵 텍스처 생성 (ColorRect 그리드)
static func _create_map_texture(map_data: Array, tile_defs: Array, width: int, height: int) -> Control:
	var container = Control.new()
	container.size = Vector2(width * PIXEL_SIZE, height * PIXEL_SIZE)

	for y in range(height):
		if y >= map_data.size():
			break
		for x in range(width):
			if x >= map_data[y].size():
				break
			var tile_idx: int = map_data[y][x]
			var color := Color(0.15, 0.15, 0.15, 0.9)

			# tile_defs에서 색상 가져오기
			if tile_idx < tile_defs.size():
				var detail: String = tile_defs[tile_idx].get("detail", "")
				if TILE_COLORS.has(detail):
					color = TILE_COLORS[detail]
				else:
					color = tile_defs[tile_idx].get("color", color)
					color.a = 0.9

			var px = ColorRect.new()
			px.size = Vector2(PIXEL_SIZE, PIXEL_SIZE)
			px.position = Vector2(x * PIXEL_SIZE, y * PIXEL_SIZE)
			px.color = color
			px.mouse_filter = Control.MOUSE_FILTER_IGNORE
			container.add_child(px)

	return container
