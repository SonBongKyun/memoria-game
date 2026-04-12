## Player — 아렐
## 탑다운 2D 이동, 상호작용, 기본 상태 관리.
extends CharacterBody2D

const SPEED: float = 120.0
const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드

@onready var sprite: AnimatedSprite2D = $AnimatedSprite2D
@onready var interaction_ray: RayCast2D = $InteractionRay

var facing_direction: Vector2 = Vector2.DOWN
var can_move: bool = true
var _step_timer: float = 0.0
var _breath_time: float = 0.0  # S52: 호흡 애니메이션
const STEP_INTERVAL: float = 0.25

func _ready() -> void:
	add_to_group("player")
	_setup_placeholder_sprites()
	if sprite and sprite.sprite_frames:
		sprite.play("idle_down")
	print("[Player] Arrel ready")

func _physics_process(delta: float) -> void:
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
		if sprite:
			sprite.scale = Vector2(1.0, 1.0)  # 이동 중 스케일 초기화
	else:
		_update_animation(facing_direction, false)
		# S52: 정지 시 호흡 미세 스케일
		_breath_time += delta
		if sprite:
			sprite.scale = Vector2(1.0 + sin(_breath_time * 2.0) * 0.01, 1.0 - sin(_breath_time * 2.0) * 0.008)

	# S41: 지형별 발걸음 SFX
	if input_vector != Vector2.ZERO:
		_step_timer += delta
		if _step_timer >= STEP_INTERVAL:
			_step_timer = 0.0
			var terrain = _get_terrain_type()
			AudioManager.play_step(terrain)
	else:
		_step_timer = 0.0

func _input(event: InputEvent) -> void:
	# 상호작용 (Space / Enter) — 탐색 모드에서만
	if event.is_action_pressed("interact") and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_try_interact()

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprites() -> void:
	sprite.sprite_frames = PixelSprite.create_frames(PixelSprite.arrel_config())

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
