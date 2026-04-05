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

## 플레이스홀더 스프라이트 (플레이어와 다른 색상)
func _setup_placeholder_sprite() -> void:
	var img = Image.create(SPRITE_SIZE, SPRITE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	# 몸통
	var outline = npc_color.darkened(0.4)
	_draw_rect(img, 3, 4, 28, 26, outline)
	_draw_rect(img, 4, 5, 26, 24, npc_color)

	# 눈 (정면 고정)
	var eye_color = Color(0.9, 0.85, 0.7)
	_draw_rect(img, 9, 10, 4, 4, eye_color)
	_draw_rect(img, 19, 10, 4, 4, eye_color)

	# 발
	_draw_rect(img, 8, 26, 6, 4, outline)
	_draw_rect(img, 18, 26, 6, 4, outline)

	# 이름 첫 글자 표시 (시각적 구분용) — 가슴 부분에 밝은 점
	_draw_rect(img, 13, 17, 6, 6, npc_color.lightened(0.3))

	var tex = ImageTexture.create_from_image(img)
	sprite.texture = tex

func _draw_rect(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for px in range(x, mini(x + w, SPRITE_SIZE)):
		for py in range(y, mini(y + h, SPRITE_SIZE)):
			if px >= 0 and py >= 0:
				img.set_pixel(px, py, color)
