## Player, 아렐
## 탑다운 2D 이동, 상호작용, 기본 상태 관리.
## S57: Camera2D 시스템 + 탐색 폴리시 (대시, 가속, 먼지, 인터랙션 인디케이터, 피젯)
extends CharacterBody2D

const BASE_SPEED: float = 120.0
const SPRINT_MULTIPLIER: float = 1.62
const SHEET_SPRITE_SCALE: Vector2 = Vector2(0.40, 0.40)
const SHEET_SPRITE_OFFSET: Vector2 = Vector2(0, -52)
const FIELD_SPRITE_SCALE: Vector2 = Vector2(0.36, 0.36)
const FIELD_SPRITE_OFFSET: Vector2 = Vector2(0, -72)
const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드
const ACCELERATION: float = 1100.0
const TURN_ACCELERATION: float = 1550.0
const DECELERATION: float = 1350.0
const SPRINT_BLEND_SPEED: float = 7.5
const MOVE_VISUAL_THRESHOLD: float = 7.0
const WALK_STEP_DISTANCE: float = 26.0
const SPRINT_STEP_DISTANCE: float = 32.0
const MEMORY_PULSE_RADIUS: float = 150.0
const MEMORY_PULSE_COOLDOWN: float = 6.0

@onready var sprite: AnimatedSprite2D = $AnimatedSprite2D
@onready var interaction_ray: RayCast2D = $InteractionRay
@onready var camera: Camera2D = $Camera2D

var facing_direction: Vector2 = Vector2.DOWN
var can_move: bool = true
var _step_distance: float = 0.0
var _breath_time: float = 0.0  # S52: 호흡 애니메이션

# --- S57: Camera ---
var _camera_base_zoom: Vector2 = Vector2(2.25, 2.25)
var _camera_look_ahead: float = 28.0
var _camera_look_offset: Vector2 = Vector2.ZERO
var _camera_event_shake_offset: Vector2 = Vector2.ZERO
var _shake_phase: float = 0.0
var _shake_intensity: float = 0.0
var _shake_duration: float = 0.0
var _shake_timer: float = 0.0

# --- S57: Sprint & Afterimage ---
var _is_sprinting: bool = false
var _sprint_blend: float = 0.0
var _afterimage_timer: float = 0.0
const AFTERIMAGE_INTERVAL: float = 0.14

# --- S57: Footstep particles ---
var _dust_timer: float = 0.0
const DUST_INTERVAL_MIN: float = 0.13  # ~8 프레임 @60fps
const DUST_INTERVAL_MAX: float = 0.2   # ~12 프레임 @60fps
var _next_dust_time: float = 0.15

# --- S57: Idle fidget ---
var _idle_time: float = 0.0
const FIDGET_START: float = 5.0  # 5초 대기 후 피젯 시작
var _fidget_timer: float = 0.0

# --- S58: Movement squash/stretch ---
var _was_moving: bool = false  # 이전 프레임 이동 상태 (시작/정지 감지용)
var _move_squash_tween: Tween  # 현재 스쿼시/스트레치 트윈 (중복 방지)

# --- S57: Interaction indicator ---
var _interact_indicator: Label = null
var _indicator_bob_time: float = 0.0

# --- S92: Memory Pulse ---
var _memory_pulse_cooldown: float = 0.0

# --- S150: 걷기 바운스 / 발딛기 동기 / 기울기 / 방향 히스테리시스 ---
var _bob_phase: float = 0.0          # 걸음 위상 (속도 비례 진행)
var _sprite_base_offset: Vector2 = Vector2.ZERO
var _sprite_rest_scale: Vector2 = SHEET_SPRITE_SCALE
var _anim_suffix: String = "down"    # 대각선 지터 방지용 최근 방향
var _actual_speed: float = 0.0
var _last_move_direction: Vector2 = Vector2.DOWN
var _ground_shadow: Node2D = null
var _turn_lean: float = 0.0
var _step_impact: float = 0.0
var _foot_side: float = -1.0
var _field_flow: FieldFlow = null

# --- 기억 잔광: 플레이어가 지닌 은은한 캐리드 라이트 ---
var _memory_light: PointLight2D = null
var _light_time: float = randf() * TAU

func _ready() -> void:
	add_to_group("player")
	motion_mode = CharacterBody2D.MOTION_MODE_FLOATING
	max_slides = 5
	safe_margin = 0.08
	_field_flow = FieldFlow.new()
	_field_flow.name = "FieldFlow"
	add_child(_field_flow)
	_setup_placeholder_sprites()
	if sprite and sprite.sprite_frames:
		sprite.play("idle_down")
		_sprite_base_offset = sprite.offset
		FieldActorVisuals.apply_finish(sprite, Color(0.34, 0.60, 0.92), 0.78, 0.13)
	_ground_shadow = MapEffects.add_drop_shadow(self, Color(0.34, 0.60, 0.92))
	_memory_light = MapEffects.add_carried_light(self)
	_setup_camera()
	_setup_interact_indicator()
	print("[Player] Arrel ready, Camera2D + exploration polish active")

