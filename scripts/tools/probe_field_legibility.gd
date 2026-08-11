extends Node

## S238: 실루엣 경계를, 렌더 차이가 아니라 **스프라이트 알파**에서 얻는다.
##
## S237의 계측기는 "캐릭터 있는 화면 - 없는 화면"으로 마스크를 만들었다.
## 진단해 보니 그 마스크는 캐릭터가 아니었다: 모든 맵에서 외접 상자가 표본 상자
## 전체(140x220)였고 rim_forest는 채움 1.00, 즉 상자가 통째로 "캐릭터"로 잡혔다.
## 캐릭터는 2000~4000px인데 마스크는 최대 30,796px였다. 배경(렌즈 맥동, 안개,
## 흔들리는 초목)이 프레임마다 변해서 전부 차이로 잡힌 것이다.
## 그래서 S237이 보고한 gain은 신뢰할 수 없다.
##
## 이번에는 렌더를 전혀 쓰지 않고 마스크를 만든다. 스프라이트의 현재 프레임
## 텍스처에서 알파를 읽어 경계 텍셀을 찾고, 그 텍셀과 "바로 바깥의 투명한 이웃"을
## 각각 화면 좌표로 옮긴다. 배경이 무엇을 하든 마스크는 흔들리지 않는다.
##
## 판정은 짝지어 한다: 테두리를 끈 화면과 켠 화면을 연속 프레임으로 찍고,
## 같은 경계 좌표에서 (안쪽 - 바깥쪽) 대비를 각각 잰다.

const MAPS: Array[String] = [
	"rim_forest", "verdan_market", "belt_waystation", "drift_shelter", "crumbling_coast",
	"the_seam", "seam_outskirts", "forgotten_forest", "colorless_waste", "bl07_void",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var gains: Array[float] = []
	for map_id in MAPS:
		var gain := await _measure(map_id)
		if gain > 0.0:
			gains.append(gain)
	if not gains.is_empty():
		var total := 0.0
		for g in gains:
			total += g
		print("LEGIBILITY_SUMMARY mean_gain=%.2fx maps=%d" % [total / gains.size(), gains.size()])
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
	var sprite := player.get_node_or_null("AnimatedSprite2D") as AnimatedSprite2D if player != null else null
	var material := sprite.material as ShaderMaterial if sprite != null else null
	if material == null:
		map.queue_free()
		await get_tree().process_frame
		return -1.0

	var pairs := _boundary_pairs(sprite)
	if pairs.is_empty():
		map.queue_free()
		await get_tree().process_frame
		return -1.0

	# 게임이 실제로 쓰는 값(지면 밝기에 맞춰진 값)으로 켜야 의미가 있다.
	var shipped: float = float(material.get_shader_parameter("rim_strength"))
	# 배경은 프레임마다 조금씩 변한다(반딧불, 비, 물결). 그 표류를 상쇄하려면
	# 켠 화면을 끈 화면 두 장 **사이에** 끼워 찍고, 앞뒤 평균과 비교해야 한다.
	# 앞의 한 장하고만 비교하면 신호가 대조군보다 두 배 긴 시간 간격을 갖게 되어
	# 표류가 효과로 둔갑한다.
	material.set_shader_parameter("rim_strength", 0.0)
	await get_tree().process_frame
	var before := _edge_contrast(await _grab(), pairs)
	material.set_shader_parameter("rim_strength", shipped)
	await get_tree().process_frame
	var on := _edge_contrast(await _grab(), pairs)
	material.set_shader_parameter("rim_strength", 0.0)
	await get_tree().process_frame
	var after := _edge_contrast(await _grab(), pairs)
	material.set_shader_parameter("rim_strength", shipped)

	var off := (before + after) * 0.5
	# 대조군: 같은 조건 두 장의 편차. 이 맵에서 계측기가 갖는 잡음 바닥이다.
	var null_gain := after / maxf(before, 0.0001)
	var gain := on / maxf(off, 0.0001)
	print("LEGIBILITY %-18s gain=%.2fx null=%.2fx rim=%.2f off=%.4f on=%.4f" % [map_id, gain, null_gain, shipped, off, on])

	map.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return gain

func _grab() -> Image:
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

## 현재 프레임 텍스처의 알파에서 경계 텍셀과 그 바깥 이웃을 찾아 화면 좌표로 옮긴다.
func _boundary_pairs(sprite: AnimatedSprite2D) -> Array:
	var frames := sprite.sprite_frames
	if frames == null:
		return []
	var texture := frames.get_frame_texture(sprite.animation, sprite.frame)
	if texture == null:
		return []
	var image := texture.get_image()
	if image == null or image.is_empty():
		return []
	var width := image.get_width()
	var height := image.get_height()
	var transform := sprite.get_global_transform_with_canvas()
	# AnimatedSprite2D는 텍스처를 중심 정렬로 그리고 offset을 더한다.
	var origin := Vector2(-width, -height) * 0.5 + sprite.offset
	var pairs: Array = []
	for y in range(1, height - 1):
		for x in range(1, width - 1):
			if image.get_pixel(x, y).a <= 0.6:
				continue
			for step in [Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1)]:
				if image.get_pixel(x + step.x, y + step.y).a > 0.2:
					continue
				# 바깥으로 두 텍셀 나가야 외곽선 자체가 아닌 배경을 집는다.
				var inside := transform * (origin + Vector2(x + 0.5, y + 0.5))
				var outside := transform * (origin + Vector2(x + 0.5, y + 0.5) + Vector2(step) * 3.0)
				pairs.append([inside, outside])
				break
	return pairs

func _edge_contrast(image: Image, pairs: Array) -> float:
	var total := 0.0
	var samples := 0
	for pair in pairs:
		var a: Vector2 = pair[0]
		var b: Vector2 = pair[1]
		if not _inside(image, a) or not _inside(image, b):
			continue
		total += absf(_luma(image.get_pixel(int(a.x), int(a.y))) - _luma(image.get_pixel(int(b.x), int(b.y))))
		samples += 1
	return total / maxf(float(samples), 1.0)

func _inside(image: Image, point: Vector2) -> bool:
	return point.x >= 0.0 and point.y >= 0.0 and point.x < image.get_width() and point.y < image.get_height()

func _luma(color: Color) -> float:
	return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b
