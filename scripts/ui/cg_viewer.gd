## CgViewer (Autoload)
## Full-screen CG display and dialogue background CG support.
extends CanvasLayer

const FADE_DURATION: float = 0.6

var is_showing: bool = false
var auto_close_timer: float = 0.0
var waiting_for_input: bool = false

var bg: ColorRect
var cg_texture: TextureRect
var cg_top_wash: ColorRect
var cg_lower_wash: ColorRect
var overlay_label: RichTextLabel
var continue_panel: PanelContainer
var continue_label: Label
var tween: Tween

var _closing: bool = false
var _on_closed_callback: Callable
var _text_panel: PanelContainer

signal cg_shown(image_path: String)
signal cg_closed()

func _ready() -> void:
	layer = 45
	_build_ui()
	_hide_all()
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	if InputManager and not InputManager.input_mode_changed.is_connected(_update_continue_hint):
		InputManager.input_mode_changed.connect(_update_continue_hint)
	print("[CgViewer] Ready")

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0, 0, 0, 1)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(bg)

	cg_texture = TextureRect.new()
	cg_texture.set_anchors_preset(Control.PRESET_FULL_RECT)
	cg_texture.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	cg_texture.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	cg_texture.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(cg_texture)

	cg_top_wash = ColorRect.new()
	cg_top_wash.anchor_left = 0.0
	cg_top_wash.anchor_right = 1.0
	cg_top_wash.anchor_top = 0.0
	cg_top_wash.anchor_bottom = 0.28
	cg_top_wash.color = Color(0.015, 0.012, 0.018, 0.0)
	cg_top_wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(cg_top_wash)

	cg_lower_wash = ColorRect.new()
	cg_lower_wash.anchor_left = 0.0
	cg_lower_wash.anchor_right = 1.0
	cg_lower_wash.anchor_top = 0.54
	cg_lower_wash.anchor_bottom = 1.0
	cg_lower_wash.color = Color(0.012, 0.010, 0.016, 0.0)
	cg_lower_wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(cg_lower_wash)

	_text_panel = PanelContainer.new()
	_text_panel.anchor_left = 0.1
	_text_panel.anchor_right = 0.9
	_text_panel.anchor_top = 0.75
	_text_panel.anchor_bottom = 0.95
	_text_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0, 0, 0, 0.7)
	style.set_content_margin_all(16)
	style.set_corner_radius_all(4)
	_text_panel.add_theme_stylebox_override("panel", style)
	_text_panel.visible = false
	root.add_child(_text_panel)

	overlay_label = RichTextLabel.new()
	overlay_label.bbcode_enabled = false
	overlay_label.fit_content = false
	overlay_label.scroll_active = false
	overlay_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay_label.add_theme_font_size_override("normal_font_size", 16)
	overlay_label.add_theme_color_override("default_color", Color(0.9, 0.87, 0.82))
	_text_panel.add_child(overlay_label)

	continue_panel = PanelContainer.new()
	continue_panel.anchor_left = 1.0
	continue_panel.anchor_right = 1.0
	continue_panel.anchor_top = 1.0
	continue_panel.anchor_bottom = 1.0
	continue_panel.offset_left = -206
	continue_panel.offset_right = -28
	continue_panel.offset_top = -62
	continue_panel.offset_bottom = -24
	continue_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	continue_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.015, 0.013, 0.02, 0.82),
		Color(0.68, 0.54, 0.33, 0.55),
		1,
		5,
		8
	))
	continue_panel.visible = false
	root.add_child(continue_panel)

	continue_label = Label.new()
	continue_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	continue_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	continue_label.add_theme_font_size_override("font_size", 12)
	continue_label.add_theme_color_override("font_color", Color(0.86, 0.80, 0.68))
	UITheme.apply_ui_font(continue_label)
	continue_panel.add_child(continue_label)
	_update_continue_hint()

