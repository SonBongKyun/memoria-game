## PixelSprite — 픽셀아트 캐릭터 스프라이트 생성 유틸리티
## Image.set_pixel()로 상세한 픽셀아트 캐릭터를 생성.
## 32x32 프레임, 4방향 x (idle + 4 walk) 애니메이션.
class_name PixelSprite

const SIZE: int = 32
const HALF: int = 16

## 캐릭터 설정 딕셔너리
## {
##   skin: Color, hair: Color, hair_style: "short"/"medium"/"long",
##   coat: Color, shirt: Color, pants: Color, boots: Color,
##   eye: Color, accessory: Color, accessory_type: "none"/"brooch"/"sword"/"scar"
## }

## SpriteFrames 생성 (4방향 idle + walk)
static func create_frames(config: Dictionary) -> SpriteFrames:
	var frames = SpriteFrames.new()

	var directions = ["down", "up", "left", "right"]
	for dir in directions:
		# idle
		var idle_name = "idle_" + dir
		frames.add_animation(idle_name)
		frames.set_animation_speed(idle_name, 1)
		frames.set_animation_loop(idle_name, true)
		var idle_img = _draw_character(config, dir, 0)
		frames.add_frame(idle_name, ImageTexture.create_from_image(idle_img))

		# walk
		var walk_name = "walk_" + dir
		frames.add_animation(walk_name)
		frames.set_animation_speed(walk_name, 8)
		frames.set_animation_loop(walk_name, true)
		for step in range(4):
			var walk_img = _draw_character(config, dir, step + 1)
			frames.add_frame(walk_name, ImageTexture.create_from_image(walk_img))

	# 기본 "default" 애니메이션 제거
	if frames.has_animation("default"):
		frames.remove_animation("default")

	return frames

