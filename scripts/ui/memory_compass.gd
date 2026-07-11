## MemoryCompass (Autoload)
## Reads memory density and makes world rewrites visible during exploration.
extends CanvasLayer

const PANEL_SIZE := Vector2(284, 118)
const HIDDEN_ALPHA := 0.12
const VISIBLE_ALPHA := 0.94

var panel: PanelContainer
var compass_title: Label
var status_label: Label
var lore_label: Label
var burn_label: Label
var art_plate: TextureRect
var needle: ColorRect
var needle_glow: ColorRect
var flash: ColorRect

var _user_hidden := false
var _pulse_time := 0.0
var _tick := 0.0
var _density := 1.0
var _target_angle := 0.0
var _last_scene_key := ""
var _last_burn_text := ""

func _ready() -> void:
	layer = 24
	_build_ui()
	_connect_signals()
	_refresh_compass(true)
	set_process(true)
	set_process_input(true)

func _build_ui() -> void:
	flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(0.72, 0.62, 0.48, 0.0)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(flash)

	panel = PanelContainer.new()
	panel.anchor_left = 1.0
	panel.anchor_right = 1.0
	panel.anchor_top = 0.0
	panel.anchor_bottom = 0.0
	panel.offset_left = -PANEL_SIZE.x - 18
	panel.offset_right = -18
	panel.offset_top = 154
	panel.offset_bottom = 154 + PANEL_SIZE.y
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.modulate.a = 0.0
	panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.018, 0.016, 0.026, 0.9),
		Color(0.66, 0.52, 0.31, 0.62),
		1,
		5,
		10
	))
	add_child(panel)

	art_plate = TextureRect.new()
	art_plate.set_anchors_preset(Control.PRESET_FULL_RECT)
	art_plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	art_plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	art_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	art_plate.modulate = Color(0.62, 0.54, 0.42, 0.16)
	if ResourceLoader.exists("res://assets/cg/generated/ui_memory_compass_close.png"):
		art_plate.texture = load("res://assets/cg/generated/ui_memory_compass_close.png")
	panel.add_child(art_plate)

	var shade := ColorRect.new()
	shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	shade.color = Color(0.0, 0.0, 0.0, 0.34)
	shade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(shade)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 10)
	panel.add_child(row)

	var dial := Control.new()
	dial.custom_minimum_size = Vector2(68, 86)
	row.add_child(dial)

	var dial_bg := PanelContainer.new()
	dial_bg.position = Vector2(4, 9)
	dial_bg.size = Vector2(60, 60)
	dial_bg.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.06, 0.055, 0.072, 0.92),
		Color(0.74, 0.64, 0.42, 0.7),
		1,
		30,
		0
	))
	dial.add_child(dial_bg)

	var center_dot := ColorRect.new()
	center_dot.position = Vector2(31, 36)
	center_dot.size = Vector2(6, 6)
	center_dot.color = Color(0.92, 0.76, 0.43, 0.95)
	dial.add_child(center_dot)

	needle_glow = ColorRect.new()
	needle_glow.position = Vector2(34, 38)
	needle_glow.size = Vector2(28, 4)
	needle_glow.pivot_offset = Vector2(0, 2)
	needle_glow.color = Color(0.9, 0.54, 0.36, 0.22)
	dial.add_child(needle_glow)

	needle = ColorRect.new()
	needle.position = Vector2(34, 39)
	needle.size = Vector2(25, 2)
	needle.pivot_offset = Vector2(0, 1)
	needle.color = Color(0.96, 0.78, 0.42, 1.0)
	dial.add_child(needle)

	var text_box := VBoxContainer.new()
	text_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_box.add_theme_constant_override("separation", 3)
	row.add_child(text_box)

	compass_title = Label.new()
	compass_title.text = "MEMORY COMPASS"
	compass_title.add_theme_font_size_override("font_size", 11)
	compass_title.add_theme_color_override("font_color", Color(0.92, 0.79, 0.52))
	text_box.add_child(compass_title)

	status_label = Label.new()
	status_label.add_theme_font_size_override("font_size", 15)
	status_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	text_box.add_child(status_label)

	lore_label = Label.new()
	lore_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	lore_label.custom_minimum_size.x = 184
	lore_label.add_theme_font_size_override("font_size", 11)
	lore_label.add_theme_color_override("font_color", Color(0.62, 0.59, 0.52))
	text_box.add_child(lore_label)

	burn_label = Label.new()
	burn_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	burn_label.custom_minimum_size.x = 184
	burn_label.add_theme_font_size_override("font_size", 11)
	burn_label.add_theme_color_override("font_color", Color(0.86, 0.58, 0.42))
	text_box.add_child(burn_label)

