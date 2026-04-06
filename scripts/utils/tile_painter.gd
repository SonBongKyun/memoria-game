## TilePainter — 픽셀아트 타일셋 생성 + TileMapLayer 세팅 유틸리티
## Image.set_pixel()로 상세한 타일을 그려서 TileSet 아틀라스를 만듦.
class_name TilePainter

const TILE: int = 32

## TileMapLayer를 생성하고 타일 데이터를 배치
## tile_defs: Array of {color: Color, detail: String}
## map_data: 2D Array of tile indices
## Returns: TileMapLayer (add_child 필요)
static func create_tilemap(tile_defs: Array, map_data: Array, width: int, height: int) -> TileMapLayer:
	var tilemap = TileMapLayer.new()
	tilemap.z_index = -1

	# 아틀라스 이미지 생성 (타일 수 x 1)
	var count = tile_defs.size()
	var atlas_img = Image.create(TILE * count, TILE, false, Image.FORMAT_RGBA8)

	for i in range(count):
		var def = tile_defs[i]
		var base_color: Color = def.get("color", Color(0.3, 0.3, 0.3))
		var detail: String = def.get("detail", "flat")
		_paint_tile(atlas_img, i * TILE, 0, base_color, detail)

	# TileSet 생성
	var tileset = TileSet.new()
	tileset.tile_size = Vector2i(TILE, TILE)

	var source = TileSetAtlasSource.new()
	source.texture = ImageTexture.create_from_image(atlas_img)
	source.texture_region_size = Vector2i(TILE, TILE)

	# 각 타일 등록
	for i in range(count):
		source.create_tile(Vector2i(i, 0))

	var source_id = tileset.add_source(source)
	tilemap.tile_set = tileset

	# 맵 데이터 배치
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				var tile_idx = map_data[y][x]
				if tile_idx >= 0 and tile_idx < count:
					tilemap.set_cell(Vector2i(x, y), source_id, Vector2i(tile_idx, 0))

	return tilemap

## 충돌 레이어 추가 (특정 타일 인덱스에)
static func add_collisions(tilemap: TileMapLayer, map_data: Array, width: int, height: int, wall_indices: Array) -> Array[StaticBody2D]:
	var bodies: Array[StaticBody2D] = []
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				var idx = map_data[y][x]
				if idx in wall_indices:
					var body = StaticBody2D.new()
					body.position = Vector2(x * TILE + TILE / 2.0, y * TILE + TILE / 2.0)
					body.collision_layer = 1
					var shape = CollisionShape2D.new()
					var rect = RectangleShape2D.new()
					rect.size = Vector2(TILE, TILE)
					shape.shape = rect
					body.add_child(shape)
					bodies.append(body)
	return bodies

## ========== 타일 그리기 ==========

static func _paint_tile(img: Image, ox: int, oy: int, base: Color, detail: String) -> void:
	# 기본 채우기
	_fill(img, ox, oy, TILE, TILE, base)

	# 엣지 셰이딩 (깊이감) — detail 페인팅 전에 적용
	_paint_edge_shading(img, ox, oy, base, detail)

	match detail:
		"grass":
			_paint_grass(img, ox, oy, base)
		"tree":
			_paint_tree(img, ox, oy, base)
		"bush":
			_paint_bush(img, ox, oy, base)
		"water":
			_paint_water(img, ox, oy, base)
		"path":
			_paint_path(img, ox, oy, base)
		"stone":
			_paint_stone(img, ox, oy, base)
		"wall":
			_paint_wall(img, ox, oy, base)
		"stall":
			_paint_stall(img, ox, oy, base)
		"door":
			_paint_door(img, ox, oy, base)
		"alley":
			_paint_alley(img, ox, oy, base)
		"sand":
			_paint_sand(img, ox, oy, base)
		"cliff":
			_paint_cliff(img, ox, oy, base)
		"rock":
			_paint_rock(img, ox, oy, base)
		"hut":
			_paint_hut(img, ox, oy, base)
		"garden":
			_paint_garden(img, ox, oy, base)
		"lantern":
			_paint_lantern(img, ox, oy, base)
		"void":
			_paint_void(img, ox, oy, base)
		"fragment":
			_paint_fragment(img, ox, oy, base)
		"crack":
			_paint_crack(img, ox, oy, base)
		"core":
			_paint_core(img, ox, oy, base)

