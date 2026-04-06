## MapEffects — 맵 환경 효과 유틸리티
## 물 반짝임, 랜턴 빛, 보이드 파티클 등.
class_name MapEffects

const TILE: int = 32

## 물 타일 반짝임 효과 추가
## parent에 추가된 ColorRect들을 반환 (caller가 _process에서 업데이트)
static func add_water_shimmer(parent: Node2D, map_data: Array, width: int, height: int, water_index: int) -> Array[ColorRect]:
	var shimmers: Array[ColorRect] = []
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == water_index:
					# 간헐적으로만 추가 (성능)
					if (x + y) % 3 == 0:
						var rect = ColorRect.new()
						rect.size = Vector2(TILE, 2)
						rect.position = Vector2(x * TILE, y * TILE + randi_range(4, 28))
						rect.color = Color(0.4, 0.6, 0.8, 0.0)
						rect.z_index = 0
						rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
						# 메타에 위상 저장 (각각 다른 타이밍)
						rect.set_meta("phase", randf() * TAU)
						parent.add_child(rect)
						shimmers.append(rect)
	return shimmers

## 물 반짝임 업데이트 (_process에서 호출)
static func update_water_shimmer(shimmers: Array[ColorRect], time: float) -> void:
	for rect in shimmers:
		if is_instance_valid(rect):
			var phase = rect.get_meta("phase", 0.0)
			var alpha = (sin(time * 1.5 + phase) + 1.0) * 0.15
			rect.color.a = alpha

## 랜턴 빛 효과 추가
static func add_lantern_lights(parent: Node2D, map_data: Array, width: int, height: int, lantern_index: int) -> Array[ColorRect]:
	var lights: Array[ColorRect] = []
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				if map_data[y][x] == lantern_index:
					# 따뜻한 원형 빛 (큰 반투명 사각형)
					var size = TILE * 3
					var rect = ColorRect.new()
					rect.size = Vector2(size, size)
					rect.position = Vector2(x * TILE - TILE, y * TILE - TILE)
					rect.color = Color(0.9, 0.7, 0.3, 0.08)
					rect.z_index = 1
					rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
					rect.set_meta("phase", randf() * TAU)
					parent.add_child(rect)
					lights.append(rect)
	return lights

## 랜턴 빛 업데이트 (_process에서 호출)
static func update_lantern_lights(lights: Array[ColorRect], time: float) -> void:
	for rect in lights:
		if is_instance_valid(rect):
			var phase = rect.get_meta("phase", 0.0)
			# 약한 깜빡임 (촛불 느낌)
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
	particles.amount = 20
	particles.lifetime = 5.0
	particles.visibility_rect = Rect2(-400, -400, 800, 800)

	parent.add_child(particles)
	return particles

## 맵 비네트 오버레이 (CanvasLayer 기반 — 카메라 독립)
static func add_vignette(parent: Node, intensity: float = 0.4) -> CanvasLayer:
	var layer = CanvasLayer.new()
	layer.layer = 3  # 맵 위, UI 아래

	var container = Control.new()
	container.set_anchors_preset(Control.PRESET_FULL_RECT)
	container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(container)

	# 상단 그라데이션
	var top = ColorRect.new()
	top.set_anchors_preset(Control.PRESET_TOP_WIDE)
	top.offset_bottom = 100
	top.color = Color(0, 0, 0, intensity)
	top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(top)

	# 하단 그라데이션
	var bottom = ColorRect.new()
	bottom.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	bottom.offset_top = -80
	bottom.color = Color(0, 0, 0, intensity * 0.7)
	bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(bottom)

	# 좌측
	var left = ColorRect.new()
	left.set_anchors_preset(Control.PRESET_LEFT_WIDE)
	left.offset_right = 60
	left.color = Color(0, 0, 0, intensity * 0.5)
	left.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(left)

	# 우측
	var right = ColorRect.new()
	right.set_anchors_preset(Control.PRESET_RIGHT_WIDE)
	right.offset_left = -60
	right.color = Color(0, 0, 0, intensity * 0.5)
	right.mouse_filter = Control.MOUSE_FILTER_IGNORE
	container.add_child(right)

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
