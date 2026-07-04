## Player — 아렐
## 탑다운 2D 이동, 상호작용, 기본 상태 관리.
## S57: Camera2D 시스템 + 탐색 폴리시 (대시, 가속, 먼지, 인터랙션 인디케이터, 피젯)
extends CharacterBody2D

const BASE_SPEED: float = 120.0
const SPRINT_MULTIPLIER: float = 1.8
const SHEET_SPRITE_SCALE: Vector2 = Vector2.ONE
const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드
const ACCELERATION: float = 600.0   # px/s^2 — 가속
const DECELERATION: float = 800.0   # px/s^2 — 감속 (더 빠르게 멈춤)
const MEMORY_PULSE_RADIUS: float = 150.0
const MEMORY_PULSE_COOLDOWN: float = 6.0

@onready var sprite: AnimatedSprite2D = $AnimatedSprite2D
@onready var interaction_ray: RayCast2D = $InteractionRay
@onready var camera: Camera2D = $Camera2D

var facing_direction: Vector2 = Vector2.DOWN
var can_move: bool = true
var _step_timer: float = 0.0
var _breath_time: float = 0.0  # S52: 호흡 애니메이션
const STEP_INTERVAL: float = 0.25

# --- S57: Camera ---
var _camera_base_zoom: Vector2 = Vector2(2.25, 2.25)
var _camera_look_ahead: float = 50.0  # 이동 방향으로 미리보기 px
var _shake_intensity: float = 0.0
var _shake_duration: float = 0.0
var _shake_timer: float = 0.0

# --- S57: Sprint & Afterimage ---
var _is_sprinting: bool = false
var _afterimage_timer: float = 0.0
const AFTERIMAGE_INTERVAL: float = 0.06  # 잔상 생성 간격

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
var _last_footfall: int = 0          # 발이 땅에 닿은 횟수 (먼지 동기용)
var _sprite_base_offset: Vector2 = Vector2.ZERO
var _anim_suffix: String = "down"    # 대각선 지터 방지용 최근 방향

func _ready() -> void:
	add_to_group("player")
	_setup_placeholder_sprites()
	if sprite and sprite.sprite_frames:
		sprite.play("idle_down")
		_sprite_base_offset = sprite.offset
	_setup_camera()
	_setup_interact_indicator()
	print("[Player] Arrel ready — Camera2D + exploration polish active")

## Camera2D 초기 설정
func _setup_camera() -> void:
	if not camera:
		return
	camera.enabled = true
	# 픽셀 스내핑 (pixel-perfect for 32px tiles)
	camera.set_meta("pixel_snap", true)
	# 맵 한계 — 씬에서 MAP_WIDTH/MAP_HEIGHT 읽기
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
	_interact_indicator.text = "[E]"
	_interact_indicator.add_theme_font_size_override("font_size", 10)
	_interact_indicator.add_theme_color_override("font_color", Color(1.0, 0.95, 0.7, 0.95))
	_interact_indicator.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.8))
	_interact_indicator.add_theme_constant_override("shadow_offset_x", 1)
	_interact_indicator.add_theme_constant_override("shadow_offset_y", 1)
	_interact_indicator.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_interact_indicator.position = Vector2(-14, -40)
	_interact_indicator.z_index = 10
	_interact_indicator.visible = false
	add_child(_interact_indicator)

