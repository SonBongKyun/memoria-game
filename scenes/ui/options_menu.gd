## OptionsMenu (Autoload) -- 옵션 메뉴
## 볼륨 조절, 풀스크린 토글, 접근성 옵션. PauseMenu/타이틀에서 열기.
## S55: Accessibility overhaul — font size, high contrast, screen shake, colorblind, reduce motion.
extends CanvasLayer

var is_open: bool = false

# UI 노드
var overlay: ColorRect
var panel: PanelContainer
var scroll_container: ScrollContainer
var difficulty_btn: Button
var master_slider: HSlider
var bgm_slider: HSlider
var sfx_slider: HSlider
var text_speed_slider: HSlider
var fullscreen_check: CheckButton
var master_value_label: Label
var bgm_value_label: Label
var sfx_value_label: Label
var text_speed_value_label: Label

# S55: Accessibility UI references
var font_size_btn: Button
var high_contrast_check: CheckButton
var shake_check: CheckButton
var colorblind_btn: Button
var reduce_motion_check: CheckButton
var auto_advance_check: CheckButton

# 설정 기본값
var settings: Dictionary = {
	"master_volume": 80,
	"bgm_volume": 70,
	"sfx_volume": 80,
	"text_speed": 3,
	"difficulty": 1,  # 0=Easy, 1=Normal, 2=Hard
	"fullscreen": false,
	"font_scale": 1.0,           # Legacy (kept for save compat)
	"screen_shake": true,        # S53/S55: Screen shake toggle
	"colorblind_mode": 0,        # S55: 0=Off, 1=Deuteranopia, 2=Protanopia, 3=Tritanopia
	"locale": "en",              # S54: Language (en/ko)
	"resolution": 0,             # S55: 0=720p, 1=1080p, 2=1440p
	# S55: New accessibility settings
	"dialogue_font_size": 0,     # 0=Normal, 1=Large, 2=Extra Large
	"high_contrast": false,      # Increases outlines, brighter borders
	"reduce_motion": false,      # Disables particles, simplifies animations
	"auto_advance_narration": true, # Auto-advance narration lines
}

const SETTINGS_PATH: String = "user://settings.json"

func _ready() -> void:
	layer = 56
	_load_settings()
	_build_ui()
	_hide_ui()
	_apply_settings()
	process_mode = Node.PROCESS_MODE_ALWAYS
	print("[OptionsMenu] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if is_open and event.is_action_pressed("cancel"):
		close()
		get_viewport().set_input_as_handled()

func open() -> void:
	if is_open:
		return
	is_open = true
	# Sync UI with current settings
	master_slider.value = settings.master_volume
	bgm_slider.value = settings.bgm_volume
	sfx_slider.value = settings.sfx_volume
	text_speed_slider.value = settings.text_speed
	fullscreen_check.button_pressed = settings.fullscreen
	if shake_check:
		shake_check.button_pressed = settings.screen_shake
	if high_contrast_check:
		high_contrast_check.button_pressed = settings.high_contrast
	if reduce_motion_check:
		reduce_motion_check.button_pressed = settings.reduce_motion
	if auto_advance_check:
		auto_advance_check.button_pressed = settings.auto_advance_narration
	_update_difficulty_label()
	_update_font_size_label()
	_update_colorblind_label()
	_update_value_labels()
	overlay.visible = true
	panel.visible = true
	AudioManager.play_sfx("ui_open")

func close() -> void:
	if not is_open:
		return
	is_open = false
	_save_settings()
	_apply_settings()
	AudioManager.play_sfx("ui_close")
	_hide_ui()

func _hide_ui() -> void:
	if overlay:
		overlay.visible = false
	if panel:
		panel.visible = false

func _apply_settings() -> void:
	# Locale
	if settings.has("locale"):
		GameManager.current_locale = settings["locale"]
	# Master volume (bus index 0)
	var master_db = linear_to_db(settings.master_volume / 100.0)
	AudioServer.set_bus_volume_db(0, master_db)

	# BGM volume
	if AudioManager and AudioManager.bgm_player:
		var bgm_base_db: float = -5.0
		var bgm_scale = settings.bgm_volume / 100.0
		AudioManager.bgm_player.volume_db = bgm_base_db + linear_to_db(bgm_scale)

	# SFX volume
	if AudioManager and AudioManager.sfx_player:
		var sfx_base_db: float = -3.0
		var sfx_scale = settings.sfx_volume / 100.0
		AudioManager.sfx_player.volume_db = sfx_base_db + linear_to_db(sfx_scale)

	# Fullscreen
	if settings.fullscreen:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	else:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
		_apply_resolution(settings.get("resolution", 0))

	# S55: Apply dialogue font size to DialogueBox
	if DialogueBox and DialogueBox.has_method("refresh_font_size"):
		DialogueBox.refresh_font_size()

	# S55: Apply high contrast
	_apply_high_contrast(settings.get("high_contrast", false))

## S55: 해상도 적용 (윈도우 모드에서만)
func _apply_resolution(idx: int) -> void:
	var resolutions = [
		Vector2i(1280, 720),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440),
	]
	idx = clampi(idx, 0, resolutions.size() - 1)
	var target = resolutions[idx]
	DisplayServer.window_set_size(target)
	var screen_size = DisplayServer.screen_get_size()
	var window_pos = (screen_size - target) / 2
	DisplayServer.window_set_position(Vector2i(maxi(window_pos.x, 0), maxi(window_pos.y, 0)))

