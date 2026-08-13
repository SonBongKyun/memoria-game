extends Node

## S243: VN CG 위 각 레이어가 **실제로 칠하는 픽셀**을 차분 영상으로 뽑는다.
##
## 앞선 계측은 띠 안쪽/바깥의 평균 밝기 차이를 봤는데, 여섯 장면 중 넷에서
## 델타가 정확히 0이었다. 즉 그 장면들에서는 레이어가 아예 안 그려진다.
## 효과가 있는 둘도 0.006~0.008로 작았다. 그런데 원본 스크린샷에는 네 변이
## 뚜렷한 띠가 분명히 보인다. 평균값 지표로는 어느 레이어가 그걸 그리는지
## 가려지지 않는다.
##
## 그래서 레이어를 하나씩 껐다 켜고 화면을 빼서, 그 레이어가 건드린 자리를
## 그대로 그림으로 남긴다. 차이를 8배 증폭해 눈에 보이게 한다. 어느 레이어가
## 어디를 칠하는지 추측할 필요가 없어진다.

const SCENE_ID := "ch13_third_person"
const LAYERS: Array[String] = ["_cg_detail_top", "_cg_focus_glow", "_cg_lower_wash", "_cg_vignette"]
const AMPLIFY := 8.0

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	await get_tree().process_frame

	SceneFlow.play(SCENE_ID, 0)
	await get_tree().create_timer(0.5).timeout
	for i in range(2):
		SceneFlow.advance()
		await get_tree().create_timer(0.2).timeout
	await get_tree().create_timer(1.8).timeout

	var ui: Node = SceneFlow.get("_vn_ui")
	if ui == null:
		print("VN_FOOTPRINT ui=null")
		get_tree().quit(1)
		return
	# 맥동하는 트윈을 전부 죽여야 두 캡처가 같은 조건이 된다.
	for layer_name in LAYERS:
		var node := ui.get(layer_name) as CanvasItem
		if node != null and node.has_meta("detail_tween"):
			var prev = node.get_meta("detail_tween")
			if prev != null and prev is Tween and prev.is_valid():
				prev.kill()
	await get_tree().process_frame

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	for layer_name in LAYERS:
		var node := ui.get(layer_name) as CanvasItem
		if node == null:
			print("VN_FOOTPRINT %-18s <null>" % layer_name)
			continue
		var shipped := node.modulate.a
		var was_visible := node.visible
		var on := await _grab()
		node.visible = false
		await get_tree().process_frame
		var off := await _grab()
		node.visible = was_visible
		await get_tree().process_frame

		var diff := _difference(on, off)
		diff["image"].save_png(ProjectSettings.globalize_path(
			"res://tmp/visual_audit/vn_footprint_%s.png" % layer_name.substr(1)))
		var edge := _edge_sharpness(on, off)
		print("VN_FOOTPRINT %-18s alpha=%.3f touched=%.1f%% max_delta=%.4f bbox=(%d, %d)-(%d, %d) step_x=%.4f@%d step_y=%.4f@%d" % [
			layer_name, shipped, diff["touched"] * 100.0, diff["max"],
			diff["bbox"].position.x, diff["bbox"].position.y,
			diff["bbox"].end.x, diff["bbox"].end.y,
			edge["step_x"], edge["at_x"], edge["step_y"], edge["at_y"]])
	print("VN_FOOTPRINT_DONE")
	get_tree().quit(0)

func _grab() -> Image:
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

func _difference(on: Image, off: Image) -> Dictionary:
	var width := on.get_width()
	var height := on.get_height()
	var out := Image.create(width, height, false, Image.FORMAT_RGB8)
	var touched := 0
	var max_delta := 0.0
	var min_x := width
	var min_y := height
	var max_x := 0
	var max_y := 0
	for y in range(height):
		for x in range(width):
			var a := on.get_pixel(x, y)
			var b := off.get_pixel(x, y)
			var delta := absf(_luma(a) - _luma(b))
			if delta > 0.002:
				touched += 1
				max_delta = maxf(max_delta, delta)
				min_x = mini(min_x, x)
				min_y = mini(min_y, y)
				max_x = maxi(max_x, x)
				max_y = maxi(max_y, y)
			var shown := clampf(delta * AMPLIFY, 0.0, 1.0)
			out.set_pixel(x, y, Color(shown, shown, shown))
	if touched == 0:
		min_x = 0
		min_y = 0
	return {
		"image": out,
		"touched": float(touched) / float(width * height),
		"max": max_delta,
		"bbox": Rect2i(Vector2i(min_x, min_y), Vector2i(max_x - min_x, max_y - min_y)),
	}

## 경계가 얼마나 날카로운지. 레이어가 기여한 양(on-off)을 행/열로 평균 내고
## 이웃 줄 사이의 최대 도약을 잰다. 부드러운 광원은 도약이 작고, 사각형으로
## 뚝 끊기는 것은 그 자리에 뾰족한 값이 선다.
func _edge_sharpness(on: Image, off: Image) -> Dictionary:
	var width := on.get_width()
	var height := on.get_height()
	var rows: Array[float] = []
	var cols: Array[float] = []
	cols.resize(width)
	cols.fill(0.0)
	for y in range(height):
		var row_total := 0.0
		for x in range(width):
			var delta := _luma(on.get_pixel(x, y)) - _luma(off.get_pixel(x, y))
			row_total += delta
			cols[x] += delta
		rows.append(row_total / float(width))
	for x in range(width):
		cols[x] /= float(height)
	var step_y := 0.0
	var at_y := 0
	for y in range(1, height):
		var jump := absf(rows[y] - rows[y - 1])
		if jump > step_y:
			step_y = jump
			at_y = y
	var step_x := 0.0
	var at_x := 0
	for x in range(1, width):
		var jump := absf(cols[x] - cols[x - 1])
		if jump > step_x:
			step_x = jump
			at_x = x
	return {"step_x": step_x, "at_x": at_x, "step_y": step_y, "at_y": at_y}

func _luma(color: Color) -> float:
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b
