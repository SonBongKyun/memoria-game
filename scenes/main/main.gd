## Main title screen.
## S81: Calm single-image title built around GAME START.png.
extends Control

@onready var new_game_btn: Button = $VBoxContainer/NewGameButton
@onready var continue_btn: Button = $VBoxContainer/ContinueButton
@onready var options_btn: Button = $VBoxContainer/OptionsButton
@onready var quit_btn: Button = $VBoxContainer/QuitButton
@onready var menu_container: VBoxContainer = $VBoxContainer

const TITLE_BG_PATH: String = "res://assets/cg/generated/ui_title_memoria_premium.png"
const TITLE_BGM_PATH: String = "res://assets/audio/bgm/title.mp3"

var _bg: TextureRect
var _shade: ColorRect
var _title_stack: VBoxContainer
var _aftermath_btn: Button

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_build_background()
	_build_title_copy()
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

	_shade = ColorRect.new()
	_shade.set_anchors_preset(PRESET_FULL_RECT)
	_shade.color = Color(0.0, 0.0, 0.0, 0.24)
	_shade.mouse_filter = MOUSE_FILTER_IGNORE
	_shade.z_index = -1
	add_child(_shade)
	move_child(_shade, 1)

func _build_title_copy() -> void:
	_title_stack = VBoxContainer.new()
	_title_stack.anchor_left = 0.055
	_title_stack.anchor_right = 0.54
	_title_stack.anchor_top = 0.12
	_title_stack.anchor_bottom = 0.34
	_title_stack.add_theme_constant_override("separation", 4)
	_title_stack.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_title_stack)

	var title = Label.new()
	title.text = "MEMORIA"
	UITheme.apply_title_font(title)
	title.add_theme_font_size_override("font_size", 54)
	title.add_theme_color_override("font_color", Color(0.92, 0.78, 0.48))
	title.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.82))
	title.add_theme_constant_override("shadow_outline_size", 4)
	title.add_theme_constant_override("shadow_offset_x", 2)
	title.add_theme_constant_override("shadow_offset_y", 3)
	_title_stack.add_child(title)

	var subtitle = Label.new()
	subtitle.text = "The Price of Oblivion"
	UITheme.apply_title_font(subtitle)
	subtitle.add_theme_font_size_override("font_size", 22)
	subtitle.add_theme_color_override("font_color", Color(0.86, 0.80, 0.68))
	subtitle.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.82))
	subtitle.add_theme_constant_override("shadow_outline_size", 3)
	subtitle.add_theme_constant_override("shadow_offset_x", 1)
	subtitle.add_theme_constant_override("shadow_offset_y", 2)
	_title_stack.add_child(subtitle)

	var line = ColorRect.new()
	line.custom_minimum_size = Vector2(300, 2)
	line.color = Color(0.82, 0.62, 0.30, 0.64)
	_title_stack.add_child(line)

	var tag = Label.new()
	tag.text = "Burn what you remember. Carry what remains."
	UITheme.apply_body_font(tag)
	tag.add_theme_font_size_override("font_size", 14)
	tag.add_theme_color_override("font_color", Color(0.67, 0.62, 0.54))
	tag.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.70))
	tag.add_theme_constant_override("shadow_outline_size", 2)
	_title_stack.add_child(tag)

