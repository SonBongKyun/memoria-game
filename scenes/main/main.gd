## Main Scene — 타이틀 화면
## Cover.png 배경 + New Game / Continue / Quit 메뉴.
extends Control

@onready var continue_btn: Button = $VBoxContainer/ContinueButton
var ng_plus_btn: Button = null
var _title_label: Label
var _subtitle_label: Label
var _overlay_rect: ColorRect
var _motes: Array[ColorRect] = []
var _intro_t: float = 0.0
var _bg_primary: TextureRect
var _bg_secondary: TextureRect
var _bg_cycle: float = 0.0
var _bg_candidates: Array[String] = []
var _bg_index: int = 0
var _prompt_label: Label
var _intro_skipped: bool = false
var _intro_tween: Tween

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_setup_background()
	_setup_menu()
	_build_title_overlay()
	_spawn_ambient_motes()
	_play_intro_fade()
	_build_continue_prompt()
	print("=== MEMORIA: The Price of Oblivion ===")

func _setup_background() -> void:
	# Dark fantasy 배경 슬라이드(cover + cg 후보)
	_bg_primary = TextureRect.new()
	_bg_primary.set_anchors_preset(PRESET_FULL_RECT)
	_bg_primary.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg_primary.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg_primary.z_index = -2
	_bg_primary.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_bg_primary)

	_bg_secondary = TextureRect.new()
	_bg_secondary.set_anchors_preset(PRESET_FULL_RECT)
	_bg_secondary.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg_secondary.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg_secondary.z_index = -1
	_bg_secondary.mouse_filter = MOUSE_FILTER_IGNORE
	_bg_secondary.modulate.a = 0.0
	add_child(_bg_secondary)

	_bg_candidates = _get_intro_background_candidates()
	if _bg_candidates.is_empty():
		_bg_candidates = ["res://assets/cg/cover.png"]
	_apply_bg_texture(_bg_primary, _bg_candidates[0])

	# 어두운 오버레이 (메뉴 가독성)
	_overlay_rect = ColorRect.new()
	_overlay_rect.set_anchors_preset(PRESET_FULL_RECT)
	_overlay_rect.color = Color(0, 0, 0, 0.45)
	_overlay_rect.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_overlay_rect)
	move_child(_overlay_rect, 1)

func _setup_menu() -> void:
	# NG+ 버튼 동적 추가 (New Game 아래)
	if GameManager.is_ng_plus_unlocked():
		ng_plus_btn = Button.new()
		ng_plus_btn.text = "New Game+"
		ng_plus_btn.pressed.connect(_on_ng_plus_pressed)
		# New Game 버튼 바로 아래에 삽입
		var new_game_idx = 0
		for i in range($VBoxContainer.get_child_count()):
			var child = $VBoxContainer.get_child(i)
			if child is Button and child.text == "New Game":
				new_game_idx = i + 1
				break
		$VBoxContainer.add_child(ng_plus_btn)
		$VBoxContainer.move_child(ng_plus_btn, new_game_idx)

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
	GameManager.ng_plus_cycle = 0
	GameManager.player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": 0,
		"elia_with_party": true,
		"items": {},
	}
	SceneTransition.change_scene("res://scenes/maps/rim_forest.tscn")

func _on_continue_pressed() -> void:
	SaveManager.load_game(1)

func _on_options_pressed() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_ng_plus_pressed() -> void:
	GameManager.start_new_game_plus()
	SceneTransition.change_scene("res://scenes/maps/rim_forest.tscn")

func _on_quit_pressed() -> void:
	get_tree().quit()


func _input(event: InputEvent) -> void:
	if _intro_skipped:
		return
	if event.is_action_pressed("ui_accept") or event.is_action_pressed("interact"):
		_skip_intro()

func _process(delta: float) -> void:
	_intro_t += delta
	if _overlay_rect:
		_overlay_rect.color.a = 0.43 + sin(_intro_t * 0.4) * 0.04
	if _title_label:
		_title_label.modulate.a = 0.86 + sin(_intro_t * 1.3) * 0.08
	_bg_cycle += delta
	if _bg_candidates.size() > 1 and _bg_cycle >= 8.0:
		_bg_cycle = 0.0
		_cycle_background()

	if _prompt_label:
		_prompt_label.modulate.a = 0.35 + sin(_intro_t * 2.4) * 0.25

	for m in _motes:
		if not is_instance_valid(m):
			continue
		var speed: float = m.get_meta("speed", 8.0)
		var phase: float = m.get_meta("phase", 0.0)
		m.position.y -= speed * delta
		m.position.x += sin(_intro_t * 0.8 + phase) * 6.0 * delta
		m.modulate.a = 0.2 + sin(_intro_t * 0.9 + phase) * 0.08
		if m.position.y < -12:
			m.position.y = 740
			m.position.x = randf_range(0, 1280)

