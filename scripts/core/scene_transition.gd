## SceneTransition (Autoload)
## 씬 전환 처리. 페이드/다이아몬드 와이프/디졸브/커튼/로딩 화면 효과 지원.
## S57: curtain wipe, loading screen, transition diversity.
extends CanvasLayer

var transition_rect: ColorRect
var tween: Tween

# 와이프 효과용
var wipe_rects: Array = []

# S57: Curtain wipe rects
var _curtain_left: ColorRect
var _curtain_right: ColorRect

# S57: Chapter name mapping for loading screen
const CHAPTER_NAMES: Dictionary = {
	1: "Chapter I\nRim Forest",
	2: "Chapter II\nVerdan Market",
	3: "Chapter III\nBelt Waystation",
	4: "Chapter IV\nDrift Shelter",
	5: "Chapter V\nCrumbling Coast",
	6: "Chapter VI\nThe Seam",
	7: "Chapter VII\nSeam Outskirts",
	8: "Chapter VIII\nForgotten Forest",
	9: "Chapter IX\nColorless Waste",
	10: "Chapter X\nBL-07 Void",
}

func _ready() -> void:
	layer = 100  # 최상위 레이어
	_create_transition_rect()
	print("[SceneTransition] Ready")

func _create_transition_rect() -> void:
	transition_rect = ColorRect.new()
	transition_rect.color = Color.BLACK
	transition_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	transition_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	transition_rect.modulate.a = 0.0
	add_child(transition_rect)

## 기본 페이드 전환 (S54: 맵별 스타일 자동 감지 — styled=true일 때)
func change_scene(scene_path: String, duration: float = 0.5, styled: bool = false) -> void:
	# S54: styled 모드에서 맵별 전환 효과 자동 적용
	if styled:
		var style = _detect_style(scene_path)
		if style != "fade":
			await change_scene_styled(scene_path, style)
			return
	if tween:
		tween.kill()
	transition_rect.color = Color.BLACK
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration)
	await tween.finished

	get_tree().change_scene_to_file(scene_path)

	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration)
	await tween.finished

	# S57: 맵 전환 후 테마 파티클 버스트
	_spawn_biome_particles(scene_path)

## 전투 진입용 다이아몬드 와이프 전환 (S57: 전투 줌 추가)
func change_scene_battle(scene_path: String) -> void:
	# S57: 전투 진입 전 카메라 줌인 (플레이어가 있을 때만)
	var player = get_tree().get_first_node_in_group("player")
	if player and player.has_method("battle_zoom_in"):
		await player.battle_zoom_in(0.2)
	await _diamond_wipe_out(0.6)
	get_tree().change_scene_to_file(scene_path)
	await _diamond_wipe_in(0.5)
	# S57: 줌 리셋 (맵 복귀 시 player가 새로 로드되므로 자동 리셋됨)

## 다이아몬드 와이프 아웃 (화면 덮기)
func _diamond_wipe_out(duration: float) -> void:
	_clear_wipe_rects()

	var cols = 16
	var rows = 9
	var cell_w = 1280.0 / cols
	var cell_h = 720.0 / rows
	var max_delay = duration * 0.6

	for y in range(rows):
		for x in range(cols):
			var rect = ColorRect.new()
			rect.size = Vector2(cell_w + 2, cell_h + 2)
			rect.position = Vector2(x * cell_w - 1, y * cell_h - 1)
			rect.color = Color.BLACK
			rect.scale = Vector2.ZERO
			rect.pivot_offset = rect.size / 2.0
			rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
			add_child(rect)
			wipe_rects.append(rect)

			# 중앙에서 퍼져나가는 딜레이
			var cx = (x - cols / 2.0) / (cols / 2.0)
			var cy = (y - rows / 2.0) / (rows / 2.0)
			var dist = sqrt(cx * cx + cy * cy) / 1.42  # 0~1 정규화
			var delay = dist * max_delay

			var t = create_tween()
			t.tween_property(rect, "scale", Vector2(1.0, 1.0), duration * 0.35).set_delay(delay).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

	await get_tree().create_timer(duration).timeout

