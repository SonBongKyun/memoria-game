## Minimap, 우상단 미니맵 오버레이
## 현재 맵 레이아웃 + 플레이어 위치 표시.
## CanvasLayer 기반, EXPLORATION에서만 표시.
class_name Minimap

const MINIMAP_SIZE := Vector2(108, 96)
const MINIMAP_MARGIN := Vector2(12, 12)
const PIXEL_SIZE := 3  # clean, compact map footprint
const PLAYER_SIZE := 5
const OBJECTIVE_SIZE := 6

## S216: 관심 지점(POI) 마커.
## 미니맵에는 여태 플레이어/동행자/이야기 목표 세 개뿐이라, 맵에 실제로 놓여 있는
## 은닉 상자·유물·주민을 전혀 알려 주지 않았다. 그렇다고 전부 표시하면 탐색이
## 사라지므로, 플레이어 주변만 드러난다. 기억 파동으로 주변을 감지한다는 설정과도 맞다.
const POI_REVEAL_TILES := 6.0
const THREAT_REVEAL_TILES := 10.0
const POI_SIZE := 4
const THREAT_SIZE := 5
const MINIMAP_FRAME_PATH := "res://assets/cg/generated/ui_minimap_compass_frame_v1.png"

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

	# 컨테이너, 우상단
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
	# Compact Memory Compass plate. Its center stays quiet so route pixels and
	# objective markers remain the strongest information layer.
	if ResourceLoader.exists(MINIMAP_FRAME_PATH):
		var frame := TextureRect.new()
		frame.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		frame.stretch_mode = TextureRect.STRETCH_SCALE
		frame.texture = load(MINIMAP_FRAME_PATH)
		frame.position = Vector2.ZERO
		frame.size = MINIMAP_SIZE
		frame.mouse_filter = Control.MOUSE_FILTER_IGNORE
		frame.modulate = Color(0.82, 0.78, 0.74, 0.88)
		container.add_child(frame)

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

	# S216: POI 마커 풀 + 범례.
	# 맵에 놓인 은닉 상자, 유물, 주민을 플레이어 주변에서만 드러낸다.
	# 마커 풀은 첫 갱신 때 실제 배치 결과를 보고 채운다 (_update_poi_markers 참고).
	var poi_points: Array = []
	var poi_markers: Array = []

	# 범례는 단어마다 마커와 같은 색을 입힌다.
	# 흰 글씨로 "발견물 · 유물 · 주민"만 적어 두면 어떤 색이 무엇인지 알 수 없다.
	# S230: 범례는 지도 아래 지형 위에 맨 글자로 얹혀 있었다. 숲 캔버스 위에서는
	# 3px 외곽선만으로는 읽히지 않아, 색 구분표 자체가 무의미했다. 읽을 바탕을 깐다.
	var legend_plate := PanelContainer.new()
	legend_plate.name = "MinimapLegend"
	legend_plate.position = Vector2(0, MINIMAP_SIZE.y - 1)
	legend_plate.custom_minimum_size = Vector2(MINIMAP_SIZE.x + 4, 0)
	legend_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var legend_style := StyleBoxFlat.new()
	legend_style.bg_color = Color(0.020, 0.018, 0.028, 0.96)
	legend_style.border_color = Color(0.42, 0.36, 0.26, 0.68)
	legend_style.set_border_width_all(1)
	legend_style.set_corner_radius_all(3)
	legend_style.content_margin_left = 5
	legend_style.content_margin_right = 5
	legend_style.content_margin_top = 3
	legend_style.content_margin_bottom = 3
	legend_plate.add_theme_stylebox_override("panel", legend_style)

	var legend := RichTextLabel.new()
	legend.bbcode_enabled = true
	legend.fit_content = true
	legend.scroll_active = false
	legend.custom_minimum_size = Vector2(MINIMAP_SIZE.x - 6, 0)
	legend.add_theme_font_size_override("normal_font_size", UITheme.MIN_META_FONT_SIZE)
	legend.add_theme_font_override("normal_font", UITheme.make_meta_font())
	legend.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.85))
	legend.add_theme_constant_override("outline_size", 1)
	legend.add_theme_constant_override("line_separation", 2)
	var is_ko := GameManager.current_locale == "ko"
	legend.text = "[color=#%s]%s[/color] [color=#%s]%s[/color] [color=#%s]%s[/color]" % [
		_poi_color("cache").to_html(false), "발견물" if is_ko else "Cache",
		_poi_color("curio").to_html(false), "유물" if is_ko else "Relic",
		_poi_color("voice").to_html(false), "주민" if is_ko else "Local",
	]
	legend.text += "\n[color=#%s]▲[/color] %s" % [_poi_color("threat").to_html(false), ("위협" if is_ko else "Threat")]
	legend.mouse_filter = Control.MOUSE_FILTER_IGNORE
	legend_plate.add_child(legend)
	legend_plate.visible = false
	container.add_child(legend_plate)

	parent.add_child(layer)

	# 가시성 연동, is_instance_valid 체크 (씬 전환 시 freed 방지)
	var state_changed_callback := func(state):
		if is_instance_valid(container):
			container.visible = (state == GameManager.GameState.EXPLORATION)
	GameManager.state_changed.connect(state_changed_callback)
	layer.tree_exiting.connect(func():
		if GameManager.state_changed.is_connected(state_changed_callback):
			GameManager.state_changed.disconnect(state_changed_callback)
	, CONNECT_ONE_SHOT)
	container.visible = (GameManager.current_state == GameManager.GameState.EXPLORATION)

	return {
		"layer": layer,
		"container": container,
		"player_marker": marker,
		"elia_marker": elia_marker,
		"objective_marker": objective_marker,
		"map_ref": weakref(parent),
		"poi_points": poi_points,
		"poi_markers": poi_markers,
		"legend": legend_plate,
		"map_offset": map_image.position,
		"map_width": map_width,
		"map_height": map_height,
	}

