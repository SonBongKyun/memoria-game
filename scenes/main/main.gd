## Main title screen.
## S225: Cinematic title composition built around the existing painterly key art.
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
var _rift_glow: TextureRect
var _ash_particles: GPUParticles2D
var _ash_material: ParticleProcessMaterial
var _title_stack: VBoxContainer
var _title_rail: TextureRect
var _menu_backdrop: PanelContainer
var _menu_heading: Label
var _menu_kicker: Label
var _menu_footer: Label
var _menu_rule: ColorRect
var _letterbox_top: ColorRect
var _letterbox_bottom: ColorRect
var _aftermath_btn: Button
var _ambient_time := 0.0
var _parallax_offset := Vector2.ZERO
var _reduce_motion := false
var _intro_complete := false

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_reduce_motion = OptionsMenu != null and OptionsMenu.is_reduce_motion()
	_build_background()
	_build_title_copy()
	_setup_menu()
	_fade_in_title()
	_play_title_bgm()
	resized.connect(_layout_cinematic_canvas)
	_layout_cinematic_canvas()
	set_process(true)
	print("=== MEMORIA: The Price of Oblivion ===")

func _build_background() -> void:
	_bg = TextureRect.new()
	_bg.name = "CinematicBackground"
	_bg.set_anchors_preset(PRESET_FULL_RECT)
	_bg.offset_left = -18.0
	_bg.offset_top = -14.0
	_bg.offset_right = 18.0
	_bg.offset_bottom = 14.0
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg.mouse_filter = MOUSE_FILTER_IGNORE
	_bg.z_index = -20
	_bg.modulate.a = 1.0
	if ResourceLoader.exists(TITLE_BG_PATH):
		_bg.texture = load(TITLE_BG_PATH)
	add_child(_bg)
	move_child(_bg, 0)

	_rift_glow = TextureRect.new()
	_rift_glow.name = "MemoryRiftGlow"
	_rift_glow.anchor_left = 0.315
	_rift_glow.anchor_right = 0.535
	_rift_glow.anchor_top = -0.06
	_rift_glow.anchor_bottom = 1.04
	_rift_glow.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_rift_glow.stretch_mode = TextureRect.STRETCH_SCALE
	_rift_glow.texture = _make_radial_texture(
		PackedFloat32Array([0.0, 0.24, 0.58, 1.0]),
		PackedColorArray([
			Color(1.0, 0.77, 0.34, 0.18),
			Color(0.66, 0.38, 0.85, 0.12),
			Color(0.19, 0.09, 0.34, 0.045),
			Color(0.0, 0.0, 0.0, 0.0),
		])
	)
	var glow_material := CanvasItemMaterial.new()
	glow_material.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	_rift_glow.material = glow_material
	_rift_glow.mouse_filter = MOUSE_FILTER_IGNORE
	_rift_glow.z_index = -19
	add_child(_rift_glow)

	_build_ash_particles()

	_shade = ColorRect.new()
	_shade.name = "CinematicVignette"
	_shade.set_anchors_preset(PRESET_FULL_RECT)
	_shade.color = Color.WHITE
	_shade.material = _make_vignette_material()
	_shade.mouse_filter = MOUSE_FILTER_IGNORE
	_shade.z_index = -16
	add_child(_shade)

	_letterbox_top = _make_letterbox("LetterboxTop")
	_letterbox_top.anchor_bottom = 0.0
	_letterbox_top.offset_bottom = 24.0
	add_child(_letterbox_top)

	_letterbox_bottom = _make_letterbox("LetterboxBottom")
	_letterbox_bottom.anchor_top = 1.0
	_letterbox_bottom.anchor_bottom = 1.0
	_letterbox_bottom.offset_top = -24.0
	add_child(_letterbox_bottom)

func _make_radial_texture(offsets: PackedFloat32Array, colors: PackedColorArray) -> GradientTexture2D:
	var gradient := Gradient.new()
	gradient.offsets = offsets
	gradient.colors = colors
	var texture := GradientTexture2D.new()
	texture.gradient = gradient
	texture.width = 512
	texture.height = 512
	texture.fill = GradientTexture2D.FILL_RADIAL
	texture.fill_from = Vector2(0.5, 0.5)
	texture.fill_to = Vector2(0.98, 0.5)
	return texture

