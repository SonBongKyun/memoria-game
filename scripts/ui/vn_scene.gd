## VNScene — 비주얼 노벨 씬 UI
## S60: SceneFlow 오토로드가 구동하는 CG + 포트레이트 + 텍스트 렌더러.
## 클릭/Enter/Space/E로 진행. 선택지는 마우스 클릭.
extends CanvasLayer

const PORTRAIT_SIZE: int = 300
const TYPEWRITER_SPEED: float = 0.025
const CG_FADE_DURATION: float = 0.8
const PORTRAIT_DIM: Color = Color(0.45, 0.45, 0.5, 1.0)
const PORTRAIT_BRIGHT: Color = Color(1, 1, 1, 1)
const CG_LOWER_WASH_ALPHA: float = 0.18
const CG_FOCUS_GLOW_ALPHA: float = 0.18
const CG_VIGNETTE_ALPHA: float = 0.34
const CG_TEXT_PLATE_VIGNETTE_ALPHA: float = 0.14
const CG_REFERENCE_CARD_VIGNETTE_ALPHA: float = 0.04
const CG_ALIAS_FALLBACKS: Dictionary = {
	"ch1_twisted_forest": "res://assets/cg/generated/story_ch1_twisted_forest_path.png",
	"ch1_stump2": "res://assets/cg/generated/story_ch1_memory_shrine.png",
	"ch1_ash_forest": "res://assets/cg/generated/story_ch1_memory_shrine.png",
	"ch1_green_tree": "res://assets/cg/generated/story_ch1_green_tree_dawn.png",
}
const DEFAULT_CG_FALLBACK: String = "res://assets/cg/generated/chapter_splash_rim_forest.png"
const DIALOGUE_OVERLAY_PATH: String = "res://assets/cg/generated/ui_vn_memory_frame_overlay.png"
const CHOICE_OVERLAY_PATH: String = "res://assets/cg/generated/ui_vn_choice_archive_overlay.png"

# 노드
var _bg: ColorRect
var _cg_current: TextureRect
var _cg_next: TextureRect
var _cg_detail_top: TextureRect
var _cg_vignette: TextureRect
var _cg_focus_glow: TextureRect
var _cg_lower_wash: ColorRect
var _portrait_left_frame: PanelContainer
var _portrait_right_frame: PanelContainer
var _portrait_left: TextureRect
var _portrait_right: TextureRect
var _portrait_left_shadow: TextureRect
var _portrait_right_shadow: TextureRect
var _name_label: Label
var _name_panel: PanelContainer
var _text_label: RichTextLabel
var _text_panel: PanelContainer
var _continue_indicator: Label
var _continue_tween: Tween
var _choice_header: Label
var _choice_hint: Label
var _choice_container: VBoxContainer
var _dialogue_frame_art: TextureRect
var _choice_frame_art: TextureRect
var _letterbox_top: ColorRect
var _letterbox_bottom: ColorRect

# S61: 기억 왜곡 VFX
var _glitch_overlay: ColorRect          # 전체화면 플래시/색상 틴트
var _chroma_r: TextureRect              # 색수차 레이어 (붉은 채널)
var _chroma_b: TextureRect              # 색수차 레이어 (푸른 채널)
var _is_distorted_line: bool = false    # 현재 대사가 왜곡 상태인지
# S69: 연소 잔열 비네트 (가장자리만 따뜻하게 타고 난 흔적)
var _ember_vignette: TextureRect
var _film_grain: ColorRect
var _film_grain_time: float = 0.0
# S73: 책 페이지 넘기기 — 대사 advance 시 종이 휨 효과
var _page_turn_overlay: TextureRect
var _last_displayed_text: String = ""

# 상태
var _current_step: Dictionary = {}
var _typing_done: bool = true
var _typed_chars: int = 0
var _full_text: String = ""
var _type_timer: float = 0.0
var _waiting_for_input: bool = false
var _active_side: String = ""  # "left" or "right" (말하는 쪽)

# 포트레이트 슬롯 상태
var _left_portrait_id: String = ""
var _right_portrait_id: String = ""

# 포트레이트 맵 참조 (DialogueBox의 것과 동일)
var _portrait_map: Dictionary = {}

