## HybridDepthStage renders a small real-time 3D diorama into the existing 2D UI.
## The 2D game keeps ownership of collision, character identity, text and input;
## 3D is reserved for depth, landmarks and reaction to important actions.
class_name HybridDepthStage
extends Control

enum StageMode { BATTLE, ATLAS, RELIC }

const PROFILE_DATA: Dictionary = {
	"rim_forest": {"accent": Color(0.57, 0.72, 0.38), "stone": Color(0.11, 0.15, 0.12), "motif": "roots"},
	"verdan_market": {"accent": Color(0.86, 0.58, 0.25), "stone": Color(0.19, 0.13, 0.10), "motif": "stalls"},
	"belt_waystation": {"accent": Color(0.70, 0.58, 0.38), "stone": Color(0.16, 0.14, 0.13), "motif": "signals"},
	"drift_shelter": {"accent": Color(0.48, 0.65, 0.76), "stone": Color(0.12, 0.15, 0.18), "motif": "markers"},
	"crumbling_coast": {"accent": Color(0.42, 0.72, 0.75), "stone": Color(0.11, 0.15, 0.17), "motif": "masts"},
	"the_seam": {"accent": Color(0.88, 0.63, 0.28), "stone": Color(0.14, 0.11, 0.17), "motif": "lanterns"},
	"seam_outskirts": {"accent": Color(0.65, 0.48, 0.82), "stone": Color(0.12, 0.09, 0.16), "motif": "threshold"},
	"forgotten_forest": {"accent": Color(0.48, 0.74, 0.54), "stone": Color(0.08, 0.14, 0.11), "motif": "roots"},
	"colorless_waste": {"accent": Color(0.70, 0.67, 0.58), "stone": Color(0.16, 0.15, 0.17), "motif": "markers"},
	"bl07_void": {"accent": Color(0.58, 0.38, 0.88), "stone": Color(0.08, 0.06, 0.13), "motif": "void"},
	"world_map": {"accent": Color(0.88, 0.65, 0.30), "stone": Color(0.10, 0.09, 0.13), "motif": "routes"},
}

var profile_id: String = "rim_forest"
var stage_mode: StageMode = StageMode.BATTLE
var viewport: SubViewport
var texture_rect: TextureRect
var scene_root: Node3D
var motion_root: Node3D
var camera: Camera3D
var orbit_root: Node3D
var focus_root: Node3D

var _accent: Color = Color(0.72, 0.58, 0.32)
var _stone: Color = Color(0.12, 0.11, 0.15)
var _motif: String = "markers"
var _base_camera_position: Vector3 = Vector3(0, 4, 10)
var _camera_look_target: Vector3 = Vector3(0, 0.5, 0)
var _impact_offset: float = 0.0
var _impact_lift: float = 0.0
var _focus_pan: float = 0.0
var _time: float = 0.0
var _atlas_markers: Array[MeshInstance3D] = []
var _atlas_materials: Array[StandardMaterial3D] = []
var battle_player_focus_root: Node3D
var battle_enemy_focus_root: Node3D
var _battle_player_focus_material: StandardMaterial3D
var _battle_enemy_focus_material: StandardMaterial3D

static func create_stage(profile: String, mode: StageMode) -> HybridDepthStage:
	var stage := HybridDepthStage.new()
	stage.profile_id = profile
	stage.stage_mode = mode
	stage.mouse_filter = Control.MOUSE_FILTER_IGNORE
	stage.process_mode = Node.PROCESS_MODE_ALWAYS
	stage.set_anchors_preset(Control.PRESET_FULL_RECT)
	stage._build_stage()
	return stage

static func profile_from_scene(scene_path: String) -> String:
	var lowered := scene_path.to_lower()
	for candidate: String in [
		"rim_forest", "verdan_market", "belt_waystation", "drift_shelter",
		"crumbling_coast", "the_seam", "seam_outskirts", "forgotten_forest",
		"colorless_waste", "bl07_void",
	]:
		if candidate in lowered:
			return candidate
	return "rim_forest"