## Camera2D 초기 설정
func _setup_camera() -> void:
	if not camera:
		return
	camera.enabled = true
	# Clear Gameplay View trades a little magnification for more map context,
	# keeping the 32px field from reading as a wall of oversized cells.
	_camera_base_zoom = Vector2(2.0, 2.0) if OptionsMenu.is_clean_gameplay_visuals() else Vector2(2.25, 2.25)
	camera.zoom = _camera_base_zoom
	camera.position_smoothing_enabled = true
	camera.position_smoothing_speed = 8.0
	camera.drag_horizontal_enabled = false
	camera.drag_vertical_enabled = false
	camera.set_meta("pixel_snap", false)
	camera.set_meta("ambient_camera_offset", Vector2.ZERO)
	# 맵 한계, 씬에서 MAP_WIDTH/MAP_HEIGHT 읽기
	_apply_camera_limits()

## 현재 맵의 크기를 읽어서 카메라 리밋 적용
func _apply_camera_limits() -> void:
	if not camera:
		return
	var scene = get_tree().current_scene if get_tree() else null
	var map_w: int = 25
	var map_h: int = 18
	var tile_size: int = 32
	if scene:
		if "MAP_WIDTH" in scene:
			map_w = scene.MAP_WIDTH
		if "MAP_HEIGHT" in scene:
			map_h = scene.MAP_HEIGHT
		if "TILE_SIZE" in scene:
			tile_size = scene.TILE_SIZE
	camera.limit_left = 0
	camera.limit_top = 0
	camera.limit_right = map_w * tile_size
	camera.limit_bottom = map_h * tile_size

## 인터랙션 인디케이터 "E" 아이콘 생성
func _setup_interact_indicator() -> void:
	_interact_indicator = Label.new()
	_interact_indicator.text = "E  대화"
	_interact_indicator.custom_minimum_size = Vector2(72, 24)
	_interact_indicator.add_theme_font_override("font", UITheme.make_ui_font())
	_interact_indicator.add_theme_font_size_override("font_size", 13)
	_interact_indicator.add_theme_color_override("font_color", Color(1.0, 0.91, 0.64, 0.98))
	_interact_indicator.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.8))
	_interact_indicator.add_theme_constant_override("shadow_offset_x", 1)
	_interact_indicator.add_theme_constant_override("shadow_offset_y", 1)
	_interact_indicator.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_interact_indicator.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	var indicator_style := StyleBoxFlat.new()
	indicator_style.bg_color = Color(0.035, 0.030, 0.050, 0.92)
	indicator_style.border_color = Color(0.82, 0.62, 0.30, 0.82)
	indicator_style.set_border_width_all(1)
	indicator_style.set_corner_radius_all(6)
	_interact_indicator.add_theme_stylebox_override("normal", indicator_style)
	_interact_indicator.position = Vector2(-22, -42)
	_interact_indicator.scale = Vector2(0.62, 0.62)
	_interact_indicator.z_index = 10
	_interact_indicator.visible = false
	add_child(_interact_indicator)