func _make_vignette_material() -> ShaderMaterial:
	var shader := Shader.new()
	shader.code = """
shader_type canvas_item;

void fragment() {
	vec2 centered = UV * 2.0 - 1.0;
	float oval = length(centered * vec2(0.76, 1.0));
	float edge = smoothstep(0.34, 1.12, oval);
	float bars = pow(abs(centered.y), 3.2) * 0.22;
	float left_shelter = (1.0 - smoothstep(0.0, 0.42, UV.x)) * 0.16;
	float right_shelter = smoothstep(0.58, 1.0, UV.x) * 0.24;
	float rift = exp(-42.0 * pow(UV.x - 0.425, 2.0) - 5.0 * pow(UV.y - 0.5, 2.0));
	float alpha = clamp(0.10 + edge * 0.58 + bars + left_shelter + right_shelter - rift * 0.08, 0.0, 0.84);
	COLOR = vec4(0.006, 0.005, 0.014, alpha);
}
"""
	var material := ShaderMaterial.new()
	material.shader = shader
	return material

func _build_ash_particles() -> void:
	_ash_particles = GPUParticles2D.new()
	_ash_particles.name = "TitleAsh"
	_ash_particles.amount = 30
	_ash_particles.lifetime = 9.0
	_ash_particles.randomness = 0.72
	_ash_particles.preprocess = 9.0
	_ash_particles.visibility_rect = Rect2(-900.0, -520.0, 1800.0, 1040.0)
	_ash_particles.texture = _make_radial_texture(
		PackedFloat32Array([0.0, 0.34, 1.0]),
		PackedColorArray([
			Color(1.0, 0.91, 0.70, 0.9),
			Color(0.74, 0.58, 0.42, 0.62),
			Color(0.15, 0.10, 0.18, 0.0),
		])
	)
	_ash_material = ParticleProcessMaterial.new()
	_ash_material.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	_ash_material.direction = Vector3(-0.18, -1.0, 0.0)
	_ash_material.spread = 26.0
	_ash_material.initial_velocity_min = 4.0
	_ash_material.initial_velocity_max = 12.0
	_ash_material.gravity = Vector3(-0.7, -1.8, 0.0)
	_ash_material.scale_min = 0.45
	_ash_material.scale_max = 1.1
	_ash_material.color = Color(0.78, 0.69, 0.62, 0.34)
	_ash_particles.process_material = _ash_material
	_ash_particles.emitting = not _reduce_motion
	_ash_particles.z_index = -17
	add_child(_ash_particles)

func _make_letterbox(node_name: String) -> ColorRect:
	var strip := ColorRect.new()
	strip.name = node_name
	strip.anchor_left = 0.0
	strip.anchor_right = 1.0
	strip.color = Color(0.004, 0.003, 0.008, 0.94)
	strip.mouse_filter = MOUSE_FILTER_IGNORE
	strip.z_index = 20
	return strip