## ── 개별 타일 페인팅 ──

static func _paint_grass(img: Image, ox: int, oy: int, base: Color) -> void:
	# 풀 텍스처: 랜덤 밝기 변화 + 풀잎 힌트
	for y in range(TILE):
		for x in range(TILE):
			var v = randf_range(-0.04, 0.04)
			_px(img, ox + x, oy + y, _shift(base, v))
	# 풀잎 (짧은 세로 선)
	for i in range(8):
		var gx = randi_range(2, TILE - 3)
		var gy = randi_range(4, TILE - 2)
		var gc = _shift(base, randf_range(0.05, 0.12))
		_px(img, ox + gx, oy + gy, gc)
		_px(img, ox + gx, oy + gy - 1, gc)
	# 가끔 꽃
	if randi_range(0, 3) == 0:
		var fx = randi_range(4, TILE - 5)
		var fy = randi_range(4, TILE - 5)
		var fc = [Color(0.8, 0.7, 0.2), Color(0.7, 0.3, 0.3), Color(0.8, 0.8, 0.5)][randi_range(0, 2)]
		_px(img, ox + fx, oy + fy, fc)

static func _paint_tree(img: Image, ox: int, oy: int, _base: Color) -> void:
	# 줄기
	var trunk = Color(0.35, 0.22, 0.12)
	_fill(img, ox + 13, oy + 20, 6, 12, trunk)
	_fill(img, ox + 14, oy + 22, 4, 10, _shift(trunk, 0.05))
	# 수관 (원형 느낌)
	var canopy = Color(0.12, 0.35, 0.15)
	var canopy2 = Color(0.08, 0.28, 0.12)
	for y in range(20):
		for x in range(TILE):
			var dx = x - 16.0
			var dy = y - 10.0
			if dx * dx + dy * dy < 120:
				var c = canopy if ((x + y) % 3 != 0) else canopy2
				_px(img, ox + x, oy + y, _shift(c, randf_range(-0.02, 0.02)))
	# 하이라이트
	for y in range(4, 10):
		for x in range(10, 16):
			var dx = x - 13.0
			var dy = y - 7.0
			if dx * dx + dy * dy < 12:
				_px(img, ox + x, oy + y, _shift(canopy, 0.08))

static func _paint_bush(img: Image, ox: int, oy: int, base: Color) -> void:
	# 덤불: 작은 원형 수관
	var bush_c = Color(0.15, 0.32, 0.12)
	for y in range(8, TILE):
		for x in range(4, TILE - 4):
			var dx = x - 16.0
			var dy = y - 20.0
			if dx * dx + dy * dy < 80:
				_px(img, ox + x, oy + y, _shift(bush_c, randf_range(-0.03, 0.03)))
	# 하단 그림자
	for x in range(6, TILE - 6):
		_px(img, ox + x, oy + TILE - 2, _shift(base, -0.05))

static func _paint_water(img: Image, ox: int, oy: int, base: Color) -> void:
	var deep = _shift(base, -0.05)
	for y in range(TILE):
		for x in range(TILE):
			var wave = sin((x + y * 0.5) * 0.5) * 0.04
			_px(img, ox + x, oy + y, _shift(base, wave))
	# 파도 라인
	for i in range(3):
		var wy = 6 + i * 10
		var highlight = Color(0.25, 0.4, 0.6, 0.6)
		for x in range(TILE):
			var offset = int(sin(x * 0.3 + i * 2.0) * 2)
			var py = wy + offset
			if py >= 0 and py < TILE:
				_px(img, ox + x, oy + py, highlight)