func _ready() -> void:
	layer = 50
	_load_portrait_map()
	_build_ui()
	_build_glitch_layer()
	SceneFlow.step_changed.connect(_on_step_changed)
	SceneFlow.scene_ended.connect(_on_scene_ended)
	if has_node("/root/MemoryManager"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	set_process_input(true)
	set_process(true)

func _exit_tree() -> void:
	if has_node("/root/SceneFlow"):
		if SceneFlow.step_changed.is_connected(_on_step_changed):
			SceneFlow.step_changed.disconnect(_on_step_changed)
		if SceneFlow.scene_ended.is_connected(_on_scene_ended):
			SceneFlow.scene_ended.disconnect(_on_scene_ended)
	if has_node("/root/MemoryManager") and MemoryManager.memory_burned.is_connected(_on_memory_burned):
		MemoryManager.memory_burned.disconnect(_on_memory_burned)

func _load_portrait_map() -> void:
	# DialogueBox 오토로드에서 포트레이트 매핑 공유
	if has_node("/root/DialogueBox") and "PORTRAIT_MAP" in DialogueBox:
		_portrait_map = DialogueBox.PORTRAIT_MAP.duplicate()

## ===================== UI BUILD =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_PASS
	add_child(root)

	# 배경 (검은색)
	_bg = ColorRect.new()
	_bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	_bg.color = Color(0.02, 0.02, 0.03, 1)
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_bg)

	# CG 레이어 — 현재 + 다음(크로스페이드)
	_cg_current = _make_cg_rect()
	_cg_current.modulate = Color(1, 1, 1, 0)
	root.add_child(_cg_current)

	_cg_next = _make_cg_rect()
	_cg_next.modulate = Color(1, 1, 1, 0)
	root.add_child(_cg_next)

	_cg_detail_top = _make_cg_detail_rect()
	root.add_child(_cg_detail_top)

	_cg_focus_glow = _make_focus_glow_rect()
	root.add_child(_cg_focus_glow)

	_cg_lower_wash = ColorRect.new()
	_cg_lower_wash.anchor_left = 0.0
	_cg_lower_wash.anchor_right = 1.0
	_cg_lower_wash.anchor_top = 0.54
	_cg_lower_wash.anchor_bottom = 1.0
	_cg_lower_wash.color = Color(0.018, 0.014, 0.022, CG_LOWER_WASH_ALPHA)
	_cg_lower_wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_cg_lower_wash)

	_cg_vignette = _make_cinematic_vignette_rect()
	root.add_child(_cg_vignette)

	# 레터박스 (연출용)
	_letterbox_top = ColorRect.new()
	_letterbox_top.anchor_left = 0.0
	_letterbox_top.anchor_right = 1.0
	_letterbox_top.anchor_top = 0.0
	_letterbox_top.anchor_bottom = 0.0
	_letterbox_top.offset_bottom = 60
	_letterbox_top.color = Color(0, 0, 0, 0.85)
	_letterbox_top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_letterbox_top)

	_letterbox_bottom = ColorRect.new()
	_letterbox_bottom.anchor_left = 0.0
	_letterbox_bottom.anchor_right = 1.0
	_letterbox_bottom.anchor_top = 1.0
	_letterbox_bottom.anchor_bottom = 1.0
	_letterbox_bottom.offset_top = -60
	_letterbox_bottom.color = Color(0, 0, 0, 0.85)
	_letterbox_bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_letterbox_bottom)

	# 포트레이트 (좌/우)
	_portrait_left_shadow = _make_portrait_shadow_rect(true)
	root.add_child(_portrait_left_shadow)
	_portrait_left_frame = _make_portrait_frame_rect(true)
	root.add_child(_portrait_left_frame)
	_portrait_left = _make_portrait_rect(true)
	root.add_child(_portrait_left)
	_portrait_right_shadow = _make_portrait_shadow_rect(false)
	root.add_child(_portrait_right_shadow)
	_portrait_right_frame = _make_portrait_frame_rect(false)
	root.add_child(_portrait_right_frame)
	_portrait_right = _make_portrait_rect(false)
	root.add_child(_portrait_right)

	_dialogue_frame_art = _make_interface_overlay(DIALOGUE_OVERLAY_PATH)
	root.add_child(_dialogue_frame_art)
	_choice_frame_art = _make_interface_overlay(CHOICE_OVERLAY_PATH)
	_choice_frame_art.visible = false
	root.add_child(_choice_frame_art)

	# 대화 박스 패널
	_text_panel = PanelContainer.new()
	_text_panel.anchor_left = 0.08
	_text_panel.anchor_right = 0.92
	_text_panel.anchor_top = 1.0
	_text_panel.anchor_bottom = 1.0
	_text_panel.offset_top = -200
	_text_panel.offset_bottom = -30
	_text_panel.mouse_filter = Control.MOUSE_FILTER_PASS
	var tstyle = StyleBoxFlat.new()
	tstyle.bg_color = Color(0.025, 0.023, 0.032, 0.90)
	tstyle.border_color = Color(0.82, 0.66, 0.40, 0.62)
	tstyle.set_border_width(SIDE_LEFT, 1)
	tstyle.set_border_width(SIDE_TOP, 2)
	tstyle.set_border_width(SIDE_RIGHT, 1)
	tstyle.set_border_width(SIDE_BOTTOM, 2)
	tstyle.set_content_margin_all(22)
	tstyle.set_corner_radius_all(5)
	_text_panel.add_theme_stylebox_override("panel", tstyle)
	root.add_child(_text_panel)

	_text_label = RichTextLabel.new()
	_text_label.bbcode_enabled = true
	_text_label.fit_content = false
	_text_label.scroll_active = false
	# S71: 책 같은 가독성 — 사이즈 키우고 행간 넓히기. theme.tres가 serif 폰트 자동 적용
	_text_label.add_theme_font_size_override("normal_font_size", 22)
	_text_label.add_theme_font_override("normal_font", UITheme.make_body_font())
	_text_label.add_theme_constant_override("line_separation", 9)
	_text_label.add_theme_color_override("default_color", Color(0.94, 0.91, 0.84))
	# 부드러운 검정 그림자로 어두운 CG 위에서도 가독성 확보
	_text_label.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.7))
	_text_label.add_theme_constant_override("shadow_offset_x", 1)
	_text_label.add_theme_constant_override("shadow_offset_y", 1)
	_text_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_text_panel.add_child(_text_label)

	# 이름 패널 (대화박스 위)
	_name_panel = PanelContainer.new()
	_name_panel.anchor_left = 0.08
	_name_panel.anchor_right = 0.08
	_name_panel.anchor_top = 1.0
	_name_panel.anchor_bottom = 1.0
	_name_panel.offset_left = -20
	_name_panel.offset_top = -290
	_name_panel.offset_bottom = -245
	_name_panel.offset_right = 208
	var nstyle = StyleBoxFlat.new()
	nstyle.bg_color = Color(0.12, 0.09, 0.05, 0.95)
	nstyle.border_color = Color(0.75, 0.6, 0.35, 0.9)
	nstyle.set_border_width_all(2)
	nstyle.set_content_margin_all(6)
	nstyle.set_corner_radius_all(4)
	_name_panel.add_theme_stylebox_override("panel", nstyle)
	_name_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_name_panel)

	_name_label = Label.new()
	_name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	UITheme.apply_title_font(_name_label)
	# S71: 화자 이름 — 살짝 더 큼 + letter_spacing 느낌의 voff
	_name_label.add_theme_font_size_override("font_size", 20)
	_name_label.add_theme_color_override("font_color", Color(0.97, 0.86, 0.55))
	_name_label.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.8))
	_name_label.add_theme_constant_override("shadow_outline_size", 2)
	_name_label.add_theme_constant_override("shadow_offset_x", 1)
	_name_label.add_theme_constant_override("shadow_offset_y", 1)
	_name_panel.add_child(_name_label)
	_name_panel.visible = false

	# 계속 표시 화살표
	_continue_indicator = Label.new()
	_continue_indicator.text = "NEXT"
	UITheme.apply_ui_font(_continue_indicator)
	_continue_indicator.anchor_left = 1.0
	_continue_indicator.anchor_right = 1.0
	_continue_indicator.anchor_top = 1.0
	_continue_indicator.anchor_bottom = 1.0
	_continue_indicator.offset_left = -130
	_continue_indicator.offset_right = -90
	_continue_indicator.offset_top = -55
	_continue_indicator.offset_bottom = -25
	_continue_indicator.add_theme_font_size_override("font_size", 10)
	_continue_indicator.add_theme_color_override("font_color", Color(0.82, 0.68, 0.42, 0.78))
	_continue_indicator.add_theme_constant_override("outline_size", 1)
	_continue_indicator.add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 0.55))
	_continue_indicator.visible = false
	root.add_child(_continue_indicator)

	_choice_header = Label.new()
	_choice_header.anchor_left = 0.18
	_choice_header.anchor_right = 0.82
	_choice_header.anchor_top = 0.18
	_choice_header.anchor_bottom = 0.18
	_choice_header.offset_top = -4
	_choice_header.offset_bottom = 32
	_choice_header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	UITheme.apply_title_font(_choice_header)
	_choice_header.add_theme_font_size_override("font_size", 20)
	_choice_header.add_theme_color_override("font_color", Color(0.96, 0.78, 0.45, 0.92))
	_choice_header.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.85))
	_choice_header.add_theme_constant_override("shadow_outline_size", 2)
	_choice_header.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_choice_header.visible = false
	root.add_child(_choice_header)

	_choice_hint = Label.new()
	_choice_hint.anchor_left = 0.18
	_choice_hint.anchor_right = 0.82
	_choice_hint.anchor_top = 0.22
	_choice_hint.anchor_bottom = 0.22
	_choice_hint.offset_top = -2
	_choice_hint.offset_bottom = 28
	_choice_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_choice_hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	UITheme.apply_ui_font(_choice_hint)
	_choice_hint.add_theme_font_size_override("font_size", 13)
	_choice_hint.add_theme_color_override("font_color", Color(0.86, 0.82, 0.72, 0.76))
	_choice_hint.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75))
	_choice_hint.add_theme_constant_override("shadow_outline_size", 1)
	_choice_hint.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_choice_hint.visible = false
	root.add_child(_choice_hint)

	# 선택지 컨테이너
	_choice_container = VBoxContainer.new()
	_choice_container.anchor_left = 0.20
	_choice_container.anchor_right = 0.80
	_choice_container.anchor_top = 0.29
	_choice_container.anchor_bottom = 0.77
	_choice_container.add_theme_constant_override("separation", 12)
	_choice_container.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(_choice_container)
	_choice_container.visible = false

func _make_interface_overlay(path: String) -> TextureRect:
	var overlay := TextureRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	overlay.stretch_mode = TextureRect.STRETCH_SCALE
	overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if ResourceLoader.exists(path):
		overlay.texture = load(path)
	else:
		overlay.visible = false
	return overlay