func _physics_process(delta: float) -> void:
	if _memory_pulse_cooldown > 0.0:
		_memory_pulse_cooldown = maxf(0.0, _memory_pulse_cooldown - delta)

	if not can_move or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		_idle_time = 0.0
		return

	# 입력 처리
	var input_vector = Vector2.ZERO
	input_vector.x = Input.get_axis("move_left", "move_right")
	input_vector.y = Input.get_axis("move_up", "move_down")
	input_vector = input_vector.normalized()

	# S57: Sprint
	_is_sprinting = Input.is_action_pressed("sprint") and input_vector != Vector2.ZERO
	var speed = BASE_SPEED * (SPRINT_MULTIPLIER if _is_sprinting else 1.0)

	# S57: Acceleration / Deceleration (lerp velocity)
	var target_velocity = input_vector * speed
	if input_vector != Vector2.ZERO:
		velocity = velocity.move_toward(target_velocity, ACCELERATION * delta)
	else:
		velocity = velocity.move_toward(Vector2.ZERO, DECELERATION * delta)

	move_and_slide()

	# 애니메이션 방향
	var is_moving = input_vector != Vector2.ZERO
	if is_moving:
		facing_direction = input_vector
		_update_animation(input_vector, true)
		_update_raycast_direction()
		# S58: Movement start squash — brief compression when beginning to walk
		if not _was_moving and sprite:
			_play_move_squash(Vector2(0.95, 1.05), 0.05)
		# S58: Sprint stretch — elongate in movement direction while sprinting
		if _is_sprinting and sprite:
			if abs(input_vector.x) > abs(input_vector.y):
				sprite.scale = sprite.scale.lerp(_scaled_sprite(Vector2(1.08, 0.92)), 8.0 * delta)
			else:
				sprite.scale = sprite.scale.lerp(_scaled_sprite(Vector2(0.92, 1.08)), 8.0 * delta)
		elif sprite:
			sprite.scale = sprite.scale.lerp(SHEET_SPRITE_SCALE, 10.0 * delta)
		_idle_time = 0.0
		_fidget_timer = 0.0
	else:
		# S150: 감속 중에는 걷기 애니 유지 — 몸이 미끄러지는데 발이 멈추는 어색함 제거
		_update_animation(facing_direction, velocity.length() > 12.0)
		# S58: Movement stop stretch — brief elongation when stopping
		if _was_moving and sprite:
			_play_move_squash(Vector2(1.05, 0.95), 0.08)
		# S52: 정지 시 호흡 미세 스케일 + S57: 피젯
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
	_was_moving = is_moving

	# S41: 지형별 발걸음 SFX
	if input_vector != Vector2.ZERO:
		_step_timer += delta
		if _step_timer >= STEP_INTERVAL:
			_step_timer = 0.0
			var terrain = _get_terrain_type()
			AudioManager.play_step(terrain)
			GameManager.add_stat("steps_taken")  # S55: 걸음 수 추적
	else:
		_step_timer = 0.0

	# S57: Camera look-ahead
	_update_camera_look_ahead(delta)

	# S57: Camera shake
	_update_camera_shake(delta)

	# S160: clarity mode removes movement trails and footstep debris.
	var clean_view := OptionsMenu.is_clean_gameplay_visuals()
	# S57: Afterimage (sprint)
	if _is_sprinting and not clean_view:
		_afterimage_timer += delta
		if _afterimage_timer >= AFTERIMAGE_INTERVAL:
			_afterimage_timer = 0.0
			_spawn_afterimage()
	else:
		_afterimage_timer = 0.0

	# S150: 걷기 바운스 + 발딛기 먼지 동기 + 수평 기울기
	# 바운스 위상이 발걸음의 단일 진실원 — 먼지가 발이 닿는 프레임에 정확히 맞음.
	if sprite:
		if velocity.length() > 12.0:
			_bob_phase += velocity.length() * delta * 0.085
			sprite.offset.y = _sprite_base_offset.y - absf(sin(_bob_phase)) * 1.6
			var footfall := int(_bob_phase / PI)
			if footfall != _last_footfall:
				_last_footfall = footfall
				if not clean_view:
					_spawn_dust()
			# 수평 이동 시 진행 방향으로 미세하게 기울어짐 (달릴수록 더)
			sprite.rotation = lerp_angle(sprite.rotation, (velocity.x / (BASE_SPEED * SPRINT_MULTIPLIER)) * 0.055, 10.0 * delta)
		else:
			sprite.offset.y = lerpf(sprite.offset.y, _sprite_base_offset.y, 12.0 * delta)
			sprite.rotation = lerp_angle(sprite.rotation, 0.0, 12.0 * delta)

	# S57: Interaction indicator
	_update_interact_indicator(delta)

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("memory_pulse") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_memory_pulse()
		get_viewport().set_input_as_handled()
		return

	# 상호작용 (Space / Enter) — 탐색 모드에서만
	if event.is_action_pressed("interact") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_interact()

# ─── Camera ────────────────────────────────────────

## 카메라 룩어헤드: 이동 방향으로 오프셋
func _update_camera_look_ahead(delta: float) -> void:
	if not camera:
		return
	var target_offset = Vector2.ZERO
	if velocity.length() > 10.0 and not OptionsMenu.is_clean_gameplay_visuals():
		target_offset = velocity.normalized() * _camera_look_ahead
	camera.offset = camera.offset.lerp(target_offset, 3.0 * delta)

