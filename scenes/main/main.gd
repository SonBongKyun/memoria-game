## Main Scene — 타이틀 화면 (S55: Steam-quality redesign)
## Animated ash particles, glow title, sequenced fade-in, "Press Any Key", version display, ambient wind.
extends Control

@onready var continue_btn: Button = $VBoxContainer/ContinueButton
var ng_plus_btn: Button = null
var boss_rush_btn: Button = null

# S55: Title screen nodes
var _bg: TextureRect
var _overlay: ColorRect
var _ash_particles: GPUParticles2D
var _title_label: Label
var _title_glow_label: Label  # duplicate for glow effect
var _press_any_key: Label
var _menu_container: VBoxContainer
var _version_label: Label
var _wind_player: AudioStreamPlayer

# S55: State machine for intro sequence
enum IntroState { FADE_BG, FADE_TITLE, PRESS_ANY_KEY, SHOW_MENU }
var _intro_state: int = IntroState.FADE_BG
var _menu_ready: bool = false
var _glow_tween: Tween
var _pak_tween: Tween  # press-any-key fade tween

const GAME_VERSION: String = "v0.9.0"

# S59: Splash screen state
var _splash_shown: bool = false
var _splash_overlay: ColorRect

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_build_title_screen()
	# S59: Show splash screen first, then intro sequence
	if not _splash_shown:
		_show_splash_screen()
	else:
		_start_intro_sequence()
		_play_ambient_wind()
	print("=== MEMORIA: The Price of Oblivion ===")

## ===================== BUILD =====================