func _build_glitch_layer() -> void:
	# 색수차 복사본 (CG 위에 R/B 채널 오프셋)
	_chroma_r = _make_cg_rect()
	_chroma_r.modulate = Color(1, 0, 0, 0)
	_chroma_r.z_index = 1
	add_child(_chroma_r)

	_chroma_b = _make_cg_rect()
	_chroma_b.modulate = Color(0, 0.4, 1, 0)
	_chroma_b.z_index = 1
	add_child(_chroma_b)

	# 전체화면 글리치 오버레이
	_glitch_overlay = ColorRect.new()
	_glitch_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_glitch_overlay.color = Color(0.85, 0.2, 0.15, 0)
	_glitch_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_glitch_overlay.z_index = 2
	add_child(_glitch_overlay)

	# S69: 연소 잔열 비네트 — 가장자리에 타고 난 듯한 따뜻한 그라디언트
	_ember_vignette = TextureRect.new()
	_ember_vignette.set_anchors_preset(Control.PRESET_FULL_RECT)
	_ember_vignette.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_ember_vignette.stretch_mode = TextureRect.STRETCH_SCALE
	_ember_vignette.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_ember_vignette.z_index = 1
	_ember_vignette.modulate.a = 0.0
	var grad = Gradient.new()
	grad.add_point(0.0, Color(0.85, 0.35, 0.2, 0.0))
	grad.add_point(0.55, Color(0.85, 0.3, 0.15, 0.0))
	grad.add_point(0.85, Color(0.7, 0.2, 0.1, 0.55))
	grad.add_point(1.0, Color(0.4, 0.1, 0.05, 0.85))
	var gtex = GradientTexture2D.new()
	gtex.gradient = grad
	gtex.width = 256
	gtex.height = 256
	gtex.fill = GradientTexture2D.FILL_RADIAL
	gtex.fill_from = Vector2(0.5, 0.5)
	gtex.fill_to = Vector2(1.0, 0.5)
	_ember_vignette.texture = gtex
	add_child(_ember_vignette)

	# S69: 필름 그레인 — 셰이더 노이즈 오버레이 (전체 화면 위에 살짝)
	_film_grain = ColorRect.new()
	_film_grain.set_anchors_preset(Control.PRESET_FULL_RECT)
	_film_grain.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_film_grain.z_index = 3
	var grain_shader = Shader.new()
	grain_shader.code = """
shader_type canvas_item;

uniform float grain_strength : hint_range(0.0, 0.2) = 0.045;
uniform float time_scale : hint_range(0.0, 60.0) = 16.0;
uniform float u_time = 0.0;

float hash21(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

void fragment() {
	vec2 uv = UV * vec2(640.0, 360.0);
	float t = floor(u_time * time_scale);
	float n = hash21(uv + vec2(t * 0.137, t * 0.731));
	// 회색 노이즈, 알파만 살짝 깜빡이게 — 색조엔 영향 X
	COLOR = vec4(n, n, n, grain_strength);
}
"""
	var sm = ShaderMaterial.new()
	sm.shader = grain_shader
	_film_grain.material = sm
	add_child(_film_grain)

	# S73: 페이지 넘김 오버레이 — 종이 그림자 + 살짝 밝은 페이지 엣지가 좌→우로 스윕
	_page_turn_overlay = TextureRect.new()
	_page_turn_overlay.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	_page_turn_overlay.offset_top = -260
	_page_turn_overlay.offset_bottom = -10
	_page_turn_overlay.offset_left = 80
	_page_turn_overlay.offset_right = -80
	_page_turn_overlay.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_page_turn_overlay.stretch_mode = TextureRect.STRETCH_SCALE
	_page_turn_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_page_turn_overlay.z_index = 4
	_page_turn_overlay.modulate.a = 0.0
	_page_turn_overlay.pivot_offset = Vector2.ZERO
	# 가로 그라디언트: 어두운 그림자 → 밝은 페이지 엣지 → 투명
	var pg_grad = Gradient.new()
	pg_grad.add_point(0.00, Color(0.05, 0.04, 0.03, 0.0))
	pg_grad.add_point(0.40, Color(0.10, 0.08, 0.05, 0.65))
	pg_grad.add_point(0.50, Color(0.95, 0.90, 0.78, 0.85))
	pg_grad.add_point(0.60, Color(0.80, 0.75, 0.62, 0.55))
	pg_grad.add_point(1.00, Color(0.50, 0.45, 0.38, 0.0))
	var pg_tex = GradientTexture2D.new()
	pg_tex.gradient = pg_grad
	pg_tex.width = 1024
	pg_tex.height = 32
	pg_tex.fill = GradientTexture2D.FILL_LINEAR
	pg_tex.fill_from = Vector2(0.0, 0.5)
	pg_tex.fill_to = Vector2(1.0, 0.5)
	_page_turn_overlay.texture = pg_tex
	add_child(_page_turn_overlay)

func _make_cg_rect() -> TextureRect:
	var tr = TextureRect.new()
	tr.set_anchors_preset(Control.PRESET_FULL_RECT)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return tr

func _make_cg_detail_rect() -> TextureRect:
	var tr = TextureRect.new()
	tr.anchor_left = 0.0
	tr.anchor_right = 1.0
	tr.anchor_top = 0.0
	tr.anchor_bottom = 0.34
	tr.offset_top = -18
	tr.offset_bottom = 24
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.modulate = Color(0.88, 0.80, 0.68, 0.0)
	return tr

func _make_focus_glow_rect() -> TextureRect:
	var tr = TextureRect.new()
	tr.anchor_left = 0.12
	tr.anchor_right = 0.88
	tr.anchor_top = 0.42
	tr.anchor_bottom = 1.03
	tr.offset_top = -20
	tr.offset_bottom = 28
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_SCALE
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.modulate = Color(1.0, 0.78, 0.46, CG_FOCUS_GLOW_ALPHA)
	var grad := Gradient.new()
	grad.add_point(0.0, Color(0.72, 0.42, 0.16, 0.24))
	grad.add_point(0.45, Color(0.30, 0.18, 0.10, 0.12))
	grad.add_point(1.0, Color(0.0, 0.0, 0.0, 0.0))
	var tex := GradientTexture2D.new()
	tex.gradient = grad
	tex.width = 512
	tex.height = 256
	tex.fill = GradientTexture2D.FILL_RADIAL
	tex.fill_from = Vector2(0.5, 0.76)
	tex.fill_to = Vector2(0.96, 0.28)
	tr.texture = tex
	return tr

func _make_cinematic_vignette_rect() -> TextureRect:
	var tr = TextureRect.new()
	tr.set_anchors_preset(Control.PRESET_FULL_RECT)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_SCALE
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.modulate = Color(0.72, 0.64, 0.58, CG_VIGNETTE_ALPHA)
	var grad := Gradient.new()
	grad.add_point(0.0, Color(0.0, 0.0, 0.0, 0.0))
	grad.add_point(0.58, Color(0.0, 0.0, 0.0, 0.03))
	grad.add_point(0.84, Color(0.0, 0.0, 0.0, 0.22))
	grad.add_point(1.0, Color(0.0, 0.0, 0.0, 0.42))
	var tex := GradientTexture2D.new()
	tex.gradient = grad
	tex.width = 512
	tex.height = 288
	tex.fill = GradientTexture2D.FILL_RADIAL
	tex.fill_from = Vector2(0.5, 0.48)
	tex.fill_to = Vector2(1.04, 0.52)
	tr.texture = tex
	return tr

