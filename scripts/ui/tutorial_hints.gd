## TutorialHints (Autoload), 첫 경험 시 컨텍스트 힌트 표시
## 주요 첫 순간에 팝업 힌트를 표시하고, 이미 본 힌트는 다시 표시하지 않음.
extends CanvasLayer

const HINT_BANNER_PATH: String = "res://assets/cg/generated/ui_tutorial_hint_banner.png"

## 패널 폭을 정하는 값. _show_panel이 라벨에 폭을 알려 줄 때 같은 값을 써야 한다.
const PANEL_ANCHOR_LEFT: float = 0.20
const PANEL_ANCHOR_RIGHT: float = 0.80
const PANEL_MARGIN: float = 16.0
const PANEL_TOP: float = 10.0
const PANEL_SLIDE: float = 70.0
const PANEL_MIN_HEIGHT: float = 54.0

## S245: 전투 화면은 위쪽 띠가 목표 카드·전장 관측·적 정보로 꽉 차 있다. 예전
## 위치(y=10)에서 힌트는 EnemyReadout 9,677px와 CombatCue 9,651px를 덮었다.
##
## 어디로 옮길지는 화면을 훑어서 정했다. 같은 폭의 띠를 10px 간격으로 내리며
## 겹침을 쟀는데, 그림까지 세면 최소가 y=0이었다. 4초 동안 무대 그림을 가리는
## 것과 적의 HP를 가리는 것은 값이 다르므로 글자와 막대만 다시 세었더니
## y 180~250 구간이 유일하게 **겹침 0**이었다. 그 한가운데를 쓴다.
##
## 탐색 화면은 위쪽이 비어 있으므로 예전 자리를 그대로 둔다.
const PANEL_TOP_BATTLE: float = 200.0

var shown_hints: Array = []  # 이미 표시된 힌트 ID 목록 (SaveManager 연동)

# 힌트 정의
## S226: The opening hours teach three things and nothing else: what a burn
## costs, what the directive asks, and what the approach bought.
const HINTS: Dictionary = {
	"first_battle": "Read the objective card first. Attack builds BREAK, Burn buys damage with a memory you will not get back.",
	"first_burn": "That memory is gone for good. The world and the people in it will adjust around the hole it left.",
	"first_approach": "How you reached the threat carried into the fight. Phase Step for an ambush, hold your nerve for a guarded entry, pulse for a witness entry.",
	"first_shop": "Trade Grains for memories and items. Sell what you don't need.",
	"first_equipment": "Equip gear from the shop to boost your stats.",
	"first_status_effect": "Status effects last several turns. Use Antidote to cure poison.",
	"first_pulse": "Press Q to send out a Memory Pulse. Nearby echoes will briefly answer.",
	"first_break": "Exploit enemy weaknesses to fill BREAK. Broken enemies lose a turn and take heavier damage.",
	"first_resonance": "Strong tactical play builds Resonance. Higher Resonance boosts damage and post-battle rewards.",
	"first_directive": "Every encounter carries one directive. Completing it raises your battle grade and extends the reward chain; some directives forbid burning.",
}

