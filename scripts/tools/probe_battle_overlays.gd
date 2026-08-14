extends Node

## S249: 전투 화면에서 무엇이 실제로 무대를 어둡게 하는지 하나씩 꺼서 잰다.
##
## 두 번 추측했고 두 번 빗나갔다. 3D 무대 조명을 지역별로 올렸더니 밝기가
## 0.0813 → 0.0825로 거의 안 움직였고, 배경 CG의 후퇴량을 지역별로 풀었더니
## 0.0833이었다. 둘 다 지배 항이 아니었다.
##
## 이름을 추측하지 않는다. 전체 화면급 오버레이를 전부 찾아 하나씩 끄고 표본 띠의
## 밝기가 얼마나 오르는지 잰다. 가장 크게 올리는 것이 범인이다.
## (VN에서 하드 엣지 사각형의 정체를 가른 것도 같은 방법이었다.)

const BAND_TOP := 0.25
const BAND_BOTTOM := 0.58

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 1
	GameManager.change_state(GameManager.GameState.BATTLE)
	BattleManager.start_battle("ash_crawler", "res://scenes/maps/rim_forest.tscn")

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.8).timeout

	var base := _band_luma(await _grab())
	print("OVERLAY_BASE luma=%.4f" % base)

	var viewport := get_viewport().get_visible_rect().size
	var candidates: Array = []
	_collect(battle, viewport, candidates)
	# 큰 것부터 본다.
	candidates.sort_custom(func(a, b): return a["area"] > b["area"])

	for entry in candidates.slice(0, 14):
		var node: CanvasItem = entry["node"]
		if not is_instance_valid(node) or not node.visible:
			continue
		node.visible = false
		await get_tree().process_frame
		var without := _band_luma(await _grab())
		node.visible = true
		await get_tree().process_frame
		var lift := without - base
		if absf(lift) >= 0.002:
			print("OVERLAY %-28s %-14s area=%.0f%%  끄면 %+.4f" % [
				entry["name"], entry["class"], entry["area"] / (viewport.x * viewport.y) * 100.0, lift])
	print("BATTLE_OVERLAYS_DONE")
	get_tree().quit(0)

func _grab() -> Image:
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

## 화면의 3할 이상을 덮는 그리기 노드만 후보로 삼는다.
func _collect(node: Node, viewport: Vector2, out: Array) -> void:
	var item := node as CanvasItem
	if item != null and item.visible:
		var control := item as Control
		if control != null:
			var rect := control.get_global_rect()
			var area := rect.get_area()
			var draws := control is ColorRect or control is TextureRect or control is Panel
			if draws and area >= viewport.x * viewport.y * 0.3:
				out.append({"node": item, "name": String(control.name), "class": control.get_class(), "area": area})
	for child in node.get_children():
		_collect(child, viewport, out)

func _band_luma(image: Image) -> float:
	var top := int(image.get_height() * BAND_TOP)
	var bottom := int(image.get_height() * BAND_BOTTOM)
	var total := 0.0
	var count := 0
	for y in range(top, bottom, 2):
		for x in range(0, image.get_width(), 3):
			var c := image.get_pixel(x, y)
			total += 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
			count += 1
	return total / maxf(float(count), 1.0)
