## MapEffects, 맵 환경 효과 유틸리티
## 물 반짝임, 랜턴 빛, 보이드 파티클 등.
class_name MapEffects

const TILE: int = 32

# S41: 셰이더 캐시 (한번 로드한 셰이더 재사용)
static var _shader_cache: Dictionary = {}

## S160: gameplay clarity is the default. Story CG/VN presentation is untouched;
## only screen-space effects layered over interactive maps are suppressed.
static func _clean_gameplay_view() -> bool:
	return OptionsMenu == null or OptionsMenu.is_clean_gameplay_visuals()

static func _empty_layer(parent: Node, layer_index: int = 0) -> CanvasLayer:
	var layer := CanvasLayer.new()
	layer.layer = layer_index
	parent.add_child(layer)
	return layer

static func _get_shader(path: String) -> Shader:
	if _shader_cache.has(path):
		return _shader_cache[path]
	if ResourceLoader.exists(path):
		var shader = load(path) as Shader
		_shader_cache[path] = shader
		return shader
	return null

## 물 타일 반짝임 효과 추가, 셰이더 기반 물 왜곡 (S40)
## parent에 추가된 ColorRect들을 반환 (caller가 _process에서 업데이트)
static func add_water_shimmer(parent: Node2D, map_data: Array, width: int, height: int, water_index: int) -> Array[ColorRect]:
	var shimmers: Array[ColorRect] = []
	if _clean_gameplay_view():
		return shimmers
	var shader_path = "res://assets/shaders/water_distortion.gdshader"
	var shader_res = _get_shader(shader_path)
	var has_shader = shader_res != null

	# S41: 물 반짝임 최적화, 5타일마다 1개로 줄여 ColorRect 수 감소
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == water_index:
					if (x + y) % 5 == 0:
						var rect = ColorRect.new()
						rect.size = Vector2(TILE, 2)
						rect.position = Vector2(x * TILE, y * TILE + randi_range(4, 28))
						rect.color = Color(0.4, 0.6, 0.8, 0.0)
						rect.z_index = 0
						rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
						rect.set_meta("phase", randf() * TAU)
						parent.add_child(rect)
						shimmers.append(rect)

	# 셰이더 기반 물 오버레이, 넓은 물 영역에 왜곡 효과
	if has_shader:
		_add_water_overlay(parent, map_data, width, height, water_index, shader_res)

	return shimmers

## 물 영역에 셰이더 오버레이 배치 (왜곡 + 반짝임)
static func _add_water_overlay(parent: Node2D, map_data: Array, width: int, height: int, water_index: int, shader_res: Shader) -> void:
	# 물 타일 연속 영역을 row 단위로 그룹핑
	for y in range(height):
		var start_x = -1
		for x in range(width + 1):
			var is_water = false
			if x < width and y < map_data.size() and x < map_data[y].size():
				is_water = (map_data[y][x] == water_index)
			if is_water and start_x < 0:
				start_x = x
			elif not is_water and start_x >= 0:
				# 연속 물 구간 발견, 오버레이 배치
				var span = x - start_x
				if span >= 2:  # 최소 2타일 이상
					var overlay = ColorRect.new()
					overlay.size = Vector2(span * TILE, TILE)
					overlay.position = Vector2(start_x * TILE, y * TILE)
					overlay.color = Color(0, 0, 0, 0)  # 셰이더가 알파 제어
					overlay.z_index = 1
					overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
					var mat = ShaderMaterial.new()
					mat.shader = shader_res
					mat.set_shader_parameter("wave_speed", 1.2 + randf() * 0.6)
					mat.set_shader_parameter("shimmer_intensity", 0.12)
					overlay.material = mat
					parent.add_child(overlay)
				start_x = -1

## 물 반짝임 업데이트 (_process에서 호출)
static func update_water_shimmer(shimmers: Array[ColorRect], time: float) -> void:
	for rect in shimmers:
		if is_instance_valid(rect):
			var phase = rect.get_meta("phase", 0.0)
			var alpha = (sin(time * 1.5 + phase) + 1.0) * 0.15
			rect.color.a = alpha

## 랜턴 빛 효과 추가, 셰이더 글로우 (S40)
static func add_lantern_lights(parent: Node2D, map_data: Array, width: int, height: int, lantern_index: int) -> Array[ColorRect]:
	var lights: Array[ColorRect] = []
	var shader_path = "res://assets/shaders/glow_pulse.gdshader"
	var shader_res = _get_shader(shader_path)
	var has_shader = shader_res != null

	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == lantern_index:
					var size = TILE * 3
					var rect = ColorRect.new()
					rect.size = Vector2(size, size)
					rect.position = Vector2(x * TILE - TILE, y * TILE - TILE)
					rect.z_index = 1
					rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
					rect.set_meta("phase", randf() * TAU)
					# 셰이더 적용
					if has_shader:
						rect.color = Color(0, 0, 0, 0)  # 셰이더가 알파 제어
						var mat = ShaderMaterial.new()
						mat.shader = shader_res
						mat.set_shader_parameter("glow_color", Color(0.95, 0.75, 0.3, 0.5))
						mat.set_shader_parameter("pulse_speed", 2.0 + randf() * 1.5)
						mat.set_shader_parameter("min_intensity", 0.25)
						mat.set_shader_parameter("max_intensity", 0.65)
						mat.set_shader_parameter("glow_radius", 0.4)
						rect.material = mat
					else:
						rect.color = Color(0.9, 0.7, 0.3, 0.08)
					parent.add_child(rect)
					lights.append(rect)
	return lights

## 랜턴 빛 업데이트 (_process에서 호출)
static func update_lantern_lights(lights: Array[ColorRect], time: float) -> void:
	for rect in lights:
		if is_instance_valid(rect):
			if rect.material:
				continue  # 셰이더가 자체 펄스 처리
			var phase = rect.get_meta("phase", 0.0)
			var flicker = 0.06 + sin(time * 3.0 + phase) * 0.02 + sin(time * 7.0 + phase * 2) * 0.01
			rect.color.a = flicker

## 보이드 파티클 생성 (떠다니는 보라색 입자)
## S59 호환: 맵별로 크기/색상/수량 커스터마이즈 가능
static func add_void_particles(parent: Node2D, map_width: float = 640.0, map_height: float = 640.0, color_override: Color = Color(0, 0, 0, 0), amount: int = 25) -> GPUParticles2D:
	var particles = GPUParticles2D.new()
	if _clean_gameplay_view():
		particles.emitting = false
		parent.add_child(particles)
		return particles
	var mat = ParticleProcessMaterial.new()

	mat.direction = Vector3(0, -0.3, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 5.0
	mat.initial_velocity_max = 15.0
	mat.gravity = Vector3(0, -5, 0)  # 위로 떠오르는 느낌
	mat.scale_min = 1.0
	mat.scale_max = 2.5
	# 색상 오버라이드 지원 (alpha > 0이면 커스텀)
	var base_color = Color(0.3, 0.1, 0.5, 0.4) if color_override.a == 0.0 else color_override
	mat.color = base_color

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	var fade_color = Color(base_color.r, base_color.g, base_color.b, 0.0)
	var mid_color = Color(base_color.r * 1.2, base_color.g * 1.2, base_color.b * 1.2, base_color.a)
	g.set_color(0, fade_color)
	g.set_offset(0, 0.0)
	g.add_point(0.3, mid_color)
	g.add_point(0.7, base_color)
	g.set_color(1, fade_color)
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(map_width * 0.5, map_height * 0.5, 0)

	particles.process_material = mat
	particles.amount = amount
	particles.lifetime = 5.0
	particles.position = Vector2(map_width * 0.5, map_height * 0.5)
	particles.visibility_rect = Rect2(-map_width * 0.5, -map_height * 0.5, map_width, map_height)

	parent.add_child(particles)

	# S40: 보이드 환경 글로우 오버레이
	var shader_path = "res://assets/shaders/glow_pulse.gdshader"
	var glow_shader = _get_shader(shader_path)
	if glow_shader:
		var glow = ColorRect.new()
		glow.size = Vector2(200, 200)
		glow.position = Vector2(-100, -100)
		glow.color = Color(0, 0, 0, 0)  # 셰이더가 알파 제어
		glow.z_index = -1
		glow.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var glow_mat = ShaderMaterial.new()
		glow_mat.shader = glow_shader
		glow_mat.set_shader_parameter("glow_color", Color(0.4, 0.15, 0.6, 0.35))
		glow_mat.set_shader_parameter("pulse_speed", 1.2)
		glow_mat.set_shader_parameter("min_intensity", 0.15)
		glow_mat.set_shader_parameter("max_intensity", 0.45)
		glow_mat.set_shader_parameter("glow_radius", 0.45)
		glow.material = glow_mat
		parent.add_child(glow)

	return particles

## 맵 비네트 오버레이, 셰이더 기반 원형 비네트 (S40)
static func add_vignette(parent: Node, intensity: float = 0.4) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 3)
	var layer = CanvasLayer.new()
	layer.layer = 3  # 맵 위, UI 아래

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rect.color = Color(0, 0, 0, 0)  # 셰이더가 알파를 제어

	var shader_path = "res://assets/shaders/vignette.gdshader"
	var vignette_shader = _get_shader(shader_path)
	if vignette_shader:
		var shader_mat = ShaderMaterial.new()
		shader_mat.shader = vignette_shader
		shader_mat.set_shader_parameter("intensity", intensity)
		shader_mat.set_shader_parameter("outer_radius", 0.85)
		shader_mat.set_shader_parameter("inner_radius", 0.35)
		rect.material = shader_mat

	layer.add_child(rect)
	parent.add_child(layer)
	return layer

## 안개 효과 (CanvasLayer 기반, 카메라 독립)
static func add_fog(parent: Node, color: Color = Color(0.2, 0.2, 0.25, 0.08)) -> Array[ColorRect]:
	if _clean_gameplay_view():
		return []
	var layer = CanvasLayer.new()
	layer.layer = 2  # 비네트 아래

	var fogs: Array[ColorRect] = []
	for i in range(3):
		var fog = ColorRect.new()
		fog.size = Vector2(randi_range(300, 500), randi_range(100, 200))
		fog.position = Vector2(randf_range(-100, 900), randf_range(100, 500))
		fog.color = color
		fog.mouse_filter = Control.MOUSE_FILTER_IGNORE
		fog.set_meta("speed_x", randf_range(3.0, 8.0))
		fog.set_meta("phase", randf() * TAU)
		layer.add_child(fog)
		fogs.append(fog)

	parent.add_child(layer)
	return fogs

## 챕터 타이틀 카드 오버레이
## 페이드 인 → 홀드 → 페이드 아웃 후 자동 제거. CanvasLayer 반환.
static func show_chapter_title(parent: Node, chapter_num: int, title: String, subtitle: String = "") -> CanvasLayer:
	title = GameManager.localized_runtime_text(title)
	subtitle = GameManager.localized_runtime_text(subtitle)
	var layer = CanvasLayer.new()
	layer.layer = 4  # 비네트(3) 위, UI 아래

	var art_path := _get_chapter_art_path(chapter_num)

	# 풀스크린 어둠 배경
	var bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0, 0, 0, 0.78)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(bg)

	var art_bg: TextureRect = null
	var art_panel: TextureRect = null
	if art_path != "" and ResourceLoader.exists(art_path):
		art_bg = TextureRect.new()
		art_bg.set_anchors_preset(Control.PRESET_FULL_RECT)
		art_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		art_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		art_bg.texture = load(art_path)
		art_bg.modulate = Color(0.72, 0.66, 0.58, 0.0)
		art_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(art_bg)

		var lower_wash = ColorRect.new()
		lower_wash.anchor_left = 0.0
		lower_wash.anchor_right = 1.0
		lower_wash.anchor_top = 0.58
		lower_wash.anchor_bottom = 1.0
		lower_wash.color = Color(0.0, 0.0, 0.0, 0.42)
		lower_wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(lower_wash)

		art_panel = TextureRect.new()
		art_panel.anchor_left = 0.18
		art_panel.anchor_right = 0.82
		art_panel.anchor_top = 0.17
		art_panel.anchor_bottom = 0.48
		art_panel.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		art_panel.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		art_panel.texture = load(art_path)
		art_panel.modulate = Color(1.0, 0.94, 0.82, 0.0)
		art_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(art_panel)

	# 중앙 컨테이너
	var container = VBoxContainer.new()
	container.set_anchors_preset(Control.PRESET_CENTER)
	container.offset_top = 64
	container.grow_horizontal = Control.GROW_DIRECTION_BOTH
	container.grow_vertical = Control.GROW_DIRECTION_BOTH
	container.alignment = BoxContainer.ALIGNMENT_CENTER
	container.add_theme_constant_override("separation", 8)
	container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(container)

	# "CHAPTER X" 소형 앰버 텍스트
	var chapter_label = Label.new()
	chapter_label.text = "CHAPTER %d" % chapter_num
	chapter_label.add_theme_font_size_override("font_size", 16)
	chapter_label.add_theme_color_override("font_color", Color(0.6, 0.5, 0.35))
	chapter_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	chapter_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(chapter_label)

	# 타이틀 대형 텍스트
	var title_label = Label.new()
	title_label.text = title
	title_label.add_theme_font_size_override("font_size", 28)
	title_label.add_theme_color_override("font_color", Color(0.85, 0.78, 0.65))
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(title_label)

	# 선택적 서브타이틀
	if subtitle != "":
		var sub_label = Label.new()
		sub_label.text = subtitle
		sub_label.add_theme_font_size_override("font_size", 14)
		sub_label.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
		sub_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		sub_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
		container.add_child(sub_label)

	# 초기 투명, CanvasLayer에는 modulate가 없으므로 bg+container를 조작
	bg.modulate = Color(1, 1, 1, 0)
	container.modulate = Color(1, 1, 1, 0)
	parent.add_child(layer)

	# 애니메이션: 페이드 인 0.5s → 홀드 2.0s → 페이드 아웃 0.8s → 제거
	var tween = parent.create_tween()
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, 0.5)
	tween.tween_property(container, "modulate:a", 1.0, 0.5)
	if art_bg != null:
		tween.tween_property(art_bg, "modulate:a", 0.30, 0.65)
	if art_panel != null:
		tween.tween_property(art_panel, "modulate:a", 0.72, 0.65)
	tween.set_parallel(false)
	tween.tween_interval(2.0)
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 0.0, 0.8)
	tween.tween_property(container, "modulate:a", 0.0, 0.8)
	if art_bg != null:
		tween.tween_property(art_bg, "modulate:a", 0.0, 0.8)
	if art_panel != null:
		tween.tween_property(art_panel, "modulate:a", 0.0, 0.8)
	tween.set_parallel(false)
	tween.tween_callback(layer.queue_free)

	# await용: 트윈 완료까지 대기 가능
	await tween.finished

	return layer

