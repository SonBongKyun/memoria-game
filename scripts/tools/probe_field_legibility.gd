extends Node

## S237: 실루엣이 배경에서 떠오르는가를, 실루엣 경계에서만 잰다.
##
## S236에서 이 측정을 세 번 시도해 세 번 실패했다:
##   1) 몸통-배경 평균 차이 — 테두리는 1~2px만 바꾸므로 평균이 반응하지 않는다.
##   2) 가로줄 최대 기울기 — 주변 소품까지 훑어 오염된다.
##   3) 맵에서의 확대 A/B — 파티클과 안개가 프레임마다 달라 셰이더 차이를 가린다.
##
## 이번에는 텍스처 좌표 계산도, 배경 추정도 하지 않는다.
## 같은 장면을 캐릭터가 있을 때와 없을 때 두 번 렌더한다. 두 그림의 차이가
## 곧 캐릭터가 화면에서 차지한 정확한 마스크다. 그 마스크의 경계 픽셀에서
## "캐릭터가 그린 색"과 "그 자리에 원래 있던 배경색"을 같은 좌표로 비교한다.
##
## 소품도, 스프라이트 스케일도 개입할 수 없다. 배경은 추정이 아니라 실측이다.
##
## 다만 배경은 완전히 정지하지 않는다(렌즈 셰이더의 TIME 맥동, 흐르는 안개).
## 두 번 렌더 사이의 그 미세한 변화가 마스크에 섞이면 경계 픽셀이 수만 개로 불어난다.
## 그래서 두 가지를 더 한다:
##   - 마스크를 플레이어 주변 상자로 한정한다.
##   - 절대값 하나를 믿는 대신, 같은 실행 안에서 테두리를 끈 화면과 켠 화면을
##     연속 프레임으로 찍어 **짝지어** 비교한다. 배경 흔들림은 두 조건에 똑같이
##     들어가므로 비교에서 상쇄된다.

const MAPS: Array[String] = [
	"rim_forest", "verdan_market", "belt_waystation", "drift_shelter", "crumbling_coast",
	"the_seam", "seam_outskirts", "forgotten_forest", "colorless_waste", "bl07_void",
]
## 캐릭터가 차지했다고 인정할 최소 변화량. 남은 미세 흔들림을 배제한다.
const OCCUPANCY_THRESHOLD: float = 0.02
## 플레이어를 넉넉히 감싸는 상자 (표시 높이 50px, 카메라 줌 2.25 기준).
const MASK_HALF_WIDTH: int = 70
const MASK_HALF_HEIGHT: int = 110
const RIM_STRENGTH: float = 0.42

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	# 파티클과 흔들림을 멈춰 두 번의 렌더가 같은 배경을 갖게 한다.
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var total := 0.0
	var count := 0
	var worst := 999.0
	var worst_map := ""
	for map_id in MAPS:
		var value := await _measure(map_id)
		if value < 0.0:
			continue
		total += value
		count += 1
		if value < worst:
			worst = value
			worst_map = map_id
	if count > 0:
		print("LEGIBILITY_SUMMARY mean_on=%.4f worst_on=%.4f (%s) maps=%d" % [total / count, worst, worst_map, count])
	print("FIELD_LEGIBILITY_PROBE_DONE")
	get_tree().quit(0)

