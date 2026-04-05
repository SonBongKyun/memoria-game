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
	if frames.has_animation("default"):
		frames.remove_animation("default")

	# 아렐 색상
	var skin = Color(0.82, 0.7, 0.6)
	var hair = Color(0.45, 0.48, 0.55)       # 재색 머리
	var coat = Color(0.15, 0.18, 0.28)       # 어두운 코트
	var coat_light = Color(0.2, 0.24, 0.35)  # 코트 밝은 면
	var pants = Color(0.12, 0.12, 0.15)
	var eye = Color(0.3, 0.55, 0.85)         # 파란 눈
	var outline = Color(0.08, 0.08, 0.1)

	for dir_name in ["down", "up", "left", "right"]:
		var idle_name = "idle_" + dir_name
		frames.add_animation(idle_name)
		frames.set_animation_speed(idle_name, 4)
		frames.set_animation_loop(idle_name, true)
		frames.add_frame(idle_name, _create_arrel_frame(dir_name, 0, skin, hair, coat, coat_light, pants, eye, outline))

		var walk_name = "walk_" + dir_name
		frames.add_animation(walk_name)
		frames.set_animation_speed(walk_name, 8)
		frames.set_animation_loop(walk_name, true)
		for f in range(4):
			frames.add_frame(walk_name, _create_arrel_frame(dir_name, f, skin, hair, coat, coat_light, pants, eye, outline))

	sprite.sprite_frames = frames

func _create_arrel_frame(dir: String, frame: int, skin: Color, hair: Color, coat: Color, coat_l: Color, pants: Color, eye: Color, out: Color) -> Texture2D:
	var S = SPRITE_SIZE
	var img = Image.create(S, S, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	# 걷기 흔들림
	var bob = 0
	if frame == 1: bob = -1
	elif frame == 3: bob = -1
	var leg_off = 0
	if frame == 1: leg_off = -2
	elif frame == 3: leg_off = 2

	# === 다리 (하단) ===
	_draw_rect_on_image(img, 10 + leg_off, 24 - bob, 4, 7, pants)     # 왼다리
	_draw_rect_on_image(img, 18 - leg_off, 24 - bob, 4, 7, pants)     # 오른다리
	_draw_rect_on_image(img, 9 + leg_off, 28 - bob, 6, 3, out)        # 왼발
	_draw_rect_on_image(img, 17 - leg_off, 28 - bob, 6, 3, out)       # 오른발

	# === 몸통 (코트) ===
	_draw_rect_on_image(img, 8, 13 + bob, 16, 12, out)     # 아웃라인
	_draw_rect_on_image(img, 9, 14 + bob, 14, 10, coat)    # 코트
	# 코트 중앙선 / 밝은 면
	if dir == "down":
		_draw_rect_on_image(img, 15, 14 + bob, 2, 10, coat_l)  # 중앙 단추 라인
	elif dir == "left":
		_draw_rect_on_image(img, 9, 14 + bob, 4, 10, coat_l)
	elif dir == "right":
		_draw_rect_on_image(img, 19, 14 + bob, 4, 10, coat_l)

	# === 팔 ===
	_draw_rect_on_image(img, 5, 15 + bob, 4, 9, coat)   # 왼팔
	_draw_rect_on_image(img, 23, 15 + bob, 4, 9, coat)  # 오른팔
	_draw_rect_on_image(img, 5, 23 + bob, 4, 2, skin)   # 왼손
	_draw_rect_on_image(img, 23, 23 + bob, 4, 2, skin)  # 오른손

	# === 머리 ===
	_draw_rect_on_image(img, 8, 1 + bob, 16, 14, out)    # 머리 아웃라인
	_draw_rect_on_image(img, 9, 2 + bob, 14, 12, skin)   # 얼굴

	# 머리카락
	match dir:
		"down":
			_draw_rect_on_image(img, 8, 1 + bob, 16, 5, hair)      # 앞머리
			_draw_rect_on_image(img, 8, 1 + bob, 3, 10, hair)      # 왼쪽 옆머리
			_draw_rect_on_image(img, 21, 1 + bob, 3, 10, hair)     # 오른쪽 옆머리
			# 눈
			_draw_rect_on_image(img, 11, 8 + bob, 3, 3, Color.WHITE)
			_draw_rect_on_image(img, 18, 8 + bob, 3, 3, Color.WHITE)
			_draw_rect_on_image(img, 12, 9 + bob, 2, 2, eye)
			_draw_rect_on_image(img, 19, 9 + bob, 2, 2, eye)
			# 입
			_draw_rect_on_image(img, 14, 12 + bob, 4, 1, Color(0.65, 0.5, 0.45))
		"up":
			_draw_rect_on_image(img, 8, 1 + bob, 16, 12, hair)     # 뒷머리 전체
			_draw_rect_on_image(img, 7, 8 + bob, 3, 6, hair)       # 왼쪽 늘어진 머리
			_draw_rect_on_image(img, 22, 8 + bob, 3, 6, hair)      # 오른쪽
		"left":
			_draw_rect_on_image(img, 8, 1 + bob, 16, 5, hair)
			_draw_rect_on_image(img, 8, 1 + bob, 5, 12, hair)      # 왼쪽 머리 두꺼움
			_draw_rect_on_image(img, 21, 1 + bob, 3, 8, hair)
			_draw_rect_on_image(img, 11, 8 + bob, 3, 3, Color.WHITE)
			_draw_rect_on_image(img, 12, 9 + bob, 2, 2, eye)
			_draw_rect_on_image(img, 13, 12 + bob, 3, 1, Color(0.65, 0.5, 0.45))
		"right":
			_draw_rect_on_image(img, 8, 1 + bob, 16, 5, hair)
			_draw_rect_on_image(img, 19, 1 + bob, 5, 12, hair)
			_draw_rect_on_image(img, 8, 1 + bob, 3, 8, hair)
			_draw_rect_on_image(img, 18, 8 + bob, 3, 3, Color.WHITE)
			_draw_rect_on_image(img, 18, 9 + bob, 2, 2, eye)
			_draw_rect_on_image(img, 16, 12 + bob, 3, 1, Color(0.65, 0.5, 0.45))

	return ImageTexture.create_from_image(img)

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