static func _get_chapter_art_path(chapter_num: int) -> String:
	var art_map := {
		1: "res://assets/cg/generated/chapter_splash_rim_forest.png",
		2: "res://assets/cg/generated/chapter_splash_verdan_market.png",
		3: "res://assets/cg/generated/chapter_splash_belt_waystation.png",
		4: "res://assets/cg/generated/chapter_splash_drift_shelter.png",
		5: "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
		6: "res://assets/cg/generated/chapter_splash_the_seam.png",
		7: "res://assets/cg/generated/chapter_splash_seam_outskirts.png",
		8: "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
		9: "res://assets/cg/generated/memory_compass_resonance_cinematic.png",
		10: "res://assets/cg/generated/chapter_splash_bl07_void.png",
		11: "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_elia_anchor_v3.png",
	}
	return art_map.get(chapter_num, "")

## ===================== 날씨 효과 =====================

## 비 효과 (CanvasLayer 기반)
static func add_rain(parent: Node, intensity: float = 1.0, color: Color = Color(0.6, 0.65, 0.8, 0.3)) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 2)
	var layer = CanvasLayer.new()
	layer.layer = 2
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0.2, 1.0, 0)
	mat.spread = 5.0
	mat.initial_velocity_min = 300.0 * intensity
	mat.initial_velocity_max = 450.0 * intensity
	mat.gravity = Vector3(20, 800 * intensity, 0)
	mat.scale_min = 0.5
	mat.scale_max = 1.5
	mat.color = color
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(700, 10, 0)
	particles.process_material = mat
	particles.amount = int(80 * intensity)
	particles.lifetime = 1.2
	particles.position = Vector2(640, -20)
	particles.visibility_rect = Rect2(-700, -50, 1400, 800)
	layer.add_child(particles)
	parent.add_child(layer)
	return layer

