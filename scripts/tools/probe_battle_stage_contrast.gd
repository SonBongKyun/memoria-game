extends Node

## S241: 전투 무대에서 각 전투원이 얼마나 "보이는지"를 잰다.
##
## 콘택트 시트에서 반복해 걸린 것: 왼쪽 아군은 또렷한데 오른쪽 적은 배경에 잠겨
## 있다. 그림자 파수꾼과 카이로스는 어디에 서 있는지조차 알기 어렵다. 턴제에서
## 적은 조준 대상이므로 분위기가 아니라 가독성 문제다.
##
## 첫 시도는 무효였다. 판 중앙 35% 반경을 "몸통"으로 잡았는데, 아렐에게 그 자리는
## 얼굴이 아니라 검은 코트였다(luma 0.046). 눈이 읽는 얼굴과 머리카락은 표본 밖에
## 있었고, 그래서 아군이 적보다 14배 안 보인다는 거꾸로 된 답이 나왔다. 게다가
## 플레이트는 STRETCH_KEEP_ASPECT_CENTERED라 텍스처가 사각형을 채우지도 않아
## UV 매핑까지 어긋나 있었다.
##
## 이번에는 "어디가 얼굴인가"를 추측하지 않는다. 실제로 그려지는 영역 전체의
## 평균 밝기와 국소 대비(RMS)를 재고, 같은 화면의 빈 무대 한 조각을 대조군으로
## 함께 잰다. 안 보이는 캐릭터란 곧 그 자리의 구조량이 빈 배경과 다르지 않다는 뜻이다.
## 그리는 사각형은 KEEP_ASPECT_CENTERED 규칙으로 직접 계산해 어긋남을 없앤다.