## 다이아몬드 와이프 인 (화면 열기)
func _diamond_wipe_in(duration: float) -> void:
	if wipe_rects.is_empty():
		return

	var cols = 16
	var rows = 9
	var max_delay = duration * 0.5

	for rect in wipe_rects:
		if not is_instance_valid(rect):
			continue
		var pos = rect.position
		var cx = (pos.x / 1280.0 - 0.5) * 2.0
		var cy = (pos.y / 720.0 - 0.5) * 2.0
		var dist = sqrt(cx * cx + cy * cy) / 1.42
		var delay = dist * max_delay

		var t = create_tween()
		t.tween_property(rect, "scale", Vector2.ZERO, duration * 0.3).set_delay(delay).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
		t.tween_callback(rect.queue_free)

	await get_tree().create_timer(duration + 0.1).timeout
	wipe_rects.clear()

func _clear_wipe_rects() -> void:
	for rect in wipe_rects:
		if is_instance_valid(rect):
			rect.queue_free()
	wipe_rects.clear()

## S40: 원형 와이프 전환 (아이리스) — CG/보스전 전환용
func change_scene_iris(scene_path: String, duration: float = 0.8) -> void:
	await _iris_wipe_out(duration)
	get_tree().change_scene_to_file(scene_path)
	await _iris_wipe_in(duration * 0.7)

func _iris_wipe_out(duration: float) -> void:
	var iris = ColorRect.new()
	iris.set_anchors_preset(Control.PRESET_FULL_RECT)
	iris.color = Color(0, 0, 0, 0)  # 셰이더가 알파 제어
	iris.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var shader_code = """
shader_type canvas_item;
uniform float radius : hint_range(0.0, 1.5) = 1.2;
uniform vec2 center = vec2(0.5, 0.5);
uniform float edge_softness : hint_range(0.0, 0.1) = 0.03;
void fragment() {
	vec2 uv = UV - center;
	uv.x *= SCREEN_PIXEL_SIZE.y / SCREEN_PIXEL_SIZE.x;
	float dist = length(uv);
	float alpha = smoothstep(radius - edge_softness, radius, dist);
	COLOR = vec4(0.0, 0.0, 0.0, alpha);
}
"""
	var shader = Shader.new()
	shader.code = shader_code
	var mat = ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("radius", 1.2)
	mat.set_shader_parameter("center", Vector2(0.5, 0.5))
	iris.material = mat
	add_child(iris)

	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("radius", val), 1.2, 0.0, duration).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	await t.finished
	iris.queue_free()

func _iris_wipe_in(duration: float) -> void:
	var iris = ColorRect.new()
	iris.set_anchors_preset(Control.PRESET_FULL_RECT)
	iris.color = Color(0, 0, 0, 0)  # 셰이더가 알파 제어
	iris.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var shader_code = """
shader_type canvas_item;
uniform float radius : hint_range(0.0, 1.5) = 0.0;
uniform vec2 center = vec2(0.5, 0.5);
uniform float edge_softness : hint_range(0.0, 0.1) = 0.03;
void fragment() {
	vec2 uv = UV - center;
	uv.x *= SCREEN_PIXEL_SIZE.y / SCREEN_PIXEL_SIZE.x;
	float dist = length(uv);
	float alpha = smoothstep(radius - edge_softness, radius, dist);
	COLOR = vec4(0.0, 0.0, 0.0, alpha);
}
"""
	var shader = Shader.new()
	shader.code = shader_code
	var mat = ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("radius", 0.0)
	mat.set_shader_parameter("center", Vector2(0.5, 0.5))
	iris.material = mat
	add_child(iris)

	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("radius", val), 0.0, 1.2, duration).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	await t.finished
	iris.queue_free()

## S54/S57: 맵별 전환 스타일 자동 감지 — curtain added for markets/shelters
const TRANSITION_STYLES: Dictionary = {
	"bl07_void": "glitch",
	"seam_outskirts": "glitch",
	"rim_forest": "leaves",
	"forgotten_forest": "leaves",
	"colorless_waste": "dust",
	"crumbling_coast": "dust",
	"drift_shelter": "curtain",
	"belt_waystation": "curtain",
	"verdan_market": "curtain",
	"the_seam": "mist",
}