func _build_stage() -> void:
	var data: Dictionary = PROFILE_DATA.get(profile_id, PROFILE_DATA["rim_forest"])
	_accent = data.get("accent", Color(0.72, 0.58, 0.32))
	_stone = data.get("stone", Color(0.12, 0.11, 0.15))
	_motif = String(data.get("motif", "markers"))

	viewport = SubViewport.new()
	viewport.name = "Hybrid3DViewport"
	viewport.size = _viewport_size_for_mode()
	viewport.transparent_bg = true
	viewport.own_world_3d = true
	viewport.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	viewport.handle_input_locally = false
	add_child(viewport)

	scene_root = Node3D.new()
	scene_root.name = "DioramaWorld"
	viewport.add_child(scene_root)
	motion_root = Node3D.new()
	motion_root.name = "DepthMotion"
	scene_root.add_child(motion_root)

	_add_environment()
	_add_camera()
	match stage_mode:
		StageMode.ATLAS:
			_build_atlas()
		StageMode.RELIC:
			_build_relic()
		_:
			_build_battle_diorama()

	texture_rect = TextureRect.new()
	texture_rect.name = "Hybrid3DTexture"
	texture_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	texture_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	texture_rect.texture = viewport.get_texture()
	texture_rect.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	texture_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(texture_rect)

func _viewport_size_for_mode() -> Vector2i:
	match stage_mode:
		StageMode.ATLAS:
			return Vector2i(640, 420)
		StageMode.RELIC:
			return Vector2i(560, 320)
		_:
			return Vector2i(640, 360)

func _add_environment() -> void:
	var world_environment := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0, 0, 0, 0)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.42, 0.45, 0.55)
	environment.ambient_light_energy = 0.78
	world_environment.environment = environment
	scene_root.add_child(world_environment)

	var key_light := DirectionalLight3D.new()
	key_light.light_color = Color(1.0, 0.82, 0.58)
	key_light.light_energy = 1.05
	key_light.rotation_degrees = Vector3(-52, -28, 0)
	key_light.shadow_enabled = false
	scene_root.add_child(key_light)

	var memory_light := OmniLight3D.new()
	memory_light.light_color = _accent
	memory_light.light_energy = 2.2
	memory_light.omni_range = 9.0
	memory_light.position = Vector3(0, 2.2, 1.8)
	memory_light.shadow_enabled = false
	scene_root.add_child(memory_light)

func _add_camera() -> void:
	camera = Camera3D.new()
	camera.name = "DioramaCamera"
	camera.current = true
	match stage_mode:
		StageMode.ATLAS:
			_base_camera_position = Vector3(0, 8.2, 10.8)
			_camera_look_target = Vector3(0, 0.0, -0.2)
			camera.fov = 37.0
		StageMode.RELIC:
			_base_camera_position = Vector3(0, 2.8, 7.8)
			_camera_look_target = Vector3(0, 0.45, 0)
			camera.fov = 34.0
		_:
			_base_camera_position = Vector3(0, 4.1, 10.6)
			_camera_look_target = Vector3(0, 0.35, 0)
			camera.fov = 45.0
	scene_root.add_child(camera)
	camera.look_at_from_position(_base_camera_position, _camera_look_target, Vector3.UP)

