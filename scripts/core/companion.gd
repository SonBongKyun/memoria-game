## Companion, 동행 NPC (엘리아)
## CharacterBody2D 기반. 플레이어를 따라다니며 대화 가능.
extends CharacterBody2D

const IMPORTED_SHEET_SCALE: Vector2 = Vector2(0.40, 0.40)
const IMPORTED_SHEET_OFFSET: Vector2 = Vector2(0, -52)
const FIELD_SPRITE_SCALE: Vector2 = Vector2(0.36, 0.36)
const FIELD_SPRITE_OFFSET: Vector2 = Vector2(0, -72)

const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드
const FOLLOW_SPEED: float = 112.0
const FORMATION_DISTANCE: float = 48.0
const ARRIVAL_RADIUS: float = 7.0
const TRAIL_SAMPLE_DISTANCE: float = 8.0
const MAX_TRAIL_POINTS: int = 96
const WARP_DISTANCE: float = 310.0
const FOLLOW_ACCEL: float = 900.0
const TURN_ACCEL: float = 1250.0
const SPRINT_CATCHUP: float = 1.48

@export var npc_name: String = "Elia"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = "elia_talk"
@export var npc_color: Color = Color(0.45, 0.55, 0.65, 1.0)
@export var repeat_line: String = ""  # 재대화 시 표시할 대사

var _talked_keys: Dictionary = {}  # 이미 진행한 dialogue_key 추적

var sprite: AnimatedSprite2D
var target: Node2D = null  # 따라갈 대상 (Player)

# S150: 이동 자연화 상태
var _smooth_dir: Vector2 = Vector2.DOWN  # 방향 스무딩 (급회전 방지)
var _breath_time: float = 0.0
var _bob_phase: float = 0.0
var _base_offset: Vector2 = Vector2.ZERO
var _base_offset_captured: bool = false
var _rest_scale: Vector2 = Vector2.ONE
var _presence_ring: Line2D
var _presence_shadow: Polygon2D
var _presence_time: float = 0.0
var _trail_points: Array[Vector2] = []
var _anim_suffix: String = "down"
var _actual_speed: float = 0.0
var _stuck_time: float = 0.0

func _ready() -> void:
	motion_mode = CharacterBody2D.MOTION_MODE_FLOATING
	max_slides = 5
	safe_margin = 0.08
	# Sprite2D → AnimatedSprite2D 교체 (픽셀 스���라이트 지원)
	if has_node("Sprite2D"):
		$Sprite2D.queue_free()
	sprite = AnimatedSprite2D.new()
	sprite.name = "AnimatedSprite2D"
	add_child(sprite)
	_setup_placeholder_sprite()
	_add_companion_presence()
	# 씬 트리에서 Player 찾기
	await get_tree().process_frame
	target = get_tree().get_first_node_in_group("player")
	if not target:
		var players = get_tree().get_nodes_in_group("player")
		if players.is_empty():
			# fallback: 부모에서 Player 찾기
			var parent = get_parent()
			if parent and parent.has_node("Player"):
				target = parent.get_node("Player")
	_reset_target_trail()
	print("[Companion] %s ready, following %s" % [npc_name, target.name if target else "nobody"])

