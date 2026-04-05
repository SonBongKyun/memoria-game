## SceneTransition (Autoload)
## 씬 전환 (페이드 인/아웃) 처리.
extends CanvasLayer

var transition_rect: ColorRect
var tween: Tween

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

## 페이드 아웃 → 씬 전환 → 페이드 인
func change_scene(scene_path: String, duration: float = 0.5) -> void:
	# 페이드 아웃 (화면 어두워짐)
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 1.0, duration)
	await tween.finished

	# 씬 전환
	get_tree().change_scene_to_file(scene_path)

	# 페이드 인 (화면 밝아짐)
	tween = create_tween()
	tween.tween_property(transition_rect, "modulate:a", 0.0, duration)
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