func _make_portrait_shadow_rect(is_left: bool) -> TextureRect:
	var tr = TextureRect.new()
	tr.custom_minimum_size = Vector2(PORTRAIT_SIZE + 70, 128)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_SCALE
	tr.texture = _make_portrait_shadow_texture()
	tr.anchor_top = 1.0
	tr.anchor_bottom = 1.0
	tr.offset_top = -284
	tr.offset_bottom = -142
	if is_left:
		tr.anchor_left = 0.0
		tr.anchor_right = 0.0
		tr.offset_left = -6
		tr.offset_right = PORTRAIT_SIZE + 64
	else:
		tr.anchor_left = 1.0
		tr.anchor_right = 1.0
		tr.offset_left = -PORTRAIT_SIZE - 64
		tr.offset_right = 6
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.modulate = Color(0.0, 0.0, 0.0, 0.0)
	tr.visible = false
	return tr

func _make_portrait_shadow_texture() -> Texture2D:
	var grad = Gradient.new()
	grad.add_point(0.0, Color(0, 0, 0, 0.52))
	grad.add_point(0.55, Color(0, 0, 0, 0.30))
	grad.add_point(1.0, Color(0, 0, 0, 0.0))
	var tex = GradientTexture2D.new()
	tex.gradient = grad
	tex.width = 384
	tex.height = 128
	tex.fill = GradientTexture2D.FILL_RADIAL
	tex.fill_from = Vector2(0.5, 0.5)
	tex.fill_to = Vector2(0.98, 0.5)
	return tex

func _make_portrait_frame_rect(is_left: bool) -> PanelContainer:
	var panel := PanelContainer.new()
	panel.anchor_top = 1.0
	panel.anchor_bottom = 1.0
	panel.offset_top = -PORTRAIT_SIZE - 188
	panel.offset_bottom = -172
	if is_left:
		panel.anchor_left = 0.0
		panel.anchor_right = 0.0
		panel.offset_left = 12
		panel.offset_right = PORTRAIT_SIZE + 28
	else:
		panel.anchor_left = 1.0
		panel.anchor_right = 1.0
		panel.offset_left = -PORTRAIT_SIZE - 28
		panel.offset_right = -12
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.visible = false
	panel.modulate.a = 0.0
	panel.add_theme_stylebox_override("panel", _make_portrait_frame_style(Color(0.72, 0.64, 0.46)))
	return panel

func _make_portrait_frame_style(accent: Color) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.018, 0.014, 0.022, 0.30)
	style.border_color = Color(accent.r, accent.g, accent.b, 0.72)
	style.set_border_width(SIDE_LEFT, 2)
	style.set_border_width(SIDE_TOP, 1)
	style.set_border_width(SIDE_RIGHT, 2)
	style.set_border_width(SIDE_BOTTOM, 2)
	style.set_corner_radius_all(6)
	style.shadow_color = Color(0.0, 0.0, 0.0, 0.50)
	style.shadow_size = 14
	style.shadow_offset = Vector2(0, 8)
	return style

func _make_portrait_rect(is_left: bool) -> TextureRect:
	var tr = TextureRect.new()
	tr.custom_minimum_size = Vector2(PORTRAIT_SIZE, PORTRAIT_SIZE)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT
	tr.anchor_top = 1.0
	tr.anchor_bottom = 1.0
	tr.offset_top = -PORTRAIT_SIZE - 180
	tr.offset_bottom = -180
	if is_left:
		tr.anchor_left = 0.0
		tr.anchor_right = 0.0
		tr.offset_left = 20
		tr.offset_right = PORTRAIT_SIZE + 20
	else:
		tr.anchor_left = 1.0
		tr.anchor_right = 1.0
		tr.offset_left = -PORTRAIT_SIZE - 20
		tr.offset_right = -20
	tr.modulate = PORTRAIT_DIM
	tr.pivot_offset = Vector2(PORTRAIT_SIZE * 0.5, PORTRAIT_SIZE * 0.88)
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.visible = false
	return tr

## ===================== STEP HANDLER =====================

func _on_step_changed(step: Dictionary) -> void:
	_current_step = step
	_is_distorted_line = step.get("_distorted", false)

	# CG 변경
	if step.has("cg"):
		_change_cg(step.cg, float(step.get("fade", CG_FADE_DURATION)))

	# S61: 왜곡 라인은 약한 색수차 + 대사 사전 글리치
	if _is_distorted_line:
		_play_subtle_distortion()

	# 포트레이트 변경/제거
	if step.has("hide_left"):
		_set_portrait_side("left", "")
	if step.has("hide_right"):
		_set_portrait_side("right", "")

	# System log (시스템 메시지)
	if step.has("system_log"):
		_show_system_log(GameManager.localized_value(step, "system_log", String(step.system_log)))
		_waiting_for_input = true
		_show_continue(true)
		return

	# 선택지
	if step.has("choice"):
		_show_choices(step.choice)
		return

	# 일반 대사 / 나레이션
	var speaker: String = step.get("speaker", "")
	var text: String = ""
	if step.has("text"):
		text = GameManager.localized_value(step, "text", "")
	else:
		text = GameManager.localized_value(step, "narrate", "")
	var portrait: String = step.get("portrait", "")
	var side: String = step.get("side", "")

	if _should_hide_portraits_for_cg_line(step, speaker):
		_clear_portraits()
		_active_side = ""
		_highlight_speaking_side("")
	elif portrait != "" and side != "":
		_set_portrait_side(side, portrait)
		if _uses_single_portrait_composition(speaker):
			_set_portrait_side(_opposite_side(side), "")
		_active_side = side
		_highlight_speaking_side(side)
	elif speaker == "":
		# 나레이션 — 양쪽 포트레이트 어둡게
		_active_side = ""
		_highlight_speaking_side("")

	_display_line(speaker, text)

func _on_scene_ended(_id: String) -> void:
	# UI는 SceneFlow가 _close_vn_ui에서 제거
	prepare_for_close()

func prepare_for_close() -> void:
	_show_continue(false)
	_waiting_for_input = false
	_typing_done = true
	_choice_container.visible = false
	_choice_header.visible = false
	_choice_hint.visible = false
	_text_panel.visible = false
	_name_panel.visible = false
	_dialogue_frame_art.visible = false
	_choice_frame_art.visible = false
	_clear_portraits()
	for node in [_cg_current, _cg_next, _cg_detail_top, _cg_vignette, _cg_focus_glow, _cg_lower_wash, _glitch_overlay, _chroma_r, _chroma_b, _ember_vignette, _film_grain, _page_turn_overlay]:
		if node != null and is_instance_valid(node):
			node.visible = false
			if node is CanvasItem:
				(node as CanvasItem).modulate.a = 0.0
	visible = false
	set_process(false)
	set_process_input(false)

## ===================== CG =====================

func _change_cg(cg_ref: String, fade: float) -> void:
	var path = _resolve_cg_path(cg_ref)
	if path == "":
		return
	if not ResourceLoader.exists(path):
		push_warning("[VNScene] CG not found: %s" % path)
		return
	var tex = load(path)
	if tex == null:
		return

	# 크로스페이드: next에 새 이미지 올리고 페이드인, 끝나면 current와 교체
	_apply_cg_presentation_profile(path)
	_cg_next.texture = tex
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(_cg_next, "modulate:a", 1.0, fade)
	tw.tween_property(_cg_current, "modulate:a", 0.0, fade)
	if _cg_detail_top != null:
		tw.tween_property(_cg_detail_top, "modulate:a", 0.0, minf(0.25, fade))
	tw.chain().tween_callback(Callable(self, "_swap_cg"))

func _swap_cg() -> void:
	_cg_current.texture = _cg_next.texture
	_cg_current.modulate.a = 1.0
	_cg_next.texture = null
	_cg_next.modulate.a = 0.0
	# S69: Ken Burns — 정적 일러스트에 미세한 줌+팬으로 생명감
	_start_ken_burns(_cg_current)
	_sync_cg_presentation_layers()

