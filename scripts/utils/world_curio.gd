## WorldCurio is a one-time regional landmark with a small RPG choice.
## It turns exploration art and world lore into tactical preparation instead of
## another automatic pickup: study for Field Focus, salvage an item, or attune
## to recover before the next visible hunt.
class_name WorldCurio
extends StaticBody2D

var map_id: String = ""
var curio_id: String = ""
var title: String = "Field Relic"
var title_ko: String = "현장 유물"
var lore: String = "A memory has settled into the shape of an object."
var lore_ko: String = "기억 하나가 사물의 형태로 가라앉아 있다."
var art_path: String = ""
var salvage_item: String = "potion"
var salvage_count: int = 1

var _choice_layer: CanvasLayer = null

func _ready() -> void:
	if GameManager.get_flag(_resolved_flag()):
		queue_free()
		return
	collision_layer = 12
	collision_mask = 0
	_add_visuals()

func interact() -> void:
	if GameManager.get_flag(_resolved_flag()) or is_instance_valid(_choice_layer):
		return
	_show_choice()

func _resolved_flag() -> String:
	return "world_curio_%s_%s" % [map_id, curio_id]

func _localized(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

func _add_visuals() -> void:
	var collider := CollisionShape2D.new()
	var shape := CircleShape2D.new()
	shape.radius = 18.0
	collider.shape = shape
	collider.position = Vector2(0, -2)
	add_child(collider)

	var shadow := Polygon2D.new()
	shadow.polygon = PackedVector2Array([
		Vector2(-18, 8), Vector2(-9, 4), Vector2(9, 4), Vector2(18, 8),
		Vector2(9, 12), Vector2(-9, 12),
	])
	shadow.color = Color(0.0, 0.0, 0.0, 0.38)
	shadow.z_index = -1
	add_child(shadow)

	var relic := Polygon2D.new()
	relic.polygon = PackedVector2Array([
		Vector2(0, -22), Vector2(13, -7), Vector2(10, 10),
		Vector2(0, 16), Vector2(-10, 10), Vector2(-13, -7),
	])
	relic.color = Color(0.075, 0.065, 0.11, 0.98)
	relic.z_index = 1
	add_child(relic)

	var fracture := Line2D.new()
	fracture.points = PackedVector2Array([
		Vector2(-5, -14), Vector2(2, -7), Vector2(-2, 0),
		Vector2(6, 7), Vector2(1, 13),
	])
	fracture.width = 1.6
	fracture.default_color = Color(0.92, 0.72, 0.31, 0.92)
	fracture.z_index = 2
	add_child(fracture)

	var orbit := Line2D.new()
	orbit.points = PackedVector2Array([
		Vector2(-17, -3), Vector2(-10, -11), Vector2(0, -14),
		Vector2(10, -11), Vector2(17, -3), Vector2(10, 5),
		Vector2(0, 8), Vector2(-10, 5), Vector2(-17, -3),
	])
	orbit.width = 1.0
	orbit.default_color = Color(0.56, 0.47, 0.80, 0.70)
	orbit.z_index = 2
	add_child(orbit)

	var glow := PointLight2D.new()
	glow.energy = 0.52
	glow.color = Color(0.72, 0.59, 1.0, 1.0)
	glow.texture_scale = 0.32
	glow.z_index = 0
	add_child(glow)

	var tween := create_tween().set_loops().set_parallel(true)
	tween.tween_property(relic, "position:y", -3.0, 1.25).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(orbit, "modulate:a", 0.35, 1.25).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.chain().tween_property(relic, "position:y", 0.0, 1.25).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.parallel().tween_property(orbit, "modulate:a", 1.0, 1.25).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)

