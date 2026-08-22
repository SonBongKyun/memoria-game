## NPC, 범용 NPC 스크립트
## StaticBody2D 기반. interact() 호출 시 대화 시작.
extends StaticBody2D

const SPRITE_SIZE: int = 48  # S42: 48x48 업그레이드

@export var npc_name: String = "NPC"
@export var dialogue_file: String = "res://data/chapter1_dialogue.json"
@export var dialogue_key: String = ""
@export var npc_color: Color = Color(0.6, 0.3, 0.35)  # 기본: 붉은 톤
@export var repeat_line: String = ""  # 재대화 시 표시할 대사 (빈칸이면 기본 대사)
@export var repeat_dialogue_key: String = ""  # opt-in authored 재대화 (빈칸이면 기존 한 줄 대사)
@export var display_name_ko: String = ""  # 월드 인구용 로컬라이즈 이름
@export var repeat_line_ko: String = ""  # 월드 인구용 로컬라이즈 대사
@export_file("*.png") var field_art_path: String = ""  # 단방향 월드 NPC 전용 필드 아트

var _talked_keys: Dictionary = {}  # 이미 진행한 dialogue_key 추적
var sprite: AnimatedSprite2D

func _ready() -> void:
	add_to_group("npcs")
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
	_face_toward_player()
	print("[NPC] %s, interact triggered" % npc_name)

	# S226: 연소한 기억에 대한 일회성 반응이 우선한다.
	var burn_reaction := PerceptionFilter.take_burn_reaction(self, dialogue_file)
	if not burn_reaction.is_empty():
		DialogueManager.load_and_start(String(burn_reaction.file), String(burn_reaction.key))
		return

	if dialogue_key == "":
		var ambient_line := _get_runtime_line()
		DialogueManager.start_dialogue([
			{"speaker": _get_runtime_name(), "text": ambient_line, "portrait": ""}
		])
		return

	var talk_flag = "talked_%s_%s" % [npc_name, dialogue_key]
	if _talked_keys.has(dialogue_key) or GameManager.get_flag(talk_flag):
		# 이미 대화한 NPC. 명시적으로 authored 재대화를 지정한 NPC만
		# DialogueManager를 다시 사용하고, 나머지는 기존 한 줄 동작을 유지한다.
		if repeat_dialogue_key != "":
			DialogueManager.load_and_start(dialogue_file, repeat_dialogue_key)
			return
		var line := _get_runtime_line()
		DialogueManager.start_dialogue([
			{"speaker": _get_runtime_name(), "text": line, "portrait": ""}
		])
		return

	# 첫 대화 시작 + 완료 시 플래그 설정
	_talked_keys[dialogue_key] = true
	DialogueManager.dialogue_ended.connect(_on_first_talk_ended.bind(talk_flag), CONNECT_ONE_SHOT)
	DialogueManager.load_and_start(dialogue_file, dialogue_key)

func _on_first_talk_ended(talk_flag: String) -> void:
	GameManager.set_flag(talk_flag)

func _get_runtime_name() -> String:
	if GameManager.current_locale == "ko" and display_name_ko != "":
		return display_name_ko
	return npc_name

func _get_runtime_line() -> String:
	if GameManager.current_locale == "ko" and repeat_line_ko != "":
		return repeat_line_ko
	return GameManager.localized_runtime_text(repeat_line) if repeat_line != "" else "..."

## PixelSprite 유틸리티로 상세한 픽셀아트 스프라이트 생성
func _setup_placeholder_sprite() -> void:
	var config := _get_character_config()
	if field_art_path != "" and ResourceLoader.exists(field_art_path):
		sprite.sprite_frames = _create_static_field_frames(field_art_path)
		var field_texture := sprite.sprite_frames.get_frame_texture("idle_down", 0)
		PixelSprite.apply_field_profile(sprite, field_texture, _field_target_height())
		sprite.play("idle_down")
		_add_character_grounding(_get_npc_accent_color())
		return
	var sheet_keys := {
		"Elia": "elia",
		"Malet": "malet",
		"Mallet": "malet",
		"Tobias": "tobias",
		"Kairos": "kairos",
		"Nera": "nera",
		"Veil": "veil",
		"Sable": "sable",
	}
	var sheet_key: String = sheet_keys.get(npc_name, "")
	var sheet_path := "res://assets/sprites/characters/%s_sheet/idle_01.png" % sheet_key
	var uses_field_sprite := sheet_key != "" and PixelSprite.has_field_sprite_frames(sheet_key)
	var uses_authored_sheet := uses_field_sprite or (sheet_key != "" and ResourceLoader.exists(sheet_path))
	if uses_authored_sheet:
		sprite.sprite_frames = PixelSprite.create_sheet_frames(sheet_key)
		# Purpose-built field frames share one 128x160 canvas and foot baseline.
		# Legacy boards preserve their old normalization as a fallback only.
		if uses_field_sprite:
			var field_texture := sprite.sprite_frames.get_frame_texture("idle_down", 0)
			PixelSprite.apply_field_profile(sprite, field_texture, _field_target_height())
		else:
			sprite.position = Vector2(0, 2)
			var authored_scale := 0.40 if sheet_key == "elia" else 0.32
			var authored_offset := -52.0 if sheet_key == "elia" else -65.0
			sprite.offset = Vector2(0, authored_offset)
			sprite.scale = Vector2(authored_scale, authored_scale)
			sprite.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	else:
		sprite.sprite_frames = PixelSprite.create_frames(config)
		sprite.position = Vector2(0, 2)
		sprite.offset = Vector2.ZERO
		sprite.scale = Vector2.ONE
		sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.play("idle_down")
	_add_character_grounding(_get_npc_accent_color())
	if npc_name in ["Malet", "Mallet"] and not uses_authored_sheet:
		_add_malet_details()

