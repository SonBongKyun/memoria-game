## NPC — 범용 NPC 스크립트
## StaticBody2D 기반. interact() 호출 시 대화 시작.
extends StaticBody2D

const SPRITE_SIZE: int = 32

@export var npc_name: String = "NPC"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = ""
@export var npc_color: Color = Color(0.6, 0.3, 0.35)  # 기본: 붉은 톤

@onready var sprite: Sprite2D = $Sprite2D

func _ready() -> void:
	_setup_placeholder_sprite()
	print("[NPC] %s ready" % npc_name)

## 플레이어의 RayCast가 호출하는 상호작용 인터페이스
func interact() -> void:
	if DialogueManager.is_active:
		return
	print("[NPC] %s — interact triggered" % npc_name)
	if dialogue_key != "":
		DialogueManager.load_and_start(dialogue_file, dialogue_key)
	else:
		# dialogue_key 미설정 시 기본 대사
		DialogueManager.start_dialogue([
			{"speaker": npc_name, "text": "...", "portrait": ""}
		])

## 플레이스홀더 스프라이트 (인간형 NPC)
func _setup_placeholder_sprite() -> void:
	var S = SPRITE_SIZE
	var img = Image.create(S, S, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	var skin = Color(0.8, 0.7, 0.6)
	var cloth = npc_color
	var cloth_d = npc_color.darkened(0.3)
	var hair_c = npc_color.lightened(0.2)
	var out = Color(0.08, 0.08, 0.1)
	var eye_c = Color(0.9, 0.75, 0.4)

	# 다리
	_draw_rect(img, 11, 24, 4, 6, cloth_d)
	_draw_rect(img, 17, 24, 4, 6, cloth_d)
	_draw_rect(img, 10, 28, 5, 3, out)
	_draw_rect(img, 17, 28, 5, 3, out)

	# 몸통
	_draw_rect(img, 8, 14, 16, 11, out)
	_draw_rect(img, 9, 15, 14, 9, cloth)

	# 팔
	_draw_rect(img, 5, 15, 4, 8, cloth)
	_draw_rect(img, 23, 15, 4, 8, cloth)
	_draw_rect(img, 5, 22, 4, 2, skin)
	_draw_rect(img, 23, 22, 4, 2, skin)

	# 머리
	_draw_rect(img, 8, 1, 16, 14, out)
	_draw_rect(img, 9, 2, 14, 12, skin)

	# 머리카락
	_draw_rect(img, 8, 0, 16, 5, hair_c)
	_draw_rect(img, 8, 1, 3, 8, hair_c)
	_draw_rect(img, 21, 1, 3, 8, hair_c)

	# 눈
	_draw_rect(img, 11, 7, 3, 3, Color.WHITE)
	_draw_rect(img, 18, 7, 3, 3, Color.WHITE)
	_draw_rect(img, 12, 8, 2, 2, eye_c)
	_draw_rect(img, 19, 8, 2, 2, eye_c)

	# 입
	_draw_rect(img, 14, 11, 4, 1, Color(0.65, 0.5, 0.45))

	sprite.texture = ImageTexture.create_from_image(img)

func _draw_rect(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for px in range(x, mini(x + w, SPRITE_SIZE)):
		for py in range(y, mini(y + h, SPRITE_SIZE)):
			if px >= 0 and py >= 0:
				img.set_pixel(px, py, color)
