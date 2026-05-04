## DialogueBox (Autoload)
## 대화 UI 표시. DialogueManager 시그널을 받아서 화면에 렌더링.
## 하단 텍스트 박스 + 좌측 포트레이트 + 선택지.
extends CanvasLayer

const TYPEWRITER_SPEEDS: Dictionary = {1: 0.06, 2: 0.045, 3: 0.03, 4: 0.015, 5: 0.0}
const BOX_HEIGHT: int = 160
const PORTRAIT_SIZE: int = 96

# 포트레이트 이미지 매핑 (portrait 키 → 파일 경로)
const PORTRAIT_MAP: Dictionary = {
	"arrel_neutral": "res://assets/portraits/arrel_neutral.jpg",
	"arrel_side": "res://assets/portraits/arrel_side.jpg",
	"arrel_wounded": "res://assets/portraits/arrel_wounded.jpg",
	"arrel_angry": "res://assets/portraits/arrel_angry.jpg",
	"arrel_pain": "res://assets/portraits/arrel_pain.jpg",
	"arrel_determined": "res://assets/portraits/arrel_determined.jpg",
	"arrel_sad": "res://assets/portraits/arrel_sad.jpg",
	"arrel_cold": "res://assets/portraits/arrel_cold.jpg",
	"arrel_rage": "res://assets/portraits/arrel_rage.jpg",
	"arrel_pensive": "res://assets/portraits/arrel_pensive.jpg",
	"arrel_battle": "res://assets/portraits/arrel_battle.png",
	"elia_neutral": "res://assets/portraits/elia_neutral.jpg",
	"elia_concern": "res://assets/portraits/elia_concern.jpg",
	"elia_hopeful": "res://assets/portraits/elia_hopeful.jpg",
	"elia_sad": "res://assets/portraits/elia_sad.jpg",
	"elia_determined": "res://assets/portraits/elia_determined.jpg",
	"elia_calm": "res://assets/portraits/elia_calm.jpg",
	"elia_side": "res://assets/portraits/elia_side.jpg",
	"elia_void": "res://assets/portraits/elia_void.jpg",
	"malet_neutral": "res://assets/portraits/malet_neutral.jpg",
	"malet_desk": "res://assets/portraits/malet_desk.jpg",
	"kairos_neutral": "res://assets/portraits/kairos_neutral.jpg",
	"sable_neutral": "res://assets/portraits/sable_neutral.jpg",
	"sable_calm": "res://assets/portraits/sable_calm.jpg",
	"nera_neutral": "res://assets/portraits/nera_neutral.jpg",
	"seric_neutral": "res://assets/portraits/seric_neutral.jpg",
	"tobias_neutral": "res://assets/portraits/tobias_neutral.jpg",
	# S47: 신규 포트레이트 19장
	"arrel_default2": "res://assets/portraits/arrel_default2.jpg",
	"arrel_cold2": "res://assets/portraits/arrel_cold2.jpg",
	"arrel_heroic": "res://assets/portraits/arrel_heroic.png",
	"arrel_wounded2": "res://assets/portraits/arrel_wounded2.jpg",
	"arrel_burn": "res://assets/portraits/arrel_burn.jpg",
	"arrel_exhausted": "res://assets/portraits/arrel_exhausted.jpg",
	"elia_wind": "res://assets/portraits/elia_wind.jpg",
	"elia_default2": "res://assets/portraits/elia_default2.jpg",
	"elia_void2": "res://assets/portraits/elia_void2.jpg",
	"elia_calm2": "res://assets/portraits/elia_calm2.jpg",
	"elia_wind2": "res://assets/portraits/elia_wind2.jpg",
	"elia_mature": "res://assets/portraits/elia_mature.jpg",
	"nera_bureau": "res://assets/portraits/nera_bureau.jpg",
	"malet_smirk": "res://assets/portraits/malet_smirk.jpg",
	"malet_casual": "res://assets/portraits/malet_casual.jpg",
	"seric_clipboard": "res://assets/portraits/seric_clipboard.jpg",
	"sable_portrait": "res://assets/portraits/sable_neutral.jpg",
	"kairos_portrait": "res://assets/portraits/kairos_neutral.jpg",
	"tobias_uniform": "res://assets/portraits/tobias_uniform.jpg",
	# S53: 카이로스/토비아스 추가 포트레이트
	"kairos_cold": "res://assets/portraits/kairos_cold.jpg",
	"kairos_amused": "res://assets/portraits/kairos_amused.jpg",
	"tobias_concerned": "res://assets/portraits/tobias_concerned.jpg",
}
const DEFAULT_PORTRAITS: Dictionary = {
	"Arrel": "arrel_neutral",
	"Elia": "elia_neutral",
	"Malet": "malet_neutral",
	"Kairos": "kairos_neutral",
	"Sable": "sable_neutral",
	"Nera": "nera_neutral",
	"Seric": "seric_neutral",
	"Tobias": "tobias_neutral",
}