## 단일 삽화 NPC의 네 "방향"은 모두 같은 그림이다.
## S235: 그 사실을 표시해 둔다. 자세를 바꿔도 화면이 달라지지 않는 대상이므로,
## 몸을 돌릴 때 좌우 반전을 써야 한다는 것을 _apply_facing이 이 표식으로 안다.
func _create_static_field_frames(art_path: String) -> SpriteFrames:
	var frames := SpriteFrames.new()
	var texture := load(art_path) as Texture2D
	for direction in ["down", "up", "left", "right"]:
		var animation: String = "idle_" + String(direction)
		frames.add_animation(animation)
		frames.set_animation_speed(animation, 1.0)
		frames.set_animation_loop(animation, true)
		frames.add_frame(animation, texture)
	if sprite != null:
		sprite.set_meta("field_static_plate", true)
	return frames

func _field_target_height() -> float:
	var lowered := npc_name.to_lower()
	return PixelSprite.FIELD_CHILD_HEIGHT if "child" in lowered else PixelSprite.FIELD_ADULT_HEIGHT

func _face_toward_player() -> void:
	if sprite == null or sprite.sprite_frames == null:
		return
	var player := get_tree().get_first_node_in_group("player") as Node2D
	if player == null:
		return
	var direction := player.global_position - global_position
	var suffix := "down"
	if absf(direction.x) > absf(direction.y):
		suffix = "right" if direction.x > 0.0 else "left"
	else:
		suffix = "down" if direction.y > 0.0 else "up"
	var animation := "idle_" + suffix
	if sprite.sprite_frames.has_animation(animation):
		sprite.play(animation)

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
	FieldActorVisuals.apply_finish(sprite, accent, 0.68, 0.085)
	FieldActorVisuals.add_grounding(self, accent)
	_awaken_presence()

## ===================== S235: 서 있는 사람도 살아 있어야 한다 =====================
##
## 월드 인구 NPC는 단일 정지 이미지였다. 실측하면 3초 동안 5명 전원이 0.00px 움직였고,
## 호흡도 무게 이동도 없었다. 시장 배회 NPC만 움직이고 나머지 19개 맵의 주민들은
## 그대로 굳어 있었다. 걸어 다니게 만드는 것이 아니라, 서 있는 자세에 숨을 넣는다.
##   - 이미 있는 ambient 드라이버가 호흡/무게/접지 보정을 해 준다 (배회 NPC와 같은 문법).
##   - 플레이어가 가까이 오면 몸을 돌린다. 지나가면 원래 방향으로 돌아온다.
const PRESENCE_NOTICE_RADIUS: float = 96.0
const PRESENCE_RELEASE_RADIUS: float = 132.0  # 경계에서 깜빡이지 않도록 이력을 둔다

var _presence_driver: FieldActorVisuals = null
var _base_facing: String = "down"
var _facing_player: bool = false
var _presence_scan: float = 0.0

func _awaken_presence() -> void:
	if sprite == null:
		return
	_base_facing = _suffix_from_animation(String(sprite.animation))
	_presence_driver = FieldActorVisuals.attach_ambient_driver(sprite)
	set_process(true)

func _process(delta: float) -> void:
	if sprite == null or not is_instance_valid(sprite):
		return
	if GameManager.current_state != GameManager.GameState.EXPLORATION:
		return
	# 매 프레임 플레이어를 찾을 필요는 없다. 사람이 알아채는 속도면 충분하다.
	_presence_scan -= delta
	if _presence_scan > 0.0:
		return
	_presence_scan = 0.2

	var player := get_tree().get_first_node_in_group("player") as Node2D
	if player == null:
		return
	var distance := global_position.distance_to(player.global_position)
	if _facing_player and distance > PRESENCE_RELEASE_RADIUS:
		_facing_player = false
		_apply_facing(_base_facing)
	elif not _facing_player and distance <= PRESENCE_NOTICE_RADIUS:
		_facing_player = true
		_apply_facing(_suffix_toward(player.global_position))
	elif _facing_player:
		_apply_facing(_suffix_toward(player.global_position))

func _suffix_toward(point: Vector2) -> String:
	var direction := point - global_position
	if absf(direction.x) > absf(direction.y):
		return "right" if direction.x > 0.0 else "left"
	return "down" if direction.y > 0.0 else "up"

func _suffix_from_animation(animation: String) -> String:
	var parts := animation.split("_")
	return String(parts[parts.size() - 1]) if parts.size() > 1 else "down"

## 네 방향 시트를 가진 NPC는 자세를 바꾸고, 단일 삽화 NPC는 좌우 반전으로 몸을 돌린다.
## 정지 삽화에 없는 방향을 지어내지 않으면서도 "알아챘다"는 신호는 남긴다.
func _apply_facing(suffix: String) -> void:
	if sprite == null or sprite.sprite_frames == null:
		return
	var animation := "idle_" + suffix
	var distinct_poses: bool = not bool(sprite.get_meta("field_static_plate", false))
	if distinct_poses and sprite.sprite_frames.has_animation(animation):
		if sprite.animation != animation:
			sprite.play(animation)
		sprite.flip_h = false
		return
	if suffix == "left" or suffix == "right":
		sprite.flip_h = suffix == "left"

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
