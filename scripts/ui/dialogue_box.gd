## DialogueBox (Autoload)
## 대화 UI 표시. DialogueManager 시그널을 받아서 화면에 렌더링.
## 하단 텍스트 박스 + 좌측 포트레이트 + 선택지.
## S55: Letterbox, screen tint, text emphasis (*bold*), narration sound, auto-advance.
extends CanvasLayer

const TYPEWRITER_SPEEDS: Dictionary = {1: 0.06, 2: 0.045, 3: 0.03, 4: 0.015, 5: 0.0}
const BOX_HEIGHT: int = 160
const PORTRAIT_SIZE: int = 96

# 포트레이트 이미지 매핑 (portrait 키 -> 파일 경로)
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
var portrait_texture: TextureRect
var portrait_fallback: ColorRect
var portrait_label: Label
var speaker_label: Label
var text_label: RichTextLabel
var choice_container: VBoxContainer
var indicator: Label  # ▼ 다음 대사 표시

var full_text: String = ""
var displayed_chars: int = 0
var is_typing: bool = false
var typewriter_timer: float = 0.0

# S54: Character text blip SFX (Undertale style)
var _blip_player: AudioStreamPlayer = null
var _blip_char_count: int = 0
var _blip_stream: AudioStreamWAV = null

# S54: Portrait transition tracking
var _current_portrait_key: String = ""
var _portrait_tween: Tween

# S54: Dialogue camera effects
var _cam_tween: Tween
var _cam_original_zoom: Vector2 = Vector2(1.0, 1.0)
var _cam_original_offset: Vector2 = Vector2.ZERO
var _cam_initialized: bool = false
const BLIP_PITCH_MAP: Dictionary = {
	"Arrel": 1.0,
	"Elia": 1.3,
	"Sable": 0.7,
	"Tobias": 0.9,
	"Kairos": 0.6,
	"Malet": 0.8,
}
const BLIP_INTERVAL: int = 2
var _current_blip_pitch: float = 1.0

# S54: Dialogue direction tags
var _line_speed_override: float = -1.0
var _line_shake: bool = false
var _line_pause_time: float = 0.0
var _pause_timer: float = 0.0
var _is_paused: bool = false

# S55: Letterbox bars
var _letterbox_top: ColorRect = null
var _letterbox_bottom: ColorRect = null
var _letterbox_active: bool = false
var _letterbox_tween: Tween
const LETTERBOX_HEIGHT: int = 60

# S55: Screen tint overlay
var _tint_overlay: ColorRect = null
var _tint_tween: Tween

# S55: Narration page-turn SFX player
var _narration_sfx_player: AudioStreamPlayer = null
var _narration_sfx_stream: AudioStreamWAV = null

# S55: Auto-advance for narration
var _auto_advance_timer: float = 0.0
var _auto_advance_active: bool = false
const AUTO_ADVANCE_DELAY: float = 3.0

# S55: Track current speaker for auto-advance
var _current_speaker: String = ""

# S55: BBCode text for emphasis rendering
var _bbcode_text: String = ""

func _ready() -> void:
	layer = 50
	_build_ui()
	_build_letterbox()
	_build_tint_overlay()
	_setup_blip_player()
	_setup_narration_sfx()
	_connect_signals()
	hide_box()
	print("[DialogueBox] Ready")

## S54: Generate blip sound (simple square wave) and create AudioStreamPlayer
func _setup_blip_player() -> void:
	_blip_player = AudioStreamPlayer.new()
	_blip_player.bus = "Master"
	add_child(_blip_player)
	_blip_stream = AudioStreamWAV.new()
	_blip_stream.format = AudioStreamWAV.FORMAT_8_BITS
	_blip_stream.mix_rate = 22050
	_blip_stream.stereo = false
	var sample_count: int = int(22050 * 0.04)
	var data: PackedByteArray = PackedByteArray()
	data.resize(sample_count)
	var freq: float = 440.0
	for i in range(sample_count):
		var t: float = float(i) / 22050.0
		var val: float = 1.0 if fmod(t * freq, 1.0) < 0.5 else -1.0
		var env: float = 1.0 - float(i) / float(sample_count)
		val *= env * 0.3
		data[i] = int((val * 0.5 + 0.5) * 255.0)
	_blip_stream.data = data
	_blip_player.stream = _blip_stream

