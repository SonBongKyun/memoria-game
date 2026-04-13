## TilePainter — 픽셀아트 타일셋 생성 + TileMapLayer 세팅 유틸리티
## Image.set_pixel()로 상세한 타일을 그려서 TileSet 아틀라스를 만듦.
## S55: 타일 변형 시스템 — 타일 타입당 4 변형으로 시각적 반복 감소
class_name TilePainter

const TILE: int = 32
const VARIATIONS: int = 4  ## S55: 타일 타입당 변형 수

## TileMapLayer를 생성하고 타일 데이터를 배치
## tile_defs: Array of {color: Color, detail: String}
## map_data: 2D Array of tile indices
## Returns: TileMapLayer (add_child 필요)
static func create_tilemap(tile_defs: Array, map_data: Array, width: int, height: int) -> TileMapLayer:
	var tilemap = TileMapLayer.new()
	tilemap.z_index = -1

	# S55: 아틀라스 이미지 생성 (타일 수 x VARIATIONS)
	var count = tile_defs.size()
	var total_tiles = count * VARIATIONS
	var atlas_img = Image.create(TILE * total_tiles, TILE, false, Image.FORMAT_RGBA8)

	for i in range(count):
		var def = tile_defs[i]
		var base_color: Color = def.get("color", Color(0.3, 0.3, 0.3))
		var detail: String = def.get("detail", "flat")
		# 변형 0: 기본
		_paint_tile(atlas_img, (i * VARIATIONS) * TILE, 0, base_color, detail)
		# 변형 1~3: 색상/디테일 약간 변형
		for v in range(1, VARIATIONS):
			var varied_color = _vary_color(base_color, v)
			_paint_tile(atlas_img, (i * VARIATIONS + v) * TILE, 0, varied_color, detail)

	# TileSet 생성
	var tileset = TileSet.new()
	tileset.tile_size = Vector2i(TILE, TILE)

	var source = TileSetAtlasSource.new()
	source.texture = ImageTexture.create_from_image(atlas_img)
	source.texture_region_size = Vector2i(TILE, TILE)

	# 각 타일 등록 (전체 변형 포함)
	for i in range(total_tiles):
		source.create_tile(Vector2i(i, 0))

	var source_id = tileset.add_source(source)
	tilemap.tile_set = tileset

	# 맵 데이터 배치 (랜덤 변형 선택)
	for y in range(height):
		for x in range(width):
			if y < map_data.size() and x < map_data[y].size():
				var tile_idx = map_data[y][x]
				if tile_idx >= 0 and tile_idx < count:
					# 위치 기반 시드로 결정론적 변형 선택 (세이브/로드 일관성)
					var variation = _position_hash(x, y) % VARIATIONS
					var atlas_idx = tile_idx * VARIATIONS + variation
					tilemap.set_cell(Vector2i(x, y), source_id, Vector2i(atlas_idx, 0))

	return tilemap

## S55: 색상 변형 생성 — 미세한 색조/명도 변화
static func _vary_color(base: Color, variation_index: int) -> Color:
	# 각 변형은 고유한 오프셋 패턴을 가짐
	var offsets = [
		Vector3(0.0, 0.0, 0.0),       # 변형 0: 기본 (사용 안 됨)
		Vector3(0.02, -0.01, 0.01),    # 변형 1: 약간 따뜻하게
		Vector3(-0.01, 0.01, -0.02),   # 변형 2: 약간 차갑게
		Vector3(-0.02, -0.01, 0.02),   # 변형 3: 약간 보라 틴트
	]
	var idx = clampi(variation_index, 0, offsets.size() - 1)
	var off = offsets[idx]
	return Color(
		clampf(base.r + off.x, 0.0, 1.0),
		clampf(base.g + off.y, 0.0, 1.0),
		clampf(base.b + off.z, 0.0, 1.0),
		base.a
	)

