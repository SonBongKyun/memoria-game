extends Node

## S237 개발용. 캐릭터가 배경에서 실제로 얼마나 떠오르는지 잰다.
##
## 실루엣 마감 셰이더의 외곽선은 거의 검정(0.025, 0.035, 0.060) 고정이다.
## 밝은 숲길에서는 잘 듣지만, 대부분의 맵은 어둡다(휘도 0.09~0.19).
## 어두운 배경에 검은 테두리는 아무 일도 하지 않는다. 그 가설을 숫자로 확인한다.
##
## 측정: 플레이어 발밑 기준으로 몸통 영역과 그 바깥 고리의 평균 휘도 차이.
## 값이 클수록 배경에서 떠오른다.

const MAPS: Array[String] = [
	"rim_forest", "verdan_market", "belt_waystation", "drift_shelter", "crumbling_coast",
	"the_seam", "seam_outskirts", "forgotten_forest", "colorless_waste", "bl07_void",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	var total := 0.0
	var count := 0
	var worst := 999.0
	var worst_map := ""
	for map_id in MAPS:
		var separation := await _measure(map_id)
		if separation < 0.0:
			continue
		total += separation
		count += 1
		if separation < worst:
			worst = separation
			worst_map = map_id
	if count > 0:
		print("LEGIBILITY_SUMMARY mean_edge=%.4f worst_edge=%.4f (%s) maps=%d" % [total / count, worst, worst_map, count])
	print("FIELD_LEGIBILITY_PROBE_DONE")
	get_tree().quit(0)

func _measure(map_id: String) -> float:
	var path := "res://scenes/maps/%s.tscn" % map_id
	if not ResourceLoader.exists(path):
		return -1.0
	var map: Node = load(path).instantiate()
	add_child(map)
	await get_tree().create_timer(1.3).timeout
	await RenderingServer.frame_post_draw

	var player := get_tree().get_first_node_in_group("player") as Node2D
	if player == null:
		map.queue_free()
		await get_tree().process_frame
		return -1.0
	# 플레이어의 화면 좌표. 카메라 변환을 그대로 쓴다.
	var screen := player.get_global_transform_with_canvas().origin
	var image := get_viewport().get_texture().get_image()

	# 실루엣이 "떠오르는가"는 몸통 평균이 아니라 경계에서 생기는 밝기 단차다.
	# 테두리는 1~2px만 바꾸므로 평균으로는 보이지 않는다. 가로로 훑어 최대 기울기를 잰다.
	var edge := 0.0
	var rows := 0
	for row_offset in range(-58, -12, 4):
		var row_edge := _max_gradient(image, screen + Vector2(0, float(row_offset)), 46)
		edge = maxf(edge, row_edge)
		rows += 1
	var body := _mean_luma(image, screen + Vector2(0, -34), Vector2(14, 26))
	var background := (
		_mean_luma(image, screen + Vector2(-38, -34), Vector2(12, 26))
		+ _mean_luma(image, screen + Vector2(38, -34), Vector2(12, 26))
	) * 0.5
	print("LEGIBILITY %-18s edge=%.4f body=%.3f bg=%.3f flat=%.4f" % [map_id, edge, body, background, absf(body - background)])

	# 수치만으로는 판단하지 않는다. 가장 어두운 맵은 캐릭터 주변을 확대해 남긴다.
	if map_id in ["forgotten_forest", "bl07_void", "drift_shelter"]:
		var crop := Image.create(120, 110, false, image.get_format())
		crop.blit_rect(image, Rect2i(Vector2i(int(screen.x) - 60, int(screen.y) - 92), Vector2i(120, 110)), Vector2i.ZERO)
		crop.resize(480, 440, Image.INTERPOLATE_NEAREST)
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
		crop.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/legibility_%s.png" % map_id))

	map.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return edge

## 한 가로줄을 훑으며 인접 픽셀 간 최대 휘도 단차를 찾는다.
## 실루엣 경계가 만드는 대비가 여기 잡힌다.
func _max_gradient(image: Image, center: Vector2, half_width: int) -> float:
	var y := int(center.y)
	if y < 0 or y >= image.get_height():
		return 0.0
	var best := 0.0
	var x0 := maxi(int(center.x) - half_width, 1)
	var x1 := mini(int(center.x) + half_width, image.get_width() - 1)
	for x in range(x0, x1):
		var a := image.get_pixel(x, y)
		var b := image.get_pixel(x + 1, y)
		var la := 0.2126 * a.r + 0.7152 * a.g + 0.0722 * a.b
		var lb := 0.2126 * b.r + 0.7152 * b.g + 0.0722 * b.b
		best = maxf(best, absf(lb - la))
	return best

func _mean_luma(image: Image, center: Vector2, half: Vector2) -> float:
	var total := 0.0
	var samples := 0
	var x0 := int(center.x - half.x)
	var x1 := int(center.x + half.x)
	var y0 := int(center.y - half.y)
	var y1 := int(center.y + half.y)
	for x in range(maxi(x0, 0), mini(x1, image.get_width())):
		for y in range(maxi(y0, 0), mini(y1, image.get_height())):
			var c := image.get_pixel(x, y)
			total += 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
			samples += 1
	return total / maxf(float(samples), 1.0)
