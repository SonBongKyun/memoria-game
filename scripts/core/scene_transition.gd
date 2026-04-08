## SceneTransition (Autoload)
## 씬 전환 처리. 페이드/다이아몬드 와이프/디졸브 효과 지원.
extends CanvasLayer

var transition_rect: ColorRect
var tween: Tween

# 와이프 효과용
var wipe_rects: Array = []

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

## 기본 페이드 전환
func change_scene(scene_path: String, duration: float = 0.5) -> void:
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration)
	await tween.finished

	get_tree().change_scene_to_file(scene_path)

	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration)
	await tween.finished

## 전투 진입용 다이아몬드 와이프 전환
func change_scene_battle(scene_path: String) -> void:
	await _diamond_wipe_out(0.6)
	get_tree().change_scene_to_file(scene_path)
	await _diamond_wipe_in(0.5)

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
	iris.color = Color.BLACK
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
	iris.color = Color.BLACK
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