## S54/S57: 맵 전환 스타일 색상/설정 — curtain added
const STYLE_CONFIGS: Dictionary = {
	"fade": {"color": Color.BLACK, "duration": 0.5},
	"glitch": {"color": Color(0.15, 0.0, 0.2, 1.0), "duration": 0.35},
	"leaves": {"color": Color(0.1, 0.2, 0.08, 1.0), "duration": 0.6},
	"dust": {"color": Color(0.25, 0.18, 0.1, 1.0), "duration": 0.55},
	"mist": {"color": Color(0.7, 0.75, 0.85, 1.0), "duration": 0.7},
	"curtain": {"color": Color(0.03, 0.02, 0.05), "duration": 0.6},
}

## S54: 스타일 자동 감지 — 씬 경로에서 맵 이름 추출
func _detect_style(scene_path: String) -> String:
	for map_key in TRANSITION_STYLES:
		if map_key in scene_path:
			return TRANSITION_STYLES[map_key]
	return "fade"

## S54: 맵별 전환 효과 적용 씬 전환
func change_scene_styled(scene_path: String, style: String = "") -> void:
	if style == "":
		style = _detect_style(scene_path)
	var config = STYLE_CONFIGS.get(style, STYLE_CONFIGS["fade"])
	var dur: float = config["duration"]

	if style == "curtain":
		# S57: Curtain wipe for indoor/market maps
		await _curtain_close(dur)
		get_tree().change_scene_to_file(scene_path)
		await _curtain_open(dur * 0.8)
	elif style == "glitch":
		await _glitch_transition_out(dur)
		get_tree().change_scene_to_file(scene_path)
		await _glitch_transition_in(dur * 0.8)
	elif style == "leaves":
		transition_rect.color = config["color"]
		transition_rect.modulate = Color(0.6, 1.0, 0.5, 0.0)  # green tint
		await _tinted_fade_out(dur)
		get_tree().change_scene_to_file(scene_path)
		await _tinted_fade_in(dur)
		transition_rect.color = Color.BLACK
		transition_rect.modulate = Color(1, 1, 1, 0)
	elif style == "dust":
		transition_rect.color = config["color"]
		transition_rect.modulate = Color(1.0, 0.85, 0.6, 0.0)  # sandy tint
		await _tinted_fade_out(dur)
		get_tree().change_scene_to_file(scene_path)
		await _tinted_fade_in(dur)
		transition_rect.color = Color.BLACK
		transition_rect.modulate = Color(1, 1, 1, 0)
	elif style == "mist":
		transition_rect.color = config["color"]
		transition_rect.modulate = Color(1, 1, 1, 0)
		await _slow_mist_out(dur)
		get_tree().change_scene_to_file(scene_path)
		await _slow_mist_in(dur)
		transition_rect.color = Color.BLACK
		transition_rect.modulate = Color(1, 1, 1, 0)
	else:
		await change_scene(scene_path, dur)

	# S57: 전환 완료 후 테마 파티클 (change_scene 경유 시 중복 방지 — else만)
	if style != "fade":
		_spawn_biome_particles(scene_path)

## S54: Glitch transition — rapid flash/noise for void maps
func _glitch_transition_out(duration: float) -> void:
	transition_rect.color = Color(0.15, 0.0, 0.2)
	# Rapid flashes
	var flash_count: int = 6
	var flash_dur: float = duration / float(flash_count)
	for i in range(flash_count):
		transition_rect.modulate.a = randf_range(0.3, 0.9) if i % 2 == 0 else randf_range(0.0, 0.3)
		transition_rect.color = Color(randf_range(0.05, 0.2), 0.0, randf_range(0.1, 0.3))
		await get_tree().create_timer(flash_dur).timeout
	# Final solid
	transition_rect.color = Color(0.08, 0.0, 0.12)
	transition_rect.modulate.a = 1.0
	await get_tree().create_timer(0.1).timeout

func _glitch_transition_in(duration: float) -> void:
	var flash_count: int = 4
	var flash_dur: float = duration / float(flash_count)
	for i in range(flash_count):
		transition_rect.modulate.a = randf_range(0.4, 0.8) if i % 2 == 0 else randf_range(0.1, 0.5)
		transition_rect.color = Color(randf_range(0.05, 0.15), 0.0, randf_range(0.08, 0.2))
		await get_tree().create_timer(flash_dur).timeout
	transition_rect.color = Color.BLACK
	transition_rect.modulate.a = 0.0