const ENCOUNTERS: Array[Dictionary] = [
	{"preset": "ash_crawler", "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"preset": "forest_shade", "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
	{"preset": "void_beast", "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"preset": "threshold_shade", "chapter": 7, "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"preset": "shade_sentinel", "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"preset": "kairos", "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"

	var by_role: Dictionary = {"player": [], "ally": [], "enemy": [], "stage": []}
	for encounter in ENCOUNTERS:
		var result := await _measure(encounter)
		for role in result:
			by_role[role].append(result[role])
	for role in ["player", "ally", "enemy", "stage"]:
		var rows: Array = by_role[role]
		if rows.is_empty():
			continue
		var mean_total := 0.0
		var rms_total := 0.0
		for row in rows:
			mean_total += float(row["mean"])
			rms_total += float(row["rms"])
		print("STAGE_STRUCTURE_MEAN %-7s luma=%.4f rms=%.4f n=%d" % [
			role, mean_total / rows.size(), rms_total / rows.size(), rows.size()])
	print("BATTLE_STAGE_CONTRAST_DONE")
	get_tree().quit(0)

func _measure(encounter: Dictionary) -> Dictionary:
	GameManager.current_chapter = int(encounter["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = GameManager.current_chapter >= 4
	BattleManager.tobias_in_party = GameManager.current_chapter >= 3 and GameManager.current_chapter < 7
	BattleManager.start_battle(String(encounter["preset"]), String(encounter["scene"]))

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.6).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()

	var out: Dictionary = {}
	for role in ["player", "ally", "enemy"]:
		var node := battle.get(role + "_sprite") as CanvasItem
		if node == null or not node.visible:
			continue
		var reading := _read_plate(image, node)
		if reading.is_empty():
			continue
		out[role] = reading
		print("STAGE_STRUCTURE %-15s %-7s luma=%.4f rms=%.4f src_rms=%.4f fit_rms=%.4f drawn=%s" % [
			encounter["preset"], role, reading["mean"], reading["rms"],
			reading.get("src_rms", -1.0), reading.get("fit_rms", -1.0), reading.get("drawn", "?")])

	# 대조군: 무대 한가운데의 빈 대역. 전투원이 서 있지 않은 곳이다.
	var stage := _read_patch(image, Rect2(
		image.get_width() * 0.50, image.get_height() * 0.28,
		image.get_width() * 0.12, image.get_height() * 0.20))
	if not stage.is_empty():
		out["stage"] = stage
		print("STAGE_STRUCTURE %-15s %-7s luma=%.4f rms=%.4f px=%d" % [
			encounter["preset"], "stage", stage["mean"], stage["rms"], stage["samples"]])

	battle.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return out

## 실제로 그려지는 텍스처 사각형을 KEEP_ASPECT_CENTERED 규칙으로 계산하고,
## 셰이더의 타원 마스크가 남기는 안쪽만 표본으로 삼는다.
func _read_plate(image: Image, node: CanvasItem) -> Dictionary:
	var texture: Texture2D = node.get("texture") as Texture2D
	if texture == null:
		return {}
	var rect_size: Vector2 = node.get("size")
	var tex_size: Vector2 = texture.get_size()
	if rect_size.x <= 1.0 or rect_size.y <= 1.0 or tex_size.x <= 1.0 or tex_size.y <= 1.0:
		return {}
	var scale: float = minf(rect_size.x / tex_size.x, rect_size.y / tex_size.y)
	var drawn: Vector2 = tex_size * scale
	var origin: Vector2 = (rect_size - drawn) * 0.5
	var xform := node.get_global_transform_with_canvas()

	var points: Array[Vector2] = []
	var step := 2
	for y in range(0, int(drawn.y), step):
		for x in range(0, int(drawn.x), step):
			var uv := Vector2(float(x) / drawn.x, float(y) / drawn.y)
			# 셰이더와 같은 타원식. 0.78부터 알파가 깎이므로 확실히 남는 안쪽만 쓴다.
			var centered := (uv - Vector2(0.5, 0.5)) * Vector2(1.0, 0.92)
			if centered.length() * 2.0 > 0.76:
				continue
			var screen := xform * (origin + Vector2(x, y))
			if _inside(image, screen):
				points.append(screen)
	var rendered := _stats(image, points)
	# 원화 자체의 구조량. 렌더된 값과 나란히 놓으면 원인이 그림인지 렌더 경로인지 갈린다.
	var source := texture.get_image()
	if not rendered.is_empty() and source != null and not source.is_empty():
		var src_points: Array[Vector2] = []
		for y in range(0, source.get_height(), 3):
			for x in range(0, source.get_width(), 3):
				var uv := Vector2(float(x) / source.get_width(), float(y) / source.get_height())
				var centered := (uv - Vector2(0.5, 0.5)) * Vector2(1.0, 0.92)
				if centered.length() * 2.0 > 0.76:
					continue
				if source.get_pixel(x, y).a < 0.5:
					continue
				src_points.append(Vector2(x, y))
		var src_stats := _stats(source, src_points)
		if not src_stats.is_empty():
			rendered["src_mean"] = src_stats["mean"]
			rendered["src_rms"] = src_stats["rms"]
		# 그려지는 크기로 줄인 원화. 손실이 축소 탓인지 셰이더 탓인지 가른다.
		var shrunk := Image.create_from_data(source.get_width(), source.get_height(),
			source.has_mipmaps(), source.get_format(), source.get_data())
		shrunk.resize(maxi(int(drawn.x), 1), maxi(int(drawn.y), 1), Image.INTERPOLATE_LANCZOS)
		var shrunk_points: Array[Vector2] = []
		for y in range(0, shrunk.get_height()):
			for x in range(0, shrunk.get_width()):
				var uv := Vector2(float(x) / shrunk.get_width(), float(y) / shrunk.get_height())
				var centered := (uv - Vector2(0.5, 0.5)) * Vector2(1.0, 0.92)
				if centered.length() * 2.0 > 0.76:
					continue
				if shrunk.get_pixel(x, y).a < 0.5:
					continue
				shrunk_points.append(Vector2(x, y))
		var shrunk_stats := _stats(shrunk, shrunk_points)
		if not shrunk_stats.is_empty():
			rendered["fit_rms"] = shrunk_stats["rms"]
		rendered["drawn"] = "%dx%d" % [int(drawn.x), int(drawn.y)]
	return rendered

func _read_patch(image: Image, rect: Rect2) -> Dictionary:
	var points: Array[Vector2] = []
	for y in range(int(rect.position.y), int(rect.end.y), 2):
		for x in range(int(rect.position.x), int(rect.end.x), 2):
			var point := Vector2(x, y)
			if _inside(image, point):
				points.append(point)
	return _stats(image, points)

func _stats(image: Image, points: Array[Vector2]) -> Dictionary:
	if points.size() < 400:
		return {}
	var lumas: Array[float] = []
	var total := 0.0
	for point in points:
		var luma := _luma(image.get_pixel(int(point.x), int(point.y)))
		lumas.append(luma)
		total += luma
	var mean := total / float(lumas.size())
	var variance := 0.0
	for luma in lumas:
		variance += (luma - mean) * (luma - mean)
	return {"mean": mean, "rms": sqrt(variance / float(lumas.size())), "samples": lumas.size()}

func _inside(image: Image, point: Vector2) -> bool:
	return point.x >= 0.0 and point.y >= 0.0 and point.x < image.get_width() and point.y < image.get_height()

func _luma(color: Color) -> float:
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b