## 눈 효과 (CanvasLayer 기반)
static func add_snow(parent: Node, intensity: float = 1.0) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 2)
	var layer = CanvasLayer.new()
	layer.layer = 2
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 1, 0)
	mat.spread = 30.0
	mat.initial_velocity_min = 20.0 * intensity
	mat.initial_velocity_max = 60.0 * intensity
	mat.gravity = Vector3(5, 30 * intensity, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.0
	mat.color = Color(0.85, 0.88, 0.95, 0.5)
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(700, 10, 0)
	particles.process_material = mat
	particles.amount = int(40 * intensity)
	particles.lifetime = 6.0
	particles.position = Vector2(640, -30)
	particles.visibility_rect = Rect2(-700, -50, 1400, 800)
	layer.add_child(particles)
	parent.add_child(layer)
	return layer

## 짙은 안개 (CanvasLayer 기반, 동적 불투명도)
static func add_heavy_fog(parent: Node, color: Color = Color(0.3, 0.3, 0.35, 0.12)) -> Array[ColorRect]:
	if _clean_gameplay_view():
		return []
	var layer = CanvasLayer.new()
	layer.layer = 2
	var fogs: Array[ColorRect] = []
	for i in range(5):
		var fog = ColorRect.new()
		fog.size = Vector2(randi_range(400, 700), randi_range(150, 300))
		fog.position = Vector2(randf_range(-200, 1000), randf_range(50, 600))
		fog.color = color
		fog.mouse_filter = Control.MOUSE_FILTER_IGNORE
		fog.set_meta("speed_x", randf_range(2.0, 6.0))
		fog.set_meta("speed_y", randf_range(-1.0, 1.0))
		fog.set_meta("phase", randf() * TAU)
		layer.add_child(fog)
		fogs.append(fog)
	parent.add_child(layer)
	return fogs

## 짙은 안개 업데이트
static func update_heavy_fog(fogs: Array[ColorRect], time: float) -> void:
	for fog in fogs:
		if is_instance_valid(fog):
			var sx = fog.get_meta("speed_x", 4.0)
			var sy = fog.get_meta("speed_y", 0.0)
			var phase = fog.get_meta("phase", 0.0)
			fog.position.x += sx * 0.016
			fog.position.y += sin(time * 0.3 + phase) * sy * 0.016
			fog.color.a = 0.08 + sin(time * 0.4 + phase) * 0.04
			if fog.position.x > 1500:
				fog.position.x = -fog.size.x

## 안개 업데이트 (_process에서 호출)
static func update_fog(fogs: Array[ColorRect], time: float) -> void:
	for fog in fogs:
		if is_instance_valid(fog):
			var speed = fog.get_meta("speed_x", 5.0)
			var phase = fog.get_meta("phase", 0.0)
			fog.position.x += speed * 0.016  # ~60fps
			fog.color.a = 0.05 + sin(time * 0.5 + phase) * 0.03
			if fog.position.x > 1400:
				fog.position.x = -fog.size.x

## ===================== S43: 애니메이션 타일 효과 =====================

## 풀 타일에 흔들리는 풀잎 오버레이 추가
static func add_grass_sway(parent: Node2D, map_data: Array, width: int, height: int, grass_index: int) -> Array[ColorRect]:
	var blades: Array[ColorRect] = []
	if _clean_gameplay_view():
		return blades
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == grass_index and randi() % 4 == 0:
					var blade = ColorRect.new()
					blade.size = Vector2(2, randi_range(4, 7))
					blade.position = Vector2(x * TILE + randi_range(4, 28), y * TILE + randi_range(10, 24))
					blade.color = Color(0.2, 0.42, 0.18, 0.5)
					blade.z_index = 0
					blade.mouse_filter = Control.MOUSE_FILTER_IGNORE
					blade.pivot_offset = Vector2(1, blade.size.y)
					blade.set_meta("phase", randf() * TAU)
					blade.set_meta("speed", randf_range(1.5, 3.0))
					parent.add_child(blade)
					blades.append(blade)
	return blades

## 풀 흔들림 업데이트 (_process에서 호출)
static func update_grass_sway(blades: Array[ColorRect], time: float) -> void:
	for blade in blades:
		if is_instance_valid(blade):
			var phase = blade.get_meta("phase", 0.0)
			var speed = blade.get_meta("speed", 2.0)
			blade.rotation = sin(time * speed + phase) * 0.15

## 횃불/랜턴에 불꽃 파티클 추가
static func add_fire_particles(parent: Node2D, map_data: Array, width: int, height: int, lantern_index: int) -> Array[GPUParticles2D]:
	var fires: Array[GPUParticles2D] = []
	if _clean_gameplay_view():
		return fires
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == lantern_index:
					var particles = GPUParticles2D.new()
					var mat = ParticleProcessMaterial.new()
					mat.direction = Vector3(0, -1, 0)
					mat.spread = 15.0
					mat.initial_velocity_min = 8.0
					mat.initial_velocity_max = 20.0
					mat.gravity = Vector3(0, -15, 0)
					mat.scale_min = 0.5
					mat.scale_max = 1.5
					var gradient = GradientTexture1D.new()
					var g = Gradient.new()
					g.set_color(0, Color(1, 0.8, 0.3, 0.8))
					g.add_point(0.4, Color(1, 0.5, 0.1, 0.6))
					g.set_color(1, Color(0.5, 0.2, 0.05, 0.0))
					gradient.gradient = g
					mat.color_ramp = gradient
					mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
					mat.emission_sphere_radius = 3.0
					particles.process_material = mat
					particles.amount = 6
					particles.lifetime = 0.8
					particles.position = Vector2(x * TILE + TILE / 2.0, y * TILE + 6)
					particles.z_index = 2
					particles.visibility_rect = Rect2(-20, -30, 40, 40)
					parent.add_child(particles)
					fires.append(particles)
	return fires

## ===================== S42: 2D 조명 시스템 =====================

## 맵에 환경 조명 추가 (CanvasModulate로 전체 어둡게 + PointLight2D)
static func add_ambient_lighting(parent: Node2D, ambient_color: Color = Color(0.6, 0.6, 0.7, 1.0)) -> CanvasModulate:
	var modulate = CanvasModulate.new()
	modulate.color = Color.WHITE if _clean_gameplay_view() else ambient_color
	parent.add_child(modulate)
	return modulate

## 포인트 라이트 생성 (횃불, 랜턴, 빛나는 오브젝트)
static func add_point_light(parent: Node2D, pos: Vector2, color: Color = Color(1.0, 0.85, 0.5), energy: float = 1.0, radius: float = 128.0, shadow: bool = false) -> PointLight2D:
	var light = PointLight2D.new()
	light.position = pos
	light.color = color
	light.energy = energy
	light.texture = _create_light_texture(int(radius))
	light.texture_scale = 1.0
	light.shadow_enabled = shadow
	light.blend_mode = Light2D.BLEND_MODE_ADD
	# Canvas maps carry their own baked, controlled lantern values. Runtime
	# additive lights otherwise read as square patches over that pixel art.
	if parent.get_node_or_null("EnvironmentCanvas") != null:
		light.visible = false
	parent.add_child(light)
	return light

## 맵 타일 기반 자동 라이트 배치 (랜턴 위치에 PointLight2D)
static func add_tile_lights(parent: Node2D, map_data: Array, width: int, height: int, lantern_index: int, light_color: Color = Color(1.0, 0.85, 0.5)) -> Array[PointLight2D]:
	var lights: Array[PointLight2D] = []
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == lantern_index:
					var pos = Vector2(x * TILE + TILE / 2.0, y * TILE + TILE / 2.0)
					var light = add_point_light(parent, pos, light_color, 0.8, 96.0)
					light.set_meta("phase", randf() * TAU)
					lights.append(light)
	return lights

## 라이트 플리커 업데이트 (_process에서 호출)
static func update_point_lights(lights: Array[PointLight2D], time: float) -> void:
	for light in lights:
		if is_instance_valid(light):
			var phase = light.get_meta("phase", 0.0)
			light.energy = 0.7 + sin(time * 3.0 + phase) * 0.15 + sin(time * 7.5 + phase * 2.0) * 0.08

## 원형 라이트 텍스처 프로시저럴 생성
static func _create_light_texture(radius: int) -> ImageTexture:
	var size = radius * 2
	var img = Image.create(size, size, false, Image.FORMAT_RGBA8)
	var center = Vector2(radius, radius)
	for y in range(size):
		for x in range(size):
			var dist = Vector2(x, y).distance_to(center) / radius
			var alpha = clampf(1.0 - dist * dist, 0.0, 1.0)  # 부드러운 감쇠
			img.set_pixel(x, y, Color(1, 1, 1, alpha))
	return ImageTexture.create_from_image(img)

## ===================== S42: 패럴랙스 배경 =====================

## 맵에 패럴랙스 배경 추가 (깊이감 레이어)
static func add_parallax_background(parent: Node2D, config: Dictionary = {}) -> ParallaxBackground:
	var bg = ParallaxBackground.new()
	bg.scroll_ignore_camera_zoom = true
	parent.add_child(bg)

	var sky_color = config.get("sky", Color(0.08, 0.1, 0.15))
	var far_color = config.get("far", Color(0.12, 0.15, 0.2))
	var mid_color = config.get("mid", Color(0.15, 0.18, 0.14))
	var map_w = config.get("width", 800)
	var map_h = config.get("height", 576)

	# Layer 0: 하늘 (고정)
	var sky_layer = ParallaxLayer.new()
	sky_layer.motion_scale = Vector2.ZERO  # 고정
	bg.add_child(sky_layer)
	var sky_rect = ColorRect.new()
	sky_rect.size = Vector2(map_w + 400, map_h + 200)
	sky_rect.position = Vector2(-200, -100)
	sky_rect.color = sky_color
	sky_rect.z_index = -30
	sky_layer.add_child(sky_rect)
	# 하늘 그라디언트, 위는 밝게, 아래는 어둡게
	for i in range(6):
		var grad = ColorRect.new()
		grad.size = Vector2(map_w + 400, 30)
		grad.position = Vector2(-200, -100 + i * 30)
		grad.color = Color(sky_color.r + 0.02 * (6 - i), sky_color.g + 0.02 * (6 - i), sky_color.b + 0.03 * (6 - i), 0.3)
		grad.z_index = -29
		grad.mouse_filter = Control.MOUSE_FILTER_IGNORE
		sky_layer.add_child(grad)

	# Layer 1: 먼 산/건물 실루엣 (느린 스크롤)
	var far_layer = ParallaxLayer.new()
	far_layer.motion_scale = Vector2(0.15, 0.1)
	bg.add_child(far_layer)
	_add_silhouette_mountains(far_layer, far_color, map_w, map_h)

	# Layer 2: 중간 나무/구조물 (중간 스크롤)
	var mid_layer = ParallaxLayer.new()
	mid_layer.motion_scale = Vector2(0.35, 0.2)
	bg.add_child(mid_layer)
	_add_midground_elements(mid_layer, mid_color, map_w, map_h, config.get("biome", "forest"))

	return bg

## 먼 산/실루엣 레이어 생성
static func _add_silhouette_mountains(layer: ParallaxLayer, color: Color, w: int, h: int) -> void:
	# 3~5개 산 봉우리
	var num_peaks = randi_range(3, 5)
	for i in range(num_peaks):
		var peak_w = randi_range(150, 300)
		var peak_h = randi_range(60, 140)
		var peak_x = int(float(i) / num_peaks * (w + 200)) - 100 + randi_range(-40, 40)
		var peak_y = h - 200 - peak_h + randi_range(0, 40)

		# 삼각형을 ColorRect 스택으로 표현
		for row in range(peak_h):
			var ratio = 1.0 - float(row) / peak_h
			var row_w = int(peak_w * ratio)
			if row_w < 2:
				continue
			var rect = ColorRect.new()
			rect.size = Vector2(row_w, 2)
			rect.position = Vector2(peak_x + int((peak_w - row_w) / 2.0), peak_y + row)
			# 높이별 색상 변화
			var bright = 0.02 * (1.0 - ratio)
			rect.color = Color(color.r + bright, color.g + bright, color.b + bright + 0.01, 0.7)
			rect.z_index = -25
			rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
			layer.add_child(rect)

## 중간 레이어 요소 (바이옴별)
static func _add_midground_elements(layer: ParallaxLayer, color: Color, w: int, h: int, biome: String) -> void:
	var num_elements = randi_range(5, 8)
	for i in range(num_elements):
		var ex = int(float(i) / num_elements * (w + 100)) - 50 + randi_range(-30, 30)
		var ey = h - 180 + randi_range(-20, 40)

		match biome:
			"forest":
				_add_tree_silhouette(layer, Vector2(ex, ey), color)
			"coast":
				_add_rock_silhouette(layer, Vector2(ex, ey), color)
			"market":
				_add_building_silhouette(layer, Vector2(ex, ey), color)
			"void":
				_add_crystal_silhouette(layer, Vector2(ex, ey), color)
			_:
				_add_tree_silhouette(layer, Vector2(ex, ey), color)

## 나무 실루엣
static func _add_tree_silhouette(layer: ParallaxLayer, pos: Vector2, color: Color) -> void:
	# 줄기
	var trunk = ColorRect.new()
	trunk.size = Vector2(4, randi_range(25, 45))
	trunk.position = pos
	trunk.color = _darken_c(color, 0.05)
	trunk.z_index = -20
	trunk.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(trunk)
	# 수관 (원형 근사)
	var canopy_r = randi_range(12, 24)
	for dy in range(-canopy_r, canopy_r + 1, 3):
		var half_w = int(sqrt(maxf(canopy_r * canopy_r - dy * dy, 0)))
		if half_w < 2:
			continue
		var rect = ColorRect.new()
		rect.size = Vector2(half_w * 2, 3)
		rect.position = Vector2(pos.x + 2 - half_w, pos.y - canopy_r + dy)
		rect.color = Color(color.r, color.g + 0.02, color.b, 0.6 + randf() * 0.15)
		rect.z_index = -20
		rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(rect)

## 바위 실루엣
static func _add_rock_silhouette(layer: ParallaxLayer, pos: Vector2, color: Color) -> void:
	var rw = randi_range(20, 50)
	var rh = randi_range(15, 35)
	for row in range(rh):
		var ratio = 1.0 - float(row) / rh
		var row_w = int(rw * (0.5 + 0.5 * ratio))
		var rect = ColorRect.new()
		rect.size = Vector2(row_w, 2)
		rect.position = Vector2(pos.x + int((rw - row_w) / 2.0), pos.y + row)
		rect.color = Color(color.r + 0.01, color.g, color.b, 0.5)
		rect.z_index = -20
		rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(rect)

## 건물 실루엣
static func _add_building_silhouette(layer: ParallaxLayer, pos: Vector2, color: Color) -> void:
	var bw = randi_range(25, 50)
	var bh = randi_range(30, 60)
	var rect = ColorRect.new()
	rect.size = Vector2(bw, bh)
	rect.position = pos
	rect.color = Color(color.r, color.g, color.b, 0.5)
	rect.z_index = -20
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(rect)
	# 창문
	for wy in range(2):
		for wx in range(2):
			var win = ColorRect.new()
			win.size = Vector2(4, 5)
			win.position = Vector2(pos.x + 6 + wx * (bw - 16), pos.y + 8 + wy * 18)
			win.color = Color(0.6, 0.55, 0.3, 0.3)
			win.z_index = -19
			win.mouse_filter = Control.MOUSE_FILTER_IGNORE
			layer.add_child(win)

## 크리스탈 실루엣 (보이드)
static func _add_crystal_silhouette(layer: ParallaxLayer, pos: Vector2, color: Color) -> void:
	var ch = randi_range(30, 60)
	var cw = randi_range(8, 16)
	for row in range(ch):
		var ratio = 1.0 - abs(2.0 * row / ch - 1.0)
		var row_w = int(cw * ratio) + 2
		var rect = ColorRect.new()
		rect.size = Vector2(row_w, 2)
		rect.position = Vector2(pos.x + int((cw - row_w) / 2.0), pos.y + row)
		rect.color = Color(color.r + 0.05 * ratio, color.g, color.b + 0.1 * ratio, 0.4 + 0.2 * ratio)
		rect.z_index = -20
		rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
		layer.add_child(rect)

## ===================== S46: 맵 비주얼 강화 =====================

## 반딧불 파티클 (숲/Seam 맵용)
static func add_fireflies(parent: Node, count: int = 15, color: Color = Color(0.6, 0.9, 0.4, 0.6)) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 2)
	var layer = CanvasLayer.new()
	layer.layer = 2

	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 0, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 3.0
	mat.initial_velocity_max = 12.0
	mat.gravity = Vector3(0, -2, 0)
	mat.scale_min = 1.0
	mat.scale_max = 2.5

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(color.r, color.g, color.b, 0.0))
	g.add_point(0.2, color)
	g.add_point(0.5, Color(color.r, color.g, color.b, color.a * 0.7))
	g.add_point(0.8, color)
	g.set_color(1, Color(color.r, color.g, color.b, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(600, 350, 0)

	particles.process_material = mat
	particles.amount = count
	particles.lifetime = 4.0
	particles.position = Vector2(640, 360)
	particles.visibility_rect = Rect2(-700, -400, 1400, 800)
	layer.add_child(particles)
	parent.add_child(layer)
	return layer

## 대기 열 왜곡 (해안/시장 맵용, BackBufferCopy + 셰이더)
static func add_heat_haze(parent: Node, strength: float = 0.003) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 1)
	var shader_path = "res://assets/shaders/heat_haze.gdshader"
	var shader_res = _get_shader(shader_path)
	if shader_res == null:
		return null

	var layer = CanvasLayer.new()
	layer.layer = 1  # 가장 낮은 레이어

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var mat = ShaderMaterial.new()
	mat.shader = shader_res
	mat.set_shader_parameter("distortion_strength", strength)
	mat.set_shader_parameter("wave_speed", 1.5)
	mat.set_shader_parameter("wave_frequency", 8.0)
	rect.material = mat

	layer.add_child(rect)
	parent.add_child(layer)
	return layer

## 동적 라이트 펄스, CanvasModulate 색상을 시간에 따라 미세 변화
static func update_ambient_pulse(modulate: CanvasModulate, base_color: Color, time: float, intensity: float = 0.03) -> void:
	if not is_instance_valid(modulate):
		return
	var pulse = sin(time * 0.3) * intensity
	modulate.color = Color(
		base_color.r + pulse,
		base_color.g + pulse * 0.8,
		base_color.b + pulse * 0.5,
		base_color.a
	)

## ===================== S46: 기억 연소 월드 반응 =====================

## 기억 연소 비율에 따라 화면 채도를 낮추는 오버레이 (CanvasLayer)
## burn_count가 높을수록 세계가 회색으로 빠져감 + 보이드 보라 틴트
static func add_burn_desaturation(parent: Node) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 3)
	var burn_count = MemoryManager.get_burn_count()
	if burn_count <= 0:
		return null  # 연소 없으면 효과 없음

	var shader_path = "res://assets/shaders/desaturation.gdshader"
	var shader_res = _get_shader(shader_path)
	if shader_res == null:
		return null

	var layer = CanvasLayer.new()
	layer.layer = 3  # 비네트와 동일 레이어

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rect.color = Color(1, 1, 1, 1)

	# 연소량 → 채도 감소 (0~8+ 연소: 0%~40% 탈색)
	var desat = clampf(burn_count * 0.05, 0.0, 0.4)
	# 5회 이상 연소 시 보이드 보라 틴트 시작
	var tint = clampf((burn_count - 4) * 0.03, 0.0, 0.15)

	var mat = ShaderMaterial.new()
	mat.shader = shader_res
	mat.set_shader_parameter("desaturation", desat)
	mat.set_shader_parameter("tint_color", Color(0.15, 0.12, 0.18, 1.0))
	mat.set_shader_parameter("tint_strength", tint)
	rect.material = mat

	layer.add_child(rect)
	parent.add_child(layer)
	return layer

## ===================== S52: 2D 그림자 시스템 =====================
## PointLight2D에 그림자 활성화 + 벽/나무 타일에 오클루더 자동 추가
static func enable_shadows_on_lights(lights: Array) -> void:
	for light in lights:
		if light is PointLight2D:
			light.shadow_enabled = true
			light.shadow_color = Color(0.0, 0.0, 0.05, 0.7)
			light.shadow_filter = PointLight2D.SHADOW_FILTER_PCF5
			light.shadow_filter_smooth = 1.5

## 벽/나무 타일에 LightOccluder2D 추가 (그림자 드리움)
static func add_tile_occluders(parent: Node2D, map_data: Array, width: int, height: int, wall_indices: Array) -> Array[LightOccluder2D]:
	var occluders: Array[LightOccluder2D] = []
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] in wall_indices:
					var occ = LightOccluder2D.new()
					occ.position = Vector2(x * TILE + TILE / 2.0, y * TILE + TILE / 2.0)
					var poly = OccluderPolygon2D.new()
					var half = TILE / 2.0 - 1
					poly.polygon = PackedVector2Array([
						Vector2(-half, -half), Vector2(half, -half),
						Vector2(half, half), Vector2(-half, half)
					])
					poly.cull_mode = OccluderPolygon2D.CULL_DISABLED
					occ.occluder = poly
					parent.add_child(occ)
					occluders.append(occ)
	return occluders

## ===================== S52: 컬러 그레이딩 포스트프로세스 =====================
## 맵별 분위기 색조 보정 (셰이더 기반)
static func add_color_grading(parent: Node2D, settings: Dictionary) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 4)
	var layer = CanvasLayer.new()
	layer.layer = 4  # 비네트(3) 위

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE

	# 순수 컬러 오버레이 방식 (셰이더 불필요)
	var tint: Color = settings.get("tint", Color(1, 1, 1, 0))
	var brightness: float = settings.get("brightness", 0.0)
	var contrast: float = settings.get("contrast", 1.0)

	# tint 블렌드, 투명 오버레이
	rect.color = Color(tint.r, tint.g, tint.b, tint.a * 0.15)

	# contrast/brightness, CanvasItem modulate
	if brightness != 0.0:
		var b = 1.0 + brightness
		rect.modulate = Color(b, b, b, 1.0)

	layer.add_child(rect)
	parent.add_child(layer)
	return layer