## S54: Tinted fade (leaves/dust)
func _tinted_fade_out(duration: float) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	await tween.finished

func _tinted_fade_in(duration: float) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	await tween.finished

## S54: Slow mist fade (shelter/waystation) — slower with easing
func _slow_mist_out(duration: float) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
	await tween.finished

func _slow_mist_in(duration: float) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
	await tween.finished

## 페이드 아웃만 (컷씬 전환용)
func fade_out(duration: float = 0.5) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration)
	await tween.finished

## 페이드 인만
func fade_in(duration: float = 0.5) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration)
	await tween.finished

# ══════════════════════════════════════════════════════════════
# S57: CURTAIN WIPE — two halves slide apart/together
# ══════════════════════════════════════════════════════════════

func change_scene_curtain(scene_path: String, duration: float = 0.6) -> void:
	await _curtain_close(duration)
	get_tree().change_scene_to_file(scene_path)
	await _curtain_open(duration * 0.8)

func _curtain_close(duration: float) -> void:
	_curtain_left = ColorRect.new()
	_curtain_left.color = Color(0.03, 0.02, 0.05)
	_curtain_left.position = Vector2(-640, 0)
	_curtain_left.size = Vector2(640, 720)
	_curtain_left.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_curtain_left)

	_curtain_right = ColorRect.new()
	_curtain_right.color = Color(0.03, 0.02, 0.05)
	_curtain_right.position = Vector2(1280, 0)
	_curtain_right.size = Vector2(640, 720)
	_curtain_right.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_curtain_right)

	var t = create_tween().set_parallel(true)
	t.tween_property(_curtain_left, "position:x", 0.0, duration).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	t.tween_property(_curtain_right, "position:x", 640.0, duration).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)
	await t.finished

func _curtain_open(duration: float) -> void:
	if not is_instance_valid(_curtain_left) or not is_instance_valid(_curtain_right):
		return
	var t = create_tween().set_parallel(true)
	t.tween_property(_curtain_left, "position:x", -640.0, duration).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	t.tween_property(_curtain_right, "position:x", 1280.0, duration).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_QUAD)
	await t.finished
	if is_instance_valid(_curtain_left):
		_curtain_left.queue_free()
	if is_instance_valid(_curtain_right):
		_curtain_right.queue_free()

# ══════════════════════════════════════════════════════════════
# S57: LOADING SCREEN — stylized chapter name with breathing animation
# ══════════════════════════════════════════════════════════════