func _physics_process(delta: float) -> void:
	if not target or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		_actual_speed = 0.0
		return
	if sprite and not _base_offset_captured:
		_base_offset = sprite.offset
		_base_offset_captured = true

	_record_target_trail()
	var target_distance := global_position.distance_to(target.global_position)
	var follow_point := _get_trail_follow_point()
	var to_follow_point := follow_point - global_position
	var follow_distance := to_follow_point.length()

	if target_distance > WARP_DISTANCE or _stuck_time > 1.35:
		_warp_to_trail(follow_point)
		return

	var raw_dir := to_follow_point.normalized() if follow_distance > 0.001 else _smooth_dir
	var target_speed := 0.0
	if follow_distance > ARRIVAL_RADIUS:
		var response := clampf((follow_distance - ARRIVAL_RADIUS) / 72.0, 0.0, 1.0)
		var catchup := lerpf(0.72, SPRINT_CATCHUP, clampf((target_distance - FORMATION_DISTANCE) / 150.0, 0.0, 1.0))
		if target is CharacterBody2D:
			var target_speed_floor := (target as CharacterBody2D).velocity.length() * 0.92
			target_speed = maxf(FOLLOW_SPEED * lerpf(0.42, catchup, response), target_speed_floor * response)
		else:
			target_speed = FOLLOW_SPEED * lerpf(0.42, catchup, response)

	var direction_response := 1.0 - exp(-9.0 * delta)
	_smooth_dir = _smooth_dir.lerp(raw_dir, direction_response)
	if _smooth_dir.length_squared() > 0.001:
		_smooth_dir = _smooth_dir.normalized()
	var desired_velocity := _smooth_dir * target_speed
	var reversing := velocity.length_squared() > 16.0 and velocity.dot(desired_velocity) < 0.0
	velocity = velocity.move_toward(desired_velocity, (TURN_ACCEL if reversing else FOLLOW_ACCEL) * delta)

	var position_before_move := global_position
	move_and_slide()
	var moved_distance := global_position.distance_to(position_before_move)
	_actual_speed = moved_distance / maxf(delta, 0.0001)
	if target_speed > 24.0 and follow_distance > 32.0 and moved_distance < 0.12:
		_stuck_time += delta
	else:
		_stuck_time = maxf(0.0, _stuck_time - delta * 2.0)

	var visually_moving := _actual_speed > 6.0
	_update_animation(_smooth_dir if visually_moving else raw_dir, visually_moving)

	# S150: 걷기 바운스 + 정지 호흡 (플레이어와 같은 문법)
	if sprite:
		if visually_moving:
			_bob_phase += moved_distance * PI / 28.0
			var gait := sin(_bob_phase)
			sprite.offset.y = _base_offset.y - absf(gait) * 0.80
			sprite.offset.x = _base_offset.x + gait * (0.42 if absf(_smooth_dir.y) > 0.5 else 0.20)
			sprite.rotation = lerp_angle(sprite.rotation, (velocity.x / (FOLLOW_SPEED * SPRINT_CATCHUP)) * 0.032, 9.0 * delta)
			_breath_time = 0.0
			sprite.scale = sprite.scale.lerp(_rest_scale, 10.0 * delta)
		else:
			sprite.offset.x = lerpf(sprite.offset.x, _base_offset.x, 12.0 * delta)
			sprite.offset.y = lerpf(sprite.offset.y, _base_offset.y, 12.0 * delta)
			sprite.rotation = lerp_angle(sprite.rotation, 0.0, 12.0 * delta)
			_breath_time += delta
			sprite.scale = _rest_scale * Vector2(1.0 + sin(_breath_time * 1.8) * 0.008, 1.0 - sin(_breath_time * 1.8) * 0.006)
	_update_companion_presence(delta, visually_moving)

func _reset_target_trail() -> void:
	_trail_points.clear()
	if target == null:
		return
	_trail_points.append(global_position)
	_trail_points.append(target.global_position)

func _record_target_trail() -> void:
	if target == null:
		return
	if _trail_points.is_empty():
		_reset_target_trail()
		return
	var target_position := target.global_position
	if _trail_points.back().distance_to(target_position) < TRAIL_SAMPLE_DISTANCE:
		return
	_trail_points.append(target_position)
	while _trail_points.size() > MAX_TRAIL_POINTS:
		_trail_points.pop_front()

func _get_trail_follow_point() -> Vector2:
	if target == null:
		return global_position
	if _trail_points.size() < 2:
		return target.global_position
	var accumulated := 0.0
	for i in range(_trail_points.size() - 1, 0, -1):
		var newer := _trail_points[i]
		var older := _trail_points[i - 1]
		var segment := newer.distance_to(older)
		if segment <= 0.001:
			continue
		if accumulated + segment >= FORMATION_DISTANCE:
			var along_segment := (FORMATION_DISTANCE - accumulated) / segment
			return newer.lerp(older, along_segment)
		accumulated += segment
	return _trail_points.front()

func _warp_to_trail(follow_point: Vector2) -> void:
	global_position = follow_point
	velocity = Vector2.ZERO
	_actual_speed = 0.0
	_stuck_time = 0.0
	_reset_target_trail()
	_update_animation(_smooth_dir, false)
	if sprite:
		sprite.modulate.a = 0.0
		var tween := create_tween()
		tween.tween_property(sprite, "modulate:a", 1.0, 0.28).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)