## ===================== S52: 캐릭터 드롭 섀도우 =====================
## 플레이어/NPC 발밑에 타원형 그림자 추가
## Premium screen lens: subtle paper grain, cinematic edges, and faint light shafts.
## This is screen-space so every map can gain a stronger authored look without
## rewriting tile or prop construction.
## ===================== S236: 대기 예산 (Atmosphere Budget) =====================
##
## 맵마다 여섯 겹의 전면 오버레이(비네트, 컬러 그레이딩, 삽화 대기, 안개, 깊이
## 그라디언트, 프리미엄 렌즈)를 각자 손으로 정한 숫자로 쌓고 있었다. 1장 맵인
## rim_forest만 공들여 낮게 조정돼 있었고, 나머지는 무거운 기본값을 그대로 들고 갔다.
##
## 전 맵을 같은 조건으로 렌더해 재 보니 rim_forest 대비 대비도가 43~53%,
## bl07_void는 34%였다. 의뢰해서 그린 맵 캔버스가 단색 안개에 덮여 사라지고
## 캐릭터도 어두운 점이 되어 있었다.
##
## 여기서 예산을 한 번만 정한다. 맵은 "이 장소가 어떤 곳인가"만 선언하고,
## 각 레이어에 얼마를 쓸지는 이 함수가 rim_forest의 검증된 비율에서 파생한다.
## mood를 올려도 상한이 있으므로, 가장 짓눌린 맵조차 예전 rim_forest의 두 배를
## 넘지 않는다. 분위기는 색으로 말하고, 읽기는 포기하지 않는다.

## 핵심 규칙: **밝기는 곱셈에서, 분위기는 겹칠에서.**
##
## premium_lens 셰이더는 tint_color를 단색으로 칠하고, 그 알파를
## vignette + letterbox + tint_strength + shaft 의 합으로 정한다.
## 실측해 보니 verdan_market은 그 합이 0.635였다. 화면의 63%가 단색 한 겹이었고,
## 그것이 곧 이 맵들이 의존하던 광원이기도 했다. 단색을 덮으면 밝아지지만
## 그만큼 명암 차이가 지워진다. rim_forest만 그 합이 0.303이라 유일하게 읽혔다.
##
## 그래서 밝기는 CanvasModulate(장면 전체 곱셈, 대비 보존)로 옮기고
## 겹칠은 분위기가 필요한 만큼만 남긴다.
const ATMOSPHERE_BASE := {
	"vignette": 0.12,
	"fog_alpha": 0.016,
	"fog_density": 0.16,
	"depth": 0.035,
	"splash": 0.08,
	"lens_vignette": 0.10,
	"lens_tint": 0.020,
	"lens_shafts": 0.030,
	"lens_letterbox": 0.06,
	"ambient": 1.00,
}
## mood 1.0(가장 짓눌린 곳)에서의 상한. 겹칠 총량은 여기서도 0.33을 넘지 않는다.
const ATMOSPHERE_HEAVY := {
	"vignette": 0.22,
	"fog_alpha": 0.038,
	"fog_density": 0.55,
	"depth": 0.060,
	"splash": 0.10,
	"lens_vignette": 0.18,
	"lens_tint": 0.040,
	"lens_shafts": 0.050,
	"lens_letterbox": 0.06,
	"ambient": 0.82,
}

static func _atmosphere_value(key: String, mood: float) -> float:
	return lerpf(float(ATMOSPHERE_BASE[key]), float(ATMOSPHERE_HEAVY[key]), clampf(mood, 0.0, 1.0))

## 맵 하나의 대기를 통째로 세운다.
##
## identity:
##   hue        : 이 장소의 바탕색. 그레이딩과 안개가 여기서 파생한다.
##   light      : 이 장소에 드는 빛의 색. 바탕색과 다른 것이 정상이다
##                (보라색 방에 드는 호박색 광선처럼). 예산은 "얼마나"만 정하고
##                "무슨 색"은 맵이 정한다.
##   mood       : 0.0 열린 곳 .. 1.0 짓눌린 곳. 오버레이 총량을 정한다.
##   saturation : 1.0 정상 .. 0.0 무채. colorless_waste 같은 곳의 정체성.
##   brightness : 그레이딩 밝기 보정 (기본 0.0)
##   splash     : 삽화 대기 텍스처 경로 (없으면 생략)
##   fog        : "none"(기본) | "soft" | "heavy". 원래 쓰던 맵에만 준다.
static func apply_atmosphere(map: Node2D, identity: Dictionary) -> void:
	if map == null:
		return
	var hue: Color = identity.get("hue", Color(0.4, 0.45, 0.4))
	var mood: float = clampf(float(identity.get("mood", 0.4)), 0.0, 1.0)
	var saturation: float = clampf(float(identity.get("saturation", 1.0)), 0.0, 1.0)
	var brightness: float = float(identity.get("brightness", 0.0))
	var fog_mode: String = String(identity.get("fog", "none"))
	var light: Color = identity.get("light", Color(hue.r * 1.7, hue.g * 1.7, hue.b * 1.7, 1.0))

	# 그레이딩 색은 채도 선언을 반영한다. 무채 지대는 색조차 회색으로 수렴한다.
	var grey := (hue.r + hue.g + hue.b) / 3.0
	var graded := Color(
		lerpf(grey, hue.r, saturation),
		lerpf(grey, hue.g, saturation),
		lerpf(grey, hue.b, saturation),
		1.0
	)
	var fog_color := Color(graded.r, graded.g, graded.b, _atmosphere_value("fog_alpha", mood))

	# 밝기는 여기서 온다. 곱셈이라 명암 관계가 보존된다.
	# 색조는 빛의 색 쪽으로 살짝만 기울인다. 전부 기울이면 화면이 단색이 된다.
	var light_peak := maxf(maxf(light.r, light.g), maxf(light.b, 0.001))
	var ambient_strength := _atmosphere_value("ambient", mood)
	add_ambient_lighting(map, Color(
		lerpf(1.0, light.r / light_peak, 0.35) * ambient_strength,
		lerpf(1.0, light.g / light_peak, 0.35) * ambient_strength,
		lerpf(1.0, light.b / light_peak, 0.35) * ambient_strength
	))
	add_vignette(map, _atmosphere_value("vignette", mood))
	add_color_grading(map, {"tint": graded, "brightness": brightness})
	var splash := String(identity.get("splash", ""))
	if splash != "" and ResourceLoader.exists(splash):
		# 삽화 대기의 틴트는 색을 덮으라고 있는 것이 아니라 붙잡으라고 있다.
		# 원본 맵들의 틴트는 0.7~1.0대의 밝은 값이었다. 빛의 색조만 빌려 오고 밝기는 지킨다.
		var splash_peak := maxf(maxf(light.r, light.g), maxf(light.b, 0.001))
		var splash_tint := Color(
			lerpf(1.0, light.r / splash_peak, 0.34),
			lerpf(1.0, light.g / splash_peak, 0.34),
			lerpf(1.0, light.b / splash_peak, 0.34),
			1.0
		)
		add_illustration_atmosphere(map, splash, _atmosphere_value("splash", mood), splash_tint)
	match fog_mode:
		"heavy":
			pass  # 맵이 매 프레임 갱신하므로 apply_atmosphere_heavy_fog로 따로 세운다
		"soft":
			add_fog(map, fog_color)
		_:
			pass
	add_depth_gradient(map, _atmosphere_value("depth", mood))
	add_premium_map_lens(map, {
		"tint": light,
		"vignette": _atmosphere_value("lens_vignette", mood),
		"tint_strength": _atmosphere_value("lens_tint", mood),
		"shafts": _atmosphere_value("lens_shafts", mood),
		"glints": 1,
		"grain": 0.0,
		"letterbox": _atmosphere_value("lens_letterbox", mood),
	})

## 짙은 안개도 맵이 매 프레임 갱신하므로 참조를 돌려준다.
static func apply_atmosphere_heavy_fog(map: Node2D, identity: Dictionary) -> Array[ColorRect]:
	var hue: Color = identity.get("hue", Color(0.4, 0.45, 0.4))
	var mood: float = clampf(float(identity.get("mood", 0.4)), 0.0, 1.0)
	return add_heavy_fog(map, Color(hue.r, hue.g, hue.b, _atmosphere_value("fog_alpha", mood) * 1.4))

## 흐르는 안개 띠는 맵이 참조를 들고 있어야 하므로 따로 세운다.
static func apply_atmosphere_fog_layer(map: Node2D, identity: Dictionary) -> Array[ColorRect]:
	var hue: Color = identity.get("hue", Color(0.4, 0.45, 0.4))
	var mood: float = clampf(float(identity.get("mood", 0.4)), 0.0, 1.0)
	var saturation: float = clampf(float(identity.get("saturation", 1.0)), 0.0, 1.0)
	var grey := (hue.r + hue.g + hue.b) / 3.0
	var color := Color(
		lerpf(grey, hue.r, saturation),
		lerpf(grey, hue.g, saturation),
		lerpf(grey, hue.b, saturation),
		_atmosphere_value("fog_alpha", mood)
	)
	return add_fog_layer(map, _atmosphere_value("fog_density", mood), color, 2.0)

static func add_premium_map_lens(parent: Node2D, settings: Dictionary = {}) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, int(settings.get("layer", 5)))
	var layer = CanvasLayer.new()
	layer.layer = int(settings.get("layer", 5))

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rect.color = Color(0, 0, 0, 0)

	var shader_path = "res://assets/shaders/premium_lens.gdshader"
	var shader_res = _get_shader(shader_path)
	if shader_res:
		var mat = ShaderMaterial.new()
		mat.shader = shader_res
		mat.set_shader_parameter("tint_color", settings.get("tint", Color(0.72, 0.58, 0.36, 1.0)))
		mat.set_shader_parameter("vignette_strength", float(settings.get("vignette", 0.42)))
		mat.set_shader_parameter("tint_strength", float(settings.get("tint_strength", 0.09)))
		# Grain is opt-in. On low-resolution top-down maps it reads as crawling
		# compression noise and obscures the pixel silhouettes during movement.
		mat.set_shader_parameter("grain_strength", float(settings.get("grain", 0.0)))
		mat.set_shader_parameter("letterbox_strength", float(settings.get("letterbox", 0.18)))
		mat.set_shader_parameter("shaft_strength", float(settings.get("shafts", 0.07)))
		mat.set_shader_parameter("pulse_speed", float(settings.get("pulse", 0.35)))
		rect.material = mat
	else:
		var tint: Color = settings.get("tint", Color(0.72, 0.58, 0.36, 1.0))
		rect.color = Color(tint.r, tint.g, tint.b, float(settings.get("fallback_alpha", 0.12)))

	layer.add_child(rect)

	var glint_count: int = int(settings.get("glints", 2))
	for i in range(glint_count):
		var glint = TextureRect.new()
		glint.anchor_left = randf_range(-0.08, 0.72)
		glint.anchor_right = glint.anchor_left + randf_range(0.18, 0.34)
		glint.anchor_top = randf_range(-0.10, 0.32)
		glint.anchor_bottom = glint.anchor_top + randf_range(0.22, 0.42)
		glint.mouse_filter = Control.MOUSE_FILTER_IGNORE
		glint.modulate = Color(1.0, 0.86, 0.55, randf_range(0.045, 0.085))
		glint.rotation = randf_range(-0.35, -0.16)
		glint.texture = _make_linear_gradient_texture(
			Color(1.0, 0.82, 0.45, 0.0),
			Color(1.0, 0.88, 0.58, 0.42),
			Color(1.0, 0.82, 0.45, 0.0)
		)
		layer.add_child(glint)

	parent.add_child(layer)
	return layer

static func _make_linear_gradient_texture(left: Color, mid: Color, right: Color) -> GradientTexture2D:
	var grad = Gradient.new()
	grad.set_color(0, left)
	grad.add_point(0.5, mid)
	grad.set_color(1, right)
	var tex = GradientTexture2D.new()
	tex.gradient = grad
	tex.width = 256
	tex.height = 64
	tex.fill = GradientTexture2D.FILL_LINEAR
	tex.fill_from = Vector2(0.0, 0.5)
	tex.fill_to = Vector2(1.0, 0.5)
	return tex

static func add_drop_shadow(
		character: Node2D,
		accent: Color = Color(0.38, 0.56, 0.82, 1.0)
	) -> Node2D:
	# S213: one grounding contract for player, companions, story NPCs and
	# ambient walkers.  The old call sites keep working while the shared helper
	# supplies a two-layer contact shadow and a restrained Memory-color edge.
	return FieldActorVisuals.add_grounding(character, accent)

## ===================== S52: 향상된 바이옴 파티클 =====================

## 꽃가루/포자 파티클 (숲 맵)
static func add_pollen_particles(parent: Node2D, count: int = 15, area: Vector2 = Vector2(800, 600), color: Color = Color(0.8, 0.85, 0.5, 0.3)) -> Array[ColorRect]:
	var particles: Array[ColorRect] = []
	if _clean_gameplay_view():
		return particles
	for i in range(count):
		var p = ColorRect.new()
		p.size = Vector2(randf_range(2, 4), randf_range(2, 4))
		p.color = Color(color.r + randf_range(-0.1, 0.1), color.g + randf_range(-0.1, 0.1), color.b, color.a)
		p.position = Vector2(randf_range(0, area.x), randf_range(0, area.y))
		p.set_meta("drift_x", randf_range(-8, 8))
		p.set_meta("drift_y", randf_range(-12, -3))
		p.set_meta("wave_phase", randf() * TAU)
		p.set_meta("wave_amp", randf_range(15, 40))
		p.set_meta("base_x", p.position.x)
		p.set_meta("area", area)
		p.z_index = 5
		parent.add_child(p)
		particles.append(p)
	return particles