func _connect_signals() -> void:
	if GameManager and GameManager.has_signal("state_changed"):
		GameManager.state_changed.connect(_on_state_changed)
	if MemoryManager and MemoryManager.has_signal("memory_burned"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	if MemoryManager and MemoryManager.has_signal("memory_added"):
		MemoryManager.memory_added.connect(_on_memory_changed)
	if MemoryManager and MemoryManager.has_signal("memory_faded"):
		MemoryManager.memory_faded.connect(_on_memory_changed)
	if WorldRewriteDirector and WorldRewriteDirector.has_signal("rewrite_manifested"):
		WorldRewriteDirector.rewrite_manifested.connect(_on_rewrite_manifested)

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_C:
		_user_hidden = not _user_hidden
		_refresh_visibility()

func _process(delta: float) -> void:
	_tick += delta
	_pulse_time = maxf(_pulse_time - delta, 0.0)
	if _tick >= 0.45:
		_tick = 0.0
		_refresh_compass(false)

	var drift := sin(Time.get_ticks_msec() * 0.0017) * (0.18 + (1.0 - _density) * 0.42)
	var wobble := sin(Time.get_ticks_msec() * 0.0051) * _pulse_time * 0.22
	needle.rotation = lerp_angle(needle.rotation, _target_angle + drift + wobble, 0.08)
	needle_glow.rotation = needle.rotation
	needle_glow.modulate.a = 0.55 + sin(Time.get_ticks_msec() * 0.006) * 0.22 + _pulse_time * 0.18
	if _pulse_time <= 0.0 and burn_label.text != "":
		burn_label.modulate.a = maxf(burn_label.modulate.a - delta * 0.25, 0.0)

func _on_state_changed(_new_state) -> void:
	_refresh_visibility()

func _on_memory_changed(_memory) -> void:
	_refresh_compass(true)

func _on_memory_burned(memory) -> void:
	_last_burn_text = _make_burn_line(memory)
	_pulse_time = 2.4
	burn_label.modulate.a = 1.0
	_rewrite_current_scene()
	_show_world_flash(memory)
	_refresh_compass(true)

func _on_rewrite_manifested(_memory, report: Dictionary) -> void:
	if report.has("compass"):
		_last_burn_text = String(report.compass)
		_pulse_time = maxf(_pulse_time, 1.8)
		burn_label.modulate.a = 1.0
		_refresh_compass(true)

func _refresh_visibility() -> void:
	var should_show := not _user_hidden
	if GameManager:
		should_show = should_show and GameManager.current_state == GameManager.GameState.EXPLORATION
	# Keep the large panel event-driven in clean view so the field stays readable.
	if OptionsMenu != null and OptionsMenu.is_clean_gameplay_visuals():
		should_show = should_show and _pulse_time > 0.0
	panel.visible = should_show
	if should_show:
		var target_alpha := VISIBLE_ALPHA if _pulse_time > 0.0 else 0.86
		panel.modulate.a = lerpf(panel.modulate.a, target_alpha, 0.3)
	else:
		panel.modulate.a = 0.0

func _refresh_compass(force: bool) -> void:
	_refresh_visibility()
	if not panel.visible and not force:
		return

	_density = _compute_density()
	var scene_key := _get_scene_key()
	if force or scene_key != _last_scene_key:
		_last_scene_key = scene_key
	var profile := _get_place_profile(scene_key, _density)
	status_label.text = profile.get("status", "THREADS HUM")
	lore_label.text = profile.get("line", "The needle listens for what still remembers.")
	burn_label.text = _last_burn_text
	_target_angle = profile.get("angle", 0.0)
	var tint: Color = profile.get("color", Color(0.92, 0.76, 0.43))
	needle.color = tint
	needle_glow.color = Color(tint.r, tint.g, tint.b, 0.24)
	status_label.add_theme_color_override("font_color", tint.lightened(0.2))

func _compute_density() -> float:
	if not MemoryManager or MemoryManager.memories.is_empty():
		return 1.0
	var total := 0.0
	var intact := 0.0
	for memory in MemoryManager.memories:
		var weight := float(5 - int(memory.grade))
		total += weight
		if not memory.is_burned and not memory.is_faded:
			intact += weight
		elif not memory.is_burned:
			intact += weight * 0.35
	if total <= 0.0:
		return 1.0
	return clampf(intact / total, 0.0, 1.0)

func _get_scene_key() -> String:
	var scene := get_tree().current_scene
	if scene == null:
		return ""
	var file_name := scene.scene_file_path.get_file().get_basename()
	return file_name

func _get_place_profile(scene_key: String, density: float) -> Dictionary:
	match scene_key:
		"rim_forest":
			return {"status": "NEEDLE SPINS", "line": "Low density. Rim memories scatter like ash.", "angle": PI * 1.72, "color": Color(0.70, 0.83, 0.46)}
		"verdan_market":
			return {"status": "BUREAU INK", "line": "The needle follows recorded debt before it follows north.", "angle": PI * 0.18, "color": Color(0.90, 0.64, 0.34)}
		"belt_waystation":
			return {"status": "RECORD HUM", "line": "Paper, soil, stone. Everything here tries to remember.", "angle": PI * 0.38, "color": Color(0.76, 0.62, 0.46)}
		"drift_shelter":
			return {"status": "ANCHOR LOW", "line": "Elia's residue holds the page in its binding.", "angle": PI * 0.62, "color": Color(0.62, 0.70, 0.90)}
		"crumbling_coast":
			return {"status": "GATE STATIC", "line": "The seal interrupts direction. The needle answers in fragments.", "angle": PI * 0.91, "color": Color(0.70, 0.80, 0.96)}
		"the_seam":
			return {"status": "THREADS HOLD", "line": "Connection is not memory, but it can carry one across the dark.", "angle": PI * 1.08, "color": Color(0.92, 0.68, 0.42)}
		"seam_outskirts":
			return {"status": "ECHO SHELL", "line": "A hollow can still answer if the echo is shaped kindly.", "angle": PI * 1.24, "color": Color(0.74, 0.57, 0.96)}
		"forgotten_forest":
			return {"status": "NEEDLE LISTENS", "line": "The trees remember being trees. The soil remembers rain.", "angle": PI * 1.44, "color": Color(0.58, 0.76, 0.42)}
		"colorless_waste":
			return {"status": "NEEDLE BLEEDS", "line": "The Waste has no north. It has hunger.", "angle": PI * 1.64, "color": Color(0.78, 0.78, 0.82)}
		"bl07_void":
			return {"status": "NEEDLE MELTS", "line": "Near BL-07, even direction forgets itself.", "angle": PI * 1.9, "color": Color(0.78, 0.54, 1.0)}

	if density < 0.34:
		return {"status": "NEEDLE STUTTERS", "line": "Too many burned contours. The world guesses at its own shape.", "angle": PI * 1.58, "color": Color(0.82, 0.52, 0.42)}
	if density < 0.62:
		return {"status": "THREADS THIN", "line": "The compass points toward what the world can still keep.", "angle": PI * 0.84, "color": Color(0.86, 0.72, 0.46)}
	return {"status": "THREADS HUM", "line": "The needle rests where memory density is strongest.", "angle": PI * 0.12, "color": Color(0.92, 0.78, 0.44)}

func _make_burn_line(memory) -> String:
	if memory == null:
		return "A contour vanished. The world reorders around the gap."
	var title := String(memory.title)
	var npc := String(memory.related_npc)
	if npc == "Elia":
		return "Anchor thread frays: %s" % title
	if npc == "Tobias":
		return "Record contour preserved: %s" % title
	if npc == "Sable":
		return "A witness turns away: %s" % title
	if memory.grade >= MemoryManager.MemoryGrade.GRADE_2:
		return "Identity fault detected: %s" % title
	return "World forgot a small truth: %s" % title

func _rewrite_current_scene() -> void:
	var scene := get_tree().current_scene
	if scene == null:
		return
	PerceptionFilter.apply(scene)
	if scene.has_method("_on_memory_rewrite"):
		scene.call_deferred("_on_memory_rewrite")

func _show_world_flash(memory) -> void:
	var tint := Color(0.72, 0.62, 0.48, 0.0)
	if memory != null and memory.grade >= MemoryManager.MemoryGrade.GRADE_2:
		tint = Color(0.78, 0.36, 0.28, 0.0)
	flash.color = tint
	var tween := create_tween()
	tween.tween_property(flash, "color:a", 0.18, 0.08)
	tween.tween_property(flash, "color:a", 0.0, 0.55).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	var panel_tween := create_tween()
	panel_tween.tween_property(panel, "scale", Vector2(1.035, 1.035), 0.08)
	panel_tween.tween_property(panel, "scale", Vector2.ONE, 0.35).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