## Show a brief loading screen between major scene transitions.
## Displays chapter name with breathing animation, holds 1s minimum.
func change_scene_with_loading(scene_path: String, chapter_num: int = -1) -> void:
	# Determine chapter from GameManager if not provided
	if chapter_num < 0:
		chapter_num = GameManager.current_chapter

	# Fade to black
	if tween:
		tween.kill()
	transition_rect.color = Color.BLACK
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, 0.4)
	await tween.finished

	# Build loading overlay
	var loading_overlay = ColorRect.new()
	loading_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	loading_overlay.color = Color(0.03, 0.02, 0.05)
	loading_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(loading_overlay)

	# Chapter title label
	var chapter_text = CHAPTER_NAMES.get(chapter_num, "Chapter %d" % chapter_num)
	var title_lbl = Label.new()
	title_lbl.text = chapter_text
	title_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	title_lbl.set_anchors_preset(Control.PRESET_CENTER)
	title_lbl.position = Vector2(-200, -40)
	title_lbl.size = Vector2(400, 80)
	title_lbl.add_theme_font_size_override("font_size", 28)
	title_lbl.add_theme_color_override("font_color", Color(0.7, 0.6, 0.45, 0.0))
	title_lbl.add_theme_color_override("font_outline_color", Color(0.2, 0.15, 0.1))
	title_lbl.add_theme_constant_override("outline_size", 2)
	title_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	loading_overlay.add_child(title_lbl)

	# Small decorative line
	var line = ColorRect.new()
	line.set_anchors_preset(Control.PRESET_CENTER)
	line.position = Vector2(-40, 30)
	line.size = Vector2(80, 1)
	line.color = Color(0.5, 0.4, 0.3, 0.0)
	line.mouse_filter = Control.MOUSE_FILTER_IGNORE
	loading_overlay.add_child(line)

	# Fade in title with breathing
	var load_tween = create_tween()
	load_tween.tween_property(title_lbl, "theme_override_colors/font_color",
		Color(0.75, 0.65, 0.45, 1.0), 0.5).set_ease(Tween.EASE_OUT)
	load_tween.parallel().tween_property(line, "color:a", 0.5, 0.5)

	# Breathing pulse
	var breath = create_tween().set_loops(2)
	breath.tween_property(title_lbl, "theme_override_colors/font_color",
		Color(0.85, 0.72, 0.5, 1.0), 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE).set_delay(0.5)
	breath.tween_property(title_lbl, "theme_override_colors/font_color",
		Color(0.65, 0.55, 0.4, 0.8), 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)

	# Actually load scene (during the 1s hold)
	get_tree().change_scene_to_file(scene_path)

	# Hold for at least 1 second
	await get_tree().create_timer(1.2).timeout

	# Kill breathing tween
	if breath and breath.is_valid():
		breath.kill()

	# Fade out loading screen
	var fade_tw = create_tween()
	fade_tw.tween_property(loading_overlay, "modulate:a", 0.0, 0.4).set_ease(Tween.EASE_OUT)
	fade_tw.parallel().tween_property(transition_rect, "modulate:a", 0.0, 0.4)
	await fade_tw.finished
	loading_overlay.queue_free()

	# S57: 맵 전환 후 테마 파티클 버스트
	_spawn_biome_particles(scene_path)

## ===================== S58: Chapter Completion Screen =====================