## 맵에 실제로 배치된 관심 지점을 모은다. 종류별로 모양과 색이 다르다.
static func _collect_points_of_interest(map_node: Node2D) -> Array:
	var points: Array = []
	if map_node == null or not is_instance_valid(map_node):
		return points
	# 월드 인구(주민/발견물/유물)는 맵 바로 아래가 아니라 "WorldPopulation" 컨테이너
	# 안에 들어간다. 맵 직속 자식만 훑으면 대부분을 놓친다.
	var sources: Array[Node] = [map_node]
	var population := map_node.get_node_or_null("WorldPopulation")
	if population != null:
		sources.append(population)
	for source: Node in sources:
		points.append_array(_scan_children_for_poi(source))
	return points

static func _scan_children_for_poi(parent: Node) -> Array:
	var points: Array = []
	for child in parent.get_children():
		if not (child is Node2D):
			continue
		var node2d := child as Node2D
		var name := String(child.name)
		if child is WorldCurio:
			points.append({"pos": node2d.position, "kind": "curio"})
		elif name.begins_with("WorldCache_"):
			points.append({"pos": node2d.position, "kind": "cache"})
		elif name.begins_with("WorldVoice_"):
			points.append({"pos": node2d.position, "kind": "voice"})
		elif name.begins_with("WorldThreat_"):
			points.append({"pos": node2d.position, "kind": "threat"})
		elif child is Area2D and child.get_node_or_null("DiscoveryMarker") != null:
			# S209에서 만든 은닉 상자 / 기억 단서 마커.
			points.append({"pos": node2d.position, "kind": "cache"})
	return points

static func _poi_color(kind: String) -> Color:
	match kind:
		"curio":
			return Color(0.72, 0.56, 0.94, 0.95)
		"voice":
			return Color(0.58, 0.80, 0.92, 0.90)
		"threat":
			return Color(0.98, 0.38, 0.30, 0.96)
		_:
			# 목표 마커(주황 금색 마름모)와 헷갈리지 않도록 옅은 호박색으로 띄운다.
			return Color(0.96, 0.90, 0.55, 0.95)

