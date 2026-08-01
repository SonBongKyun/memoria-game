## HybridDepthStage renders a small real-time 3D diorama into the existing 2D UI.
## The 2D game keeps ownership of collision, character identity, text and input;
## 3D is reserved for depth, landmarks and reaction to important actions.
class_name HybridDepthStage
extends Control

enum StageMode { BATTLE, ATLAS, RELIC, FOREGROUND }

## S211: 전투 무대 지면 기준.
## 2D 전투원의 발끝(battle_scene.STAGE_BASELINE_Y)과 3D 지평선이 같은 높이에서
## 만나야 "2D 캐릭터가 3D 공간에 서 있다"로 읽힌다. 두 값은 함께 조정할 것.
const ARENA_FLOOR_Y: float = -1.12
const ARENA_FLOOR_SIZE: float = 64.0

## S212: 2D 전투원이 서 있는 3D 지점.
## 무대 바닥의 포커스 링, 3D 접지 그림자, 그리고 2D 스프라이트의 카메라 결합이
## 모두 같은 좌표를 쓴다. 세 가지가 어긋나면 캐릭터가 공간에서 떠 버린다.
const ANCHOR_PLAYER: Vector3 = Vector3(-2.85, ARENA_FLOOR_Y, 0.55)
const ANCHOR_ALLY: Vector3 = Vector3(-4.35, ARENA_FLOOR_Y, -0.35)
const ANCHOR_SUPPORT: Vector3 = Vector3(-1.35, ARENA_FLOOR_Y, -0.95)
const ANCHOR_ENEMY: Vector3 = Vector3(2.85, ARENA_FLOOR_Y, 0.55)

## 무대가 그려지는 논리 캔버스. S210 이후 뷰포트는 1280x720으로 고정된다.
const CANVAS_SIZE: Vector2 = Vector2(1280.0, 720.0)

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