static func _paint_path(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.03, 0.03)))
	# 자갈 힌트
	for i in range(5):
		var px = randi_range(3, TILE - 4)
		var py = randi_range(3, TILE - 4)
		_px(img, ox + px, oy + py, _shift(base, -0.08))
		_px(img, ox + px + 1, oy + py, _shift(base, -0.06))

static func _paint_stone(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.02, 0.02)))
	# 돌 줄눈 (수평/수직 선)
	var grout = _shift(base, -0.08)
	for x in range(TILE):
		_px(img, ox + x, oy + 15, grout)
	for y in range(0, 15):
		_px(img, ox + 16, oy + y, grout)
	for y in range(16, TILE):
		_px(img, ox + 8, oy + y, grout)
		_px(img, ox + 24, oy + y, grout)

static func _paint_wall(img: Image, ox: int, oy: int, base: Color) -> void:
	# 벽돌 패턴
	var mortar = _shift(base, -0.06)
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.02, 0.02)))
	# 벽돌 줄
	for row in range(4):
		var ry = row * 8
		for x in range(TILE):
			_px(img, ox + x, oy + ry, mortar)
		var offset = 8 if row % 2 == 0 else 0
		for col in range(4):
			var cx = offset + col * 16
			if cx > 0 and cx < TILE:
				for y in range(ry, mini(ry + 8, TILE)):
					_px(img, ox + cx, oy + y, mortar)
	# 어두운 상단
	for x in range(TILE):
		_px(img, ox + x, oy, _shift(base, -0.1))

static func _paint_stall(img: Image, ox: int, oy: int, base: Color) -> void:
	_fill(img, ox, oy, TILE, TILE, base)
	# 천막 줄무늬
	var stripe = _shift(base, 0.1)
	for x in range(TILE):
		if (x / 4) % 2 == 0:
			for y in range(8):
				_px(img, ox + x, oy + y, stripe)
	# 테이블
	_fill(img, ox + 2, oy + 10, TILE - 4, 4, _shift(base, -0.08))
	# 물건들 (작은 점)
	for i in range(3):
		var gx = 4 + i * 9
		_fill(img, ox + gx, oy + 8, 3, 2, _shift(base, randf_range(0.1, 0.2)))

static func _paint_door(img: Image, ox: int, oy: int, base: Color) -> void:
	_fill(img, ox, oy, TILE, TILE, _shift(base, -0.05))
	# 문 프레임
	_fill(img, ox + 8, oy + 4, 16, 24, base)
	_fill(img, ox + 10, oy + 6, 12, 22, _shift(base, 0.05))
	# 손잡이
	_px(img, ox + 20, oy + 18, Color(0.6, 0.5, 0.3))

static func _paint_alley(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.02, 0.02)))
	# 물웅덩이 느낌
	for i in range(2):
		var px = randi_range(6, TILE - 8)
		var py = randi_range(6, TILE - 8)
		for dy in range(3):
			for dx in range(4):
				_px(img, ox + px + dx, oy + py + dy, _shift(base, -0.05))

static func _paint_sand(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			var v = sin(x * 0.4 + y * 0.3) * 0.03
			_px(img, ox + x, oy + y, _shift(base, v + randf_range(-0.02, 0.02)))
	# 바람 자국
	for i in range(2):
		var sy = randi_range(4, TILE - 4)
		for x in range(randi_range(4, 10), randi_range(18, TILE - 2)):
			_px(img, ox + x, oy + sy, _shift(base, 0.04))

static func _paint_cliff(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			var gradient = float(y) / TILE * 0.06
			_px(img, ox + x, oy + y, _shift(base, gradient + randf_range(-0.02, 0.02)))
	# 균열 선
	var cx = randi_range(8, 24)
	for y in range(4, TILE - 4):
		cx += randi_range(-1, 1)
		cx = clampi(cx, 4, TILE - 4)
		_px(img, ox + cx, oy + y, _shift(base, -0.06))

static func _paint_rock(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.03, 0.03)))
	# 돌 표면 디테일
	for i in range(4):
		var rx = randi_range(4, TILE - 6)
		var ry = randi_range(4, TILE - 6)
		_px(img, ox + rx, oy + ry, _shift(base, -0.06))
		_px(img, ox + rx + 1, oy + ry, _shift(base, 0.04))