func _setup_menu() -> void:
	menu_container.visible = true
	menu_container.anchor_left = 0.720
	menu_container.anchor_top = 0.500
	menu_container.anchor_right = 0.956
	menu_container.anchor_bottom = 0.910
	menu_container.offset_left = 0
	menu_container.offset_top = 0
	menu_container.offset_right = 0
	menu_container.offset_bottom = 0
	menu_container.alignment = BoxContainer.ALIGNMENT_CENTER
	menu_container.add_theme_constant_override("separation", 11)
	menu_container.modulate.a = 1.0

	new_game_btn.text = "New Game"
	continue_btn.text = "Continue"
	if _aftermath_btn == null:
		_aftermath_btn = Button.new()
		_aftermath_btn.name = "AftermathPreviewButton"
		_aftermath_btn.pressed.connect(_on_aftermath_preview_pressed)
		menu_container.add_child(_aftermath_btn)
		menu_container.move_child(_aftermath_btn, 2)
	_aftermath_btn.text = "Part II: Aftermath"
	options_btn.text = "Options"
	quit_btn.text = "Quit"

	for btn in menu_container.get_children():
		if btn is Button:
			_style_title_button(btn)

	continue_btn.disabled = not SaveManager.has_save(1)
	if continue_btn.disabled:
		continue_btn.mouse_default_cursor_shape = Control.CURSOR_ARROW

func _style_title_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(250, 42)
	btn.focus_mode = Control.FOCUS_ALL

	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0.035, 0.030, 0.045, 0.72)
	normal.border_color = Color(0.58, 0.44, 0.24, 0.55)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(4)
	normal.set_content_margin_all(8)
	btn.add_theme_stylebox_override("normal", normal)

	var hover = normal.duplicate()
	hover.bg_color = Color(0.12, 0.095, 0.075, 0.86)
	hover.border_color = Color(0.95, 0.72, 0.36, 0.90)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	var pressed = hover.duplicate()
	pressed.bg_color = Color(0.16, 0.12, 0.08, 0.94)
	btn.add_theme_stylebox_override("pressed", pressed)

	var disabled = normal.duplicate()
	disabled.bg_color = Color(0.025, 0.025, 0.030, 0.45)
	disabled.border_color = Color(0.24, 0.22, 0.20, 0.32)
	btn.add_theme_stylebox_override("disabled", disabled)

	btn.add_theme_font_size_override("font_size", 17)
	btn.add_theme_color_override("font_color", Color(0.82, 0.76, 0.66, 0.95))
	btn.add_theme_color_override("font_hover_color", Color(1.0, 0.86, 0.52, 1.0))
	btn.add_theme_color_override("font_focus_color", Color(1.0, 0.86, 0.52, 1.0))
	btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.90, 0.62, 1.0))
	btn.add_theme_color_override("font_disabled_color", Color(0.38, 0.36, 0.34, 0.85))
	btn.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75))
	btn.add_theme_constant_override("shadow_outline_size", 2)

	btn.mouse_entered.connect(func():
		if has_node("/root/AudioManager"):
			AudioManager.play_sfx("ui_hover")
	)

func _fade_in_title() -> void:
	_bg.modulate.a = 1.0
	menu_container.modulate.a = 0.0
	if _title_stack:
		_title_stack.modulate.a = 0.0
	var tw = create_tween().set_parallel(true)
	tw.tween_property(menu_container, "modulate:a", 1.0, 0.45).set_trans(Tween.TRANS_SINE)
	if _title_stack:
		tw.tween_property(_title_stack, "modulate:a", 1.0, 0.55).set_trans(Tween.TRANS_SINE)

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

func _on_aftermath_preview_pressed() -> void:
	_play_select_sfx()
	GameManager.story_flags.clear()
	GameManager.story_flags["part2_aftershock_preview"] = true
	GameManager.current_chapter = 11
	GameManager.ng_plus_cycle = 0
	GameManager.player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": 0,
		"elia_with_party": true,
		"items": {},
	}
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._init_starting_memories()
	MemoryManager.add_chapter_memories(11)
	SceneFlow.import_data({})
	SceneFlow.pending_scene_id = "ch11_departure"
	SceneTransition.change_scene_styled("res://scenes/main/vn_host.tscn")

func _on_options_pressed() -> void:
	_play_select_sfx()
	OptionsMenu.open()

func _on_quit_pressed() -> void:
	_play_select_sfx()
	get_tree().quit()

func _play_select_sfx() -> void:
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_select")