func _build_battle_diorama() -> void:
	var dark_material := _make_material(_stone, Color.BLACK, 0.78)
	var mid_material := _make_material(_stone.lightened(0.12), _accent.darkened(0.72), 0.72, 0.32)
	var accent_material := _make_material(_accent.darkened(0.20), _accent, 0.94, 0.92)
	var ghost_material := _make_material(_accent.darkened(0.48), _accent.darkened(0.12), 0.46, 0.58)
	var floor_trace := _make_material(_stone.lightened(0.08), _accent.darkened(0.18), 0.20, 0.46)
	var memory_trace := _make_material(_accent.darkened(0.34), _accent, 0.42, 0.74)

	# A transparent traced arena preserves the illustration and character silhouettes.
	# The previous opaque disc covered most of the 2D battle frame.
	_add_floor_trace(motion_root, 5.65, 2.65, -1.12, 26, floor_trace, 3)
	_add_floor_trace(motion_root, 3.75, 1.72, -1.105, 22, memory_trace, 5)
	_build_battle_focus_rings()
	for i in range(9):
		var x := -5.0 + float(i) * 1.25
		var depth := -2.25 + float((i * 7) % 5) * 0.72
		var height := 1.25 + float((i * 11) % 7) * 0.27
		var width := 0.16 + float(i % 3) * 0.05
		_add_box(motion_root, Vector3(x, -1.05 + height * 0.5, depth), Vector3(width, height, width), mid_material, Vector3(0.0, 0.0, sin(float(i)) * 0.08))

	match _motif:
		"roots":
			_add_root_motif(dark_material, accent_material)
		"stalls":
			_add_market_motif(dark_material, accent_material)
		"signals":
			_add_signal_motif(mid_material, accent_material)
		"masts":
			_add_mast_motif(mid_material, accent_material)
		"lanterns":
			_add_lantern_motif(dark_material, accent_material)
		"threshold":
			_add_threshold_motif(dark_material, accent_material)
		"void":
			_add_void_motif(ghost_material, accent_material)
		_:
			_add_marker_motif(mid_material, accent_material)

	for i in range(9):
		var shard_angle := TAU * float(i) / 9.0
		var shard_pos := Vector3(cos(shard_angle) * 4.4, 0.15 + float(i % 3) * 0.55, sin(shard_angle) * 2.4 - 0.7)
		_add_box(motion_root, shard_pos, Vector3(0.08, 0.38 + float(i % 2) * 0.18, 0.08), ghost_material, Vector3(shard_angle * 0.15, shard_angle, shard_angle * 0.08))
	set_battle_focus("neutral")

func _build_battle_focus_rings() -> void:
	battle_player_focus_root = Node3D.new()
	battle_player_focus_root.name = "PlayerFocusRing"
	battle_player_focus_root.position = Vector3(-2.85, 0.0, 0.55)
	motion_root.add_child(battle_player_focus_root)
	_battle_player_focus_material = _make_material(Color(0.10, 0.22, 0.34), Color(0.34, 0.74, 1.0), 0.30, 0.42)
	_add_floor_trace(battle_player_focus_root, 1.34, 0.72, -1.085, 24, _battle_player_focus_material, 4)
	_add_floor_trace(battle_player_focus_root, 0.92, 0.48, -1.08, 20, _battle_player_focus_material, 5)

	battle_enemy_focus_root = Node3D.new()
	battle_enemy_focus_root.name = "EnemyFocusRing"
	battle_enemy_focus_root.position = Vector3(2.85, 0.0, 0.55)
	motion_root.add_child(battle_enemy_focus_root)
	_battle_enemy_focus_material = _make_material(Color(0.30, 0.09, 0.13), Color(1.0, 0.30, 0.24), 0.28, 0.38)
	_add_floor_trace(battle_enemy_focus_root, 1.38, 0.74, -1.085, 24, _battle_enemy_focus_material, 4)
	_add_floor_trace(battle_enemy_focus_root, 0.94, 0.50, -1.08, 20, _battle_enemy_focus_material, 5)