static func _paint_hut(img: Image, ox: int, oy: int, base: Color) -> void:
	# 벽
	_fill(img, ox, oy + 8, TILE, TILE - 8, base)
	for y in range(8, TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.02, 0.02)))
	# 지붕
	var roof = _shift(base, -0.12)
	for y in range(8):
		var indent = y
		for x in range(indent, TILE - indent):
			_px(img, ox + x, oy + y, _shift(roof, randf_range(-0.02, 0.02)))
	# 문
	_fill(img, ox + 12, oy + 18, 8, 14, _shift(base, -0.08))
	# 창문
	_fill(img, ox + 4, oy + 14, 5, 5, Color(0.3, 0.35, 0.45, 0.8))
	_fill(img, ox + 23, oy + 14, 5, 5, Color(0.3, 0.35, 0.45, 0.8))

static func _paint_garden(img: Image, ox: int, oy: int, base: Color) -> void:
	# 풀 베이스
	for y in range(TILE):
		for x in range(TILE):
			_px(img, ox + x, oy + y, _shift(base, randf_range(-0.03, 0.03)))
	# 꽃들 (여러 색상)
	var flower_colors = [
		Color(0.7, 0.2, 0.25), Color(0.6, 0.5, 0.15),
		Color(0.25, 0.15, 0.5), Color(0.8, 0.5, 0.2),
	]
	for i in range(randi_range(5, 8)):
		var fx = randi_range(2, TILE - 3)
		var fy = randi_range(2, TILE - 3)
		var fc = flower_colors[randi_range(0, flower_colors.size() - 1)]
		_px(img, ox + fx, oy + fy, fc)
		_px(img, ox + fx, oy + fy - 1, _shift(base, 0.05))  # 줄기

static func _paint_lantern(img: Image, ox: int, oy: int, base: Color) -> void:
	# 바닥
	_paint_stone(img, ox, oy, base)
	# 랜턴 기둥
	_fill(img, ox + 15, oy + 12, 2, 16, Color(0.3, 0.25, 0.2))
	# 빛
	var glow = Color(0.9, 0.7, 0.3)
	_fill(img, ox + 13, oy + 8, 6, 5, glow)
	# 후광 (반투명 느낌 — 밝은 색으로)
	for y in range(4, 18):
		for x in range(8, 24):
			var dx = x - 16.0
			var dy = y - 10.0
			var dist = dx * dx + dy * dy
			if dist < 60:
				var current = img.get_pixel(ox + x, oy + y)
				var blend = 0.15 * (1.0 - dist / 60.0)
				_px(img, ox + x, oy + y, current.lerp(glow, blend))