## 꽃가루 업데이트 (사인파 흔들림 + 느린 부유)
static func update_pollen(particles: Array, time: float, delta: float) -> void:
	for p in particles:
		if p == null or not is_instance_valid(p):
			continue
		var area: Vector2 = p.get_meta("area", Vector2(800, 600))
		var phase: float = p.get_meta("wave_phase", 0.0)
		var amp: float = p.get_meta("wave_amp", 20.0)
		var base_x: float = p.get_meta("base_x", p.position.x)

		p.position.y += p.get_meta("drift_y", -5.0) * delta
		base_x += p.get_meta("drift_x", 0.0) * delta
		p.position.x = base_x + sin(time * 0.8 + phase) * amp
		p.color.a = 0.2 + sin(time * 1.2 + phase) * 0.15

		p.set_meta("base_x", base_x)
		# 화면 밖 → 리셋
		if p.position.y < -20:
			p.position.y = area.y + 10
			base_x = randf_range(0, area.x)
			p.set_meta("base_x", base_x)

## 재/잿가루 파티클 (황무지/보이드 맵)
static func add_ash_particles(parent: Node2D, count: int = 20, area: Vector2 = Vector2(800, 600), color: Color = Color(0.4, 0.35, 0.3, 0.25)) -> Array[ColorRect]:
	var particles: Array[ColorRect] = []
	if _clean_gameplay_view():
		return particles
	for i in range(count):
		var p = ColorRect.new()
		var sz = randf_range(1.5, 3.5)
		p.size = Vector2(sz, sz)
		p.color = Color(color.r + randf_range(-0.05, 0.05), color.g + randf_range(-0.05, 0.05), color.b + randf_range(-0.05, 0.05), color.a)
		p.position = Vector2(randf_range(0, area.x), randf_range(0, area.y))
		p.set_meta("fall_speed", randf_range(8, 22))
		p.set_meta("sway_phase", randf() * TAU)
		p.set_meta("sway_amp", randf_range(20, 50))
		p.set_meta("base_x", p.position.x)
		p.set_meta("area", area)
		p.rotation = randf() * TAU
		p.z_index = 5
		parent.add_child(p)
		particles.append(p)
	return particles

## 재 업데이트 (느리게 떨어지면서 좌우 흔들림)
static func update_ash(particles: Array, time: float, delta: float) -> void:
	for p in particles:
		if p == null or not is_instance_valid(p):
			continue
		var area: Vector2 = p.get_meta("area", Vector2(800, 600))
		var phase: float = p.get_meta("sway_phase", 0.0)
		var amp: float = p.get_meta("sway_amp", 30.0)
		var base_x: float = p.get_meta("base_x", p.position.x)

		p.position.y += p.get_meta("fall_speed", 15.0) * delta
		p.position.x = base_x + sin(time * 0.6 + phase) * amp
		p.rotation += delta * 0.3
		p.color.a = 0.15 + sin(time + phase) * 0.1

		if p.position.y > area.y + 20:
			p.position.y = -10
			base_x = randf_range(0, area.x)
			p.set_meta("base_x", base_x)

## 보이드 촉수/와이프 파티클 (보이드 맵 전용)
static func add_void_tendrils(parent: Node2D, count: int = 6, area: Vector2 = Vector2(800, 600)) -> Array[ColorRect]:
	var tendrils: Array[ColorRect] = []
	if _clean_gameplay_view():
		return tendrils
	for i in range(count):
		var t = ColorRect.new()
		t.size = Vector2(randf_range(2, 4), randf_range(40, 100))
		t.color = Color(0.15, 0.08, 0.25, 0.12)
		t.position = Vector2(randf_range(50, area.x - 50), area.y)
		t.set_meta("base_y", t.position.y)
		t.set_meta("phase", randf() * TAU)
		t.set_meta("speed", randf_range(0.3, 0.8))
		t.set_meta("reach", randf_range(30, 80))
		t.z_index = 1
		t.pivot_offset = Vector2(t.size.x / 2, t.size.y)
		parent.add_child(t)
		tendrils.append(t)
	return tendrils

## 보이드 촉수 업데이트 (바닥에서 올라왔다 내려감)
static func update_void_tendrils(tendrils: Array, time: float, _delta: float = 0.0) -> void:
	for t in tendrils:
		if t == null or not is_instance_valid(t):
			continue
		var phase: float = t.get_meta("phase", 0.0)
		var speed: float = t.get_meta("speed", 0.5)
		var reach: float = t.get_meta("reach", 50.0)
		var base_y: float = t.get_meta("base_y", t.position.y)

		var wave = sin(time * speed + phase)
		t.position.y = base_y - abs(wave) * reach
		t.color.a = 0.06 + abs(wave) * 0.1
		t.rotation = sin(time * speed * 0.7 + phase) * 0.15  # 미세 흔들림

## ===================== S52: 스무스 카메라 =====================
## 카메라 설정 헬퍼 (플레이어에 Camera2D 부착)
static func setup_smooth_camera(player: Node2D, zoom_level: float = 1.0, ambient_shake_intensity: float = 0.0) -> Camera2D:
	if _clean_gameplay_view():
		ambient_shake_intensity = 0.0
	# 기존 카메라 체크, S57: 기존 카메라의 줌을 유지 (player.tscn Camera2D)
	for child in player.get_children():
		if child is Camera2D:
			child.position_smoothing_enabled = true
			child.position_smoothing_speed = 8.0
			child.drag_horizontal_enabled = false
			child.drag_vertical_enabled = false
			child.set_meta("ambient_camera_offset", Vector2.ZERO)
			# S57: 기존 카메라가 있으면 줌 유지 (player.gd에서 관리)
			if ambient_shake_intensity > 0.0:
				child.set_meta("ambient_shake", ambient_shake_intensity)
			# S57: 맵별 카메라 리밋 재적용
			if player.has_method("refresh_camera_limits"):
				player.refresh_camera_limits()
			return child

	var cam = Camera2D.new()
	cam.enabled = true
	cam.position_smoothing_enabled = true
	cam.position_smoothing_speed = 8.0
	cam.drag_horizontal_enabled = false
	cam.drag_vertical_enabled = false
	cam.zoom = Vector2(zoom_level, zoom_level)
	cam.set_meta("pixel_snap", false)
	cam.set_meta("ambient_camera_offset", Vector2.ZERO)
	if ambient_shake_intensity > 0.0:
		cam.set_meta("ambient_shake", ambient_shake_intensity)
	player.add_child(cam)
	return cam

## 카메라 이벤트 줌 (극적 순간용)
static func camera_event_zoom(cam: Camera2D, target_zoom: float, duration: float = 0.8) -> void:
	if cam == null:
		return
	var tween = cam.create_tween()
	tween.tween_property(cam, "zoom", Vector2(target_zoom, target_zoom), duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_CUBIC)

## 카메라 미세 흔들림 (지속적 환경 효과)
static func camera_ambient_shake(cam: Camera2D, intensity: float = 0.5) -> void:
	if cam == null:
		return
	if _clean_gameplay_view():
		cam.set_meta("ambient_shake", 0.0)
		cam.set_meta("ambient_camera_offset", Vector2.ZERO)
		return
	cam.set_meta("ambient_shake", intensity)

## 카메라 미세 흔들림 업데이트 (_process에서 호출)
static func update_camera_shake(cam: Camera2D, time: float) -> void:
	if cam == null:
		return
	if _clean_gameplay_view():
		cam.set_meta("ambient_camera_offset", Vector2.ZERO)
		return
	var intensity: float = cam.get_meta("ambient_shake", 0.0)
	if intensity <= 0.0:
		cam.set_meta("ambient_camera_offset", Vector2.ZERO)
		return
	var raw_offset = Vector2(
		sin(time * 7.3) * intensity + sin(time * 13.1) * intensity * 0.5,
		cos(time * 5.7) * intensity + cos(time * 11.9) * intensity * 0.5
	)
	# Player composes anticipation, ambience and event shake once per frame.
	cam.set_meta("ambient_camera_offset", raw_offset)

## S53: 파티클 오브젝트 풀
static var _particle_pool: Array[ColorRect] = []
const MAX_POOL_SIZE: int = 50

static func _get_pooled_rect() -> ColorRect:
	if _particle_pool.size() > 0:
		return _particle_pool.pop_back()
	return ColorRect.new()

static func _return_to_pool(rect: ColorRect) -> void:
	if _particle_pool.size() < MAX_POOL_SIZE:
		rect.visible = false
		if rect.get_parent():
			rect.get_parent().remove_child(rect)
		_particle_pool.append(rect)
	else:
		rect.queue_free()

## S53: 뷰포트 외 파티클 비활성화
static func cull_offscreen_particles(particles: Array, viewport_rect: Rect2) -> void:
	for p in particles:
		if p == null or not is_instance_valid(p):
			continue
		p.visible = viewport_rect.has_point(p.global_position) if p.is_inside_tree() else true

## ===================== S53: 동적 날씨 전환 =====================
## 시간 경과에 따라 날씨 강도 변화
static func update_weather_intensity(rain_node: Node, time: float, base_intensity: float = 1.0) -> void:
	if rain_node == null or not is_instance_valid(rain_node):
		return
	# 사인파 기반 강도 변화 (느린 주기)
	var cycle = sin(time * 0.05) * 0.5 + 0.5  # 0~1 oscillation
	var intensity = base_intensity * (0.4 + cycle * 0.6)  # 40%~100%
	if rain_node is GPUParticles2D:
		rain_node.amount_ratio = intensity
	elif rain_node is ColorRect:
		rain_node.color.a = intensity * 0.3

## 안개 밀도 동적 변화
static func update_dynamic_fog(fog_rects: Array, time: float, base_alpha: float = 0.06) -> void:
	var cycle = sin(time * 0.03) * 0.5 + 0.5
	for rect in fog_rects:
		if rect == null or not is_instance_valid(rect):
			continue
		var phase = rect.get_meta("phase", 0.0)
		rect.color.a = base_alpha * (0.5 + cycle * 0.5) + sin(time * 0.5 + phase) * base_alpha * 0.3

## 번개 효과 (비 맵 전용)
static func add_lightning(parent: Node2D) -> ColorRect:
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(0.9, 0.9, 1.0, 0.0)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 10
	flash.set_meta("next_flash", randf_range(8.0, 25.0))
	flash.set_meta("timer", 0.0)
	flash.set_meta("clarity_disabled", _clean_gameplay_view())
	parent.add_child(flash)
	return flash

static func update_lightning(flash_rect: ColorRect, delta: float) -> void:
	if flash_rect == null or not is_instance_valid(flash_rect):
		return
	if flash_rect.get_meta("clarity_disabled", false):
		flash_rect.color.a = 0.0
		return
	var timer = flash_rect.get_meta("timer", 0.0) + delta
	var next = flash_rect.get_meta("next_flash", 15.0)
	flash_rect.set_meta("timer", timer)
	if timer >= next:
		# 번개 플래시!
		flash_rect.color.a = randf_range(0.15, 0.35)
		flash_rect.set_meta("timer", 0.0)
		flash_rect.set_meta("next_flash", randf_range(8.0, 25.0))
	elif flash_rect.color.a > 0:
		flash_rect.color.a = maxf(0.0, flash_rect.color.a - delta * 3.0)

## ===================== S57: 앰비언트 와일드라이프 =====================

## 사인파 궤적 반딧불 (숲/습지 맵, 개별 이동, GPUParticles2D보다 세밀한 제어)
## 4px 밝은 점이 사인파 경로를 따라 천천히 떠다님
static func add_drifting_fireflies(parent: Node2D, count: int = 10, area: Vector2 = Vector2(800, 576), color: Color = Color(0.6, 0.95, 0.4, 0.6)) -> Array[ColorRect]:
	var flies: Array[ColorRect] = []
	if _clean_gameplay_view():
		return flies
	for i in range(count):
		var fly = ColorRect.new()
		fly.size = Vector2(3, 3)
		fly.color = Color(color.r + randf_range(-0.1, 0.1), color.g, color.b + randf_range(-0.1, 0.1), 0.0)
		fly.position = Vector2(randf_range(40, area.x - 40), randf_range(40, area.y - 40))
		fly.z_index = 6
		fly.mouse_filter = Control.MOUSE_FILTER_IGNORE
		fly.set_meta("base_x", fly.position.x)
		fly.set_meta("base_y", fly.position.y)
		fly.set_meta("freq_x", randf_range(0.2, 0.6))
		fly.set_meta("freq_y", randf_range(0.15, 0.45))
		fly.set_meta("amp_x", randf_range(30, 80))
		fly.set_meta("amp_y", randf_range(20, 50))
		fly.set_meta("phase_x", randf() * TAU)
		fly.set_meta("phase_y", randf() * TAU)
		fly.set_meta("blink_phase", randf() * TAU)
		fly.set_meta("area", area)
		parent.add_child(fly)
		flies.append(fly)
	return flies

