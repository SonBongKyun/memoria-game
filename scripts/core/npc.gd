## NPC — 범용 NPC 스크립트
## StaticBody2D 기반. interact() 호출 시 대화 시작.
extends StaticBody2D

const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드

@export var npc_name: String = "NPC"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = ""
@export var npc_color: Color = Color(0.6, 0.3, 0.35)  # 기본: 붉은 톤
@export var repeat_line: String = ""  # 재대화 시 표시할 대사 (빈칸이면 기본 대사)

var _talked_keys: Dictionary = {}  # 이미 진행한 dialogue_key 추적
var sprite: AnimatedSprite2D

func _ready() -> void:
	# 맵 위에 얼굴 삽화를 축소해 놓던 구형 표현을 제거하고,
	# 플레이어/동료와 같은 4방향 캐릭터 애니메이션 규격을 사용한다.
	if has_node("Sprite2D"):
		$Sprite2D.queue_free()
	sprite = AnimatedSprite2D.new()
	sprite.name = "CharacterSprite"
	sprite.z_index = 2
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	add_child(sprite)
	_setup_placeholder_sprite()
	print("[NPC] %s ready" % npc_name)

## 플레이어의 RayCast가 호출하는 상호작용 인터페이스
func interact() -> void:
	if DialogueManager.is_active:
		return
	print("[NPC] %s — interact triggered" % npc_name)
	if dialogue_key == "":
		DialogueManager.start_dialogue([
			{"speaker": npc_name, "text": "...", "portrait": ""}
		])
		return

	var talk_flag = "talked_%s_%s" % [npc_name, dialogue_key]
	if _talked_keys.has(dialogue_key) or GameManager.get_flag(talk_flag):
		# 이미 대화한 NPC — 짧은 후속 대사
		var line = GameManager.localized_runtime_text(repeat_line) if repeat_line != "" else "..."
		DialogueManager.start_dialogue([
			{"speaker": npc_name, "text": line, "portrait": ""}
		])
		return

	# 첫 대화 시작 + 완료 시 플래그 설정
	_talked_keys[dialogue_key] = true
	DialogueManager.dialogue_ended.connect(_on_first_talk_ended.bind(talk_flag), CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(dialogue_file, dialogue_key)

func _on_first_talk_ended(talk_flag: String) -> void:
	GameManager.set_flag(talk_flag)

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprite() -> void:
	var config := _get_character_config()
	var sheet_path := "res://assets/sprites/characters/elia_sheet/idle_01.png"
	if npc_name == "Elia" and ResourceLoader.exists(sheet_path):
		sprite.sprite_frames = PixelSprite.create_sheet_frames("elia")
		sprite.position = Vector2(0, 2)
		sprite.offset = Vector2(0, -52)
		sprite.scale = Vector2(0.40, 0.40)
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	else:
		sprite.sprite_frames = PixelSprite.create_frames(config)
		sprite.position = Vector2(0, 2)
		sprite.offset = Vector2.ZERO
		sprite.scale = Vector2.ONE
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.play("idle_down")
	_add_character_grounding(_get_npc_accent_color())
	if npc_name in ["Malet", "Mallet"]:
		_add_malet_details()

func _add_malet_details() -> void:
	# Two readable memory vials and a fine gold chain give Malet a unique map
	# silhouette without returning to the old full portrait-card presentation.
	var chain := Line2D.new()
	chain.width = 1.0
	chain.default_color = Color(0.78, 0.58, 0.24, 0.88)
	chain.points = PackedVector2Array([Vector2(4, 5), Vector2(9, 11), Vector2(12, 18)])
	chain.z_index = 4
	add_child(chain)
	for data in [
		{"x": 9.0, "c": Color(0.50, 0.22, 0.78, 0.95)},
		{"x": 13.0, "c": Color(0.16, 0.55, 0.86, 0.92)},
	]:
		var vial := Polygon2D.new()
		var x := float(data.x)
		vial.polygon = PackedVector2Array([Vector2(x - 1, 11), Vector2(x + 1, 11), Vector2(x + 2, 17), Vector2(x - 2, 17)])
		vial.color = data.c
		vial.z_index = 5
		add_child(vial)

func _get_character_config() -> Dictionary:
	match npc_name:
		"Elia":
			return PixelSprite.elia_config()
		"Sable":
			return PixelSprite.sable_config()
		"Malet", "Mallet":
			return {
				"skin": Color(0.88, 0.78, 0.70),
				"hair": Color(0.08, 0.07, 0.09),
				"hair_style": "medium",
				"coat": Color(0.07, 0.06, 0.10),
				"shirt": Color(0.24, 0.15, 0.31),
				"pants": Color(0.06, 0.05, 0.08),
				"boots": Color(0.03, 0.025, 0.04),
				"eye": Color(0.88, 0.63, 0.18),
				"accessory": Color(0.76, 0.58, 0.24),
				"accessory_type": "brooch",
			}
		"Tobias", "Seric":
			return PixelSprite.npc_scholar_config()
		"Kairos", "Nera", "Handler":
			return PixelSprite.npc_bureau_agent_config()
		"Guard":
			return PixelSprite.npc_guard_config()
		"Old Man":
			return PixelSprite.npc_elder_config()
		"Nervous Trader":
			return PixelSprite.npc_merchant_config()
		"Gardener":
			return PixelSprite.npc_villager_f_config()
		"Ashen Figure", "Prisoner", "Han":
			return PixelSprite.npc_traveler_config()
		_:
			return PixelSprite.npc_config(npc_color)

func _add_character_grounding(accent: Color) -> void:
	var shadow := Polygon2D.new()
	shadow.polygon = PackedVector2Array([
		Vector2(-15, 21), Vector2(-8, 17), Vector2(8, 17), Vector2(15, 21),
		Vector2(8, 24), Vector2(-8, 24)
	])
	shadow.color = Color(0.0, 0.0, 0.0, 0.30)
	shadow.z_index = 0
	add_child(shadow)

	var ring := Line2D.new()
	ring.width = 1.1
	ring.default_color = Color(accent.r, accent.g, accent.b, 0.30)
	ring.points = PackedVector2Array([
		Vector2(-11, 22), Vector2(-6, 25), Vector2(6, 25), Vector2(11, 22)
	])
	ring.z_index = 1
	add_child(ring)

func _get_npc_accent_color() -> Color:
	match npc_name:
		"Malet", "Mallet":
			return Color(0.75, 0.58, 0.28)
		"Sable":
			return Color(0.72, 0.62, 0.80)
		"Tobias":
			return Color(0.68, 0.58, 0.50)
		"Kairos":
			return Color(0.50, 0.74, 0.62)
		"Nera":
			return Color(0.62, 0.70, 0.82)
		"Seric":
			return Color(0.74, 0.68, 0.54)
		_:
			return Color(0.70, 0.56, 0.34)