func _save_settings() -> void:
	var file = FileAccess.open(SETTINGS_PATH, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(settings, "\t"))
		file.close()

func _load_settings() -> void:
	if not FileAccess.file_exists(SETTINGS_PATH):
		return
	var file = FileAccess.open(SETTINGS_PATH, FileAccess.READ)
	if not file:
		return
	var text = file.get_as_text()
	file.close()
	var json = JSON.new()
	if json.parse(text) == OK:
		var data = json.data
		if data is Dictionary:
			for key in settings.keys():
				if data.has(key):
					settings[key] = data[key]

const DIFFICULTY_LABELS: Dictionary = {0: "Easy", 1: "Normal", 2: "Hard"}
const DIFFICULTY_COLORS: Dictionary = {
	0: Color(0.5, 0.75, 0.5),
	1: Color(0.85, 0.7, 0.45),
	2: Color(0.85, 0.4, 0.35),
}

func _update_difficulty_label() -> void:
	if difficulty_btn:
		var diff = settings.get("difficulty", 1)
		difficulty_btn.text = DIFFICULTY_LABELS.get(diff, "Normal")
		difficulty_btn.add_theme_color_override("font_color", DIFFICULTY_COLORS.get(diff, Color(0.85, 0.7, 0.45)))

const TEXT_SPEED_LABELS: Dictionary = {
	1: "Slow",
	2: "Slow+",
	3: "Normal",
	4: "Fast",
	5: "Instant",
}

const FONT_SIZE_LABELS: Dictionary = {0: "Normal", 1: "Large", 2: "Extra Large"}
const COLORBLIND_LABELS: Dictionary = {0: "Off", 1: "Deuteranopia", 2: "Protanopia", 3: "Tritanopia"}

func _update_font_size_label() -> void:
	if font_size_btn:
		var level = settings.get("dialogue_font_size", 0)
		font_size_btn.text = FONT_SIZE_LABELS.get(level, "Normal")

func _update_colorblind_label() -> void:
	if colorblind_btn:
		var mode = settings.get("colorblind_mode", 0)
		colorblind_btn.text = COLORBLIND_LABELS.get(mode, "Off")