# UI 노드 (코드로 생성)
var panel: PanelContainer
var portrait_texture: TextureRect  # 실제 ��미지 표시
var portrait_fallback: ColorRect   # 이미지 없을 때 fallback
var portrait_label: Label          # fallback용 이니셜
var speaker_label: Label
var text_label: RichTextLabel
var choice_container: VBoxContainer
var indicator: Label  # ▼ 다음 대사 표시

var full_text: String = ""
var displayed_chars: int = 0
var is_typing: bool = false
var typewriter_timer: float = 0.0
var _ui_time: float = 0.0

func _ready() -> void:
	layer = 50  # SceneTransition(100)보다 아래, 게임 위
	_build_ui()
	_connect_signals()
	hide_box()
	print("[DialogueBox] Ready")

func _get_typewriter_speed() -> float:
	var spd = OptionsMenu.settings.get("text_speed", 3) if OptionsMenu else 3
	return TYPEWRITER_SPEEDS.get(spd, 0.03)

func _process(delta: float) -> void:
	_ui_time += delta
	if indicator and indicator.visible:
		indicator.modulate.a = 0.45 + sin(_ui_time * 4.0) * 0.25
	if is_typing:
		var speed = _get_typewriter_speed()
		if speed <= 0.0:
			# Instant mode
			displayed_chars = full_text.length()
			text_label.text = full_text
			is_typing = false
			indicator.visible = true
			return
		typewriter_timer += delta
		while typewriter_timer >= speed and displayed_chars < full_text.length():
			typewriter_timer -= speed
			displayed_chars += 1
			text_label.text = full_text.substr(0, displayed_chars)

		if displayed_chars >= full_text.length():
			is_typing = false
			indicator.visible = true