func _apply_cg_presentation_profile(path: String) -> void:
	var key := path.to_lower()
	var is_reference_card := key.contains("/sheet_") or key.contains("\\sheet_")
	var is_text_plate := key.contains("chapter_sealed_zone") or key.contains("memory_loss_warning")
	var is_story_cg := key.contains("/generated/story_") or key.contains("/generated/dialogue_") or key.contains("/generated/chapter_splash_") or key.contains("/generated/cinematic_") or key.contains("/generated/memory_compass_")

	var wash_alpha := CG_LOWER_WASH_ALPHA
	var glow_alpha := CG_FOCUS_GLOW_ALPHA
	var vignette_alpha := CG_VIGNETTE_ALPHA
	if is_reference_card:
		wash_alpha = 0.06
		glow_alpha = 0.0
		vignette_alpha = CG_REFERENCE_CARD_VIGNETTE_ALPHA
	elif is_text_plate:
		wash_alpha = 0.08
		glow_alpha = 0.0
		vignette_alpha = CG_TEXT_PLATE_VIGNETTE_ALPHA
	elif is_story_cg:
		wash_alpha = 0.12
		glow_alpha = 0.08
		vignette_alpha = 0.24

	if _cg_lower_wash:
		_cg_lower_wash.color.a = wash_alpha
	if _cg_focus_glow:
		_cg_focus_glow.modulate.a = glow_alpha
	if _cg_vignette:
		_cg_vignette.modulate.a = vignette_alpha

func _sync_cg_presentation_layers() -> void:
	if _cg_detail_top == null:
		return
	_cg_detail_top.texture = null
	_cg_detail_top.modulate.a = 0.0
	_cg_detail_top.texture = _cg_current.texture
	_cg_detail_top.position = Vector2.ZERO
	_cg_detail_top.scale = Vector2(1.0, 1.0)
	_cg_detail_top.modulate = Color(0.88, 0.80, 0.68, 0.0)
	if _cg_detail_top.has_meta("detail_tween"):
		var prev = _cg_detail_top.get_meta("detail_tween")
		if prev is Tween and is_instance_valid(prev):
			prev.kill()
	var fade_tw = create_tween()
	fade_tw.tween_property(_cg_detail_top, "modulate:a", 0.13, 0.55).set_trans(Tween.TRANS_SINE)
	var detail_tw = create_tween().set_loops()
	detail_tw.tween_interval(0.55)
	detail_tw.tween_property(_cg_detail_top, "modulate:a", 0.18, 5.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	detail_tw.tween_property(_cg_detail_top, "modulate:a", 0.10, 5.0).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	_cg_detail_top.set_meta("detail_tween", detail_tw)

## S69: Ken Burns — 8~12초에 걸쳐 1.0 → 1.05 줌 + ±20px 팬
func _start_ken_burns(rect: TextureRect) -> void:
	if rect == null or rect.texture == null:
		return
	# 이전 트윈 정리
	if rect.has_meta("kb_tween"):
		var prev = rect.get_meta("kb_tween")
		if prev is Tween and is_instance_valid(prev):
			prev.kill()
	# 시작 상태
	rect.pivot_offset = rect.size / 2.0
	rect.scale = Vector2(1.0, 1.0)
	var pan_x = randf_range(-18.0, 18.0)
	var pan_y = randf_range(-10.0, 10.0)
	var orig_pos = rect.position
	rect.position = orig_pos
	# 트윈
	var dur = randf_range(9.0, 13.0)
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(rect, "scale", Vector2(1.05, 1.05), dur).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tw.tween_property(rect, "position", orig_pos + Vector2(pan_x, pan_y), dur).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	rect.set_meta("kb_tween", tw)

func _resolve_cg_path(ref: String) -> String:
	if ref == "":
		return ""
	if ref.begins_with("res://"):
		return ref
	if CG_ALIAS_FALLBACKS.has(ref):
		return CG_ALIAS_FALLBACKS[ref]
	for ext in [".png", ".jpg"]:
		var game_image_path = "res://assets/cg/game_image/" + ref + ext
		if ResourceLoader.exists(game_image_path):
			return game_image_path
	# 짧은 이름 → cg 폴더에서 jpg/png 자동 탐색
	for ext in [".jpg", ".png"]:
		var p = "res://assets/cg/" + ref + ext
		if ResourceLoader.exists(p):
			return p
	return DEFAULT_CG_FALLBACK

## ===================== PORTRAIT =====================

func _set_portrait_side(side: String, portrait_id: String) -> void:
	var target: TextureRect = _portrait_left if side == "left" else _portrait_right
	var frame: PanelContainer = _portrait_left_frame if side == "left" else _portrait_right_frame

	if portrait_id == "":
		target.visible = false
		if frame != null:
			frame.visible = false
			frame.modulate.a = 0.0
		_set_portrait_shadow_alpha(side, 0.0)
		if side == "left":
			_left_portrait_id = ""
		else:
			_right_portrait_id = ""
		return

	if (side == "left" and _left_portrait_id == portrait_id) or \
	   (side == "right" and _right_portrait_id == portrait_id):
		target.visible = true
		return

	var path = _portrait_map.get(portrait_id, "")
	if path == "" or not ResourceLoader.exists(path):
		# 폴백
		target.visible = false
		if frame != null:
			frame.visible = false
			frame.modulate.a = 0.0
		_set_portrait_shadow_alpha(side, 0.0)
		return

	target.texture = load(path)
	target.visible = true
	_show_portrait_frame(side, portrait_id)
	var shadow := _portrait_left_shadow if side == "left" else _portrait_right_shadow
	if shadow != null:
		shadow.visible = true
		shadow.scale = Vector2(0.96, 0.96)
		var stw = create_tween()
		stw.tween_property(shadow, "scale", Vector2(1.0, 1.0), 0.28).set_trans(Tween.TRANS_SINE)
	target.scale = Vector2(0.985, 0.985)
	var tw = create_tween()
	tw.tween_property(target, "scale", Vector2(1.0, 1.0), 0.24).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)

	if side == "left":
		_left_portrait_id = portrait_id
	else:
		_right_portrait_id = portrait_id

func _uses_single_portrait_composition(speaker: String) -> bool:
	return speaker == "Arrel" or speaker == "Elia"

func _show_portrait_frame(side: String, portrait_id: String) -> void:
	var frame: PanelContainer = _portrait_left_frame if side == "left" else _portrait_right_frame
	if frame == null:
		return
	frame.add_theme_stylebox_override("panel", _make_portrait_frame_style(_portrait_accent_for_id(portrait_id)))
	frame.visible = true
	frame.modulate.a = 0.0
	var tw := create_tween()
	tw.tween_property(frame, "modulate:a", 0.70, 0.20).set_trans(Tween.TRANS_SINE)

func _portrait_accent_for_id(portrait_id: String) -> Color:
	var key := portrait_id.to_lower()
	if key.begins_with("arrel"):
		return Color(0.72, 0.76, 0.95)
	if key.begins_with("elia"):
		return Color(0.62, 0.82, 0.92)
	if key.begins_with("sable"):
		return Color(0.74, 0.62, 0.92)
	if key.begins_with("malet") or key.begins_with("mallet"):
		return Color(0.92, 0.64, 0.34)
	if key.begins_with("tobias"):
		return Color(0.74, 0.68, 0.52)
	return Color(0.82, 0.68, 0.44)

func _should_hide_portraits_for_cg_line(step: Dictionary, speaker: String) -> bool:
	if not step.has("cg"):
		return false
	var cg_ref := String(step.get("cg", "")).to_lower()
	if cg_ref == "":
		return false
	if _uses_full_scene_story_cg(cg_ref):
		return true
	if not _uses_single_portrait_composition(speaker):
		return false
	var speaker_key := speaker.to_lower()
	return cg_ref.contains(speaker_key) or cg_ref.contains("arrel_elia") or cg_ref.contains("duo")