func _build_title_overlay() -> void:
	_title_label = Label.new()
	_title_label.text = "MEMORIA"
	_title_label.set_anchors_preset(PRESET_TOP_WIDE)
	_title_label.offset_top = 78
	_title_label.offset_bottom = 150
	_title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_title_label.add_theme_font_size_override("font_size", 56)
	_title_label.add_theme_color_override("font_color", Color(0.95, 0.85, 0.7))
	_title_label.modulate.a = 0.0
	add_child(_title_label)

	_subtitle_label = Label.new()
	_subtitle_label.text = "The Price of Oblivion"
	_subtitle_label.set_anchors_preset(PRESET_TOP_WIDE)
	_subtitle_label.offset_top = 146
	_subtitle_label.offset_bottom = 176
	_subtitle_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_subtitle_label.add_theme_font_size_override("font_size", 20)
	_subtitle_label.add_theme_color_override("font_color", Color(0.72, 0.68, 0.62))
	_subtitle_label.modulate.a = 0.0
	add_child(_subtitle_label)

func _spawn_ambient_motes() -> void:
	for i in range(28):
		var m = ColorRect.new()
		m.size = Vector2(randf_range(1.5, 3.0), randf_range(1.5, 3.0))
		m.color = Color(0.85, 0.78, 0.66, 0.22)
		m.position = Vector2(randf_range(0, 1280), randf_range(0, 720))
		m.mouse_filter = MOUSE_FILTER_IGNORE
		m.z_index = 2
		m.set_meta("speed", randf_range(6.0, 16.0))
		m.set_meta("phase", randf() * TAU)
		add_child(m)
		_motes.append(m)

func _play_intro_fade() -> void:
	if _title_label == null or _subtitle_label == null:
		return
	$VBoxContainer.modulate.a = 0.0
	_intro_tween = create_tween()
	_intro_tween.set_parallel(true)
	_intro_tween.tween_property(_title_label, "modulate:a", 1.0, 0.8)
	_intro_tween.tween_property(_subtitle_label, "modulate:a", 1.0, 1.0)
	_intro_tween.set_parallel(false)
	_intro_tween.tween_interval(0.25)
	_intro_tween.tween_property($VBoxContainer, "modulate:a", 1.0, 0.4)
	_intro_tween.tween_callback(func():
		if _prompt_label:
			_prompt_label.visible = true
	)


func _get_intro_background_candidates() -> Array[String]:
	var candidates: Array[String] = ["res://assets/cg/cover.png"]
	var dir = DirAccess.open("res://assets/cg")
	if dir == null:
		return candidates
	dir.list_dir_begin()
	while true:
		var f = dir.get_next()
		if f == "":
			break
		if dir.current_is_dir():
			continue
		var lf = f.to_lower()
		if lf.ends_with(".png") or lf.ends_with(".jpg") or lf.ends_with(".jpeg"):
			if lf.find("cover") >= 0 or lf.find("void") >= 0 or lf.find("forest") >= 0 or lf.find("wasteland") >= 0 or lf.find("coast") >= 0:
				candidates.append("res://assets/cg/" + f)
	dir.list_dir_end()
	return candidates

func _apply_bg_texture(node: TextureRect, path: String) -> void:
	if node == null:
		return
	if ResourceLoader.exists(path):
		node.texture = load(path)

func _cycle_background() -> void:
	if _bg_primary == null or _bg_secondary == null or _bg_candidates.is_empty():
		return
	_bg_index = (_bg_index + 1) % _bg_candidates.size()
	_apply_bg_texture(_bg_secondary, _bg_candidates[_bg_index])
	_bg_secondary.modulate.a = 0.0
	var t = create_tween()
	t.set_parallel(true)
	t.tween_property(_bg_secondary, "modulate:a", 1.0, 1.8)
	t.tween_property(_bg_primary, "modulate:a", 0.0, 1.8)
	t.set_parallel(false)
	t.tween_callback(func():
		var tmp = _bg_primary
		_bg_primary = _bg_secondary
		_bg_secondary = tmp
		_bg_secondary.modulate.a = 0.0
		_bg_primary.modulate.a = 1.0
	)


func _build_continue_prompt() -> void:
	_prompt_label = Label.new()
	_prompt_label.text = "Press Enter to Begin"
	_prompt_label.set_anchors_preset(PRESET_BOTTOM_WIDE)
	_prompt_label.offset_top = -42
	_prompt_label.offset_bottom = -10
	_prompt_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_prompt_label.add_theme_font_size_override("font_size", 14)
	_prompt_label.add_theme_color_override("font_color", Color(0.78, 0.72, 0.62))
	_prompt_label.visible = false
	add_child(_prompt_label)

func _skip_intro() -> void:
	_intro_skipped = true
	if _intro_tween:
		_intro_tween.kill()
	if _title_label:
		_title_label.modulate.a = 1.0
	if _subtitle_label:
		_subtitle_label.modulate.a = 1.0
	$VBoxContainer.modulate.a = 1.0
	if _prompt_label:
		_prompt_label.visible = true
	AudioManager.play_sfx("ui_select")