## 반딧불 업데이트 (사인파 이동 + 알파 깜빡임)
static func update_drifting_fireflies(flies: Array[ColorRect], time: float) -> void:
	for fly in flies:
		if not is_instance_valid(fly):
			continue
		var bx: float = fly.get_meta("base_x", 400.0)
		var by: float = fly.get_meta("base_y", 300.0)
		var fx: float = fly.get_meta("freq_x", 0.4)
		var fy: float = fly.get_meta("freq_y", 0.3)
		var ax: float = fly.get_meta("amp_x", 50.0)
		var ay: float = fly.get_meta("amp_y", 30.0)
		var px: float = fly.get_meta("phase_x", 0.0)
		var py: float = fly.get_meta("phase_y", 0.0)
		var bp: float = fly.get_meta("blink_phase", 0.0)

		fly.position.x = bx + sin(time * fx + px) * ax
		fly.position.y = by + sin(time * fy + py) * ay
		var blink = maxf(sin(time * 1.2 + bp), 0.0)
		fly.color.a = blink * 0.6

## 낙엽 효과 (나무에서 떨어지는 작은 잎사귀)
static func add_falling_leaves(parent: Node2D, count: int = 8, area: Vector2 = Vector2(800, 576), color: Color = Color(0.45, 0.55, 0.2, 0.5)) -> Array[ColorRect]:
	var leaves: Array[ColorRect] = []
	if _clean_gameplay_view():
		return leaves
	for i in range(count):
		var leaf = ColorRect.new()
		leaf.size = Vector2(randi_range(3, 5), randi_range(3, 5))
		var hue_shift = randf_range(-0.1, 0.15)
		leaf.color = Color(
			clampf(color.r + hue_shift + randf_range(-0.05, 0.05), 0, 1),
			clampf(color.g - abs(hue_shift) * 0.3, 0, 1),
			clampf(color.b + randf_range(-0.05, 0.05), 0, 1),
			color.a
		)
		leaf.position = Vector2(randf_range(0, area.x), randf_range(-50, area.y))
		leaf.z_index = 5
		leaf.mouse_filter = Control.MOUSE_FILTER_IGNORE
		leaf.pivot_offset = leaf.size / 2.0
		leaf.set_meta("fall_speed", randf_range(12, 28))
		leaf.set_meta("sway_phase", randf() * TAU)
		leaf.set_meta("sway_amp", randf_range(25, 60))
		leaf.set_meta("spin_speed", randf_range(0.5, 2.0))
		leaf.set_meta("base_x", leaf.position.x)
		leaf.set_meta("area", area)
		parent.add_child(leaf)
		leaves.append(leaf)
	return leaves

## 낙엽 업데이트 (낙하 + 좌우 흔들림 + 회전)
static func update_falling_leaves(leaves: Array[ColorRect], time: float, delta: float) -> void:
	for leaf in leaves:
		if not is_instance_valid(leaf):
			continue
		var area: Vector2 = leaf.get_meta("area", Vector2(800, 576))
		var speed: float = leaf.get_meta("fall_speed", 20.0)
		var phase: float = leaf.get_meta("sway_phase", 0.0)
		var amp: float = leaf.get_meta("sway_amp", 40.0)
		var spin: float = leaf.get_meta("spin_speed", 1.0)
		var bx: float = leaf.get_meta("base_x", leaf.position.x)

		leaf.position.y += speed * delta
		leaf.position.x = bx + sin(time * 0.7 + phase) * amp
		leaf.rotation = sin(time * spin + phase) * 0.8
		leaf.color.a = 0.3 + sin(time * 0.5 + phase) * 0.15

		if leaf.position.y > area.y + 30:
			leaf.position.y = -20
			bx = randf_range(0, area.x)
			leaf.set_meta("base_x", bx)

## ===================== S57: 시간대 색조 변화 =====================

## 플레이타임 기반 시간대 색상 시프트 (CanvasLayer 오버레이)
## ~30분 실시간 주기: 새벽(따뜻) → 낮(중립) → 황혼(차가움) → 밤(어두움)
static func add_time_of_day(parent: Node2D) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, 5)
	var layer = CanvasLayer.new()
	layer.layer = 5

	var rect = ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rect.color = Color(0, 0, 0, 0)
	rect.set_meta("tod_active", true)
	layer.add_child(rect)
	parent.add_child(layer)
	return layer

## 시간대 색상 업데이트 (맵의 _process에서 호출)
static func update_time_of_day(layer: CanvasLayer, elapsed_time: float) -> void:
	if layer == null or not is_instance_valid(layer):
		return
	var cycle = fmod(elapsed_time, 1800.0) / 1800.0
	var tint: Color
	var alpha: float

	if cycle < 0.2:
		tint = Color(0.9, 0.6, 0.3)
		alpha = 0.06 * (1.0 - cycle / 0.2)
	elif cycle < 0.5:
		tint = Color(1.0, 1.0, 0.95)
		alpha = 0.01
	elif cycle < 0.7:
		var t = (cycle - 0.5) / 0.2
		tint = Color(0.4, 0.35, 0.6)
		alpha = 0.04 * t
	else:
		var t = (cycle - 0.7) / 0.3
		tint = Color(0.15, 0.18, 0.35)
		alpha = 0.04 + 0.04 * sin(t * PI)

	if layer.get_child_count() > 0:
		var rect = layer.get_child(0)
		if rect is ColorRect:
			rect.color = Color(tint.r, tint.g, tint.b, alpha)

## ===================== S57: 플레이어 포그 오브 워 라이트 =====================

## 플레이어에 큰 범위 PointLight2D 부착 (주변 밝힘, 먼 곳 어둡게)
static func add_player_fog_light(player: Node2D, radius: float = 300.0, energy: float = 1.2, color: Color = Color(1.0, 0.95, 0.85)) -> PointLight2D:
	for child in player.get_children():
		if child is PointLight2D and child.has_meta("fog_light"):
			return child

	var light = PointLight2D.new()
	light.color = color
	light.energy = energy
	light.texture = _create_light_texture(int(radius))
	light.texture_scale = 1.0
	light.blend_mode = Light2D.BLEND_MODE_ADD
	light.set_meta("fog_light", true)
	player.add_child(light)
	return light

## ===================== S57: 로어 오브젝트 글로우 =====================

## 맵에 로어/숨겨진 아이템 글로우 포인트 추가 (펄싱 라이트)
static func add_lore_glow(parent: Node2D, pos: Vector2, color: Color = Color(0.8, 0.7, 0.3, 0.6), radius: float = 48.0) -> PointLight2D:
	var light = PointLight2D.new()
	light.position = pos
	light.color = color
	light.energy = 0.6
	light.texture = _create_light_texture(int(radius))
	light.texture_scale = 1.0
	light.blend_mode = Light2D.BLEND_MODE_ADD
	light.set_meta("lore_glow", true)
	light.set_meta("phase", randf() * TAU)
	light.set_meta("base_energy", 0.6)
	parent.add_child(light)

	var marker = ColorRect.new()
	marker.size = Vector2(4, 4)
	marker.position = pos - Vector2(2, 2)
	marker.color = Color(color.r, color.g, color.b, 0.5)
	marker.z_index = 3
	marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
	marker.set_meta("lore_marker", true)
	marker.set_meta("phase", light.get_meta("phase"))
	parent.add_child(marker)

	return light

## 로어 글로우 업데이트 (펄싱 에너지 + 마커 알파)
static func update_lore_glows(parent: Node2D, time: float) -> void:
	for child in parent.get_children():
		if child is PointLight2D and child.has_meta("lore_glow"):
			var phase: float = child.get_meta("phase", 0.0)
			var base_e: float = child.get_meta("base_energy", 0.6)
			child.energy = base_e + sin(time * 2.0 + phase) * 0.25
		elif child is ColorRect and child.has_meta("lore_marker"):
			var phase: float = child.get_meta("phase", 0.0)
			child.color.a = 0.3 + sin(time * 2.0 + phase) * 0.2

## ===================== S57: 맵 전환 테마 파티클 =====================

## 맵 진입 시 테마별 파티클 버스트 (자동 소멸)
static func spawn_transition_particles(parent: Node, biome: String = "forest") -> void:
	if _clean_gameplay_view():
		return
	var layer = CanvasLayer.new()
	layer.layer = 6

	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	particles.one_shot = true
	particles.emitting = true
	particles.position = Vector2(640, 360)

	match biome:
		"forest":
			mat.direction = Vector3(0, 1, 0)
			mat.spread = 180.0
			mat.initial_velocity_min = 40.0
			mat.initial_velocity_max = 120.0
			mat.gravity = Vector3(10, 40, 0)
			mat.scale_min = 1.5
			mat.scale_max = 3.5
			mat.color = Color(0.35, 0.5, 0.2, 0.5)
			particles.amount = 30
			particles.lifetime = 1.8
		"belt", "coast":
			mat.direction = Vector3(0.3, 0.5, 0)
			mat.spread = 120.0
			mat.initial_velocity_min = 20.0
			mat.initial_velocity_max = 80.0
			mat.gravity = Vector3(15, 25, 0)
			mat.scale_min = 1.0
			mat.scale_max = 2.5
			mat.color = Color(0.6, 0.5, 0.3, 0.4)
			particles.amount = 40
			particles.lifetime = 2.0
		"void":
			mat.direction = Vector3(0, -1, 0)
			mat.spread = 160.0
			mat.initial_velocity_min = 15.0
			mat.initial_velocity_max = 50.0
			mat.gravity = Vector3(0, -20, 0)
			mat.scale_min = 2.0
			mat.scale_max = 4.0
			mat.color = Color(0.2, 0.1, 0.35, 0.35)
			particles.amount = 25
			particles.lifetime = 2.5
		"shelter", "mist":
			mat.direction = Vector3(0.5, 0, 0)
			mat.spread = 90.0
			mat.initial_velocity_min = 10.0
			mat.initial_velocity_max = 40.0
			mat.gravity = Vector3(5, -5, 0)
			mat.scale_min = 3.0
			mat.scale_max = 6.0
			mat.color = Color(0.7, 0.75, 0.85, 0.25)
			particles.amount = 15
			particles.lifetime = 2.0
		_:
			mat.direction = Vector3(0, 0.5, 0)
			mat.spread = 180.0
			mat.initial_velocity_min = 20.0
			mat.initial_velocity_max = 60.0
			mat.gravity = Vector3(0, 20, 0)
			mat.scale_min = 1.0
			mat.scale_max = 2.0
			mat.color = Color(0.5, 0.5, 0.5, 0.3)
			particles.amount = 20
			particles.lifetime = 1.5

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(640, 360, 0)

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(mat.color.r, mat.color.g, mat.color.b, 0.0))
	g.add_point(0.2, mat.color)
	g.add_point(0.6, Color(mat.color.r, mat.color.g, mat.color.b, mat.color.a * 0.7))
	g.set_color(1, Color(mat.color.r, mat.color.g, mat.color.b, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	particles.process_material = mat
	particles.visibility_rect = Rect2(-700, -400, 1400, 800)
	layer.add_child(particles)
	parent.add_child(layer)

	var cleanup_time = particles.lifetime + 0.5
	var timer = parent.get_tree().create_timer(cleanup_time)
	timer.timeout.connect(func():
		if is_instance_valid(layer):
			layer.queue_free()
	)

## ===================== S59: 인터랙티브 프롭 시스템 =====================

## 맵에 상호작용 가능한 소형 오브젝트 배치
## type: "barrel" (Grains), "crate" (아이템), "sign" (텍스트), "campfire" (HP 회복)
static func add_interactive_prop(map: Node2D, pos: Vector2, type: String, config: Dictionary = {}) -> Area2D:
	var area = Area2D.new()
	area.position = pos + Vector2(16, 16)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect_shape = RectangleShape2D.new()
	rect_shape.size = Vector2(28, 28)
	shape.shape = rect_shape
	area.add_child(shape)

	var flag_name = "prop_%s_%d_%d" % [type, int(pos.x), int(pos.y)]
	var interacted = GameManager.get_flag(flag_name)

	# S209: 비주얼.
	# 예전에는 갈색/회색 ColorRect 사각형이 타일 위에 그대로 떠 있었다.
	# 이제 캐릭터와 같은 절차적 픽셀아트 스프라이트를 쓴다.
	var visual = Sprite2D.new()
	visual.texture = PixelSprite.create_prop_texture(type, interacted)
	visual.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	visual.z_index = 1
	# 스프라이트는 32x32 캔버스에 바닥이 아래쪽에 그려져 있다. 발밑을 타일에 맞춘다.
	visual.position = Vector2(0, -2)
	visual.set_meta("prop_type", type)

	match type:
		"sign":
			# "!" 텍스트 마커
			var marker = Label.new()
			marker.text = "!"
			marker.add_theme_font_size_override("font_size", 13)
			marker.add_theme_color_override("font_color", Color(1.0, 0.9, 0.5))
			marker.position = Vector2(-3, -30)
			marker.mouse_filter = Control.MOUSE_FILTER_IGNORE
			area.add_child(marker)
		"campfire":
			# 불꽃 파티클
			var fire = GPUParticles2D.new()
			var fire_mat = ParticleProcessMaterial.new()
			fire_mat.direction = Vector3(0, -1, 0)
			fire_mat.spread = 20.0
			fire_mat.initial_velocity_min = 6.0
			fire_mat.initial_velocity_max = 14.0
			fire_mat.gravity = Vector3(0, -10, 0)
			fire_mat.scale_min = 0.5
			fire_mat.scale_max = 1.5
			var fire_grad = GradientTexture1D.new()
			var fg = Gradient.new()
			fg.set_color(0, Color(1.0, 0.85, 0.3, 0.9))
			fg.add_point(0.4, Color(1.0, 0.5, 0.1, 0.7))
			fg.set_color(1, Color(0.5, 0.15, 0.05, 0.0))
			fire_grad.gradient = fg
			fire_mat.color_ramp = fire_grad
			fire_mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
			fire_mat.emission_sphere_radius = 4.0
			fire.process_material = fire_mat
			fire.amount = 8
			fire.lifetime = 0.6
			fire.position = Vector2(0, -4)
			fire.z_index = 2
			fire.visibility_rect = Rect2(-16, -24, 32, 32)
			area.add_child(fire)
			# 오렌지 글로우
			var glow = ColorRect.new()
			glow.size = Vector2(48, 48)
			glow.position = Vector2(-24, -28)
			glow.color = Color(0.9, 0.6, 0.2, 0.06)
			glow.z_index = -1
			glow.mouse_filter = Control.MOUSE_FILTER_IGNORE
			glow.set_meta("campfire_glow", true)
			glow.set_meta("phase", randf() * TAU)
			area.add_child(glow)

	area.add_child(visual)

	# 상호작용 처리
	if not interacted:
		area.body_entered.connect(func(body):
			if body.name != "Player" or GameManager.current_state != GameManager.GameState.EXPLORATION:
				return
			if GameManager.get_flag(flag_name):
				return
			GameManager.set_flag(flag_name)
			match type:
				"barrel":
					var grains = randi_range(1, 3)
					GameManager.player_data.grains += grains
					NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)
					AudioManager.play_sfx("ui_select")
					visual.texture = PixelSprite.create_prop_texture("barrel", true)
				"crate":
					var roll = randi() % 100
					if roll < 40:
						GameManager.add_item("potion", 1)
						NotificationToast.show_toast("Found a Potion!", NotificationToast.ToastType.SUCCESS)
					elif roll < 70:
						var grains = randi_range(2, 5)
						GameManager.player_data.grains += grains
						NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)
					else:
						NotificationToast.show_toast("The crate is empty.", NotificationToast.ToastType.INFO)
					AudioManager.play_sfx("ui_select")
					visual.texture = PixelSprite.create_prop_texture("crate", true)
				"sign":
					var text = config.get("text", "A faded sign. The words are gone.")
					NotificationToast.show_toast(text, NotificationToast.ToastType.INFO)
				"campfire":
					var heal = config.get("heal", 5)
					GameManager.player_data.hp = mini(GameManager.player_data.hp + heal, GameManager.player_data.max_hp)
					NotificationToast.show_toast("Rested by the fire. +%d HP" % heal, NotificationToast.ToastType.SUCCESS)
					AudioManager.play_sfx("ui_select")
		)

	map.add_child(area)
	return area