func _measure(map_id: String) -> float:
	var path := "res://scenes/maps/%s.tscn" % map_id
	if not ResourceLoader.exists(path):
		return -1.0
	var map: Node = load(path).instantiate()
	add_child(map)
	await get_tree().create_timer(1.4).timeout

	var player := get_tree().get_first_node_in_group("player") as Node2D
	if player == null:
		map.queue_free()
		await get_tree().process_frame
		return -1.0

	var sprite := player.get_node_or_null("AnimatedSprite2D") as AnimatedSprite2D
	var material := sprite.material as ShaderMaterial if sprite != null else null
	if material == null:
		map.queue_free()
		await get_tree().process_frame
		return -1.0

	var screen := player.get_global_transform_with_canvas().origin
	# 테두리 끈 화면 -> 켠 화면 -> 캐릭터 없는 화면. 연속 프레임으로 찍는다.
	material.set_shader_parameter("rim_strength", 0.0)
	await get_tree().process_frame
	var rim_off := await _grab()
	material.set_shader_parameter("rim_strength", RIM_STRENGTH)
	await get_tree().process_frame
	var rim_on := await _grab()
	# Camera2D는 CanvasItem이 아니라 부모를 숨겨도 그대로 동작한다. 화면은 움직이지 않는다.
	player.visible = false
	await get_tree().process_frame
	var background := await _grab()
	player.visible = true

	var off := _boundary_separation(rim_off, background, screen)
	var on := _boundary_separation(rim_on, background, screen)
	var gain := float(on["separation"]) / maxf(float(off["separation"]), 0.0001)
	print("LEGIBILITY %-18s off=%.4f on=%.4f gain=%.2fx boundary_px=%d" % [
		map_id, float(off["separation"]), float(on["separation"]), gain, int(off["boundary"])
	])
	# 같은 실행, 연속 프레임이므로 이 두 장은 정당한 A/B다. 눈으로도 확인한다.
	if map_id in ["forgotten_forest", "the_seam", "drift_shelter"]:
		_save_pair(map_id, rim_off, rim_on, screen)
	var result := on

	map.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return float(result["separation"])

func _grab() -> Image:
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

## 두 그림의 차이로 캐릭터 마스크를 만들고, 그 경계에서만 대비를 잰다.
func _boundary_separation(with_actor: Image, without_actor: Image, focus: Vector2) -> Dictionary:
	# 플레이어 주변만 본다. 먼 곳의 안개나 파티클이 마스크에 섞이지 않도록.
	var x0 := maxi(int(focus.x) - MASK_HALF_WIDTH, 0)
	var x1 := mini(int(focus.x) + MASK_HALF_WIDTH, with_actor.get_width())
	var y0 := maxi(int(focus.y) - MASK_HALF_HEIGHT, 0)
	var y1 := mini(int(focus.y) + MASK_HALF_HEIGHT, with_actor.get_height())
	var occupied := {}
	for y in range(y0, y1):
		for x in range(x0, x1):
			var a := with_actor.get_pixel(x, y)
			var b := without_actor.get_pixel(x, y)
			if absf(a.r - b.r) + absf(a.g - b.g) + absf(a.b - b.b) > OCCUPANCY_THRESHOLD:
				occupied[Vector2i(x, y)] = true
	if occupied.is_empty():
		return {"separation": 0.0, "boundary": 0, "body": 0}

	# 경계 = 캐릭터가 칠한 픽셀 중, 이웃 하나라도 캐릭터가 아닌 것.
	var total := 0.0
	var samples := 0
	for key in occupied:
		var point: Vector2i = key
		var is_boundary := false
		for offset in [Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1)]:
			if not occupied.has(point + offset):
				is_boundary = true
				break
		if not is_boundary:
			continue
		# 캐릭터가 그린 색 vs 그 자리에 원래 있던 배경색. 같은 좌표라 정렬 오차가 없다.
		total += absf(_luma(with_actor.get_pixel(point.x, point.y)) - _luma(without_actor.get_pixel(point.x, point.y)))
		samples += 1
	return {
		"separation": total / maxf(float(samples), 1.0),
		"boundary": samples,
		"body": occupied.size(),
	}

func _save_pair(map_id: String, off: Image, on: Image, focus: Vector2) -> void:
	var size := Vector2i(140, 150)
	var origin := Vector2i(int(focus.x) - size.x / 2, int(focus.y) - size.y + 24)
	var sheet := Image.create(size.x * 2, size.y, false, off.get_format())
	sheet.blit_rect(off, Rect2i(origin, size), Vector2i.ZERO)
	sheet.blit_rect(on, Rect2i(origin, size), Vector2i(size.x, 0))
	sheet.resize(size.x * 6, size.y * 3, Image.INTERPOLATE_NEAREST)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	sheet.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/rim_%s.png" % map_id))

func _luma(color: Color) -> float:
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b