func _play_blip() -> void:
	if _blip_player == null or _blip_stream == null:
		return
	var sfx_vol: float = OptionsMenu.settings.get("sfx_volume", 80) / 100.0 if OptionsMenu else 0.8
	_blip_player.volume_db = linear_to_db(sfx_vol * 0.4)
	_blip_player.pitch_scale = _current_blip_pitch + randf_range(-0.05, 0.05)
	_blip_player.play()

## S55: Narration page-turn SFX (soft crinkle/rustle)
func _setup_narration_sfx() -> void:
	_narration_sfx_player = AudioStreamPlayer.new()
	_narration_sfx_player.bus = "Master"
	add_child(_narration_sfx_player)

	# Generate a short paper rustle sound (~120ms of filtered noise with envelope)
	var sample_rate: int = 22050
	var duration: float = 0.12
	var count: int = int(sample_rate * duration)
	var samples := PackedFloat32Array()
	samples.resize(count)
	var prev: float = 0.0
	for i in range(count):
		var t: float = float(i) / sample_rate
		var raw: float = randf_range(-1.0, 1.0)
		# Bandpass-ish filter: high alpha for crinkle texture
		prev = prev + 0.15 * (raw - prev)
		# Envelope: quick attack, medium decay
		var env: float = 0.0
		var norm_t: float = t / duration
		if norm_t < 0.1:
			env = norm_t / 0.1
		else:
			env = (1.0 - norm_t) / 0.9
		samples[i] = prev * env * 0.25  # quiet

	var byte_data := PackedByteArray()
	for s in samples:
		var val: int = int(clampf(s, -1.0, 1.0) * 32767)
		byte_data.append(val & 0xFF)
		byte_data.append((val >> 8) & 0xFF)

	_narration_sfx_stream = AudioStreamWAV.new()
	_narration_sfx_stream.data = byte_data
	_narration_sfx_stream.format = AudioStreamWAV.FORMAT_16_BITS
	_narration_sfx_stream.mix_rate = sample_rate
	_narration_sfx_stream.stereo = false
	_narration_sfx_player.stream = _narration_sfx_stream

func _play_narration_sfx() -> void:
	if _narration_sfx_player == null or _narration_sfx_stream == null:
		return
	var sfx_vol: float = OptionsMenu.settings.get("sfx_volume", 80) / 100.0 if OptionsMenu else 0.8
	_narration_sfx_player.volume_db = linear_to_db(sfx_vol * 0.3)
	_narration_sfx_player.pitch_scale = randf_range(0.9, 1.1)
	_narration_sfx_player.play()

func _get_typewriter_speed() -> float:
	var spd = OptionsMenu.settings.get("text_speed", 3) if OptionsMenu else 3
	return TYPEWRITER_SPEEDS.get(spd, 0.03)

func _process(delta: float) -> void:
	# S54: Handle pause tag
	if _is_paused:
		_pause_timer -= delta
		if _pause_timer <= 0.0:
			_is_paused = false
		return

	if is_typing:
		var speed = _line_speed_override if _line_speed_override >= 0.0 else _get_typewriter_speed()
		if speed <= 0.0:
			# Instant mode
			displayed_chars = _bbcode_text.length()
			text_label.text = _bbcode_text
			is_typing = false
			indicator.visible = true
			_line_shake = false
			_start_auto_advance_if_narration()
			return
		typewriter_timer += delta
		while typewriter_timer >= speed and displayed_chars < full_text.length():
			typewriter_timer -= speed
			displayed_chars += 1
			# Update BBCode text up to displayed_chars
			text_label.text = _build_visible_bbcode(displayed_chars)

		if displayed_chars >= full_text.length():
			is_typing = false
			text_label.text = _bbcode_text  # show full formatted text
			indicator.visible = true
			_line_shake = false
			_start_auto_advance_if_narration()

	# S55: Auto-advance timer for narration lines
	if _auto_advance_active:
		_auto_advance_timer -= delta
		if _auto_advance_timer <= 0.0:
			_auto_advance_active = false
			if not choice_container.visible:
				DialogueManager.advance()

	# S54: Screen shake effect during dialogue
	if _line_shake and is_typing:
		# S55: Respect screen_shake setting
		var shake_enabled: bool = OptionsMenu.settings.get("screen_shake", true) if OptionsMenu else true
		if shake_enabled:
			var viewport = get_viewport()
			if viewport:
				viewport.canvas_transform.origin = Vector2(randf_range(-1.5, 1.5), randf_range(-1.5, 1.5))
	elif not _line_shake:
		var viewport = get_viewport()
		if viewport and not is_typing:
			viewport.canvas_transform.origin = Vector2.ZERO

