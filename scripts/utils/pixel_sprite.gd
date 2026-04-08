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