func _build_title_copy() -> void:
	_title_rail = TextureRect.new()
	_title_rail.name = "TitleGoldRail"
	_title_rail.anchor_left = 0.043
	_title_rail.anchor_right = 0.046
	_title_rail.anchor_top = 0.105
	_title_rail.anchor_bottom = 0.425
	_title_rail.texture = _make_vertical_gradient_texture()
	_title_rail.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_title_rail.mouse_filter = MOUSE_FILTER_IGNORE
	_title_rail.z_index = 2
	add_child(_title_rail)

	_title_stack = VBoxContainer.new()
	_title_stack.name = "TitleStack"
	_title_stack.anchor_left = 0.055
	_title_stack.anchor_right = 0.56
	_title_stack.anchor_top = 0.105
	_title_stack.anchor_bottom = 0.435
	_title_stack.add_theme_constant_override("separation", 5)
	_title_stack.mouse_filter = MOUSE_FILTER_IGNORE
	_title_stack.z_index = 3
	add_child(_title_stack)

	var eyebrow := Label.new()
	eyebrow.name = "TitleEyebrow"
	eyebrow.text = "A DARK FANTASY OF MEMORY AND LOSS" if GameManager.current_locale != "ko" else "기억과 상실의 다크 판타지"
	UITheme.apply_ui_font(eyebrow)
	eyebrow.add_theme_font_size_override("font_size", 13)
	eyebrow.add_theme_color_override("font_color", Color(0.73, 0.67, 0.60, 0.9))
	eyebrow.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.9))
	eyebrow.add_theme_constant_override("shadow_outline_size", 2)
	_title_stack.add_child(eyebrow)

	var title := Label.new()
	title.name = "TitleWordmark"
	title.text = "MEMORIA"
	UITheme.apply_title_font(title)
	title.add_theme_font_size_override("font_size", 78)
	title.add_theme_color_override("font_color", Color(0.96, 0.82, 0.52))
	title.add_theme_color_override("font_outline_color", Color(0.11, 0.055, 0.025, 0.92))
	title.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.9))
	title.add_theme_constant_override("outline_size", 2)
	title.add_theme_constant_override("shadow_outline_size", 7)
	title.add_theme_constant_override("shadow_offset_x", 3)
	title.add_theme_constant_override("shadow_offset_y", 5)
	_title_stack.add_child(title)

	var subtitle := Label.new()
	subtitle.name = "TitleSubtitle"
	subtitle.text = GameManager.loc("title_subtitle")
	UITheme.apply_title_font(subtitle)
	subtitle.add_theme_font_size_override("font_size", 21)
	subtitle.add_theme_color_override("font_color", Color(0.91, 0.85, 0.75))
	subtitle.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.82))
	subtitle.add_theme_constant_override("shadow_outline_size", 3)
	subtitle.add_theme_constant_override("shadow_offset_x", 1)
	subtitle.add_theme_constant_override("shadow_offset_y", 2)
	_title_stack.add_child(subtitle)

	var divider := HBoxContainer.new()
	divider.name = "TitleDivider"
	divider.custom_minimum_size = Vector2(360.0, 12.0)
	divider.add_theme_constant_override("separation", 8)
	_title_stack.add_child(divider)
	var line_left := ColorRect.new()
	line_left.custom_minimum_size = Vector2(116.0, 1.0)
	line_left.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	line_left.color = Color(0.86, 0.64, 0.29, 0.72)
	divider.add_child(line_left)
	var diamond := Label.new()
	diamond.text = "◆"
	UITheme.apply_ui_font(diamond)
	diamond.add_theme_font_size_override("font_size", 10)
	diamond.add_theme_color_override("font_color", Color(0.95, 0.76, 0.38, 0.92))
	divider.add_child(diamond)
	var line_right := ColorRect.new()
	line_right.custom_minimum_size = Vector2(176.0, 1.0)
	line_right.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	line_right.color = Color(0.86, 0.64, 0.29, 0.42)
	divider.add_child(line_right)

	var tag := Label.new()
	tag.name = "TitleTagline"
	tag.text = GameManager.loc("title_tagline")
	UITheme.apply_body_font(tag)
	tag.add_theme_font_size_override("font_size", 15)
	tag.add_theme_color_override("font_color", Color(0.74, 0.70, 0.66))
	tag.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.70))
	tag.add_theme_constant_override("shadow_outline_size", 2)
	_title_stack.add_child(tag)

func _make_vertical_gradient_texture() -> GradientTexture2D:
	var gradient := Gradient.new()
	gradient.offsets = PackedFloat32Array([0.0, 0.18, 0.72, 1.0])
	gradient.colors = PackedColorArray([
		Color(0.84, 0.59, 0.23, 0.0),
		Color(0.95, 0.73, 0.35, 0.92),
		Color(0.58, 0.36, 0.16, 0.58),
		Color(0.20, 0.11, 0.06, 0.0),
	])
	var texture := GradientTexture2D.new()
	texture.gradient = gradient
	texture.width = 8
	texture.height = 256
	texture.fill = GradientTexture2D.FILL_LINEAR
	texture.fill_from = Vector2(0.5, 0.0)
	texture.fill_to = Vector2(0.5, 1.0)
	return texture