func _update_value_labels() -> void:
	if master_value_label:
		master_value_label.text = "%d%%" % int(master_slider.value)
	if bgm_value_label:
		bgm_value_label.text = "%d%%" % int(bgm_slider.value)
	if sfx_value_label:
		sfx_value_label.text = "%d%%" % int(sfx_slider.value)
	if text_speed_value_label:
		var spd = int(text_speed_slider.value)
		text_speed_value_label.text = TEXT_SPEED_LABELS.get(spd, "Normal")

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# Dark overlay
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.6)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	# Center panel (wider to fit more options)
	panel = PanelContainer.new()
	panel.anchor_left = 0.2
	panel.anchor_right = 0.8
	panel.anchor_top = 0.05
	panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.95)
	style.border_color = Color(0.4, 0.3, 0.2, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	# Scroll container for all settings (many options now)
	scroll_container = ScrollContainer.new()
	scroll_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll_container.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	panel.add_child(scroll_container)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll_container.add_child(vbox)

	# Title
	var title = Label.new()
	title.text = "OPTIONS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))
	vbox.add_child(title)

	_add_separator(vbox)

	# ========== AUDIO SECTION ==========
	_add_section_header(vbox, "AUDIO")

	# Master Volume
	master_value_label = Label.new()
	master_slider = _create_slider_row(vbox, "Master Volume", 80, master_value_label)
	master_slider.value_changed.connect(func(val: float):
		settings.master_volume = int(val)
		_update_value_labels()
		var db = linear_to_db(val / 100.0)
		AudioServer.set_bus_volume_db(0, db)
	)

	# BGM Volume
	bgm_value_label = Label.new()
	bgm_slider = _create_slider_row(vbox, "BGM Volume", 70, bgm_value_label)
	bgm_slider.value_changed.connect(func(val: float):
		settings.bgm_volume = int(val)
		_update_value_labels()
		if AudioManager and AudioManager.bgm_player:
			var bgm_scale = val / 100.0
			AudioManager.bgm_player.volume_db = -5.0 + linear_to_db(bgm_scale)
	)

	# SFX Volume
	sfx_value_label = Label.new()
	sfx_slider = _create_slider_row(vbox, "SFX Volume", 80, sfx_value_label)
	sfx_slider.value_changed.connect(func(val: float):
		settings.sfx_volume = int(val)
		_update_value_labels()
		if AudioManager and AudioManager.sfx_player:
			var sfx_scale = val / 100.0
			AudioManager.sfx_player.volume_db = -3.0 + linear_to_db(sfx_scale)
		AudioManager.play_sfx("ui_select")
	)

	_add_separator(vbox)

	# ========== GAMEPLAY SECTION ==========
	_add_section_header(vbox, "GAMEPLAY")

	# Text Speed
	text_speed_value_label = Label.new()
	text_speed_slider = _create_speed_slider_row(vbox, "Text Speed", settings.text_speed, text_speed_value_label)
	text_speed_slider.value_changed.connect(func(val: float):
		settings.text_speed = int(val)
		_update_value_labels()
	)

	# Difficulty
	var diff_row = HBoxContainer.new()
	diff_row.add_theme_constant_override("separation", 12)
	vbox.add_child(diff_row)

	var diff_label = Label.new()
	diff_label.text = "Difficulty"
	diff_label.add_theme_font_size_override("font_size", 15)
	diff_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	diff_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	diff_row.add_child(diff_label)

	difficulty_btn = Button.new()
	difficulty_btn.custom_minimum_size = Vector2(100, 30)
	_style_cycle_button(difficulty_btn)
	_update_difficulty_label()
	difficulty_btn.pressed.connect(func():
		settings.difficulty = (settings.difficulty + 1) % 3
		_update_difficulty_label()
		AudioManager.play_sfx("ui_select")
	)
	diff_row.add_child(difficulty_btn)

	# Auto-advance narration
	auto_advance_check = _create_toggle_row(vbox, "Auto-Advance Narration", settings.auto_advance_narration)
	auto_advance_check.toggled.connect(func(toggled: bool):
		settings.auto_advance_narration = toggled
	)

	_add_separator(vbox)

	# ========== DISPLAY SECTION ==========
	_add_section_header(vbox, "DISPLAY")

	# Fullscreen
	fullscreen_check = _create_toggle_row(vbox, "Fullscreen", settings.fullscreen)
	fullscreen_check.toggled.connect(func(toggled: bool):
		settings.fullscreen = toggled
		if toggled:
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
		else:
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
			_apply_resolution(settings.get("resolution", 1))
	)

	# Resolution
	var res_row = HBoxContainer.new()
	res_row.add_theme_constant_override("separation", 12)
	vbox.add_child(res_row)

	var res_label = Label.new()
	res_label.text = "Resolution"
	res_label.add_theme_font_size_override("font_size", 15)
	res_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	res_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	res_row.add_child(res_label)

	var res_option = OptionButton.new()
	res_option.add_item("720p (1280x720)", 0)
	res_option.add_item("1080p (1920x1080)", 1)
	res_option.add_item("1440p (2560x1440)", 2)
	res_option.selected = settings.get("resolution", 1)
	res_option.add_theme_font_size_override("font_size", 14)
	res_option.item_selected.connect(func(idx: int):
		settings["resolution"] = idx
		if not settings.fullscreen:
			_apply_resolution(idx)
	)
	res_row.add_child(res_option)

	# Language
	var lang_row = HBoxContainer.new()
	lang_row.add_theme_constant_override("separation", 12)
	vbox.add_child(lang_row)

	var lang_label = Label.new()
	lang_label.text = "Language"
	lang_label.add_theme_font_size_override("font_size", 15)
	lang_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	lang_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lang_row.add_child(lang_label)

	var lang_btn = Button.new()
	lang_btn.custom_minimum_size = Vector2(130, 30)
	_style_cycle_button(lang_btn)
	var lang_map = {"en": "English", "ko": "한국어"}
	lang_btn.text = lang_map.get(GameManager.current_locale, "English")
	lang_btn.pressed.connect(func():
		if GameManager.current_locale == "en":
			GameManager.current_locale = "ko"
		else:
			GameManager.current_locale = "en"
		lang_btn.text = lang_map.get(GameManager.current_locale, "English")
		settings["locale"] = GameManager.current_locale
		AudioManager.play_sfx("ui_select")
	)
	lang_row.add_child(lang_btn)

	_add_separator(vbox)

	# ========== ACCESSIBILITY SECTION ==========
	_add_section_header(vbox, "ACCESSIBILITY")

	# Font Size (3 levels: Normal / Large / Extra Large)
	var font_row = HBoxContainer.new()
	font_row.add_theme_constant_override("separation", 12)
	vbox.add_child(font_row)

	var font_label = Label.new()
	font_label.text = "Dialogue Font Size"
	font_label.add_theme_font_size_override("font_size", 15)
	font_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	font_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	font_row.add_child(font_label)

	font_size_btn = Button.new()
	font_size_btn.custom_minimum_size = Vector2(130, 30)
	_style_cycle_button(font_size_btn)
	_update_font_size_label()
	font_size_btn.pressed.connect(func():
		settings.dialogue_font_size = (settings.dialogue_font_size + 1) % 3
		_update_font_size_label()
		# Also update legacy font_scale for compat
		match settings.dialogue_font_size:
			0: settings.font_scale = 1.0
			1: settings.font_scale = 1.25
			2: settings.font_scale = 1.5
		# Live preview
		if DialogueBox and DialogueBox.has_method("refresh_font_size"):
			DialogueBox.refresh_font_size()
		AudioManager.play_sfx("ui_select")
	)
	font_row.add_child(font_size_btn)

	# High Contrast Mode
	high_contrast_check = _create_toggle_row(vbox, "High Contrast", settings.high_contrast)
	high_contrast_check.toggled.connect(func(toggled: bool):
		settings.high_contrast = toggled
		_apply_high_contrast(toggled)
	)

	# Screen Shake
	shake_check = _create_toggle_row(vbox, "Screen Shake", settings.screen_shake)
	shake_check.toggled.connect(func(toggled: bool):
		settings.screen_shake = toggled
	)

	# Colorblind Mode (Off / Deuteranopia / Protanopia / Tritanopia)
	var cb_row = HBoxContainer.new()
	cb_row.add_theme_constant_override("separation", 12)
	vbox.add_child(cb_row)

	var cb_label = Label.new()
	cb_label.text = "Colorblind Mode"
	cb_label.add_theme_font_size_override("font_size", 15)
	cb_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	cb_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cb_row.add_child(cb_label)

	colorblind_btn = Button.new()
	colorblind_btn.custom_minimum_size = Vector2(140, 30)
	_style_cycle_button(colorblind_btn)
	_update_colorblind_label()
	colorblind_btn.pressed.connect(func():
		settings.colorblind_mode = (settings.colorblind_mode + 1) % 4
		_update_colorblind_label()
		AudioManager.play_sfx("ui_select")
	)
	cb_row.add_child(colorblind_btn)

	# Reduce Motion
	reduce_motion_check = _create_toggle_row(vbox, "Reduce Motion", settings.reduce_motion)
	reduce_motion_check.toggled.connect(func(toggled: bool):
		settings.reduce_motion = toggled
	)

	# Description text for accessibility
	var desc = Label.new()
	desc.text = "Font Size affects dialogue text. High Contrast increases text outlines and UI borders. Reduce Motion disables particles and simplifies animations."
	desc.add_theme_font_size_override("font_size", 11)
	desc.add_theme_color_override("font_color", Color(0.45, 0.42, 0.38, 0.7))
	desc.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vbox.add_child(desc)

	_add_separator(vbox)

	# Back button
	var back_btn = Button.new()
	back_btn.text = GameManager.loc("back")
	back_btn.custom_minimum_size = Vector2(0, 40)
	_style_button(back_btn)
	back_btn.pressed.connect(close)
	back_btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	vbox.add_child(back_btn)

## ===================== HELPER: Section Headers =====================

func _add_section_header(parent: VBoxContainer, text: String) -> void:
	var label = Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", 13)
	label.add_theme_color_override("font_color", Color(0.55, 0.48, 0.35, 0.8))
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
	parent.add_child(label)

func _add_separator(parent: VBoxContainer) -> void:
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 6)
	parent.add_child(sep)

