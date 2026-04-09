## MapEffects — 맵 환경 효과 유틸리티
## 물 반짝임, 랜턴 빛, 보이드 파티클 등.
class_name MapEffects

const TILE: int = 32

# S41: 셰이더 캐시 (한번 로드한 셰이더 재사용)
static var _shader_cache: Dictionary = {}

static func _get_shader(path: String) -> Shader:
	if _shader_cache.has(path):
		return _shader_cache[path]
	if ResourceLoader.exists(path):
		var shader = load(path) as Shader
		_shader_cache[path] = shader
		return shader
	return null

## 물 타일 반짝임 효과 추가 — 셰이더 기반 물 왜곡 (S40)
## parent에 추가된 ColorRect들을 반환 (caller가 _process에서 업데이트)
static func add_water_shimmer(parent: Node2D, map_data: Array, width: int, height: int, water_index: int) -> Array[ColorRect]:
	var shimmers: Array[ColorRect] = []
	var shader_path = "res://assets/shaders/water_distortion.gdshader"
	var shader_res = _get_shader(shader_path)
	var has_shader = shader_res != null

	# S41: 물 반짝임 최적화 — 5타일마다 1개로 줄여 ColorRect 수 감소
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

	# 셰이더 기반 물 오버레이 — 넓은 물 영역에 왜곡 효과
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
				# 연속 물 구간 발견 — 오버레이 배치
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

## 랜턴 빛 효과 추가 — 셰이더 글로우 (S40)
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
static func add_void_particles(parent: Node2D) -> GPUParticles2D:
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()

	mat.direction = Vector3(0, -0.3, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 5.0
	mat.initial_velocity_max = 15.0
	mat.gravity = Vector3(0, -5, 0)  # 위로 떠오르는 느낌
	mat.scale_min = 1.0
	mat.scale_max = 2.5
	mat.color = Color(0.3, 0.1, 0.5, 0.4)

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.3, 0.1, 0.5, 0.0))
	g.set_offset(0, 0.0)
	g.add_point(0.3, Color(0.4, 0.15, 0.6, 0.4))
	g.add_point(0.7, Color(0.3, 0.1, 0.5, 0.3))
	g.set_color(1, Color(0.2, 0.05, 0.3, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(320, 320, 0)

	particles.process_material = mat
	particles.amount = 25
	particles.lifetime = 5.0
	particles.visibility_rect = Rect2(-400, -400, 800, 800)

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

## 맵 비네트 오버레이 — 셰이더 기반 원형 비네트 (S40)
static func add_vignette(parent: Node, intensity: float = 0.4) -> CanvasLayer:
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

## 안개 효과 (CanvasLayer 기반 — 카메라 독립)
static func add_fog(parent: Node, color: Color = Color(0.2, 0.2, 0.25, 0.08)) -> Array[ColorRect]:
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
	var layer = CanvasLayer.new()
	layer.layer = 4  # 비네트(3) 위, UI 아래

	# 풀스크린 어둠 배경
	var bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0, 0, 0, 0.85)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(bg)

	# 중앙 컨테이너
	var container = VBoxContainer.new()
	container.set_anchors_preset(Control.PRESET_CENTER)
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

	# 초기 투명 — CanvasLayer에는 modulate가 없으므로 bg+container를 조작
	bg.modulate = Color(1, 1, 1, 0)
	container.modulate = Color(1, 1, 1, 0)
	parent.add_child(layer)

	# 애니메이션: 페이드 인 0.5s → 홀드 2.0s → 페이드 아웃 0.8s → 제거
	var tween = parent.create_tween()
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, 0.5)
	tween.tween_property(container, "modulate:a", 1.0, 0.5)
	tween.set_parallel(false)
	tween.tween_interval(2.0)
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 0.0, 0.8)
	tween.tween_property(container, "modulate:a", 0.0, 0.8)
	tween.set_parallel(false)
	tween.tween_callback(layer.queue_free)

	# await용: 트윈 완료까지 대기 가능
	await tween.finished

	return layer

## ===================== 날씨 효과 =====================

## 비 효과 (CanvasLayer 기반)
static func add_rain(parent: Node, intensity: float = 1.0, color: Color = Color(0.6, 0.65, 0.8, 0.3)) -> CanvasLayer:
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
	modulate.color = ambient_color
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
	# 하늘 그라디언트 — 위는 밝게, 아래는 어둡게
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

## 대기 열 왜곡 (해안/시장 맵용 — BackBufferCopy + 셰이더)
static func add_heat_haze(parent: Node, strength: float = 0.003) -> CanvasLayer:
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

## 동적 라이트 펄스 — CanvasModulate 색상을 시간에 따라 미세 변화
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

## 색상 유틸
static func _darken_c(color: Color, amount: float) -> Color:
	return Color(maxf(color.r - amount, 0), maxf(color.g - amount, 0), maxf(color.b - amount, 0), color.a)
