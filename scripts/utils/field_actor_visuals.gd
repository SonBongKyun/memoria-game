## Shared presentation language for every exploration character.
##
## The authored field paintings remain the source of identity.  This layer only
## supplies the things a still illustration cannot: a stable silhouette,
## contact with the floor, role-colored Memory accents, and restrained motion
## for ambient walkers.
class_name FieldActorVisuals
extends Node

const FINISH_SHADER: Shader = preload("res://assets/shaders/field_actor_finish.gdshader")
const GROUNDING_NAME: StringName = &"FieldGrounding"
const AMBIENT_DRIVER_NAME: StringName = &"FieldMotionDriver"

## S235: 서 있는 자세의 진폭 (지면 위 픽셀 기준, 몸 높이 50px 대비).
## 호흡 0.55px는 카메라 줌 2.25에서 약 1.2 화면 픽셀이다. 읽히되 흔들리지 않는 폭.
const IDLE_BREATH_PX: float = 0.55
const IDLE_SWAY_PX: float = 0.26

var _ambient_actor: AnimatedSprite2D
var _ambient_grounding: Node2D
var _rest_offset: Vector2 = Vector2.ZERO
var _rest_scale: Vector2 = Vector2.ONE
var _last_position: Vector2 = Vector2.ZERO
var _travel_phase: float = 0.0
var _idle_phase: float = 0.0
var _initialized: bool = false
## 로컬 offset을 "지면 위 픽셀"로 되돌리는 보정. 스프라이트 스케일의 역수다.
var _amplitude_comp: float = 1.0


## S237: 실루엣은 두 겹으로 읽힌다.
## 바깥은 어두워서 밝은 지형에서 캐릭터를 떼어 내고, 안쪽은 밝아서 어두운 지형에서
## 캐릭터를 떠오르게 한다. 어두운 한 겹만으로는 어두운 맵에서 사라진다
## (실측: forgotten_forest에서 몸통-배경 차이 0.0014).
const SILHOUETTE_OUTLINE: Color = Color(0.025, 0.035, 0.060, 0.92)
const SILHOUETTE_RIM: Color = Color(0.82, 0.88, 0.98, 1.0)
const SILHOUETTE_RIM_STRENGTH: float = 0.42

static func apply_finish(
		sprite: AnimatedSprite2D,
		accent: Color,
		outline_strength: float = 0.72,
		accent_strength: float = 0.11
	) -> void:
	if sprite == null:
		return
	var material := ShaderMaterial.new()
	material.shader = FINISH_SHADER
	material.set_shader_parameter("accent_color", accent)
	material.set_shader_parameter("outline_strength", outline_strength)
	material.set_shader_parameter("accent_strength", accent_strength)
	material.set_shader_parameter("outline_color", SILHOUETTE_OUTLINE)
	material.set_shader_parameter("rim_color", SILHOUETTE_RIM)
	material.set_shader_parameter("rim_strength", SILHOUETTE_RIM_STRENGTH)
	sprite.material = material
	sprite.set_meta("field_actor_finish", true)
	sprite.set_meta("field_actor_accent", accent)