## POI 개수에 맞춰 마커 풀을 채운다.
static func _ensure_poi_markers(data: Dictionary, container: Control) -> void:
	var points: Array = data.get("poi_points", [])
	var markers: Array = data.get("poi_markers", [])
	while markers.size() < points.size():
		var poi := ColorRect.new()
		poi.size = Vector2(POI_SIZE, POI_SIZE)
		poi.z_index = 1
		poi.visible = false
		poi.mouse_filter = Control.MOUSE_FILTER_IGNORE
		container.add_child(poi)
		markers.append(poi)
	# 종류가 바뀌었을 수 있으므로 색과 모양을 매번 맞춘다.
	for i in range(points.size()):
		var marker := markers[i] as ColorRect
		if marker == null or not is_instance_valid(marker):
			continue
		var kind := String((points[i] as Dictionary).get("kind", "cache"))
		marker.size = Vector2(THREAT_SIZE, THREAT_SIZE) if kind == "threat" else Vector2(POI_SIZE, POI_SIZE)
		marker.color = _poi_color(kind)
		# 색만으로 구분하지 않는다. 유물은 마름모로 돌려 형태로도 읽히게 한다.
		marker.rotation = PI / 4.0 if kind in ["curio", "threat"] else 0.0
		marker.pivot_offset = marker.size / 2.0
	data["poi_markers"] = markers

## POI 마커를 갱신한다. 플레이어 반경 밖은 감춘다.
static func _update_poi_markers(data: Dictionary, player_pos: Vector2, tile_size: int) -> void:
	var container := data.get("container") as Control
	var map_ref: WeakRef = data.get("map_ref")
	if container == null or map_ref == null:
		return
	var map_node := map_ref.get_ref() as Node2D
	if map_node == null:
		return

	# S216: 지연 스캔.
	# 월드 인구(주민/발견물/유물)는 맵 _ready 이후에 배치되므로, 미니맵 생성 시점에
	# 한 번만 훑으면 대부분을 놓친다. 자식 수가 달라졌으면 다시 훑고 마커를 보충한다.
	var child_count := map_node.get_child_count()
	if int(data.get("poi_scan_children", -1)) != child_count:
		data["poi_scan_children"] = child_count
		data["poi_points"] = _collect_points_of_interest(map_node)
		_ensure_poi_markers(data, container)

	var points: Array = data.get("poi_points", [])
	var markers: Array = data.get("poi_markers", [])
	var offset: Vector2 = data.map_offset
	var mw: int = data.map_width
	var mh: int = data.map_height
	var reveal := POI_REVEAL_TILES * float(tile_size)

	var shown := 0
	for i in range(markers.size()):
		var marker := markers[i] as ColorRect
		if marker == null or not is_instance_valid(marker):
			continue
		if i >= points.size():
			marker.visible = false
			continue
		var point: Dictionary = points[i]
		var world: Vector2 = point.get("pos", Vector2.ZERO)
		var kind := String(point.get("kind", "cache"))
		var point_reveal := THREAT_REVEAL_TILES * float(tile_size) if kind == "threat" else reveal
		if world.distance_to(player_pos) > point_reveal:
			marker.visible = false
			continue
		marker.visible = true
		shown += 1
		var nx := clampf(world.x / float(mw * tile_size), 0.0, 1.0)
		var ny := clampf(world.y / float(mh * tile_size), 0.0, 1.0)
		marker.position = Vector2(
			offset.x + nx * mw * PIXEL_SIZE - marker.size.x / 2.0,
			offset.y + ny * mh * PIXEL_SIZE - marker.size.y / 2.0
		)
	var legend := data.get("legend") as Control
	if legend != null and is_instance_valid(legend):
		legend.visible = shown > 0

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
	_update_poi_markers(data, player_pos, tile_size)

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
			if not GameManager.get_flag("ch3_class_seven_message"):
				return Vector2(12 * 32, 9 * 32)
			return Vector2(23.5 * 32, 9 * 32)
		"drift_shelter":
			if GameManager.get_flag("canon_ch5_classifier_ready") \
					or GameManager.get_flag("canon_ch6_seam_ready"):
				return null
			return Vector2(23.5 * 32, 9 * 32)
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