func set_battle_focus(side: String) -> void:
	if stage_mode != StageMode.BATTLE or battle_player_focus_root == null or battle_enemy_focus_root == null:
		return
	var player_active := side == "player"
	var enemy_active := side == "enemy" or side == "enemy_break"
	var enemy_broken := side == "enemy_break"
	_set_battle_focus_material(_battle_player_focus_material, Color(0.34, 0.74, 1.0), player_active, false)
	_set_battle_focus_material(_battle_enemy_focus_material, Color(1.0, 0.30, 0.24), enemy_active, enemy_broken)
	var player_target := Vector3(1.13, 1.0, 1.13) if player_active else Vector3(0.91, 1.0, 0.91)
	var enemy_target := Vector3(1.18, 1.0, 1.18) if enemy_active else Vector3(0.91, 1.0, 0.91)
	if OptionsMenu != null and OptionsMenu.is_reduce_motion():
		battle_player_focus_root.scale = player_target
		battle_enemy_focus_root.scale = enemy_target
	else:
		var tween := create_tween().set_parallel(true)
		tween.tween_property(battle_player_focus_root, "scale", player_target, 0.24).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		tween.tween_property(battle_enemy_focus_root, "scale", enemy_target, 0.24).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	_focus_pan = -0.22 if player_active else (0.24 if enemy_active else 0.0)

func _set_battle_focus_material(material: StandardMaterial3D, accent: Color, active: bool, broken: bool) -> void:
	if material == null:
		return
	var alpha := 0.56 if active else 0.18
	material.albedo_color = Color(accent.r * 0.34, accent.g * 0.34, accent.b * 0.34, alpha)
	material.emission = Color(1.0, 0.76, 0.24) if broken else accent
	material.emission_energy_multiplier = 1.65 if broken else (1.10 if active else 0.20)

func _build_atlas() -> void:
	var base_material := _make_material(_stone, Color.BLACK, 0.70)
	var route_material := _make_material(_accent.darkened(0.32), _accent, 0.94, 0.82)
	var contour_material := _make_material(_stone.lightened(0.10), _accent.darkened(0.30), 0.22, 0.34)
	var route_floor_material := _make_material(_accent.darkened(0.45), _accent, 0.30, 0.44)
	_add_floor_trace(motion_root, 5.05, 3.15, -0.38, 30, contour_material, 4)
	_add_floor_trace(motion_root, 3.35, 2.05, -0.37, 24, contour_material, 6)
	var route_points: Array[Vector3] = [
		Vector3(-3.75, 0.0, 1.95), Vector3(-2.95, 0.0, 1.10),
		Vector3(-2.05, 0.0, 0.35), Vector3(-1.20, 0.0, -0.52),
		Vector3(-0.22, 0.0, -1.18), Vector3(0.82, 0.0, -0.62),
		Vector3(1.72, 0.0, 0.12), Vector3(2.52, 0.0, -0.58),
		Vector3(3.25, 0.0, -1.42), Vector3(3.92, 0.0, -2.24),
	]
	for i in range(route_points.size() - 1):
		_add_beam_between(motion_root, route_points[i] + Vector3.UP * 0.09, route_points[i + 1] + Vector3.UP * 0.09, 0.055, route_material)
	for i in range(route_points.size()):
		_add_cylinder(motion_root, route_points[i] + Vector3(0, -0.32, 0), 0.34, 0.045, 16, route_floor_material)
		var marker_material := _make_material(_stone.lightened(0.17), _accent.darkened(0.60), 1.0, 0.24)
		var height := 0.48 + float(i) * 0.055
		var marker := _add_cylinder(motion_root, route_points[i] + Vector3.UP * (height * 0.5), 0.15, height, 8, marker_material)
		marker.set_meta("route_index", i)
		marker.set_meta("rest_scale", Vector3.ONE)
		_atlas_markers.append(marker)
		_atlas_materials.append(marker_material)
		_add_box(motion_root, route_points[i] + Vector3(0, height + 0.10, 0), Vector3(0.16, 0.16, 0.16), route_material, Vector3(0, PI * 0.25, PI * 0.25))
	_add_atlas_landmarks(route_points, base_material, route_material)
	focus_route(clampi(GameManager.current_chapter, 1, 10))