static func add_grounding(
		character: Node2D,
		accent: Color = Color(0.38, 0.56, 0.82, 1.0)
	) -> Node2D:
	if character == null:
		return null
	var existing := character.get_node_or_null(NodePath(String(GROUNDING_NAME))) as Node2D
	if existing != null:
		var existing_contact := existing.get_node_or_null("MemoryContact") as Line2D
		if existing_contact != null:
			existing_contact.default_color = Color(accent.r, accent.g, accent.b, existing_contact.default_color.a)
		return existing

	var grounding := Node2D.new()
	grounding.name = GROUNDING_NAME
	grounding.z_index = -2
	grounding.position = Vector2(0, 4)
	grounding.set_meta("field_actor_grounding", true)
	grounding.set_meta("base_position", grounding.position)

	var soft_shadow := _make_ellipse("SoftShadow", Vector2(15.5, 5.2), Color(0.012, 0.016, 0.026, 0.12))
	grounding.add_child(soft_shadow)
	var contact_shadow := _make_ellipse("ContactShadow", Vector2(11.5, 3.4), Color(0.006, 0.008, 0.014, 0.20))
	grounding.add_child(contact_shadow)

	var memory_contact := Line2D.new()
	memory_contact.name = "MemoryContact"
	memory_contact.width = 1.05
	memory_contact.default_color = Color(accent.r, accent.g, accent.b, 0.27)
	memory_contact.points = PackedVector2Array([
		Vector2(-9.5, 1.8),
		Vector2(-5.0, 4.2),
		Vector2(0.0, 4.8),
		Vector2(5.0, 4.2),
		Vector2(9.5, 1.8),
	])
	memory_contact.z_index = 1
	grounding.add_child(memory_contact)

	# Ambient citizens are AnimatedSprite2D roots, so their authored scale would
	# otherwise shrink the world-space shadow a second time.
	if character is AnimatedSprite2D:
		var actor_scale := (character as AnimatedSprite2D).scale
		grounding.scale = Vector2(
			1.0 / maxf(absf(actor_scale.x), 0.001),
			1.0 / maxf(absf(actor_scale.y), 0.001)
		)
		grounding.position = Vector2(
			0.0,
			4.0 / maxf(absf(actor_scale.y), 0.001)
		)
		grounding.set_meta("compensate_parent_scale", true)
	character.add_child(grounding)
	return grounding


static func update_grounding(
		grounding: Node2D,
		velocity: Vector2,
		travel_phase: float,
		moving: bool,
		delta: float,
		speed_reference: float = 120.0,
		scaled_parent: AnimatedSprite2D = null
	) -> void:
	if grounding == null or not is_instance_valid(grounding):
		return
	var speed_ratio := clampf(velocity.length() / maxf(speed_reference, 1.0), 0.0, 1.5)
	var lift := absf(sin(travel_phase)) if moving else 0.0
	var desired_scale := Vector2(
		1.0 + speed_ratio * 0.035 - lift * 0.065,
		1.0 - lift * 0.10
	)
	var base_position: Vector2 = grounding.get_meta("base_position", Vector2(0, 4))
	var target_position := base_position + Vector2(
		-velocity.x / maxf(speed_reference, 1.0) * 0.9,
		lift * 0.18
	)

	if scaled_parent != null:
		var parent_scale := scaled_parent.scale
		desired_scale *= Vector2(
			1.0 / maxf(absf(parent_scale.x), 0.001),
			1.0 / maxf(absf(parent_scale.y), 0.001)
		)
		target_position = Vector2(
			target_position.x / maxf(absf(parent_scale.x), 0.001),
			target_position.y / maxf(absf(parent_scale.y), 0.001)
		)
		grounding.rotation = -scaled_parent.rotation

	var response := 1.0 - exp(-12.0 * delta)
	grounding.scale = grounding.scale.lerp(desired_scale, response)
	grounding.position = grounding.position.lerp(target_position, response)
	var contact := grounding.get_node_or_null("MemoryContact") as Line2D
	if contact != null:
		var base_alpha := 0.34 if moving else 0.24
		contact.default_color.a = base_alpha + sin(travel_phase * 2.0) * (0.035 if moving else 0.015)


static func attach_ambient_driver(sprite: AnimatedSprite2D) -> FieldActorVisuals:
	if sprite == null:
		return null
	var existing := sprite.get_node_or_null(NodePath(String(AMBIENT_DRIVER_NAME))) as FieldActorVisuals
	if existing != null:
		return existing
	var driver := FieldActorVisuals.new()
	driver.name = AMBIENT_DRIVER_NAME
	sprite.add_child(driver)
	driver._ambient_actor = sprite
	driver._ambient_grounding = sprite.get_node_or_null(NodePath(String(GROUNDING_NAME))) as Node2D
	driver._rest_offset = sprite.offset
	driver._rest_scale = sprite.scale
	driver._amplitude_comp = 1.0 / maxf(absf(sprite.scale.y), 0.02)
	driver._last_position = sprite.global_position
	driver._initialized = true
	return driver


