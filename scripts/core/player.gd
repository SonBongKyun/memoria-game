## Player — 아렐
## 탑다운 2D 이동, 상호작용, 기본 상태 관리.
extends CharacterBody2D

const SPEED: float = 120.0
const SPRITE_SIZE: int = 32

@onready var sprite: AnimatedSprite2D = $AnimatedSprite2D
@onready var interaction_ray: RayCast2D = $InteractionRay

var facing_direction: Vector2 = Vector2.DOWN
var can_move: bool = true

func _ready() -> void:
	add_to_group("player")
	_setup_placeholder_sprites()
	sprite.play("idle_down")
	print("[Player] Arrel ready")

func _physics_process(_delta: float) -> void:
	if not can_move or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		return

	# 입력 처리
	var input_vector = Vector2.ZERO
	input_vector.x = Input.get_axis("move_left", "move_right")
	input_vector.y = Input.get_axis("move_up", "move_down")
	input_vector = input_vector.normalized()

	# 이동
	velocity = input_vector * SPEED
	move_and_slide()

	# 애니메이션 방향
	if input_vector != Vector2.ZERO:
		facing_direction = input_vector
		_update_animation(input_vector, true)
		_update_raycast_direction()
	else:
		_update_animation(facing_direction, false)

func _input(event: InputEvent) -> void:
	# 상호작용 (Space / Enter) — 탐색 모드에서만
	if event.is_action_pressed("interact") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_interact()

## 플레이스홀더 SpriteFrames 동적 생성
## 나중에 실제 스프라이트 에셋으로 교체 시 이 함수만 제거하면 됨.
func _setup_placeholder_sprites() -> void:
	var frames = SpriteFrames.new()
	# 기본 "default" 애니메이션 제거
	if frames.has_animation("default"):
		frames.remove_animation("default")

	# 방향별 색상 (몸통 / 방향 표시)
	var dir_colors = {
		"down": Color(0.2, 0.25, 0.4),   # 정면 — 다크 블루
		"up": Color(0.15, 0.2, 0.35),    # 뒷면 — 더 어두운 블루
		"left": Color(0.18, 0.22, 0.38), # 왼쪽
		"right": Color(0.22, 0.28, 0.42) # 오른쪽
	}
	var eye_color = Color(0.85, 0.6, 0.2)  # 주황 눈 (방향 표시)
	var body_outline = Color(0.1, 0.12, 0.2)

	for dir_name in ["down", "up", "left", "right"]:
		var body_color = dir_colors[dir_name]

		# idle 애니메이션 (1프레임)
		var idle_name = "idle_" + dir_name
		frames.add_animation(idle_name)
		frames.set_animation_speed(idle_name, 4)
		frames.set_animation_loop(idle_name, true)
		var idle_tex = _create_frame(body_color, body_outline, eye_color, dir_name, 0)
		frames.add_frame(idle_name, idle_tex)

		# walk 애니메이션 (4프레임 — 발 위치 변화)
		var walk_name = "walk_" + dir_name
		frames.add_animation(walk_name)
		frames.set_animation_speed(walk_name, 8)
		frames.set_animation_loop(walk_name, true)
		for frame_idx in range(4):
			var walk_tex = _create_frame(body_color, body_outline, eye_color, dir_name, frame_idx)
			frames.add_frame(walk_name, walk_tex)

	sprite.sprite_frames = frames

## 단일 프레임 이미지 생성
func _create_frame(body_color: Color, outline_color: Color, eye_color: Color, direction: String, frame_idx: int) -> Texture2D:
	var img = Image.create(SPRITE_SIZE, SPRITE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))  # 투명 배경

	# 몸통 (아웃라인 포함)
	_draw_rect_on_image(img, 3, 4, 28, 26, outline_color)   # 아웃라인
	_draw_rect_on_image(img, 4, 5, 26, 24, body_color)      # 몸통

	# 걷기 프레임별 발 위치 (좌/우 발 번갈아 움직임)
	var foot_offset = 0
	match frame_idx:
		0: foot_offset = 0
		1: foot_offset = -2
		2: foot_offset = 0
		3: foot_offset = 2

	# 발 (하단)
	_draw_rect_on_image(img, 8 + foot_offset, 26, 6, 4, outline_color)   # 왼발
	_draw_rect_on_image(img, 18 - foot_offset, 26, 6, 4, outline_color)  # 오른발

	# 방향 표시 (눈 or 화살표)
	match direction:
		"down":
			_draw_rect_on_image(img, 9, 10, 4, 4, eye_color)    # 왼쪽 눈
			_draw_rect_on_image(img, 19, 10, 4, 4, eye_color)   # 오른쪽 눈
		"up":
			# 뒷모습 — 머리카락 표시
			_draw_rect_on_image(img, 6, 2, 20, 6, Color(0.15, 0.15, 0.25))
		"left":
			_draw_rect_on_image(img, 6, 10, 4, 4, eye_color)    # 왼쪽에 눈
		"right":
			_draw_rect_on_image(img, 22, 10, 4, 4, eye_color)   # 오른쪽에 눈

	var tex = ImageTexture.create_from_image(img)
	return tex

## Image에 사각형 그리기 헬퍼
func _draw_rect_on_image(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for px in range(x, mini(x + w, SPRITE_SIZE)):
		for py in range(y, mini(y + h, SPRITE_SIZE)):
			if px >= 0 and py >= 0:
				img.set_pixel(px, py, color)

## 애니메이션 업데이트
func _update_animation(direction: Vector2, is_moving: bool) -> void:
	if not sprite or not sprite.sprite_frames:
		return

	var anim_prefix = "walk_" if is_moving else "idle_"
	var dir_suffix: String

	if abs(direction.x) > abs(direction.y):
		dir_suffix = "right" if direction.x > 0 else "left"
	else:
		dir_suffix = "down" if direction.y > 0 else "up"

	var anim_name = anim_prefix + dir_suffix
	if sprite.animation != anim_name:
		sprite.play(anim_name)

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

## 이동 잠금/해제 (컷씬, 대화 중)
func lock_movement() -> void:
	can_move = false
	velocity = Vector2.ZERO

func unlock_movement() -> void:
	can_move = true