static func _paint_void(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			var noise = randf_range(-0.01, 0.01)
			_px(img, ox + x, oy + y, _shift(base, noise))
	# 가끔 빛나는 점 (보이드 에너지)
	if randi_range(0, 2) == 0:
		var sx = randi_range(4, TILE - 5)
		var sy = randi_range(4, TILE - 5)
		_px(img, ox + sx, oy + sy, Color(0.15, 0.05, 0.25))

static func _paint_fragment(img: Image, ox: int, oy: int, base: Color) -> void:
	_paint_void(img, ox, oy, Color(0.02, 0.02, 0.05))
	# 부유 파편 (중앙에 불규칙한 형태)
	for y in range(8, 24):
		for x in range(8, 24):
			var dx = x - 16.0
			var dy = y - 16.0
			if dx * dx + dy * dy < 40 + randf_range(-10, 10):
				_px(img, ox + x, oy + y, _shift(base, randf_range(-0.03, 0.03)))

static func _paint_crack(img: Image, ox: int, oy: int, base: Color) -> void:
	for y in range(TILE):
		for x in range(TILE):
			var gradient = float(y) / TILE * 0.04
			_px(img, ox + x, oy + y, _shift(base, gradient + randf_range(-0.01, 0.01)))
	# 보라색 균열 빛
	var crack_c = Color(0.15, 0.03, 0.2)
	var cx = randi_range(10, 22)
	for y in range(2, TILE - 2):
		cx += randi_range(-1, 1)
		cx = clampi(cx, 4, TILE - 4)
		_px(img, ox + cx, oy + y, crack_c)
		_px(img, ox + cx - 1, oy + y, _shift(crack_c, -0.02))

static func _paint_core(img: Image, ox: int, oy: int, base: Color) -> void:
	# 맥동 코어 — 중심으로 갈수록 밝아지는 패턴
	for y in range(TILE):
		for x in range(TILE):
			var dx = x - 16.0
			var dy = y - 16.0
			var dist = sqrt(dx * dx + dy * dy) / 16.0
			var intensity = maxf(0, 1.0 - dist) * 0.15
			_px(img, ox + x, oy + y, _shift(base, intensity + randf_range(-0.02, 0.02)))

## ── 엣지 셰이딩 (깊이감) ──

static func _paint_edge_shading(img: Image, ox: int, oy: int, base: Color, detail: String) -> void:
	# 벽/나무/건물은 강한 셰이딩, 바닥류는 약한 셰이딩
	var shadow_strength: float = 0.0
	var highlight_strength: float = 0.0

	match detail:
		"tree", "wall", "cliff", "rock", "hut", "stall":
			shadow_strength = 0.08
			highlight_strength = 0.05
		"grass", "path", "sand", "stone", "alley", "garden":
			shadow_strength = 0.03
			highlight_strength = 0.02
		"water", "void", "fragment", "crack", "core":
			shadow_strength = 0.02
			highlight_strength = 0.01
		"door", "lantern":
			shadow_strength = 0.05
			highlight_strength = 0.03
		_:
			return

	# 상단 하이라이트 (빛 받는 면)
	for x in range(TILE):
		for i in range(2):
			var alpha = highlight_strength * (1.0 - float(i) / 2.0)
			_px(img, ox + x, oy + i, _shift(base, alpha))

	# 좌측 하이라이트 (약하게)
	for y in range(TILE):
		_px(img, ox, oy + y, _shift(base, highlight_strength * 0.5))

	# 하단 그림자 (깊이감)
	for x in range(TILE):
		for i in range(3):
			var row = TILE - 1 - i
			var alpha = shadow_strength * (1.0 - float(i) / 3.0)
			_px(img, ox + x, oy + row, _shift(base, -alpha))

	# 우측 그림자 (약하게)
	for y in range(TILE):
		for i in range(2):
			var col = TILE - 1 - i
			var alpha = shadow_strength * 0.6 * (1.0 - float(i) / 2.0)
			_px(img, ox + col, oy + y, _shift(base, -alpha))

## ========== 헬퍼 ==========

static func _fill(img: Image, x: int, y: int, w: int, h: int, color: Color) -> void:
	for py in range(h):
		for px in range(w):
			_px(img, x + px, y + py, color)

static func _px(img: Image, x: int, y: int, color: Color) -> void:
	if x >= 0 and x < img.get_width() and y >= 0 and y < img.get_height():
		img.set_pixel(x, y, color)

static func _shift(color: Color, amount: float) -> Color:
	return Color(
		clampf(color.r + amount, 0, 1),
		clampf(color.g + amount, 0, 1),
		clampf(color.b + amount, 0, 1),
		color.a
	)
