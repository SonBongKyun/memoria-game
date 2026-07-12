## TutorialHints (Autoload) — 첫 경험 시 컨텍스트 힌트 표시
## 주요 첫 순간에 팝업 힌트를 표시하고, 이미 본 힌트는 다시 표시하지 않음.
extends CanvasLayer

const HINT_BANNER_PATH: String = "res://assets/cg/generated/ui_tutorial_hint_banner.png"

var shown_hints: Array = []  # 이미 표시된 힌트 ID 목록 (SaveManager 연동)

# 힌트 정의
const HINTS: Dictionary = {
	"first_battle": "Press Attack to strike, or Burn a memory for powerful skills.",
	"first_burn": "Burning memories is powerful but permanent. Choose wisely.",
	"first_shop": "Trade Grains for memories and items. Sell what you don't need.",
	"first_equipment": "Equip gear from the shop to boost your stats.",
	"first_status_effect": "Status effects last several turns. Use Antidote to cure poison.",
	"first_pulse": "Press Q to send out a Memory Pulse. Nearby echoes will briefly answer.",
	"first_break": "Exploit enemy weaknesses to fill BREAK. Broken enemies lose a turn and take heavier damage.",
	"first_resonance": "Strong tactical play builds Resonance. Higher Resonance boosts damage and post-battle rewards.",
}

const HINTS_KO: Dictionary = {
	"first_battle": "공격으로 피해를 주거나, 기억을 연소해 강력한 기술을 사용할 수 있습니다.",
	"first_burn": "연소한 기억은 되돌아오지 않습니다. 힘과 상실 사이에서 신중히 선택하세요.",
	"first_shop": "그레인으로 기억과 아이템을 거래할 수 있습니다. 필요 없는 물품은 판매하세요.",
	"first_equipment": "상점에서 장비를 착용하면 능력치가 상승합니다.",
	"first_status_effect": "상태 이상은 여러 턴 지속됩니다. 독은 해독제로 치료할 수 있습니다.",
	"first_pulse": "Q를 누르면 기억 파동을 방출합니다. 가까운 메아리가 잠시 응답합니다.",
	"first_break": "약점을 공략해 BREAK를 채우세요. 붕괴된 적은 한 턴 행동하지 못하고 더 큰 피해를 받습니다.",
	"first_resonance": "효율적인 전투는 공명을 높입니다. 공명이 높을수록 피해와 전투 보상이 증가합니다.",
}

# UI 노드
var _banner: TextureRect
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
	var hint_text: String = HINTS_KO.get(hint_id, HINTS[hint_id]) if GameManager.current_locale == "ko" else HINTS[hint_id]
	_show_panel(hint_text)
	print("[TutorialHints] Showing hint: %s" % hint_id)

func _show_panel(text: String) -> void:
	if _dismiss_tween and _dismiss_tween.is_valid():
		_dismiss_tween.kill()
	_label.text = text
	if _banner:
		_banner.visible = true
		_banner.modulate.a = 0.0
		_banner.position.y = -60
	_panel.visible = true
	_panel.modulate.a = 0.0
	_panel.position.y = -60
	var tw = create_tween().set_parallel(true)
	tw.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	if _banner:
		tw.tween_property(_banner, "modulate:a", 0.78, 0.3).set_ease(Tween.EASE_OUT)
		tw.tween_property(_banner, "position:y", 10.0, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
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
	if _banner:
		_dismiss_tween.tween_property(_banner, "modulate:a", 0.0, 0.25)
		_dismiss_tween.tween_property(_banner, "position:y", -60.0, 0.25).set_ease(Tween.EASE_IN)
	_dismiss_tween.tween_property(_panel, "modulate:a", 0.0, 0.25)
	_dismiss_tween.tween_property(_panel, "position:y", -60.0, 0.25).set_ease(Tween.EASE_IN)
	_dismiss_tween.chain().tween_callback(func():
		if _banner:
			_banner.visible = false
		_panel.visible = false
	)

func _hide_ui() -> void:
	if _banner:
		_banner.visible = false
	if _panel:
		_panel.visible = false

func _build_ui() -> void:
	_root = Control.new()
	_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_root)

	if ResourceLoader.exists(HINT_BANNER_PATH):
		_banner = TextureRect.new()
		_banner.texture = load(HINT_BANNER_PATH)
		_banner.anchor_left = 0.16
		_banner.anchor_right = 0.84
		_banner.anchor_top = 0.0
		_banner.anchor_bottom = 0.0
		_banner.offset_top = -2
		_banner.offset_bottom = 78
		_banner.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		_banner.stretch_mode = TextureRect.STRETCH_SCALE
		_banner.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_banner.modulate = Color(1.0, 0.92, 0.78, 0.0)
		_root.add_child(_banner)

	_panel = PanelContainer.new()
	_panel.anchor_left = 0.20
	_panel.anchor_right = 0.80
	_panel.anchor_top = 0.0
	_panel.anchor_bottom = 0.0
	_panel.offset_top = 10
	_panel.offset_bottom = 64
	_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.030, 0.026, 0.038, 0.70)
	style.border_color = Color(0.70, 0.56, 0.34, 0.36)
	style.set_border_width(SIDE_LEFT, 1)
	style.set_border_width(SIDE_TOP, 2)
	style.set_border_width(SIDE_RIGHT, 1)
	style.set_border_width(SIDE_BOTTOM, 1)
	style.set_corner_radius_all(5)
	style.set_content_margin_all(16)
	_panel.add_theme_stylebox_override("panel", style)
	_root.add_child(_panel)

	_label = Label.new()
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_label.add_theme_font_size_override("font_size", 15)
	_label.add_theme_color_override("font_color", Color(0.92, 0.86, 0.72))
	_label.add_theme_color_override("font_shadow_color", Color(0.0, 0.0, 0.0, 0.58))
	_label.add_theme_constant_override("shadow_offset_x", 1)
	_label.add_theme_constant_override("shadow_offset_y", 1)
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
