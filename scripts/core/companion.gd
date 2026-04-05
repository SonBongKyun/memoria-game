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
	var S = SPRITE_SIZE
	var img = Image.create(S, S, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	var skin = Color(0.85, 0.75, 0.68)
	var hair = Color(0.78, 0.8, 0.85)        # 은백색 긴 머리
	var cloak = Color(0.3, 0.25, 0.2)        # 갈색 망토
	var cloak_l = Color(0.38, 0.32, 0.25)
	var dress = Color(0.55, 0.5, 0.45)       # 밝은 옷
	var eye = Color(0.35, 0.6, 0.9)          # 파란 눈
	var out = Color(0.08, 0.08, 0.1)

	# 다리
	_draw_rect(img, 11, 24, 4, 6, dress)
	_draw_rect(img, 17, 24, 4, 6, dress)
	_draw_rect(img, 10, 28, 5, 3, out)
	_draw_rect(img, 17, 28, 5, 3, out)

	# 몸통 (치마/망토)
	_draw_rect(img, 8, 14, 16, 11, out)
	_draw_rect(img, 9, 15, 14, 9, cloak)
	_draw_rect(img, 11, 15, 10, 9, dress)

	# 팔 (망토 소매)
	_draw_rect(img, 5, 15, 4, 8, cloak)
	_draw_rect(img, 23, 15, 4, 8, cloak)
	_draw_rect(img, 5, 22, 4, 2, skin)
	_draw_rect(img, 23, 22, 4, 2, skin)

	# 머리
	_draw_rect(img, 8, 1, 16, 14, out)
	_draw_rect(img, 9, 2, 14, 12, skin)

	# 긴 머리카락 (은백색)
	_draw_rect(img, 8, 0, 16, 6, hair)        # 앞머리
	_draw_rect(img, 6, 1, 4, 14, hair)         # 왼쪽 긴 머리
	_draw_rect(img, 22, 1, 4, 14, hair)        # 오른쪽 긴 머리
	_draw_rect(img, 5, 14, 3, 8, hair)         # 왼쪽 늘어진 머리
	_draw_rect(img, 24, 14, 3, 8, hair)        # 오른쪽 늘어진 머리

	# 눈
	_draw_rect(img, 11, 7, 3, 3, Color.WHITE)
	_draw_rect(img, 18, 7, 3, 3, Color.WHITE)
	_draw_rect(img, 12, 8, 2, 2, eye)
	_draw_rect(img, 19, 8, 2, 2, eye)

	# 입
	_draw_rect(img, 14, 11, 4, 1, Color(0.7, 0.55, 0.5))

	# 망토 브로치
	_draw_rect(img, 15, 15, 2, 2, Color(0.7, 0.55, 0.3))

	sprite.texture = ImageTexture.create_from_image(img)

func _draw_rect(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for px in range(x, mini(x + w, SPRITE_SIZE)):
		for py in range(y, mini(y + h, SPRITE_SIZE)):
			if px >= 0 and py >= 0:
				img.set_pixel(px, py, color)
