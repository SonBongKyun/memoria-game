## CgViewer (Autoload) — CG 이벤트 시스템
## 풀스크린 CG 표시 + 텍스트 오버레이. 스토리 핵심 장면 연출용.
extends CanvasLayer

const FADE_DURATION: float = 0.6

var is_showing: bool = false
var auto_close_timer: float = 0.0
var waiting_for_input: bool = false

# UI 노드
var bg: ColorRect
var cg_texture: TextureRect
var overlay_label: RichTextLabel
var tween: Tween

# 콜백 (CG 닫힌 후 실행)
var _on_closed_callback: Callable

signal cg_shown(image_path: String)
signal cg_closed()

func _ready() -> void:
	layer = 45  # MemoryUI(40)와 DialogueBox(50) 사이
	_build_ui()
	_hide_all()

	# DialogueManager와 연동
	DialogueManager.dialogue_line.connect(_on_dialogue_line)
	print("[CgViewer] Ready")

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# 검은 배경
	bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0, 0, 0, 1)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(bg)

	# CG 이미지
	cg_texture = TextureRect.new()
	cg_texture.set_anchors_preset(Control.PRESET_FULL_RECT)
	cg_texture.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	cg_texture.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	cg_texture.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(cg_texture)

	# 텍스트 오버레이 (하단)
	var text_panel = PanelContainer.new()
	text_panel.anchor_left = 0.1
	text_panel.anchor_right = 0.9
	text_panel.anchor_top = 0.75
	text_panel.anchor_bottom = 0.95
	text_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0, 0, 0, 0.7)
	style.set_content_margin_all(16)
	style.set_corner_radius_all(4)
	text_panel.add_theme_stylebox_override("panel", style)
	text_panel.visible = false
	root.add_child(text_panel)

	overlay_label = RichTextLabel.new()
	overlay_label.bbcode_enabled = false
	overlay_label.fit_content = false
	overlay_label.scroll_active = false
	overlay_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay_label.add_theme_font_size_override("normal_font_size", 16)
	overlay_label.add_theme_color_override("default_color", Color(0.9, 0.87, 0.82))
	text_panel.add_child(overlay_label)

	# text_panel 참조 저장
	overlay_label.set_meta("panel", text_panel)

## CG 표시 — image_path: res:// 경로, text: 오버레이 텍스트 (빈 문자열이면 숨김)
func show_cg(image_path: String, text: String = "", auto_close_sec: float = 0.0, callback: Callable = Callable()) -> void:
	if not ResourceLoader.exists(image_path):
		push_error("[CgViewer] Image not found: %s" % image_path)
		return

	var tex = load(image_path)
	cg_texture.texture = tex
	_on_closed_callback = callback

	# 텍스트 오버레이
	var text_panel = overlay_label.get_meta("panel") as PanelContainer
	if text != "":
		overlay_label.text = text
		text_panel.visible = true
	else:
		text_panel.visible = false

	# 페이드 인
	bg.modulate.a = 0.0
	cg_texture.modulate.a = 0.0
	bg.visible = true
	cg_texture.visible = true
	is_showing = true

	if tween:
		tween.kill()
	tween = create_tween()
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 1.0, FADE_DURATION)
	await tween.finished

	# 자동 닫기 or 입력 대기
	if auto_close_sec > 0:
		await get_tree().create_timer(auto_close_sec).timeout
		close_cg()
	else:
		waiting_for_input = true

	cg_shown.emit(image_path)

## CG 닫기
func close_cg() -> void:
	if not is_showing:
		return

	waiting_for_input = false

	if tween:
		tween.kill()
	tween = create_tween()
	tween.set_parallel(true)
	tween.tween_property(bg, "modulate:a", 0.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 0.0, FADE_DURATION)
	await tween.finished

	_hide_all()
	is_showing = false
	cg_closed.emit()

	if _on_closed_callback.is_valid():
		_on_closed_callback.call()
		_on_closed_callback = Callable()

func _hide_all() -> void:
	bg.visible = false
	cg_texture.visible = false
	var text_panel = overlay_label.get_meta("panel") as PanelContainer
	text_panel.visible = false

## 입력: Space로 CG 닫기
func _unhandled_input(event: InputEvent) -> void:
	if not is_showing or not waiting_for_input:
		return

	if event.is_action_pressed("interact"):
		close_cg()
		get_viewport().set_input_as_handled()

## DialogueManager 연동: 대화 라인에 "cg" 키가 있으면 배경으로 표시
func _on_dialogue_line(_speaker: String, _text: String, _portrait: String) -> void:
	if DialogueManager.current_index < DialogueManager.current_dialogue.size():
		var line = DialogueManager.current_dialogue[DialogueManager.current_index]
		if line.has("cg"):
			_show_cg_background(line.cg)

## 대화 중 CG 배경 표시 (입력 차단 없음 — 대화와 동시 진행)
func _show_cg_background(image_path: String) -> void:
	if not ResourceLoader.exists(image_path):
		return
	var tex = load(image_path)
	cg_texture.texture = tex

	bg.modulate.a = 0.0
	cg_texture.modulate.a = 0.0
	bg.visible = true
	cg_texture.visible = true
	is_showing = true
	waiting_for_input = false  # 대화 중이므로 입력 차단 안 함

	if tween:
		tween.kill()
	tween = create_tween().set_parallel(true)
	tween.tween_property(bg, "modulate:a", 1.0, FADE_DURATION)
	tween.tween_property(cg_texture, "modulate:a", 1.0, FADE_DURATION)

	# 대화 끝나면 CG도 닫기
	if not DialogueManager.dialogue_ended.is_connected(_on_dialogue_ended_close_cg):
		DialogueManager.dialogue_ended.connect(_on_dialogue_ended_close_cg, CONNECT_ONE_SHOT)

func _on_dialogue_ended_close_cg() -> void:
	if is_showing:
		close_cg()
