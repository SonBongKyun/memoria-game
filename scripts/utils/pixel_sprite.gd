## PixelSprite — 픽셀아트 캐릭터 스프라이트 생성 유틸리티
## Image.set_pixel()로 상세한 픽셀아트 캐릭터를 생성.
## S42: 48x48 프레임으로 업그레이드, 디테일 대폭 강화.
## 4방향 x (idle + 4 walk) 애니메이션.
class_name PixelSprite

const SIZE: int = 48
const HALF: int = 24

## SpriteFrames 생성 (4방향 idle + walk)
static func create_frames(config: Dictionary) -> SpriteFrames:
	var frames = SpriteFrames.new()

	var directions = ["down", "up", "left", "right"]
	for dir in directions:
		var idle_name = "idle_" + dir
		frames.add_animation(idle_name)
		frames.set_animation_speed(idle_name, 1)
		frames.set_animation_loop(idle_name, true)
		var idle_img = _draw_character(config, dir, 0)
		frames.add_frame(idle_name, ImageTexture.create_from_image(idle_img))

		var walk_name = "walk_" + dir
		frames.add_animation(walk_name)
		frames.set_animation_speed(walk_name, 8)
		frames.set_animation_loop(walk_name, true)
		for step in range(4):
			var walk_img = _draw_character(config, dir, step + 1)
			frames.add_frame(walk_name, ImageTexture.create_from_image(walk_img))

	if frames.has_animation("default"):
		frames.remove_animation("default")

	return frames