## ===================== S209: 발견물 마커 =====================

## 숨은 상자 / 기억 단서 마커.
## 예전에는 32x32 ColorRect였고, 기본값인 Clean Gameplay View에서는 알파가 0이라
## 화면에 아무것도 없었다. 즉 "숨겨진 보상"이 우연히 밟기 전에는 존재조차 알 수 없었다.
## 이제는 절차적 픽셀아트로 그리되, 눈에 띄는 정도는 억제해 탐색의 재미를 남긴다.
## `kind`는 "chest" 또는 "clue".
static func make_discovery_marker(kind: String) -> Node2D:
	var holder := Node2D.new()
	holder.name = "DiscoveryMarker"
	holder.z_index = 0

	var sprite := Sprite2D.new()
	sprite.texture = PixelSprite.create_prop_texture(kind, false)
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(0, -2)
	sprite.modulate = Color(1, 1, 1, 0.92)
	holder.add_child(sprite)

	# 아주 옅은 발밑 광원. Clean View에서도 남겨서 "여기 뭔가 있다"는 신호는 유지한다.
	var glow := ColorRect.new()
	glow.size = Vector2(30, 12)
	glow.position = Vector2(-15, 4)
	glow.color = Color(0.72, 0.58, 0.28, 0.10) if kind == "chest" else Color(0.42, 0.58, 0.86, 0.10)
	glow.z_index = -1
	glow.mouse_filter = Control.MOUSE_FILTER_IGNORE
	holder.add_child(glow)
	return holder

## ===================== S59: NPC 배회 시스템 =====================

## NPC를 반경 내에서 천천히 배회시키는 시스템 (트윈 기반)
static func add_npc_wander(npc_node: Node2D, radius: float = 48.0) -> void:
	if npc_node == null or not is_instance_valid(npc_node):
		return
	var spawn_pos = npc_node.position
	npc_node.set_meta("wander_spawn", spawn_pos)
	npc_node.set_meta("wander_radius", radius)
	npc_node.set_meta("wander_active", true)
	# 첫 배회 시작
	_start_wander_step(npc_node)