## GPT Image 2 환경 컷아웃은 평면 배경 장식이 아니라 실제 3D 월드의
## 깊이 표식으로 사용한다. 같은 Texture2D를 재사용해 드로우/메모리 비용을
## 억제하면서도, 절차적 박스만으로는 전달되지 않던 지역의 정체성을 보강한다.
const ILLUSTRATED_LANDMARK_PATHS: Dictionary = {
	"roots": "res://assets/environment/hybrid_depth/motif_root_spire_v1.png",
	"stalls": "res://assets/environment/hybrid_depth/motif_relay_obelisk_v1.png",
	"signals": "res://assets/environment/hybrid_depth/motif_relay_obelisk_v1.png",
	"markers": "res://assets/environment/hybrid_depth/motif_relay_obelisk_v1.png",
	"masts": "res://assets/environment/hybrid_depth/motif_wrecked_mast_v1.png",
	"lanterns": "res://assets/environment/hybrid_depth/motif_memory_lantern_v1.png",
	"threshold": "res://assets/environment/hybrid_depth/motif_void_monolith_v1.png",
	"void": "res://assets/environment/hybrid_depth/motif_void_monolith_v1.png",
	"routes": "res://assets/environment/hybrid_depth/motif_relay_obelisk_v1.png",
}
const MEMORY_LANTERN_PATH := "res://assets/environment/hybrid_depth/motif_memory_lantern_v1.png"
const ROOT_SPIRE_PATH := "res://assets/environment/hybrid_depth/motif_root_spire_v1.png"
const RELAY_OBELISK_PATH := "res://assets/environment/hybrid_depth/motif_relay_obelisk_v1.png"
const WRECKED_MAST_PATH := "res://assets/environment/hybrid_depth/motif_wrecked_mast_v1.png"
const VOID_MONOLITH_PATH := "res://assets/environment/hybrid_depth/motif_void_monolith_v1.png"

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
var arena_floor: MeshInstance3D
var _rest_projection_cache: Dictionary = {}
var _contact_shadows: Dictionary = {}
const ENTRANCE_DURATION: float = 1.15
var _burn_flare: float = 0.0
var _burn_ember_root: Node3D = null
var _burn_ember_life: float = 0.0
## S213: 전경 레이어는 전투 무대의 카메라 상태를 그대로 따라간다.
## 리그가 어긋나면 전경과 배경의 시차가 맞지 않아 한 공간으로 읽히지 않는다.
var follow_stage: HybridDepthStage = null
var _entrance_active: bool = false
var _entrance_progress: float = 0.0
var battle_player_focus_root: Node3D
var battle_enemy_focus_root: Node3D
var _battle_player_focus_material: StandardMaterial3D
var _battle_enemy_focus_material: StandardMaterial3D
var illustrated_landmarks: Array[Sprite3D] = []

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
		StageMode.FOREGROUND:
			_build_foreground()
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
	# S211: 공기 원근.
	# 먼 기둥이 배경 색으로 녹아들면, 같은 저폴리 상자들도 "깊이가 있는 공간"으로
	# 읽힌다. 3D를 늘리지 않고 깊이감만 얻는 가장 저렴한 방법이다.
	if stage_mode == StageMode.BATTLE:
		environment.fog_enabled = true
		environment.fog_mode = Environment.FOG_MODE_DEPTH
		environment.fog_light_color = _stone.lightened(0.06)
		environment.fog_light_energy = 0.9
		environment.fog_density = 0.0
		environment.fog_depth_begin = 9.0
		environment.fog_depth_end = 34.0
		environment.fog_depth_curve = 1.35
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
			# FOREGROUND는 전투 무대와 같은 카메라 리그를 쓴다. 리그가 다르면
			# 전경과 배경의 시차가 어긋나 한 공간으로 읽히지 않는다.
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

	# S211: 실제 바닥면.
	# 점선 타원만 있던 시절에는 3D가 켜져 있어도 캐릭터가 "어떤 공간"에 서 있는지
	# 읽히지 않았다. 카메라 쪽은 진하고 지평선 쪽은 사라지는 바닥을 깔아, 배경
	# 일러스트를 덮지 않으면서도 발밑에 땅이 생기게 한다.
	_add_arena_floor()

	# 무대 경계를 나타내던 기존 궤적은 바닥 위의 은은한 표식으로 남긴다.
	_add_floor_trace(motion_root, 5.65, 2.65, ARENA_FLOOR_Y + 0.02, 26, floor_trace, 3)
	_add_floor_trace(motion_root, 3.75, 1.72, ARENA_FLOOR_Y + 0.03, 22, memory_trace, 5)
	_build_battle_focus_rings()

	# S211: 깊이 레이어.
	# 예전에는 기둥 9개가 한 줄로만 서 있어서 평면적이었다. 세 겹으로 나눠 배치하면
	# 카메라가 움직일 때 레이어끼리 시차가 생기고, 안개가 먼 열을 배경으로 녹인다.
	var depth_layers := [
		{"z": -3.4, "count": 11, "spread": 8.4, "height": 2.1, "width": 0.34, "material": mid_material},
		{"z": -7.2, "count": 9, "spread": 13.0, "height": 3.4, "width": 0.52, "material": dark_material},
		{"z": -12.5, "count": 7, "spread": 19.0, "height": 5.2, "width": 0.78, "material": dark_material},
	]
	for layer: Dictionary in depth_layers:
		var count: int = int(layer["count"])
		var spread: float = float(layer["spread"])
		var z: float = float(layer["z"])
		for i in range(count):
			var t: float = float(i) / float(maxi(count - 1, 1))
			var x: float = lerp(-spread, spread, t)
			# 결정적 흔들림. 매번 같은 배치를 만들어 캡처 비교가 가능하다.
			var jitter: float = sin(float(i) * 2.39 + z) * 0.55
			var height: float = float(layer["height"]) * (0.78 + absf(sin(float(i) * 1.7)) * 0.42)
			var width: float = float(layer["width"])
			_add_box(
				motion_root,
				Vector3(x + jitter, ARENA_FLOOR_Y + height * 0.5, z + cos(float(i) * 1.3) * 0.8),
				Vector3(width, height, width),
				layer["material"],
				Vector3(0.0, sin(float(i)) * 0.2, 0.0)
			)

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
	_build_illustrated_battle_layer()

	for i in range(9):
		var shard_angle := TAU * float(i) / 9.0
		var shard_pos := Vector3(cos(shard_angle) * 5.6, 0.35 + float(i % 3) * 0.62, sin(shard_angle) * 2.6 - 3.1)
		_add_box(motion_root, shard_pos, Vector3(0.08, 0.38 + float(i % 2) * 0.18, 0.08), ghost_material, Vector3(shard_angle * 0.15, shard_angle, shard_angle * 0.08))

	# S212: 전투원의 접지 그림자를 3D 바닥 위에 눕힌다.
	# 2D 타원은 화면에 붙어 있어서 카메라가 움직이면 캐릭터와 함께 미끄러졌다.
	# 바닥면에 놓인 그림자는 원근을 따라 눌리고 함께 패닝되므로, 캐릭터가 그 지점에
	# "닿아 있다"는 인상이 생긴다.
	_contact_shadows["player"] = _add_contact_shadow(ANCHOR_PLAYER, 1.05, 0.42)
	_contact_shadows["ally"] = _add_contact_shadow(ANCHOR_ALLY, 0.82, 0.34)
	_contact_shadows["support"] = _add_contact_shadow(ANCHOR_SUPPORT, 0.82, 0.32)
	_contact_shadows["enemy"] = _add_contact_shadow(ANCHOR_ENEMY, 1.30, 0.46)
	set_battle_focus("neutral")