func _build_title_screen() -> void:
	# --- Background ---
	_bg = TextureRect.new()
	_bg.set_anchors_preset(PRESET_FULL_RECT)
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	var tex = load("res://assets/cg/cover.png")
	if tex:
		_bg.texture = tex
	else:
		var fallback = ColorRect.new()
		fallback.set_anchors_preset(PRESET_FULL_RECT)
		fallback.color = Color(0.08, 0.08, 0.1)
		add_child(fallback)
	_bg.z_index = -1
	_bg.mouse_filter = MOUSE_FILTER_IGNORE
	_bg.modulate.a = 0.0  # start invisible for fade-in
	add_child(_bg)
	move_child(_bg, 0)

	# Dark overlay for readability
	_overlay = ColorRect.new()
	_overlay.set_anchors_preset(PRESET_FULL_RECT)
	_overlay.color = Color(0, 0, 0, 0.5)
	_overlay.mouse_filter = MOUSE_FILTER_IGNORE
	_overlay.modulate.a = 0.0
	add_child(_overlay)
	move_child(_overlay, 1)

	# --- Ash Particles ---
	_ash_particles = GPUParticles2D.new()
	_ash_particles.z_index = 0
	_ash_particles.amount = 60
	_ash_particles.lifetime = 8.0
	_ash_particles.preprocess = 4.0
	_ash_particles.visibility_rect = Rect2(-640, -360, 1280, 720)
	_ash_particles.position = Vector2(640, 360)

	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(-1.0, 0.3, 0.0)
	mat.spread = 25.0
	mat.initial_velocity_min = 15.0
	mat.initial_velocity_max = 35.0
	mat.gravity = Vector3(0, 3.0, 0)
	mat.scale_min = 0.5
	mat.scale_max = 1.5
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(700, 400, 0)
	mat.color = Color(0.6, 0.5, 0.4, 0.35)

	# Color gradient: fade in and out
	var color_curve = GradientTexture1D.new()
	var grad = Gradient.new()
	grad.set_offset(0, 0.0)
	grad.set_color(0, Color(0.7, 0.55, 0.4, 0.0))
	grad.add_point(0.15, Color(0.7, 0.55, 0.4, 0.3))
	grad.add_point(0.7, Color(0.5, 0.4, 0.35, 0.25))
	grad.set_offset(grad.get_point_count() - 1, 1.0)
	grad.set_color(grad.get_point_count() - 1, Color(0.4, 0.35, 0.3, 0.0))
	color_curve.gradient = grad
	mat.color_ramp = color_curve

	_ash_particles.process_material = mat

	add_child(_ash_particles)

	# --- Title (behind glow) ---
	# S71: 타이틀 — 더 크고 자간 강조. theme.tres가 stylized serif 자동 적용
	_title_glow_label = Label.new()
	_title_glow_label.text = "M E M O R I A"
	_title_glow_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_title_glow_label.set_anchors_preset(PRESET_CENTER_TOP)
	_title_glow_label.position = Vector2(-300, 92)
	_title_glow_label.size = Vector2(600, 100)
	_title_glow_label.add_theme_font_size_override("font_size", 72)
	_title_glow_label.add_theme_color_override("font_color", Color(0.75, 0.55, 0.25, 0.45))
	_title_glow_label.modulate.a = 0.0
	_title_glow_label.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_title_glow_label)

	_title_label = Label.new()
	_title_label.text = "M E M O R I A"
	_title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_title_label.set_anchors_preset(PRESET_CENTER_TOP)
	_title_label.position = Vector2(-300, 92)
	_title_label.size = Vector2(600, 100)
	_title_label.add_theme_font_size_override("font_size", 72)
	_title_label.add_theme_color_override("font_color", Color(0.95, 0.82, 0.5))
	_title_label.add_theme_color_override("font_outline_color", Color(0.18, 0.12, 0.08))
	_title_label.add_theme_constant_override("outline_size", 4)
	_title_label.modulate.a = 0.0
	_title_label.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_title_label)

	# Subtitle
	var subtitle = Label.new()
	# S65 (A안): VN 정체성 강화 서브타이틀 — 한 줄 카피로 게임 본질 전달
	subtitle.text = "The Price of Oblivion  ·  A story of what you choose to forget"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.set_anchors_preset(PRESET_CENTER_TOP)
	subtitle.position = Vector2(-300, 160)
	subtitle.size = Vector2(600, 40)
	subtitle.add_theme_font_size_override("font_size", 16)
	subtitle.add_theme_color_override("font_color", Color(0.6, 0.5, 0.4, 0.75))
	subtitle.modulate.a = 0.0
	subtitle.name = "SubtitleLabel"
	subtitle.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(subtitle)

	# --- Press Any Key ---
	_press_any_key = Label.new()
	_press_any_key.text = "Press Any Key"
	_press_any_key.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_press_any_key.set_anchors_preset(PRESET_CENTER_BOTTOM)
	_press_any_key.position = Vector2(-150, -120)
	_press_any_key.size = Vector2(300, 40)
	_press_any_key.add_theme_font_size_override("font_size", 16)
	_press_any_key.add_theme_color_override("font_color", Color(0.65, 0.58, 0.45, 0.8))
	_press_any_key.modulate.a = 0.0
	_press_any_key.visible = false
	_press_any_key.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_press_any_key)

	# --- Version label (bottom-right) ---
	_version_label = Label.new()
	_version_label.text = GAME_VERSION
	_version_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_version_label.anchor_left = 1.0
	_version_label.anchor_top = 1.0
	_version_label.anchor_right = 1.0
	_version_label.anchor_bottom = 1.0
	_version_label.offset_left = -120
	_version_label.offset_top = -30
	_version_label.offset_right = -12
	_version_label.offset_bottom = -8
	_version_label.add_theme_font_size_override("font_size", 11)
	_version_label.add_theme_color_override("font_color", Color(0.4, 0.38, 0.35, 0.5))
	_version_label.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(_version_label)

	# --- Menu Buttons (initially hidden) ---
	_setup_menu()

## ===================== INTRO SEQUENCE =====================

func _start_intro_sequence() -> void:
	_intro_state = IntroState.FADE_BG

	# Phase 1: Fade in background (1.2s)
	var tween = create_tween()
	tween.tween_property(_bg, "modulate:a", 1.0, 1.2).set_ease(Tween.EASE_OUT)
	tween.parallel().tween_property(_overlay, "modulate:a", 1.0, 1.2).set_ease(Tween.EASE_OUT)

	# Phase 2: Fade in title (0.8s after bg)
	tween.tween_callback(func(): _intro_state = IntroState.FADE_TITLE)
	tween.tween_property(_title_label, "modulate:a", 1.0, 0.8).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	tween.parallel().tween_property(_title_glow_label, "modulate:a", 1.0, 0.8).set_ease(Tween.EASE_OUT)
	# Subtitle
	var sub = get_node_or_null("SubtitleLabel")
	if sub:
		tween.parallel().tween_property(sub, "modulate:a", 1.0, 1.0).set_ease(Tween.EASE_OUT)

	# Phase 3: Show "Press Any Key" (0.3s after title)
	tween.tween_interval(0.3)
	tween.tween_callback(_show_press_any_key)

