extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/directional_cast_ui.png"
const DIRECTIONS := [
	{"label": "아래", "animation": "idle_down"},
	{"label": "위", "animation": "idle_up"},
	{"label": "왼쪽", "animation": "idle_left"},
	{"label": "오른쪽", "animation": "idle_right"},
]
const CAST := [
	{"label": "아렐", "sheet": "arrel"},
	{"label": "엘리아", "sheet": "elia"},
	{"label": "말렛", "sheet": "malet"},
	{"label": "토비아스", "sheet": "tobias"},
	{"label": "카이로스", "sheet": "kairos"},
	{"label": "세이블", "sheet": "sable"},
]

func _ready() -> void:
	ExplorationHUD.visible = false
	PauseMenu.visible = false
	DialogueBox.visible = false
	_build_gallery()
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().create_timer(0.35).timeout
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("DIRECTIONAL_CAST_CAPTURE_PASS path=%s cast=%d directions=%d font=%s" % [OUTPUT_PATH, CAST.size(), DIRECTIONS.size(), UITheme.make_body_font().font_names[0]])
	get_tree().quit(0)

func _build_gallery() -> void:
	var background := ColorRect.new()
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.color = Color("10141b")
	add_child(background)

	var panel := PanelContainer.new()
	panel.position = Vector2(34, 26)
	panel.size = Vector2(1212, 668)
	panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(Color("181d27e8"), Color("d6b469"), 2, 16, 16))
	add_child(panel)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 8)
	panel.add_child(root)

	var title := Label.new()
	title.text = "인물 방향 시트 · 기억의 행로"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_override("font", UITheme.make_title_font())
	title.add_theme_font_size_override("font_size", 25)
	title.add_theme_color_override("font_color", Color("f1dfb0"))
	root.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "방향을 바꿀 때마다 실루엣과 시선이 즉시 구분됩니다"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_override("font", UITheme.make_body_font())
	subtitle.add_theme_font_size_override("font_size", 15)
	subtitle.add_theme_color_override("font_color", Color("bfc5d0"))
	root.add_child(subtitle)

	var headers := HBoxContainer.new()
	headers.custom_minimum_size.y = 28
	headers.add_theme_constant_override("separation", 8)
	root.add_child(headers)
	_add_header(headers, "인물", 150)
	for direction in DIRECTIONS:
		_add_header(headers, direction.label, 244)

	for cast_member in CAST:
		var row := HBoxContainer.new()
		row.custom_minimum_size.y = 82
		row.add_theme_constant_override("separation", 8)
		root.add_child(row)
		var name_label := Label.new()
		name_label.text = cast_member.label
		name_label.custom_minimum_size = Vector2(150, 82)
		name_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		name_label.add_theme_font_override("font", UITheme.make_ui_font())
		name_label.add_theme_font_size_override("font_size", 17)
		name_label.add_theme_color_override("font_color", Color("ead9ad"))
		row.add_child(name_label)

		var frames: SpriteFrames
		var sheet_key: String = cast_member.sheet
		var authored: bool = sheet_key != "sable"
		if authored:
			frames = PixelSprite.create_sheet_frames(sheet_key)
		else:
			frames = PixelSprite.create_frames(PixelSprite.sable_config())
		for direction in DIRECTIONS:
			assert(frames.has_animation(direction.animation), "%s is missing %s" % [sheet_key, direction.animation])
			var cell := PanelContainer.new()
			cell.custom_minimum_size = Vector2(244, 82)
			cell.add_theme_stylebox_override("panel", UITheme.make_panel_style(Color("11161fc0"), Color("555f72"), 1, 8, 0))
			row.add_child(cell)
			var sprite := AnimatedSprite2D.new()
			sprite.sprite_frames = frames
			sprite.animation = direction.animation
			sprite.frame = 0
			sprite.position = Vector2(122, 52)
			sprite.scale = Vector2(0.43, 0.43) if authored else Vector2(1.15, 1.15)
			sprite.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR if authored else CanvasItem.TEXTURE_FILTER_NEAREST
			cell.add_child(sprite)

func _add_header(parent: HBoxContainer, text_value: String, width: float) -> void:
	var label := Label.new()
	label.text = text_value
	label.custom_minimum_size = Vector2(width, 28)
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.add_theme_font_override("font", UITheme.make_ui_font())
	label.add_theme_font_size_override("font_size", 13)
	label.add_theme_color_override("font_color", Color("9fa8b8"))
	parent.add_child(label)