## S212: 바닥에 눕는 원형 접지 그림자.
func _add_contact_shadow(anchor: Vector3, radius: float, strength: float) -> MeshInstance3D:
	var plane := PlaneMesh.new()
	plane.size = Vector2(radius * 2.0, radius * 2.0)
	var instance := MeshInstance3D.new()
	instance.name = "ContactShadow"
	instance.mesh = plane
	# 바닥면과 z-fighting 하지 않도록 아주 살짝 띄운다.
	instance.position = Vector3(anchor.x, ARENA_FLOOR_Y + 0.012, anchor.z)
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF

	var shader_path := "res://assets/shaders/arena_contact_shadow.gdshader"
	if ResourceLoader.exists(shader_path):
		var material := ShaderMaterial.new()
		material.shader = load(shader_path)
		material.set_shader_parameter("shadow_strength", strength)
		instance.material_override = material
	else:
		instance.material_override = _make_material(Color.BLACK, Color.BLACK, strength)
	scene_root.add_child(instance)
	return instance

## S211: 무대 바닥면. 가까울수록 진하고 지평선 쪽으로 사라진다.
func _add_arena_floor() -> void:
	var plane := PlaneMesh.new()
	plane.size = Vector2(ARENA_FLOOR_SIZE, ARENA_FLOOR_SIZE)
	plane.subdivide_width = 1
	plane.subdivide_depth = 1

	var instance := MeshInstance3D.new()
	instance.name = "ArenaFloor"
	instance.mesh = plane
	# 바닥 중심을 카메라 앞쪽으로 당겨, 화면 아래 절반이 지면으로 채워지게 한다.
	instance.position = Vector3(0, ARENA_FLOOR_Y, -ARENA_FLOOR_SIZE * 0.5 + 12.0)
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF

	var shader_path := "res://assets/shaders/arena_floor.gdshader"
	if ResourceLoader.exists(shader_path):
		var material := ShaderMaterial.new()
		material.shader = load(shader_path)
		material.set_shader_parameter("floor_color", Color(_stone.r, _stone.g, _stone.b))
		material.set_shader_parameter("grid_color", _accent.darkened(0.25))
		material.set_shader_parameter("near_alpha", 0.80)
		material.set_shader_parameter("fade_start", 5.0)
		material.set_shader_parameter("fade_end", 30.0)
		material.set_shader_parameter("grid_spacing", 2.2)
		material.set_shader_parameter("grid_width", 0.035)
		material.set_shader_parameter("grid_strength", 0.50)
		material.set_shader_parameter("side_fade", 16.0)
		instance.material_override = material
	else:
		instance.material_override = _make_material(_stone, Color.BLACK, 0.55)
	# 바닥은 흔들리는 motion_root가 아니라 고정된 scene_root에 붙인다.
	# 배경 기둥이 미묘하게 흔들리는 것은 공기감이지만, 지면이 함께 흔들리면
	# 캐릭터가 딛고 선 땅이 출렁이는 것처럼 보인다.
	scene_root.add_child(instance)
	arena_floor = instance

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
	# S212: 턴 포커스 이동량.
	# 0.22는 화면에서 2px도 안 되는 이동이라, 3D와 결합해 놓아도 아무도 알아채지
	# 못했다. 공간이 실제로 움직였다고 읽히는 최소치까지 올린다.
	_focus_pan = -0.85 if player_active else (0.92 if enemy_active else 0.0)

func _set_battle_focus_material(material: StandardMaterial3D, accent: Color, active: bool, broken: bool) -> void:
	if material == null:
		return
	var alpha := 0.56 if active else 0.18
	material.albedo_color = Color(accent.r * 0.34, accent.g * 0.34, accent.b * 0.34, alpha)
	material.emission = Color(1.0, 0.76, 0.24) if broken else accent
	material.emission_energy_multiplier = 1.65 if broken else (1.10 if active else 0.20)