## Show a brief "Chapter X Complete" screen with stats before transitioning.
## Called from map scripts instead of directly transitioning.
func change_scene_chapter_complete(scene_path: String, completed_chapter: int) -> void:
	# Gather chapter stats from GameManager
	var stats = GameManager.get_chapter_stats()
	var battles = stats.get("battles", 0)
	var burns = stats.get("burns", 0)
	var time_secs = stats.get("time_seconds", 0.0)
	var mins = int(time_secs) / 60
	var secs = int(time_secs) % 60

	# Fade to black
	if tween:
		tween.kill()
	transition_rect.color = Color.BLACK
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, 0.4)
	await tween.finished

	# Build completion overlay
	var overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.03, 0.02, 0.05)
	overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(overlay)

	# "Chapter X Complete" title
	var ch_name = CHAPTER_NAMES.get(completed_chapter, "Chapter %d" % completed_chapter)
	var title_lbl = Label.new()
	title_lbl.text = ch_name.split("\n")[0] if "\n" in ch_name else ch_name
	title_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	title_lbl.set_anchors_preset(Control.PRESET_CENTER)
	title_lbl.position = Vector2(-200, -80)
	title_lbl.size = Vector2(400, 40)
	title_lbl.add_theme_font_size_override("font_size", 28)
	title_lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5, 0.0))
	title_lbl.add_theme_color_override("font_outline_color", Color(0.15, 0.1, 0.05))
	title_lbl.add_theme_constant_override("outline_size", 2)
	title_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(title_lbl)

	# "Complete" subtitle
	var complete_lbl = Label.new()
	complete_lbl.text = "Complete"
	complete_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	complete_lbl.set_anchors_preset(Control.PRESET_CENTER)
	complete_lbl.position = Vector2(-200, -45)
	complete_lbl.size = Vector2(400, 30)
	complete_lbl.add_theme_font_size_override("font_size", 18)
	complete_lbl.add_theme_color_override("font_color", Color(0.65, 0.55, 0.4, 0.0))
	complete_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(complete_lbl)

	# Decorative separator line
	var line = ColorRect.new()
	line.set_anchors_preset(Control.PRESET_CENTER)
	line.position = Vector2(-50, -15)
	line.size = Vector2(100, 1)
	line.color = Color(0.5, 0.4, 0.3, 0.0)
	line.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(line)

	# Stats labels — battles, burns, time
	var stats_data: Array = [
		{"label": "Battles Won", "value": str(battles)},
		{"label": "Memories Burned", "value": str(burns)},
		{"label": "Time", "value": "%d:%02d" % [mins, secs]},
	]
	var stat_labels: Array = []
	var y_offset = 0
	for i in range(stats_data.size()):
		var stat = stats_data[i]
		var row = HBoxContainer.new()
		row.set_anchors_preset(Control.PRESET_CENTER)
		row.position = Vector2(-120, y_offset)
		row.size = Vector2(240, 24)
		row.mouse_filter = Control.MOUSE_FILTER_IGNORE

		var name_lbl = Label.new()
		name_lbl.text = stat["label"]
		name_lbl.add_theme_font_size_override("font_size", 14)
		name_lbl.add_theme_color_override("font_color", Color(0.55, 0.5, 0.4, 0.0))
		name_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		name_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		row.add_child(name_lbl)

		var val_lbl = Label.new()
		val_lbl.text = stat["value"]
		val_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val_lbl.add_theme_font_size_override("font_size", 14)
		val_lbl.add_theme_color_override("font_color", Color(0.8, 0.7, 0.5, 0.0))
		val_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		val_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		row.add_child(val_lbl)

		overlay.add_child(row)
		stat_labels.append({"name": name_lbl, "value": val_lbl})
		y_offset += 28

	# Animate: fade in title
	var tw1 = create_tween()
	tw1.tween_property(title_lbl, "theme_override_colors/font_color",
		Color(0.85, 0.75, 0.5, 1.0), 0.4).set_ease(Tween.EASE_OUT)

	# Complete label fade in (delayed)
	var tw2 = create_tween()
	tw2.tween_interval(0.3)
	tw2.tween_property(complete_lbl, "theme_override_colors/font_color",
		Color(0.65, 0.55, 0.4, 1.0), 0.3)

	# Separator line
	var tw_line = create_tween()
	tw_line.tween_interval(0.5)
	tw_line.tween_property(line, "color:a", 0.5, 0.3)

	# Stats appear one by one
	for i in range(stat_labels.size()):
		var sl = stat_labels[i]
		var tw_stat = create_tween()
		tw_stat.tween_interval(0.7 + i * 0.3)
		tw_stat.tween_property(sl["name"], "theme_override_colors/font_color",
			Color(0.55, 0.5, 0.4, 1.0), 0.25).set_ease(Tween.EASE_OUT)
		tw_stat.parallel().tween_property(sl["value"], "theme_override_colors/font_color",
			Color(0.8, 0.7, 0.5, 1.0), 0.25).set_ease(Tween.EASE_OUT)

	# Hold for display, then fade out and transition
	await get_tree().create_timer(3.0).timeout

	# Fade out overlay
	var tw_out = create_tween()
	tw_out.tween_property(overlay, "modulate:a", 0.0, 0.5).set_ease(Tween.EASE_IN)
	await tw_out.finished
	overlay.queue_free()

	# Mark new chapter start for next tracking
	GameManager.mark_chapter_start()

	# Now do the actual scene transition
	get_tree().change_scene_to_file(scene_path)

	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, 0.4)
	await tween.finished

	_spawn_biome_particles(scene_path)

## ===================== S57: 맵 전환 테마 파티클 =====================

# 맵 바이옴 감지
const BIOME_MAP: Dictionary = {
	"rim_forest": "forest",
	"forgotten_forest": "forest",
	"verdan_market": "forest",
	"belt_waystation": "belt",
	"crumbling_coast": "coast",
	"colorless_waste": "coast",
	"bl07_void": "void",
	"seam_outskirts": "void",
	"the_seam": "void",
	"drift_shelter": "shelter",
}

func _detect_biome(scene_path: String) -> String:
	for map_key in BIOME_MAP:
		if map_key in scene_path:
			return BIOME_MAP[map_key]
	return ""

## 씬 전환 후 테마 파티클 스폰 (전투 씬 제외)
func _spawn_biome_particles(scene_path: String) -> void:
	if "battle" in scene_path:
		return
	var biome = _detect_biome(scene_path)
	if biome == "":
		return
	var scene = get_tree().current_scene
	if scene:
		MapEffects.spawn_transition_particles(scene, biome)