func _setup_menu() -> void:
	_build_menu_frame()
	menu_container.visible = true
	menu_container.anchor_left = 0.720
	menu_container.anchor_top = 0.490
	menu_container.anchor_right = 0.956
	menu_container.anchor_bottom = 0.875
	menu_container.offset_left = 0
	menu_container.offset_top = 0
	menu_container.offset_right = 0
	menu_container.offset_bottom = 0
	menu_container.alignment = BoxContainer.ALIGNMENT_CENTER
	menu_container.add_theme_constant_override("separation", 9)
	menu_container.modulate.a = 1.0
	menu_container.z_index = 4

	if _aftermath_btn == null:
		_aftermath_btn = Button.new()
		_aftermath_btn.name = "AftermathPreviewButton"
		_aftermath_btn.pressed.connect(_on_aftermath_preview_pressed)
		menu_container.add_child(_aftermath_btn)
		menu_container.move_child(_aftermath_btn, 2)

	var menu_copy := [
		GameManager.loc("new_game"),
		GameManager.loc("continue"),
		GameManager.loc("aftermath"),
		GameManager.loc("options"),
		GameManager.loc("quit"),
	]
	var menu_buttons: Array[Button] = [new_game_btn, continue_btn, _aftermath_btn, options_btn, quit_btn]
	for i in range(menu_buttons.size()):
		menu_buttons[i].text = "%02d    %s" % [i + 1, menu_copy[i]]

	for btn in menu_container.get_children():
		if btn is Button:
			_style_title_button(btn)

	continue_btn.disabled = not SaveManager.has_save(1)
	if continue_btn.disabled:
		continue_btn.mouse_default_cursor_shape = Control.CURSOR_ARROW

func _build_menu_frame() -> void:
	_menu_backdrop = PanelContainer.new()
	_menu_backdrop.name = "MenuBackdrop"
	_menu_backdrop.anchor_left = 0.685
	_menu_backdrop.anchor_right = 0.970
	_menu_backdrop.anchor_top = 0.390
	_menu_backdrop.anchor_bottom = 0.930
	_menu_backdrop.mouse_filter = MOUSE_FILTER_IGNORE
	_menu_backdrop.z_index = 1
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.018, 0.014, 0.027, 0.76)
	panel_style.border_color = Color(0.55, 0.40, 0.22, 0.54)
	panel_style.set_border_width_all(1)
	panel_style.set_border_width(SIDE_LEFT, 3)
	panel_style.set_corner_radius_all(2)
	panel_style.shadow_color = Color(0.0, 0.0, 0.0, 0.68)
	panel_style.shadow_size = 16
	panel_style.shadow_offset = Vector2(-6.0, 8.0)
	_menu_backdrop.add_theme_stylebox_override("panel", panel_style)
	add_child(_menu_backdrop)

	_menu_kicker = Label.new()
	_menu_kicker.name = "MenuKicker"
	_menu_kicker.anchor_left = 0.718
	_menu_kicker.anchor_right = 0.955
	_menu_kicker.anchor_top = 0.420
	_menu_kicker.anchor_bottom = 0.450
	_menu_kicker.text = "MEMORY ARCHIVE  //  00"
	UITheme.apply_ui_font(_menu_kicker)
	_menu_kicker.add_theme_font_size_override("font_size", 13)
	_menu_kicker.add_theme_color_override("font_color", Color(0.72, 0.60, 0.44, 0.92))
	_menu_kicker.mouse_filter = MOUSE_FILTER_IGNORE
	_menu_kicker.z_index = 3
	add_child(_menu_kicker)

	_menu_heading = Label.new()
	_menu_heading.name = "MenuHeading"
	_menu_heading.anchor_left = 0.718
	_menu_heading.anchor_right = 0.955
	_menu_heading.anchor_top = 0.448
	_menu_heading.anchor_bottom = 0.485
	_menu_heading.text = "기억의 문을 연다" if GameManager.current_locale == "ko" else "ENTER THE REMEMBERED PATH"
	UITheme.apply_title_font(_menu_heading)
	_menu_heading.add_theme_font_size_override("font_size", 18)
	_menu_heading.add_theme_color_override("font_color", Color(0.91, 0.85, 0.76))
	_menu_heading.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.85))
	_menu_heading.add_theme_constant_override("shadow_outline_size", 2)
	_menu_heading.mouse_filter = MOUSE_FILTER_IGNORE
	_menu_heading.z_index = 3
	add_child(_menu_heading)

	_menu_rule = ColorRect.new()
	_menu_rule.name = "MenuGoldRule"
	_menu_rule.anchor_left = 0.718
	_menu_rule.anchor_right = 0.955
	_menu_rule.anchor_top = 0.482
	_menu_rule.anchor_bottom = 0.484
	_menu_rule.color = Color(0.78, 0.56, 0.27, 0.48)
	_menu_rule.mouse_filter = MOUSE_FILTER_IGNORE
	_menu_rule.z_index = 3
	add_child(_menu_rule)

	_menu_footer = Label.new()
	_menu_footer.name = "MenuFooter"
	_menu_footer.anchor_left = 0.718
	_menu_footer.anchor_right = 0.955
	_menu_footer.anchor_top = 0.886
	_menu_footer.anchor_bottom = 0.916
	_menu_footer.text = "↑ ↓  선택    ENTER  결정" if GameManager.current_locale == "ko" else "↑ ↓  SELECT    ENTER  CONFIRM"
	UITheme.apply_ui_font(_menu_footer)
	_menu_footer.add_theme_font_size_override("font_size", 13)
	_menu_footer.add_theme_color_override("font_color", Color(0.58, 0.55, 0.52, 0.92))
	_menu_footer.mouse_filter = MOUSE_FILTER_IGNORE
	_menu_footer.z_index = 3
	add_child(_menu_footer)