## ===================== HELPER: Toggle row =====================

func _create_toggle_row(parent: VBoxContainer, label_text: String, default_val: bool) -> CheckButton:
	var row = HBoxContainer.new()
	row.add_theme_constant_override("separation", 12)
	parent.add_child(row)

	var label = Label.new()
	label.text = label_text
	label.add_theme_font_size_override("font_size", 15)
	label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(label)

	var check = CheckButton.new()
	check.button_pressed = default_val
	row.add_child(check)
	return check

## ===================== HELPER: Cycle button style =====================

func _style_cycle_button(btn: Button) -> void:
	btn.add_theme_font_size_override("font_size", 14)
	btn.add_theme_color_override("font_color", Color(0.85, 0.7, 0.45))
	var btn_style = StyleBoxFlat.new()
	btn_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
	btn_style.border_color = Color(0.4, 0.3, 0.2, 0.5)
	btn_style.set_border_width_all(1)
	btn_style.set_corner_radius_all(3)
	btn_style.set_content_margin_all(4)
	btn.add_theme_stylebox_override("normal", btn_style)
	var hover = btn_style.duplicate()
	hover.border_color = Color(0.7, 0.55, 0.3, 0.8)
	btn.add_theme_stylebox_override("hover", hover)

## ===================== S55: High Contrast application =====================

