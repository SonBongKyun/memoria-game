## UITheme — MEMORIA 공통 UI 색상/스타일 상수
## 모든 UI에서 일관된 테마 사용을 위한 유틸리티.
class_name UITheme

# ── 기본 팔레트 ──
const BG_DARK := Color(0.06, 0.05, 0.08)         # 가장 어두운 배경
const BG_PANEL := Color(0.08, 0.07, 0.1, 0.92)    # 패널 배경
const BG_OVERLAY := Color(0.02, 0.02, 0.04, 0.85) # 풀스크린 오버레이
const BORDER := Color(0.3, 0.25, 0.2, 0.8)        # 테두리 (앰버)
const BORDER_DIM := Color(0.25, 0.2, 0.15, 0.5)   # 연한 테두리

# ── 텍스트 색상 ──
const TEXT_PRIMARY := Color(0.85, 0.82, 0.78)     # 일반 대사
const TEXT_NARRATION := Color(0.6, 0.55, 0.5)     # 나레이션
const TEXT_SYSTEM := Color(0.3, 0.65, 0.55)       # 시스템 로그
const TEXT_DIM := Color(0.5, 0.45, 0.4)           # 보조/힌트
const TEXT_ACCENT := Color(0.75, 0.6, 0.4)        # 강조 (제목)

# ── 캐릭터별 이름 색상 ──
const SPEAKER_COLORS := {
	"Arrel": Color(0.55, 0.65, 0.85),    # 은청색
	"Elia": Color(0.7, 0.75, 0.85),      # 은빛 라벤더
	"Sable": Color(0.6, 0.5, 0.65),      # 짙은 보라
	"Malet": Color(0.75, 0.6, 0.4),      # 앰버
	"Kairos": Color(0.5, 0.7, 0.6),      # 차가운 청록
	"???": Color(0.5, 0.5, 0.55),        # 불명
}
const SPEAKER_DEFAULT := Color(0.75, 0.6, 0.4)

# ── 등급 색상 ──
const GRADE_COLORS := [
	Color(0.5, 0.5, 0.45),     # Grade 5 — 회색
	Color(0.55, 0.5, 0.35),    # Grade 4 — 갈색
	Color(0.4, 0.5, 0.6),      # Grade 3 — 청색
	Color(0.6, 0.45, 0.55),    # Grade 2 — 보라
	Color(0.7, 0.55, 0.3),     # Grade 1 — 금색
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

# ── 폰트 렌더링 품질 (다크판타지 VN 톤을 위한 부드러운 세리프 렌더) ──
## SystemFont에 공통 렌더링 설정 적용 — 서브픽셀/라이트 힌팅/밉맵으로
## 세리프 글자 가장자리를 매끈하게, 스케일 시 뭉개짐 없이.
static func _tune_font(font: SystemFont) -> SystemFont:
	font.antialiasing = TextServer.FONT_ANTIALIASING_GRAY
	font.hinting = TextServer.HINTING_LIGHT
	font.subpixel_positioning = TextServer.SUBPIXEL_POSITIONING_AUTO
	font.generate_mipmaps = true
	font.allow_system_fallback = true
	return font

## 한국어 우선 스택은 명조(세리프) 계열로 — 분위기의 핵심.
## Noto Serif KR(Win10+ 기본 탑재) → Batang(항상 존재) → Nanum Myeongjo → 고딕 폴백.
static func make_title_font() -> SystemFont:
	var font := SystemFont.new()
	var latin_names := [
		"Cinzel",
		"Trajan Pro",
		"Cormorant SC",
		"Cormorant Garamond",
		"Noto Serif KR",
		"Georgia",
		"Constantia",
		"serif",
	]
	# 제목: 획이 굵고 고전적인 명조 — Gungsuh(궁서)까지 폴백으로 무게감 부여
	var korean_names := ["Noto Serif KR", "Batang", "Nanum Myeongjo", "Gungsuh", "Malgun Gothic", "serif"]
	font.font_names = PackedStringArray(korean_names if GameManager.current_locale == "ko" else latin_names)
	return _tune_font(font)

static func make_body_font() -> SystemFont:
	var font := SystemFont.new()
	var latin_names := [
		"Cormorant Garamond",
		"EB Garamond",
		"Noto Serif KR",
		"Georgia",
		"Constantia",
		"Palatino Linotype",
		"serif",
	]
	# 본문: 가독성 높은 명조 — 시스템 UI 고딕(Malgun) 대신 세리프 우선
	var korean_names := ["Noto Serif KR", "Batang", "Nanum Myeongjo", "Malgun Gothic", "serif"]
	font.font_names = PackedStringArray(korean_names if GameManager.current_locale == "ko" else latin_names)
	return _tune_font(font)

## UI(버튼/HUD)는 깔끔한 산세리프 유지 — 정보 가독성 우선.
static func make_ui_font() -> SystemFont:
	var font := SystemFont.new()
	font.font_names = PackedStringArray([
		"Pretendard",
		"Malgun Gothic",
		"Noto Sans KR",
		"Segoe UI",
		"Arial",
		"sans-serif",
	])
	return _tune_font(font)

static func apply_title_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_title_font())

static func apply_body_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_body_font())

static func apply_ui_font(control: Control) -> void:
	if control:
		control.add_theme_font_override("font", make_ui_font())

## 화자 이름 색상 가져오기
static func get_speaker_color(speaker: String) -> Color:
	match speaker:
		"Mallet":
			return Color(0.75, 0.6, 0.4)
		"Nera":
			return Color(0.62, 0.7, 0.82)
		"Seric":
			return Color(0.74, 0.68, 0.54)
		"Tobias":
			return Color(0.68, 0.58, 0.50)
		"Veil":
			return Color(0.54, 0.48, 0.70)
	return SPEAKER_COLORS.get(speaker, SPEAKER_DEFAULT)