## 상호작용 (Player의 RayCast가 호출)
func interact() -> void:
	if DialogueManager.is_active:
		return

	var talk_flag = "talked_%s_%s" % [npc_name, dialogue_key]
	if _talked_keys.has(dialogue_key) or GameManager.get_flag(talk_flag):
		# 이미 대화한 동행, 짧은 후속 대사
		var line = repeat_line if repeat_line != "" else "..."
		DialogueManager.start_dialogue([
			{"speaker": npc_name, "text": line, "portrait": ""}
		])
		return

	_talked_keys[dialogue_key] = true
	DialogueManager.dialogue_ended.connect(func(): GameManager.set_flag(talk_flag), CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(dialogue_file, dialogue_key)

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprite() -> void:
	var config = PixelSprite.elia_config()
	var field_key := "sable" if npc_name == "Sable" else "elia"
	if PixelSprite.has_field_sprite_frames(field_key):
		sprite.sprite_frames = PixelSprite.create_sheet_frames(field_key)
		var field_texture := sprite.sprite_frames.get_frame_texture("idle_down", 0)
		_rest_scale = PixelSprite.apply_field_profile(sprite, field_texture)
	# npc_name에 따라 다른 config 사용
	elif npc_name == "Sable":
		config = PixelSprite.sable_config()
		sprite.sprite_frames = PixelSprite.create_frames(config)
		_rest_scale = Vector2.ONE
		sprite.scale = _rest_scale
		sprite.offset = Vector2.ZERO
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	else:
		var sheet_path := "res://assets/sprites/characters/elia_sheet/idle_01.png"
		if ResourceLoader.exists(sheet_path):
			sprite.sprite_frames = PixelSprite.create_sheet_frames("elia")
			_rest_scale = IMPORTED_SHEET_SCALE
			sprite.offset = IMPORTED_SHEET_OFFSET
			sprite.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
		else:
			sprite.sprite_frames = PixelSprite.create_frames(PixelSprite.elia_config())
			_rest_scale = Vector2.ONE
			sprite.offset = Vector2.ZERO
			sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		sprite.scale = _rest_scale
	sprite.play("idle_down")

func _add_companion_presence() -> void:
	_presence_shadow = Polygon2D.new()
	_presence_shadow.polygon = PackedVector2Array([
		Vector2(-14, 2), Vector2(-8, -1), Vector2(8, -1), Vector2(14, 2),
		Vector2(8, 5), Vector2(-8, 5)
	])
	_presence_shadow.color = Color(0.0, 0.0, 0.0, 0.28)
	_presence_shadow.z_index = -1
	add_child(_presence_shadow)

	var accent := Color(0.86, 0.68, 0.34, 0.34) if npc_name == "Elia" else Color(0.67, 0.52, 0.84, 0.34)
	_presence_ring = Line2D.new()
	_presence_ring.width = 1.1
	_presence_ring.default_color = accent
	_presence_ring.points = PackedVector2Array([
		Vector2(-10, 2), Vector2(-5, 5), Vector2(5, 5), Vector2(10, 2)
	])
	_presence_ring.z_index = 0
	add_child(_presence_ring)

func _update_companion_presence(delta: float, moving: bool) -> void:
	if _presence_ring == null or not is_instance_valid(_presence_ring):
		return
	_presence_time += delta
	var base_alpha := 0.48 if moving else 0.32
	var pulse := sin(_presence_time * (3.2 if moving else 1.7)) * 0.06
	_presence_ring.default_color.a = clampf(base_alpha + pulse, 0.20, 0.56)
	_presence_ring.scale.x = 1.0 + (0.08 if moving else 0.03) * sin(_presence_time * 2.0)

## 애니메이션 방향 업데이트
func _update_animation(direction: Vector2, is_moving: bool) -> void:
	if not sprite or not sprite.sprite_frames:
		return
	var prefix = "walk_" if is_moving else "idle_"
	var ax := absf(direction.x)
	var ay := absf(direction.y)
	if ax > ay * 1.18:
		_anim_suffix = "right" if direction.x > 0.0 else "left"
	elif ay > ax * 1.18:
		_anim_suffix = "down" if direction.y > 0.0 else "up"
	elif _anim_suffix == "left" and direction.x > 0.05:
		_anim_suffix = "right"
	elif _anim_suffix == "right" and direction.x < -0.05:
		_anim_suffix = "left"
	elif _anim_suffix == "up" and direction.y > 0.05:
		_anim_suffix = "down"
	elif _anim_suffix == "down" and direction.y < -0.05:
		_anim_suffix = "up"
	var anim = prefix + _anim_suffix
	if sprite.animation != anim:
		sprite.play(anim)
	if is_moving:
		sprite.speed_scale = clampf(_actual_speed / FOLLOW_SPEED, 0.65, 1.55)
	else:
		sprite.speed_scale = 1.0