## S55: Start auto-advance countdown if the current line is narration (no speaker)
func _start_auto_advance_if_narration() -> void:
	if _current_speaker == "" and not choice_container.visible:
		# Check if auto-advance is enabled in options
		var auto_adv: bool = OptionsMenu.settings.get("auto_advance_narration", true) if OptionsMenu else true
		if auto_adv:
			_auto_advance_active = true
			_auto_advance_timer = AUTO_ADVANCE_DELAY

## ===================== S55: LETTERBOX =====================

func _build_letterbox() -> void:
	_letterbox_top = ColorRect.new()
	_letterbox_top.color = Color(0, 0, 0, 1)
	_letterbox_top.anchor_left = 0.0
	_letterbox_top.anchor_right = 1.0
	_letterbox_top.anchor_top = 0.0
	_letterbox_top.anchor_bottom = 0.0
	_letterbox_top.offset_top = -LETTERBOX_HEIGHT
	_letterbox_top.offset_bottom = 0
	_letterbox_top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_letterbox_top.z_index = -1
	add_child(_letterbox_top)

	_letterbox_bottom = ColorRect.new()
	_letterbox_bottom.color = Color(0, 0, 0, 1)
	_letterbox_bottom.anchor_left = 0.0
	_letterbox_bottom.anchor_right = 1.0
	_letterbox_bottom.anchor_top = 1.0
	_letterbox_bottom.anchor_bottom = 1.0
	_letterbox_bottom.offset_top = 0
	_letterbox_bottom.offset_bottom = LETTERBOX_HEIGHT
	_letterbox_bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_letterbox_bottom.z_index = -1
	add_child(_letterbox_bottom)