func _uses_full_scene_story_cg(cg_ref: String) -> bool:
	return cg_ref.contains("/generated/story_") or cg_ref.contains("/generated/dialogue_")

func _clear_portraits() -> void:
	_set_portrait_side("left", "")
	_set_portrait_side("right", "")

func _opposite_side(side: String) -> String:
	return "right" if side == "left" else "left"

func _highlight_speaking_side(side: String) -> void:
	if side == "left":
		_portrait_left.modulate = PORTRAIT_BRIGHT
		_portrait_right.modulate = PORTRAIT_DIM
		_set_portrait_frame_alpha("left", 0.92)
		_set_portrait_frame_alpha("right", 0.38)
		_set_portrait_shadow_alpha("left", 0.42)
		_set_portrait_shadow_alpha("right", 0.18)
	elif side == "right":
		_portrait_right.modulate = PORTRAIT_BRIGHT
		_portrait_left.modulate = PORTRAIT_DIM
		_set_portrait_frame_alpha("right", 0.92)
		_set_portrait_frame_alpha("left", 0.38)
		_set_portrait_shadow_alpha("right", 0.42)
		_set_portrait_shadow_alpha("left", 0.18)
	else:
		_portrait_left.modulate = PORTRAIT_DIM
		_portrait_right.modulate = PORTRAIT_DIM
		_set_portrait_frame_alpha("left", 0.42)
		_set_portrait_frame_alpha("right", 0.42)
		_set_portrait_shadow_alpha("left", 0.20)
		_set_portrait_shadow_alpha("right", 0.20)

func _set_portrait_frame_alpha(side: String, alpha: float) -> void:
	var frame: PanelContainer = _portrait_left_frame if side == "left" else _portrait_right_frame
	var portrait: TextureRect = _portrait_left if side == "left" else _portrait_right
	if frame == null:
		return
	if portrait == null or not portrait.visible:
		frame.visible = false
		frame.modulate.a = 0.0
		return
	frame.visible = true
	var tw := create_tween()
	tw.tween_property(frame, "modulate:a", alpha, 0.22).set_trans(Tween.TRANS_SINE)

func _set_portrait_shadow_alpha(side: String, alpha: float) -> void:
	var shadow := _portrait_left_shadow if side == "left" else _portrait_right_shadow
	var portrait := _portrait_left if side == "left" else _portrait_right
	if shadow == null:
		return
	if portrait == null or not portrait.visible:
		shadow.visible = false
		shadow.modulate.a = 0.0
		return
	shadow.visible = true
	var tw = create_tween()
	tw.tween_property(shadow, "modulate:a", alpha, 0.22).set_trans(Tween.TRANS_SINE)

## ===================== S61: 기억 왜곡 VFX =====================

## 기억 연소 순간 — 강한 글리치 (붉은 플래시 + 색수차 분리 + 셰이크)
func _on_memory_burned(_memory) -> void:
	if not visible:
		return
	_play_burn_glitch()

func _play_burn_glitch() -> void:
	# 1. 붉은 플래시
	_glitch_overlay.color = Color(0.95, 0.25, 0.15, 0.55)
	_glitch_overlay.color = Color(0.95, 0.25, 0.15, 0.55)
	var tw_flash = create_tween()
	tw_flash.tween_property(_glitch_overlay, "color:a", 0.0, 0.9).set_trans(Tween.TRANS_EXPO)

	# S69: 연소 잔열 — 가장자리 비네트가 천천히 차오르고 더 천천히 식음
	if _ember_vignette:
		var tw_ember = create_tween()
		tw_ember.tween_property(_ember_vignette, "modulate:a", 0.85, 0.5).set_trans(Tween.TRANS_QUAD)
		tw_ember.tween_interval(1.2)
		tw_ember.tween_property(_ember_vignette, "modulate:a", 0.0, 3.5).set_trans(Tween.TRANS_SINE)

	# 2. 색수차 분리 (CG가 있을 때만)
	if _cg_current.texture != null:
		_chroma_r.texture = _cg_current.texture
		_chroma_b.texture = _cg_current.texture
		_chroma_r.modulate.a = 0.55
		_chroma_b.modulate.a = 0.55
		_chroma_r.position = Vector2(-8, -2)
		_chroma_b.position = Vector2(8, 2)
		var tw_ch = create_tween()
		tw_ch.set_parallel(true)
		tw_ch.tween_property(_chroma_r, "position", Vector2.ZERO, 0.7).set_trans(Tween.TRANS_EXPO)
		tw_ch.tween_property(_chroma_b, "position", Vector2.ZERO, 0.7).set_trans(Tween.TRANS_EXPO)
		tw_ch.tween_property(_chroma_r, "modulate:a", 0.0, 0.7)
		tw_ch.tween_property(_chroma_b, "modulate:a", 0.0, 0.7)

	# 3. 글리치 사운드 (있을 때만)
	if has_node("/root/AudioManager"):
		if AudioManager.has_method("play_sfx"):
			AudioManager.play_sfx("memory_burn")

	# 4. 텍스트 스크램블 (표시 중인 라인이 있으면 잠깐 깨진 문자로 치환 후 복원)
	if _text_label.text != "":
		var original = _text_label.text
		_text_label.text = _scramble_text(original)
		await get_tree().create_timer(0.12).timeout
		if is_instance_valid(_text_label):
			_text_label.text = original

## 왜곡된 대사에 붙는 약한 효과 (색수차만 약하게)
func _play_subtle_distortion() -> void:
	if _cg_current.texture == null:
		return
	_chroma_r.texture = _cg_current.texture
	_chroma_b.texture = _cg_current.texture
	_chroma_r.modulate.a = 0.25
	_chroma_b.modulate.a = 0.25
	_chroma_r.position = Vector2(-3, 0)
	_chroma_b.position = Vector2(3, 0)
	# 상주형: 다음 스텝에서 해제
	await get_tree().create_timer(1.2).timeout
	if not is_instance_valid(_chroma_r):
		return
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(_chroma_r, "modulate:a", 0.0, 0.4)
	tw.tween_property(_chroma_b, "modulate:a", 0.0, 0.4)
	tw.tween_property(_chroma_r, "position", Vector2.ZERO, 0.4)
	tw.tween_property(_chroma_b, "position", Vector2.ZERO, 0.4)

func _scramble_text(source: String) -> String:
	var glitch_chars = "▓▒░█▄▀#@%&*?!"
	var out = ""
	for i in source.length():
		var c = source[i]
		if c == " " or c == "\n":
			out += c
		elif randf() < 0.7:
			out += glitch_chars[randi() % glitch_chars.length()]
		else:
			out += c
	return out

## ===================== TEXT =====================

func _display_line(speaker: String, text: String) -> void:
	_dialogue_frame_art.visible = true
	_choice_frame_art.visible = false
	# S73: 책 페이지 넘기기 — 이전 줄에서 새 줄로 전환 시 종이 스윕
	if _last_displayed_text != "" and _last_displayed_text != text:
		_play_page_turn()
	_last_displayed_text = text

	_full_text = text
	_typed_chars = 0
	_typing_done = false
	_type_timer = 0.0
	_waiting_for_input = false
	_show_continue(false)

	if speaker == "":
		_name_panel.visible = false
	else:
		_name_label.text = GameManager.localized_speaker(speaker)
		_name_label.add_theme_color_override("font_color", _color_for_speaker(speaker))
		_name_panel.visible = true

	_text_label.text = ""