## NPC 배회 단일 스텝 (목표 이동 → 대기 → 반복)
static func _start_wander_step(npc_node: Node2D) -> void:
	if npc_node == null or not is_instance_valid(npc_node):
		return
	if not npc_node.get_meta("wander_active", false):
		return

	var spawn: Vector2 = npc_node.get_meta("wander_spawn", npc_node.position)
	var radius: float = npc_node.get_meta("wander_radius", 48.0)

	# 반경 내 랜덤 목표 지점
	var angle: float = randf() * TAU
	var dist: float = randf_range(radius * 0.2, radius)
	var target_position: Vector2 = spawn + Vector2(cos(angle) * dist, sin(angle) * dist)

	# Keep market walkers restrained and readable: the authored gait plays while
	# translation eases in and out, then returns to the matching idle direction.
	var move_dist: float = npc_node.position.distance_to(target_position)
	var duration: float = maxf(move_dist / 26.0, 0.45)
	var travel_direction: Vector2 = target_position - npc_node.position
	# S210: 걸음 속도를 실제 이동 속도에 맞춘다.
	# 배회 NPC는 초당 약 26px로 움직이는데 다리는 플레이어와 같은 속도로 돌고 있었다.
	# 발이 지면을 긁으며 미끄러지던 원인. 평균 속도를 기준으로 보폭 주기를 낮춘다.
	#
	# S235: 평균만으로는 부족했다. 이동은 SINE 가감속이라 실제 속도가 0에서 평균의
	# 1.57배까지 오르내리는데 다리는 평균 한 값으로 고정돼 있었다. 실측하면 가장 느린
	# 순간에 다리가 몸보다 초당 28.9px 앞서 있었다. 걷기 시작과 끝에서 발이 제자리를
	# 긁는다는 뜻이다. 이제 매 프레임 실제 속도로 보폭을 맞춘다.
	_set_wander_animation(npc_node, travel_direction, true, move_dist / duration)
	_attach_wander_gait_sync(npc_node, move_dist, duration)

	var tween = npc_node.create_tween()
	# QUAD 가감속은 시작과 끝에서 속도가 크게 흔들려 발이 미끄러진다. SINE이 더 완만하다.
	tween.tween_property(npc_node, "position", target_position, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
	tween.tween_callback(func():
		if npc_node != null and is_instance_valid(npc_node):
			_detach_wander_gait_sync(npc_node)
			_set_wander_animation(npc_node, travel_direction, false)
	)
	var wait = randf_range(1.8, 4.2)
	tween.tween_interval(wait)
	# 반복
	var npc_ref: WeakRef = weakref(npc_node)
	tween.tween_callback(func():
		var node: Node2D = npc_ref.get_ref() as Node2D
		if node != null and is_instance_valid(node):
			_start_wander_step(node)
	)

## 배회 NPC의 방향/재생 애니메이션을 맞춘다.
## `travel_speed`(px/s)가 주어지면 걸음 주기를 실제 이동 속도에 비례시켜, 발이
## 지면을 긁는 미끄러짐을 없앤다. 기준은 플레이어의 보행 속도(WANDER_GAIT_REFERENCE).
const WANDER_GAIT_REFERENCE: float = 120.0

## S235: 이동하는 동안 매 프레임 보폭 주기를 실제 속도에 맞추는 감시자.
##
## 위치를 미분해서 속도를 구할 수도 있지만, 그 값은 구조적으로 한 프레임 늦고
## 떨림을 막으려 필터를 걸면 지연이 더 붙는다. 실측하면 그 지연이 남은 미끄러짐의
## 대부분이었다. 이동 곡선을 우리가 정했으므로 속도는 그냥 계산하면 된다.
##
## SINE 가감속(EASE_IN_OUT)의 위치는 D * (1 - cos(pi*t/d)) / 2 이므로
## 속도는 D*pi/(2d) * sin(pi*t/d) 다. 지연도 없고 필터도 필요 없다.
class WanderGaitSync extends Node:
	var walker: AnimatedSprite2D
	var distance: float = 0.0
	var duration: float = 0.0
	var _elapsed: float = 0.0

	func _process(delta: float) -> void:
		if walker == null or not is_instance_valid(walker):
			queue_free()
			return
		if duration <= 0.0:
			return
		_elapsed = minf(_elapsed + delta, duration)
		var peak := distance * PI / (2.0 * duration)
		var speed := peak * sin(PI * _elapsed / duration)
		walker.speed_scale = clampf(speed / WANDER_GAIT_REFERENCE, 0.0, 1.6)

static func _attach_wander_gait_sync(npc_node: Node2D, distance: float, duration: float) -> void:
	var walker := npc_node as AnimatedSprite2D
	if walker == null:
		return
	_detach_wander_gait_sync(npc_node)
	var sync := WanderGaitSync.new()
	sync.name = "WanderGaitSync"
	sync.walker = walker
	sync.distance = distance
	sync.duration = duration
	walker.add_child(sync)

static func _detach_wander_gait_sync(npc_node: Node2D) -> void:
	if npc_node == null or not is_instance_valid(npc_node):
		return
	var existing := npc_node.get_node_or_null("WanderGaitSync")
	if existing != null:
		npc_node.remove_child(existing)
		existing.queue_free()
	var walker := npc_node as AnimatedSprite2D
	if walker != null:
		walker.speed_scale = 1.0

static func _set_wander_animation(npc_node: Node2D, direction: Vector2, moving: bool, travel_speed: float = 0.0) -> void:
	var animated := npc_node as AnimatedSprite2D
	if animated == null or animated.sprite_frames == null:
		return
	var suffix := "down"
	if absf(direction.x) > absf(direction.y):
		suffix = "right" if direction.x > 0.0 else "left"
	else:
		suffix = "down" if direction.y > 0.0 else "up"
	var animation := ("walk_" if moving else "idle_") + suffix
	if not animated.sprite_frames.has_animation(animation):
		return
	if moving and travel_speed > 0.0:
		animated.speed_scale = clampf(travel_speed / WANDER_GAIT_REFERENCE, 0.25, 1.6)
	else:
		animated.speed_scale = 1.0
	animated.play(animation)

## ===================== S59: 트리거 접근 글로우 =====================

## 스토리 트리거 Area2D에 접근 시 펄싱 글로우 테두리 추가
## 플레이어 위치 기반, _process에서 호출
static func update_trigger_approach_glow(map: Node2D, player_pos: Vector2, time: float) -> void:
	for child in map.get_children():
		if not (child is Area2D):
			continue
		# 이미 글로우가 있는지 체크
		var glow_border: ColorRect = null
		for sub in child.get_children():
			if sub is ColorRect and sub.has_meta("approach_glow"):
				glow_border = sub
				break

		var dist = player_pos.distance_to(child.global_position)
		if dist < 52.0:
			# 접근, 글로우 생성 또는 업데이트
			if glow_border == null:
				glow_border = ColorRect.new()
				# 트리거 크기 추정 (CollisionShape2D에서)
				# Use a compact diamond beacon instead of revealing the full invisible
				# trigger rectangle, which looked like debug geometry in play.
				var trigger_size = Vector2(8, 8)
				glow_border.size = trigger_size
				glow_border.position = Vector2(-4, -18)
				glow_border.rotation = PI / 4.0
				glow_border.color = Color(1.0, 1.0, 1.0, 0.0)
				glow_border.z_index = 3
				glow_border.mouse_filter = Control.MOUSE_FILTER_IGNORE
				glow_border.set_meta("approach_glow", true)
				# 내부를 투명하게 (테두리만 표시), 내부 마스크
				var inner = ColorRect.new()
				inner.size = Vector2(4, 4)
				inner.position = Vector2(2, 2)
				inner.color = Color(0, 0, 0, 0)
				inner.mouse_filter = Control.MOUSE_FILTER_IGNORE
				inner.set_meta("glow_inner", true)
				glow_border.add_child(inner)
				child.add_child(glow_border)
			# 펄싱 알파
			var pulse = 0.55 + sin(time * 3.4) * 0.18
			glow_border.color = Color(0.98, 0.78, 0.38, pulse)
			var beacon_scale := 0.90 + sin(time * 3.4) * 0.10
			glow_border.scale = Vector2(beacon_scale, beacon_scale)
		else:
			# 멀어짐, 글로우 제거
			if glow_border != null:
				glow_border.queue_free()

## ===================== S59: 프로시저럴 안개 레이어 =====================

## 맵에 깊이감 있는 프로시저럴 안개 추가 (대형 블러 렉트가 천천히 드리프트)
## 바이옴별 밀도/색상/속도 조절
static func add_fog_layer(map: Node2D, density: float = 0.5, color: Color = Color(0.3, 0.3, 0.35, 0.06), speed: float = 3.0) -> Array[ColorRect]:
	if _clean_gameplay_view():
		return []
	var fog_count = int(3 + density * 3)  # 3~6개 안개 렉트
	var fogs: Array[ColorRect] = []

	var layer = CanvasLayer.new()
	layer.layer = 2

	for i in range(fog_count):
		var fog = ColorRect.new()
		var w = randf_range(250, 550) * (0.8 + density * 0.4)
		var h = randf_range(80, 200) * (0.8 + density * 0.4)
		fog.size = Vector2(w, h)
		fog.position = Vector2(randf_range(-200, 1100), randf_range(50, 650))
		fog.color = Color(
			color.r + randf_range(-0.03, 0.03),
			color.g + randf_range(-0.03, 0.03),
			color.b + randf_range(-0.03, 0.03),
			color.a * randf_range(0.7, 1.3)
		)
		fog.mouse_filter = Control.MOUSE_FILTER_IGNORE
		fog.set_meta("fog_speed", speed * randf_range(0.6, 1.4))
		fog.set_meta("fog_phase", randf() * TAU)
		fog.set_meta("fog_base_alpha", fog.color.a)
		fog.set_meta("fog_drift_y", randf_range(-0.5, 0.5))
		layer.add_child(fog)
		fogs.append(fog)

	map.add_child(layer)
	return fogs

## 프로시저럴 안개 업데이트 (_process에서 호출)
static func update_fog_layer(fogs: Array[ColorRect], time: float) -> void:
	for fog in fogs:
		if not is_instance_valid(fog):
			continue
		var speed: float = fog.get_meta("fog_speed", 3.0)
		var phase: float = fog.get_meta("fog_phase", 0.0)
		var base_a: float = fog.get_meta("fog_base_alpha", 0.06)
		var drift_y: float = fog.get_meta("fog_drift_y", 0.0)

		fog.position.x += speed * 0.016
		fog.position.y += sin(time * 0.2 + phase) * drift_y * 0.016
		fog.color.a = base_a + sin(time * 0.35 + phase) * base_a * 0.4

		# 화면 밖 → 리셋
		if fog.position.x > 1500:
			fog.position.x = -fog.size.x - randf_range(0, 200)
			fog.position.y = randf_range(50, 650)

## ===================== S59: 바람에 의한 초목 흔들림 =====================

## 풀/덤불 타일에 미세한 수평 흔들림 적용 (사인파 x 오프셋)
## _process에서 호출할 필요 없음, 트윈 기반 자동 루프
static func add_wind_sway(map: Node2D, strength: float = 2.0) -> void:
	if _clean_gameplay_view():
		return
	# 맵의 기존 풀잎 오버레이(ColorRect)에 바람 흔들림 메타 추가
	# S43 add_grass_sway로 만든 blade들에 적용
	for child in map.get_children():
		if child is ColorRect and child.has_meta("phase"):
			child.set_meta("wind_strength", strength)
			child.set_meta("wind_phase_offset", randf() * TAU)

## 바람 흔들림 업데이트 (_process에서 호출)
## 기존 update_grass_sway와 함께 사용, 추가 x 오프셋
static func update_wind_sway(map: Node2D, time: float) -> void:
	for child in map.get_children():
		if child is ColorRect and child.has_meta("wind_strength"):
			var ws: float = child.get_meta("wind_strength", 2.0)
			var wp: float = child.get_meta("wind_phase_offset", 0.0)
			# 원래 위치 기반 사인파 x 오프셋
			var offset_x = sin(time * 0.8 + wp) * ws
			child.position.x += offset_x * 0.016  # delta 근사

## ===================== S59: 깊이 기반 조명 그라디언트 =====================

## 맵 상단을 약간 밝게, 하단을 약간 어둡게 하는 수직 그라디언트 오버레이
## 머리 위 광원 시뮬레이션 (5~10% 차이)
static func add_depth_gradient(map: Node2D, intensity: float = 0.08) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(map, 2)
	var layer = CanvasLayer.new()
	layer.layer = 2  # 안개와 같은 레이어

	# 상단: 밝은 오버레이 (서서히 사라짐)
	var top_grad = ColorRect.new()
	top_grad.set_anchors_preset(Control.PRESET_FULL_RECT)
	top_grad.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_grad.color = Color(1.0, 0.98, 0.9, 0.0)  # 기본 투명

	# 수직 그라디언트를 여러 줄 ColorRect로 근사
	var strip_count = 8
	for i in range(strip_count):
		var strip = ColorRect.new()
		strip.set_anchors_preset(Control.PRESET_TOP_WIDE)
		strip.anchor_top = float(i) / strip_count
		strip.anchor_bottom = float(i + 1) / strip_count
		strip.mouse_filter = Control.MOUSE_FILTER_IGNORE

		# 상단(i=0)은 밝게, 하단(i=strip_count-1)은 어둡게
		var t = float(i) / (strip_count - 1)
		if t < 0.5:
			# 상단 반, 약간 밝게
			var bright = (0.5 - t) * 2.0 * intensity
			strip.color = Color(1.0, 0.98, 0.92, bright)
		else:
			# 하단 반, 약간 어둡게
			var dark = (t - 0.5) * 2.0 * intensity
			strip.color = Color(0.0, 0.0, 0.05, dark)

		layer.add_child(strip)

	map.add_child(layer)
	return layer

## Curated CG plates as subtle in-map atmosphere.
## S133: Never draw these above gameplay. They are background mood only.
static func add_illustration_atmosphere(parent: Node, texture_path: String, alpha: float = 0.12, tint: Color = Color(1, 1, 1, 1), layer_index: int = -20) -> CanvasLayer:
	if _clean_gameplay_view():
		return _empty_layer(parent, mini(layer_index, -20))
	if texture_path == "" or not ResourceLoader.exists(texture_path):
		return null

	var layer = CanvasLayer.new()
	layer.layer = mini(layer_index, -20)
	layer.follow_viewport_enabled = false

	var plate = TextureRect.new()
	plate.set_anchors_preset(Control.PRESET_FULL_RECT)
	plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	plate.texture = load(texture_path)
	plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var safe_alpha := clampf(alpha * 0.45, 0.015, 0.055)
	plate.modulate = Color(tint.r, tint.g, tint.b, safe_alpha)
	layer.add_child(plate)

	var detail_plate = TextureRect.new()
	detail_plate.anchor_left = 0.0
	detail_plate.anchor_right = 1.0
	detail_plate.anchor_top = 0.0
	detail_plate.anchor_bottom = 0.38
	detail_plate.offset_top = -16
	detail_plate.offset_bottom = 16
	detail_plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	detail_plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	detail_plate.texture = load(texture_path)
	detail_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	detail_plate.modulate = Color(tint.r, tint.g, tint.b, clampf(alpha * 0.35, 0.012, 0.05))
	layer.add_child(detail_plate)

	parent.add_child(layer)

	var tw = plate.create_tween().set_loops()
	tw.tween_property(plate, "modulate:a", safe_alpha * 0.65, 5.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tw.tween_property(plate, "modulate:a", safe_alpha, 5.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	var detail_tw = detail_plate.create_tween().set_loops()
	detail_tw.tween_property(detail_plate, "modulate:a", clampf(safe_alpha * 1.15, 0.012, 0.06), 6.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	detail_tw.tween_property(detail_plate, "modulate:a", clampf(safe_alpha * 0.7, 0.01, 0.045), 6.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	return layer

## A map canvas is the readable, play-space-safe counterpart to a CG overlay.
## It sits below the live collision/interaction layer and keeps generated
## environment art visible in Clean Gameplay View, where atmospheric overlays
## are intentionally suppressed for clarity.
static func add_map_canvas(parent: Node2D, tilemap: TileMapLayer, texture_path: String, world_size: Vector2, settings: Dictionary = {}) -> Sprite2D:
	if texture_path == "" or not ResourceLoader.exists(texture_path):
		return null
	var texture := load(texture_path) as Texture2D
	if texture == null or texture.get_size().x <= 0 or texture.get_size().y <= 0:
		return null

	var canvas := Sprite2D.new()
	canvas.name = "EnvironmentCanvas"
	canvas.texture = texture
	canvas.centered = false
	canvas.position = Vector2.ZERO
	canvas.scale = Vector2(world_size.x / texture.get_size().x, world_size.y / texture.get_size().y)
	canvas.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	canvas.z_index = int(settings.get("z_index", -2))
	canvas.modulate = settings.get("tint", Color.WHITE)
	canvas.show_behind_parent = true
	parent.add_child(canvas)

	# S238: 이 맵의 지면이 얼마나 밝은지 한 번 재서 실루엣 마감에 알려 준다.
	# 밝은 테는 어두운 지면에서만 이득이므로, 배우가 자기가 선 땅을 알아야 한다.
	var canvas_image := texture.get_image()
	if canvas_image != null and not canvas_image.is_empty():
		var probe := canvas_image.duplicate() as Image
		probe.resize(24, 24, Image.INTERPOLATE_BILINEAR)
		var sum := 0.0
		for y in range(probe.get_height()):
			for x in range(probe.get_width()):
				var pixel := probe.get_pixel(x, y)
				sum += 0.2126 * pixel.r + 0.7152 * pixel.g + 0.0722 * pixel.b
		var tint: Color = canvas.modulate
		var tint_luma := 0.2126 * tint.r + 0.7152 * tint.g + 0.0722 * tint.b
		FieldActorVisuals.set_ground_luma(sum / float(probe.get_width() * probe.get_height()) * tint_luma)

	# The canvas owns the broad values, paths, and environmental landmarks;
	# the existing TileMap keeps collision boundaries and interaction placement
	# legible without covering the generated artwork in a second full map.
	if tilemap != null:
		var terrain_alpha := clampf(float(settings.get("terrain_alpha", 0.18)), 0.0, 1.0)
		tilemap.modulate = Color(1.0, 1.0, 1.0, terrain_alpha)
		tilemap.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	return canvas

## Static NPCs keep their authored field-sprite scale, then receive only a
## restrained breath. This avoids overwriting four-direction character sheets
## with an absolute scale during per-map ambience updates.
static func update_npc_idle_motion(npc: Node, time: float) -> void:
	if npc == null or not is_instance_valid(npc):
		return
	var sprite := npc.get_node_or_null("CharacterSprite") as AnimatedSprite2D
	if sprite == null:
		sprite = npc.get_node_or_null("AnimatedSprite2D") as AnimatedSprite2D
	if sprite == null:
		return
	if not sprite.has_meta("field_idle_rest_scale"):
		sprite.set_meta("field_idle_rest_scale", sprite.scale)
		sprite.set_meta("field_idle_rest_offset", sprite.offset)
		sprite.set_meta("field_idle_rest_rotation", sprite.rotation)
	var rest_scale: Vector2 = sprite.get_meta("field_idle_rest_scale", sprite.scale)
	var rest_offset: Vector2 = sprite.get_meta("field_idle_rest_offset", sprite.offset)
	var rest_rotation: float = float(sprite.get_meta("field_idle_rest_rotation", sprite.rotation))
	var phase := time * 1.5
	if npc is Node2D:
		phase += (npc as Node2D).position.x * 0.1
	var breath := sin(phase)
	sprite.scale = rest_scale * Vector2(1.0 + breath * 0.008, 1.0 - breath * 0.006)
	# S213: a slow weight transfer keeps static story characters alive while
	# preserving their authored foot baseline and readable interaction pose.
	sprite.offset = rest_offset + Vector2(sin(phase * 0.43) * 0.28, -absf(sin(phase * 0.51)) * 0.16)
	sprite.rotation = rest_rotation + sin(phase * 0.37) * 0.0035
	var grounding := npc.get_node_or_null("FieldGrounding") as Node2D
	if grounding != null:
		var previous_time := float(npc.get_meta("field_idle_previous_time", time))
		var idle_delta := clampf(time - previous_time, 0.0, 0.1)
		npc.set_meta("field_idle_previous_time", time)
		FieldActorVisuals.update_grounding(grounding, Vector2.ZERO, phase * 0.35, false, idle_delta)

## 캠프파이어 글로우 업데이트 (인터랙티브 프롭용, _process에서 호출)
static func update_campfire_glows(map: Node2D, time: float) -> void:
	for child in map.get_children():
		if child is Area2D:
			for sub in child.get_children():
				if sub is ColorRect and sub.has_meta("campfire_glow"):
					var phase: float = sub.get_meta("phase", 0.0)
					sub.color.a = 0.04 + sin(time * 2.5 + phase) * 0.025

## 색상 유틸
static func _darken_c(color: Color, amount: float) -> Color:
	return Color(maxf(color.r - amount, 0), maxf(color.g - amount, 0), maxf(color.b - amount, 0), color.a)