func _show_choice() -> void:
	var tree := get_tree()
	if tree == null:
		return
	_choice_layer = CanvasLayer.new()
	_choice_layer.name = "WorldCurioChoice"
	_choice_layer.layer = 82
	_choice_layer.process_mode = Node.PROCESS_MODE_ALWAYS
	_choice_layer.set_meta("was_paused", tree.paused)
	tree.root.add_child(_choice_layer)
	tree.paused = true

	var shade := ColorRect.new()
	shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	shade.color = Color(0.008, 0.006, 0.015, 0.90)
	shade.mouse_filter = Control.MOUSE_FILTER_STOP
	_choice_layer.add_child(shade)

	if art_path != "" and ResourceLoader.exists(art_path):
		var art := TextureRect.new()
		art.anchor_left = 0.04
		art.anchor_top = 0.04
		art.anchor_right = 0.96
		art.anchor_bottom = 0.58
		art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		art.texture = load(art_path) as Texture2D
		art.modulate = Color(0.72, 0.70, 0.80, 0.46)
		art.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_choice_layer.add_child(art)

	var depth_stage := HybridDepthStage.create_stage(map_id, HybridDepthStage.StageMode.RELIC)
	depth_stage.name = "RelicDepthDiorama"
	depth_stage.anchor_left = 0.20
	depth_stage.anchor_right = 0.80
	depth_stage.anchor_top = 0.035
	depth_stage.anchor_bottom = 0.57
	depth_stage.modulate = Color(0.96, 0.92, 1.0, 0.88)
	_choice_layer.add_child(depth_stage)

	var panel := PanelContainer.new()
	panel.anchor_left = 0.08
	panel.anchor_top = 0.51
	panel.anchor_right = 0.92
	panel.anchor_bottom = 0.95
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.025, 0.021, 0.042, 0.98)
	panel_style.border_color = Color(0.62, 0.49, 0.24, 0.90)
	panel_style.set_border_width_all(2)
	panel_style.corner_radius_top_left = 8
	panel_style.corner_radius_top_right = 8
	panel_style.corner_radius_bottom_left = 8
	panel_style.corner_radius_bottom_right = 8
	panel_style.content_margin_left = 28.0
	panel_style.content_margin_right = 28.0
	panel_style.content_margin_top = 20.0
	panel_style.content_margin_bottom = 18.0
	panel.add_theme_stylebox_override("panel", panel_style)
	_choice_layer.add_child(panel)

	var content := VBoxContainer.new()
	content.add_theme_constant_override("separation", 12)
	panel.add_child(content)

	var heading := Label.new()
	heading.text = _localized(title, title_ko)
	heading.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	heading.add_theme_font_size_override("font_size", 23)
	heading.add_theme_color_override("font_color", Color(0.96, 0.80, 0.42))
	content.add_child(heading)

	var body := Label.new()
	body.text = _localized(lore, lore_ko)
	body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	body.add_theme_font_size_override("font_size", 15)
	body.add_theme_color_override("font_color", Color(0.82, 0.80, 0.88))
	content.add_child(body)

	var prompt := Label.new()
	prompt.text = _localized("Choose what Arrel takes from this place.", "아렐이 이 장소에서 무엇을 가져갈지 선택하세요.")
	prompt.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	prompt.add_theme_font_size_override("font_size", 13)
	prompt.add_theme_color_override("font_color", Color(0.61, 0.58, 0.70))
	content.add_child(prompt)

	var row := HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_CENTER
	row.add_theme_constant_override("separation", 10)
	content.add_child(row)
	var first_button: Button = null
	for choice_data: Dictionary in [
		{"id": "study", "label": _localized("STUDY  ·  Field Focus", "조사  ·  현장 집중")},
		{"id": "salvage", "label": _localized("SALVAGE  ·  Item", "회수  ·  아이템")},
		{"id": "attune", "label": _localized("ATTUNE  ·  Recover", "동조  ·  회복")},
	]:
		var button := Button.new()
		button.text = String(choice_data.label)
		button.custom_minimum_size = Vector2(210, 46)
		button.add_theme_font_size_override("font_size", 14)
		var choice_id := String(choice_data.id)
		button.pressed.connect(func() -> void: _resolve_choice(choice_id))
		row.add_child(button)
		if first_button == null:
			first_button = button

	var leave := Button.new()
	leave.text = _localized("Leave it untouched", "손대지 않고 떠난다")
	leave.custom_minimum_size = Vector2(0, 36)
	leave.flat = true
	leave.pressed.connect(_close_choice)
	content.add_child(leave)
	if first_button != null:
		first_button.grab_focus()

func _resolve_choice(choice_id: String) -> void:
	if GameManager.get_flag(_resolved_flag()):
		_close_choice()
		return
	GameManager.set_flag(_resolved_flag())
	GameManager.set_flag("%s_%s" % [_resolved_flag(), choice_id])
	match choice_id:
		"study":
			var gained := GameManager.add_field_focus(1)
			if gained > 0:
				NotificationToast.show_toast(_localized("Field Focus gained for the next battle.", "다음 전투를 위한 현장 집중을 얻었습니다."), NotificationToast.ToastType.SUCCESS)
			else:
				GameManager.player_data.grains += 12
				NotificationToast.show_toast(_localized("The finding was already understood. +12 Grains.", "이미 이해한 발견입니다. +12 그레인."), NotificationToast.ToastType.SUCCESS)
		"salvage":
			if GameManager.ITEMS.has(salvage_item):
				GameManager.add_item(salvage_item, maxi(salvage_count, 1))
			else:
				GameManager.player_data.grains += 15
				NotificationToast.show_toast("+15 Grains", NotificationToast.ToastType.SUCCESS)
		"attune":
			var max_hp := int(GameManager.player_data.get("max_hp", 100))
			var hp := int(GameManager.player_data.get("hp", max_hp))
			var recovery := maxi(int(ceil(float(max_hp) * 0.25)), 1)
			var actual := mini(recovery, maxi(max_hp - hp, 0))
			GameManager.player_data.hp = hp + actual
			if actual > 0:
				NotificationToast.show_toast(_localized("The relic steadies you. +%d HP" % actual, "유물이 몸을 안정시킵니다. +%d HP" % actual), NotificationToast.ToastType.SUCCESS)
			else:
				GameManager.player_data.grains += 10
				NotificationToast.show_toast(_localized("The excess resonance crystallizes. +10 Grains.", "남은 공명이 결정화됩니다. +10 그레인."), NotificationToast.ToastType.SUCCESS)
	AudioManager.play_sfx("ui_select")
	_close_choice()
	queue_free()

func _close_choice() -> void:
	if not is_instance_valid(_choice_layer):
		return
	var was_paused := bool(_choice_layer.get_meta("was_paused", false))
	_choice_layer.queue_free()
	_choice_layer = null
	get_tree().paused = was_paused