## S213: 전경 레이어.
##
## 시차는 가까운 물체에서 가장 강하게 읽힌다. 지금까지 3D는 전부 캐릭터 뒤에 있어서
## 깊이의 절반만 쓰고 있었다. 카메라 앞쪽에 몇 개를 두고 2D 캐릭터 "위"에 합성하면
## 카메라가 조금만 움직여도 공간감이 크게 살아난다.
##
## 다만 전투는 정보를 읽어야 하는 화면이다. 그래서 화면 양쪽 가장자리에만 세우고,
## 초점이 나간 것처럼 어둡고 흐릿하게 둔다. 가운데(전투원과 UI)는 절대 가리지 않는다.
func _build_foreground() -> void:
	var near_material := _make_material(_stone.darkened(0.55), Color.BLACK, 0.44)
	# 화면 양쪽 가장자리만 걸치게 한다.
	# 배치는 눈대중이 아니라 실측으로 잡았다 (probe로 화면 점유율을 재서 결정):
	#   x=±3.2 -> 한쪽 12%  (너무 많이 먹는다)
	#   x=±3.5 -> 한쪽 7.2% (채택)
	#   x=±3.9 -> 한쪽 0.8% (거의 안 보인다)
	# 전투원은 화면의 18%와 78% 지점에 서므로 어느 쪽도 가리지 않는다.
	for side: float in [-1.0, 1.0]:
		_add_box(
			motion_root,
			Vector3(side * 3.50, 1.2, 7.2),
			Vector3(1.5, 9.0, 1.0),
			near_material,
			Vector3(0.0, side * 0.16, side * 0.03)
		)
		_add_box(
			motion_root,
			Vector3(side * 3.15, 0.6, 8.4),
			Vector3(2.0, 11.0, 1.0),
			near_material,
			Vector3(0.0, side * -0.12, 0.0)
		)
	_build_illustrated_foreground_layer()

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
	_add_illustrated_atlas_landmarks(route_points)
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
	_add_illustrated_relic()
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

## S211: 바이옴 색을 지닌 배경 랜드마크.
## 예전에는 x=0에 밝은 기둥 하나를 세웠는데, 그 자리는 두 전투원 사이의 시선이
## 모이는 지점이라 3D를 켜는 순간 정체불명의 발광 상자가 화면 중앙에 떠 버렸다.
## 좌우로 갈라 안개 속에 세우면 같은 색 정보를 주면서 무대 중앙을 비워 둔다.
func _add_flanking_landmarks(accent_material: StandardMaterial3D, size: Vector3) -> void:
	for side: float in [-1.0, 1.0]:
		_add_box(
			motion_root,
			Vector3(side * 5.4, ARENA_FLOOR_Y + size.y * 0.5, -7.4),
			size,
			accent_material,
			Vector3(0, side * 0.35, 0)
		)