## UI 구조 코드 생성
func _build_ui() -> void:
	# 전체 컨테이너 (화면 하단)
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# 패널 (하단 대화 박스)
	panel = PanelContainer.new()
	panel.anchor_left = 0.0
	panel.anchor_right = 1.0
	panel.anchor_top = 1.0
	panel.anchor_bottom = 1.0
	panel.offset_top = -BOX_HEIGHT
	panel.offset_bottom = 0
	panel.offset_left = 16
	panel.offset_right = -16

	# 스타일 (어두운 반투명 — 서고 모티프)
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.07, 0.1, 0.92)
	style.border_color = Color(0.3, 0.25, 0.2, 0.8)
	style.set_border_width_all(2)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(12)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	# 내부 HBox (포트레이트 | 텍스트 영역)
	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 16)
	panel.add_child(hbox)

	# 포트레이트 영역
	var portrait_container = VBoxContainer.new()
	portrait_container.custom_minimum_size = Vector2(PORTRAIT_SIZE, 0)
	hbox.add_child(portrait_container)

	# 실제 이미지 (TextureRect)
	portrait_texture = TextureRect.new()
	portrait_texture.custom_minimum_size = Vector2(PORTRAIT_SIZE, PORTRAIT_SIZE)
	portrait_texture.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	portrait_texture.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	portrait_texture.visible = false
	portrait_container.add_child(portrait_texture)

	# fallback (ColorRect + 이니셜, 이미지 없을 때)
	portrait_fallback = ColorRect.new()
	portrait_fallback.custom_minimum_size = Vector2(PORTRAIT_SIZE, PORTRAIT_SIZE)
	portrait_fallback.color = Color(0.2, 0.2, 0.25)
	portrait_fallback.visible = false
	portrait_container.add_child(portrait_fallback)

	portrait_label = Label.new()
	portrait_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	portrait_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	portrait_label.add_theme_font_size_override("font_size", 32)
	portrait_label.add_theme_color_override("font_color", Color(0.7, 0.6, 0.5))
	portrait_label.set_anchors_preset(Control.PRESET_FULL_RECT)
	portrait_fallback.add_child(portrait_label)

	# 텍스트 영역 (VBox: 이름 + 대사)
	var text_area = VBoxContainer.new()
	text_area.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_area.add_theme_constant_override("separation", 4)
	hbox.add_child(text_area)

	# 화자 이름
	speaker_label = Label.new()
	speaker_label.add_theme_font_size_override("font_size", 14)
	speaker_label.add_theme_color_override("font_color", Color(0.75, 0.6, 0.4))
	text_area.add_child(speaker_label)

	# 대사 텍스트
	text_label = RichTextLabel.new()
	text_label.bbcode_enabled = false
	text_label.fit_content = false
	text_label.scroll_active = false
	text_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	text_label.add_theme_font_size_override("normal_font_size", 16)
	text_label.add_theme_color_override("default_color", Color(0.85, 0.82, 0.78))
	text_area.add_child(text_label)

	# 다음 대사 표시기 (▼)
	indicator = Label.new()
	indicator.text = "▼"
	indicator.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	indicator.add_theme_font_size_override("font_size", 12)
	indicator.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4, 0.7))
	indicator.visible = false
	text_area.add_child(indicator)

	# 선택지 컨테이너 (대화 박스 위에 표시)
	choice_container = VBoxContainer.new()
	choice_container.anchor_left = 0.3
	choice_container.anchor_right = 0.7
	choice_container.anchor_top = 1.0
	choice_container.anchor_bottom = 1.0
	choice_container.offset_top = -(BOX_HEIGHT + 20)
	choice_container.offset_bottom = -(BOX_HEIGHT + 4)
	choice_container.add_theme_constant_override("separation", 4)
	choice_container.visible = false
	root.add_child(choice_container)

## DialogueManager 시그널 연결
func _connect_signals() -> void:
	DialogueManager.dialogue_started.connect(_on_dialogue_started)
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	DialogueManager.dialogue_choice.connect(_on_dialogue_choice)
	DialogueManager.dialogue_ended.connect(_on_dialogue_ended)

func _on_dialogue_started() -> void:
	show_box()

func _on_dialogue_line(speaker: String, text: String, portrait: String) -> void:
	_pulse_dialogue_panel()
	_clear_choices()

	# 화자 이름 (나레이션이면 숨김)
	if speaker == "" or speaker == "system_log":
		speaker_label.text = ""
		portrait_texture.visible = false
		portrait_fallback.visible = false
		# 나레이션 스타일
		text_label.add_theme_color_override("default_color", UITheme.TEXT_NARRATION)
		if speaker == "system_log":
			text_label.add_theme_color_override("default_color", UITheme.TEXT_SYSTEM)
	else:
		speaker_label.text = speaker
		speaker_label.add_theme_color_override("font_color", UITheme.get_speaker_color(speaker))
		text_label.add_theme_color_override("default_color", UITheme.TEXT_PRIMARY)
		_update_portrait(speaker, portrait)

	# 타자기 효과로 텍스트 표시
	full_text = text
	displayed_chars = 0
	text_label.text = ""
	is_typing = true
	typewriter_timer = 0.0
	indicator.visible = false

func _on_dialogue_choice(choices: Array) -> void:
	_clear_choices()
	choice_container.visible = true

	for i in range(choices.size()):
		var choice = choices[i]
		var btn = Button.new()
		btn.text = choice.get("text", "...")
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

		# 선택지 스타일
		var btn_style = StyleBoxFlat.new()
		btn_style.bg_color = Color(0.12, 0.1, 0.15, 0.9)
		btn_style.border_color = Color(0.35, 0.3, 0.25, 0.6)
		btn_style.set_border_width_all(1)
		btn_style.set_corner_radius_all(2)
		btn_style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", btn_style)

		var hover_style = btn_style.duplicate()
		hover_style.bg_color = Color(0.18, 0.15, 0.22, 0.95)
		hover_style.border_color = Color(0.75, 0.6, 0.4, 0.8)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)

		btn.add_theme_color_override("font_color", Color(0.8, 0.75, 0.7))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.85, 0.6))
		btn.add_theme_font_size_override("font_size", 14)

		var idx = i  # 클로저 캡처용
		btn.pressed.connect(func():
			AudioManager.play_sfx("ui_select")
			_on_choice_selected(idx)
		)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		choice_container.add_child(btn)

	# 첫 번째 버튼에 포커스
	if choice_container.get_child_count() > 0:
		choice_container.get_child(0).grab_focus()

