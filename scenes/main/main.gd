## Main title screen.
## S81: Calm single-image title built around GAME START.png.
extends Control

@onready var new_game_btn: Button = $VBoxContainer/NewGameButton
@onready var continue_btn: Button = $VBoxContainer/ContinueButton
@onready var options_btn: Button = $VBoxContainer/OptionsButton
@onready var quit_btn: Button = $VBoxContainer/QuitButton
@onready var menu_container: VBoxContainer = $VBoxContainer

const TITLE_BG_PATH: String = "res://assets/cg/game_image/game_start.png"
const TITLE_BGM_PATH: String = "res://assets/audio/bgm/title.mp3"

var _bg: TextureRect

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_build_background()
	_setup_menu()
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
	_bg.modulate.a = 1.0
	if ResourceLoader.exists(TITLE_BG_PATH):
		_bg.texture = load(TITLE_BG_PATH)
	add_child(_bg)
	move_child(_bg, 0)

func _setup_menu() -> void:
	# The GAME START illustration already contains the visible menu.
	# Keep these as invisible hit targets only, so the art is not overpainted.
	menu_container.visible = true
	menu_container.anchor_left = 0.720
	menu_container.anchor_top = 0.652
	menu_container.anchor_right = 0.956
	menu_container.anchor_bottom = 0.928
	menu_container.offset_left = 0
	menu_container.offset_top = 0
	menu_container.offset_right = 0
	menu_container.offset_bottom = 0
	menu_container.alignment = BoxContainer.ALIGNMENT_CENTER
	menu_container.add_theme_constant_override("separation", 10)
	menu_container.modulate.a = 0.0

	new_game_btn.text = ""
	continue_btn.text = ""
	options_btn.text = ""
	quit_btn.text = ""

	for btn in menu_container.get_children():
		if btn is Button:
			_style_title_button(btn)

	continue_btn.disabled = not SaveManager.has_save(1)
	if continue_btn.disabled:
		continue_btn.mouse_default_cursor_shape = Control.CURSOR_ARROW

func _style_title_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(238, 35)
	btn.focus_mode = Control.FOCUS_ALL

	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0, 0, 0, 0.0)
	normal.border_color = Color(0.72, 0.58, 0.35, 0.0)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(2)
	normal.set_content_margin_all(5)
	btn.add_theme_stylebox_override("normal", normal)

	var hover = normal.duplicate()
	hover.bg_color = Color(0, 0, 0, 0.0)
	hover.border_color = Color(0, 0, 0, 0.0)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	var pressed = hover.duplicate()
	btn.add_theme_stylebox_override("pressed", pressed)

	var disabled = normal.duplicate()
	disabled.bg_color = Color(0, 0, 0, 0.0)
	btn.add_theme_stylebox_override("disabled", disabled)

	btn.add_theme_color_override("font_color", Color(1, 1, 1, 0.0))
	btn.add_theme_color_override("font_hover_color", Color(1, 1, 1, 0.0))
	btn.add_theme_color_override("font_focus_color", Color(1, 1, 1, 0.0))
	btn.add_theme_color_override("font_pressed_color", Color(1, 1, 1, 0.0))
	btn.add_theme_color_override("font_disabled_color", Color(1, 1, 1, 0.0))

	btn.mouse_entered.connect(func():
		if has_node("/root/AudioManager"):
			AudioManager.play_sfx("ui_hover")
	)

func _fade_in_title() -> void:
	_bg.modulate.a = 1.0
	menu_container.modulate.a = 0.0

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