static func ambient_accent(preset_name: String) -> Color:
	match preset_name:
		"villager_f":
			return Color(0.54, 0.70, 0.78)
		"fisherman":
			return Color(0.72, 0.60, 0.43)
		"villager_m", "merchant":
			return Color(0.78, 0.58, 0.30)
		"elder":
			return Color(0.66, 0.55, 0.78)
		"child":
			return Color(0.52, 0.68, 0.88)
		"bureau_agent":
			return Color(0.44, 0.72, 0.61)
		"traveler":
			return Color(0.58, 0.52, 0.42)
		_:
			return Color(0.58, 0.60, 0.66)


static func _make_ellipse(node_name: String, radius: Vector2, color: Color) -> Polygon2D:
	var ellipse := Polygon2D.new()
	ellipse.name = node_name
	var points := PackedVector2Array()
	for i in range(24):
		var angle := TAU * float(i) / 24.0
		points.append(Vector2(cos(angle) * radius.x, sin(angle) * radius.y))
	ellipse.polygon = points
	ellipse.color = color
	return ellipse


func _process(delta: float) -> void:
	if not _initialized or _ambient_actor == null or not is_instance_valid(_ambient_actor):
		return
	var current_position := _ambient_actor.global_position
	var travel := current_position - _last_position
	var speed := travel.length() / maxf(delta, 0.0001)
	var moving := speed > 2.0
	var velocity := travel / maxf(delta, 0.0001)
	_last_position = current_position
	_idle_phase += delta

	# S235: 아래 진폭은 "지면 위 픽셀"로 읽어야 한다.
	# AnimatedSprite2D.offset은 로컬 공간이라 sprite.scale이 다시 곱해진다.
	# 모든 필드 배우는 apply_field_profile로 표시 높이 50px에 맞춰지므로 scale은
	# 원본 그림 크기에 따라 제각각이다(실측 0.04 ~ 0.34). 보정 없이 상수를 쓰면
	# 같은 0.18이 어떤 사람에게는 화면상 0.18px, 어떤 사람에게는 0.02px가 된다.
	# 실측했더니 서 있는 주민의 호흡은 0.02~0.18 렌더 픽셀, 즉 보이지 않았다.
	# 이제 진폭을 스케일로 나눠, 누구에게나 같은 픽셀만큼 움직이게 한다.
	if moving:
		_travel_phase += travel.length() * PI / 27.0
		var gait := sin(_travel_phase)
		_ambient_actor.offset = _rest_offset + Vector2(gait * 0.34, -absf(gait) * 0.68) * _amplitude_comp
		_ambient_actor.rotation = lerp_angle(
			_ambient_actor.rotation,
			clampf(velocity.x / 120.0, -1.0, 1.0) * 0.024,
			1.0 - exp(-9.0 * delta)
		)
		var foot_plant := absf(gait)
		var desired_scale := _rest_scale * Vector2(1.0 + foot_plant * 0.008, 1.0 - foot_plant * 0.006)
		_ambient_actor.scale = _ambient_actor.scale.lerp(desired_scale, 1.0 - exp(-10.0 * delta))
	else:
		# 서 있는 사람의 호흡. 몸 높이(50px)의 약 1%가 눈에 띄되 과하지 않은 폭이다.
		var breath := sin(_idle_phase * 1.45)
		_ambient_actor.offset = _ambient_actor.offset.lerp(
			_rest_offset + Vector2(breath * IDLE_SWAY_PX, -absf(breath) * IDLE_BREATH_PX) * _amplitude_comp,
			1.0 - exp(-5.0 * delta)
		)
		_ambient_actor.rotation = lerp_angle(_ambient_actor.rotation, sin(_idle_phase * 0.55) * 0.0035, 1.0 - exp(-5.0 * delta))
		_ambient_actor.scale = _ambient_actor.scale.lerp(
			_rest_scale * Vector2(1.0 + breath * 0.004, 1.0 - breath * 0.003),
			1.0 - exp(-5.0 * delta)
		)

	update_grounding(
		_ambient_grounding,
		velocity,
		_travel_phase,
		moving,
		delta,
		120.0,
		_ambient_actor
	)