func _on_dialogue_ended() -> void:
	hide_box()

func _on_choice_selected(index: int) -> void:
	_clear_choices()
	DialogueManager.select_choice(index)

## 포트레이트 업데이트 — 이미지가 있으면 표시, 없으면 fallback
func _update_portrait(speaker: String, portrait_key: String = "") -> void:
	# portrait 키 결정: 명시적 키 > 화자별 기본값
	var key = portrait_key
	if key == "" and DEFAULT_PORTRAITS.has(speaker):
		key = DEFAULT_PORTRAITS[speaker]

	# 이미지 로드 시도
	if key != "" and PORTRAIT_MAP.has(key):
		var path = PORTRAIT_MAP[key]
		if ResourceLoader.exists(path):
			var tex = load(path)
			portrait_texture.texture = tex
			portrait_texture.visible = true
			portrait_fallback.visible = false
			return

	# fallback: ColorRect + 이니셜
	var colors = {
		"Arrel": Color(0.2, 0.25, 0.4),
		"Elia": Color(0.45, 0.55, 0.65),
	}
	portrait_fallback.color = colors.get(speaker, Color(0.25, 0.25, 0.3))
	portrait_label.text = speaker.substr(0, 1).to_upper() if speaker.length() > 0 else "?"
	portrait_texture.visible = false
	portrait_fallback.visible = true

## 선택지 정리
func _clear_choices() -> void:
	for child in choice_container.get_children():
		child.queue_free()
	choice_container.visible = false

## 박스 표시/숨김
func show_box() -> void:
	panel.visible = true
	# S53: 대화 박스 슬라이드 업 애니메이션
	var original_top = panel.offset_top
	panel.offset_top = 0  # 화면 아래에서 시작
	panel.modulate.a = 0.0
	var tween = create_tween().set_parallel(true)
	tween.tween_property(panel, "offset_top", original_top, 0.25).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tween.tween_property(panel, "modulate:a", 1.0, 0.2).set_ease(Tween.EASE_OUT)

func hide_box() -> void:
	# S53: 대화 박스 슬라이드 다운 애니메이션
	choice_container.visible = false
	is_typing = false
	if panel.visible:
		var tween = create_tween().set_parallel(true)
		tween.tween_property(panel, "offset_top", 0, 0.15).set_ease(Tween.EASE_IN)
		tween.tween_property(panel, "modulate:a", 0.0, 0.15)
		tween.chain().tween_callback(func():
			panel.visible = false
			panel.offset_top = -BOX_HEIGHT
			panel.modulate.a = 1.0
		)
	else:
		panel.visible = false

## 입력 처리 (타자기 스킵 / 대사 넘기기)
func _unhandled_input(event: InputEvent) -> void:
	if not DialogueManager.is_active:
		return

	if event.is_action_pressed("interact"):
		if is_typing:
			# 타자기 진행 중 → 즉시 전체 표시
			displayed_chars = full_text.length()
			text_label.text = full_text
			is_typing = false
			indicator.visible = true
			get_viewport().set_input_as_handled()
		elif not choice_container.visible:
			# 다음 대사로
			AudioManager.play_sfx("confirm")
			DialogueManager.advance()
			get_viewport().set_input_as_handled()


func _pulse_dialogue_panel() -> void:
	if panel == null:
		return
	panel.modulate = Color(1, 1, 1, 0.9)
	panel.scale = Vector2(0.995, 0.995)
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(panel, "modulate:a", 1.0, 0.2)
	tw.tween_property(panel, "scale", Vector2.ONE, 0.2)
