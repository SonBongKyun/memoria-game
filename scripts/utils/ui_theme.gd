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
const MIN_META_FONT_SIZE := 13
const MIN_UI_FONT_SIZE := 14
const MIN_BODY_FONT_SIZE := 20

# ── 타입 스케일 (1280x720 기준) ──
## S230: 화면마다 12/13/14/15/17을 즉흥적으로 쓰던 것을 한 단계표로 모았다.
## 새 UI는 숫자를 직접 쓰지 말고 이 상수를 쓴다.
const SIZE_DISPLAY := 40   # 타이틀 로고급
const SIZE_TITLE := 26     # 화면 제목
const SIZE_HEADING := 20   # 패널 제목
const SIZE_BODY := 20      # 대사/나레이션 본문
const SIZE_UI := 16        # 버튼, 주요 HUD 수치
const SIZE_LABEL := 14     # 보조 라벨
const SIZE_META := 13      # 칩, 상태 태그

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
##
## S230 — 가변 폰트의 "진짜" 굵기를 쓴다.
## 번들된 두 폰트는 가변 폰트이고, 기본 인스턴스가 가장 얇은 마스터다.
##   NotoSansKR-VF  : wght 100..900, 기본 100 (Thin)
##   NotoSerifKR-VF : wght 200..900, 기본 200 (ExtraLight)
## 이전에는 그 얇은 마스터에 variation_embolden(가짜 굵기)을 덧씌워 획을 부풀렸다.
## 가짜 굵기는 글리프 외곽선을 균일하게 밀어내기 때문에, 획이 촘촘한 한글에서
## 속공간이 메워지고 가로획이 뭉개진다. 이제는 wght 축 인스턴스를 직접 요청한다.
const SANS_FONT_PATH := "res://assets/fonts/NotoSansKR-VF.ttf"
const SERIF_FONT_PATH := "res://assets/fonts/NotoSerifKR-VF.ttf"

## 세리프 = 이야기(대사/나레이션/제목), 산세리프 = 인터페이스(버튼/HUD/수치).
const TITLE_FONT_PATH := SERIF_FONT_PATH
const BODY_FONT_PATH := SERIF_FONT_PATH
const UI_FONT_PATH := SANS_FONT_PATH

const WEIGHT_REGULAR := 400
const WEIGHT_MEDIUM := 500
const WEIGHT_SEMIBOLD := 600
const WEIGHT_BOLD := 700

## 어두운 배경 위 흰 글자는 시각적으로 더 굵어 보이므로, 본문은 Medium까지만 올린다.
const BODY_WEIGHT := WEIGHT_MEDIUM
const UI_WEIGHT := WEIGHT_SEMIBOLD
const META_WEIGHT := WEIGHT_MEDIUM
const TITLE_WEIGHT := WEIGHT_SEMIBOLD

static var _font_files: Dictionary = {}
static var _font_variations: Dictionary = {}

static func _load_bundled_font(path: String) -> FontFile:
	if _font_files.has(path):
		return _font_files[path]
	var font := load(path) as FontFile
	assert(font != null, "Bundled font is missing: %s" % path)
	font.antialiasing = TextServer.FONT_ANTIALIASING_GRAY
	font.hinting = TextServer.HINTING_NORMAL
	font.subpixel_positioning = TextServer.SUBPIXEL_POSITIONING_DISABLED
	font.generate_mipmaps = false
	_font_files[path] = font
	return font

## 지정한 굵기의 실제 가변 폰트 인스턴스. 같은 조합은 한 번만 만든다.
static func make_weighted_font(path: String, weight: int) -> FontVariation:
	var key := "%s@%d" % [path, weight]
	if _font_variations.has(key):
		return _font_variations[key]
	var variation := FontVariation.new()
	variation.base_font = _load_bundled_font(path)
	variation.variation_opentype = {"wght": weight}
	_font_variations[key] = variation
	return variation

static func make_title_font() -> Font:
	return make_weighted_font(TITLE_FONT_PATH, TITLE_WEIGHT)

static func make_body_font() -> Font:
	return make_weighted_font(BODY_FONT_PATH, BODY_WEIGHT)

static func make_ui_font() -> Font:
	return make_weighted_font(UI_FONT_PATH, UI_WEIGHT)

## 칩·상태 태그처럼 작고 촘촘한 글자. UI 굵기보다 한 단계 가벼워야 뭉치지 않는다.
static func make_meta_font() -> Font:
	return make_weighted_font(UI_FONT_PATH, META_WEIGHT)

## 폰트가 실제로 요청하고 있는 wght 값. 축이 없으면 0을 돌려준다.
static func get_font_weight(font: Font) -> int:
	if not (font is FontVariation):
		return 0
	var axes: Dictionary = (font as FontVariation).variation_opentype
	for key in axes.keys():
		if String(key) == "wght":
			return int(axes[key])
	var server := TextServerManager.get_primary_interface()
	if server:
		var numeric_tag := server.name_to_tag("wght")
		if axes.has(numeric_tag):
			return int(axes[numeric_tag])
	return 0

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

static func apply_meta_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_meta_font())

## S230: 라벨 한 개의 서체·크기·색·외곽선을 한 번에 확정한다.
## 전투 HUD처럼 라벨이 수십 개인 화면에서 크기/외곽선 누락을 막는다.
static func style_label(control: Control, font: Font, font_size: int, color: Color, outline: int = 1) -> void:
	if control == null:
		return
	if control is RichTextLabel:
		control.add_theme_font_override("normal_font", font)
		control.add_theme_font_size_override("normal_font_size", font_size)
		control.add_theme_color_override("default_color", color)
		control.add_theme_color_override("font_outline_color", TEXT_OUTLINE)
		control.add_theme_constant_override("outline_size", outline)
		return
	control.add_theme_font_override("font", font)
	control.add_theme_font_size_override("font_size", font_size)
	if control is Button:
		control.add_theme_color_override("font_color", color)
		control.add_theme_color_override("font_outline_color", TEXT_OUTLINE)
		control.add_theme_constant_override("outline_size", outline)
		return
	control.add_theme_color_override("font_color", color)
	control.add_theme_color_override("font_outline_color", TEXT_OUTLINE)
	control.add_theme_constant_override("outline_size", outline)

## HUD 수치·라벨용 단축. 크기 하한을 강제한다.
static func style_ui_label(control: Control, color: Color = TEXT_PRIMARY, font_size: int = SIZE_UI) -> void:
	style_label(control, make_ui_font(), maxi(font_size, MIN_UI_FONT_SIZE), color)

## 칩·상태 태그용 단축.
static func style_meta_label(control: Control, color: Color = TEXT_DIM, font_size: int = SIZE_META) -> void:
	style_label(control, make_meta_font(), maxi(font_size, MIN_META_FONT_SIZE), color)

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