func _apply_high_contrast(enabled: bool) -> void:
	# Modify DialogueBox panel border and text outline
	if DialogueBox and DialogueBox.panel:
		var panel_style = DialogueBox.panel.get_theme_stylebox("panel")
		if panel_style is StyleBoxFlat:
			if enabled:
				panel_style.border_color = Color(0.7, 0.6, 0.45, 1.0)
				panel_style.set_border_width_all(3)
			else:
				panel_style.border_color = Color(0.3, 0.25, 0.2, 0.8)
				panel_style.set_border_width_all(2)
	# Speaker label outline
	if DialogueBox and DialogueBox.speaker_label:
		if enabled:
			DialogueBox.speaker_label.add_theme_constant_override("outline_size", 2)
			DialogueBox.speaker_label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
		else:
			DialogueBox.speaker_label.add_theme_constant_override("outline_size", 0)
	# Text label: brighter default color in high contrast
	if DialogueBox and DialogueBox.text_label:
		if enabled:
			DialogueBox.text_label.add_theme_color_override("default_color", Color(0.95, 0.93, 0.9))
		else:
			DialogueBox.text_label.add_theme_color_override("default_color", Color(0.85, 0.82, 0.78))

## ===================== S55: Utility queries for other systems =====================

## Check if screen shake is enabled
static func is_shake_enabled() -> bool:
	if OptionsMenu:
		return OptionsMenu.settings.get("screen_shake", true)
	return true

