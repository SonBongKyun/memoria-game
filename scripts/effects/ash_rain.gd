## AshRain — 재비 파티클 이펙트
## 회색 반투명 플레이크가 느리게 내려오는 환경 효과.
## Player 노드에 부착하여 카메라를 따라다님.
extends GPUParticles2D

func _ready() -> void:
	_setup_particles()
	print("[AshRain] Ash rain started")

func _setup_particles() -> void:
	if OptionsMenu.is_clean_gameplay_visuals():
		amount = 1
		emitting = false
		visible = false
		return
	amount = 18
	lifetime = 4.0
	emitting = true
	z_index = 5

	# 방출 영역: 화면 너비만큼 위에서
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 1, 0)  # 아래로
	mat.spread = 15.0  # 약간의 퍼짐
	mat.initial_velocity_min = 15.0
	mat.initial_velocity_max = 25.0
	mat.gravity = Vector3(0, 8, 0)  # 약한 중력

	# 방출 영역 (카메라 뷰포트 기준 넓은 영역)
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(400, 10, 0)

	# 좌우 흔들림
	mat.turbulence_enabled = true
	mat.turbulence_noise_strength = 1.2
	mat.turbulence_noise_speed_random = 0.5
	mat.turbulence_noise_scale = 4.0

	# 크기
	mat.scale_min = 0.5
	mat.scale_max = 1.0

	# 색상: 회색 반투명, 서서히 사라짐
	mat.color = Color(0.6, 0.58, 0.55, 0.24)
	var color_ramp = GradientTexture1D.new()
	var grad = Gradient.new()
	grad.set_color(0, Color(0.65, 0.6, 0.55, 0.28))
	grad.set_color(1, Color(0.5, 0.48, 0.45, 0.0))
	color_ramp.gradient = grad
	mat.color_ramp = color_ramp

	process_material = mat

	# 위치: 플레이어 위 200px
	position = Vector2(0, -200)

	# 텍스처: 작은 사각형 (코드 생성)
	var img = Image.create(4, 4, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.7, 0.67, 0.62, 0.6))
	var tex = ImageTexture.create_from_image(img)
	texture = tex

## 강도 조절 (스토리 진행에 따라)
func set_intensity(level: float) -> void:
	amount = int(8 + 18 * clampf(level, 0.0, 1.0))

## 켜기/끄기
func start_rain() -> void:
	emitting = not OptionsMenu.is_clean_gameplay_visuals()
	visible = emitting

func stop_rain() -> void:
	emitting = false
