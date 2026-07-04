## Companion — 동행 NPC (엘리아)
## CharacterBody2D 기반. 플레이어를 따라다니며 대화 가능.
extends CharacterBody2D

const SHEET_SPRITE_SCALE: Vector2 = Vector2.ONE

const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드
const FOLLOW_SPEED: float = 100.0
const MIN_DISTANCE: float = 40.0   # 이 거리 이내면 멈춤
const MAX_DISTANCE: float = 200.0  # 이 거리 넘으면 텔레포트
# S150: 자연스러운 추적 — 소프트존/가속/방향 스무딩
const SOFT_ZONE: float = 70.0        # MIN~MIN+SOFT 구간에서 속도가 거리 비례로 상승
const FOLLOW_ACCEL: float = 480.0    # px/s^2
const SPRINT_CATCHUP: float = 1.6    # 플레이어 질주 시 최대 배속

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

func _ready() -> void:
	# Sprite2D → AnimatedSprite2D 교체 (픽셀 스���라이트 지원)
	if has_node("Sprite2D"):
		$Sprite2D.queue_free()
	sprite = AnimatedSprite2D.new()
	sprite.name = "AnimatedSprite2D"
	add_child(sprite)
	_setup_placeholder_sprite()
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
	print("[Companion] %s ready — following %s" % [npc_name, target.name if target else "nobody"])

func _physics_process(delta: float) -> void:
	if not target or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		return
	if sprite and not _base_offset_captured:
		_base_offset = sprite.offset
		_base_offset_captured = true

	var dist = position.distance_to(target.position)

	# 너무 멀면 텔레포트 (S150: 순간이동 대신 짧은 페이드로 눈에 덜 띄게)
	if dist > MAX_DISTANCE:
		position = target.position + Vector2(30, 20)
		velocity = Vector2.ZERO
		_update_animation(Vector2.DOWN, false)
		if sprite:
			sprite.modulate.a = 0.0
			var tw = create_tween()
			tw.tween_property(sprite, "modulate:a", 1.0, 0.35).set_ease(Tween.EASE_OUT)
		return

	# S150: 소프트존 — 거리 비례 목표 속도 (이진 정지/출발로 인한 덜컹거림 제거)
	# MIN 이내: 0 / MIN~MIN+SOFT: 선형 상승 / 그 위: 최대 (플레이어 질주 시 추가 가속)
	var raw_dir := (target.position - position).normalized()
	var target_speed := 0.0
	if dist > MIN_DISTANCE:
		var t := clampf((dist - MIN_DISTANCE) / SOFT_ZONE, 0.0, 1.0)
		var catchup := 1.0
		if target is CharacterBody2D and target.velocity.length() > 130.0:
			catchup = SPRINT_CATCHUP
		target_speed = FOLLOW_SPEED * lerpf(0.35, catchup, t)

	# 가속/감속 + 방향 스무딩 (급회전 대신 호를 그리며 따라옴)
	_smooth_dir = _smooth_dir.slerp(raw_dir, clampf(7.0 * delta, 0.0, 1.0)).normalized()
	velocity = velocity.move_toward(_smooth_dir * target_speed, FOLLOW_ACCEL * delta)
	if velocity.length() > 4.0:
		move_and_slide()

	var visually_moving := velocity.length() > 12.0
	_update_animation(_smooth_dir if visually_moving else raw_dir, visually_moving)

	# S150: 걷기 바운스 + 정지 호흡 (플레이어와 같은 문법)
	if sprite:
		if visually_moving:
			_bob_phase += velocity.length() * delta * 0.085
			sprite.offset.y = _base_offset.y - absf(sin(_bob_phase)) * 1.4
			sprite.rotation = lerp_angle(sprite.rotation, (velocity.x / (FOLLOW_SPEED * SPRINT_CATCHUP)) * 0.05, 9.0 * delta)
			_breath_time = 0.0
			sprite.scale = sprite.scale.lerp(SHEET_SPRITE_SCALE, 10.0 * delta)
		else:
			sprite.offset.y = lerpf(sprite.offset.y, _base_offset.y, 12.0 * delta)
			sprite.rotation = lerp_angle(sprite.rotation, 0.0, 12.0 * delta)
			_breath_time += delta
			sprite.scale = SHEET_SPRITE_SCALE * Vector2(1.0 + sin(_breath_time * 1.8) * 0.008, 1.0 - sin(_breath_time * 1.8) * 0.006)

## 상호작용 (Player의 RayCast가 호출)
func interact() -> void:
	if DialogueManager.is_active:
		return

	var talk_flag = "talked_%s_%s" % [npc_name, dialogue_key]
	if _talked_keys.has(dialogue_key) or GameManager.get_flag(talk_flag):
		# 이미 대화한 동행 — 짧은 후속 대사
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
	# npc_name에 따라 다른 config 사용
	if npc_name == "Sable":
		config = PixelSprite.sable_config()
		sprite.sprite_frames = PixelSprite.create_frames(config)
		sprite.scale = Vector2.ONE
	else:
		# Match Arrel's true top-down four-direction animation instead of reusing
		# a side-view frame for three directions.
		sprite.sprite_frames = PixelSprite.create_frames(PixelSprite.elia_config())
		sprite.scale = Vector2.ONE
	sprite.play("idle_down")

## 애니메이션 방향 업데이트
func _update_animation(direction: Vector2, is_moving: bool) -> void:
	if not sprite or not sprite.sprite_frames:
		return
	var prefix = "walk_" if is_moving else "idle_"
	var suffix: String
	if abs(direction.x) > abs(direction.y):
		suffix = "right" if direction.x > 0 else "left"
	else:
		suffix = "down" if direction.y > 0 else "up"
	var anim = prefix + suffix
	if sprite.animation != anim:
		sprite.play(anim)
	# S150: 이동 속도 연동 애니 속도 — 따라잡기 가속 시 발 미끄러짐 제거
	if is_moving:
		sprite.speed_scale = clampf(velocity.length() / FOLLOW_SPEED, 0.6, 1.7)
	else:
		sprite.speed_scale = 1.0