## Check if reduce motion is enabled
static func is_reduce_motion() -> bool:
	if OptionsMenu:
		return OptionsMenu.settings.get("reduce_motion", false)
	return false

## Check if high contrast is enabled
static func is_high_contrast() -> bool:
	if OptionsMenu:
		return OptionsMenu.settings.get("high_contrast", false)
	return false

## Get colorblind mode (0=off, 1=deutan, 2=protan, 3=tritan)
static func get_colorblind_mode() -> int:
	if OptionsMenu:
		return OptionsMenu.settings.get("colorblind_mode", 0)
	return 0

## S55: Get a status effect indicator shape string for colorblind accessibility
## Returns a shape prefix to prepend to status effect names for shape+color redundancy
static func get_status_shape(effect_name: String) -> String:
	var mode = get_colorblind_mode()
	if mode == 0:
		return ""  # No shape needed when colorblind mode is off
	var shapes: Dictionary = {
		"poison": "^",       # triangle for poison
		"burn": "o",         # circle for burn
		"weakened": "#",     # square for weakened
		"stunned": "<>",     # diamond for stun
		"shielded": "[]",    # brackets for shield
		"regeneration": "*", # star for regen
	}
	return shapes.get(effect_name.to_lower(), ".")

## S55: Get colorblind-safe color for a status effect
static func get_status_color(effect_name: String) -> Color:
	var mode = get_colorblind_mode()
	var default_colors: Dictionary = {
		"poison": Color(0.3, 0.7, 0.3),
		"burn": Color(0.9, 0.4, 0.1),
		"weakened": Color(0.6, 0.4, 0.7),
		"stunned": Color(0.9, 0.85, 0.2),
		"shielded": Color(0.3, 0.6, 0.9),
		"regeneration": Color(0.4, 0.9, 0.5),
	}
	if mode == 0:
		return default_colors.get(effect_name.to_lower(), Color.WHITE)

	# Colorblind-safe palette (blue/orange/yellow which are safe for most types)
	var safe_colors: Dictionary = {
		"poison": Color(0.0, 0.45, 0.7),
		"burn": Color(0.9, 0.6, 0.0),
		"weakened": Color(0.8, 0.4, 0.7),
		"stunned": Color(0.95, 0.9, 0.25),
		"shielded": Color(0.35, 0.7, 0.9),
		"regeneration": Color(0.0, 0.6, 0.5),
	}
	return safe_colors.get(effect_name.to_lower(), Color.WHITE)

## ===================== SLIDER BUILDERS =====================

func _create_slider_row(parent: VBoxContainer, label_text: String, default_val: int, value_label: Label) -> HSlider:
	var row_vbox = VBoxContainer.new()
	row_vbox.add_theme_constant_override("separation", 4)
	parent.add_child(row_vbox)

	var header = HBoxContainer.new()
	row_vbox.add_child(header)

	var label = Label.new()
	label.text = label_text
	label.add_theme_font_size_override("font_size", 15)
	label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(label)

	value_label.text = "%d%%" % default_val
	value_label.add_theme_font_size_override("font_size", 14)
	value_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))
	value_label.custom_minimum_size = Vector2(50, 0)
	value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	header.add_child(value_label)

	var slider = HSlider.new()
	slider.min_value = 0
	slider.max_value = 100
	slider.step = 1
	slider.value = default_val
	slider.custom_minimum_size = Vector2(0, 20)
	_style_slider(slider)
	row_vbox.add_child(slider)
	return slider

