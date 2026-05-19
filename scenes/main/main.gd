## Main title screen.
## S81: Calm single-image title built around GAME START.png.
extends Control

@onready var new_game_btn: Button = $VBoxContainer/NewGameButton
@onready var continue_btn: Button = $VBoxContainer/ContinueButton
@onready var options_btn: Button = $VBoxContainer/OptionsButton
@onready var quit_btn: Button = $VBoxContainer/QuitButton
@onready var menu_container: VBoxContainer = $VBoxContainer

const GAME_VERSION: String = "v0.9.1"
const TITLE_BG_PATH: String = "res://assets/cg/game_image/game_start.png"
const TITLE_BGM_PATH: String = "res://assets/audio/bgm/title.mp3"

var _bg: TextureRect
var _shade: ColorRect
var _version_label: Label
var _menu_tween: Tween

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_build_background()
	_setup_menu()
	_build_version_label()
	_fade_in_title()
	_play_title_bgm()
	print("=== MEMORIA: The Price of Oblivion ===")

func _build_background() -> void:
	_bg = TextureRect.new()
	_bg.set_anchors_preset(PRESET_FULL_RECT)
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg.mouse_filter = MOUSE_FILTER_IGNORE
	_bg.z_index = -2
	_bg.modulate.a = 0.0
	if ResourceLoader.exists(TITLE_BG_PATH):
		_bg.texture = load(TITLE_BG_PATH)
	add_child(_bg)
	move_child(_bg, 0)

	# A light veil keeps the busy illustration readable without covering the art.
	_shade = ColorRect.new()
	_shade.set_anchors_preset(PRESET_FULL_RECT)
	_shade.color = Color(0.015, 0.012, 0.018, 0.10)
	_shade.mouse_filter = MOUSE_FILTER_IGNORE
	_shade.z_index = -1
	_shade.modulate.a = 0.0
	add_child(_shade)
	move_child(_shade, 1)

func _setup_menu() -> void:
	# The image already contains the title and menu frame. These buttons sit on top
	# of that frame as quiet hit targets with only a subtle focus/hover treatment.
	menu_container.visible = true
	menu_container.anchor_left = 0.742
	menu_container.anchor_top = 0.623
	menu_container.anchor_right = 0.946
	menu_container.anchor_bottom = 0.890
	menu_container.offset_left = 0
	menu_container.offset_top = 0
	menu_container.offset_right = 0
	menu_container.offset_bottom = 0
	menu_container.alignment = BoxContainer.ALIGNMENT_CENTER
	menu_container.add_theme_constant_override("separation", 6)
	menu_container.modulate.a = 0.0

	new_game_btn.text = "NEW GAME"
	continue_btn.text = "CONTINUE"
	options_btn.text = "SETTINGS"
	quit_btn.text = "EXIT"

	for btn in menu_container.get_children():
		if btn is Button:
			_style_title_button(btn)

	continue_btn.disabled = not SaveManager.has_save(1)
	if continue_btn.disabled:
		continue_btn.mouse_default_cursor_shape = Control.CURSOR_ARROW

func _style_title_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(260, 42)
	btn.focus_mode = Control.FOCUS_ALL
	btn.add_theme_font_size_override("font_size", 19)

	var font = SystemFont.new()
	font.font_names = PackedStringArray([
		"Cinzel",
		"Cinzel Decorative",
		"Trajan Pro",
		"Constantia",
		"Palatino Linotype",
		"Book Antiqua",
		"Garamond",
		"serif",
	])
	font.font_weight = 500
	font.antialiasing = TextServer.FONT_ANTIALIASING_LCD
	btn.add_theme_font_override("font", font)

	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0, 0, 0, 0.0)
	normal.border_color = Color(0.72, 0.58, 0.35, 0.0)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(2)
	normal.set_content_margin_all(6)
	btn.add_theme_stylebox_override("normal", normal)

	var hover = normal.duplicate()
	hover.bg_color = Color(0.05, 0.06, 0.10, 0.16)
	hover.border_color = Color(0.75, 0.62, 0.38, 0.55)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	var pressed = hover.duplicate()
	pressed.bg_color = Color(0.10, 0.09, 0.12, 0.24)
	pressed.border_color = Color(0.95, 0.80, 0.48, 0.70)
	btn.add_theme_stylebox_override("pressed", pressed)

	var disabled = normal.duplicate()
	disabled.bg_color = Color(0, 0, 0, 0.0)
	btn.add_theme_stylebox_override("disabled", disabled)

	btn.add_theme_color_override("font_color", Color(0.92, 0.86, 0.74, 0.08))
	btn.add_theme_color_override("font_hover_color", Color(0.98, 0.90, 0.68, 0.95))
	btn.add_theme_color_override("font_focus_color", Color(0.98, 0.90, 0.68, 0.95))
	btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.94, 0.72, 1.0))
	btn.add_theme_color_override("font_disabled_color", Color(0.42, 0.40, 0.38, 0.20))

	btn.mouse_entered.connect(func():
		if has_node("/root/AudioManager"):
			AudioManager.play_sfx("ui_hover")
	)

func _build_version_label() -> void:
	_version_label = Label.new()
	_version_label.text = GAME_VERSION
	_version_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_version_label.anchor_left = 1.0
	_version_label.anchor_top = 1.0
	_version_label.anchor_right = 1.0
	_version_label.anchor_bottom = 1.0
	_version_label.offset_left = -110
	_version_label.offset_top = -30
	_version_label.offset_right = -14
	_version_label.offset_bottom = -8
	_version_label.add_theme_font_size_override("font_size", 11)
	_version_label.add_theme_color_override("font_color", Color(0.65, 0.62, 0.55, 0.38))
	_version_label.mouse_filter = MOUSE_FILTER_IGNORE
	_version_label.modulate.a = 0.0
	add_child(_version_label)

func _fade_in_title() -> void:
	if _menu_tween:
		_menu_tween.kill()
	_menu_tween = create_tween()
	_menu_tween.set_parallel(true)
	_menu_tween.tween_property(_bg, "modulate:a", 1.0, 0.8).set_ease(Tween.EASE_OUT)
	_menu_tween.tween_property(_shade, "modulate:a", 1.0, 0.8).set_ease(Tween.EASE_OUT)
	_menu_tween.tween_property(menu_container, "modulate:a", 1.0, 0.7).set_delay(0.25).set_ease(Tween.EASE_OUT)
	_menu_tween.tween_property(_version_label, "modulate:a", 1.0, 0.7).set_delay(0.35).set_ease(Tween.EASE_OUT)

func _play_title_bgm() -> void:
	if has_node("/root/AudioManager") and ResourceLoader.exists(TITLE_BGM_PATH):
		AudioManager.play_bgm(TITLE_BGM_PATH)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.is_pressed() and not event.echo:
		if not menu_container.has_focus():
			new_game_btn.grab_focus()

func _on_new_game_pressed() -> void:
	_play_select_sfx()
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._init_starting_memories()
	GameManager.story_flags.clear()
	GameManager.current_chapter = 1
	GameManager.ng_plus_cycle = 0
	GameManager.player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": 0,
		"elia_with_party": true,
		"items": {},
	}
	SceneFlow.pending_scene_id = "ch1_prologue"
	SceneTransition.change_scene_styled("res://scenes/main/vn_host.tscn")

func _on_continue_pressed() -> void:
	_play_select_sfx()
	SaveManager.load_game(1)

func _on_options_pressed() -> void:
	_play_select_sfx()
	OptionsMenu.open()

func _on_quit_pressed() -> void:
	_play_select_sfx()
	get_tree().quit()

func _play_select_sfx() -> void:
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_select")