func _style_title_button(btn: Button) -> void:
	btn.custom_minimum_size = Vector2(270, 44)
	btn.focus_mode = Control.FOCUS_ALL
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	UITheme.apply_ui_font(btn)

	var normal = StyleBoxFlat.new()
	normal.bg_color = Color(0.030, 0.024, 0.040, 0.64)
	normal.border_color = Color(0.48, 0.36, 0.22, 0.48)
	normal.set_border_width_all(1)
	normal.set_border_width(SIDE_LEFT, 2)
	normal.set_corner_radius_all(2)
	normal.set_content_margin_all(9)
	normal.content_margin_left = 16.0
	btn.add_theme_stylebox_override("normal", normal)

	var hover = normal.duplicate()
	hover.bg_color = Color(0.15, 0.105, 0.065, 0.88)
	hover.border_color = Color(1.0, 0.76, 0.36, 0.94)
	hover.set_border_width(SIDE_LEFT, 4)
	hover.content_margin_left = 19.0
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	var pressed = hover.duplicate()
	pressed.bg_color = Color(0.16, 0.12, 0.08, 0.94)
	btn.add_theme_stylebox_override("pressed", pressed)

	var disabled = normal.duplicate()
	disabled.bg_color = Color(0.025, 0.025, 0.030, 0.45)
	disabled.border_color = Color(0.24, 0.22, 0.20, 0.32)
	btn.add_theme_stylebox_override("disabled", disabled)

	btn.add_theme_font_size_override("font_size", 16)
	btn.add_theme_color_override("font_color", Color(0.84, 0.80, 0.73, 0.98))
	btn.add_theme_color_override("font_hover_color", Color(1.0, 0.86, 0.52, 1.0))
	btn.add_theme_color_override("font_focus_color", Color(1.0, 0.86, 0.52, 1.0))
	btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.90, 0.62, 1.0))
	btn.add_theme_color_override("font_disabled_color", Color(0.38, 0.36, 0.34, 0.85))
	btn.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75))
	btn.add_theme_constant_override("shadow_outline_size", 2)
	btn.add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 0.82))
	btn.add_theme_constant_override("outline_size", 1)

	btn.mouse_entered.connect(_on_title_button_mouse_entered.bind(btn))
	btn.mouse_exited.connect(_on_title_button_mouse_exited.bind(btn))
	btn.focus_entered.connect(_set_title_button_emphasis.bind(btn, true))
	btn.focus_exited.connect(_on_title_button_focus_exited.bind(btn))
	btn.resized.connect(_update_title_button_pivot.bind(btn))
	_update_title_button_pivot(btn)