func _physics_process(delta: float) -> void:
	if _memory_pulse_cooldown > 0.0:
		_memory_pulse_cooldown = maxf(0.0, _memory_pulse_cooldown - delta)

	if not can_move or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		_actual_speed = 0.0
		if _field_flow:
			_field_flow.update_motion(delta, 0.0, false, false, 0.0)
		_sprint_blend = move_toward(_sprint_blend, 0.0, SPRINT_BLEND_SPEED * delta)
		_idle_time = 0.0
		_turn_lean = move_toward(_turn_lean, 0.0, delta * 0.9)
		_step_impact = move_toward(_step_impact, 0.0, delta * 9.0)
		_update_animation(_last_move_direction, false)
		if sprite:
			sprite.offset = sprite.offset.lerp(_sprite_base_offset, 1.0 - exp(-12.0 * delta))
			sprite.rotation = lerp_angle(sprite.rotation, 0.0, 1.0 - exp(-12.0 * delta))
			sprite.scale = sprite.scale.lerp(_sprite_rest_scale, 1.0 - exp(-12.0 * delta))
		_update_ground_shadow(delta, false)
		_was_moving = false
		return

	# Input.get_vector preserves controller pressure while normalizing keyboard diagonals.
	var input_vector := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	var has_intent := input_vector.length_squared() > 0.0004
	if has_intent:
		_last_move_direction = input_vector.normalized()

	if Input.is_action_just_pressed("field_dash") and _field_flow:
		var step_direction := input_vector if has_intent else _last_move_direction
		if _field_flow.try_phase_step(step_direction):
			_last_move_direction = _field_flow.dash_direction
			_play_phase_step_vfx()

	var dash_active := _field_flow != null and _field_flow.is_dashing()
	if dash_active:
		input_vector = _field_flow.dash_direction
		has_intent = true

	var wants_sprint := Input.is_action_pressed("sprint") and has_intent
	_sprint_blend = move_toward(_sprint_blend, 1.0 if wants_sprint or dash_active else 0.0, SPRINT_BLEND_SPEED * delta)
	var sprint_curve := _sprint_blend * _sprint_blend * (3.0 - 2.0 * _sprint_blend)
	var speed := BASE_SPEED * lerpf(1.0, SPRINT_MULTIPLIER, sprint_curve)
	if dash_active:
		speed *= _field_flow.get_speed_multiplier()
	var target_velocity := input_vector * speed
	if has_intent:
		var reversing := velocity.length_squared() > 16.0 and velocity.dot(target_velocity) < 0.0
		var acceleration := 4200.0 if dash_active else (TURN_ACCELERATION if reversing else ACCELERATION)
		velocity = velocity.move_toward(target_velocity, acceleration * delta)
	else:
		velocity = velocity.move_toward(Vector2.ZERO, DECELERATION * delta)

	var position_before_move := global_position
	move_and_slide()
	var moved_distance := global_position.distance_to(position_before_move)
	_actual_speed = moved_distance / maxf(delta, 0.0001)
	_is_sprinting = dash_active or (_sprint_blend > 0.45 and _actual_speed > BASE_SPEED * 1.12)

	# Direction comes from intent, but locomotion feedback comes from real travel.
	var visually_moving := _actual_speed > MOVE_VISUAL_THRESHOLD
	if has_intent:
		_update_animation(input_vector, visually_moving)
		facing_direction = _cardinal_direction(_anim_suffix)
		_update_raycast_direction()
		if visually_moving and not _was_moving and sprite:
			_play_move_squash(Vector2(0.95, 1.05), 0.05)
		_idle_time = 0.0
		_fidget_timer = 0.0
	else:
		_update_animation(_last_move_direction, visually_moving)
		if _was_moving and sprite:
			_play_move_squash(Vector2(1.05, 0.95), 0.08)
		_idle_time += delta
		_breath_time += delta
		if sprite:
			if _idle_time >= FIDGET_START:
				# 피젯: 더 눈에 띄는 랜덤 미세 떨림
				_fidget_timer += delta
				if _fidget_timer > randf_range(2.0, 4.0):
					_fidget_timer = 0.0
					_do_fidget()
			# Only apply breathing if no active squash tween
			if not _move_squash_tween or not _move_squash_tween.is_running():
				sprite.scale = _scaled_sprite(Vector2(1.0 + sin(_breath_time * 2.0) * 0.01, 1.0 - sin(_breath_time * 2.0) * 0.008))
	_was_moving = visually_moving
	if _field_flow:
		_field_flow.update_motion(delta, moved_distance, visually_moving, wants_sprint, _actual_speed)

	var clean_view := OptionsMenu.is_clean_gameplay_visuals()
	_update_footfalls(moved_distance, clean_view)

	# S57: Camera look-ahead
	_update_camera_look_ahead(delta)

	# S57: Camera shake
	_update_camera_shake(delta)

	# A restrained sprint echo reads as speed without becoming a constant trail.
	if _is_sprinting and not clean_view:
		_afterimage_timer += delta
		var echo_interval := 0.052 if dash_active else AFTERIMAGE_INTERVAL
		if _afterimage_timer >= echo_interval:
			_afterimage_timer = 0.0
			_spawn_afterimage()
	else:
		_afterimage_timer = 0.0

	_turn_lean = move_toward(_turn_lean, 0.0, delta * 0.58)
	_step_impact = move_toward(_step_impact, 0.0, delta * 7.5)
	if sprite:
		if visually_moving:
			var stride_distance := SPRINT_STEP_DISTANCE if _is_sprinting else WALK_STEP_DISTANCE
			_bob_phase += moved_distance * PI / stride_distance
			var gait := sin(_bob_phase)
			sprite.offset.y = _sprite_base_offset.y - absf(gait) * (1.15 if _is_sprinting else 0.85)
			sprite.offset.x = _sprite_base_offset.x + gait * (0.50 if absf(_last_move_direction.y) > 0.5 else 0.24)
			var lean := (velocity.x / (BASE_SPEED * SPRINT_MULTIPLIER)) * 0.036 + _turn_lean
			sprite.rotation = lerp_angle(sprite.rotation, lean, 10.0 * delta)
			if not _move_squash_tween or not _move_squash_tween.is_running():
				var movement_shape := Vector2.ONE
				if dash_active:
					movement_shape = Vector2(1.16, 0.84) if absf(input_vector.x) > absf(input_vector.y) else Vector2(0.84, 1.16)
				elif _is_sprinting:
					movement_shape = Vector2(1.045, 0.965) if absf(input_vector.x) > absf(input_vector.y) else Vector2(0.965, 1.045)
				movement_shape *= Vector2(1.0 + _step_impact * 0.018, 1.0 - _step_impact * 0.024)
				sprite.scale = sprite.scale.lerp(_scaled_sprite(movement_shape), 1.0 - exp(-10.0 * delta))
		else:
			sprite.offset.x = lerpf(sprite.offset.x, _sprite_base_offset.x, 12.0 * delta)
			sprite.offset.y = lerpf(sprite.offset.y, _sprite_base_offset.y, 12.0 * delta)
			sprite.rotation = lerp_angle(sprite.rotation, 0.0, 12.0 * delta)
	_update_ground_shadow(delta, visually_moving)
	_update_memory_light(delta)

	# S57: Interaction indicator
	_update_interact_indicator(delta)

## 기억 잔광 호흡. 메모리 펄스 직후에는 잠깐 밝아졌다가 가라앉는다.
func _update_memory_light(delta: float) -> void:
	if _memory_light == null or not is_instance_valid(_memory_light):
		return
	_light_time += delta
	var base: float = _memory_light.get_meta("base_energy", 0.5)
	if OptionsMenu.is_clean_gameplay_visuals():
		base *= 0.62
	var breath := sin(_light_time * 1.7) * 0.05 + sin(_light_time * 4.3) * 0.02
	_memory_light.energy = lerpf(_memory_light.energy, base + breath, 1.0 - exp(-6.0 * delta))