## S73: 페이지 넘김 효과 — 종이 엣지 + 그림자가 좌→우로 약 0.32s 스윕
func _play_page_turn() -> void:
	if _page_turn_overlay == null:
		return
	# CanvasLayer는 get_viewport_rect를 직접 못 씀 — viewport 노드 경유
	var vp = get_viewport().get_visible_rect().size
	var span = vp.x  # 화면 가로 전체를 횡단
	# 시작: 화면 왼쪽 밖
	_page_turn_overlay.position.x = -span * 0.4
	_page_turn_overlay.position.x = -span * 0.4
	_page_turn_overlay.modulate.a = 0.0
	# 위쪽으로 살짝 휘어진 듯한 인상 — 회전 -3도
	_page_turn_overlay.rotation_degrees = -2.0
	_page_turn_overlay.rotation_degrees = -2.0
	var tw = create_tween()
	tw.set_parallel(true)
	# 페이지 엣지가 화면 횡단 (왼→오른쪽 끝까지)
	tw.tween_property(_page_turn_overlay, "position:x", span, 0.32).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	# 알파: 페이드인 → 페이드아웃 (peak at 50%)
	tw.tween_property(_page_turn_overlay, "modulate:a", 0.85, 0.12).set_trans(Tween.TRANS_SINE)
	tw.chain().tween_property(_page_turn_overlay, "modulate:a", 0.0, 0.20).set_trans(Tween.TRANS_SINE)
	# 페이지 휨 — 회전 살짝 변화
	tw.parallel().tween_property(_page_turn_overlay, "rotation_degrees", 1.5, 0.32).set_trans(Tween.TRANS_SINE)
	# 종이 사운드 (있으면)
	tw.parallel().tween_property(_page_turn_overlay, "rotation_degrees", 1.5, 0.32).set_trans(Tween.TRANS_SINE)
	if has_node("/root/AudioManager") and AudioManager.has_method("play_sfx"):
		AudioManager.play_sfx("page_turn")

func _color_for_speaker(speaker: String) -> Color:
	match speaker:
		"Arrel":
			return Color(0.95, 0.85, 0.55)
		"Elia":
			return Color(0.7, 0.9, 0.75)
		"Malet":
			return Color(0.85, 0.75, 0.9)
		"Kairos":
			return Color(0.9, 0.6, 0.5)
		"Sable":
			return Color(0.6, 0.75, 0.95)
		"Tobias":
			return Color(0.85, 0.8, 0.7)
		_:
			return Color(0.9, 0.9, 0.9)

func _process(delta: float) -> void:
	# S69: 필름 그레인 시간 업데이트 (매 프레임 노이즈 패턴 변경)
	_film_grain_time += delta
	if _film_grain and _film_grain.material is ShaderMaterial:
		(_film_grain.material as ShaderMaterial).set_shader_parameter("u_time", _film_grain_time)

	if _typing_done or _full_text == "":
		return
	_type_timer += delta
	while _type_timer >= TYPEWRITER_SPEED and _typed_chars < _full_text.length():
		_type_timer -= TYPEWRITER_SPEED
		_typed_chars += 1
		_text_label.text = _full_text.substr(0, _typed_chars)
	if _typed_chars >= _full_text.length():
		_typing_done = true
		_waiting_for_input = true
		_show_continue(true)

func _show_continue(on: bool) -> void:
	if _continue_tween:
		_continue_tween.kill()
	_continue_tween = null
	_continue_indicator.visible = on
	if not on:
		_continue_indicator.modulate.a = 1.0
		return
	_continue_indicator.modulate.a = 0.72
	_continue_tween = create_tween().set_loops()
	_continue_tween.tween_property(_continue_indicator, "modulate:a", 1.0, 0.7).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	_continue_tween.tween_property(_continue_indicator, "modulate:a", 0.54, 0.7).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)

## ===================== SYSTEM LOG =====================

func _show_system_log(msg: String) -> void:
	_dialogue_frame_art.visible = true
	_choice_frame_art.visible = false
	# 대화박스를 시스템 로그 스타일로 표시 (SYSTEM 라벨 + 청록색)
	_name_panel.visible = true
	_name_label.text = GameManager.localized_speaker("System")
	_name_label.add_theme_color_override("font_color", Color(0.5, 0.85, 0.95))
	_full_text = msg
	_typed_chars = msg.length()
	_typing_done = true
	_text_label.text = msg

## ===================== CHOICES =====================