func _show_press_any_key() -> void:
	_intro_state = IntroState.PRESS_ANY_KEY
	_press_any_key.visible = true
	_press_any_key.modulate.a = 0.0
	# Fade pulse loop
	_start_pak_pulse()

func _start_pak_pulse() -> void:
	if _pak_tween:
		_pak_tween.kill()
	_pak_tween = create_tween().set_loops()
	_pak_tween.tween_property(_press_any_key, "modulate:a", 0.9, 1.2).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
	_pak_tween.tween_property(_press_any_key, "modulate:a", 0.2, 1.2).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)

## Start the glow pulse on title (loops forever)
func _start_title_glow() -> void:
	if _glow_tween:
		_glow_tween.kill()
	_glow_tween = create_tween().set_loops()
	_glow_tween.tween_property(_title_glow_label, "modulate:a", 0.6, 2.0).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
	_glow_tween.tween_property(_title_glow_label, "modulate:a", 0.25, 2.0).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)

func _show_menu() -> void:
	if _menu_ready:
		return
	_menu_ready = true
	_intro_state = IntroState.SHOW_MENU

	# Hide press any key
	if _pak_tween:
		_pak_tween.kill()
	var hide_tween = create_tween()
	hide_tween.tween_property(_press_any_key, "modulate:a", 0.0, 0.2)
	hide_tween.tween_callback(func(): _press_any_key.visible = false)

	# Start title glow pulse
	_start_title_glow()

	# Sequentially fade in each button
	$VBoxContainer.visible = true
	var delay = 0.0
	for child in $VBoxContainer.get_children():
		if child is Button:
			child.modulate.a = 0.0
			child.position.x = -20  # start offset left
			var btn_tween = create_tween()
			btn_tween.tween_interval(delay)
			btn_tween.tween_property(child, "modulate:a", 1.0, 0.3).set_ease(Tween.EASE_OUT)
			btn_tween.parallel().tween_property(child, "position:x", 0.0, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
			delay += 0.1

	AudioManager.play_sfx("ui_open")

## ===================== AMBIENT WIND SFX =====================

func _play_ambient_wind() -> void:
	_wind_player = AudioStreamPlayer.new()
	_wind_player.bus = "Master"
	add_child(_wind_player)

	# Generate a looping wind noise
	var sample_rate: int = 22050
	var duration: float = 4.0  # 4-second loop
	var sample_count: int = int(sample_rate * duration)
	var samples := PackedFloat32Array()
	samples.resize(sample_count)

	# Low-frequency filtered noise for wind
	var prev: float = 0.0
	var alpha: float = 0.04  # low-pass filter coefficient
	for i in range(sample_count):
		var t: float = float(i) / sample_rate
		var raw: float = randf_range(-1.0, 1.0)
		# Low-pass filter
		prev = prev + alpha * (raw - prev)
		# Slow volume modulation for natural feel
		var mod: float = 0.5 + 0.5 * sin(t * 0.4 * TAU)
		samples[i] = prev * mod * 0.12  # very quiet

	# Convert to 16-bit PCM
	var byte_data := PackedByteArray()
	for s in samples:
		var val: int = int(clampf(s, -1.0, 1.0) * 32767)
		byte_data.append(val & 0xFF)
		byte_data.append((val >> 8) & 0xFF)

	var stream := AudioStreamWAV.new()
	stream.data = byte_data
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = sample_rate
	stream.stereo = false
	stream.loop_mode = AudioStreamWAV.LOOP_FORWARD
	stream.loop_begin = 0
	stream.loop_end = sample_count

	_wind_player.stream = stream
	# Respect SFX volume
	var sfx_vol: float = OptionsMenu.settings.get("sfx_volume", 80) / 100.0 if OptionsMenu else 0.8
	_wind_player.volume_db = linear_to_db(sfx_vol * 0.3)
	_wind_player.play()

## ===================== INPUT =====================

func _unhandled_input(event: InputEvent) -> void:
	if _intro_state == IntroState.PRESS_ANY_KEY and not _menu_ready:
		if event is InputEventKey or event is InputEventMouseButton or event is InputEventJoypadButton:
			if event.is_pressed():
				_show_menu()
				get_viewport().set_input_as_handled()

## ===================== MENU SETUP =====================

func _setup_menu() -> void:
	$VBoxContainer.visible = false  # Hidden until after Press Any Key

	# S65 (A안 피벗): NG+/Boss Rush 타이틀 노출 제거 — VN 정체성에 맞지 않음.
	# 코드는 보존, UI 진입점만 차단. 후속 컨텐츠 패치에서 재활성 검토.

	# Style all buttons with hover animations
	for btn in $VBoxContainer.get_children():
		if btn is Button:
			_style_button(btn)

	# Continue button activation + S58: show play time on continue button
	if continue_btn:
		continue_btn.disabled = not SaveManager.has_save(1)
		if continue_btn.disabled:
			continue_btn.modulate.a = 0.4
		else:
			# Show play time from save data on the continue button
			var save_info = SaveManager.get_save_info(1)
			if not save_info.is_empty():
				var play_time_str = ""
				var game_data_in_save = save_info  # get_save_info returns flat dict
				# Try to get play time from the full save file
				var save_path = SaveManager._get_save_path(1)
				if FileAccess.file_exists(save_path):
					var _file = FileAccess.open(save_path, FileAccess.READ)
					if _file:
						var _json = JSON.new()
						if _json.parse(_file.get_as_text()) == OK and _json.data is Dictionary:
							var gd = _json.data.get("game", {})
							var ps = gd.get("play_stats", {})
							var secs = ps.get("play_time_seconds", 0.0)
							if secs > 0:
								var h = int(secs) / 3600
								var m = (int(secs) % 3600) / 60
								play_time_str = " (%dh %02dm)" % [h, m]
						_file.close()
				var ch = save_info.get("chapter", 1)
				var loc_name = save_info.get("location", "")
				if loc_name != "":
					continue_btn.text = "Continue — Ch.%d %s%s" % [ch, loc_name, play_time_str]
				elif play_time_str != "":
					continue_btn.text = "Continue%s" % play_time_str

func _style_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(220, 48)
	btn.add_theme_font_size_override("font_size", 18)

	# Normal style
	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0.08, 0.07, 0.1, 0.85)
	normal.border_color = Color(0.5, 0.38, 0.2, 0.6)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(3)
	normal.set_content_margin_all(10)
	btn.add_theme_stylebox_override("normal", normal)

	# Hover style
	var hover = normal.duplicate()
	hover.bg_color = Color(0.12, 0.1, 0.15, 0.95)
	hover.border_color = Color(0.75, 0.55, 0.25, 0.9)
	hover.border_width_bottom = 2
	hover.border_width_left = 2
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	# Pressed style
	var pressed = normal.duplicate()
	pressed.bg_color = Color(0.15, 0.12, 0.08, 0.95)
	pressed.border_color = Color(0.85, 0.65, 0.3, 1.0)
	btn.add_theme_stylebox_override("pressed", pressed)

	btn.add_theme_color_override("font_color", Color(0.75, 0.68, 0.55))
	btn.add_theme_color_override("font_hover_color", Color(0.95, 0.82, 0.5))
	btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.9, 0.6))

	# S55: Hover animation — scale pulse + slide highlight
	btn.mouse_entered.connect(func():
		AudioManager.play_sfx("ui_hover")
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(1.03, 1.03), 0.12).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	)
	btn.mouse_exited.connect(func():
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.15).set_ease(Tween.EASE_OUT)
	)
	btn.focus_entered.connect(func():
		AudioManager.play_sfx("ui_hover")
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(1.03, 1.03), 0.12).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	)
	btn.focus_exited.connect(func():
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.15).set_ease(Tween.EASE_OUT)
	)

	# S57: Button press scale feedback (squish on press)
	btn.button_down.connect(func():
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(0.95, 0.95), 0.05)
	)
	btn.button_up.connect(func():
		var t = create_tween()
		t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.08).set_ease(Tween.EASE_OUT)
	)

	# Set pivot to center for scaling
	btn.pivot_offset = btn.custom_minimum_size / 2.0

