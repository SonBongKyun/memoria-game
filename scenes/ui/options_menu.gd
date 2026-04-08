## OptionsMenu (Autoload) — 옵션 메뉴
## 볼륨 조절, 풀스크린 토글. PauseMenu/타이틀에서 열기.
extends CanvasLayer

var is_open: bool = false

# UI 노드
var overlay: ColorRect
var panel: PanelContainer
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

# 설정 기본값
var settings: Dictionary = {
	"master_volume": 80,
	"bgm_volume": 70,
	"sfx_volume": 80,
	"text_speed": 3,
	"difficulty": 1,  # 0=Easy, 1=Normal, 2=Hard
	"fullscreen": false,
}

const SETTINGS_PATH: String = "user://settings.json"

func _ready() -> void:
	layer = 56  # PauseMenu(55) 위
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
	# 슬라이더/체크 값 동기화
	master_slider.value = settings.master_volume
	bgm_slider.value = settings.bgm_volume
	sfx_slider.value = settings.sfx_volume
	text_speed_slider.value = settings.text_speed
	fullscreen_check.button_pressed = settings.fullscreen
	_update_difficulty_label()
	_update_value_labels()
	overlay.visible = true
	panel.visible = true
	AudioManager.play_sfx("ui_open")

func close() -> void:
	if not is_open:
		return
	is_open = false
	_save_settings()
	AudioManager.play_sfx("ui_close")
	_hide_ui()

func _hide_ui() -> void:
	if overlay:
		overlay.visible = false
	if panel:
		panel.visible = false

func _apply_settings() -> void:
	# Master 볼륨 (bus index 0)
	var master_db = linear_to_db(settings.master_volume / 100.0)
	AudioServer.set_bus_volume_db(0, master_db)

	# BGM 볼륨 — AudioManager.bgm_player에 직접 적용
	if AudioManager and AudioManager.bgm_player:
		var bgm_base_db: float = -5.0
		var bgm_scale = settings.bgm_volume / 100.0
		AudioManager.bgm_player.volume_db = bgm_base_db + linear_to_db(bgm_scale)

	# SFX 볼륨 — AudioManager.sfx_player에 직접 적용
	if AudioManager and AudioManager.sfx_player:
		var sfx_base_db: float = -3.0
		var sfx_scale = settings.sfx_volume / 100.0
		AudioManager.sfx_player.volume_db = sfx_base_db + linear_to_db(sfx_scale)

	# 풀스크린
	if settings.fullscreen:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	else:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)

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

	# 어두운 오버레이
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.6)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	# 중앙 패널
	panel = PanelContainer.new()
	panel.anchor_left = 0.25
	panel.anchor_right = 0.75
	panel.anchor_top = 0.15
	panel.anchor_bottom = 0.85
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.95)
	style.border_color = Color(0.4, 0.3, 0.2, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 16)
	panel.add_child(vbox)

	# 타이틀
	var title = Label.new()
	title.text = "OPTIONS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))
	vbox.add_child(title)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 8)
	vbox.add_child(sep)

	# --- Master Volume ---
	master_value_label = Label.new()
	master_slider = _create_slider_row(vbox, "Master Volume", 80, master_value_label)
	master_slider.value_changed.connect(func(val: float):
		settings.master_volume = int(val)
		_update_value_labels()
		var db = linear_to_db(val / 100.0)
		AudioServer.set_bus_volume_db(0, db)
	)

	# --- BGM Volume ---
	bgm_value_label = Label.new()
	bgm_slider = _create_slider_row(vbox, "BGM Volume", 70, bgm_value_label)
	bgm_slider.value_changed.connect(func(val: float):
		settings.bgm_volume = int(val)
		_update_value_labels()
		if AudioManager and AudioManager.bgm_player:
			var bgm_scale = val / 100.0
			AudioManager.bgm_player.volume_db = -5.0 + linear_to_db(bgm_scale)
	)

	# --- SFX Volume ---
	sfx_value_label = Label.new()
	sfx_slider = _create_slider_row(vbox, "SFX Volume", 80, sfx_value_label)
	sfx_slider.value_changed.connect(func(val: float):
		settings.sfx_volume = int(val)
		_update_value_labels()
		if AudioManager and AudioManager.sfx_player:
			var sfx_scale = val / 100.0
			AudioManager.sfx_player.volume_db = -3.0 + linear_to_db(sfx_scale)
		# 미리듣기
		AudioManager.play_sfx("ui_select")
	)

	# --- Text Speed ---
	text_speed_value_label = Label.new()
	text_speed_slider = _create_speed_slider_row(vbox, "Text Speed", settings.text_speed, text_speed_value_label)
	text_speed_slider.value_changed.connect(func(val: float):
		settings.text_speed = int(val)
		_update_value_labels()
	)

	# --- Difficulty ---
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
	difficulty_btn.add_theme_font_size_override("font_size", 14)
	difficulty_btn.add_theme_color_override("font_color", Color(0.85, 0.7, 0.45))
	var diff_style = StyleBoxFlat.new()
	diff_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
	diff_style.border_color = Color(0.4, 0.3, 0.2, 0.5)
	diff_style.set_border_width_all(1)
	diff_style.set_corner_radius_all(3)
	diff_style.set_content_margin_all(4)
	difficulty_btn.add_theme_stylebox_override("normal", diff_style)
	var diff_hover = diff_style.duplicate()
	diff_hover.border_color = Color(0.7, 0.55, 0.3, 0.8)
	difficulty_btn.add_theme_stylebox_override("hover", diff_hover)
	_update_difficulty_label()
	difficulty_btn.pressed.connect(func():
		settings.difficulty = (settings.difficulty + 1) % 3
		_update_difficulty_label()
		AudioManager.play_sfx("ui_select")
	)
	diff_row.add_child(difficulty_btn)

	# 구분선
	var sep2 = HSeparator.new()
	sep2.add_theme_constant_override("separation", 8)
	vbox.add_child(sep2)

	# --- Fullscreen ---
	var fs_row = HBoxContainer.new()
	fs_row.add_theme_constant_override("separation", 12)
	vbox.add_child(fs_row)

	var fs_label = Label.new()
	fs_label.text = "Fullscreen"
	fs_label.add_theme_font_size_override("font_size", 15)
	fs_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	fs_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	fs_row.add_child(fs_label)

	fullscreen_check = CheckButton.new()
	fullscreen_check.button_pressed = settings.fullscreen
	fullscreen_check.toggled.connect(func(toggled: bool):
		settings.fullscreen = toggled
		if toggled:
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
		else:
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	)
	fs_row.add_child(fullscreen_check)

	# 구분선
	var sep3 = HSeparator.new()
	sep3.add_theme_constant_override("separation", 8)
	vbox.add_child(sep3)

	# --- Back 버튼 ---
	var back_btn = Button.new()
	back_btn.text = "Back"
	back_btn.custom_minimum_size = Vector2(0, 40)
	_style_button(back_btn)
	back_btn.pressed.connect(close)
	back_btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	vbox.add_child(back_btn)

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

	# 슬라이더 스타일
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

	# 슬라이더 스타일
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

	row_vbox.add_child(slider)
	return slider

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