func show_cg(image_path: String, text: String = "", auto_close_sec: float = 0.0, callback: Callable = Callable()) -> void:
	if not ResourceLoader.exists(image_path):
		push_error("[CgViewer] Image not found: %s" % image_path)
		return

	_closing = false
	_on_closed_callback = callback
	cg_texture.texture = load(image_path)
	_set_caption(text)
	_prepare_visible_state(Vector2(-14, -8), Vector2(1.025, 1.025))

	if tween:
		tween.kill()
	tween = create_tween().set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_top_wash, "color:a", 0.28, FADE_DURATION)
	tween.tween_property(cg_lower_wash, "color:a", 0.56, FADE_DURATION)
	await tween.finished

	if not is_inside_tree() or _closing:
		return
	cg_shown.emit(image_path)

	if auto_close_sec > 0.0:
		await get_tree().create_timer(auto_close_sec).timeout
		close_cg()
	else:
		waiting_for_input = true
		continue_panel.visible = true
		_update_continue_hint()

func close_cg() -> void:
	if not is_showing or _closing:
		return

	_closing = true
	waiting_for_input = false

	if not is_inside_tree():
		_finish_close(false)
		return

	if tween:
		tween.kill()
	tween = create_tween().set_parallel(true)
	tween.tween_property(bg, "modulate:a", 0.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 0.0, FADE_DURATION)
	tween.tween_property(cg_top_wash, "color:a", 0.0, FADE_DURATION)
	tween.tween_property(cg_lower_wash, "color:a", 0.0, FADE_DURATION)
	await tween.finished

	if not is_inside_tree():
		_finish_close(false)
		return
	_finish_close(true)

func _finish_close(emit_signal: bool) -> void:
	_hide_all()
	is_showing = false
	_closing = false
	if emit_signal:
		cg_closed.emit()
	if _on_closed_callback.is_valid():
		_on_closed_callback.call()
		_on_closed_callback = Callable()

func _set_caption(text: String) -> void:
	if overlay_label == null or _text_panel == null:
		return
	overlay_label.text = text
	_text_panel.visible = text != ""

func _prepare_visible_state(texture_position: Vector2, texture_scale: Vector2) -> void:
	bg.modulate.a = 0.0
	cg_texture.modulate.a = 0.0
	cg_texture.position = texture_position
	cg_texture.scale = texture_scale
	cg_top_wash.color.a = 0.0
	cg_lower_wash.color.a = 0.0
	bg.visible = true
	cg_texture.visible = true
	cg_top_wash.visible = true
	cg_lower_wash.visible = true
	is_showing = true
	waiting_for_input = false
	if continue_panel:
		continue_panel.visible = false

func _hide_all() -> void:
	if bg != null:
		bg.visible = false
	if cg_texture != null:
		cg_texture.visible = false
	if cg_top_wash != null:
		cg_top_wash.visible = false
	if cg_lower_wash != null:
		cg_lower_wash.visible = false
	if _text_panel != null:
		_text_panel.visible = false
	if continue_panel != null:
		continue_panel.visible = false

func _update_continue_hint(_mode = null) -> void:
	if continue_label == null or InputManager == null:
		return
	var label := "계속" if GameManager.current_locale == "ko" else "Continue"
	continue_label.text = InputManager.get_hint("interact", label)

func _unhandled_input(event: InputEvent) -> void:
	if not is_showing or not waiting_for_input:
		return

	if event.is_action_pressed("interact"):
		close_cg()
		get_viewport().set_input_as_handled()

func _on_dialogue_line(_speaker: String, _text: String, _portrait: String) -> void:
	if DialogueManager.current_index < 0 or DialogueManager.current_index >= DialogueManager.current_dialogue.size():
		return
	var line = DialogueManager.current_dialogue[DialogueManager.current_index]
	if line is Dictionary and line.has("cg"):
		_show_cg_background(str(line.get("cg", "")))

func _show_cg_background(image_path: String) -> void:
	if image_path == "" or not ResourceLoader.exists(image_path):
		return

	_closing = false
	cg_texture.texture = load(image_path)
	_set_caption("")
	_prepare_visible_state(Vector2(-12, -8), Vector2(1.02, 1.02))

	if tween:
		tween.kill()
	tween = create_tween().set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_top_wash, "color:a", 0.24, FADE_DURATION)
	tween.tween_property(cg_lower_wash, "color:a", 0.50, FADE_DURATION)

	if not DialogueManager.dialogue_ended.is_connected(_on_dialogue_ended_close_cg):
		DialogueManager.dialogue_ended.connect(_on_dialogue_ended_close_cg, CONNECT_ONE_SHOT)

func _on_dialogue_ended_close_cg() -> void:
	if is_showing:
		call_deferred("close_cg")
