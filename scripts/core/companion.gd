## Companion — 동행 NPC (엘리아)
## CharacterBody2D 기반. 플레이어를 따라다니며 대화 가능.
extends CharacterBody2D

const SPRITE_SIZE: int = 32
const FOLLOW_SPEED: float = 100.0
const MIN_DISTANCE: float = 40.0   # 이 거리 이내면 멈춤
const MAX_DISTANCE: float = 200.0  # 이 거리 넘으면 텔레포트

@export var npc_name: String = "Elia"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = "elia_talk"
@export var npc_color: Color = Color(0.45, 0.55, 0.65, 1.0)

@onready var sprite: Sprite2D = $Sprite2D
var target: Node2D = null  # 따라갈 대상 (Player)

func _ready() -> void:
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
		return

	# 가까우면 멈춤
	if dist < MIN_DISTANCE:
		velocity = Vector2.ZERO
		return

	# 따라가기
	var dir = (target.position - position).normalized()
	velocity = dir * FOLLOW_SPEED
	move_and_slide()

## 상호작용 (Player의 RayCast가 호출)
func interact() -> void:
	if DialogueManager.is_active:
		return
	DialogueManager.load_and_start(dialogue_file, dialogue_key)

## 플레이스홀더 스프라이트
func _setup_placeholder_sprite() -> void:
	var img = Image.create(SPRITE_SIZE, SPRITE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	var outline = npc_color.darkened(0.4)
	_draw_rect(img, 3, 4, 28, 26, outline)
	_draw_rect(img, 4, 5, 26, 24, npc_color)

	# 눈
	var eye_color = Color(0.5, 0.7, 0.9)
	_draw_rect(img, 9, 10, 4, 4, eye_color)
	_draw_rect(img, 19, 10, 4, 4, eye_color)

	# 긴 머리 (엘리아 특징)
	_draw_rect(img, 4, 2, 24, 8, Color(0.7, 0.72, 0.75))
	_draw_rect(img, 2, 8, 4, 14, Color(0.65, 0.68, 0.72))
	_draw_rect(img, 26, 8, 4, 14, Color(0.65, 0.68, 0.72))

	# 발
	_draw_rect(img, 8, 26, 6, 4, outline)
	_draw_rect(img, 18, 26, 6, 4, outline)

	var tex = ImageTexture.create_from_image(img)
	sprite.texture = tex

func _draw_rect(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for px in range(x, mini(x + w, SPRITE_SIZE)):
		for py in range(y, mini(y + h, SPRITE_SIZE)):
			if px >= 0 and py >= 0:
				img.set_pixel(px, py, color)