## 단일 프레임 그리기
static func _draw_character(config: Dictionary, direction: String, walk_frame: int) -> Image:
	var img = Image.create(SIZE, SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	var bob_y: int = 0
	var step_offset: int = 0
	if walk_frame > 0:
		match walk_frame:
			1: bob_y = 0; step_offset = -1
			2: bob_y = -1; step_offset = 0
			3: bob_y = 0; step_offset = 1
			4: bob_y = -1; step_offset = 0

	var base_y: int = 3 + bob_y

	match direction:
		"down":
			_draw_front(img, config, base_y, walk_frame, step_offset)
		"up":
			_draw_back(img, config, base_y, walk_frame, step_offset)
		"left":
			_draw_side(img, config, base_y, walk_frame, step_offset, false)
		"right":
			_draw_side(img, config, base_y, walk_frame, step_offset, true)

	# S43: 1px 아웃라인 추가 (캐릭터가 배경에서 확실히 분리됨)
	_add_outline(img, Color(0.03, 0.02, 0.05, 0.9))

	return img

## ========== 정면 (down) ==========
static func _draw_front(img: Image, c: Dictionary, by: int, walk: int, step: int) -> void:
	var skin = c.get("skin", Color(0.85, 0.72, 0.6))
	var hair = c.get("hair", Color(0.25, 0.28, 0.4))
	var coat = c.get("coat", Color(0.2, 0.22, 0.35))
	var shirt = c.get("shirt", Color(0.3, 0.28, 0.25))
	var pants = c.get("pants", Color(0.18, 0.17, 0.2))
	var boots = c.get("boots", Color(0.12, 0.1, 0.1))
	var eye = c.get("eye", Color(0.3, 0.5, 0.7))
	var hair_style = c.get("hair_style", "medium")
	var acc_type = c.get("accessory_type", "none")
	var acc_color = c.get("accessory", Color(0.7, 0.55, 0.2))

	var sk_s = _darken(skin, 0.15)
	var sk_h = _lighten(skin, 0.08)
	var h_s = _darken(hair, 0.12)
	var h_h = _lighten(hair, 0.1)
	var c_s = _darken(coat, 0.12)
	var c_h = _lighten(coat, 0.08)
	var p_s = _darken(pants, 0.1)
	var b_h = _lighten(boots, 0.12)

	var hx = 17; var hy = by

	# ── 머리카락 (윗부분) ──
	_fill_rect(img, hx, hy, 12, 3, hair)
	_fill_rect(img, hx - 1, hy + 1, 14, 2, hair)
	_fill_rect(img, hx - 2, hy + 2, 16, 1, hair)
	# 머리카락 하이라이트
	_fill_rect(img, hx + 2, hy, 3, 1, h_h)
	_set_px(img, hx + 7, hy + 1, h_h)
	if hair_style == "long":
		_fill_rect(img, hx - 2, hy, 16, 3, hair)
		_fill_rect(img, hx - 3, hy + 2, 18, 1, hair)
	# 머리카락 프레임 (얼굴 양쪽)
	_fill_rect(img, hx - 2, hy + 3, 2, 6, hair)
	_fill_rect(img, hx + 12, hy + 3, 2, 6, hair)
	if hair_style != "short":
		_fill_rect(img, hx - 2, hy + 9, 2, 3, h_s)
		_fill_rect(img, hx + 12, hy + 9, 2, 3, h_s)
	# 머리카락 세부 텍스처
	_set_px(img, hx + 3, hy + 1, h_s)
	_set_px(img, hx + 8, hy, h_s)

	# ── 얼굴 ──
	var fy = hy + 3
	_fill_rect(img, hx, fy, 12, 8, skin)
	# 눈 (2x2 크기, 하이라이트 포함)
	_fill_rect(img, hx + 2, fy + 3, 2, 2, eye)
	_fill_rect(img, hx + 8, fy + 3, 2, 2, eye)
	# 눈 하이라이트 (생기있는 흰 점)
	_set_px(img, hx + 2, fy + 3, Color(0.95, 0.97, 1.0))
	_set_px(img, hx + 8, fy + 3, Color(0.95, 0.97, 1.0))
	# 눈동자 (검은 점)
	_set_px(img, hx + 3, fy + 4, _darken(eye, 0.3))
	_set_px(img, hx + 9, fy + 4, _darken(eye, 0.3))
	# 눈썹
	_fill_rect(img, hx + 2, fy + 2, 2, 1, h_s)
	_fill_rect(img, hx + 8, fy + 2, 2, 1, h_s)
	# 코
	_set_px(img, hx + 5, fy + 5, sk_s)
	_set_px(img, hx + 6, fy + 5, sk_s)
	# 입
	_fill_rect(img, hx + 4, fy + 6, 4, 1, _darken(skin, 0.25))
	_set_px(img, hx + 5, fy + 7, _darken(skin, 0.15))  # 아래입술 힌트
	# 볼 하이라이트
	_set_px(img, hx + 1, fy + 5, sk_h)
	_set_px(img, hx + 10, fy + 5, sk_h)
	# 얼굴 그림자 (양 옆)
	_fill_rect(img, hx, fy, 1, 8, sk_s)
	_fill_rect(img, hx + 11, fy, 1, 8, sk_s)

	# ── 목 ──
	var ny = hy + 11
	_fill_rect(img, hx + 3, ny, 6, 2, skin)
	_set_px(img, hx + 3, ny, sk_s)
	_set_px(img, hx + 8, ny, sk_s)

	# ── 상체 (코트/셔츠) ──
	var ty = ny + 2
	# 어깨
	_fill_rect(img, hx - 3, ty, 18, 3, coat)
	_fill_rect(img, hx - 3, ty, 2, 3, c_s)  # 왼어깨 그림자
	_fill_rect(img, hx + 13, ty, 2, 3, c_s)  # 오른어깨 그림자
	# 코트 몸통
	_fill_rect(img, hx - 2, ty + 3, 16, 7, coat)
	# 코트 라펠/V라인 + 셔츠
	_fill_rect(img, hx + 4, ty, 4, 4, shirt)
	_set_px(img, hx + 3, ty + 1, shirt)
	_set_px(img, hx + 8, ty + 1, shirt)
	# 코트 주름 (디테일)
	_set_px(img, hx + 1, ty + 4, c_s)
	_set_px(img, hx + 2, ty + 6, c_s)
	_set_px(img, hx + 9, ty + 5, c_s)
	_set_px(img, hx + 10, ty + 3, c_s)
	# 코트 하이라이트
	_set_px(img, hx + 3, ty + 2, c_h)
	_set_px(img, hx + 7, ty + 4, c_h)
	# 코트 하단 테두리
	_fill_rect(img, hx - 2, ty + 9, 16, 1, c_s)
	# 벨트
	_fill_rect(img, hx, ty + 8, 12, 1, _darken(coat, 0.2))
	_set_px(img, hx + 5, ty + 8, acc_color)  # 버클

	# 액세서리
	if acc_type == "brooch":
		_fill_rect(img, hx + 5, ty + 1, 2, 2, acc_color)
		_set_px(img, hx + 5, ty + 1, _lighten(acc_color, 0.2))
	elif acc_type == "sword":
		_fill_rect(img, hx + 12, ty + 1, 2, 9, _darken(acc_color, 0.2))
		_fill_rect(img, hx + 11, ty, 4, 2, acc_color)  # 검자루
		_set_px(img, hx + 12, ty - 1, _lighten(acc_color, 0.15))
	elif acc_type == "scar":
		_set_px(img, hx + 1, fy + 3, _darken(skin, 0.3))
		_set_px(img, hx + 2, fy + 4, _darken(skin, 0.3))
		_set_px(img, hx + 3, fy + 5, _darken(skin, 0.25))

	# ── 팔 ──
	var ay = ty + 1
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = 2
			2: arm_swing = 0
			3: arm_swing = -2
			4: arm_swing = 0

	# 왼팔
	_fill_rect(img, hx - 5, ay + arm_swing, 3, 8, coat)
	_fill_rect(img, hx - 5, ay + arm_swing, 1, 8, c_s)
	_fill_rect(img, hx - 5, ay + 8 + arm_swing, 3, 2, skin)
	# 오른팔
	_fill_rect(img, hx + 14, ay - arm_swing, 3, 8, coat)
	_fill_rect(img, hx + 16, ay - arm_swing, 1, 8, c_s)
	_fill_rect(img, hx + 14, ay - arm_swing + 8, 3, 2, skin)

	# ── 하체 (바지) ──
	var py = ty + 10
	_fill_rect(img, hx, py, 12, 6, pants)
	# 다리 분리 + 그림자
	_fill_rect(img, hx + 5, py + 1, 2, 5, p_s)
	# 주름
	_set_px(img, hx + 2, py + 2, p_s)
	_set_px(img, hx + 9, py + 3, p_s)

	# ── 발 (부츠) ──
	var fy2 = py + 6
	_fill_rect(img, hx, fy2 + step, 5, 3, boots)
	_fill_rect(img, hx + 7, fy2 - step, 5, 3, boots)
	# 부츠 하이라이트
	_set_px(img, hx + 1, fy2 + step, b_h)
	_set_px(img, hx + 8, fy2 - step, b_h)
	# 부츠 상단 테두리
	_fill_rect(img, hx, fy2 + step, 5, 1, _darken(boots, 0.05))
	_fill_rect(img, hx + 7, fy2 - step, 5, 1, _darken(boots, 0.05))

## ========== 뒷면 (up) ==========
static func _draw_back(img: Image, c: Dictionary, by: int, walk: int, step: int) -> void:
	var hair = c.get("hair", Color(0.25, 0.28, 0.4))
	var coat = c.get("coat", Color(0.2, 0.22, 0.35))
	var pants = c.get("pants", Color(0.18, 0.17, 0.2))
	var boots = c.get("boots", Color(0.12, 0.1, 0.1))
	var hair_style = c.get("hair_style", "medium")
	var acc_type = c.get("accessory_type", "none")
	var acc_color = c.get("accessory", Color(0.7, 0.55, 0.2))
	var skin = c.get("skin", Color(0.85, 0.72, 0.6))

	var h_s = _darken(hair, 0.12)
	var h_h = _lighten(hair, 0.1)
	var c_s = _darken(coat, 0.12)
	var p_s = _darken(pants, 0.1)

	var hx = 17; var hy = by

	# ── 머리카락 (뒷면 — 더 많이 보임) ──
	_fill_rect(img, hx, hy, 12, 3, hair)
	_fill_rect(img, hx - 1, hy + 1, 14, 2, hair)
	_fill_rect(img, hx - 2, hy + 3, 16, 8, hair)
	# 머리카락 텍스처
	_set_px(img, hx + 1, hy + 4, h_s)
	_set_px(img, hx + 5, hy + 3, h_h)
	_set_px(img, hx + 8, hy + 5, h_s)
	_set_px(img, hx + 3, hy + 7, h_h)
	_set_px(img, hx + 10, hy + 6, h_s)

	if hair_style == "long":
		_fill_rect(img, hx - 2, hy + 11, 16, 4, hair)
		_fill_rect(img, hx - 1, hy + 15, 14, 1, h_s)
		_set_px(img, hx + 4, hy + 12, h_s)
		_set_px(img, hx + 9, hy + 13, h_h)
	elif hair_style == "medium":
		_fill_rect(img, hx - 1, hy + 11, 14, 2, hair)
		_set_px(img, hx + 5, hy + 11, h_s)

	# ── 목 ──
	var ny = hy + 11
	_fill_rect(img, hx + 3, ny, 6, 2, _darken(skin, 0.12))

	# ── 상체 (코트 뒷면) ──
	var ty = ny + 2
	_fill_rect(img, hx - 3, ty, 18, 3, coat)
	_fill_rect(img, hx - 2, ty + 3, 16, 7, coat)
	# 등 주름
	_fill_rect(img, hx + 5, ty + 2, 2, 6, c_s)
	_set_px(img, hx + 2, ty + 4, c_s)
	_set_px(img, hx + 9, ty + 3, c_s)
	# 벨트
	_fill_rect(img, hx, ty + 8, 12, 1, _darken(coat, 0.2))

	# 검 (등에 걸침)
	if acc_type == "sword":
		_fill_rect(img, hx + 3, ty - 3, 2, 12, _darken(acc_color, 0.2))
		_fill_rect(img, hx + 2, ty - 3, 4, 2, acc_color)
		_set_px(img, hx + 3, ty - 4, _lighten(acc_color, 0.1))

	# ── 팔 ──
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = 2
			3: arm_swing = -2
	_fill_rect(img, hx - 5, ty + 1 + arm_swing, 3, 8, coat)
	_fill_rect(img, hx + 14, ty + 1 - arm_swing, 3, 8, coat)

	# ── 하체 ──
	var py = ty + 10
	_fill_rect(img, hx, py, 12, 6, pants)
	_fill_rect(img, hx + 5, py + 1, 2, 5, p_s)

	# ── 발 ──
	var fy = py + 6
	_fill_rect(img, hx, fy + step, 5, 3, boots)
	_fill_rect(img, hx + 7, fy - step, 5, 3, boots)

## ========== 옆면 (left/right) ==========
static func _draw_side(img: Image, c: Dictionary, by: int, walk: int, step: int, flip: bool) -> void:
	var skin = c.get("skin", Color(0.85, 0.72, 0.6))
	var hair = c.get("hair", Color(0.25, 0.28, 0.4))
	var coat = c.get("coat", Color(0.2, 0.22, 0.35))
	var shirt = c.get("shirt", Color(0.3, 0.28, 0.25))
	var pants = c.get("pants", Color(0.18, 0.17, 0.2))
	var boots = c.get("boots", Color(0.12, 0.1, 0.1))
	var eye = c.get("eye", Color(0.3, 0.5, 0.7))
	var hair_style = c.get("hair_style", "medium")
	var acc_type = c.get("accessory_type", "none")
	var acc_color = c.get("accessory", Color(0.7, 0.55, 0.2))

	var sk_s = _darken(skin, 0.15)
	var h_s = _darken(hair, 0.12)
	var h_h = _lighten(hair, 0.1)
	var c_s = _darken(coat, 0.12)
	var p_s = _darken(pants, 0.1)

	var hx = 17; var hy = by

	# ── 머리카락 ──
	_fill_rect(img, hx, hy, 10, 3, hair)
	_fill_rect(img, hx - 1, hy + 1, 12, 2, hair)
	# 뒤쪽 머리
	_fill_rect(img, hx + 7, hy + 3, 4, 7, hair)
	_set_px(img, hx + 3, hy, h_h)
	if hair_style != "short":
		_fill_rect(img, hx + 7, hy + 10, 4, 3, h_s)
	if hair_style == "long":
		_fill_rect(img, hx + 7, hy + 13, 3, 3, h_s)
	# 앞머리
	_fill_rect(img, hx - 2, hy + 3, 3, 4, hair)
	_set_px(img, hx - 1, hy + 7, h_s)

	# ── 얼굴 (옆) ──
	var fy = hy + 3
	_fill_rect(img, hx, fy, 8, 8, skin)
	# 눈 (한쪽)
	_fill_rect(img, hx + 1, fy + 3, 2, 2, eye)
	_set_px(img, hx + 1, fy + 3, Color(0.95, 0.97, 1.0))
	_set_px(img, hx + 2, fy + 4, _darken(eye, 0.3))
	# 눈썹
	_fill_rect(img, hx + 1, fy + 2, 2, 1, h_s)
	# 코
	_set_px(img, hx - 1, fy + 5, skin)
	_set_px(img, hx - 1, fy + 4, sk_s)
	# 입
	_fill_rect(img, hx + 1, fy + 6, 3, 1, _darken(skin, 0.25))
	# 귀
	_fill_rect(img, hx + 7, fy + 3, 1, 2, sk_s)

	# ── 목 ──
	_fill_rect(img, hx + 3, hy + 11, 4, 2, skin)

	# ── 상체 ──
	var ty = hy + 13
	_fill_rect(img, hx - 2, ty, 14, 10, coat)
	_fill_rect(img, hx + 3, ty, 3, 3, shirt)
	# 코트 주름
	_set_px(img, hx + 1, ty + 4, c_s)
	_set_px(img, hx + 7, ty + 3, c_s)
	_set_px(img, hx + 4, ty + 6, c_s)
	# 벨트
	_fill_rect(img, hx, ty + 8, 10, 1, _darken(coat, 0.2))

	# 검 (옆에서 보이는)
	if acc_type == "sword":
		_fill_rect(img, hx + 10, ty - 3, 2, 12, _darken(acc_color, 0.2))
		_fill_rect(img, hx + 9, ty - 4, 4, 2, acc_color)
	if acc_type == "brooch":
		_fill_rect(img, hx + 3, ty + 1, 2, 2, acc_color)

	# ── 팔 (한 쪽만) ──
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = -2
			2: arm_swing = 0
			3: arm_swing = 2
			4: arm_swing = 0
	_fill_rect(img, hx - 4, ty + 2 + arm_swing, 3, 7, coat)
	_fill_rect(img, hx - 4, ty + 9 + arm_swing, 3, 2, skin)

	# ── 하체 ──
	var py = ty + 10
	_fill_rect(img, hx, py, 10, 6, pants)
	_fill_rect(img, hx + 4, py + 1, 2, 5, p_s)

	# ── 발 ──
	var fy2 = py + 6
	_fill_rect(img, hx, fy2 + step, 5, 3, boots)
	_fill_rect(img, hx + 5, fy2 - step, 5, 3, boots)

	if flip:
		_flip_horizontal(img)

## ========== 헬퍼 ==========

static func _fill_rect(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for py in range(h):
		for px in range(w):
			var fx = x + px
			var fy = y + py
			if fx >= 0 and fx < SIZE and fy >= 0 and fy < SIZE:
				img.set_pixel(fx, fy, color)

static func _set_px(img: Image, x: int, y: int, color: Color) -> void:
	if x >= 0 and x < SIZE and y >= 0 and y < SIZE:
		img.set_pixel(x, y, color)

static func _darken(color: Color, amount: float) -> Color:
	return Color(
		maxf(color.r - amount, 0),
		maxf(color.g - amount, 0),
		maxf(color.b - amount, 0),
		color.a
	)

static func _lighten(color: Color, amount: float) -> Color:
	return Color(
		minf(color.r + amount, 1),
		minf(color.g + amount, 1),
		minf(color.b + amount, 1),
		color.a
	)

## S43: 1px 아웃라인 — 불투명 픽셀 주변에 어두운 테두리 추가
static func _add_outline(img: Image, outline_color: Color) -> void:
	# 원본 알파를 먼저 복사
	var alpha_map: Array = []
	for y in range(SIZE):
		var row: Array = []
		for x in range(SIZE):
			row.append(img.get_pixel(x, y).a > 0.1)
		alpha_map.append(row)

	# 불투명 픽셀의 인접 빈 공간에 아웃라인 배치
	for y in range(SIZE):
		for x in range(SIZE):
			if alpha_map[y][x]:
				continue  # 이미 채워진 픽셀은 스킵
			# 상하좌우 + 대각선 검사
			var has_neighbor = false
			for dy in range(-1, 2):
				for dx in range(-1, 2):
					if dx == 0 and dy == 0:
						continue
					var nx = x + dx
					var ny = y + dy
					if nx >= 0 and nx < SIZE and ny >= 0 and ny < SIZE:
						if alpha_map[ny][nx]:
							has_neighbor = true
							break
				if has_neighbor:
					break
			if has_neighbor:
				img.set_pixel(x, y, outline_color)

static func _flip_horizontal(img: Image) -> void:
	var w = img.get_width()
	var h = img.get_height()
	for y in range(h):
		for x in range(int(w / 2.0)):
			var left = img.get_pixel(x, y)
			var right = img.get_pixel(w - 1 - x, y)
			img.set_pixel(x, y, right)
			img.set_pixel(w - 1 - x, y, left)

## ========== 프리셋 캐릭터 설정 ==========

## S43: 컬러 팔레트 리뉴얼 — 채도/대비 강화

static func arrel_config() -> Dictionary:
	return {
		"skin": Color(0.85, 0.72, 0.6),
		"hair": Color(0.4, 0.45, 0.65),  # 더 선명한 은청색
		"hair_style": "medium",
		"coat": Color(0.12, 0.15, 0.32),  # 더 진한 남색
		"shirt": Color(0.4, 0.32, 0.28),
		"pants": Color(0.1, 0.1, 0.16),
		"boots": Color(0.06, 0.05, 0.08),
		"eye": Color(0.2, 0.5, 0.8),  # 더 밝은 파랑
		"accessory": Color(0.55, 0.5, 0.42),
		"accessory_type": "sword",
	}

static func elia_config() -> Dictionary:
	return {
		"skin": Color(0.9, 0.8, 0.7),
		"hair": Color(0.78, 0.8, 0.88),  # 더 밝은 은발
		"hair_style": "long",
		"coat": Color(0.4, 0.3, 0.2),  # 더 따뜻한 갈색
		"shirt": Color(0.6, 0.55, 0.48),
		"pants": Color(0.28, 0.24, 0.2),
		"boots": Color(0.16, 0.13, 0.1),
		"eye": Color(0.25, 0.55, 0.9),  # 더 선명한 파랑
		"accessory": Color(0.85, 0.65, 0.2),  # 더 밝은 금색
		"accessory_type": "brooch",
	}

static func sable_config() -> Dictionary:
	return {
		"skin": Color(0.6, 0.48, 0.38),
		"hair": Color(0.1, 0.08, 0.1),
		"hair_style": "short",
		"coat": Color(0.18, 0.16, 0.22),  # 어두운 보라 톤
		"shirt": Color(0.28, 0.22, 0.26),
		"pants": Color(0.12, 0.11, 0.15),
		"boots": Color(0.08, 0.06, 0.08),
		"eye": Color(0.55, 0.4, 0.2),  # 더 강한 호박색
		"accessory": Color(0.45, 0.35, 0.3),
		"accessory_type": "scar",
	}

static func npc_config(base_color: Color) -> Dictionary:
	return {
		"skin": Color(0.82, 0.72, 0.62),
		"hair": _darken(base_color, 0.08),
		"hair_style": "short",
		"coat": base_color,
		"shirt": _lighten(base_color, 0.12),
		"pants": _darken(base_color, 0.12),
		"boots": Color(0.1, 0.08, 0.1),
		"eye": Color(0.35, 0.3, 0.25),
		"accessory_type": "none",
	}

## ========== S44: 전투 전용 대형 캐릭터 스프라이트 (128x128) ==========

const BATTLE_SIZE: int = 128
const BH: int = 64  # half

## 전투용 캐릭터 스프라이트 생성 (128x128, 사이드뷰 전투 포즈)
static func create_battle_sprite(who: String) -> ImageTexture:
	var config: Dictionary
	match who:
		"arrel": config = arrel_config()
		"elia": config = elia_config()
		"sable": config = sable_config()
		_: config = arrel_config()
	var img = Image.create(BATTLE_SIZE, BATTLE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	_draw_battle_character(img, config, who)
	_add_outline_n(img, BATTLE_SIZE, Color(0.03, 0.02, 0.05, 0.92))
	return ImageTexture.create_from_image(img)

## 전투용 대형 적 스프라이트 (128x128)
static func create_battle_enemy(enemy_type: String) -> ImageTexture:
	var img = Image.create(BATTLE_SIZE, BATTLE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	match enemy_type.to_lower():
		"void_beast": _draw_battle_void_beast(img)
		"shadow_wisp": _draw_battle_shadow_wisp(img)
		"memory_eater": _draw_battle_memory_eater(img)
		"shade_sentinel": _draw_battle_shade_sentinel(img)
		"void_stalker": _draw_battle_void_stalker(img)
		_: _draw_battle_generic_enemy(img, enemy_type)
	_add_outline_n(img, BATTLE_SIZE, Color(0.02, 0.01, 0.04, 0.95))
	return ImageTexture.create_from_image(img)

## N사이즈 아웃라인 (128x128 등 임의 크기)
static func _add_outline_n(img: Image, s: int, outline_color: Color) -> void:
	var alpha_map: Array = []
	for y in range(s):
		var row: Array = []
		for x in range(s):
			row.append(img.get_pixel(x, y).a > 0.1)
		alpha_map.append(row)
	for y in range(s):
		for x in range(s):
			if alpha_map[y][x]:
				continue
			var has_neighbor = false
			for dy in range(-1, 2):
				for dx in range(-1, 2):
					if dx == 0 and dy == 0: continue
					var nx = x + dx; var ny = y + dy
					if nx >= 0 and nx < s and ny >= 0 and ny < s:
						if alpha_map[ny][nx]:
							has_neighbor = true; break
				if has_neighbor: break
			if has_neighbor:
				img.set_pixel(x, y, outline_color)

static func _bpx(img: Image, x: int, y: int, color: Color) -> void:
	if x >= 0 and x < BATTLE_SIZE and y >= 0 and y < BATTLE_SIZE:
		img.set_pixel(x, y, color)

static func _bfill(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for py in range(h):
		for px in range(w):
			_bpx(img, x + px, y + py, color)

## 타원 채우기
static func _bellipse(img: Image, cx: int, cy: int, rx: int, ry: int, color: Color) -> void:
	for y in range(cy - ry, cy + ry + 1):
		for x in range(cx - rx, cx + rx + 1):
			var dx = float(x - cx) / float(rx)
			var dy = float(y - cy) / float(ry)
			if dx * dx + dy * dy <= 1.0:
				_bpx(img, x, y, color)

## 전투 캐릭터 그리기 (128x128 사이드뷰)
static func _draw_battle_character(img: Image, c: Dictionary, who: String) -> void:
	var skin = c.get("skin", Color(0.85, 0.72, 0.6))
	var hair = c.get("hair", Color(0.25, 0.28, 0.4))
	var coat = c.get("coat", Color(0.2, 0.22, 0.35))
	var shirt = c.get("shirt", Color(0.3, 0.28, 0.25))
	var pants = c.get("pants", Color(0.18, 0.17, 0.2))
	var boots = c.get("boots", Color(0.12, 0.1, 0.1))
	var eye = c.get("eye", Color(0.3, 0.5, 0.7))
	var hair_style = c.get("hair_style", "medium")
	var acc_color = c.get("accessory", Color(0.7, 0.55, 0.2))
	var acc_type = c.get("accessory_type", "none")

	var sk_s = _darken(skin, 0.12)
	var sk_h = _lighten(skin, 0.06)
	var h_s = _darken(hair, 0.1)
	var h_h = _lighten(hair, 0.08)
	var c_s = _darken(coat, 0.1)
	var c_h = _lighten(coat, 0.06)
	var p_s = _darken(pants, 0.08)
	var b_h = _lighten(boots, 0.1)

	# 사이드뷰: 오른쪽을 향해 서있는 포즈 (왼쪽 배치)
	var bx: int = 30  # 기본 x 오프셋
	var by: int = 6   # 기본 y 오프셋

	# ── 머리카락 ──
	_bfill(img, bx + 24, by, 28, 7, hair)
	_bfill(img, bx + 22, by + 3, 32, 5, hair)
	_bfill(img, bx + 20, by + 6, 34, 3, hair)
	# 머리카락 하이라이트
	_bfill(img, bx + 28, by + 1, 8, 2, h_h)
	_bfill(img, bx + 38, by + 3, 4, 2, h_h)
	# 머리카락 그림자
	_bfill(img, bx + 24, by + 5, 6, 2, h_s)
	# 긴 머리 (elia)
	if hair_style == "long":
		_bfill(img, bx + 18, by, 36, 6, hair)
		_bfill(img, bx + 16, by + 5, 38, 3, hair)
		# 뒤로 흘러내리는 머리카락
		_bfill(img, bx + 18, by + 9, 6, 18, hair)
		_bfill(img, bx + 16, by + 12, 5, 22, h_s)
		_bfill(img, bx + 19, by + 20, 4, 16, h_h)
	# 짧은 머리 (sable)
	if hair_style == "short":
		_bfill(img, bx + 26, by + 1, 24, 5, hair)
		_bfill(img, bx + 24, by + 4, 28, 3, hair)
	# 옆머리 프레임
	_bfill(img, bx + 20, by + 8, 4, 12, hair)
	_bfill(img, bx + 50, by + 8, 4, 10, hair)

	# ── 얼굴 ──
	var fy = by + 8
	_bfill(img, bx + 24, fy, 28, 18, skin)
	# 얼굴 그림자 (왼쪽)
	_bfill(img, bx + 24, fy, 3, 18, sk_s)
	# 얼굴 하이라이트 (오른쪽)
	_bfill(img, bx + 48, fy + 3, 3, 8, sk_h)
	# 눈 (사이드뷰 — 오른쪽 바라보는 방향, 앞쪽 눈 크게)
	# 앞쪽 눈 (4x4)
	_bfill(img, bx + 40, fy + 6, 5, 5, Color(0.95, 0.97, 1.0))  # 흰자
	_bfill(img, bx + 41, fy + 7, 3, 3, eye)  # 홍채
	_bfill(img, bx + 42, fy + 8, 2, 2, _darken(eye, 0.25))  # 동공
	_bpx(img, bx + 41, fy + 7, Color(1, 1, 1, 0.9))  # 하이라이트
	# 뒷쪽 눈 (3x4, 살짝 가려짐)
	_bfill(img, bx + 30, fy + 7, 4, 4, Color(0.93, 0.95, 0.98))
	_bfill(img, bx + 31, fy + 8, 2, 2, eye)
	_bpx(img, bx + 31, fy + 8, Color(1, 1, 1, 0.8))
	# 눈썹
	_bfill(img, bx + 39, fy + 4, 6, 2, h_s)
	_bfill(img, bx + 30, fy + 5, 4, 2, h_s)
	# 코
	_bfill(img, bx + 46, fy + 11, 4, 3, sk_s)
	_bpx(img, bx + 49, fy + 11, sk_h)
	# 입
	_bfill(img, bx + 40, fy + 15, 6, 2, _darken(skin, 0.2))
	_bpx(img, bx + 42, fy + 16, _darken(skin, 0.12))
	# 귀
	_bfill(img, bx + 22, fy + 6, 3, 5, skin)
	_bpx(img, bx + 23, fy + 7, sk_s)

	# ── 목 ──
	var ny = fy + 18
	_bfill(img, bx + 32, ny, 12, 4, skin)
	_bfill(img, bx + 32, ny, 3, 4, sk_s)

	# ── 상체 ──
	var ty = ny + 4
	# 어깨
	_bfill(img, bx + 18, ty, 40, 6, coat)
	_bfill(img, bx + 18, ty, 4, 6, c_s)
	_bfill(img, bx + 54, ty, 4, 6, c_h)
	# 코트 몸통
	_bfill(img, bx + 22, ty + 6, 32, 16, coat)
	# V라인 셔츠
	_bfill(img, bx + 34, ty, 8, 10, shirt)
	_bfill(img, bx + 33, ty + 2, 2, 6, shirt)
	_bfill(img, bx + 41, ty + 2, 2, 6, shirt)
	# 코트 주름 디테일
	_bfill(img, bx + 26, ty + 8, 2, 6, c_s)
	_bfill(img, bx + 44, ty + 10, 2, 4, c_s)
	_bfill(img, bx + 30, ty + 14, 16, 2, c_s)
	# 코트 하이라이트
	_bfill(img, bx + 36, ty + 4, 3, 3, c_h)
	_bfill(img, bx + 48, ty + 6, 2, 4, c_h)
	# 벨트
	_bfill(img, bx + 22, ty + 20, 32, 3, _darken(coat, 0.15))
	_bfill(img, bx + 36, ty + 20, 4, 3, acc_color)  # 버클

	# ── 팔 ──
	var ay = ty + 3
	# 뒷팔 (왼쪽, 약간 뒤로)
	_bfill(img, bx + 14, ay, 6, 18, coat)
	_bfill(img, bx + 14, ay, 2, 18, c_s)
	_bfill(img, bx + 14, ay + 18, 6, 4, skin)

	# 앞팔 (오른쪽) — 전투 포즈: 앞으로 뻗은 자세
	if who == "arrel":
		# 검을 들고 있는 팔
		_bfill(img, bx + 56, ay - 2, 6, 16, coat)
		_bfill(img, bx + 58, ay, 2, 16, c_h)
		_bfill(img, bx + 56, ay + 14, 6, 4, skin)
		# 검 (긴 직선, 위에서 아래로)
		_bfill(img, bx + 58, ay - 18, 3, 36, Color(0.6, 0.62, 0.68))  # 검신
		_bfill(img, bx + 59, ay - 18, 1, 36, Color(0.75, 0.78, 0.85))  # 하이라이트
		_bfill(img, bx + 56, ay + 12, 8, 3, acc_color)  # 검자루 가드
		_bfill(img, bx + 57, ay + 14, 6, 5, _darken(acc_color, 0.15))  # 그립
	elif who == "elia":
		# 손을 앞에 모은 자세 (기도/힐)
		_bfill(img, bx + 52, ay + 2, 6, 14, coat)
		_bfill(img, bx + 54, ay + 4, 2, 12, c_h)
		_bfill(img, bx + 52, ay + 16, 6, 4, skin)
		# 브로치 빛
		_bfill(img, bx + 36, ty + 1, 3, 3, acc_color)
		_bpx(img, bx + 37, ty + 1, _lighten(acc_color, 0.25))
	else:
		# 세이블: 주먹 쥔 자세
		_bfill(img, bx + 54, ay, 6, 16, coat)
		_bfill(img, bx + 56, ay + 2, 2, 14, c_h)
		_bfill(img, bx + 54, ay + 16, 7, 5, skin)
		_bfill(img, bx + 55, ay + 17, 5, 3, sk_s)

	# ── 하체 ──
	var py = ty + 23
	# 전투 자세: 다리 약간 벌림
	# 뒷다리
	_bfill(img, bx + 24, py, 12, 16, pants)
	_bfill(img, bx + 24, py, 3, 16, p_s)
	_bfill(img, bx + 30, py + 4, 2, 8, p_s)  # 주름
	# 앞다리
	_bfill(img, bx + 38, py, 12, 16, pants)
	_bfill(img, bx + 38, py, 3, 16, p_s)
	_bfill(img, bx + 44, py + 6, 2, 6, p_s)  # 주름

	# ── 부츠 ──
	var fty = py + 16
	_bfill(img, bx + 22, fty, 14, 6, boots)
	_bfill(img, bx + 22, fty + 4, 16, 3, boots)  # 발끝
	_bfill(img, bx + 24, fty, 4, 2, b_h)  # 하이라이트
	_bfill(img, bx + 36, fty, 14, 6, boots)
	_bfill(img, bx + 36, fty + 4, 16, 3, boots)
	_bfill(img, bx + 38, fty, 4, 2, b_h)

	# 흉터 (세이블)
	if acc_type == "scar":
		_bfill(img, bx + 26, fy + 6, 2, 3, _darken(skin, 0.25))
		_bfill(img, bx + 28, fy + 8, 2, 3, _darken(skin, 0.22))
		_bpx(img, bx + 30, fy + 10, _darken(skin, 0.18))

## ── 전투용 적 128x128 스프라이트 ──

static func _draw_battle_void_beast(img: Image) -> void:
	var body = Color(0.15, 0.08, 0.22)
	var body_h = Color(0.28, 0.14, 0.38)
	var body_s = Color(0.08, 0.04, 0.12)
	var eye_c = Color(0.85, 0.2, 0.95)
	var fang = Color(0.75, 0.75, 0.8)
	# 몸통 (큰 타원)
	_bellipse(img, 64, 68, 38, 26, body)
	_bellipse(img, 64, 64, 32, 20, body_h)
	# 등 텍스처 (갑각 패턴)
	for i in range(8):
		var sx = 40 + i * 7
		_bfill(img, sx, 50 + (i % 3) * 2, 4, 6, body_s)
	# 다리 4개 (두꺼운)
	for lx in [30, 48, 72, 90]:
		_bfill(img, lx, 84, 8, 24, body)
		_bfill(img, lx + 2, 84, 3, 24, body_h)
		# 발톱
		_bfill(img, lx - 1, 106, 10, 4, body_s)
	# 머리
	_bellipse(img, 64, 36, 22, 16, body_h)
	_bellipse(img, 64, 38, 18, 12, body)
	# 눈 (빛나는 보라)
	_bfill(img, 50, 30, 6, 6, eye_c)
	_bfill(img, 72, 30, 6, 6, eye_c)
	_bfill(img, 52, 32, 2, 2, Color(1, 1, 1, 0.85))
	_bfill(img, 74, 32, 2, 2, Color(1, 1, 1, 0.85))
	# 이빨
	_bfill(img, 54, 44, 4, 7, fang)
	_bfill(img, 70, 44, 4, 7, fang)
	_bfill(img, 60, 46, 3, 5, fang)
	# 보이드 연기 (위에 흩뿌림)
	for i in range(12):
		var sx = randi_range(30, 98)
		var sy = randi_range(14, 30)
		_bpx(img, sx, sy, Color(0.35, 0.12, 0.45, 0.3))
		_bpx(img, sx + 1, sy, Color(0.3, 0.1, 0.4, 0.2))
		_bpx(img, sx, sy + 1, Color(0.25, 0.08, 0.35, 0.15))

static func _draw_battle_shadow_wisp(img: Image) -> void:
	var body = Color(0.12, 0.1, 0.2)
	var glow = Color(0.45, 0.22, 0.65)
	var core = Color(0.6, 0.35, 0.85)
	# 몸통 (위에서 아래로 흐르는 유령형)
	for y in range(16, 110):
		var width_ratio = 1.0 - abs((y - 55.0) / 50.0)
		width_ratio = maxf(width_ratio, 0.15)
		var hw = int(24.0 * width_ratio)
		for x in range(64 - hw, 64 + hw):
			var alpha = 0.75 + randf_range(-0.1, 0.1)
			if y > 80: alpha *= 1.0 - (y - 80.0) / 30.0
			var c = body if (x + y) % 5 != 0 else glow
			_bpx(img, x, y, Color(c.r, c.g, c.b, alpha))
	# 코어 빛
	_bellipse(img, 64, 50, 8, 10, Color(core.r, core.g, core.b, 0.4))
	# 눈 (빛나는)
	_bfill(img, 50, 40, 6, 4, glow)
	_bfill(img, 72, 40, 6, 4, glow)
	_bfill(img, 52, 41, 2, 2, Color(0.85, 0.7, 1.0))
	_bfill(img, 74, 41, 2, 2, Color(0.85, 0.7, 1.0))
	# 떠다니는 입자
	for i in range(8):
		var px = randi_range(36, 92)
		var py = randi_range(20, 90)
		_bpx(img, px, py, Color(glow.r, glow.g, glow.b, 0.5))

static func _draw_battle_memory_eater(img: Image) -> void:
	var body = Color(0.22, 0.16, 0.13)
	var shell = Color(0.35, 0.25, 0.2)
	var shell_h = Color(0.42, 0.32, 0.25)
	var eye_c = Color(0.92, 0.72, 0.12)
	# 등딱지 (큰 타원)
	_bellipse(img, 64, 58, 36, 28, shell)
	_bellipse(img, 64, 55, 30, 22, shell_h)
	# 등딱지 무늬 (세로줄)
	_bfill(img, 62, 35, 4, 42, Color(shell_h.r + 0.06, shell_h.g + 0.04, shell_h.b + 0.02))
	_bfill(img, 48, 40, 3, 30, Color(shell.r + 0.04, shell.g + 0.02, shell.b))
	_bfill(img, 77, 40, 3, 30, Color(shell.r + 0.04, shell.g + 0.02, shell.b))
	# 다리 6개
	for i in range(3):
		var lx1 = 30 + i * 16
		_bfill(img, lx1, 78, 5, 24, body)
		_bfill(img, lx1 + 36, 78, 5, 24, body)
		# 관절
		_bfill(img, lx1, 90, 6, 3, _darken(body, 0.05))
	# 머리
	_bellipse(img, 64, 28, 18, 12, body)
	# 눈 (크고 빛남)
	_bfill(img, 52, 22, 6, 6, eye_c)
	_bfill(img, 70, 22, 6, 6, eye_c)
	_bfill(img, 54, 24, 2, 2, Color(1, 1, 0.8))
	_bfill(img, 72, 24, 2, 2, Color(1, 1, 0.8))
	# 턱 (집게)
	_bfill(img, 50, 34, 6, 10, Color(0.16, 0.12, 0.1))
	_bfill(img, 72, 34, 6, 10, Color(0.16, 0.12, 0.1))
	# 더듬이
	_bfill(img, 56, 16, 2, 8, body)
	_bfill(img, 70, 16, 2, 8, body)
	_bpx(img, 56, 15, eye_c)
	_bpx(img, 70, 15, eye_c)

static func _draw_battle_shade_sentinel(img: Image) -> void:
	var armor = Color(0.16, 0.13, 0.22)
	var armor_h = Color(0.28, 0.22, 0.38)
	var armor_s = Color(0.08, 0.06, 0.12)
	var eye_c = Color(0.92, 0.18, 0.32)
	var void_c = Color(0.45, 0.12, 0.55)
	# 몸통 (거대 갑옷)
	_bfill(img, 24, 34, 80, 64, armor)
	_bfill(img, 24, 34, 6, 64, armor_s)
	_bfill(img, 98, 34, 6, 64, armor_h)
	# 어깨 (과장된)
	_bfill(img, 10, 30, 22, 24, armor_h)
	_bfill(img, 96, 30, 22, 24, armor_h)
	# 어깨 스파이크
	_bfill(img, 14, 22, 8, 12, armor)
	_bfill(img, 106, 22, 8, 12, armor)
	_bfill(img, 16, 18, 4, 6, armor_h)
	_bfill(img, 108, 18, 4, 6, armor_h)
	# 가슴 디테일 (보이드 에너지)
	_bfill(img, 46, 52, 36, 4, void_c)
	_bellipse(img, 64, 62, 10, 10, Color(void_c.r, void_c.g, void_c.b, 0.5))
	_bellipse(img, 64, 62, 5, 5, Color(void_c.r + 0.2, void_c.g + 0.1, void_c.b + 0.2, 0.7))
	# 투구
	_bfill(img, 34, 6, 60, 30, armor_h)
	_bfill(img, 38, 2, 52, 8, armor)
	# 투구 디테일
	_bfill(img, 36, 32, 56, 4, armor_s)
	_bfill(img, 62, 2, 4, 32, armor_s)
	# 눈 (빨간 슬릿)
	_bfill(img, 44, 18, 12, 4, eye_c)
	_bfill(img, 72, 18, 12, 4, eye_c)
	_bfill(img, 46, 19, 4, 2, Color(1, 0.5, 0.5))
	_bfill(img, 74, 19, 4, 2, Color(1, 0.5, 0.5))
	# 다리
	_bfill(img, 34, 96, 18, 26, armor)
	_bfill(img, 76, 96, 18, 26, armor)
	_bfill(img, 36, 96, 4, 26, armor_h)
	_bfill(img, 78, 96, 4, 26, armor_h)
	# 검 (왼손)
	_bfill(img, 8, 10, 4, 80, Color(0.5, 0.5, 0.58))
	_bfill(img, 9, 10, 2, 80, Color(0.65, 0.65, 0.72))
	_bfill(img, 5, 86, 10, 4, acc_color_sentinel())
	# 보이드 에너지 줄기
	for i in range(6):
		var sx = randi_range(28, 100)
		for sy in range(randi_range(8, 24), randi_range(30, 50)):
			_bpx(img, sx, sy, Color(void_c.r, void_c.g, void_c.b, 0.2))

static func acc_color_sentinel() -> Color:
	return Color(0.5, 0.35, 0.28)

static func _draw_battle_void_stalker(img: Image) -> void:
	var body = Color(0.08, 0.06, 0.14)
	var body_h = Color(0.14, 0.1, 0.22)
	var glow = Color(0.55, 0.22, 0.75)
	# 몸통 (날씬한 인간형)
	for y in range(32, 106):
		var w = 16 if y < 65 else 12
		for x in range(64 - w, 64 + w):
			var c = body if (x + y) % 4 != 0 else body_h
			_bpx(img, x, y, c)
	# 팔 (길고 가는)
	_bfill(img, 28, 40, 6, 40, body)
	_bfill(img, 94, 40, 6, 40, body)
	_bfill(img, 30, 42, 2, 38, body_h)
	_bfill(img, 96, 42, 2, 38, body_h)
	# 발톱
	_bfill(img, 26, 78, 8, 5, glow)
	_bfill(img, 94, 78, 8, 5, glow)
	# 머리 (좁고 높음)
	_bellipse(img, 64, 20, 18, 16, body)
	_bellipse(img, 64, 18, 14, 12, body_h)
	# 뿔/가시
	_bfill(img, 48, 4, 4, 14, body)
	_bfill(img, 76, 4, 4, 14, body)
	_bpx(img, 49, 4, glow)
	_bpx(img, 77, 4, glow)
	# 눈 3개 (삼각 배열)
	_bfill(img, 52, 16, 5, 4, glow)
	_bfill(img, 61, 12, 5, 4, glow)
	_bfill(img, 71, 16, 5, 4, glow)
	_bfill(img, 53, 17, 2, 2, Color(0.85, 0.65, 1.0))
	_bfill(img, 62, 13, 2, 2, Color(0.85, 0.65, 1.0))
	_bfill(img, 72, 17, 2, 2, Color(0.85, 0.65, 1.0))
	# 다리
	_bfill(img, 50, 96, 6, 26, body)
	_bfill(img, 72, 96, 6, 26, body)
	_bfill(img, 52, 98, 2, 24, body_h)
	_bfill(img, 74, 98, 2, 24, body_h)
	# 꼬리
	for i in range(20):
		_bpx(img, 64 - i, 102 + i / 2, body_h)
		_bpx(img, 63 - i, 103 + i / 2, body)

static func _draw_battle_generic_enemy(img: Image, enemy_type: String) -> void:
	var hash_val = enemy_type.hash()
	var r = fmod(abs(float(hash_val)) * 0.000001, 0.4) + 0.1
	var g_val = fmod(abs(float(hash_val)) * 0.0000017, 0.3) + 0.05
	var b = fmod(abs(float(hash_val)) * 0.0000023, 0.4) + 0.1
	var body = Color(r, g_val, b)
	var body_h = _lighten(body, 0.08)
	var eye_c = Color(minf(r + 0.5, 1.0), g_val + 0.2, minf(b + 0.3, 1.0))
	# 기본 몸통
	_bellipse(img, 64, 60, 34, 38, body)
	_bellipse(img, 64, 56, 28, 30, body_h)
	# 텍스처
	for i in range(10):
		var sx = randi_range(36, 92)
		var sy = randi_range(30, 90)
		_bfill(img, sx, sy, 3, 3, _darken(body, 0.05))
	# 눈
	_bfill(img, 48, 42, 8, 6, eye_c)
	_bfill(img, 72, 42, 8, 6, eye_c)
	_bfill(img, 50, 44, 3, 3, Color(1, 1, 1, 0.8))
	_bfill(img, 74, 44, 3, 3, Color(1, 1, 1, 0.8))

## ========== S43: 전투 적 스프라이트 생성 ==========

## 적 몬스터 스프라이트 생성 (64x64)
static func create_enemy_sprite(enemy_type: String) -> ImageTexture:
	var img = Image.create(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))

	match enemy_type.to_lower():
		"void_beast":
			_draw_void_beast(img)
		"shadow_wisp":
			_draw_shadow_wisp(img)
		"memory_eater":
			_draw_memory_eater(img)
		"shade_sentinel":
			_draw_shade_sentinel(img)
		"void_stalker":
			_draw_void_stalker(img)
		_:
			_draw_generic_enemy(img, enemy_type)

	# 적에게도 아웃라인
	_add_outline_64(img, Color(0.02, 0.01, 0.04, 0.95))
	return ImageTexture.create_from_image(img)

## 보이드 비스트 — 네발짐승, 보라+검정
static func _draw_void_beast(img: Image) -> void:
	var body = Color(0.15, 0.08, 0.2)
	var body_h = Color(0.25, 0.12, 0.35)
	var eye_c = Color(0.8, 0.2, 0.9)
	# 몸통 (타원)
	for y in range(20, 50):
		for x in range(12, 52):
			var dx = (x - 32.0) / 20.0
			var dy = (y - 35.0) / 15.0
			if dx * dx + dy * dy < 1.0:
				var shade = randf_range(-0.02, 0.02)
				var c = body if (x + y) % 3 != 0 else body_h
				_px64(img, x, y, Color(c.r + shade, c.g + shade, c.b + shade, 1.0))
	# 다리 4개
	for lx in [18, 26, 38, 46]:
		for ly in range(42, 56):
			_px64(img, lx, ly, body)
			_px64(img, lx + 1, ly, body)
			_px64(img, lx + 2, ly, body)
	# 머리
	for y in range(12, 28):
		for x in range(20, 44):
			var dx = (x - 32.0) / 12.0
			var dy = (y - 20.0) / 8.0
			if dx * dx + dy * dy < 1.0:
				_px64(img, x, y, body_h)
	# 눈 (빛나는 보라)
	_fill64(img, 26, 18, 3, 3, eye_c)
	_fill64(img, 35, 18, 3, 3, eye_c)
	_px64(img, 27, 19, Color(1, 1, 1, 0.8))
	_px64(img, 36, 19, Color(1, 1, 1, 0.8))
	# 이빨
	_fill64(img, 28, 24, 2, 3, Color(0.7, 0.7, 0.75))
	_fill64(img, 34, 24, 2, 3, Color(0.7, 0.7, 0.75))
	# 보이드 연기
	for i in range(6):
		var sx = randi_range(15, 48)
		var sy = randi_range(8, 18)
		_px64(img, sx, sy, Color(0.3, 0.1, 0.4, 0.3))
		_px64(img, sx + 1, sy, Color(0.25, 0.08, 0.35, 0.2))

## 셰도우 위스프 — 떠다니는 유령
static func _draw_shadow_wisp(img: Image) -> void:
	var body = Color(0.12, 0.1, 0.18)
	var glow = Color(0.4, 0.2, 0.6)
	# 몸통 (위에서 아래로 흐르는 형태)
	for y in range(10, 55):
		var width_ratio = 1.0 - abs((y - 28.0) / 25.0)
		width_ratio = maxf(width_ratio, 0.2)
		var hw = int(12.0 * width_ratio)
		for x in range(32 - hw, 32 + hw):
			var alpha = 0.7 + randf_range(-0.1, 0.1)
			if y > 40:
				alpha *= 1.0 - (y - 40.0) / 15.0  # 아래로 갈수록 투명
			_px64(img, x, y, Color(body.r, body.g, body.b, alpha))
	# 눈 (빛나는)
	_fill64(img, 26, 22, 3, 2, glow)
	_fill64(img, 35, 22, 3, 2, glow)
	_px64(img, 27, 22, Color(0.8, 0.6, 1.0))
	_px64(img, 36, 22, Color(0.8, 0.6, 1.0))

## 메모리 이터 — 기억을 먹는 곤충형
static func _draw_memory_eater(img: Image) -> void:
	var body = Color(0.2, 0.15, 0.12)
	var shell = Color(0.3, 0.22, 0.18)
	var eye_c = Color(0.9, 0.7, 0.1)
	# 등딱지 (타원)
	for y in range(15, 45):
		for x in range(14, 50):
			var dx = (x - 32.0) / 18.0
			var dy = (y - 30.0) / 15.0
			if dx * dx + dy * dy < 1.0:
				var c = shell if dy < 0.3 else body
				_px64(img, x, y, Color(c.r + randf_range(-0.02, 0.02), c.g + randf_range(-0.02, 0.02), c.b + randf_range(-0.02, 0.02), 1.0))
	# 등딱지 무늬
	_fill64(img, 30, 18, 4, 20, Color(shell.r + 0.08, shell.g + 0.05, shell.b + 0.02))
	# 다리 6개
	for i in range(3):
		var lx1 = 16 + i * 8
		_fill64(img, lx1, 40, 2, 12, body)
		_fill64(img, lx1 + 20, 40, 2, 12, body)
	# 머리
	_fill64(img, 24, 10, 16, 8, body)
	# 눈
	_fill64(img, 27, 12, 3, 3, eye_c)
	_fill64(img, 36, 12, 3, 3, eye_c)
	# 턱
	_fill64(img, 28, 17, 3, 4, Color(0.15, 0.1, 0.08))
	_fill64(img, 33, 17, 3, 4, Color(0.15, 0.1, 0.08))

## 쉐이드 센티넬 — 보스, 거대 갑옷 형태
static func _draw_shade_sentinel(img: Image) -> void:
	var armor = Color(0.15, 0.12, 0.2)
	var armor_h = Color(0.25, 0.2, 0.35)
	var eye_c = Color(0.9, 0.15, 0.3)
	var void_c = Color(0.4, 0.1, 0.5)
	# 몸통 (넓은 갑옷)
	_fill64(img, 12, 18, 40, 35, armor)
	# 어깨
	_fill64(img, 6, 18, 10, 12, armor_h)
	_fill64(img, 48, 18, 10, 12, armor_h)
	# 어깨 스파이크
	_fill64(img, 8, 14, 4, 6, armor)
	_fill64(img, 52, 14, 4, 6, armor)
	# 가슴 디테일
	_fill64(img, 24, 28, 16, 2, void_c)
	_fill64(img, 28, 32, 8, 8, Color(void_c.r, void_c.g, void_c.b, 0.5))
	# 투구
	_fill64(img, 18, 4, 28, 16, armor_h)
	_fill64(img, 20, 2, 24, 4, armor)
	# 눈 (빨간 슬릿)
	_fill64(img, 24, 10, 6, 2, eye_c)
	_fill64(img, 34, 10, 6, 2, eye_c)
	_px64(img, 25, 10, Color(1, 0.5, 0.5))
	_px64(img, 35, 10, Color(1, 0.5, 0.5))
	# 다리
	_fill64(img, 18, 50, 8, 12, armor)
	_fill64(img, 38, 50, 8, 12, armor)
	# 보이드 에너지 줄기
	for i in range(4):
		var sx = randi_range(14, 50)
		for sy in range(randi_range(5, 15), randi_range(20, 30)):
			_px64(img, sx, sy, Color(void_c.r, void_c.g, void_c.b, 0.25))

## 보이드 스토커 — 날씬한 인간형
static func _draw_void_stalker(img: Image) -> void:
	var body = Color(0.08, 0.06, 0.12)
	var glow = Color(0.5, 0.2, 0.7)
	# 몸통
	for y in range(18, 55):
		var w = 8 if y < 35 else 6
		for x in range(32 - w, 32 + w):
			_px64(img, x, y, body)
	# 팔 (길게)
	_fill64(img, 16, 22, 3, 20, body)
	_fill64(img, 45, 22, 3, 20, body)
	# 발톱
	_fill64(img, 15, 40, 4, 3, glow)
	_fill64(img, 45, 40, 4, 3, glow)
	# 머리
	for y in range(5, 20):
		for x in range(22, 42):
			var dx = (x - 32.0) / 10.0
			var dy = (y - 12.0) / 8.0
			if dx * dx + dy * dy < 1.0:
				_px64(img, x, y, body)
	# 눈 (3개!)
	_fill64(img, 26, 10, 2, 2, glow)
	_fill64(img, 30, 8, 2, 2, glow)
	_fill64(img, 36, 10, 2, 2, glow)
	# 다리
	_fill64(img, 26, 50, 3, 12, body)
	_fill64(img, 35, 50, 3, 12, body)

## 범용 적 (이름 기반 색상)
static func _draw_generic_enemy(img: Image, enemy_type: String) -> void:
	var hash_val = enemy_type.hash()
	var r = fmod(abs(float(hash_val)) * 0.000001, 0.4) + 0.1
	var g_val = fmod(abs(float(hash_val)) * 0.0000017, 0.3) + 0.05
	var b = fmod(abs(float(hash_val)) * 0.0000023, 0.4) + 0.1
	var body = Color(r, g_val, b)
	var eye_c = Color(minf(r + 0.5, 1.0), g_val + 0.2, minf(b + 0.3, 1.0))
	# 기본 몸통
	for y in range(12, 52):
		for x in range(14, 50):
			var dx = (x - 32.0) / 18.0
			var dy = (y - 32.0) / 20.0
			if dx * dx + dy * dy < 1.0:
				_px64(img, x, y, Color(body.r + randf_range(-0.03, 0.03), body.g + randf_range(-0.03, 0.03), body.b + randf_range(-0.03, 0.03), 1.0))
	# 눈
	_fill64(img, 25, 22, 3, 3, eye_c)
	_fill64(img, 36, 22, 3, 3, eye_c)

## 64x64 헬퍼
static func _px64(img: Image, x: int, y: int, color: Color) -> void:
	if x >= 0 and x < 64 and y >= 0 and y < 64:
		img.set_pixel(x, y, color)

static func _fill64(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for py in range(h):
		for px in range(w):
			_px64(img, x + px, y + py, color)

static func _add_outline_64(img: Image, outline_color: Color) -> void:
	var s = 64
	var alpha_map: Array = []
	for y in range(s):
		var row: Array = []
		for x in range(s):
			row.append(img.get_pixel(x, y).a > 0.1)
		alpha_map.append(row)
	for y in range(s):
		for x in range(s):
			if alpha_map[y][x]:
				continue
			var has_neighbor = false
			for dy in range(-1, 2):
				for dx in range(-1, 2):
					if dx == 0 and dy == 0:
						continue
					var nx = x + dx
					var ny = y + dy
					if nx >= 0 and nx < s and ny >= 0 and ny < s:
						if alpha_map[ny][nx]:
							has_neighbor = true
							break
				if has_neighbor:
					break
			if has_neighbor:
				img.set_pixel(x, y, outline_color)