## 카메라 셰이크 업데이트
func _update_camera_shake(delta: float) -> void:
	if not camera:
		return
	if OptionsMenu.is_clean_gameplay_visuals():
		_shake_intensity = 0.0
		_shake_timer = 0.0
		return
	if _shake_timer > 0.0:
		_shake_timer -= delta
		var shake_amount = _shake_intensity * (_shake_timer / _shake_duration)
		camera.offset += Vector2(randf_range(-shake_amount, shake_amount), randf_range(-shake_amount, shake_amount))
	elif _shake_intensity > 0.0:
		_shake_intensity = 0.0

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
	ghost.modulate = Color(0.4, 0.5, 0.8, 0.5)
	ghost.z_index = z_index - 1
	get_parent().add_child(ghost)
	var t = ghost.create_tween()
	t.tween_property(ghost, "modulate:a", 0.0, 0.25)
	t.tween_callback(ghost.queue_free)

## 발밑 먼지 파티클 — S59: terrain-specific dust colors
func _spawn_dust() -> void:
	var dust = ColorRect.new()
	var size = randf_range(2.0, 3.5)
	dust.size = Vector2(size, size)
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
	dust.color = dust_color
	# 발밑 랜덤 위치
	dust.global_position = global_position + Vector2(randf_range(-6, 6), randf_range(4, 10))
	dust.z_index = z_index - 1
	get_parent().add_child(dust)
	var t = dust.create_tween()
	t.set_parallel(true)
	t.tween_property(dust, "modulate:a", 0.0, 0.3)
	t.tween_property(dust, "global_position:y", dust.global_position.y - randf_range(3, 7), 0.3)
	t.set_parallel(false)
	t.tween_callback(dust.queue_free)

## 피젯 애니메이션 (5초 대기 후)
func _do_fidget() -> void:
	if not sprite:
		return
	var t = create_tween()
	# 미세한 좌우 흔들림
	t.tween_property(sprite, "scale", _scaled_sprite(Vector2(1.02, 0.98)), 0.08)
	t.tween_property(sprite, "scale", _scaled_sprite(Vector2(0.98, 1.02)), 0.08)
	t.tween_property(sprite, "scale", SHEET_SPRITE_SCALE, 0.1)

## S58: Movement squash/stretch — brief scale pop on start/stop
func _play_move_squash(target_scale: Vector2, duration: float) -> void:
	if not sprite:
		return
	if _move_squash_tween and _move_squash_tween.is_running():
		_move_squash_tween.kill()
	_move_squash_tween = create_tween()
	_move_squash_tween.tween_property(sprite, "scale", _scaled_sprite(target_scale), duration * 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	_move_squash_tween.tween_property(sprite, "scale", SHEET_SPRITE_SCALE, duration * 0.6).set_ease(Tween.EASE_IN_OUT)

func _scaled_sprite(multiplier: Vector2) -> Vector2:
	return Vector2(SHEET_SPRITE_SCALE.x * multiplier.x, SHEET_SPRITE_SCALE.y * multiplier.y)

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
				# InputManager 연동: 컨트롤러면 "A", 키보드면 "E"
				if InputManager.is_controller_mode():
					_interact_indicator.text = "[A]"
				else:
					_interact_indicator.text = "[E]"
	_interact_indicator.visible = show
	if show:
		# 위아래 부유 효과
		_indicator_bob_time += delta * 3.0
		_interact_indicator.position.y = -40 + sin(_indicator_bob_time) * 3.0

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprites() -> void:
	# The imported side-view sheet reused the same pose for up/down/right, so
	# exploration movement did not visually turn with the player. The authored
	# 48px top-down set has true four-direction silhouettes and four walk phases.
	sprite.sprite_frames = PixelSprite.create_frames(PixelSprite.arrel_config())
	sprite.scale = Vector2.ONE

## 애니메이션 업데이트
func _update_animation(direction: Vector2, is_moving: bool) -> void:
	if not sprite or not sprite.sprite_frames:
		return

	var anim_prefix = "walk_" if is_moving else "idle_"

	# S150: 대각선 지터 방지 — 우세 축이 20% 이상 클 때만 축을 전환하고,
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
	_anim_suffix = suffix

	var anim_name = anim_prefix + suffix
	if sprite.animation != anim_name:
		sprite.play(anim_name)

	# S150: 이동 속도 연동 애니 속도 — 질주/감속 시 발과 지면의 미끄러짐 제거
	if is_moving:
		sprite.speed_scale = clampf(velocity.length() / BASE_SPEED, 0.65, 1.85)
	else:
		sprite.speed_scale = 1.0

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