## 메모리 펄스가 메아리를 찾았을 때, 방향으로 떠나가는 따뜻한 기억 입자.
## 토스트 텍스트보다 먼저 "어느 쪽인지"를 세계 안에서 읽게 한다.
func _spawn_pulse_motes(direction: Vector2) -> void:
	var parent := get_parent()
	if parent == null:
		return
	var clean_view := OptionsMenu.is_clean_gameplay_visuals()
	var count := 7 if clean_view else 12
	var base_angle := direction.angle() if direction != Vector2.ZERO else 0.0
	for i in range(count):
		var mote := Line2D.new()
		mote.width = 1.6
		mote.default_color = Color(0.94, 0.80, 0.46, 0.85)
		var spread := randf_range(-0.42, 0.42)
		mote.points = PackedVector2Array([
			Vector2.ZERO,
			Vector2(-4.0, -1.2),
			Vector2(-8.5, 0.0),
		])
		var travel := randf_range(46.0, 92.0)
		var target := Vector2.from_angle(base_angle + spread) * travel
		mote.global_position = global_position + Vector2(randf_range(-6, 6), randf_range(-14, -4))
		mote.rotation = base_angle + spread
		mote.z_index = z_index + 6
		parent.add_child(mote)
		var tw := mote.create_tween()
		tw.set_parallel(true)
		tw.tween_property(mote, "global_position", mote.global_position + target, randf_range(0.55, 0.95)).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
		tw.tween_property(mote, "modulate:a", 0.0, randf_range(0.5, 0.9)).set_trans(Tween.TRANS_SINE)
		tw.tween_property(mote, "scale", Vector2(0.5, 0.5), 0.8)
		tw.set_parallel(false)
		tw.tween_callback(mote.queue_free)

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("memory_pulse") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_memory_pulse()
		get_viewport().set_input_as_handled()
		return

	# 상호작용 (Space / Enter), 탐색 모드에서만
	if event.is_action_pressed("interact") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_interact()

# ─── Camera ────────────────────────────────────────

## 카메라 룩어헤드: 이동 방향으로 오프셋
func _update_camera_look_ahead(delta: float) -> void:
	if not camera:
		return
	var target_offset := Vector2.ZERO
	if velocity.length() > 10.0:
		var distance := 16.0 if OptionsMenu.is_clean_gameplay_visuals() else _camera_look_ahead
		var speed_ratio := clampf(velocity.length() / (BASE_SPEED * SPRINT_MULTIPLIER), 0.0, 1.0)
		var anticipation := speed_ratio * speed_ratio * (3.0 - 2.0 * speed_ratio)
		target_offset = velocity.normalized() * distance * anticipation
	var response := 1.0 - exp(-6.5 * delta)
	_camera_look_offset = _camera_look_offset.lerp(target_offset, response)
	var dash_active := _field_flow != null and _field_flow.is_dashing()
	var zoom_factor := 0.90 if dash_active else (0.965 if _is_sprinting else 1.0)
	var zoom_response := 1.0 - exp(-(13.0 if dash_active else 5.5) * delta)
	camera.zoom = camera.zoom.lerp(_camera_base_zoom * zoom_factor, zoom_response)
	_apply_camera_offset()

## 카메라 셰이크 업데이트
func _update_camera_shake(delta: float) -> void:
	if not camera:
		return
	if OptionsMenu.is_clean_gameplay_visuals():
		_shake_intensity = 0.0
		_shake_timer = 0.0
		_camera_event_shake_offset = Vector2.ZERO
		_apply_camera_offset()
		return
	if _shake_timer > 0.0:
		_shake_timer -= delta
		_shake_phase += delta * 34.0
		var shake_amount := _shake_intensity * (_shake_timer / maxf(_shake_duration, 0.001))
		_camera_event_shake_offset = Vector2(
			sin(_shake_phase * 1.13) * shake_amount,
			cos(_shake_phase * 0.91) * shake_amount * 0.72
		)
	elif _shake_intensity > 0.0:
		_shake_intensity = 0.0
		_camera_event_shake_offset = Vector2.ZERO
	else:
		_camera_event_shake_offset = _camera_event_shake_offset.lerp(Vector2.ZERO, 1.0 - exp(-14.0 * delta))
	_apply_camera_offset()

func _apply_camera_offset() -> void:
	if not camera:
		return
	var ambient: Vector2 = camera.get_meta("ambient_camera_offset", Vector2.ZERO)
	camera.offset = _camera_look_offset + ambient + _camera_event_shake_offset

## 외부에서 호출 가능한 셰이크 메서드
func shake(intensity: float = 4.0, duration: float = 0.3) -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		return
	_shake_intensity = intensity
	_shake_duration = duration
	_shake_timer = duration

## 전투 진입 시 줌인 트윈 (외부에서 호출)
func battle_zoom_in(duration: float = 0.2) -> void:
	if not camera:
		return
	var t = create_tween()
	t.tween_property(camera, "zoom", _camera_base_zoom * 1.15, duration).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	await t.finished

## 전투/씬 전환 후 줌 리셋
func reset_camera_zoom() -> void:
	if not camera:
		return
	camera.zoom = _camera_base_zoom

## 씬 전환 시 카메라 리밋 재적용 (맵이 바뀔 때)
func refresh_camera_limits() -> void:
	_apply_camera_limits()

