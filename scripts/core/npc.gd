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

@onready var sprite: Sprite2D = $Sprite2D

func _ready() -> void:
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
		var line = repeat_line if repeat_line != "" else "..."
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
	var config: Dictionary
	# 이름별 전용 config, 없으면 npc_color 기반 자동 생성
	match npc_name:
		"Sable":
			config = PixelSprite.sable_config()
		"Malet":
			config = PixelSprite.npc_config(Color(0.35, 0.32, 0.3))
			config.eye = Color(0.8, 0.6, 0.2)  # 앰버 눈 (기억 앰플)
		_:
			config = PixelSprite.npc_config(npc_color)

	# 정면 idle 프레임만 사용 (정적 NPC)
	var frames = PixelSprite.create_frames(config)
	var idle_tex = frames.get_frame_texture("idle_down", 0)
	sprite.texture = idle_tex