func _on_title_button_mouse_entered(btn: Button) -> void:
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_hover")
	_set_title_button_emphasis(btn, true)

func _on_title_button_mouse_exited(btn: Button) -> void:
	if not btn.has_focus():
		_set_title_button_emphasis(btn, false)

func _on_title_button_focus_exited(btn: Button) -> void:
	if not btn.is_hovered():
		_set_title_button_emphasis(btn, false)

func _set_title_button_emphasis(btn: Button, emphasized: bool) -> void:
	if btn.disabled:
		return
	var target_scale := Vector2(1.018, 1.018) if emphasized else Vector2.ONE
	var target_modulate := Color(1.06, 1.02, 0.94, 1.0) if emphasized else Color.WHITE
	if _reduce_motion:
		btn.scale = Vector2.ONE
		btn.modulate = target_modulate
		return
	var tween := create_tween().set_parallel(true)
	tween.tween_property(btn, "scale", target_scale, 0.14).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(btn, "modulate", target_modulate, 0.14).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)

func _update_title_button_pivot(btn: Button) -> void:
	btn.pivot_offset = Vector2(0.0, btn.size.y * 0.5)

func _fade_in_title() -> void:
	await get_tree().process_frame
	_bg.modulate.a = 0.72
	menu_container.modulate.a = 0.0
	_title_stack.modulate.a = 0.0
	_title_rail.modulate.a = 0.0
	var menu_chrome: Array[CanvasItem] = [_menu_backdrop, _menu_heading, _menu_kicker, _menu_rule, _menu_footer]
	for item in menu_chrome:
		item.modulate.a = 0.0

	if _reduce_motion:
		var calm_tween := create_tween().set_parallel(true)
		calm_tween.tween_property(_bg, "modulate:a", 1.0, 0.25)
		calm_tween.tween_property(_title_stack, "modulate:a", 1.0, 0.25)
		calm_tween.tween_property(_title_rail, "modulate:a", 1.0, 0.25)
		calm_tween.tween_property(menu_container, "modulate:a", 1.0, 0.25)
		for item in menu_chrome:
			calm_tween.tween_property(item, "modulate:a", 1.0, 0.25)
		await calm_tween.finished
	else:
		var title_target := _title_stack.position
		var rail_target := _title_rail.position
		var menu_target := menu_container.position
		_title_stack.position = title_target + Vector2(-34.0, 0.0)
		_title_rail.position = rail_target + Vector2(-16.0, 0.0)
		menu_container.position = menu_target + Vector2(30.0, 0.0)
		_letterbox_top.offset_bottom = 42.0
		_letterbox_bottom.offset_top = -42.0
		var intro := create_tween().set_parallel(true)
		intro.tween_property(_bg, "modulate:a", 1.0, 1.15).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_title_stack, "modulate:a", 1.0, 0.72).set_delay(0.18).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_title_stack, "position", title_target, 0.86).set_delay(0.18).set_trans(Tween.TRANS_QUINT).set_ease(Tween.EASE_OUT)
		intro.tween_property(_title_rail, "modulate:a", 1.0, 0.62).set_delay(0.34).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_title_rail, "position", rail_target, 0.76).set_delay(0.26).set_trans(Tween.TRANS_QUINT).set_ease(Tween.EASE_OUT)
		intro.tween_property(_menu_backdrop, "modulate:a", 1.0, 0.64).set_delay(0.48).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_menu_heading, "modulate:a", 1.0, 0.52).set_delay(0.60).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_menu_kicker, "modulate:a", 1.0, 0.52).set_delay(0.56).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_menu_rule, "modulate:a", 1.0, 0.52).set_delay(0.62).set_trans(Tween.TRANS_SINE)
		intro.tween_property(_menu_footer, "modulate:a", 1.0, 0.52).set_delay(0.72).set_trans(Tween.TRANS_SINE)
		intro.tween_property(menu_container, "modulate:a", 1.0, 0.70).set_delay(0.64).set_trans(Tween.TRANS_SINE)
		intro.tween_property(menu_container, "position", menu_target, 0.82).set_delay(0.58).set_trans(Tween.TRANS_QUINT).set_ease(Tween.EASE_OUT)
		intro.tween_property(_letterbox_top, "offset_bottom", 24.0, 1.0).set_trans(Tween.TRANS_QUINT).set_ease(Tween.EASE_OUT)
		intro.tween_property(_letterbox_bottom, "offset_top", -24.0, 1.0).set_trans(Tween.TRANS_QUINT).set_ease(Tween.EASE_OUT)
		await intro.finished

	if is_instance_valid(new_game_btn) and get_viewport().gui_get_focus_owner() == null:
		new_game_btn.grab_focus()
	_intro_complete = true

