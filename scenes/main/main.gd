## Main Scene — 타이틀 화면
## Cover.png 배경 + New Game / Continue / Quit 메뉴.
extends Control

@onready var continue_btn: Button = $VBoxContainer/ContinueButton

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_setup_background()
	_setup_menu()
	print("=== MEMORIA: The Price of Oblivion ===")

func _setup_background() -> void:
	# Cover.png 배경
	var bg = TextureRect.new()
	bg.set_anchors_preset(PRESET_FULL_RECT)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	var tex = load("res://assets/cg/cover.png")
	if tex:
		bg.texture = tex
	else:
		# fallback
		var fallback = ColorRect.new()
		fallback.set_anchors_preset(PRESET_FULL_RECT)
		fallback.color = Color(0.08, 0.08, 0.1)
		add_child(fallback)
		return
	bg.z_index = -1
	bg.mouse_filter = MOUSE_FILTER_IGNORE
	# 배경을 메뉴 뒤로
	add_child(bg)
	move_child(bg, 0)

	# 어두운 오버레이 (메뉴 가독성)
	var overlay = ColorRect.new()
	overlay.set_anchors_preset(PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.4)
	overlay.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(overlay)
	move_child(overlay, 1)

func _setup_menu() -> void:
	# 모든 버튼 스타일링
	for btn in $VBoxContainer.get_children():
		if btn is Button:
			_style_button(btn)

	# Continue 버튼 활성/비활성
	if continue_btn:
		continue_btn.disabled = not SaveManager.has_save(1)
		if continue_btn.disabled:
			continue_btn.modulate.a = 0.4

func _style_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(200, 44)
	btn.add_theme_font_size_override("font_size", 18)
	btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))

	# Normal
	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0.08, 0.07, 0.1, 0.85)
	normal.border_color = Color(0.5, 0.38, 0.2, 0.6)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(3)
	normal.set_content_margin_all(10)
	btn.add_theme_stylebox_override("normal", normal)

	# Hover
	var hover = normal.duplicate()
	hover.bg_color = Color(0.12, 0.1, 0.15, 0.95)
	hover.border_color = Color(0.75, 0.55, 0.25, 0.9)
	hover.border_width_bottom = 2
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	# Pressed
	var pressed = normal.duplicate()
	pressed.bg_color = Color(0.15, 0.12, 0.08, 0.95)
	pressed.border_color = Color(0.85, 0.65, 0.3, 1.0)
	btn.add_theme_stylebox_override("pressed", pressed)

	btn.add_theme_color_override("font_color", Color(0.75, 0.68, 0.55))
	btn.add_theme_color_override("font_hover_color", Color(0.95, 0.82, 0.5))
	btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.9, 0.6))

func _on_new_game_pressed() -> void:
	# 전체 게임 상태 초기화
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._init_starting_memories()
	GameManager.story_flags.clear()
	GameManager.current_chapter = 1
	GameManager.player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": 0,
		"elia_with_party": true,
	}
	SceneTransition.change_scene("res://scenes/maps/rim_forest.tscn")

func _on_continue_pressed() -> void:
	SaveManager.load_game(1)

func _on_options_pressed() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_quit_pressed() -> void:
	get_tree().quit()