func _create_speed_slider_row(parent: VBoxContainer, label_text: String, default_val: int, value_label: Label) -> HSlider:
	var row_vbox = VBoxContainer.new()
	row_vbox.add_theme_constant_override("separation", 4)
	parent.add_child(row_vbox)

	var header = HBoxContainer.new()
	row_vbox.add_child(header)

	var label = Label.new()
	label.text = label_text
	label.add_theme_font_size_override("font_size", 15)
	label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(label)

	value_label.text = TEXT_SPEED_LABELS.get(default_val, "Normal")
	value_label.add_theme_font_size_override("font_size", 14)
	value_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))
	value_label.custom_minimum_size = Vector2(50, 0)
	value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	header.add_child(value_label)

	var slider = HSlider.new()
	slider.min_value = 1
	slider.max_value = 5
	slider.step = 1
	slider.value = default_val
	slider.custom_minimum_size = Vector2(0, 20)
	_style_slider(slider)
	row_vbox.add_child(slider)
	return slider

func _style_slider(slider: HSlider) -> void:
	var grabber_style = StyleBoxFlat.new()
	grabber_style.bg_color = Color(0.75, 0.6, 0.3)
	grabber_style.set_corner_radius_all(4)
	grabber_style.set_content_margin_all(0)
	grabber_style.content_margin_left = 8
	grabber_style.content_margin_right = 8
	grabber_style.content_margin_top = 8
	grabber_style.content_margin_bottom = 8
	slider.add_theme_stylebox_override("grabber_area", grabber_style)
	slider.add_theme_stylebox_override("grabber_area_highlight", grabber_style)

	var slider_style = StyleBoxFlat.new()
	slider_style.bg_color = Color(0.15, 0.12, 0.18, 0.9)
	slider_style.set_corner_radius_all(3)
	slider_style.set_content_margin_all(0)
	slider_style.content_margin_top = 6
	slider_style.content_margin_bottom = 6
	slider.add_theme_stylebox_override("slider", slider_style)

func _style_button(btn: Button) -> void:
	var btn_style = StyleBoxFlat.new()
	btn_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
	btn_style.border_color = Color(0.35, 0.28, 0.2, 0.5)
	btn_style.set_border_width_all(1)
	btn_style.set_corner_radius_all(3)
	btn_style.set_content_margin_all(8)
	btn.add_theme_stylebox_override("normal", btn_style)

	var hover_style = btn_style.duplicate()
	hover_style.bg_color = Color(0.15, 0.12, 0.18, 0.95)
	hover_style.border_color = Color(0.7, 0.55, 0.3, 0.8)
	btn.add_theme_stylebox_override("hover", hover_style)
	btn.add_theme_stylebox_override("focus", hover_style)

	var press_style = btn_style.duplicate()
	press_style.bg_color = Color(0.18, 0.14, 0.1, 0.95)
	press_style.border_color = Color(0.85, 0.65, 0.3, 1.0)
	btn.add_theme_stylebox_override("pressed", press_style)

	btn.add_theme_font_size_override("font_size", 15)
	btn.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	btn.add_theme_color_override("font_hover_color", Color(0.95, 0.82, 0.5))

	# S57: Hover sound + button press scale feedback
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn.pivot_offset = Vector2(btn.custom_minimum_size.x / 2.0, btn.custom_minimum_size.y / 2.0)
	btn.button_down.connect(func():
		var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
		t.tween_property(btn, "scale", Vector2(0.95, 0.95), 0.05)
	)
	btn.button_up.connect(func():
		var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
		t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.08).set_ease(Tween.EASE_OUT)
	)