func _build_relic() -> void:
	var base_material := _make_material(_stone, Color.BLACK, 0.82)
	var accent_material := _make_material(_accent.darkened(0.16), _accent, 0.98, 1.15)
	var ghost_material := _make_material(_accent.darkened(0.52), _accent, 0.50, 0.54)
	var pedestal_material := _make_material(_stone.lightened(0.04), _accent.darkened(0.40), 0.54, 0.22)
	_add_cylinder(motion_root, Vector3(0, -0.78, 0), 2.35, 0.18, 32, pedestal_material)
	_add_cylinder(motion_root, Vector3(0, -0.59, 0), 1.52, 0.10, 24, ghost_material)
	focus_root = Node3D.new()
	focus_root.name = "MemoryRelic"
	motion_root.add_child(focus_root)
	_add_box(focus_root, Vector3(0, 0.38, 0), Vector3(0.78, 2.15, 0.78), base_material, Vector3(0.05, PI * 0.25, 0.0))
	_add_box(focus_root, Vector3(0, 1.62, 0), Vector3(0.46, 0.46, 0.46), accent_material, Vector3(0.0, PI * 0.25, PI * 0.25))
	orbit_root = Node3D.new()
	orbit_root.name = "MemoryOrbit"
	motion_root.add_child(orbit_root)
	for ring in range(2):
		var radius := 1.42 + float(ring) * 0.43
		var height := 0.45 + float(ring) * 0.44
		for i in range(18):
			var angle := TAU * float(i) / 18.0
			var position := Vector3(cos(angle) * radius, height + sin(angle * 2.0) * 0.12, sin(angle) * radius * 0.42)
			_add_box(orbit_root, position, Vector3(0.14, 0.045, 0.045), accent_material, Vector3(0, -angle, angle * 0.15))
	for i in range(7):
		var angle := TAU * float(i) / 7.0
		var position := Vector3(cos(angle) * 2.0, 0.1 + float(i % 3) * 0.48, sin(angle) * 1.2)
		_add_box(orbit_root, position, Vector3(0.11, 0.42, 0.11), ghost_material, Vector3(angle * 0.12, angle, angle * 0.08))