func _add_root_motif(dark: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		_add_box(motion_root, Vector3(side * 3.45, 0.05, -0.45), Vector3(0.52, 3.0, 0.52), dark, Vector3(0, 0, side * 0.18))
		_add_beam_between(motion_root, Vector3(side * 3.4, 1.0, -0.45), Vector3(side * 1.8, -0.88, 0.2), 0.12, dark)
	_add_flanking_landmarks(accent_material, Vector3(0.34, 1.6, 0.34))

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
	_add_flanking_landmarks(accent_material, Vector3(0.26, 4.0, 0.26))

func _add_void_motif(ghost: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	orbit_root = Node3D.new()
	motion_root.add_child(orbit_root)
	for i in range(13):
		var angle := TAU * float(i) / 13.0
		var radius := 1.25 + float(i % 4) * 0.55
		var position := Vector3(cos(angle) * radius, -0.25 + float(i % 5) * 0.52, sin(angle) * 1.4 - 0.4)
		_add_box(orbit_root, position, Vector3(0.16, 0.62 + float(i % 3) * 0.18, 0.16), ghost, Vector3(angle * 0.08, angle, angle * 0.12))
	_add_flanking_landmarks(accent_material, Vector3(0.55, 4.4, 0.55))

func _add_marker_motif(mid: StandardMaterial3D, accent_material: StandardMaterial3D) -> void:
	for side: float in [-1.0, 1.0]:
		for row in range(3):
			var x := side * (2.15 + float(row) * 0.72)
			var height := 1.4 + float(row) * 0.55
			_add_box(motion_root, Vector3(x, -1.0 + height * 0.5, -0.55), Vector3(0.36, height, 0.36), mid, Vector3(0, side * 0.12, side * 0.04))
	_add_flanking_landmarks(accent_material, Vector3(0.36, 3.0, 0.36))

func _primary_landmark_path() -> String:
	return String(ILLUSTRATED_LANDMARK_PATHS.get(_motif, RELAY_OBELISK_PATH))

func _add_illustrated_landmark(
	parent: Node3D,
	texture_path: String,
	world_position: Vector3,
	pixel_scale: float,
	tint: Color,
	flip_h: bool = false,
	landmark_name: String = "IllustratedLandmark"
) -> Sprite3D:
	if parent == null or not ResourceLoader.exists(texture_path):
		return null
	var texture := load(texture_path) as Texture2D
	if texture == null:
		return null
	var sprite := Sprite3D.new()
	sprite.name = landmark_name
	sprite.texture = texture
	sprite.position = world_position
	sprite.pixel_size = pixel_scale
	sprite.centered = true
	sprite.flip_h = flip_h
	sprite.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	sprite.double_sided = true
	# 에셋 자체에 이미 완성된 회화적 광원이 있으므로 3D 조명을 다시 곱하지 않는다.
	# stage tint와 기억 연소 tint만 공유해 검게 뭉개지지 않게 한다.
	sprite.shaded = false
	sprite.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
	sprite.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	sprite.modulate = tint
	sprite.set_meta("base_position", world_position)
	sprite.set_meta("base_modulate", tint)
	sprite.set_meta("landmark_path", texture_path)
	parent.add_child(sprite)
	illustrated_landmarks.append(sprite)
	return sprite

func _add_illustrated_pair(texture_path: String, half_span: float, depth: float, center_y: float, pixel_scale: float, tint: Color) -> void:
	_add_illustrated_landmark(
		motion_root, texture_path, Vector3(-half_span, center_y, depth), pixel_scale,
		tint, false, "IllustratedLandmarkLeft"
	)
	_add_illustrated_landmark(
		motion_root, texture_path, Vector3(half_span, center_y, depth - 0.65), pixel_scale,
		tint.darkened(0.08), true, "IllustratedLandmarkRight"
	)

## 전투 중앙은 캐릭터와 명령 판독을 위해 비워 둔다. 삽화는 좌우 원근층에만 배치해
## 실루엣과 카메라 패닝으로 깊이를 전달한다.
func _build_illustrated_battle_layer() -> void:
	match _motif:
		"roots":
			_add_illustrated_pair(ROOT_SPIRE_PATH, 4.75, -5.7, 1.88, 0.0039, Color(0.92, 0.98, 0.94, 0.84))
		"stalls":
			_add_illustrated_pair(RELAY_OBELISK_PATH, 4.60, -6.2, 1.60, 0.00355, Color(1.0, 0.92, 0.80, 0.80))
			_add_illustrated_lanterns(3, 3.1, -4.2, 0.12, 0.00134, 0.84)
		"signals":
			_add_illustrated_pair(RELAY_OBELISK_PATH, 4.55, -5.5, 1.60, 0.00355, Color(0.96, 0.92, 0.84, 0.84))
		"masts":
			_add_illustrated_pair(WRECKED_MAST_PATH, 4.80, -6.0, 1.88, 0.0039, Color(0.90, 0.98, 1.0, 0.84))
		"lanterns":
			_add_illustrated_lanterns(7, 5.15, -5.0, 0.12, 0.00134, 0.94)
		"threshold":
			_add_illustrated_pair(VOID_MONOLITH_PATH, 4.75, -6.4, 1.60, 0.00355, Color(0.94, 0.88, 1.0, 0.82))
			_add_illustrated_lanterns(2, 3.0, -4.4, 0.08, 0.00124, 0.72)
		"void":
			_add_illustrated_pair(VOID_MONOLITH_PATH, 4.65, -5.8, 1.60, 0.00355, Color(0.94, 0.86, 1.0, 0.88))
		_:
			_add_illustrated_pair(RELAY_OBELISK_PATH, 4.65, -6.0, 1.60, 0.00355, Color(0.94, 0.92, 0.88, 0.78))

func _add_illustrated_lanterns(count: int, half_span: float, depth: float, center_y: float, pixel_scale: float, alpha: float) -> void:
	for i in range(maxi(count, 1)):
		var ratio: float = 0.5 if count <= 1 else float(i) / float(count - 1)
		var x := lerpf(-half_span, half_span, ratio)
		var arc := 1.0 - absf(ratio * 2.0 - 1.0)
		_add_illustrated_landmark(
			motion_root,
			MEMORY_LANTERN_PATH,
			Vector3(x, center_y + arc * 0.28, depth - arc * 0.85),
			pixel_scale * (0.92 + arc * 0.12),
			Color(1.0, 0.94, 0.82, alpha * (0.78 + arc * 0.22)),
			i % 2 == 1,
			"MemoryLantern%02d" % i
		)

func _build_illustrated_foreground_layer() -> void:
	var path := _primary_landmark_path()
	var scale := 0.00175 if path == MEMORY_LANTERN_PATH else 0.0043
	var center_y := 0.22 if path == MEMORY_LANTERN_PATH else 2.18
	var tint := Color(Color.WHITE.lerp(_accent.lightened(0.10), 0.18), 0.42)
	_add_illustrated_landmark(motion_root, path, Vector3(-3.62, center_y, 7.35), scale, tint, false, "IllustratedForegroundLeft")
	_add_illustrated_landmark(motion_root, path, Vector3(3.62, center_y, 7.70), scale * 1.08, tint.darkened(0.10), true, "IllustratedForegroundRight")

func _add_illustrated_atlas_landmarks(points: Array[Vector3]) -> void:
	var route_art := [
		{"index": 0, "path": ROOT_SPIRE_PATH},
		{"index": 2, "path": RELAY_OBELISK_PATH},
		{"index": 4, "path": WRECKED_MAST_PATH},
		{"index": 5, "path": MEMORY_LANTERN_PATH},
		{"index": 9, "path": VOID_MONOLITH_PATH},
	]
	for entry: Dictionary in route_art:
		var index: int = int(entry["index"])
		if index < 0 or index >= points.size():
			continue
		var path := String(entry["path"])
		var scale := 0.00034 if path == MEMORY_LANTERN_PATH else 0.00048
		var center_y := 0.28 if path == MEMORY_LANTERN_PATH else 0.40
		_add_illustrated_landmark(
			motion_root, path, points[index] + Vector3(0.0, center_y, -0.08), scale,
			Color(1.0, 0.96, 0.88, 0.76), index % 2 == 0, "AtlasLandmark%02d" % index
		)

func _add_illustrated_relic() -> void:
	var path := _primary_landmark_path()
	var scale := 0.00180 if path == MEMORY_LANTERN_PATH else 0.00218
	var center_y := 0.44 if path == MEMORY_LANTERN_PATH else 0.78
	_add_illustrated_landmark(
		focus_root, path, Vector3(0.0, center_y, 0.22), scale,
		Color(1.0, 0.98, 1.0, 0.96), false, "IllustratedMemoryRelic"
	)

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

## ===================== S213: 기억 연소에 반응하는 무대 =====================
## 기억 연소는 이 게임의 심장이고, 되돌릴 수 없는 대가를 치르는 순간이다.
## 그 순간에 무대 전체가 반응하면, 3D는 장식이 아니라 메카닉의 일부가 된다.
##
## 바닥 격자가 한 번 타오르고, 잔불이 지면에서 솟았다가 사그라든다.
## 연소 등급이 높을수록 오래, 강하게 남는다.
func play_memory_burn(grade: int) -> void:
	if stage_mode != StageMode.BATTLE:
		return
	var intensity: float = clampf(0.55 + float(grade) * 0.22, 0.55, 1.4)
	_burn_flare = intensity
	_spawn_burn_embers(intensity)

func _spawn_burn_embers(intensity: float) -> void:
	if OptionsMenu != null and OptionsMenu.is_reduce_motion():
		return
	if _burn_ember_root != null and is_instance_valid(_burn_ember_root):
		_burn_ember_root.queue_free()
	_burn_ember_root = Node3D.new()
	_burn_ember_root.name = "BurnEmbers"
	scene_root.add_child(_burn_ember_root)

	var ember_material := _make_material(Color(0.92, 0.44, 0.16), Color(1.0, 0.62, 0.22), 0.85, 2.4)
	var count: int = int(round(14.0 * intensity))
	for i in range(count):
		var angle := TAU * float(i) / float(maxi(count, 1))
		var radius := 1.2 + float(i % 5) * 0.62
		var ember := _add_box(
			_burn_ember_root,
			Vector3(cos(angle) * radius, ARENA_FLOOR_Y + 0.05, sin(angle) * radius * 0.55 + 0.3),
			Vector3(0.075, 0.075, 0.075),
			ember_material
		)
		# 각 잔불은 서로 다른 속도로 솟는다. _process에서 이 메타를 읽는다.
		ember.set_meta("rise_speed", 1.05 + float(i % 4) * 0.42)
		ember.set_meta("drift", Vector3(cos(angle) * 0.22, 0.0, sin(angle) * 0.14))
	_burn_ember_life = 1.0

func _update_burn(delta: float) -> void:
	if _burn_flare > 0.0:
		_burn_flare = maxf(_burn_flare - delta * 1.25, 0.0)
		if arena_floor != null and is_instance_valid(arena_floor):
			var material := arena_floor.material_override as ShaderMaterial
			if material != null:
				# 연소 순간 격자가 타오르고, 색이 잔불 쪽으로 밀린다.
				material.set_shader_parameter("grid_strength", 0.50 + _burn_flare * 1.15)
				material.set_shader_parameter("grid_color", _accent.darkened(0.25).lerp(Color(1.0, 0.58, 0.20), minf(_burn_flare, 1.0)))

	if _burn_ember_root == null or not is_instance_valid(_burn_ember_root):
		return
	_burn_ember_life = maxf(_burn_ember_life - delta * 0.62, 0.0)
	if _burn_ember_life <= 0.0:
		_burn_ember_root.queue_free()
		_burn_ember_root = null
		return
	for child in _burn_ember_root.get_children():
		var ember := child as MeshInstance3D
		if ember == null:
			continue
		var rise: float = float(ember.get_meta("rise_speed", 1.0))
		var drift: Vector3 = ember.get_meta("drift", Vector3.ZERO)
		ember.position += (Vector3(0, rise, 0) + drift) * delta
		# 위로 갈수록 작아지며 사라진다.
		var scale_factor: float = maxf(_burn_ember_life, 0.05)
		ember.scale = Vector3.ONE * scale_factor

## S212: 전투 시작 돌리 인.
## 카메라를 뒤/위에서 기준 자세로 밀어 넣어 공간을 한 번 보여 준다. 정지 화면만
## 보면 3D인지 알기 어렵지만, 이 짧은 이동 한 번이면 깊이가 즉시 읽힌다.
## Reduce Motion에서는 건너뛴다.
func play_entrance() -> void:
	if stage_mode != StageMode.BATTLE or camera == null or not is_instance_valid(camera):
		return
	if OptionsMenu != null and OptionsMenu.is_reduce_motion():
		return
	_entrance_progress = 0.0
	_entrance_active = true

## 돌리 인 진행 중이면 카메라 기준 자세에 더해질 오프셋을 돌려준다.
func _entrance_offset(delta: float) -> Vector3:
	if not _entrance_active:
		return Vector3.ZERO
	_entrance_progress = minf(_entrance_progress + delta / ENTRANCE_DURATION, 1.0)
	if _entrance_progress >= 1.0:
		_entrance_active = false
	# 감속 곡선. 뒤(+z)와 위(+y)에서 부드럽게 내려앉는다.
	var remaining := 1.0 - ease(_entrance_progress, 0.35)
	return Vector3(0.0, 1.5 * remaining, 4.2 * remaining)

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
	_update_burn(delta)
	if follow_stage != null and is_instance_valid(follow_stage):
		# 리더의 카메라 상태를 복사한 뒤, 아래 공통 경로가 그대로 카메라를 옮긴다.
		_focus_pan = follow_stage._focus_pan
		_impact_offset = follow_stage._impact_offset
		_impact_lift = follow_stage._impact_lift
		_burn_flare = follow_stage._burn_flare
	_update_illustrated_landmarks(reduce_motion)
	_impact_offset = move_toward(_impact_offset, 0.0, delta * 3.4)
	_impact_lift = move_toward(_impact_lift, 0.0, delta * 1.8)
	var drift := sin(_time * 0.31) * 0.06 if not reduce_motion else 0.0
	var target_position := _base_camera_position + Vector3(_impact_offset + _focus_pan + drift, _impact_lift, absf(_impact_offset) * 0.12) + _entrance_offset(delta)
	camera.position = camera.position.lerp(target_position, 1.0 - exp(-9.0 * delta))
	camera.look_at(_camera_look_target + Vector3(_focus_pan * 0.24, 0, 0), Vector3.UP)

func _update_illustrated_landmarks(reduce_motion: bool) -> void:
	for i in range(illustrated_landmarks.size()):
		var sprite := illustrated_landmarks[i]
		if sprite == null or not is_instance_valid(sprite):
			continue
		var base_position: Vector3 = sprite.get_meta("base_position", sprite.position)
		if not reduce_motion and stage_mode in [StageMode.BATTLE, StageMode.FOREGROUND]:
			var phase := _time * 0.62 + float(i) * 0.77
			sprite.position = base_position + Vector3(0.0, sin(phase) * 0.028, 0.0)
		else:
			sprite.position = base_position
		var base_modulate: Color = sprite.get_meta("base_modulate", sprite.modulate)
		var burn_mix := clampf(_burn_flare * 0.30, 0.0, 0.34)
		var burn_tint := Color(1.0, 0.58, 0.24, base_modulate.a)
		sprite.modulate = base_modulate.lerp(burn_tint, burn_mix)

## ===================== S212: 3D ↔ 2D 결합 =====================
## 지금까지 하이브리드는 사실상 "3D 배경 위에 2D를 얹은" 구조였다. 카메라가 패닝해도
## 캐릭터는 화면에 못 박힌 듯 가만히 있어서, 두 층이 각자 놀았다.
##
## 여기서는 각 전투원에게 3D 앵커를 주고, 카메라가 움직였을 때 그 앵커가 화면에서
## 얼마나 이동했는지를 2D 오프셋으로 돌려준다. 기준 자세에서는 오프셋이 정확히
## 0이므로 기존 레이아웃과 트윈은 그대로 유지되고, 카메라가 흔들리거나 좌우로
## 포커스를 옮길 때만 캐릭터가 공간과 함께 움직인다.

## 캔버스 좌표(2D 발 위치)를 무대 바닥면 위의 3D 좌표로 되돌린다.
##
## 3D 앵커를 손으로 적어 두면 2D 배치를 조금만 바꿔도 그림자와 캐릭터가 어긋난다.
## 대신 "2D에서 발이 닿는 화면 지점"을 바닥 평면에 역투영해서 3D 좌표를 얻으면,
## 두 좌표계가 언제나 같은 자리를 가리킨다.
func canvas_to_floor(canvas_point: Vector2) -> Vector3:
	if camera == null or not is_instance_valid(camera) or viewport == null:
		return Vector3(0, ARENA_FLOOR_Y, 0)
	var viewport_size := Vector2(viewport.size)
	if viewport_size.x <= 0.0 or viewport_size.y <= 0.0:
		return Vector3(0, ARENA_FLOOR_Y, 0)
	var viewport_point := Vector2(
		canvas_point.x / CANVAS_SIZE.x * viewport_size.x,
		canvas_point.y / CANVAS_SIZE.y * viewport_size.y
	)
	# 기준 자세에서 역산해야 패닝 중에 앵커가 흔들리지 않는다.
	var saved_transform := camera.transform
	camera.look_at_from_position(_base_camera_position, _camera_look_target, Vector3.UP)
	var ray_origin := camera.project_ray_origin(viewport_point)
	var ray_direction := camera.project_ray_normal(viewport_point)
	camera.transform = saved_transform

	if absf(ray_direction.y) < 0.0001:
		return Vector3(0, ARENA_FLOOR_Y, 0)
	var distance := (ARENA_FLOOR_Y - ray_origin.y) / ray_direction.y
	if distance <= 0.0:
		return Vector3(0, ARENA_FLOOR_Y, 0)
	var hit := ray_origin + ray_direction * distance
	return Vector3(hit.x, ARENA_FLOOR_Y, hit.z)

## 전투 무대의 접지 그림자/포커스 링을 실제 전투원 위치로 옮긴다.
func place_battler_anchor(key: String, world_position: Vector3) -> void:
	var shadow: MeshInstance3D = _contact_shadows.get(key, null)
	if shadow != null and is_instance_valid(shadow):
		shadow.position = Vector3(world_position.x, ARENA_FLOOR_Y + 0.012, world_position.z)
	match key:
		"player":
			if battle_player_focus_root != null and is_instance_valid(battle_player_focus_root):
				battle_player_focus_root.position = Vector3(world_position.x, 0.0, world_position.z)
		"enemy":
			if battle_enemy_focus_root != null and is_instance_valid(battle_enemy_focus_root):
				battle_enemy_focus_root.position = Vector3(world_position.x, 0.0, world_position.z)

## 3D 월드 좌표를 논리 캔버스(1280x720) 좌표로 변환한다.
func world_to_canvas(world_position: Vector3) -> Vector2:
	if camera == null or not is_instance_valid(camera) or viewport == null:
		return Vector2.ZERO
	var viewport_size := Vector2(viewport.size)
	if viewport_size.x <= 0.0 or viewport_size.y <= 0.0:
		return Vector2.ZERO
	var point := camera.unproject_position(world_position)
	return Vector2(
		point.x / viewport_size.x * CANVAS_SIZE.x,
		point.y / viewport_size.y * CANVAS_SIZE.y
	)

## 카메라 기준 자세 대비, 이 앵커가 화면에서 이동한 양.
## 2D 전투원 컨테이너의 부모에 그대로 더하면 3D 공간과 함께 움직인다.
func anchor_offset(world_position: Vector3) -> Vector2:
	if camera == null or not is_instance_valid(camera):
		return Vector2.ZERO
	var current := world_to_canvas(world_position)
	var rest: Vector2 = _rest_projection_cache.get(world_position, Vector2.INF)
	if rest == Vector2.INF:
		rest = _compute_rest_projection(world_position)
		_rest_projection_cache[world_position] = rest
	return current - rest

## 카메라를 기준 자세로 되돌려 놓고 한 번만 투영해 둔다 (앵커별 캐시).
func _compute_rest_projection(world_position: Vector3) -> Vector2:
	var saved_transform := camera.transform
	camera.look_at_from_position(_base_camera_position, _camera_look_target, Vector3.UP)
	var rest := world_to_canvas(world_position)
	camera.transform = saved_transform
	return rest

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