func _show_letterbox() -> void:
	if _letterbox_active:
		return
	_letterbox_active = true
	if _letterbox_tween:
		_letterbox_tween.kill()
	_letterbox_tween = create_tween().set_parallel(true)
	_letterbox_tween.tween_property(_letterbox_top, "offset_top", 0, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	_letterbox_tween.tween_property(_letterbox_top, "offset_bottom", LETTERBOX_HEIGHT, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	_letterbox_tween.tween_property(_letterbox_bottom, "offset_top", -LETTERBOX_HEIGHT, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	_letterbox_tween.tween_property(_letterbox_bottom, "offset_bottom", 0, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

func _hide_letterbox() -> void:
	if not _letterbox_active:
		return
	_letterbox_active = false
	if _letterbox_tween:
		_letterbox_tween.kill()
	_letterbox_tween = create_tween().set_parallel(true)
	_letterbox_tween.tween_property(_letterbox_top, "offset_top", -LETTERBOX_HEIGHT, 0.3).set_ease(Tween.EASE_IN)
	_letterbox_tween.tween_property(_letterbox_top, "offset_bottom", 0, 0.3).set_ease(Tween.EASE_IN)
	_letterbox_tween.tween_property(_letterbox_bottom, "offset_top", 0, 0.3).set_ease(Tween.EASE_IN)
	_letterbox_tween.tween_property(_letterbox_bottom, "offset_bottom", LETTERBOX_HEIGHT, 0.3).set_ease(Tween.EASE_IN)

## ===================== S55: SCREEN TINT =====================

func _build_tint_overlay() -> void:
	_tint_overlay = ColorRect.new()
	_tint_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_tint_overlay.color = Color(0, 0, 0, 0)
	_tint_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_tint_overlay.z_index = -1
	add_child(_tint_overlay)

const TINT_COLORS: Dictionary = {
	"red": Color(0.4, 0.05, 0.05, 0.25),
	"dark": Color(0.0, 0.0, 0.0, 0.35),
	"blue": Color(0.05, 0.08, 0.25, 0.2),
	"sepia": Color(0.2, 0.12, 0.05, 0.2),
	"void": Color(0.1, 0.0, 0.15, 0.3),
	"gold": Color(0.25, 0.18, 0.05, 0.15),
}

func _apply_tint(tint_name: String) -> void:
	var target_color: Color = TINT_COLORS.get(tint_name, Color(0, 0, 0, 0))
	if _tint_tween:
		_tint_tween.kill()
	_tint_tween = create_tween()
	_tint_tween.tween_property(_tint_overlay, "color", target_color, 0.4).set_ease(Tween.EASE_IN_OUT)

func _clear_tint() -> void:
	if _tint_tween:
		_tint_tween.kill()
	_tint_tween = create_tween()
	_tint_tween.tween_property(_tint_overlay, "color", Color(0, 0, 0, 0), 0.3).set_ease(Tween.EASE_IN)

## ===================== S55: TEXT EMPHASIS =====================
## Convert *word* to gold-colored BBCode [color=...]*word*[/color]

func _apply_emphasis(text: String) -> String:
	# Replace *text* with BBCode color tag for gold emphasis
	var result: String = ""
	var emphasis_color: String = "#e8c860"  # gold
	# Check high contrast mode
	if OptionsMenu and OptionsMenu.settings.get("high_contrast", false):
		emphasis_color = "#ffd700"  # brighter gold
	var in_emphasis: bool = false
	var i: int = 0
	while i < text.length():
		if text[i] == "*":
			if in_emphasis:
				result += "[/color]"
				in_emphasis = false
			else:
				result += "[color=%s]" % emphasis_color
				in_emphasis = true
			i += 1
		else:
			result += text[i]
			i += 1
	# Close unclosed tag
	if in_emphasis:
		result += "[/color]"
	return result

## Build BBCode text visible up to char_count (for typewriter effect with emphasis)
func _build_visible_bbcode(char_count: int) -> String:
	# Show plain text up to char_count, then apply emphasis formatting
	var visible_plain: String = full_text.substr(0, char_count)
	return _apply_emphasis(visible_plain)

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

	# 스타일 (어두운 반투명 -- 서고 모티프)
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

	# 대사 텍스트 — S55: BBCode enabled for emphasis
	text_label = RichTextLabel.new()
	text_label.bbcode_enabled = true
	text_label.fit_content = false
	text_label.scroll_active = false
	text_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	text_label.add_theme_font_size_override("normal_font_size", _get_dialogue_font_size())
	text_label.add_theme_color_override("default_color", Color(0.85, 0.82, 0.78))
	text_area.add_child(text_label)

	# 다음 대사 표시기 (triangle)
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

## S55: Get dialogue font size based on accessibility settings
func _get_dialogue_font_size() -> int:
	var font_size_level: int = 0
	if OptionsMenu:
		font_size_level = OptionsMenu.settings.get("dialogue_font_size", 0)
	match font_size_level:
		1: return 20  # Large
		2: return 24  # Extra Large
		_: return 16  # Normal

## S55: Refresh font size (called when settings change)
func refresh_font_size() -> void:
	if text_label:
		text_label.add_theme_font_size_override("normal_font_size", _get_dialogue_font_size())

## DialogueManager 시그널 연결
func _connect_signals() -> void:
	DialogueManager.dialogue_started.connect(_on_dialogue_started)
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	DialogueManager.dialogue_choice.connect(_on_dialogue_choice)
	DialogueManager.dialogue_ended.connect(_on_dialogue_ended)

func _on_dialogue_started() -> void:
	show_box()

func _on_dialogue_line(speaker: String, text: String, portrait: String) -> void:
	_clear_choices()
	_auto_advance_active = false
	_current_speaker = speaker

	# S55: Refresh font size in case it changed
	refresh_font_size()

	# S54: Parse direction tags before display
	var parsed = _parse_direction_tags(text)
	var clean_text: String = parsed["text"]
	_line_speed_override = parsed["speed"]
	_line_shake = parsed["shake"]
	_line_pause_time = parsed["pause"]

	# S54: Apply pause if present
	if _line_pause_time > 0.0:
		_is_paused = true
		_pause_timer = _line_pause_time

	# S54: Set blip pitch based on speaker
	_current_blip_pitch = BLIP_PITCH_MAP.get(speaker, 1.0)
	_blip_char_count = 0

	# S55: Play narration page-turn sound for narration lines (no speaker)
	if speaker == "":
		_play_narration_sfx()

	# 화자 이름 (나레이션이면 숨김)
	if speaker == "" or speaker == "system_log":
		speaker_label.text = ""
		portrait_texture.visible = false
		portrait_fallback.visible = false
		text_label.add_theme_color_override("default_color", UITheme.TEXT_NARRATION)
		if speaker == "system_log":
			text_label.add_theme_color_override("default_color", UITheme.TEXT_SYSTEM)
	else:
		speaker_label.text = speaker
		speaker_label.add_theme_color_override("font_color", UITheme.get_speaker_color(speaker))
		text_label.add_theme_color_override("default_color", UITheme.TEXT_PRIMARY)
		_update_portrait(speaker, portrait)

	# S55: Apply emphasis and store both plain and formatted text
	full_text = clean_text
	_bbcode_text = _apply_emphasis(clean_text)
	displayed_chars = 0
	text_label.text = ""
	is_typing = true
	typewriter_timer = 0.0
	indicator.visible = false

## S54: Parse direction tags from dialogue text
## Supports: [shake], [slow], [fast], [pause=N], [zoom=N], [pan=X,Y], [reset]
## S55: Added [letterbox], [/letterbox], [tint=NAME], [/tint]
func _parse_direction_tags(text: String) -> Dictionary:
	var result = {"text": text, "speed": -1.0, "shake": false, "pause": 0.0}
	# [shake]
	if "[shake]" in text:
		result["shake"] = true
		result["text"] = result["text"].replace("[shake]", "")
	# [slow]
	if "[slow]" in text:
		var base_speed = _get_typewriter_speed()
		result["speed"] = base_speed * 2.5 if base_speed > 0.0 else 0.06
		result["text"] = result["text"].replace("[slow]", "")
	# [fast]
	if "[fast]" in text:
		var base_speed = _get_typewriter_speed()
		result["speed"] = base_speed * 0.4 if base_speed > 0.0 else 0.0
		result["text"] = result["text"].replace("[fast]", "")
	# [pause=N]
	var regex = RegEx.new()
	regex.compile("\\[pause=(\\d+\\.?\\d*)\\]")
	var match = regex.search(result["text"])
	if match:
		result["pause"] = float(match.get_string(1))
		result["text"] = result["text"].replace(match.get_string(0), "")
	# S54: [zoom=N]
	var zoom_regex = RegEx.new()
	zoom_regex.compile("\\[zoom=(\\d+\\.?\\d*)\\]")
	var zoom_match = zoom_regex.search(result["text"])
	if zoom_match:
		var zoom_val = float(zoom_match.get_string(1))
		result["text"] = result["text"].replace(zoom_match.get_string(0), "")
		_dialogue_zoom(zoom_val, 0.5)
	# S54: [pan=X,Y]
	var pan_regex = RegEx.new()
	pan_regex.compile("\\[pan=(-?\\d+\\.?\\d*),(-?\\d+\\.?\\d*)\\]")
	var pan_match = pan_regex.search(result["text"])
	if pan_match:
		var px = float(pan_match.get_string(1))
		var py = float(pan_match.get_string(2))
		result["text"] = result["text"].replace(pan_match.get_string(0), "")
		_dialogue_pan(Vector2(px, py), 0.5)
	# S54: [reset]
	if "[reset]" in text:
		result["text"] = result["text"].replace("[reset]", "")
		_dialogue_reset(0.4)
	# S55: [letterbox]
	if "[letterbox]" in result["text"]:
		result["text"] = result["text"].replace("[letterbox]", "")
		_show_letterbox()
	# S55: [/letterbox]
	if "[/letterbox]" in result["text"]:
		result["text"] = result["text"].replace("[/letterbox]", "")
		_hide_letterbox()
	# S55: [tint=NAME]
	var tint_regex = RegEx.new()
	tint_regex.compile("\\[tint=(\\w+)\\]")
	var tint_match = tint_regex.search(result["text"])
	if tint_match:
		var tint_name = tint_match.get_string(1)
		result["text"] = result["text"].replace(tint_match.get_string(0), "")
		_apply_tint(tint_name)
	# S55: [/tint]
	if "[/tint]" in result["text"]:
		result["text"] = result["text"].replace("[/tint]", "")
		_clear_tint()
	# Clean up extra spaces
	result["text"] = result["text"].strip_edges()
	return result

func _on_dialogue_choice(choices: Array) -> void:
	_clear_choices()
	_auto_advance_active = false
	choice_container.visible = true

	for i in range(choices.size()):
		var choice = choices[i]
		var btn = Button.new()
		btn.text = choice.get("text", "...")
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

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

		var idx = i
		btn.pressed.connect(func():
			AudioManager.play_sfx("ui_select")
			_on_choice_selected(idx)
		)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		# S58: Pop-in animation — start scaled up, overshoot settle to 1.0
		btn.scale = Vector2(0.0, 0.0)
		btn.pivot_offset = Vector2(btn.size.x * 0.5, btn.size.y * 0.5) if btn.size.x > 0 else Vector2(100, 14)
		choice_container.add_child(btn)

	# S58: Staggered pop-in animation for each choice button
	for ci in range(choice_container.get_child_count()):
		var child_btn = choice_container.get_child(ci)
		child_btn.pivot_offset = child_btn.size * 0.5 if child_btn.size.x > 0 else Vector2(100, 14)
		var pop_t = create_tween()
		pop_t.tween_interval(ci * 0.06)  # stagger delay per button
		pop_t.tween_property(child_btn, "scale", Vector2(1.15, 1.15), 0.08).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
		pop_t.tween_property(child_btn, "scale", Vector2(1.0, 1.0), 0.06).set_ease(Tween.EASE_IN_OUT)

	if choice_container.get_child_count() > 0:
		# Delay focus grab slightly to let pop-in start
		await get_tree().create_timer(0.08).timeout
		if choice_container.get_child_count() > 0:
			choice_container.get_child(0).grab_focus()

func _on_dialogue_ended() -> void:
	# S54: Reset direction effects
	_line_shake = false
	_line_speed_override = -1.0
	_is_paused = false
	_auto_advance_active = false
	_current_portrait_key = ""
	_current_speaker = ""
	var viewport = get_viewport()
	if viewport:
		viewport.canvas_transform.origin = Vector2.ZERO
	# S54: Reset dialogue camera
	_dialogue_reset(0.3)
	# S55: Clear cinematic effects
	_hide_letterbox()
	_clear_tint()
	hide_box()

func _on_choice_selected(index: int) -> void:
	_clear_choices()
	DialogueManager.select_choice(index)

## S54: 포트레이트 베이스 이름 추출
func _get_portrait_base(key: String) -> String:
	if key == "":
		return ""
	var parts = key.split("_")
	if parts.size() > 0:
		return parts[0]
	return key

## 포트레이트 업데이트 -- S54: 크로스페이드 전환 추가
func _update_portrait(speaker: String, portrait_key: String = "") -> void:
	var key = portrait_key
	if key == "" and DEFAULT_PORTRAITS.has(speaker):
		key = DEFAULT_PORTRAITS[speaker]

	if key == _current_portrait_key and key != "":
		return

	var old_key = _current_portrait_key
	_current_portrait_key = key

	if key != "" and PORTRAIT_MAP.has(key):
		var path = PORTRAIT_MAP[key]
		if ResourceLoader.exists(path):
			var tex = load(path)
			if old_key == "":
				portrait_texture.texture = tex
				portrait_texture.visible = true
				portrait_fallback.visible = false
				portrait_texture.modulate.a = 0.0
				if _portrait_tween:
					_portrait_tween.kill()
				_portrait_tween = create_tween()
				_portrait_tween.tween_property(portrait_texture, "modulate:a", 1.0, 0.15)
			elif _get_portrait_base(old_key) == _get_portrait_base(key):
				if _portrait_tween:
					_portrait_tween.kill()
				_portrait_tween = create_tween()
				_portrait_tween.tween_property(portrait_texture, "modulate:a", 0.0, 0.1)
				_portrait_tween.tween_callback(func():
					portrait_texture.texture = tex
				)
				_portrait_tween.tween_property(portrait_texture, "modulate:a", 1.0, 0.1)
			else:
				if _portrait_tween:
					_portrait_tween.kill()
				var orig_x = portrait_texture.position.x
				_portrait_tween = create_tween()
				_portrait_tween.set_parallel(true)
				_portrait_tween.tween_property(portrait_texture, "position:x", orig_x - 30, 0.15).set_ease(Tween.EASE_IN)
				_portrait_tween.tween_property(portrait_texture, "modulate:a", 0.0, 0.12)
				_portrait_tween.set_parallel(false)
				_portrait_tween.tween_callback(func():
					portrait_texture.texture = tex
					portrait_texture.position.x = orig_x + 30
				)
				_portrait_tween.set_parallel(true)
				_portrait_tween.tween_property(portrait_texture, "position:x", orig_x, 0.15).set_ease(Tween.EASE_OUT)
				_portrait_tween.tween_property(portrait_texture, "modulate:a", 1.0, 0.15)
				_portrait_tween.set_parallel(false)
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
	_current_portrait_key = ""

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
	panel.offset_top = 0
	panel.modulate.a = 0.0
	var tween = create_tween().set_parallel(true)
	tween.tween_property(panel, "offset_top", original_top, 0.25).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tween.tween_property(panel, "modulate:a", 1.0, 0.2).set_ease(Tween.EASE_OUT)

func hide_box() -> void:
	choice_container.visible = false
	is_typing = false
	_auto_advance_active = false
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
		# S54: Cancel pause on interact
		if _is_paused:
			_is_paused = false
			_pause_timer = 0.0
		# S55: Cancel auto-advance on interact
		_auto_advance_active = false
		if is_typing:
			displayed_chars = full_text.length()
			text_label.text = _bbcode_text
			is_typing = false
			_line_shake = false
			indicator.visible = true
			var viewport = get_viewport()
			if viewport:
				viewport.canvas_transform.origin = Vector2.ZERO
			get_viewport().set_input_as_handled()
		elif not choice_container.visible:
			AudioManager.play_sfx("confirm")
			DialogueManager.advance()
			get_viewport().set_input_as_handled()

## ===================== S54: Dialogue Camera Effects =====================

func _get_active_camera() -> Camera2D:
	var vp = get_viewport()
	if vp == null:
		return null
	var tree = get_tree()
	if tree == null:
		return null
	var players = tree.get_nodes_in_group("player")
	if players.size() > 0:
		for child in players[0].get_children():
			if child is Camera2D:
				return child
	var root = tree.current_scene
	if root:
		var cameras = _find_cameras(root)
		if cameras.size() > 0:
			return cameras[0]
	return null

func _find_cameras(node: Node) -> Array:
	var result = []
	if node is Camera2D:
		result.append(node)
	for child in node.get_children():
		result.append_array(_find_cameras(child))
	return result

func _init_camera_state() -> void:
	if _cam_initialized:
		return
	var cam = _get_active_camera()
	if cam:
		_cam_original_zoom = cam.zoom
		_cam_original_offset = cam.offset
		_cam_initialized = true

func _dialogue_zoom(target_scale: float, duration: float = 0.5) -> void:
	var cam = _get_active_camera()
	if cam == null:
		return
	_init_camera_state()
	if _cam_tween:
		_cam_tween.kill()
	_cam_tween = create_tween()
	var target_zoom = _cam_original_zoom * target_scale
	_cam_tween.tween_property(cam, "zoom", target_zoom, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_CUBIC)

func _dialogue_pan(offset: Vector2, duration: float = 0.5) -> void:
	var cam = _get_active_camera()
	if cam == null:
		return
	_init_camera_state()
	if _cam_tween:
		_cam_tween.kill()
	_cam_tween = create_tween()
	_cam_tween.tween_property(cam, "offset", _cam_original_offset + offset, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_CUBIC)

func _dialogue_reset(duration: float = 0.5) -> void:
	if not _cam_initialized:
		return
	var cam = _get_active_camera()
	if cam == null:
		_cam_initialized = false
		return
	if _cam_tween:
		_cam_tween.kill()
	_cam_tween = create_tween().set_parallel(true)
	_cam_tween.tween_property(cam, "zoom", _cam_original_zoom, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_CUBIC)
	_cam_tween.tween_property(cam, "offset", _cam_original_offset, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_CUBIC)
	_cam_tween.set_parallel(false)
	_cam_tween.tween_callback(func(): _cam_initialized = false)
