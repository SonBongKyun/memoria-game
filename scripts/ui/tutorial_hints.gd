## TutorialHints (Autoload) — 첫 경험 시 컨텍스트 힌트 표시
## 주요 첫 순간에 팝업 힌트를 표시하고, 이미 본 힌트는 다시 표시하지 않음.
extends CanvasLayer

var shown_hints: Array = []  # 이미 표시된 힌트 ID 목록 (SaveManager 연동)

# 힌트 정의
const HINTS: Dictionary = {
	"first_battle": "Press Attack to strike, or Burn a memory for powerful skills.",
	"first_burn": "Burning memories is powerful but permanent. Choose wisely.",
	"first_shop": "Trade Grains for memories and items. Sell what you don't need.",
	"first_equipment": "Equip gear from the shop to boost your stats.",
	"first_status_effect": "Status effects last several turns. Use Antidote to cure poison.",
}

# UI 노드
var _panel: PanelContainer
var _label: Label
var _timer: Timer
var _root: Control
var _dismiss_tween: Tween

func _ready() -> void:
	layer = 58  # PauseMenu(55) 위, 대부분의 UI 위
	process_mode = Node.PROCESS_MODE_ALWAYS
	_build_ui()
	_hide_ui()
	print("[TutorialHints] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if _panel and _panel.visible:
		if event is InputEventKey and event.pressed:
			_dismiss()
			get_viewport().set_input_as_handled()
		elif event is InputEventMouseButton and event.pressed:
			_dismiss()
			get_viewport().set_input_as_handled()

## 힌트 표시 (이미 표시된 힌트는 무시)
func show_hint(hint_id: String) -> void:
	if hint_id in shown_hints:
		return
	if not HINTS.has(hint_id):
		return
	shown_hints.append(hint_id)
	_show_panel(HINTS[hint_id])
	print("[TutorialHints] Showing hint: %s" % hint_id)

func _show_panel(text: String) -> void:
	if _dismiss_tween and _dismiss_tween.is_valid():
		_dismiss_tween.kill()
	_label.text = text
	_panel.visible = true
	_panel.modulate.a = 0.0
	_panel.position.y = -60
	var tw = create_tween().set_parallel(true)
	tw.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	tw.tween_property(_panel, "modulate:a", 1.0, 0.3).set_ease(Tween.EASE_OUT)
	tw.tween_property(_panel, "position:y", 10.0, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	_timer.start(4.0)

func _dismiss() -> void:
	if not _panel.visible:
		return
	_timer.stop()
	if _dismiss_tween and _dismiss_tween.is_valid():
		_dismiss_tween.kill()
	_dismiss_tween = create_tween().set_parallel(true)
	_dismiss_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	_dismiss_tween.tween_property(_panel, "modulate:a", 0.0, 0.25)
	_dismiss_tween.tween_property(_panel, "position:y", -60.0, 0.25).set_ease(Tween.EASE_IN)
	_dismiss_tween.chain().tween_callback(func(): _panel.visible = false)

func _hide_ui() -> void:
	if _panel:
		_panel.visible = false

func _build_ui() -> void:
	_root = Control.new()
	_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_root)

	_panel = PanelContainer.new()
	_panel.anchor_left = 0.15
	_panel.anchor_right = 0.85
	_panel.anchor_top = 0.0
	_panel.anchor_bottom = 0.0
	_panel.offset_top = 10
	_panel.offset_bottom = 60
	_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.06, 0.12, 0.92)
	style.border_color = Color(0.65, 0.5, 0.25, 0.8)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(14)
	_panel.add_theme_stylebox_override("panel", style)
	_root.add_child(_panel)

	_label = Label.new()
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_label.add_theme_font_size_override("font_size", 15)
	_label.add_theme_color_override("font_color", Color(0.9, 0.82, 0.55))
	_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	_panel.add_child(_label)

	_timer = Timer.new()
	_timer.one_shot = true
	_timer.timeout.connect(_dismiss)
	add_child(_timer)

## 세이브 데이터 내보내기
func export_data() -> Dictionary:
	return {"shown_hints": shown_hints.duplicate()}

## 세이브 데이터 불러오기
func import_data(data: Dictionary) -> void:
	if data.has("shown_hints"):
		shown_hints = data["shown_hints"]