## S55: 위치 기반 해시 (결정론적 변형 — 같은 위치는 항상 같은 변형)
static func _position_hash(x: int, y: int) -> int:
	# 간단한 해시: 큰 소수로 혼합
	return absi((x * 73856093) ^ (y * 19349663)) % 2147483647

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
	# S43: 풍부한 풀 텍스처 — 다층 노이즈 + 풀잎 다수
	var dark_grass = _shift(base, -0.04)
	var light_grass = _shift(base, 0.06)
	for y in range(TILE):
		for x in range(TILE):
			# 다층 컬러 변화
			var v1 = sin(x * 0.8 + y * 0.5) * 0.03
			var v2 = randf_range(-0.025, 0.025)
			_px(img, ox + x, oy + y, _shift(base, v1 + v2))
	# 풀잎 다수 (높이 2~4px, 방향 변화)
	for i in range(18):
		var gx = randi_range(1, TILE - 2)
		var gy = randi_range(3, TILE - 1)
		var gc = _shift(base, randf_range(0.06, 0.15))
		var h = randi_range(2, 4)
		for j in range(h):
			_px(img, ox + gx, oy + gy - j, gc)
		# 풀잎 꼭대기 밝게
		_px(img, ox + gx, oy + gy - h + 1, _shift(gc, 0.05))
	# 어두운 풀 뭉치
	for i in range(4):
		var gx = randi_range(2, TILE - 4)
		var gy = randi_range(6, TILE - 2)
		_px(img, ox + gx, oy + gy, dark_grass)
		_px(img, ox + gx + 1, oy + gy, dark_grass)
		_px(img, ox + gx, oy + gy - 1, _shift(dark_grass, 0.03))
	# 꽃 (더 자주, 더 다양)
	if randi_range(0, 2) == 0:
		var fx = randi_range(4, TILE - 5)
		var fy = randi_range(4, TILE - 5)
		var flower_colors = [Color(0.85, 0.75, 0.2), Color(0.75, 0.3, 0.35), Color(0.85, 0.85, 0.5), Color(0.6, 0.3, 0.6)]
		var fc = flower_colors[randi_range(0, flower_colors.size() - 1)]
		_px(img, ox + fx, oy + fy, fc)
		_px(img, ox + fx + 1, oy + fy, _shift(fc, -0.1))
		_px(img, ox + fx, oy + fy + 1, _shift(base, 0.05))  # 줄기

static func _paint_tree(img: Image, ox: int, oy: int, _base: Color) -> void:
	# S43: 나무 — 껍질 텍스처 + 풍성한 수관 + 그림자
	# 줄기 (껍질 텍스처)
	var trunk = Color(0.38, 0.24, 0.14)
	var trunk_dark = _shift(trunk, -0.06)
	var trunk_light = _shift(trunk, 0.04)
	_fill(img, ox + 12, oy + 18, 8, 14, trunk)
	# 나무껍질 디테일
	for y in range(18, 32):
		for x in range(12, 20):
			if randi_range(0, 2) == 0:
				_px(img, ox + x, oy + y, trunk_dark)
			elif randi_range(0, 4) == 0:
				_px(img, ox + x, oy + y, trunk_light)
	# 줄기 좌측 어둡게 (입체감)
	for y in range(18, 32):
		_px(img, ox + 12, oy + y, _shift(trunk, -0.08))
		_px(img, ox + 13, oy + y, trunk_dark)
	# 수관 (다층 원형 — 3겹)
	var canopy = Color(0.15, 0.38, 0.18)
	var canopy_dark = Color(0.08, 0.28, 0.1)
	var canopy_light = Color(0.22, 0.48, 0.25)
	# 뒤쪽 어두운 수관
	for y in range(2, 22):
		for x in range(TILE):
			var dx = x - 16.0
			var dy = y - 11.0
			if dx * dx + dy * dy < 140:
				_px(img, ox + x, oy + y, _shift(canopy_dark, randf_range(-0.02, 0.02)))
	# 메인 수관
	for y in range(1, 20):
		for x in range(TILE):
			var dx = x - 16.0
			var dy = y - 10.0
			if dx * dx + dy * dy < 115:
				var leaf_var = randf_range(-0.03, 0.03)
				var c = canopy if randi_range(0, 2) != 0 else canopy_dark
				_px(img, ox + x, oy + y, _shift(c, leaf_var))
	# 하이라이트 (상단 좌측 — 빛)
	for y in range(3, 11):
		for x in range(8, 17):
			var dx = x - 12.0
			var dy = y - 7.0
			if dx * dx + dy * dy < 18:
				_px(img, ox + x, oy + y, _shift(canopy_light, randf_range(-0.02, 0.02)))
	# 잎 가장자리 디테일
	for i in range(12):
		var lx = randi_range(3, 29)
		var ly = randi_range(1, 19)
		var dx = lx - 16.0
		var dy = ly - 10.0
		if dx * dx + dy * dy > 90 and dx * dx + dy * dy < 150:
			_px(img, ox + lx, oy + ly, _shift(canopy, randf_range(0.04, 0.1)))
	# 나무 그림자 (바닥)
	for x in range(8, 24):
		_px(img, ox + x, oy + 31, _shift(_base, -0.06))
		_px(img, ox + x, oy + 30, _shift(_base, -0.03))

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
	# S43: 풍부한 물 텍스처 — 깊이감 + 다층 파도 + 반짝임
	var deep = _shift(base, -0.06)
	var shallow = _shift(base, 0.04)
	for y in range(TILE):
		for x in range(TILE):
			var wave1 = sin((x + y * 0.5) * 0.5) * 0.04
			var wave2 = sin((x * 0.3 - y * 0.7) * 0.8) * 0.02
			var depth = sin(y * 0.2) * 0.03  # 위에서 아래로 깊이감
			_px(img, ox + x, oy + y, _shift(base, wave1 + wave2 + depth))
	# 파도 라인 (더 많고 부드럽게)
	for i in range(4):
		var wy = 4 + i * 7
		var highlight = Color(0.3, 0.5, 0.7, 0.5)
		for x in range(TILE):
			var offset = int(sin(x * 0.35 + i * 1.7) * 2.5)
			var py = wy + offset
			if py >= 0 and py < TILE:
				_px(img, ox + x, oy + py, highlight)
				if py + 1 < TILE:
					_px(img, ox + x, oy + py + 1, Color(highlight.r, highlight.g, highlight.b, 0.2))
	# 반짝임 하이라이트
	for i in range(3):
		var sx = randi_range(3, TILE - 4)
		var sy = randi_range(3, TILE - 4)
		_px(img, ox + sx, oy + sy, Color(0.5, 0.65, 0.8, 0.6))

