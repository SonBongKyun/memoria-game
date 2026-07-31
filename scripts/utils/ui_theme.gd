## UITheme, MEMORIA 공통 UI 색상/스타일 상수
## 모든 UI에서 일관된 테마 사용을 위한 유틸리티.
class_name UITheme

# ── 기본 팔레트 ──
const BG_DARK := Color(0.06, 0.05, 0.08)         # 가장 어두운 배경
const BG_PANEL := Color(0.055, 0.047, 0.070, 0.97) # 패널 배경
const BG_OVERLAY := Color(0.015, 0.015, 0.028, 0.94) # 풀스크린 오버레이
const BORDER := Color(0.48, 0.39, 0.27, 0.88)      # 테두리 (앰버)
const BORDER_DIM := Color(0.36, 0.30, 0.22, 0.68)  # 연한 테두리

# ── 텍스트 색상 ──
const TEXT_PRIMARY := Color(0.96, 0.94, 0.91)     # 일반 대사
const TEXT_NARRATION := Color(0.87, 0.84, 0.80)   # 나레이션
const TEXT_SYSTEM := Color(0.62, 0.88, 0.78)      # 시스템 로그
const TEXT_DIM := Color(0.70, 0.68, 0.66)         # 보조/힌트
const TEXT_ACCENT := Color(0.94, 0.76, 0.47)      # 강조 (제목)
const TEXT_OUTLINE := Color(0.0, 0.0, 0.0, 0.92)
const MIN_META_FONT_SIZE := 12
const MIN_UI_FONT_SIZE := 13
const MIN_BODY_FONT_SIZE := 20

# ── 캐릭터별 이름 색상 ──
const SPEAKER_COLORS := {
	"Arrel": Color(0.72, 0.82, 1.0),     # 은청색
	"Elia": Color(0.86, 0.90, 1.0),      # 은빛 라벤더
	"Sable": Color(0.82, 0.72, 0.94),    # 짙은 보라
	"Malet": Color(0.96, 0.78, 0.48),    # 앰버
	"Kairos": Color(0.68, 0.90, 0.80),   # 차가운 청록
	"???": Color(0.76, 0.76, 0.82),      # 불명
}
const SPEAKER_DEFAULT := Color(0.94, 0.76, 0.47)

# ── 등급 색상 ──
const GRADE_COLORS := [
	Color(0.5, 0.5, 0.45),     # Grade 5, 회색
	Color(0.55, 0.5, 0.35),    # Grade 4, 갈색
	Color(0.4, 0.5, 0.6),      # Grade 3, 청색
	Color(0.6, 0.45, 0.55),    # Grade 2, 보라
	Color(0.7, 0.55, 0.3),     # Grade 1, 금색
]

# ── 전투 색상 ──
const HP_PLAYER := Color(0.2, 0.45, 0.6)
const HP_ENEMY := Color(0.6, 0.15, 0.15)
const HP_LOW := Color(0.7, 0.25, 0.15)            # HP 25% 이하

# ── 패널 스타일 생성 헬퍼 ──
static func make_panel_style(bg: Color = BG_PANEL, border: Color = BORDER, border_width: int = 2, radius: int = 4, margin: int = 12) -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = bg
	style.border_color = border
	style.set_border_width_all(border_width)
	style.set_corner_radius_all(radius)
	style.set_content_margin_all(margin)
	return style

static func make_button_style(bg: Color = Color(0.12, 0.1, 0.14, 0.9), border: Color = Color(0.4, 0.3, 0.25, 0.6)) -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = bg
	style.border_color = border
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(8)
	return style

static func make_hover_style(base: StyleBoxFlat = null) -> StyleBoxFlat:
	if base:
		var hover = base.duplicate()
		hover.bg_color = base.bg_color.lightened(0.15)
		hover.border_color = Color(0.7, 0.55, 0.35, 0.8)
		return hover
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.2, 0.15, 0.22, 0.95)
	style.border_color = Color(0.7, 0.55, 0.35, 0.8)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(8)
	return style

# ── 폰트 렌더링 품질 ──
## 폰트를 프로젝트에 포함해 PC별 폴백 차이와 일부 한글 글리프 깨짐을 없앤다.
## 작은 UI에서는 밉맵과 서브픽셀 위치가 획을 갈라 보이게 할 수 있어 비활성화한다.
const TITLE_FONT_PATH := "res://assets/fonts/NotoSerifKR-VF.ttf"
const BODY_FONT_PATH := "res://assets/fonts/NotoSansKR-VF.ttf"
const UI_FONT_PATH := "res://assets/fonts/NotoSansKR-VF.ttf"

static var _title_font: FontFile
static var _body_font: Font
static var _ui_font: Font

static func _load_bundled_font(path: String) -> FontFile:
	var font := load(path) as FontFile
	assert(font != null, "Bundled font is missing: %s" % path)
	font.antialiasing = TextServer.FONT_ANTIALIASING_GRAY
	font.hinting = TextServer.HINTING_NORMAL
	font.subpixel_positioning = TextServer.SUBPIXEL_POSITIONING_DISABLED
	font.generate_mipmaps = false
	return font

static func make_title_font() -> Font:
	if _title_font == null:
		_title_font = _load_bundled_font(TITLE_FONT_PATH)
	return _title_font

static func _make_screen_font(path: String, embolden: float) -> FontVariation:
	var variation := FontVariation.new()
	variation.base_font = _load_bundled_font(path)
	variation.variation_embolden = embolden
	return variation

static func make_body_font() -> Font:
	if _body_font == null:
		_body_font = _make_screen_font(BODY_FONT_PATH, 0.32)
	return _body_font

static func make_ui_font() -> Font:
	if _ui_font == null:
		_ui_font = _make_screen_font(UI_FONT_PATH, 0.42)
	return _ui_font

static func get_font_file(font: Font) -> FontFile:
	if font is FontFile:
		return font as FontFile
	if font is FontVariation:
		return get_font_file((font as FontVariation).base_font)
	return null

static func apply_title_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_title_font())

static func apply_body_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_body_font())

static func apply_ui_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_ui_font())

static func apply_readability_finish(control: Control, font_size: int, color: Color = TEXT_PRIMARY, body_copy: bool = false) -> void:
	if control == null:
		return
	if control is RichTextLabel:
		control.add_theme_font_override("normal_font", make_body_font() if body_copy else make_ui_font())
		control.add_theme_font_size_override("normal_font_size", font_size)
		control.add_theme_color_override("default_color", color)
		control.add_theme_color_override("font_outline_color", TEXT_OUTLINE)
		control.add_theme_constant_override("outline_size", 1)
		control.add_theme_constant_override("line_separation", 9)
	else:
		control.add_theme_font_override("font", make_body_font() if body_copy else make_ui_font())
		control.add_theme_font_size_override("font_size", font_size)
		control.add_theme_color_override("font_color", color)
		control.add_theme_color_override("font_outline_color", TEXT_OUTLINE)
		control.add_theme_constant_override("outline_size", 1)

## 화자 이름 색상 가져오기
static func get_speaker_color(speaker: String) -> Color:
	match speaker:
		"Mallet":
			return Color(0.96, 0.78, 0.48)
		"Nera":
			return Color(0.76, 0.86, 1.0)
		"Seric":
			return Color(0.90, 0.82, 0.66)
		"Tobias":
			return Color(0.86, 0.75, 0.66)
		"Veil":
			return Color(0.76, 0.70, 0.94)
	return SPEAKER_COLORS.get(speaker, SPEAKER_DEFAULT)