func _layout_cinematic_canvas() -> void:
	if _bg:
		_bg.pivot_offset = _bg.size * 0.5
	if _ash_particles and _ash_material:
		_ash_particles.position = size * 0.5
		_ash_material.emission_box_extents = Vector3(maxf(size.x * 0.53, 640.0), maxf(size.y * 0.56, 360.0), 1.0)

func _process(delta: float) -> void:
	var wants_reduce_motion := OptionsMenu != null and OptionsMenu.is_reduce_motion()
	if wants_reduce_motion != _reduce_motion:
		_reduce_motion = wants_reduce_motion
		if _ash_particles:
			_ash_particles.emitting = not _reduce_motion
	if _reduce_motion:
		_parallax_offset = Vector2.ZERO
		_bg.position = Vector2(-18.0, -14.0)
		_bg.scale = Vector2.ONE
		_rift_glow.modulate.a = 0.78
		if _intro_complete:
			_title_rail.modulate.a = 0.96
		return

	_ambient_time += delta
	var viewport_size := size
	if viewport_size.x <= 0.0 or viewport_size.y <= 0.0:
		return
	var mouse_unit := get_viewport().get_mouse_position() / viewport_size - Vector2(0.5, 0.5)
	mouse_unit.x = clampf(mouse_unit.x, -0.5, 0.5)
	mouse_unit.y = clampf(mouse_unit.y, -0.5, 0.5)
	var parallax_target := mouse_unit * Vector2(-11.0, -6.0)
	_parallax_offset = _parallax_offset.lerp(parallax_target, 1.0 - exp(-delta * 1.4))
	_bg.position = Vector2(-18.0, -14.0) + _parallax_offset
	var breath := 1.012 + sin(_ambient_time * 0.24) * 0.0025
	_bg.scale = Vector2(breath, breath)
	_rift_glow.modulate.a = 0.78 + sin(_ambient_time * 0.72) * 0.12
	if _intro_complete:
		_title_rail.modulate.a = 0.86 + sin(_ambient_time * 0.58) * 0.10

func _play_title_bgm() -> void:
	if has_node("/root/AudioManager") and ResourceLoader.exists(TITLE_BGM_PATH):
		AudioManager.play_bgm(TITLE_BGM_PATH)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.is_pressed() and not event.echo:
		if not menu_container.has_focus():
			new_game_btn.grab_focus()

func _on_new_game_pressed() -> void:
	_play_select_sfx()
	WorldState.reset_to_defaults()
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
		"field_focus": 0,
		"directive_streak": 0,
		"elia_with_party": true,
		"items": {"witness_ink": 1},
		"item_quick_slots": ["witness_ink", "potion", "antidote"],
		"recent_items": [],
	}
	SceneFlow.pending_scene_id = "ch1_cold_open"
	SceneTransition.change_scene_styled("res://scenes/main/vn_host.tscn")

func _on_continue_pressed() -> void:
	_play_select_sfx()
	SaveManager.load_game(1)

func _on_aftermath_preview_pressed() -> void:
	_play_select_sfx()
	WorldState.reset_to_defaults()
	GameManager.story_flags.clear()
	GameManager.story_flags["part2_aftershock_preview"] = true
	GameManager.current_chapter = 11
	GameManager.ng_plus_cycle = 0
	GameManager.player_data = {
		"name": "Arrel",
		"hp": 100,
		"max_hp": 100,
		"grains": 0,
		"field_focus": 0,
		"directive_streak": 0,
		"elia_with_party": true,
		"items": {"witness_ink": 1},
		"item_quick_slots": ["witness_ink", "potion", "antidote"],
		"recent_items": [],
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
