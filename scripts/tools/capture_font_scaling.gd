## S210: 창을 키웠을 때 글자가 뭉개지지 않는지 확인하는 실측 캡처.
##
## stretch mode가 "viewport"였을 때는 게임 전체가 1280x720 프레임버퍼에 그려진 뒤
## 창 크기로 비트맵 확대되었다. 1080p에서 1.5배, 1440p에서 2배로 늘어나면서 모든
## 글리프가 뭉개졌다. "canvas_items"에서는 좌표계만 1280x720을 유지하고 글리프는
## 창의 실제 해상도로 래스터화된다. 이 캡처는 그 계약을 이미지로 남긴다.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/font_scaling_1080p.png"

func _ready() -> void:
	ExplorationHUD.visible = false
	PauseMenu.visible = false
	GameManager.current_locale = "ko"

	DisplayServer.window_set_size(Vector2i(1920, 1080))
	await get_tree().process_frame
	await get_tree().process_frame

	var backdrop := ColorRect.new()
	backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	backdrop.color = Color(0.035, 0.030, 0.045)
	add_child(backdrop)

	var column := VBoxContainer.new()
	column.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	column.offset_left = 60
	column.offset_top = 50
	column.offset_right = -60
	column.add_theme_constant_override("separation", 14)
	add_child(column)

	for size in [12, 14, 16, 18, 22, 28]:
		var label := Label.new()
		label.text = "%dpx · 기억은 사라지지 않아. 다만 누가 그 값을 치르느냐가 달라질 뿐이지. ABCdef 0123" % size
		label.add_theme_font_size_override("font_size", size)
		label.add_theme_color_override("font_color", Color(0.92, 0.88, 0.80))
		column.add_child(label)

	await get_tree().process_frame

	var viewport_size := get_viewport().get_visible_rect().size
	var canvas_scale := get_viewport().get_final_transform().get_scale()
	assert(get_window().content_scale_mode == Window.CONTENT_SCALE_MODE_CANVAS_ITEMS,
		"글자 선명도는 canvas_items 스트레치에 달려 있다. viewport 모드로 되돌리면 전체화면에서 텍스트가 뭉개진다.")
	assert(viewport_size.is_equal_approx(Vector2(1280, 720)),
		"논리 좌표계는 1280x720으로 고정되어야 절대 좌표 레이아웃(전투 무대 등)이 유지된다")

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	assert(image.get_width() == 1920 and image.get_height() == 1080,
		"1920x1080 창에서는 프레임버퍼도 1920x1080이어야 한다 (720p 확대가 아니라 네이티브 래스터화)")
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("FONT_SCALING_CAPTURE_PASS path=%s framebuffer=%dx%d viewport=%s canvas_scale=%.2f" % [
		OUTPUT_PATH, image.get_width(), image.get_height(), str(viewport_size), canvas_scale.x,
	])
	get_tree().quit(0)