## 단일 프레임 그리기
## walk_frame: 0=idle, 1~4=walk cycle
static func _draw_character(config: Dictionary, direction: String, walk_frame: int) -> Image:
	var img = Image.create(SIZE, SIZE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))  # 투명 배경

	var skin = config.get("skin", Color(0.85, 0.72, 0.6))
	var hair = config.get("hair", Color(0.25, 0.28, 0.4))
	var coat = config.get("coat", Color(0.2, 0.22, 0.35))
	var shirt = config.get("shirt", Color(0.3, 0.28, 0.25))
	var pants = config.get("pants", Color(0.18, 0.17, 0.2))
	var boots = config.get("boots", Color(0.12, 0.1, 0.1))
	var eye = config.get("eye", Color(0.3, 0.5, 0.7))
	var accessory_color = config.get("accessory", Color(0.7, 0.55, 0.2))
	var accessory_type = config.get("accessory_type", "none")
	var hair_style = config.get("hair_style", "medium")

	# 색상 변형 (명암)
	var skin_shadow = _darken(skin, 0.2)
	var hair_shadow = _darken(hair, 0.15)
	var coat_shadow = _darken(coat, 0.15)
	var coat_highlight = _lighten(coat, 0.1)
	var pants_shadow = _darken(pants, 0.15)
	var boots_highlight = _lighten(boots, 0.15)

	# 걷기 오프셋 (바운스)
	var bob_y: int = 0
	var step_offset: int = 0  # 발 위치
	if walk_frame > 0:
		match walk_frame:
			1: bob_y = 0; step_offset = -1
			2: bob_y = -1; step_offset = 0
			3: bob_y = 0; step_offset = 1
			4: bob_y = -1; step_offset = 0

	var base_y: int = 3 + bob_y  # 상단 여백

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

	var sk_s = _darken(skin, 0.2)
	var h_s = _darken(hair, 0.15)
	var c_s = _darken(coat, 0.15)
	var c_h = _lighten(coat, 0.1)

	# ── 머리카락 (윗부분) ──
	var hx = 12; var hy = by
	# 머리카락 상단
	_fill_rect(img, hx, hy, 8, 2, hair)
	_fill_rect(img, hx - 1, hy + 1, 10, 1, hair)
	if hair_style == "long":
		_fill_rect(img, hx - 1, hy, 10, 2, hair)
		_fill_rect(img, hx - 2, hy + 1, 12, 1, hair)
	# 머리카락 옆 (프레임)
	_fill_rect(img, hx - 1, hy + 2, 1, 4, hair)
	_fill_rect(img, hx + 8, hy + 2, 1, 4, hair)
	if hair_style != "short":
		_fill_rect(img, hx - 1, hy + 6, 1, 2, h_s)
		_fill_rect(img, hx + 8, hy + 6, 1, 2, h_s)
	# 머리카락 하이라이트
	_set_px(img, hx + 2, hy, _lighten(hair, 0.15))
	_set_px(img, hx + 5, hy + 1, _lighten(hair, 0.1))

	# ── 얼굴 ──
	var fy = hy + 2
	_fill_rect(img, hx, fy, 8, 6, skin)
	# 눈
	_set_px(img, hx + 2, fy + 2, eye)
	_set_px(img, hx + 5, fy + 2, eye)
	_set_px(img, hx + 2, fy + 1, Color(0.9, 0.95, 1.0))  # 눈 하이라이트
	_set_px(img, hx + 5, fy + 1, Color(0.9, 0.95, 1.0))
	# 코 힌트
	_set_px(img, hx + 3, fy + 3, sk_s)
	# 입
	_set_px(img, hx + 3, fy + 4, _darken(skin, 0.3))
	_set_px(img, hx + 4, fy + 4, _darken(skin, 0.3))
	# 얼굴 그림자 (양 옆)
	_fill_rect(img, hx, fy, 1, 6, sk_s)
	_fill_rect(img, hx + 7, fy, 1, 6, sk_s)

	# ── 목 ──
	var ny = hy + 8
	_fill_rect(img, hx + 2, ny, 4, 1, skin)

	# ── 상체 (코트/셔츠) ──
	var ty = ny + 1
	# 어깨
	_fill_rect(img, hx - 2, ty, 12, 2, coat)
	_fill_rect(img, hx - 2, ty, 1, 2, c_s)
	_fill_rect(img, hx + 9, ty, 1, 2, c_s)
	# 코트 몸통
	_fill_rect(img, hx - 1, ty + 2, 10, 5, coat)
	# 셔츠 V라��
	_fill_rect(img, hx + 3, ty, 2, 3, shirt)
	# 코트 주름
	_set_px(img, hx + 1, ty + 3, c_s)
	_set_px(img, hx + 6, ty + 4, c_s)
	# 코트 하이라이트
	_set_px(img, hx + 2, ty + 1, c_h)

	# 액세서리
	if acc_type == "brooch":
		_set_px(img, hx + 3, ty + 1, acc_color)
	elif acc_type == "sword":
		_fill_rect(img, hx + 8, ty + 1, 1, 6, _darken(acc_color, 0.2))
		_set_px(img, hx + 8, ty, acc_color)
	elif acc_type == "scar":
		_set_px(img, hx + 1, fy + 2, _darken(skin, 0.3))
		_set_px(img, hx + 2, fy + 3, _darken(skin, 0.3))

	# ── 팔 ──
	var ay = ty
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = 1
			2: arm_swing = 0
			3: arm_swing = -1
			4: arm_swing = 0

	# 왼팔
	_fill_rect(img, hx - 3, ay + arm_swing, 2, 6, coat)
	_fill_rect(img, hx - 3, ay + 6 + arm_swing, 2, 1, skin)
	# 오른팔
	_fill_rect(img, hx + 9, ay - arm_swing, 2, 6, coat)
	_fill_rect(img, hx + 9, ay + 6 - arm_swing, 2, 1, skin)

	# ── 하체 (바지) ──
	var py = ty + 7
	_fill_rect(img, hx, py, 8, 4, pants)
	_set_px(img, hx + 3, py, _darken(pants, 0.1))  # 벨트 힌트
	_set_px(img, hx + 4, py, _darken(pants, 0.1))
	# 다리 분리선
	_fill_rect(img, hx + 3, py + 1, 2, 3, _darken(pants, 0.1))

	# ── 발 (부츠) ──
	var fy2 = py + 4
	_fill_rect(img, hx, fy2 + step, 3, 2, boots)
	_fill_rect(img, hx + 5, fy2 - step, 3, 2, boots)
	_set_px(img, hx, fy2 + step, _lighten(boots, 0.15))
	_set_px(img, hx + 5, fy2 - step, _lighten(boots, 0.15))