# ─── Exploration Polish ────────────────────────────

## 스프린트 잔상 (Afterimage)
func _spawn_afterimage() -> void:
	if not sprite or not sprite.sprite_frames:
		return
	var ghost = AnimatedSprite2D.new()
	ghost.sprite_frames = sprite.sprite_frames
	ghost.animation = sprite.animation
	ghost.frame = sprite.frame
	ghost.global_position = global_position
	ghost.scale = sprite.scale
	ghost.offset = sprite.offset
	ghost.rotation = sprite.rotation
	ghost.texture_filter = sprite.texture_filter
	var dash_active := _field_flow != null and _field_flow.is_dashing()
	ghost.modulate = Color(0.52, 0.80, 1.0, 0.42) if dash_active else Color(0.42, 0.50, 0.72, 0.24)
	ghost.z_index = z_index - 1
	get_parent().add_child(ghost)
	var t = ghost.create_tween()
	t.set_parallel(true)
	t.tween_property(ghost, "modulate:a", 0.0, 0.18)
	if dash_active:
		t.tween_property(ghost, "scale", ghost.scale * Vector2(0.92, 1.06), 0.18)
	t.chain().tween_callback(ghost.queue_free)

func _play_phase_step_vfx() -> void:
	shake(1.25, 0.14)
	_spawn_pulse_ring(30.0, Color(0.48, 0.82, 1.0, 0.62), 0.22)
	_spawn_afterimage()
	var parent := get_parent()
	if parent:
		for side in [-1.0, 1.0]:
			var rail := Line2D.new()
			rail.name = "PhaseStepRail"
			rail.width = 1.4
			rail.default_color = Color(0.58, 0.86, 1.0, 0.72)
			rail.z_index = z_index - 1
			var direction := _field_flow.dash_direction if _field_flow else _last_move_direction
			var normal := Vector2(-direction.y, direction.x)
			rail.points = PackedVector2Array([
				normal * side * 7.0,
				-direction * 22.0 + normal * side * 10.0,
				-direction * 48.0 + normal * side * 5.0,
			])
			rail.global_position = global_position
			parent.add_child(rail)
			var rail_tween := rail.create_tween()
			rail_tween.set_parallel(true)
			rail_tween.tween_property(rail, "modulate:a", 0.0, 0.24)
			rail_tween.tween_property(rail, "width", 0.2, 0.24)
			rail_tween.chain().tween_callback(rail.queue_free)
	if sprite:
		var sprite_tween := create_tween()
		sprite_tween.tween_property(sprite, "modulate", Color(1.22, 1.38, 1.55, 1.0), 0.055)
		sprite_tween.tween_property(sprite, "modulate", Color.WHITE, 0.18)

func _update_footfalls(moved_distance: float, clean_view: bool) -> void:
	if moved_distance <= 0.01:
		return
	_step_distance += moved_distance
	var stride_distance := SPRINT_STEP_DISTANCE if _is_sprinting else WALK_STEP_DISTANCE
	while _step_distance >= stride_distance:
		_step_distance -= stride_distance
		_step_impact = 1.0
		_foot_side *= -1.0
		AudioManager.play_step(_get_terrain_type())
		GameManager.add_stat("steps_taken")
		if clean_view:
			_spawn_step_echo()
		else:
			_spawn_dust()

func _update_ground_shadow(delta: float, moving: bool) -> void:
	if _ground_shadow == null or not is_instance_valid(_ground_shadow):
		_ground_shadow = get_node_or_null("FieldGrounding") as Node2D
	if _ground_shadow == null:
		return
	FieldActorVisuals.update_grounding(
		_ground_shadow,
		velocity,
		_bob_phase,
		moving,
		delta,
		BASE_SPEED * SPRINT_MULTIPLIER
	)

## 발밑 먼지 파티클, S59: terrain-specific dust colors
func _spawn_dust() -> void:
	var dust := Node2D.new()
	# S59: Terrain-specific dust color
	var terrain = _get_terrain_type()
	var dust_color: Color
	match terrain:
		"grass":
			dust_color = Color(0.29, 0.42, 0.23, 0.6)  # green-brown
		"stone":
			dust_color = Color(0.54, 0.54, 0.54, 0.6)  # gray
		"sand":
			dust_color = Color(0.77, 0.66, 0.29, 0.6)  # yellow-tan
		_:
			# Check for void terrain via scene name
			var scene = get_tree().current_scene
			if scene and ("void" in scene.name.to_lower() or "bl07" in scene.name.to_lower() or "seam" in scene.name.to_lower()):
				dust_color = Color(0.42, 0.23, 0.54, 0.6)  # purple void dust
			else:
				dust_color = Color(0.65, 0.55, 0.4, 0.6)  # default earth
	for i in range(2):
		var wisp := Line2D.new()
		wisp.width = 1.1
		wisp.default_color = dust_color
		var spread := 3.0 + float(i) * 2.0
		wisp.points = PackedVector2Array([
			Vector2(_foot_side * (1.5 + float(i)), 0),
			Vector2(_foot_side * spread, -randf_range(1.0, 2.5)),
			Vector2(_foot_side * (spread + 2.0), -randf_range(0.5, 1.5)),
		])
		dust.add_child(wisp)
	# A paired, direction-aware wisp reads as a footfall instead of a UI square.
	var lateral := Vector2(-facing_direction.y, facing_direction.x) * _foot_side * 3.0
	dust.global_position = global_position + lateral + Vector2(0, 7)
	dust.z_index = z_index - 1
	get_parent().add_child(dust)
	var t := dust.create_tween()
	t.set_parallel(true)
	t.tween_property(dust, "modulate:a", 0.0, 0.3)
	t.tween_property(dust, "global_position:y", dust.global_position.y - randf_range(3, 7), 0.3)
	t.tween_property(dust, "scale", Vector2(1.24, 1.24), 0.3).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	t.set_parallel(false)
	t.tween_callback(dust.queue_free)