func _show_choices(choices: Array) -> void:
	# 기존 버튼 제거
	for c in _choice_container.get_children():
		c.queue_free()

	_choice_container.visible = true
	_choice_header.visible = true
	_choice_hint.visible = true
	_dialogue_frame_art.visible = false
	_choice_frame_art.visible = true
	_choice_header.text = GameManager.localized_value(_current_step, "choice_title", "결정" if GameManager.current_locale == "ko" else "DECISION")
	_choice_hint.text = GameManager.localized_value(_current_step, "choice_hint", "어떤 선택은 아렐이 지킬 것, 잃을 것, 살아남는 방식을 바꿉니다." if GameManager.current_locale == "ko" else "Some choices change what Arrel can keep, spend, or survive.")
	_text_panel.visible = false
	_name_panel.visible = false
	_show_continue(false)

	# S69: 선택지 등장 시 배경 CG/포트레이트 덤 — 결정 무게 강조
	_dim_background_for_choice(true)

	for i in range(choices.size()):
		var c: Dictionary = choices[i]
		# 조건부 선택지 (기억 필요 등)
		if c.has("requires_memory_intact"):
			if not MemoryManager.is_intact(String(c.requires_memory_intact)):
				continue
		# S70: 플래그 게이팅 — 특정 플래그가 set 되어야 노출
		if c.has("requires_flag"):
			if not GameManager.story_flags.get(c.requires_flag, false):
				continue
		if c.has("requires_not_flag"):
			if GameManager.story_flags.get(c.requires_not_flag, false):
				continue

		# S63: Memory Leverage — cost_memory 필드가 있으면 태울 기억 상태 체크
		var cost_mem_id: String = c.get("cost_memory", c.get("burn_memory", ""))
		var cost_mem = null
		var is_cost_choice: bool = c.has("cost_memory")  # 명시적 cost (UI 하이라이트용)
		if cost_mem_id != "":
			cost_mem = MemoryManager.find_memory(cost_mem_id)
			# 이미 태워진 기억이면 이 선택지 비활성화 (leverage 불가)
			if cost_mem != null and (cost_mem.is_burned or cost_mem.is_faded):
				continue
			if cost_mem == null:
				# 소실된 기억도 선택 불가
				continue

		var btn = Button.new()
		var label_text = GameManager.localized_value(c, "text", String(c.get("text", "...")))
		var extra_lines := 0
		if is_cost_choice and cost_mem != null:
			# S149: 등급 표기 — 무엇을 태우는지 무게가 보이게 (GRADE_5=0 → G5)
			var grade_num: int = 5 - int(cost_mem.grade)
			var burn_label := "연소" if GameManager.current_locale == "ko" else "Burn"
			label_text = "✦  %s\n    [%s G%d: %s]" % [label_text, burn_label, grade_num, cost_mem.title]
			# S149: 잃는 것 미리보기 — story_effect가 정의된 기억은 대가를 명시
			if cost_mem.story_effect != "" and not cost_mem.story_effect.begins_with("ENDING"):
				var effect_preview: String = cost_mem.story_effect
				if GameManager.current_locale == "ko":
					effect_preview = "연소하면 이 기억이 열어 둔 서사적 가능성을 잃습니다."
				label_text += "\n    — %s" % effect_preview
				extra_lines += 1
		elif c.has("requires_memory_intact"):
			# S149: 기억 열쇠 선택지 — 간직한 기억이 길을 연다는 것을 명시
			var key_mem = MemoryManager.find_memory(String(c.requires_memory_intact))
			if key_mem != null:
				var kept_label := "간직" if GameManager.current_locale == "ko" else "Kept"
				label_text = "✧  %s\n    [%s: %s]" % [label_text, kept_label, key_mem.title]
				extra_lines += 1
		if c.has("effect"):
			label_text += "\n    %s" % GameManager.localized_value(c, "effect", String(c.effect))
		btn.text = label_text
		var button_height := 76 if c.has("effect") else (64 if is_cost_choice else 54)
		button_height += extra_lines * 18
		btn.custom_minimum_size = Vector2(680, button_height)
		UITheme.apply_body_font(btn)
		btn.add_theme_font_size_override("font_size", 17)
		var is_key_choice: bool = (not is_cost_choice) and c.has("requires_memory_intact")
		var bstyle = StyleBoxFlat.new()
		bstyle.bg_color = Color(0.030, 0.026, 0.038, 0.94)
		bstyle.border_color = Color(0.62, 0.48, 0.28, 0.62)
		if is_cost_choice:
			bstyle.bg_color = Color(0.13, 0.065, 0.052, 0.94)
			bstyle.border_color = Color(0.9, 0.45, 0.3, 0.84)
		elif is_key_choice:
			# S149: 기억 열쇠 — 보존이 열어 준 길 (연소의 적색과 대비되는 청록)
			bstyle.bg_color = Color(0.045, 0.085, 0.095, 0.94)
			bstyle.border_color = Color(0.35, 0.78, 0.75, 0.85)
		bstyle.set_border_width(SIDE_LEFT, 2)
		bstyle.set_border_width(SIDE_TOP, 1)
		bstyle.set_border_width(SIDE_RIGHT, 1)
		bstyle.set_border_width(SIDE_BOTTOM, 1)
		bstyle.set_content_margin_all(12)
		bstyle.set_corner_radius_all(4)
		btn.add_theme_stylebox_override("normal", bstyle)
		var hover = bstyle.duplicate()
		hover.bg_color = Color(0.12, 0.092, 0.080, 0.98)
		if is_cost_choice:
			hover.bg_color = Color(0.28, 0.12, 0.08, 0.98)
			hover.border_color = Color(1.0, 0.55, 0.35, 1.0)
		elif is_key_choice:
			hover.bg_color = Color(0.07, 0.16, 0.17, 0.98)
			hover.border_color = Color(0.5, 1.0, 0.95, 1.0)
		else:
			hover.border_color = Color(0.9, 0.75, 0.45, 1.0)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_stylebox_override("focus", hover)
		btn.add_theme_color_override("font_color", Color(0.94, 0.9, 0.82))
		btn.add_theme_color_override("font_hover_color", Color(1.0, 0.88, 0.58))
		btn.pressed.connect(_on_choice_selected.bind(i))
		btn.modulate.a = 0.0
		btn.scale = Vector2(0.99, 0.99)
		_choice_container.add_child(btn)

	if _choice_container.get_child_count() == 0:
		_add_no_available_choice_button()
		return

	for ci in range(_choice_container.get_child_count()):
		var child_btn = _choice_container.get_child(ci)
		var tw = create_tween()
		tw.set_parallel(true)
		tw.tween_property(child_btn, "modulate:a", 1.0, 0.16).set_delay(ci * 0.045).set_ease(Tween.EASE_OUT)
		tw.tween_property(child_btn, "scale", Vector2.ONE, 0.16).set_delay(ci * 0.045).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_SINE)

func _add_no_available_choice_button() -> void:
	_choice_header.text = "결정 보류" if GameManager.current_locale == "ko" else "DECISION HELD"
	_choice_hint.text = "지금 고를 수 있는 선택지가 없습니다. 이야기를 계속합니다." if GameManager.current_locale == "ko" else "No available choice can be taken right now. Continue the scene."
	var btn := Button.new()
	btn.text = "계속" if GameManager.current_locale == "ko" else "Continue"
	btn.custom_minimum_size = Vector2(680, 54)
	UITheme.apply_body_font(btn)
	btn.add_theme_font_size_override("font_size", 17)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.030, 0.026, 0.038, 0.94)
	style.border_color = Color(0.62, 0.48, 0.28, 0.62)
	style.set_border_width_all(1)
	style.set_content_margin_all(12)
	style.set_corner_radius_all(4)
	btn.add_theme_stylebox_override("normal", style)
	var hover := style.duplicate()
	hover.bg_color = Color(0.12, 0.092, 0.080, 0.98)
	hover.border_color = Color(0.9, 0.75, 0.45, 1.0)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)
	btn.add_theme_color_override("font_color", Color(0.94, 0.9, 0.82))
	btn.pressed.connect(_on_no_available_choice_continue)
	_choice_container.add_child(btn)

func _on_no_available_choice_continue() -> void:
	if GameManager.current_state != GameManager.GameState.DIALOGUE:
		return
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_select")
	_choice_container.visible = false
	_choice_header.visible = false
	_choice_hint.visible = false
	_text_panel.visible = true
	_dialogue_frame_art.visible = true
	_choice_frame_art.visible = false
	_dim_background_for_choice(false)
	SceneFlow.advance()

func _on_choice_selected(index: int) -> void:
	if GameManager.current_state != GameManager.GameState.DIALOGUE:
		return
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_select")
	_choice_container.visible = false
	_choice_header.visible = false
	_choice_hint.visible = false
	_text_panel.visible = true
	_dialogue_frame_art.visible = true
	_choice_frame_art.visible = false
	# S69: 선택지 닫히면 배경 복귀
	_dim_background_for_choice(false)
	SceneFlow.select_choice(index)

## S69: 선택지 표시 시 CG/포트레이트 덤 (또는 복귀)
func _dim_background_for_choice(dim: bool) -> void:
	var target_cg = Color(0.6, 0.6, 0.65, 1.0) if dim else Color(1, 1, 1, 1)
	var target_portrait = Color(0.5, 0.5, 0.55, 1.0) if dim else Color(1, 1, 1, 1)
	var dur = 0.45
	var tw = create_tween()
	tw.set_parallel(true)
	if _cg_current.modulate.a > 0.5:
		# 알파는 유지하면서 색만 어둡게
		var c = target_cg
		c.a = _cg_current.modulate.a
		tw.tween_property(_cg_current, "modulate", c, dur).set_trans(Tween.TRANS_SINE)
	if _portrait_left and _portrait_left.texture:
		tw.tween_property(_portrait_left, "modulate", target_portrait, dur)
	if _portrait_right and _portrait_right.texture:
		tw.tween_property(_portrait_right, "modulate", target_portrait, dur)

## ===================== INPUT =====================

func _input(event: InputEvent) -> void:
	# S61b: VN 런너가 비활성일 때는 입력 처리 안 함 (탐색 맵 DialogueBox에 입력 양보)
	if not SceneFlow.is_active or GameManager.current_state != GameManager.GameState.DIALOGUE:
		return
	if _choice_container.visible:
		return  # 선택지 중에는 진행 불가

	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_try_advance()
	elif event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ENTER or event.keycode == KEY_SPACE or event.keycode == KEY_E:
			_try_advance()

func _try_advance() -> void:
	if not _typing_done:
		# 즉시 완성
		_typed_chars = _full_text.length()
		_text_label.text = _full_text
		_typing_done = true
		_waiting_for_input = true
		_show_continue(true)
		return
	if _waiting_for_input:
		_waiting_for_input = false
		_show_continue(false)
		SceneFlow.advance()