## ========== 뒷��� (up) ==========
static func _draw_back(img: Image, c: Dictionary, by: int, walk: int, step: int) -> void:
	var hair = c.get("hair", Color(0.25, 0.28, 0.4))
	var coat = c.get("coat", Color(0.2, 0.22, 0.35))
	var pants = c.get("pants", Color(0.18, 0.17, 0.2))
	var boots = c.get("boots", Color(0.12, 0.1, 0.1))
	var hair_style = c.get("hair_style", "medium")
	var acc_type = c.get("accessory_type", "none")
	var acc_color = c.get("accessory", Color(0.7, 0.55, 0.2))

	var h_s = _darken(hair, 0.15)
	var c_s = _darken(coat, 0.15)

	var hx = 12; var hy = by

	# ── 머리카락 (뒷면 — 더 많이 보임) ──
	_fill_rect(img, hx, hy, 8, 2, hair)
	_fill_rect(img, hx - 1, hy + 1, 10, 1, hair)
	_fill_rect(img, hx - 1, hy + 2, 10, 6, hair)
	# 머리카락 텍스처
	_set_px(img, hx + 1, hy + 3, h_s)
	_set_px(img, hx + 4, hy + 2, _lighten(hair, 0.1))
	_set_px(img, hx + 6, hy + 4, h_s)
	_set_px(img, hx + 2, hy + 5, _lighten(hair, 0.08))

	if hair_style == "long":
		_fill_rect(img, hx - 1, hy + 8, 10, 3, hair)
		_set_px(img, hx + 3, hy + 9, h_s)
	elif hair_style == "medium":
		_fill_rect(img, hx, hy + 8, 8, 1, hair)

	# ── 목 ──
	var ny = hy + 8
	var skin = c.get("skin", Color(0.85, 0.72, 0.6))
	_fill_rect(img, hx + 2, ny, 4, 1, _darken(skin, 0.15))

	# ── 상체 (코트 뒷면) ──
	var ty = ny + 1
	_fill_rect(img, hx - 2, ty, 12, 2, coat)
	_fill_rect(img, hx - 1, ty + 2, 10, 5, coat)
	# 코트 뒷 주름
	_fill_rect(img, hx + 3, ty + 1, 2, 5, c_s)
	_set_px(img, hx + 1, ty + 3, c_s)
	_set_px(img, hx + 6, ty + 2, c_s)

	# 검 (등에 걸침)
	if acc_type == "sword":
		_fill_rect(img, hx + 2, ty - 2, 1, 8, _darken(acc_color, 0.2))
		_set_px(img, hx + 1, ty - 2, acc_color)
		_set_px(img, hx + 3, ty - 2, acc_color)  # 검자루

	# ── 팔 ──
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = 1
			3: arm_swing = -1
	_fill_rect(img, hx - 3, ty + arm_swing, 2, 6, coat)
	_fill_rect(img, hx + 9, ty - arm_swing, 2, 6, coat)

	# ── 하체 ──
	var py = ty + 7
	_fill_rect(img, hx, py, 8, 4, pants)
	_fill_rect(img, hx + 3, py + 1, 2, 3, _darken(pants, 0.1))

	# ── 발 ──
	var fy = py + 4
	_fill_rect(img, hx, fy + step, 3, 2, boots)
	_fill_rect(img, hx + 5, fy - step, 3, 2, boots)

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

	var sk_s = _darken(skin, 0.2)
	var h_s = _darken(hair, 0.15)
	var c_s = _darken(coat, 0.15)

	# 왼쪽 기준으로 그린 후 flip
	var hx = 12; var hy = by

	# ── 머리카락 ──
	_fill_rect(img, hx, hy, 7, 2, hair)
	_fill_rect(img, hx - 1, hy + 1, 8, 1, hair)
	# 뒤쪽 머리
	_fill_rect(img, hx + 5, hy + 2, 3, 5, hair)
	if hair_style != "short":
		_fill_rect(img, hx + 5, hy + 7, 3, 2, h_s)
	if hair_style == "long":
		_fill_rect(img, hx + 5, hy + 9, 2, 2, h_s)
	# 앞머리
	_fill_rect(img, hx - 1, hy + 2, 2, 3, hair)
	_set_px(img, hx + 2, hy, _lighten(hair, 0.12))

	# ── 얼굴 (옆) ──
	var fy = hy + 2
	_fill_rect(img, hx, fy, 6, 6, skin)
	# 눈 (한쪽만)
	_set_px(img, hx + 1, fy + 2, eye)
	_set_px(img, hx + 1, fy + 1, Color(0.9, 0.95, 1.0))
	# 코
	_set_px(img, hx - 1, fy + 3, skin)
	# 입
	_set_px(img, hx + 1, fy + 4, _darken(skin, 0.3))
	# 귀
	_set_px(img, hx + 5, fy + 2, sk_s)

	# ── 목 ──
	_fill_rect(img, hx + 2, hy + 8, 3, 1, skin)

	# ── 상체 ──
	var ty = hy + 9
	_fill_rect(img, hx - 1, ty, 9, 7, coat)
	_fill_rect(img, hx + 2, ty, 2, 2, shirt)
	# 코트 주름
	_set_px(img, hx + 1, ty + 3, c_s)
	_set_px(img, hx + 5, ty + 2, c_s)

	# 검 (옆에서 보이는)
	if acc_type == "sword":
		_fill_rect(img, hx + 7, ty - 2, 1, 8, _darken(acc_color, 0.2))
		_set_px(img, hx + 7, ty - 3, acc_color)

	# 브로치
	if acc_type == "brooch":
		_set_px(img, hx + 2, ty + 1, acc_color)

	# ── 팔 (한 쪽만 보임) ──
	var arm_swing = 0
	if walk > 0:
		match walk:
			1: arm_swing = -1
			2: arm_swing = 0
			3: arm_swing = 1
			4: arm_swing = 0
	_fill_rect(img, hx - 2, ty + 1 + arm_swing, 2, 5, coat)
	_fill_rect(img, hx - 2, ty + 6 + arm_swing, 2, 1, skin)

	# ── 하체 ──
	var py = ty + 7
	_fill_rect(img, hx, py, 6, 4, pants)
	_fill_rect(img, hx + 2, py + 1, 2, 3, _darken(pants, 0.1))

	# ── 발 ──
	var fy2 = py + 4
	_fill_rect(img, hx, fy2 + step, 3, 2, boots)
	_fill_rect(img, hx + 3, fy2 - step, 3, 2, boots)

	# Flip (오른쪽)
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
		for x in range(w / 2):
			var left = img.get_pixel(x, y)
			var right = img.get_pixel(w - 1 - x, y)
			img.set_pixel(x, y, right)
			img.set_pixel(w - 1 - x, y, left)

## ========== 프리셋 캐릭터 설정 ==========

static func arrel_config() -> Dictionary:
	return {
		"skin": Color(0.82, 0.7, 0.58),
		"hair": Color(0.35, 0.38, 0.55),  # 은청색
		"hair_style": "medium",
		"coat": Color(0.15, 0.17, 0.28),  # 어두운 남색 코��
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
		"hair": Color(0.7, 0.72, 0.78),  # 은발
		"hair_style": "long",
		"coat": Color(0.35, 0.28, 0.2),  # 갈색 여행 망토
		"shirt": Color(0.55, 0.5, 0.45),
		"pants": Color(0.25, 0.22, 0.2),
		"boots": Color(0.15, 0.12, 0.1),
		"eye": Color(0.3, 0.5, 0.8),  # 파란 눈
		"accessory": Color(0.7, 0.55, 0.25),  # 금색 브로치
		"accessory_type": "brooch",
	}

static func sable_config() -> Dictionary:
	return {
		"skin": Color(0.65, 0.5, 0.4),
		"hair": Color(0.12, 0.1, 0.1),  # 짧은 검은 머리
		"hair_style": "short",
		"coat": Color(0.22, 0.2, 0.25),  # 실용적 어두운 옷
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
