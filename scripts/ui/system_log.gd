## SystemLog (Autoload) — 관리국 감지 로그
## 기억 연소 시 화면 상단에 팝업으로 표시.
## [COMBUSTION DETECTED: ...] 스타일의 관리국 로그.
extends CanvasLayer

const DISPLAY_DURATION: float = 3.5  # 표시 유지 시간
const FADE_DURATION: float = 0.8

const GRADE_TYPE_NAMES = ["Sensory Fragment", "Daily Record", "Relational Bond", "Identity Anchor", "Core Memory"]

var log_panel: PanelContainer
var log_label: RichTextLabel
var tween: Tween
var queue: Array = []  # 대기열 (연속 연소 시)
var is_showing: bool = false

func _ready() -> void:
	layer = 60  # DialogueBox(50) 위, SceneTransition(100) 아래
	_build_ui()
	log_panel.modulate.a = 0.0
	log_panel.visible = false
	MemoryManager.memory_burned.connect(_on_memory_burned)
	print("[SystemLog] Bureau Detection Log ready")

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	log_panel = PanelContainer.new()
	log_panel.anchor_left = 0.15
	log_panel.anchor_right = 0.85
	log_panel.anchor_top = 0.0
	log_panel.anchor_bottom = 0.0
	log_panel.offset_top = 16
	log_panel.offset_bottom = 100
	log_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

	# 관리국 스타일 — 어두운 배경, 청록/녹색 테두리
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.03, 0.05, 0.06, 0.92)
	style.border_color = Color(0.2, 0.5, 0.45, 0.7)
	style.set_border_width_all(1)
	style.border_width_top = 2
	style.set_corner_radius_all(2)
	style.set_content_margin_all(12)
	log_panel.add_theme_stylebox_override("panel", style)
	root.add_child(log_panel)

	log_label = RichTextLabel.new()
	log_label.bbcode_enabled = true
	log_label.fit_content = true
	log_label.scroll_active = false
	log_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	log_label.add_theme_font_size_override("normal_font_size", 13)
	log_label.add_theme_color_override("default_color", Color(0.3, 0.7, 0.6))
	log_panel.add_child(log_label)

## 기억 연소 시그널 핸들러
func _on_memory_burned(memory) -> void:
	var grade_name = GRADE_TYPE_NAMES[memory.grade]
	var residue_status = "trace detected — fading" if memory.is_residue else "no residue — permanent loss"

	var log_text = "[color=#4a9a8a][COMBUSTION DETECTED: Type %d — %s][/color]\n" % [memory.grade + 1, grade_name]
	log_text += "[color=#3a7a6a][SUBJECT: %s][/color]\n" % memory.title
	log_text += "[color=#2a6a5a][RESIDUAL RECORD: %s][/color]" % residue_status

	queue.append(log_text)
	if not is_showing:
		_show_next()

## 대기열에서 다음 로그 표시
func _show_next() -> void:
	if queue.is_empty():
		is_showing = false
		return

	is_showing = true
	var text = queue.pop_front()
	log_label.text = text
	log_panel.visible = true

	# 페이드 인
	if tween:
		tween.kill()
	tween = create_tween()
	tween.tween_property(log_panel, "modulate:a", 1.0, FADE_DURATION * 0.5)
	tween.tween_interval(DISPLAY_DURATION)
	tween.tween_property(log_panel, "modulate:a", 0.0, FADE_DURATION)
	tween.tween_callback(_on_display_finished)

func _on_display_finished() -> void:
	log_panel.visible = false
	# 대기열에 더 있으면 다음 표시
	if not queue.is_empty():
		_show_next()
	else:
		is_showing = false