static func _paint_path(img: Image, ox: int, oy: int, base: Color) -> void:
	# S43: 길 — 자갈 다수 + 발자국 흔적 + 가장자리 풀
	for y in range(TILE):
		for x in range(TILE):
			var v = randf_range(-0.03, 0.03) + sin(x * 0.6) * 0.01
			_px(img, ox + x, oy + y, _shift(base, v))
	# 자갈 (다수, 다양한 크기)
	for i in range(10):
		var gx = randi_range(2, TILE - 4)
		var gy = randi_range(2, TILE - 4)
		var gs = randi_range(1, 3)
		var gc = _shift(base, randf_range(-0.1, -0.04))
		for dy in range(gs):
			for dx in range(gs):
				_px(img, ox + gx + dx, oy + gy + dy, gc)
	# 발자국 흔적 (희미한)
	if randi_range(0, 3) == 0:
		var fx = randi_range(8, 20)
		var fy = randi_range(8, 24)
		_px(img, ox + fx, oy + fy, _shift(base, -0.04))
		_px(img, ox + fx + 4, oy + fy + 5, _shift(base, -0.04))
	# 가장자리 풀 힌트
	for i in range(3):
		var ex = randi_range(0, 3)
		var ey = randi_range(2, TILE - 3)
		_px(img, ox + ex, oy + ey, _shift(base, 0.06))
		ex = randi_range(TILE - 4, TILE - 1)
		_px(img, ox + ex, oy + ey, _shift(base, 0.06))