const HINTS_KO: Dictionary = {
	"first_battle": "먼저 목표 카드를 읽으세요. 공격은 BREAK를 쌓고, 연소는 돌아오지 않는 기억으로 화력을 삽니다.",
	"first_burn": "그 기억은 영영 사라집니다. 세계와 사람들은 그 빈자리에 맞춰 다시 정렬됩니다.",
	"first_approach": "위협에 다가간 방식이 전투로 이어집니다. 위상 이동은 기습, 버티며 접촉하면 대비 진입, 기억 파동은 증언 진입입니다.",
	"first_shop": "그레인으로 기억과 아이템을 거래할 수 있습니다. 필요 없는 물품은 판매하세요.",
	"first_equipment": "상점에서 장비를 착용하면 능력치가 상승합니다.",
	"first_status_effect": "상태 이상은 여러 턴 지속됩니다. 독은 해독제로 치료할 수 있습니다.",
	"first_pulse": "Q를 누르면 기억 파동을 방출합니다. 가까운 메아리가 잠시 응답합니다.",
	"first_break": "약점을 공략해 BREAK를 채우세요. 붕괴된 적은 한 턴 행동하지 못하고 더 큰 피해를 받습니다.",
	"first_resonance": "효율적인 전투는 공명을 높입니다. 공명이 높을수록 피해와 전투 보상이 증가합니다.",
	"first_directive": "모든 교전에는 지침이 하나 붙습니다. 달성하면 전술 등급과 연속 보상이 오르고, 일부 지침은 연소를 금지합니다.",
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

## 전투면 내려간 자리, 그 밖에는 위쪽 띠.
func _panel_top() -> float:
	if GameManager.current_state == GameManager.GameState.BATTLE:
		return PANEL_TOP_BATTLE
	return PANEL_TOP

func _show_panel(text: String) -> void:
	if _dismiss_tween and _dismiss_tween.is_valid():
		_dismiss_tween.kill()
	# S241: autowrap 라벨은 폭을 모르는 상태로 최소 크기를 물으면 "한 글자 폭으로
	# 접었을 때의 높이"를 돌려준다. 한국어 힌트 43자가 774px이 되고 여백 32를 더해
	# PanelContainer가 768x806으로 부풀었다. 720px 화면보다 큰 어두운 띠가 세로로
	# 내려와 덱의 1·3번 버튼까지 덮은 원인이다. 앵커가 정한 폭을 미리 알려 준다.
	var usable := get_viewport().get_visible_rect().size.x * (PANEL_ANCHOR_RIGHT - PANEL_ANCHOR_LEFT) - PANEL_MARGIN * 2.0
	_label.custom_minimum_size.x = maxf(usable, 120.0)
	_label.text = text
	if _banner:
		_banner.visible = true
		_banner.modulate.a = 0.0
		_banner.offset_top = _panel_top() - 12.0 - PANEL_SLIDE
		_banner.offset_bottom = _panel_top() + 68.0 - PANEL_SLIDE
	_panel.visible = true
	_panel.modulate.a = 0.0
	# Control.position 세터는 size_cache를 보존하며 offset을 다시 쓴다. 레이아웃이
	# 확정되기 전의 높이가 그대로 offset_bottom에 굳어, 라벨 최소 크기를 고쳐도
	# 패널은 계속 806px이었다. 높이를 직접 계산해 offset 두 개를 짝으로 움직인다.
	var height := maxf(_panel.get_combined_minimum_size().y, PANEL_MIN_HEIGHT)
	var top := _panel_top()
	_panel.offset_top = top - PANEL_SLIDE
	_panel.offset_bottom = _panel.offset_top + height
	var tw = create_tween().set_parallel(true)
	tw.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	if _banner:
		tw.tween_property(_banner, "modulate:a", 0.78, 0.3).set_ease(Tween.EASE_OUT)
		tw.tween_property(_banner, "offset_top", top - 12.0, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
		tw.tween_property(_banner, "offset_bottom", top + 68.0, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tw.tween_property(_panel, "modulate:a", 1.0, 0.3).set_ease(Tween.EASE_OUT)
	tw.tween_property(_panel, "offset_top", top, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	tw.tween_property(_panel, "offset_bottom", top + height, 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
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
		_dismiss_tween.tween_property(_banner, "offset_top", _panel_top() - 12.0 - PANEL_SLIDE, 0.25).set_ease(Tween.EASE_IN)
	_dismiss_tween.tween_property(_panel, "modulate:a", 0.0, 0.25)
	var height := _panel.offset_bottom - _panel.offset_top
	_dismiss_tween.tween_property(_panel, "offset_top", _panel_top() - PANEL_SLIDE, 0.25).set_ease(Tween.EASE_IN)
	_dismiss_tween.tween_property(_panel, "offset_bottom", _panel_top() - PANEL_SLIDE + height, 0.25).set_ease(Tween.EASE_IN)
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
	_panel.anchor_left = PANEL_ANCHOR_LEFT
	_panel.anchor_right = PANEL_ANCHOR_RIGHT
	_panel.anchor_top = 0.0
	_panel.anchor_bottom = 0.0
	_panel.offset_top = PANEL_TOP
	_panel.offset_bottom = PANEL_TOP + PANEL_MIN_HEIGHT
	_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.030, 0.026, 0.038, 0.70)
	style.border_color = Color(0.70, 0.56, 0.34, 0.36)
	style.set_border_width(SIDE_LEFT, 1)
	style.set_border_width(SIDE_TOP, 2)
	style.set_border_width(SIDE_RIGHT, 1)
	style.set_border_width(SIDE_BOTTOM, 1)
	style.set_corner_radius_all(5)
	style.set_content_margin_all(PANEL_MARGIN)
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