func _add_root_motif(dark: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		_add_box(motion_root, Vector3(side * 3.45, 0.05, -0.45), Vector3(0.52, 3.0, 0.52), dark, Vector3(0, 0, side * 0.18))
		_add_beam_between(motion_root, Vector3(side * 3.4, 1.0, -0.45), Vector3(side * 1.8, -0.88, 0.2), 0.12, dark)
	_add_box(motion_root, Vector3(0, -0.78, -0.2), Vector3(0.18, 0.72, 0.18), accent_material, Vector3(0, 0, PI * 0.25))

func _add_market_motif(dark: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		var x := side * 3.15
		_add_box(motion_root, Vector3(x, -0.15, -0.2), Vector3(1.3, 1.55, 0.92), dark)
		_add_box(motion_root, Vector3(x, 0.82, -0.2), Vector3(1.55, 0.12, 1.12), accent_material, Vector3(0, 0, side * 0.08))

func _add_signal_motif(mid: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		var x := side * 3.25
		_add_box(motion_root, Vector3(x, 0.35, -0.7), Vector3(0.20, 3.5, 0.20), mid, Vector3(0, 0, side * 0.06))
		_add_box(motion_root, Vector3(x, 1.15, -0.7), Vector3(1.0, 0.09, 0.09), accent_material, Vector3(0, 0, side * 0.18))

func _add_mast_motif(mid: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		var x := side * 3.4
		_add_box(motion_root, Vector3(x, 0.0, -0.5), Vector3(0.16, 3.7, 0.16), mid, Vector3(0, 0, side * 0.22))
		_add_box(motion_root, Vector3(x - side * 0.38, 0.72, -0.5), Vector3(0.72, 0.06, 0.06), accent_material, Vector3(0, 0, side * 0.22))

func _add_lantern_motif(dark: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		for row in range(3):
			var x := side * (2.5 + float(row) * 0.62)
			_add_box(motion_root, Vector3(x, -0.15 + float(row) * 0.18, -0.4), Vector3(0.12, 2.1, 0.12), dark)
			_add_box(motion_root, Vector3(x, 0.94 + float(row) * 0.18, -0.4), Vector3(0.22, 0.30, 0.22), accent_material, Vector3(0, PI * 0.25, PI * 0.25))

func _add_threshold_motif(dark: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	_add_box(motion_root, Vector3(-2.8, 0.15, -0.8), Vector3(0.36, 3.0, 0.50), dark)
	_add_box(motion_root, Vector3(2.8, 0.15, -0.8), Vector3(0.36, 3.0, 0.50), dark)
	_add_box(motion_root, Vector3(0, 1.52, -0.8), Vector3(5.9, 0.28, 0.50), dark)
	_add_box(motion_root, Vector3(0, 0.25, -0.75), Vector3(0.13, 2.25, 0.13), accent_material, Vector3(0, 0, PI * 0.25))

func _add_void_motif(ghost: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	orbit_root = Node3D.new()
	motion_root.add_child(orbit_root)
	for i in range(13):
		var angle := TAU * float(i) / 13.0
		var radius := 1.25 + float(i % 4) * 0.55
		var position := Vector3(cos(angle) * radius, -0.25 + float(i % 5) * 0.52, sin(angle) * 1.4 - 0.4)
		_add_box(orbit_root, position, Vector3(0.16, 0.62 + float(i % 3) * 0.18, 0.16), ghost, Vector3(angle * 0.08, angle, angle * 0.12))
	_add_box(motion_root, Vector3(0, 0.35, -0.3), Vector3(0.38, 2.7, 0.38), accent_material, Vector3(0, PI * 0.25, PI * 0.25))

func _add_marker_motif(mid: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		for row in range(3):
			var x := side * (2.15 + float(row) * 0.72)
			var height := 1.4 + float(row) * 0.55
			_add_box(motion_root, Vector3(x, -1.0 + height * 0.5, -0.55), Vector3(0.36, height, 0.36), mid, Vector3(0, side * 0.12, side * 0.04))
	_add_box(motion_root, Vector3(0, -0.15, -0.2), Vector3(0.20, 1.72, 0.20), accent_material, Vector3(0, PI * 0.25, PI * 0.25))

func _add_atlas_landmarks(points: Array[Vector3], base: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for i in range(points.size()):
		var side := -1.0 if i % 2 == 0 else 1.0
		var offset := Vector3(side * (0.26 + float(i % 3) * 0.08), 0.0, -0.18)
		var height := 0.20 + float(i % 4) * 0.08
		_add_box(motion_root, points[i] + offset + Vector3.UP * (height * 0.5), Vector3(0.10, height, 0.10), base, Vector3(0, float(i) * 0.31, side * 0.08))
		if i in [0, 4, 5, 9]:
			_add_box(motion_root, points[i] - offset + Vector3(0, height + 0.14, 0), Vector3(0.10, 0.22, 0.10), accent_material, Vector3(0, PI * 0.25, PI * 0.25))

func focus_route(chapter: int) -> void:
	if _atlas_markers.is_empty():
		return
	var focus_index := clampi(chapter - 1, 0, _atlas_markers.size() - 1)
	for i in range(_atlas_markers.size()):
		var marker := _atlas_markers[i]
		var material := _atlas_materials[i]
		var focused := i == focus_index
		material.albedo_color = _accent.lightened(0.08) if focused else _stone.lightened(0.17)
		material.emission = _accent if focused else _accent.darkened(0.68)
		material.emission_energy_multiplier = 1.45 if focused else 0.22
		var target_scale := Vector3(1.28, 1.42, 1.28) if focused else Vector3.ONE
		if OptionsMenu != null and OptionsMenu.is_reduce_motion():
			marker.scale = target_scale
		else:
			var tween := create_tween()
			tween.tween_property(marker, "scale", target_scale, 0.22).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	_focus_pan = lerpf(-0.32, 0.32, float(focus_index) / maxf(float(_atlas_markers.size() - 1), 1.0))

func pulse_impact(direction: float, strength: float = 1.0) -> void:
	if OptionsMenu != null and OptionsMenu.is_reduce_motion():
		return
	_impact_offset = clampf(direction, -1.0, 1.0) * clampf(strength, 0.0, 1.5) * 0.72
	_impact_lift = clampf(strength, 0.0, 1.5) * 0.22

func _process(delta: float) -> void:
	if camera == null or not is_instance_valid(camera):
		return
	_time += delta
	var reduce_motion := OptionsMenu != null and OptionsMenu.is_reduce_motion()
	if not reduce_motion:
		if orbit_root != null and is_instance_valid(orbit_root):
			orbit_root.rotation.y += delta * (0.24 if stage_mode == StageMode.RELIC else 0.10)
		if focus_root != null and is_instance_valid(focus_root):
			focus_root.position.y = sin(_time * 1.15) * 0.08
		if motion_root != null and is_instance_valid(motion_root):
			motion_root.rotation.y = sin(_time * 0.24) * (0.035 if stage_mode == StageMode.BATTLE else 0.018)
	_impact_offset = move_toward(_impact_offset, 0.0, delta * 3.4)
	_impact_lift = move_toward(_impact_lift, 0.0, delta * 1.8)
	var drift := sin(_time * 0.31) * 0.06 if not reduce_motion else 0.0
	var target_position := _base_camera_position + Vector3(_impact_offset + _focus_pan + drift, _impact_lift, absf(_impact_offset) * 0.12)
	camera.position = camera.position.lerp(target_position, 1.0 - exp(-9.0 * delta))
	camera.look_at(_camera_look_target + Vector3(_focus_pan * 0.24, 0, 0), Vector3.UP)

func _make_material(albedo: Color, emission_color: Color, alpha: float = 1.0, emission_energy: float = 0.0) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(albedo.r, albedo.g, albedo.b, alpha)
	material.roughness = 0.92
	material.metallic = 0.06
	if alpha < 0.999:
		material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	if emission_energy > 0.0:
		material.emission_enabled = true
		material.emission = emission_color
		material.emission_energy_multiplier = emission_energy
	return material

func _add_box(parent: Node3D, position: Vector3, size: Vector3, material: Material, rotation: Vector3 = Vector3.ZERO) -> MeshInstance3D:
	var mesh := BoxMesh.new()
	mesh.size = size
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.position = position
	instance.rotation = rotation
	instance.material_override = material
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(instance)
	return instance

func _add_cylinder(parent: Node3D, position: Vector3, radius: float, height: float, segments: int, material: Material) -> MeshInstance3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = height
	mesh.radial_segments = segments
	mesh.rings = 1
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.position = position
	instance.material_override = material
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	parent.add_child(instance)
	return instance

func _add_beam_between(parent: Node3D, start: Vector3, end: Vector3, radius: float, material: Material) -> MeshInstance3D:
	var direction := end - start
	var length := direction.length()
	var beam := _add_cylinder(parent, (start + end) * 0.5, radius, length, 6, material)
	if length > 0.001:
		beam.quaternion = Quaternion(Vector3.UP, direction.normalized())
	return beam

func _add_floor_trace(parent: Node3D, radius_x: float, radius_z: float, height: float, segments: int, material: Material, gap_stride: int = 0) -> void:
	var previous := Vector3(radius_x, height, 0)
	for i in range(1, segments + 1):
		var angle := TAU * float(i) / float(segments)
		var current := Vector3(cos(angle) * radius_x, height, sin(angle) * radius_z)
		if gap_stride <= 0 or i % gap_stride != 0:
			_add_beam_between(parent, previous, current, 0.026, material)
		previous = current
