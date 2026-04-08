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

static func arrel_config() -> Dictionary:
	return {
		"skin": Color(0.82, 0.7, 0.58),
		"hair": Color(0.35, 0.38, 0.55),
		"hair_style": "medium",
		"coat": Color(0.15, 0.17, 0.28),
		"shirt": Color(0.35, 0.3, 0.28),
		"pants": Color(0.12, 0.12, 0.15),
		"boots": Color(0.08, 0.07, 0.07),
		"eye": Color(0.25, 0.45, 0.65),
		"accessory": Color(0.5, 0.45, 0.4),
		"accessory_type": "sword",
	}

static func elia_config() -> Dictionary:
	return {
		"skin": Color(0.88, 0.78, 0.68),
		"hair": Color(0.7, 0.72, 0.78),
		"hair_style": "long",
		"coat": Color(0.35, 0.28, 0.2),
		"shirt": Color(0.55, 0.5, 0.45),
		"pants": Color(0.25, 0.22, 0.2),
		"boots": Color(0.15, 0.12, 0.1),
		"eye": Color(0.3, 0.5, 0.8),
		"accessory": Color(0.7, 0.55, 0.25),
		"accessory_type": "brooch",
	}

static func sable_config() -> Dictionary:
	return {
		"skin": Color(0.65, 0.5, 0.4),
		"hair": Color(0.12, 0.1, 0.1),
		"hair_style": "short",
		"coat": Color(0.22, 0.2, 0.25),
		"shirt": Color(0.3, 0.25, 0.28),
		"pants": Color(0.15, 0.14, 0.16),
		"boots": Color(0.1, 0.08, 0.08),
		"eye": Color(0.45, 0.35, 0.25),
		"accessory": Color(0.4, 0.35, 0.3),
		"accessory_type": "scar",
	}

static func npc_config(base_color: Color) -> Dictionary:
	return {
		"skin": Color(0.8, 0.7, 0.6),
		"hair": _darken(base_color, 0.1),
		"hair_style": "short",
		"coat": base_color,
		"shirt": _lighten(base_color, 0.1),
		"pants": _darken(base_color, 0.15),
		"boots": Color(0.12, 0.1, 0.1),
		"eye": Color(0.35, 0.3, 0.25),
		"accessory_type": "none",
	}