static func _paint_stone(img: Image, ox: int, oy: int, base: Color) -> void:
	# S43: 돌바닥 — 균열 + 이끼 + 풍화
	for y in range(TILE):
		for x in range(TILE):
			var v = randf_range(-0.03, 0.03) + sin(x * 0.5 + y * 0.3) * 0.015
			_px(img, ox + x, oy + y, _shift(base, v))
	# 돌 줄눈 (더 굵고 자연스러운 선)
	var grout = _shift(base, -0.1)
	var grout_h = _shift(base, -0.06)
	for x in range(TILE):
		_px(img, ox + x, oy + 15, grout)
		_px(img, ox + x, oy + 16, grout_h)
	for y in range(0, 15):
		_px(img, ox + 16, oy + y, grout)
	for y in range(16, TILE):
		_px(img, ox + 8, oy + y, grout)
		_px(img, ox + 24, oy + y, grout)
	# 균열 (랜덤 위치)
	if randi_range(0, 3) == 0:
		var cx = randi_range(4, 28)
		for dy in range(randi_range(3, 6)):
			cx += randi_range(-1, 1)
			_px(img, ox + clampi(cx, 0, 31), oy + randi_range(2, 29), _shift(base, -0.12))
	# 이끼 흔적
	if randi_range(0, 4) == 0:
		var mx = randi_range(2, 26)
		var my = randi_range(2, 26)
		for dy in range(3):
			for dx in range(randi_range(2, 5)):
				_px(img, ox + mx + dx, oy + my + dy, Color(base.r - 0.02, base.g + 0.04, base.b - 0.02))

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
	# S43: 모래 — 물결 패턴 + 조개/조약돌 + 바람 자국
	for y in range(TILE):
		for x in range(TILE):
			var wave = sin(x * 0.4 + y * 0.3) * 0.03 + sin(x * 0.15 - y * 0.2) * 0.02
			_px(img, ox + x, oy + y, _shift(base, wave + randf_range(-0.02, 0.02)))
	# 바람 자국 (곡선)
	for i in range(3):
		var sy = randi_range(3, TILE - 4)
		for x in range(randi_range(2, 8), randi_range(20, TILE - 1)):
			var offset = int(sin(x * 0.2 + i) * 1.5)
			var py = sy + offset
			if py >= 0 and py < TILE:
				_px(img, ox + x, oy + py, _shift(base, 0.05))
	# 조개/조약돌
	if randi_range(0, 3) == 0:
		var sx = randi_range(4, TILE - 6)
		var sy = randi_range(4, TILE - 6)
		_px(img, ox + sx, oy + sy, Color(0.65, 0.6, 0.55))
		_px(img, ox + sx + 1, oy + sy, Color(0.6, 0.55, 0.5))

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
	# S43: 보이드 — 맥동 에너지 패턴 + 별빛
	for y in range(TILE):
		for x in range(TILE):
			var dist = sqrt(pow(x - 16.0, 2) + pow(y - 16.0, 2)) / 22.0
			var pulse = sin(dist * 8.0) * 0.015
			var noise = randf_range(-0.01, 0.01)
			_px(img, ox + x, oy + y, _shift(base, noise + pulse))
	# 보이드 에너지 점 (더 많고 밝게)
	for i in range(randi_range(1, 4)):
		var sx = randi_range(3, TILE - 4)
		var sy = randi_range(3, TILE - 4)
		var glow = Color(0.2, 0.08, 0.35, 0.8)
		_px(img, ox + sx, oy + sy, glow)
		_px(img, ox + sx + 1, oy + sy, Color(glow.r * 0.5, glow.g * 0.5, glow.b * 0.5, 0.4))

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

## S53: 타일 경계 자동 전환 (오토타일링)
## 서로 다른 타일 인접 시 경계 블렌딩
static func auto_blend_edges(parent: Node2D, map_data: Array, width: int, height: int, tile_colors: Dictionary, tile_size: int = 32) -> Array[ColorRect]:
	var edges: Array[ColorRect] = []
	for y in range(height):
		for x in range(width):
			if y >= map_data.size() or x >= map_data[y].size():
				continue
			var current = map_data[y][x]
			# 오른쪽 인접 타일 체크
			if x + 1 < width and x + 1 < map_data[y].size():
				var right = map_data[y][x + 1]
				if right != current and tile_colors.has(current) and tile_colors.has(right):
					var edge = ColorRect.new()
					edge.size = Vector2(4, tile_size)
					edge.position = Vector2((x + 1) * tile_size - 2, y * tile_size)
					edge.color = tile_colors[current].lerp(tile_colors[right], 0.5)
					edge.color.a = 0.4
					edge.z_index = 2
					parent.add_child(edge)
					edges.append(edge)
			# 아래 인접 타일 체크
			if y + 1 < height and y + 1 < map_data.size() and x < map_data[y + 1].size():
				var below = map_data[y + 1][x]
				if below != current and tile_colors.has(current) and tile_colors.has(below):
					var edge = ColorRect.new()
					edge.size = Vector2(tile_size, 4)
					edge.position = Vector2(x * tile_size, (y + 1) * tile_size - 2)
					edge.color = tile_colors[current].lerp(tile_colors[below], 0.5)
					edge.color.a = 0.4
					edge.z_index = 2
					parent.add_child(edge)
					edges.append(edge)
	return edges