## Clean-view footfall: a tiny Memory-colored ground echo keeps walking lively
## without returning to opaque particles or long afterimage trails.
func _spawn_step_echo() -> void:
	var echo := Line2D.new()
	echo.width = 1.15
	echo.default_color = Color(0.74, 0.68, 0.48, 0.32)
	echo.points = PackedVector2Array([
		Vector2(-6, 0), Vector2(0, -2.5), Vector2(6, 0),
		Vector2(0, 2.5), Vector2(-6, 0),
	])
	echo.global_position = global_position + Vector2(0, 7)
	echo.z_index = z_index - 2
	echo.scale = Vector2(0.72, 0.72)
	get_parent().add_child(echo)
	var tween := echo.create_tween().set_parallel(true)
	tween.tween_property(echo, "scale", Vector2(1.28, 1.28), 0.32).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(echo, "modulate:a", 0.0, 0.32).set_trans(Tween.TRANS_SINE)
	tween.set_parallel(false)
	tween.tween_callback(echo.queue_free)

## 피젯 애니메이션 (5초 대기 후)
func _do_fidget() -> void:
	if not sprite:
		return
	var t = create_tween()
	# 미세한 좌우 흔들림
	t.tween_property(sprite, "scale", _scaled_sprite(Vector2(1.02, 0.98)), 0.08)
	t.tween_property(sprite, "scale", _scaled_sprite(Vector2(0.98, 1.02)), 0.08)
	t.tween_property(sprite, "scale", _sprite_rest_scale, 0.1)