## ===================== CALLBACKS =====================

func _on_new_game_pressed() -> void:
	_stop_ambient()
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
	# S60: VN 프롤로그 자동 재생하도록 플래그 설정 후 vn_host로 전환
	# (change_scene_to_file 이후 self가 해제되므로 이후 await 금지)
	SceneFlow.pending_scene_id = "ch1_prologue"
	SceneTransition.change_scene_styled("res://scenes/main/vn_host.tscn")

func _on_continue_pressed() -> void:
	_stop_ambient()
	SaveManager.load_game(1)

func _on_options_pressed() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_ng_plus_pressed() -> void:
	_stop_ambient()
	GameManager.start_new_game_plus()
	SceneTransition.change_scene_styled("res://scenes/maps/rim_forest.tscn")

func _on_boss_rush_pressed() -> void:
	_stop_ambient()
	GameManager.story_flags.clear()
	GameManager.current_chapter = 10
	GameManager.player_data.hp = 200
	GameManager.player_data.max_hp = 200
	GameManager.start_boss_rush()

func _on_quit_pressed() -> void:
	get_tree().quit()

func _stop_ambient() -> void:
	if _wind_player and _wind_player.playing:
		_wind_player.stop()

## ===================== S59: SPLASH SCREEN =====================

func _show_splash_screen() -> void:
	_splash_shown = true

	# Cover everything with a black overlay
	_splash_overlay = ColorRect.new()
	_splash_overlay.set_anchors_preset(PRESET_FULL_RECT)
	_splash_overlay.color = Color(0.02, 0.02, 0.04)
	_splash_overlay.z_index = 100
	_splash_overlay.mouse_filter = MOUSE_FILTER_STOP
	add_child(_splash_overlay)

	# "Made with Godot" label
	var godot_label = Label.new()
	godot_label.text = "Made with Godot"
	godot_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	godot_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	godot_label.set_anchors_preset(PRESET_CENTER)
	godot_label.position = Vector2(-150, -20)
	godot_label.size = Vector2(300, 40)
	godot_label.add_theme_font_size_override("font_size", 22)
	godot_label.add_theme_color_override("font_color", Color(0.7, 0.75, 0.85, 0.0))
	godot_label.mouse_filter = MOUSE_FILTER_IGNORE
	_splash_overlay.add_child(godot_label)

	# Subtle version text below
	var ver_label = Label.new()
	ver_label.text = "Engine 4.6"
	ver_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	ver_label.set_anchors_preset(PRESET_CENTER)
	ver_label.position = Vector2(-100, 18)
	ver_label.size = Vector2(200, 24)
	ver_label.add_theme_font_size_override("font_size", 12)
	ver_label.add_theme_color_override("font_color", Color(0.45, 0.48, 0.55, 0.0))
	ver_label.mouse_filter = MOUSE_FILTER_IGNORE
	_splash_overlay.add_child(ver_label)

	# Animate: fade in (0.4s), hold (0.8s), fade out (0.3s)
	var splash_tween = create_tween()
	splash_tween.tween_property(godot_label, "theme_override_colors/font_color",
		Color(0.7, 0.75, 0.85, 1.0), 0.4).set_ease(Tween.EASE_OUT)
	splash_tween.parallel().tween_property(ver_label, "theme_override_colors/font_color",
		Color(0.45, 0.48, 0.55, 0.7), 0.4).set_ease(Tween.EASE_OUT)
	splash_tween.tween_interval(0.8)
	splash_tween.tween_property(godot_label, "theme_override_colors/font_color",
		Color(0.7, 0.75, 0.85, 0.0), 0.3).set_ease(Tween.EASE_IN)
	splash_tween.parallel().tween_property(ver_label, "theme_override_colors/font_color",
		Color(0.45, 0.48, 0.55, 0.0), 0.3).set_ease(Tween.EASE_IN)
	splash_tween.tween_callback(func():
		_splash_overlay.queue_free()
		_start_intro_sequence()
		_play_ambient_wind()
	)
