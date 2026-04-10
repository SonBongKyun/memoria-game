## Companion — 동행 NPC (엘리아)
## CharacterBody2D 기반. 플레이어를 따라다니며 대화 가능.
extends CharacterBody2D

const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드
const FOLLOW_SPEED: float = 100.0
const MIN_DISTANCE: float = 40.0   # 이 거리 이내면 멈춤
const MAX_DISTANCE: float = 200.0  # 이 거리 넘으면 텔레포트

@export var npc_name: String = "Elia"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = "elia_talk"
@export var npc_color: Color = Color(0.45, 0.55, 0.65, 1.0)
@export var repeat_line: String = ""  # 재대화 시 표시할 대사

var _talked_keys: Dictionary = {}  # 이미 진행한 dialogue_key 추적

var sprite: AnimatedSprite2D
var target: Node2D = null  # 따라갈 대상 (Player)

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

func _physics_process(_delta: float) -> void:
	if not target or GameManager.current_state != GameManager.GameState.EXPLORATION:
		velocity = Vector2.ZERO
		return

	var dist = position.distance_to(target.position)

	# 너무 멀면 텔레포트
	if dist > MAX_DISTANCE:
		position = target.position + Vector2(30, 20)
		velocity = Vector2.ZERO
		_update_animation(Vector2.DOWN, false)
		return

	# 가까우면 멈춤
	if dist < MIN_DISTANCE:
		velocity = Vector2.ZERO
		var dir = (target.position - position).normalized()
		_update_animation(dir, false)
		return

	# 따라가기
	var dir = (target.position - position).normalized()
	velocity = dir * FOLLOW_SPEED
	move_and_slide()
	_update_animation(dir, true)

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