## S58: Movement squash/stretch, brief scale pop on start/stop
func _play_move_squash(target_scale: Vector2, duration: float) -> void:
	if not sprite:
		return
	if _move_squash_tween and _move_squash_tween.is_running():
		_move_squash_tween.kill()
	_move_squash_tween = create_tween()
	_move_squash_tween.tween_property(sprite, "scale", _scaled_sprite(target_scale), duration * 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	_move_squash_tween.tween_property(sprite, "scale", _sprite_rest_scale, duration * 0.6).set_ease(Tween.EASE_IN_OUT)

func _scaled_sprite(multiplier: Vector2) -> Vector2:
	return Vector2(_sprite_rest_scale.x * multiplier.x, _sprite_rest_scale.y * multiplier.y)

## 인터랙션 인디케이터 업데이트
func _update_interact_indicator(delta: float) -> void:
	if not _interact_indicator:
		return
	# RayCast로 근처 상호작용 가능 대상 감지
	var show = false
	if interaction_ray:
		interaction_ray.force_raycast_update()
		if interaction_ray.is_colliding():
			var collider = interaction_ray.get_collider()
			if collider and collider.has_method("interact"):
				show = true
				var key := "A" if InputManager.is_controller_mode() else "E"
				var is_npc: bool = collider.has_method("_face_toward_player")
				var action := ("대화" if is_npc else "조사") if GameManager.current_locale == "ko" else ("Talk" if is_npc else "Inspect")
				_interact_indicator.text = "◆  %s  %s" % [key, action]
	_interact_indicator.visible = show
	if show:
		# Restrained float and breath; the pill remains anchored to the actor.
		_indicator_bob_time += delta * 3.0
		_interact_indicator.position.y = -42 + sin(_indicator_bob_time) * 1.5
		var pulse := 1.0 + sin(_indicator_bob_time * 0.8) * 0.025
		_interact_indicator.scale = Vector2.ONE * 0.62 * pulse

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprites() -> void:
	# Field sprites are authored for the camera scale.  The original AI board
	# remains available as a fallback, but never needs to be blurred into an
	# ambiguous rear-facing silhouette during normal play.
	var sheet_path := "res://assets/sprites/characters/arrel_sheet/idle_01.png"
	if PixelSprite.has_field_sprite_frames("arrel"):
		sprite.sprite_frames = PixelSprite.create_sheet_frames("arrel")
		var field_texture := sprite.sprite_frames.get_frame_texture("idle_down", 0)
		_sprite_rest_scale = PixelSprite.apply_field_profile(sprite, field_texture)
	elif ResourceLoader.exists(sheet_path):
		sprite.sprite_frames = PixelSprite.create_sheet_frames("arrel")
		sprite.scale = SHEET_SPRITE_SCALE
		_sprite_rest_scale = SHEET_SPRITE_SCALE
		sprite.offset = SHEET_SPRITE_OFFSET
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	else:
		sprite.sprite_frames = PixelSprite.create_frames(PixelSprite.arrel_config())
		sprite.scale = Vector2.ONE
		_sprite_rest_scale = Vector2.ONE
		sprite.offset = Vector2.ZERO
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST

## 애니메이션 업데이트
func _update_animation(direction: Vector2, is_moving: bool) -> void:
	if not sprite or not sprite.sprite_frames:
		return

	var anim_prefix = "walk_" if is_moving else "idle_"

	# S150: 대각선 지터 방지, 우세 축이 20% 이상 클 때만 축을 전환하고,
	# 근사 대각선에서는 기존 방향을 유지 (부호 반전만 즉시 반영)
	var ax := absf(direction.x)
	var ay := absf(direction.y)
	var suffix := _anim_suffix
	if ax > ay * 1.2:
		suffix = "right" if direction.x > 0 else "left"
	elif ay > ax * 1.2:
		suffix = "down" if direction.y > 0 else "up"
	else:
		if suffix == "left" and direction.x > 0.01:
			suffix = "right"
		elif suffix == "right" and direction.x < -0.01:
			suffix = "left"
		elif suffix == "up" and direction.y > 0.01:
			suffix = "down"
		elif suffix == "down" and direction.y < -0.01:
			suffix = "up"
	if is_moving and suffix != _anim_suffix:
		_register_turn_pivot(_anim_suffix, suffix)
	_anim_suffix = suffix

	var anim_name = anim_prefix + suffix
	if sprite.animation != anim_name:
		sprite.play(anim_name)

	# S150: 이동 속도 연동 애니 속도, 질주/감속 시 발과 지면의 미끄러짐 제거
	if is_moving:
		sprite.speed_scale = clampf(velocity.length() / BASE_SPEED, 0.65, 1.85)
	else:
		sprite.speed_scale = 1.0

## A cardinal turn carries a very short counter-lean.  It makes direction
## changes read as a planted pivot instead of swapping one paper-doll pose for
## another while the body keeps gliding.
func _register_turn_pivot(from_suffix: String, to_suffix: String) -> void:
	var from_direction := _cardinal_direction(from_suffix)
	var to_direction := _cardinal_direction(to_suffix)
	var turn_sign := signf(from_direction.cross(to_direction))
	if is_zero_approx(turn_sign):
		turn_sign = signf(to_direction.x)
		if is_zero_approx(turn_sign):
			turn_sign = -1.0 if to_direction.y < 0.0 else 1.0
	_turn_lean = turn_sign * 0.052
	_step_impact = maxf(_step_impact, 0.42)

func _cardinal_direction(suffix: String) -> Vector2:
	match suffix:
		"left":
			return Vector2.LEFT
		"right":
			return Vector2.RIGHT
		"up":
			return Vector2.UP
		_:
			return Vector2.DOWN

## RayCast 방향 업데이트 (상호작용 감지용)
func _update_raycast_direction() -> void:
	if not interaction_ray:
		return
	interaction_ray.target_position = facing_direction * 32

## 상호작용 시도
func _try_interact() -> void:
	if not interaction_ray:
		return
	interaction_ray.force_raycast_update()
	if not interaction_ray.is_colliding():
		return

	var collider = interaction_ray.get_collider()
	if collider and collider.has_method("interact"):
		collider.interact()

func _try_memory_pulse() -> void:
	if not can_move:
		return
	if _memory_pulse_cooldown > 0.0:
		NotificationToast.show_toast("Memory Pulse recharging: %.1fs" % _memory_pulse_cooldown, NotificationToast.ToastType.INFO)
		return

	_memory_pulse_cooldown = MEMORY_PULSE_COOLDOWN
	if _field_flow:
		_field_flow.open_witness_window()
	_play_memory_pulse_vfx()
	if has_node("/root/AudioManager"):
		AudioManager.play_combat_sfx("void_pulse")
	if has_node("/root/TutorialHints"):
		TutorialHints.show_hint("first_pulse")

	var scene = get_tree().current_scene if get_tree() else null
	var result := MemoryResonance.pulse_scan(scene, global_position, MEMORY_PULSE_RADIUS)
	var count: int = int(result.get("count", 0))
	if count <= 0:
		NotificationToast.show_toast("Memory Pulse: no echo nearby", NotificationToast.ToastType.INFO)
		return

	var title: String = result.get("memory_title", "unknown memory")
	var paces: int = int(round(float(result.get("distance", 0.0)) / 32.0))
	var direction: Vector2 = result.get("direction", Vector2.ZERO)
	_spawn_pulse_motes(direction)
	var bearing := _format_pulse_direction(direction)
	var new_discoveries := int(result.get("new_discoveries", 0))
	if new_discoveries > 0:
		var gained := GameManager.add_field_focus(new_discoveries)
		if gained > 0:
			GameManager.add_stat("echoes_mapped", gained)
			var focus_text := "현장 집중 +%d · 다음 전투에 공명/리미트 보너스" % gained if GameManager.current_locale == "ko" else "Field Focus +%d · next battle starts with Resonance and Limit" % gained
			NotificationToast.show_toast(focus_text, NotificationToast.ToastType.SUCCESS)
			MemoryResonance.show_field_focus_discovery()
	elif GameManager.get_field_focus() >= GameManager.FIELD_FOCUS_MAX:
		var full_text := "현장 집중이 가득 찼다 (%d/%d)" % [GameManager.get_field_focus(), GameManager.FIELD_FOCUS_MAX] if GameManager.current_locale == "ko" else "Field Focus is full (%d/%d)" % [GameManager.get_field_focus(), GameManager.FIELD_FOCUS_MAX]
		NotificationToast.show_toast(full_text, NotificationToast.ToastType.INFO)
	NotificationToast.show_toast("Memory Pulse: %d echo%s nearby" % [count, "" if count == 1 else "es"], NotificationToast.ToastType.SUCCESS)
	var echo_text := "가장 가까운 메아리: %s · %s · %d걸음" % [title, bearing, max(paces, 1)] if GameManager.current_locale == "ko" else "Nearest echo: %s · %s · %d paces" % [title, bearing, max(paces, 1)]
	NotificationToast.show_toast(echo_text, NotificationToast.ToastType.INFO)

func _format_pulse_direction(direction: Vector2) -> String:
	if direction == Vector2.ZERO:
		return "여기" if GameManager.current_locale == "ko" else "here"
	if absf(direction.x) > absf(direction.y):
		if direction.x > 0.0:
			return "동쪽" if GameManager.current_locale == "ko" else "east"
		return "서쪽" if GameManager.current_locale == "ko" else "west"
	if direction.y > 0.0:
		return "남쪽" if GameManager.current_locale == "ko" else "south"
	return "북쪽" if GameManager.current_locale == "ko" else "north"

func _play_memory_pulse_vfx() -> void:
	_spawn_pulse_ring(MEMORY_PULSE_RADIUS * 0.65, Color(0.85, 0.72, 0.38, 0.78), 0.42)
	_spawn_pulse_ring(MEMORY_PULSE_RADIUS, Color(0.58, 0.74, 1.0, 0.52), 0.58)
	if sprite:
		var tw = create_tween()
		tw.tween_property(sprite, "modulate", Color(1.35, 1.22, 0.82, 1.0), 0.10)
		tw.tween_property(sprite, "modulate", Color.WHITE, 0.28)

func _spawn_pulse_ring(radius: float, color: Color, duration: float) -> void:
	var parent = get_parent()
	if parent == null:
		return
	var ring = Line2D.new()
	ring.width = 2.0
	ring.default_color = color
	ring.closed = true
	ring.z_index = z_index + 8
	for i in range(49):
		var angle = TAU * float(i) / 48.0
		ring.add_point(Vector2(cos(angle), sin(angle)) * radius)
	ring.global_position = global_position
	ring.scale = Vector2(0.12, 0.12)
	ring.modulate.a = 0.0
	parent.add_child(ring)
	var tw = ring.create_tween()
	tw.set_parallel(true)
	tw.tween_property(ring, "scale", Vector2.ONE, duration).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tw.tween_property(ring, "modulate:a", color.a, duration * 0.25).set_trans(Tween.TRANS_SINE)
	tw.chain().tween_property(ring, "modulate:a", 0.0, duration * 0.75).set_trans(Tween.TRANS_SINE)
	tw.chain().tween_callback(ring.queue_free)

func get_memory_pulse_status() -> Dictionary:
	return {
		"cooldown": _memory_pulse_cooldown,
		"max_cooldown": MEMORY_PULSE_COOLDOWN,
		"ready": _memory_pulse_cooldown <= 0.0,
		"field_focus": GameManager.get_field_focus(),
		"field_focus_max": GameManager.FIELD_FOCUS_MAX,
	}

func get_field_flow_status() -> Dictionary:
	if _field_flow == null:
		return {
			"flow": 0.0,
			"maximum": FieldFlow.FLOW_MAX,
			"pressure": 0.0,
			"mode": "neutral",
			"dashing": false,
			"dash_ready": false,
			"dash_cost": FieldFlow.PHASE_STEP_COST,
		}
	return _field_flow.get_status()

func set_field_threat_source(source_id: String, pressure: float) -> void:
	if _field_flow:
		_field_flow.set_threat_source(source_id, pressure)

func clear_field_threat_source(source_id: String) -> void:
	if _field_flow:
		_field_flow.clear_threat_source(source_id)

func prepare_field_entry_for_battle(source: String = "ambient") -> Dictionary:
	var entry := {
		"mode": "neutral",
		"power": 0,
		"pressure": 0.0,
		"source": source,
	}
	if _field_flow:
		entry = _field_flow.consume_battle_entry(source)
	BattleManager.prepare_field_entry(String(entry.mode), int(entry.power))
	return entry

func is_field_dashing() -> bool:
	return _field_flow != null and _field_flow.is_dashing()

## S226: A clean bypass is a played choice, so part of the Phase Step comes back
## and the next threat can still be answered.
func reward_field_bypass() -> float:
	if _field_flow == null:
		return 0.0
	return _field_flow.reward_phase_bypass()

## S41: 현재 지형 타입 감지 (맵 스크립트의 terrain_map 메타 사용)
func _get_terrain_type() -> String:
	var scene = get_tree().current_scene
	if scene and scene.has_method("get_terrain_at"):
		return scene.get_terrain_at(global_position)
	# 씬 이름 기반 폴백
	if scene:
		var sname = scene.name.to_lower()
		if "coast" in sname or "sand" in sname:
			return "sand"
		elif "void" in sname or "bl07" in sname:
			return "stone"
		elif "market" in sname:
			return "stone"
	return "grass"

## 이동 잠금/해제 (컷씬, 대화 중)
func lock_movement() -> void:
	can_move = false
	velocity = Vector2.ZERO

func unlock_movement() -> void:
	can_move = true
