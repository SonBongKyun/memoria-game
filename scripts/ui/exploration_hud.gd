## ExplorationHUD (Autoload)
## 탐색 중 좌상단에 HP/챕터/기억 정보를 표시하는 HUD.
## S57: Steam-quality upgrade, ghost HP bar, status icons, quest progress bar,
##      memory burn glow, grains popup, slide-in animation.
extends CanvasLayer

# ── 맵 이름 매핑 ──
const MAP_NAMES := {
	"rim_forest": "Rim Forest",
	"verdan_market": "Verdan Market",
	"belt_waystation": "Belt Waystation",
	"drift_shelter": "Drift Shelter",
	"crumbling_coast": "Crumbling Coast",
	"the_seam": "The Seam",
	"seam_outskirts": "Seam Outskirts",
	"forgotten_forest": "Forgotten Forest",
	"colorless_waste": "Colorless Waste",
	"bl07_void": "BL-07 Void",
	# S242: 선택 기억 장소 아홉 곳. 이름표에 등록된 적이 없어서, 이름을 찾지 못한
	# 자리는 원시 씬 ID로 되돌아갔다. 도감의 미발견 항목이 플레이어에게
	# "??? · rim_root_hollow"처럼 내부 식별자를 그대로 보여 주고 있었다.
	# 값은 각 장소 씬이 이미 선언한 title_en/title_ko를 그대로 옮긴 것이다.
	"rim_root_hollow": "Root Hollow",
	"verdan_ledger_cellar": "Ledger Cellar",
	"belt_signal_yard": "Signal Yard",
	"drift_waymarker_shrine": "Waymarker Shrine",
	"coast_cinder_harbor": "Cinder Harbor",
	"seam_lantern_ward": "Lantern Ward",
	"forest_name_hollow": "Name Hollow",
	"waste_grey_caravan": "Grey Caravan",
	"bl07_seed_vault": "Seed Vault",
}

const MAP_NAMES_KO := {
	"rim_forest": "림 숲",
	"verdan_market": "베르단 시장",
	"belt_waystation": "벨트 중계소",
	"drift_shelter": "표류 대피소",
	"crumbling_coast": "무너지는 해안",
	"the_seam": "심",
	"seam_outskirts": "심 외곽",
	"forgotten_forest": "잊힌 숲",
	"colorless_waste": "무색 황무지",
	"bl07_void": "BL-07 보이드",
	"rim_root_hollow": "기록목 뿌리 공동",
	"verdan_ledger_cellar": "기억 대출 장부실",
	"belt_signal_yard": "신호 야적장",
	"drift_waymarker_shrine": "이정표 성소",
	"coast_cinder_harbor": "신더 항구",
	"seam_lantern_ward": "랜턴 구역",
	"forest_name_hollow": "이름의 골짜기",
	"waste_grey_caravan": "회색 캐러밴",
	"bl07_seed_vault": "씨앗 금고",
}

const MAP_ART := {
	"rim_forest": "res://assets/cg/generated/story_ch1_twisted_forest_path.png",
	"verdan_market": "res://assets/cg/generated/chapter_splash_verdan_market.png",
	"belt_waystation": "res://assets/cg/generated/chapter_splash_belt_waystation.png",
	"drift_shelter": "res://assets/cg/generated/chapter_splash_drift_shelter.png",
	"crumbling_coast": "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
	"the_seam": "res://assets/cg/generated/chapter_splash_the_seam.png",
	"seam_outskirts": "res://assets/cg/generated/chapter_splash_seam_outskirts.png",
	"forgotten_forest": "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
	"colorless_waste": "res://assets/cg/generated/memory_compass_resonance_cinematic.png",
	"bl07_void": "res://assets/cg/generated/chapter_splash_bl07_void.png",
}

# Clean runtime labels kept separate from the legacy source block above so
# Korean UI never exposes mojibake inherited from older editor encodings.
const MAP_NAMES_KO_CLEAN := {
	"rim_forest": "림 숲",
	"verdan_market": "베르단 시장",
	"belt_waystation": "벨트 중계소",
	"drift_shelter": "표류 대피소",
	"crumbling_coast": "무너지는 해안",
	"the_seam": "더 심",
	"seam_outskirts": "심 외곽",
	"forgotten_forest": "망각의 숲",
	"colorless_waste": "무색 황무지",
	"bl07_void": "BL-07 공허",
	"rim_root_hollow": "기록목 뿌리 공동",
	"verdan_ledger_cellar": "기억 대출 장부실",
	"belt_signal_yard": "신호 야적장",
	"drift_waymarker_shrine": "이정표 성소",
	"coast_cinder_harbor": "신더 항구",
	"seam_lantern_ward": "랜턴 구역",
	"forest_name_hollow": "이름의 골짜기",
	"waste_grey_caravan": "회색 캐러밴",
	"bl07_seed_vault": "씨앗 금고",
}

# ── 노드 참조 ──
const HUD_PLATE_PATH: String = "res://assets/cg/generated/ui_exploration_hud_plate.png"
const HUD_ARCHIVE_OVERLAY_PATH: String = "res://assets/cg/generated/ui_exploration_archive_overlay.png"

var hud_plate_art: TextureRect
var panel: PanelContainer
var identity_label: Label
var hp_label: Label
var hp_bar: ProgressBar
var hp_ghost_bar: ProgressBar  # S57: ghost drain bar
var hp_value_label: Label
var chapter_label: Label
var memory_label: Label
var grains_label: Label
var items_label: Label
var pulse_label: Label
var equip_label: Label    # S41
var quest_card: PanelContainer
var quest_tag_label: Label
var quest_label: Label    # S41
var quest_progress_bar: ProgressBar  # S57: visual quest progress
var quest_side_label: Label  # S216: 진행 중인 의뢰
var status_icons_row: HBoxContainer  # S57: status effect icons
var controls_panel: PanelContainer
var controls_label: Label
var flow_panel: PanelContainer
## S230: 어떤 상태에서도 Field Flow 문장은 읽혀야 한다. 이 아래로는 내리지 않는다.
const FLOW_PANEL_MIN_ALPHA := 0.88
var flow_bar: ProgressBar
var pressure_bar: ProgressBar
var flow_value_label: Label
var approach_label: Label
var flow_hint_label: Label
var _flow_fill_style: StyleBoxFlat
var _pressure_fill_style: StyleBoxFlat
var _flow_pulse_time: float = 0.0  # S226: threat-pressure emphasis clock
var location_card: PanelContainer
var location_art: TextureRect
var location_title: Label
var location_subtitle: Label
var update_timer: Timer
var hp_tween: Tween
var hp_ghost_tween: Tween  # S57
var _last_hp: int = -1
var _last_grains: int = -1  # S57: track grains for popup
var _last_burned: int = -1  # S57: track burned count for glow
var _memory_glow_tween: Tween  # S57
var _slide_in_done: bool = false  # S57
var _last_location_key: String = ""
var _location_card_tween: Tween
var _hud_plate_base_pos: Vector2 = Vector2.ZERO
var _hud_plate_target_alpha: float = 0.58
var _hud_uses_full_overlay: bool = false

func _ready() -> void:
	layer = 10
	_build_ui()
	_start_timer()
	_connect_signals()
	_update_hud()
	print("[ExplorationHUD] Ready")

func _process(delta: float) -> void:
	_flow_pulse_time += delta
	if flow_panel and flow_panel.visible:
		_update_field_flow_hud()

# ── UI 구성 ──
func _build_ui() -> void:
	var clean_view := OptionsMenu != null and OptionsMenu.is_clean_gameplay_visuals()
	if not clean_view and ResourceLoader.exists(HUD_ARCHIVE_OVERLAY_PATH):
		_hud_uses_full_overlay = true
		hud_plate_art = TextureRect.new()
		hud_plate_art.texture = load(HUD_ARCHIVE_OVERLAY_PATH)
		hud_plate_art.set_anchors_preset(Control.PRESET_FULL_RECT)
		hud_plate_art.position = Vector2(-53, -20)
		hud_plate_art.scale = Vector2(1.0, 0.65)
		hud_plate_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		hud_plate_art.stretch_mode = TextureRect.STRETCH_SCALE
		hud_plate_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_hud_plate_target_alpha = 0.82
		hud_plate_art.modulate = Color(1.0, 0.96, 0.90, _hud_plate_target_alpha)
		_hud_plate_base_pos = hud_plate_art.position
		add_child(hud_plate_art)
	elif not clean_view and ResourceLoader.exists(HUD_PLATE_PATH):
		_hud_uses_full_overlay = false
		hud_plate_art = TextureRect.new()
		hud_plate_art.texture = load(HUD_PLATE_PATH)
		hud_plate_art.position = Vector2(-2, -4)
		hud_plate_art.size = Vector2(292, 190)
		hud_plate_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		hud_plate_art.stretch_mode = TextureRect.STRETCH_SCALE
		hud_plate_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
		hud_plate_art.modulate = Color(1.0, 0.92, 0.78, 0.58)
		_hud_plate_base_pos = hud_plate_art.position
		add_child(hud_plate_art)

	panel = PanelContainer.new()
	panel.position = Vector2(12, 12)
	panel.custom_minimum_size.x = 268
	panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.026, 0.022, 0.036, 0.90), # readable dark backing
		Color(0.78, 0.62, 0.38, 0.68),    # clear amber border
		1,                                # thin border
		4,                                # corner radius
		8                                 # content margin
	))
	add_child(panel)
	_build_location_card()
	_build_controls_strip()
	_build_field_flow_panel()

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 5)
	panel.add_child(vbox)

	# Keep identity and chapter context together so the persistent HUD answers
	# the immediate "who / where in the story" question before resources.
	var header_row := HBoxContainer.new()
	header_row.add_theme_constant_override("separation", 8)
	vbox.add_child(header_row)

	identity_label = Label.new()
	identity_label.text = "ARREL · JOURNEY"
	identity_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	identity_label.add_theme_font_size_override("font_size", 14)
	identity_label.add_theme_color_override("font_color", Color(0.98, 0.88, 0.68))
	header_row.add_child(identity_label)

	chapter_label = Label.new()
	chapter_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	chapter_label.add_theme_font_size_override("font_size", 13)
	chapter_label.add_theme_color_override("font_color", Color(0.78, 0.84, 0.98))
	header_row.add_child(chapter_label)

	var header_rule := HSeparator.new()
	header_rule.add_theme_constant_override("separation", 2)
	header_rule.add_theme_color_override("color", Color(0.69, 0.55, 0.32, 0.46))
	vbox.add_child(header_rule)

	# ── HP Row ──
	var hp_row := HBoxContainer.new()
	hp_row.add_theme_constant_override("separation", 6)
	vbox.add_child(hp_row)

	hp_label = Label.new()
	hp_label.text = "HP"
	hp_label.add_theme_font_size_override("font_size", 13)
	hp_label.add_theme_color_override("font_color", Color(0.82, 0.88, 1.0))
	hp_row.add_child(hp_label)

	# S57: Stacked HP bars, ghost underneath, real on top
	var hp_stack := Control.new()
	hp_stack.custom_minimum_size = Vector2(118, 12)
	hp_row.add_child(hp_stack)

	# Ghost bar (lighter, trails behind)
	hp_ghost_bar = ProgressBar.new()
	hp_ghost_bar.custom_minimum_size = Vector2(118, 12)
	hp_ghost_bar.position = Vector2.ZERO
	hp_ghost_bar.size = Vector2(118, 12)
	hp_ghost_bar.max_value = 100
	hp_ghost_bar.value = 100
	hp_ghost_bar.show_percentage = false
	var ghost_fill := StyleBoxFlat.new()
	ghost_fill.bg_color = Color(0.85, 0.35, 0.3, 0.6)  # lighter red/orange ghost
	ghost_fill.set_corner_radius_all(2)
	hp_ghost_bar.add_theme_stylebox_override("fill", ghost_fill)
	var ghost_bg := StyleBoxFlat.new()
	ghost_bg.bg_color = Color(0.08, 0.07, 0.1, 0.9)
	ghost_bg.set_corner_radius_all(2)
	hp_ghost_bar.add_theme_stylebox_override("background", ghost_bg)
	hp_stack.add_child(hp_ghost_bar)

	# Real HP bar (on top)
	hp_bar = ProgressBar.new()
	hp_bar.custom_minimum_size = Vector2(118, 12)
	hp_bar.position = Vector2.ZERO
	hp_bar.size = Vector2(118, 12)
	hp_bar.max_value = 100
	hp_bar.value = 100
	hp_bar.show_percentage = false
	var fill_style := StyleBoxFlat.new()
	fill_style.bg_color = UITheme.HP_PLAYER
	fill_style.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("fill", fill_style)
	# Transparent background so ghost shows through
	var real_bg := StyleBoxFlat.new()
	real_bg.bg_color = Color(0, 0, 0, 0)
	real_bg.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("background", real_bg)
	hp_stack.add_child(hp_bar)

	hp_value_label = Label.new()
	hp_value_label.add_theme_font_size_override("font_size", 13)
	hp_value_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	hp_row.add_child(hp_value_label)

	# S57: Status effect icons row (next to HP row)
	status_icons_row = HBoxContainer.new()
	status_icons_row.add_theme_constant_override("separation", 2)
	hp_row.add_child(status_icons_row)

	# ── Memory Row ──
	memory_label = Label.new()
	memory_label.add_theme_font_size_override("font_size", 14)
	memory_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	vbox.add_child(memory_label)

	# ── Grains Row ──
	grains_label = Label.new()
	grains_label.add_theme_font_size_override("font_size", 14)
	grains_label.add_theme_color_override("font_color", Color(0.96, 0.80, 0.46))
	vbox.add_child(grains_label)

	# ── Items Row ──
	items_label = Label.new()
	items_label.add_theme_font_size_override("font_size", 14)
	items_label.add_theme_color_override("font_color", Color(0.72, 0.90, 0.72))
	vbox.add_child(items_label)

	# S92: Memory Pulse status
	pulse_label = Label.new()
	pulse_label.add_theme_font_size_override("font_size", 13)
	pulse_label.add_theme_color_override("font_color", Color(0.88, 0.80, 0.60))
	vbox.add_child(pulse_label)

	# ── S41: Equipment Row ──
	equip_label = Label.new()
	equip_label.add_theme_font_size_override("font_size", 13)
	equip_label.add_theme_color_override("font_color", Color(0.82, 0.74, 0.98))
	vbox.add_child(equip_label)

	# ── Story objective card ──
	quest_card = PanelContainer.new()
	quest_card.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.060, 0.045, 0.070, 0.92),
		Color(0.78, 0.60, 0.34, 0.72),
		1, 5, 7
	))
	vbox.add_child(quest_card)
	var quest_box := VBoxContainer.new()
	quest_box.add_theme_constant_override("separation", 2)
	quest_card.add_child(quest_box)

	quest_tag_label = Label.new()
	quest_tag_label.text = "◆  이야기 흐름" if GameManager.current_locale == "ko" else "◆  STORY THREAD"
	quest_tag_label.add_theme_font_override("font", UITheme.make_ui_font())
	quest_tag_label.add_theme_font_size_override("font_size", 13)
	quest_tag_label.add_theme_color_override("font_color", Color(0.96, 0.78, 0.44))
	quest_box.add_child(quest_tag_label)

	quest_label = Label.new()
	quest_label.add_theme_font_size_override("font_size", 14)
	quest_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	quest_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	quest_label.custom_minimum_size.x = 180
	quest_box.add_child(quest_label)

	# S57: Quest progress bar
	quest_progress_bar = ProgressBar.new()
	quest_progress_bar.custom_minimum_size = Vector2(160, 6)
	quest_progress_bar.max_value = 1
	quest_progress_bar.value = 0
	quest_progress_bar.show_percentage = false
	quest_progress_bar.visible = false
	var quest_fill := StyleBoxFlat.new()
	quest_fill.bg_color = Color(0.7, 0.6, 0.35, 0.9)
	quest_fill.set_corner_radius_all(2)
	quest_progress_bar.add_theme_stylebox_override("fill", quest_fill)
	var quest_bg_style := StyleBoxFlat.new()
	quest_bg_style.bg_color = Color(0.12, 0.1, 0.08, 0.7)
	quest_bg_style.set_corner_radius_all(2)
	quest_progress_bar.add_theme_stylebox_override("background", quest_bg_style)

	# S216: 사이드 퀘스트 줄.
	# 예전에는 사이드 퀘스트가 이야기 목표를 "대체"해 버려서, 퀘스트를 하나 수락하면
	# 메인 목표가 화면에서 사라졌다. 게다가 태그는 계속 "이야기 흐름"이라 잘못 읽혔다.
	# 이제 이야기 목표는 항상 남고, 진행 중인 의뢰가 그 아래에 따로 붙는다.
	quest_side_label = Label.new()
	quest_side_label.add_theme_font_size_override("font_size", 13)
	quest_side_label.add_theme_color_override("font_color", Color(0.72, 0.82, 0.94))
	quest_side_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	quest_side_label.custom_minimum_size.x = 180
	quest_side_label.visible = false
	quest_box.add_child(quest_side_label)
	quest_box.add_child(quest_progress_bar)

	for label in [identity_label, hp_label, hp_value_label, chapter_label, memory_label, grains_label, items_label, pulse_label, equip_label, quest_label]:
		UITheme.apply_ui_font(label)
	if clean_view:
		panel.custom_minimum_size.x = 236
		items_label.visible = false
		equip_label.visible = false
		pulse_label.visible = false

func _build_controls_strip() -> void:
	controls_panel = PanelContainer.new()
	controls_panel.anchor_left = 1.0
	controls_panel.anchor_right = 1.0
	controls_panel.anchor_top = 1.0
	controls_panel.anchor_bottom = 1.0
	controls_panel.offset_left = -548
	controls_panel.offset_right = -18
	controls_panel.offset_top = -58
	controls_panel.offset_bottom = -16
	controls_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	controls_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.018, 0.016, 0.024, 0.94),
		Color(0.62, 0.54, 0.40, 0.62),
		1,
		5,
		8
	))
	add_child(controls_panel)

	controls_label = Label.new()
	controls_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	controls_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	controls_label.add_theme_font_size_override("font_size", UITheme.SIZE_LABEL)
	controls_label.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	UITheme.apply_ui_font(controls_label)
	controls_panel.add_child(controls_label)
	_update_controls_hint()

func _build_field_flow_panel() -> void:
	flow_panel = PanelContainer.new()
	flow_panel.name = "FieldFlowPanel"
	flow_panel.anchor_left = 0.5
	flow_panel.anchor_right = 0.5
	flow_panel.anchor_top = 1.0
	flow_panel.anchor_bottom = 1.0
	flow_panel.offset_left = -250
	flow_panel.offset_right = 250
	flow_panel.offset_top = -140
	flow_panel.offset_bottom = -62
	flow_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flow_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.016, 0.023, 0.038, 0.96),
		Color(0.46, 0.72, 0.94, 0.78),
		1,
		7,
		8
	))
	add_child(flow_panel)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 3)
	flow_panel.add_child(vbox)
	var header := HBoxContainer.new()
	vbox.add_child(header)
	var title := Label.new()
	title.text = "필드 흐름" if GameManager.current_locale == "ko" else "FIELD FLOW"
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_font_size_override("font_size", 13)
	title.add_theme_color_override("font_color", Color(0.72, 0.88, 1.0))
	UITheme.apply_ui_font(title)
	header.add_child(title)
	approach_label = Label.new()
	approach_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	approach_label.add_theme_font_size_override("font_size", UITheme.SIZE_LABEL)
	approach_label.add_theme_color_override("font_color", Color(0.84, 0.90, 1.0))
	UITheme.apply_ui_font(approach_label)
	header.add_child(approach_label)

	var flow_row := HBoxContainer.new()
	flow_row.add_theme_constant_override("separation", 7)
	vbox.add_child(flow_row)
	flow_bar = ProgressBar.new()
	flow_bar.name = "FlowBar"
	flow_bar.custom_minimum_size = Vector2(392, 8)
	flow_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	flow_bar.max_value = FieldFlow.FLOW_MAX
	flow_bar.show_percentage = false
	_flow_fill_style = StyleBoxFlat.new()
	_flow_fill_style.bg_color = Color(0.40, 0.76, 1.0, 0.94)
	_flow_fill_style.set_corner_radius_all(3)
	flow_bar.add_theme_stylebox_override("fill", _flow_fill_style)
	var flow_bg := StyleBoxFlat.new()
	flow_bg.bg_color = Color(0.04, 0.07, 0.12, 0.90)
	flow_bg.set_corner_radius_all(3)
	flow_bar.add_theme_stylebox_override("background", flow_bg)
	flow_row.add_child(flow_bar)
	flow_value_label = Label.new()
	flow_value_label.custom_minimum_size.x = 76
	flow_value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	flow_value_label.add_theme_font_size_override("font_size", 13)
	flow_value_label.add_theme_color_override("font_color", Color(0.82, 0.88, 1.0))
	UITheme.apply_ui_font(flow_value_label)
	flow_row.add_child(flow_value_label)

	pressure_bar = ProgressBar.new()
	pressure_bar.name = "ThreatPressureBar"
	pressure_bar.custom_minimum_size = Vector2(0, 3)
	pressure_bar.max_value = 1.0
	pressure_bar.show_percentage = false
	_pressure_fill_style = StyleBoxFlat.new()
	_pressure_fill_style.bg_color = Color(0.94, 0.32, 0.25, 0.88)
	_pressure_fill_style.set_corner_radius_all(2)
	pressure_bar.add_theme_stylebox_override("fill", _pressure_fill_style)
	var pressure_bg := StyleBoxFlat.new()
	pressure_bg.bg_color = Color(0.15, 0.04, 0.05, 0.46)
	pressure_bg.set_corner_radius_all(2)
	pressure_bar.add_theme_stylebox_override("background", pressure_bg)
	vbox.add_child(pressure_bar)

	flow_hint_label = Label.new()
	flow_hint_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	flow_hint_label.add_theme_font_size_override("font_size", 13)
	flow_hint_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	UITheme.apply_ui_font(flow_hint_label)
	vbox.add_child(flow_hint_label)

func _update_controls_hint(_mode = null) -> void:
	if controls_label == null or InputManager == null:
		return
	var is_ko := GameManager.current_locale == "ko"
	var interact_text := "상호작용" if is_ko else "Interact"
	var pulse_text := "기억 파동" if is_ko else "Memory Pulse"
	var dash_text := "위상 이동" if is_ko else "Phase Step"
	var archive_text := "기억 서고" if is_ko else "Archive"
	var menu_text := "메뉴" if is_ko else "Menu"
	controls_label.text = "  ".join(PackedStringArray([
		InputManager.get_hint("interact", interact_text),
		InputManager.get_hint("field_dash", dash_text),
		InputManager.get_hint("memory_pulse", pulse_text),
		InputManager.get_hint("memory_menu", archive_text),
		InputManager.get_hint("menu", menu_text),
	]))

func _build_location_card() -> void:
	location_card = PanelContainer.new()
	location_card.anchor_left = 1.0
	location_card.anchor_right = 1.0
	location_card.anchor_top = 0.0
	location_card.anchor_bottom = 0.0
	location_card.offset_left = -334
	location_card.offset_right = -18
	location_card.offset_top = 16
	location_card.offset_bottom = 138
	location_card.mouse_filter = Control.MOUSE_FILTER_IGNORE
	location_card.modulate.a = 0.0
	location_card.visible = false
	location_card.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.018, 0.016, 0.024, 0.92),
		Color(0.66, 0.52, 0.31, 0.62),
		1,
		5,
		8
	))
	add_child(location_card)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 10)
	location_card.add_child(hbox)

	location_art = TextureRect.new()
	location_art.custom_minimum_size = Vector2(116, 86)
	location_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	location_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	location_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hbox.add_child(location_art)

	var text_box := VBoxContainer.new()
	text_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_box.alignment = BoxContainer.ALIGNMENT_CENTER
	text_box.add_theme_constant_override("separation", 5)
	hbox.add_child(text_box)

	location_title = Label.new()
	UITheme.apply_title_font(location_title)
	location_title.add_theme_font_size_override("font_size", 17)
	location_title.add_theme_color_override("font_color", Color(0.98, 0.88, 0.68))
	location_title.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.65))
	location_title.add_theme_constant_override("shadow_offset_x", 1)
	location_title.add_theme_constant_override("shadow_offset_y", 1)
	location_title.autowrap_mode = TextServer.AUTOWRAP_WORD
	text_box.add_child(location_title)

	location_subtitle = Label.new()
	UITheme.apply_ui_font(location_subtitle)
	location_subtitle.add_theme_font_size_override("font_size", 13)
	location_subtitle.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	location_subtitle.autowrap_mode = TextServer.AUTOWRAP_WORD
	location_subtitle.text = "지역 이미지 참고" if GameManager.current_locale == "ko" else "Area image reference"
	text_box.add_child(location_subtitle)

func _start_timer() -> void:
	update_timer = Timer.new()
	update_timer.wait_time = 0.5
	update_timer.autostart = true
	update_timer.timeout.connect(_update_hud)
	add_child(update_timer)

func _connect_signals() -> void:
	GameManager.state_changed.connect(_on_state_changed)
	if InputManager and not InputManager.input_mode_changed.is_connected(_update_controls_hint):
		InputManager.input_mode_changed.connect(_update_controls_hint)
	# S57: Listen for memory burned to trigger glow
	if MemoryManager.has_signal("memory_burned"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	# S58: Stat gain popup
	GameManager.stat_gained.connect(_on_stat_gained)
	# Set initial visibility based on current state
	_on_state_changed(GameManager.current_state)

# ── 상태 변경 시 표시/숨김 ──
func _on_state_changed(new_state: GameManager.GameState) -> void:
	var should_show = (new_state == GameManager.GameState.EXPLORATION)
	panel.visible = should_show
	if controls_panel:
		controls_panel.visible = should_show and not OptionsMenu.is_clean_gameplay_visuals()
	if flow_panel:
		flow_panel.visible = should_show
	_update_decorative_visibility()
	# S57: Slide-in animation when entering exploration
	if should_show and not _slide_in_done:
		_play_slide_in()
		_slide_in_done = true
	elif not should_show:
		_slide_in_done = false
		if location_card:
			location_card.visible = false

# ── S57: Slide-in animation ──
func _play_slide_in() -> void:
	# Temporarily make all children invisible, then animate them in
	var children_to_animate = []
	var vbox = panel.get_child(0) as VBoxContainer
	if not vbox:
		return
	# Save original positions and animate
	panel.modulate.a = 0.0
	if hud_plate_art:
		hud_plate_art.modulate.a = 0.0
		hud_plate_art.position.x = _hud_plate_base_pos.x - 80
	var t = create_tween()
	t.tween_property(panel, "modulate:a", 1.0, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	# HP slides from left
	var orig_pos = panel.position
	panel.position.x = orig_pos.x - 80
	t.parallel().tween_property(panel, "position:x", orig_pos.x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	if hud_plate_art:
		t.parallel().tween_property(hud_plate_art, "modulate:a", _hud_plate_target_alpha, 0.3).set_ease(Tween.EASE_OUT)
		t.parallel().tween_property(hud_plate_art, "position:x", _hud_plate_base_pos.x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

# ── S57: Memory burn glow effect ──
func _on_memory_burned(_memory) -> void:
	if _memory_glow_tween and _memory_glow_tween.is_valid():
		_memory_glow_tween.kill()
	# Flash memory label gold
	memory_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
	_memory_glow_tween = create_tween()
	_memory_glow_tween.tween_property(memory_label, "theme_override_colors/font_color", UITheme.TEXT_DIM, 0.5).set_ease(Tween.EASE_OUT)

# ── S57: Grains earned popup ──
func _show_grains_popup(amount: int) -> void:
	if amount <= 0:
		return
	var popup = Label.new()
	popup.text = "+%d Grains" % amount
	UITheme.apply_ui_font(popup)
	popup.add_theme_font_size_override("font_size", 14)
	popup.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4))
	popup.add_theme_color_override("font_outline_color", Color(0.15, 0.1, 0.05))
	popup.add_theme_constant_override("outline_size", 2)
	# Position near grains label
	popup.position = grains_label.global_position + Vector2(grains_label.size.x + 8, -4)
	popup.z_index = 100
	add_child(popup)

	var t = create_tween()
	t.tween_property(popup, "position:y", popup.position.y - 30, 0.8).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.parallel().tween_property(popup, "modulate:a", 0.0, 0.8).set_ease(Tween.EASE_IN).set_delay(0.3)
	t.tween_callback(popup.queue_free)

# ── S58: Stat gain floating popup ──
func _on_stat_gained(stat_name: String, amount: int) -> void:
	if amount == 0:
		return
	var prefix = "+" if amount > 0 else ""
	var popup = Label.new()
	popup.text = "%s%d %s" % [prefix, amount, stat_name]
	UITheme.apply_ui_font(popup)
	popup.add_theme_font_size_override("font_size", 16)
	var col = Color(0.3, 1.0, 0.5) if amount > 0 else Color(1.0, 0.4, 0.3)
	popup.add_theme_color_override("font_color", col)
	popup.add_theme_color_override("font_outline_color", Color(0.05, 0.05, 0.05))
	popup.add_theme_constant_override("outline_size", 3)
	# Position near the HUD panel, offset right
	popup.position = Vector2(panel.position.x + panel.size.x + 16, panel.position.y + 8)
	popup.z_index = 120
	popup.modulate.a = 0.0
	add_child(popup)
	# Animate: fade in, float up, fade out
	var t = create_tween()
	t.tween_property(popup, "modulate:a", 1.0, 0.15)
	t.tween_property(popup, "position:y", popup.position.y - 40, 1.0).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.parallel().tween_property(popup, "modulate:a", 0.0, 0.5).set_ease(Tween.EASE_IN).set_delay(0.6)
	t.tween_callback(popup.queue_free)
	# Play a subtle SFX
	AudioManager.play_sfx("ui_select")

# ── S57: Update status effect icons ──
func _update_status_icons() -> void:
	# Clear existing icons
	for child in status_icons_row.get_children():
		child.queue_free()

	# Only show if in battle-related context or status persists
	# Check BattleManager player_statuses (may be empty outside battle)
	if not BattleManager or BattleManager.player_statuses.is_empty():
		return

	var status_colors := {
		"poison": Color(0.3, 0.8, 0.2),
		"burn": Color(0.9, 0.4, 0.1),
		"weaken": Color(0.6, 0.3, 0.7),
		"stun": Color(0.9, 0.9, 0.3),
	}

	for entry in BattleManager.player_statuses:
		var status_type: String = ""
		if entry is Dictionary:
			status_type = entry.get("type", "")
		elif "type" in entry:
			status_type = entry.type

		if status_type == "":
			continue

		var icon = ColorRect.new()
		icon.custom_minimum_size = Vector2(8, 8)
		icon.size = Vector2(8, 8)
		icon.color = status_colors.get(status_type, Color(0.5, 0.5, 0.5))
		status_icons_row.add_child(icon)

		# Pulse animation
		var pulse_tween = create_tween().set_loops()
		pulse_tween.tween_property(icon, "modulate:a", 0.4, 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
		pulse_tween.tween_property(icon, "modulate:a", 1.0, 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)

func _update_field_flow_hud() -> void:
	if flow_panel == null:
		return
	var player := get_tree().get_first_node_in_group("player")
	if player == null or not player.has_method("get_field_flow_status"):
		flow_panel.modulate.a = FLOW_PANEL_MIN_ALPHA
		flow_bar.value = 0.0
		pressure_bar.value = 0.0
		approach_label.text = "필드 연결 없음" if GameManager.current_locale == "ko" else "NO FIELD LINK"
		flow_value_label.text = "—"
		return
	flow_panel.modulate.a = 1.0
	var status: Dictionary = player.call("get_field_flow_status")
	var flow := float(status.get("flow", 0.0))
	var maximum := float(status.get("maximum", FieldFlow.FLOW_MAX))
	var pressure := float(status.get("pressure", 0.0))
	var mode := String(status.get("mode", "neutral"))
	var dash_ready := bool(status.get("dash_ready", false))
	flow_bar.max_value = maximum
	flow_bar.value = flow
	pressure_bar.value = pressure
	flow_value_label.text = "%03d / %03d" % [int(round(flow)), int(round(maximum))]

	var is_ko := GameManager.current_locale == "ko"
	var mode_copy := {
		"neutral": "이동해 흐름 축적" if is_ko else "MOVE TO BUILD",
		"ambush_ready": "기습 준비" if is_ko else "AMBUSH READY",
		"ambush": "위상 기습" if is_ko else "PHASE AMBUSH",
		"guarded": "대비 진입" if is_ko else "GUARDED ENTRY",
		"witness": "증언 진입" if is_ko else "WITNESS ENTRY",
	}
	var accent := Color(0.40, 0.76, 1.0)
	match mode:
		"ambush_ready", "ambush":
			accent = Color(1.0, 0.62, 0.28)
		"guarded":
			accent = Color(0.46, 0.88, 0.95)
		"witness":
			accent = Color(0.78, 0.62, 1.0)
	approach_label.text = String(mode_copy.get(mode, mode.to_upper()))
	approach_label.add_theme_color_override("font_color", accent)
	_flow_fill_style.bg_color = Color(accent, 0.94)
	# S226: Quiet when nothing is hunting, loud the moment something is.
	# Panel presence, not new UI, carries the pressure.
	var alarm := maxf(pressure, 0.85 if mode in ["ambush_ready", "guarded", "witness"] else 0.0)
	var pulse := 0.0
	if alarm >= 0.45:
		pulse = (sin(_flow_pulse_time * (5.0 + alarm * 3.5)) * 0.5 + 0.5) * (alarm - 0.45) * 0.5
	# S230: 예전에는 조용할 때 패널 전체를 알파 0.62까지 내렸다. 패널 배경(0.96)까지
	# 같이 흐려져 실효 불투명도가 0.60이 되고, 밝은 숲 캔버스 위에서는 글자 사이로
	# 지형이 그대로 비쳤다. "조용함"은 테두리 색과 맥동으로 말하고, 읽을 바탕은 남긴다.
	flow_panel.modulate.a = clampf(FLOW_PANEL_MIN_ALPHA + alarm * 0.12 + pulse * 0.12, 0.0, 1.0)
	var panel_style := flow_panel.get_theme_stylebox("panel") as StyleBoxFlat
	if panel_style:
		var border := accent if alarm >= 0.45 else Color(0.46, 0.72, 0.94)
		if pressure >= 0.45:
			border = Color(1.0, lerpf(0.46, 0.24, pressure), 0.22)
		panel_style.border_color = Color(border.r, border.g, border.b, clampf(0.42 + alarm * 0.52 + pulse, 0.0, 1.0))
		panel_style.set_border_width_all(2 if alarm >= 0.7 else 1)
	pressure_bar.custom_minimum_size.y = 5.0 if pressure >= 0.45 else 3.0
	var dash_icon := InputManager.get_icon("field_dash") if InputManager else "Ctrl"
	var pulse_icon := InputManager.get_icon("memory_pulse") if InputManager else "Q"
	if is_ko:
		flow_hint_label.text = "[%s] 위상 이동 %d · [%s] 위협 속 기억 증언" % [dash_icon, int(status.get("dash_cost", 42)), pulse_icon]
	else:
		flow_hint_label.text = "[%s] PHASE STEP %d · [%s] WITNESS THE THREAT" % [dash_icon, int(status.get("dash_cost", 42)), pulse_icon]
	flow_hint_label.add_theme_color_override("font_color", accent.lightened(0.08) if dash_ready or pressure > 0.0 else UITheme.TEXT_DIM)
	_pressure_fill_style.bg_color = Color(1.0, lerpf(0.44, 0.16, pressure), 0.18, 0.88)

# ── HUD 갱신 ──
func _update_hud() -> void:
	if not panel.visible:
		return
	_update_controls_hint()
	_update_field_flow_hud()
	_update_decorative_visibility()

	var pd: Dictionary = GameManager.player_data
	var hp: int = pd.get("hp", 0)
	var max_hp: int = pd.get("max_hp", 100)

	# HP bar, S57: ghost drain effect
	hp_bar.max_value = max_hp
	hp_ghost_bar.max_value = max_hp
	if hp != _last_hp:
		var prev_hp = _last_hp
		_last_hp = hp

		# Real HP drops instantly
		if hp_tween and hp_tween.is_valid():
			hp_tween.kill()
		hp_tween = create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		hp_tween.tween_property(hp_bar, "value", float(hp), 0.15)

		# Ghost bar follows slowly (drain effect)
		if hp_ghost_tween and hp_ghost_tween.is_valid():
			hp_ghost_tween.kill()
		hp_ghost_tween = create_tween().set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
		hp_ghost_tween.tween_property(hp_ghost_bar, "value", float(hp), 0.5).set_delay(0.15)

		# Update fill color based on HP threshold
		var fill: StyleBoxFlat = hp_bar.get_theme_stylebox("fill")
		if float(hp) / float(max_hp) <= 0.25:
			fill.bg_color = UITheme.HP_LOW
		else:
			fill.bg_color = UITheme.HP_PLAYER

	hp_value_label.text = "%d/%d" % [hp, max_hp]

	# Chapter context stays compact in the header; the location art card remains
	# a one-time flourish in the full presentation mode.
	var chapter_num: int = GameManager.current_chapter
	var location_name: String = _get_location_name()
	var ng_suffix = " (NG+%d)" % GameManager.ng_plus_cycle if GameManager.ng_plus_cycle > 0 else ""
	identity_label.text = "아렐 · 여정" if GameManager.current_locale == "ko" else "ARREL · JOURNEY"
	if location_name.is_empty():
		chapter_label.text = ("제%d장%s" % [chapter_num, ng_suffix]) if GameManager.current_locale == "ko" else "CH.%02d%s" % [chapter_num, ng_suffix]
	else:
		chapter_label.text = ("제%d장 · %s%s" % [chapter_num, location_name, ng_suffix]) if GameManager.current_locale == "ko" else "CH.%02d · %s%s" % [chapter_num, location_name, ng_suffix]
		_update_location_card()

	# Memories
	var held: int = MemoryManager.memories.size()
	var burned: int = MemoryManager.burned_memories.size()
	memory_label.visible = not OptionsMenu.is_clean_gameplay_visuals()
	memory_label.text = ("기억: 보유 %d, 연소 %d" % [held, burned]) if GameManager.current_locale == "ko" else "Memories: %d held, %d burned" % [held, burned]

	# S57: Memory burn counter glow (detect change via burned count)
	if _last_burned >= 0 and burned > _last_burned:
		_on_memory_burned(null)
	_last_burned = burned

	# Grains
	var grains: int = GameManager.player_data.get("grains", 0)
	grains_label.visible = not OptionsMenu.is_clean_gameplay_visuals()
	grains_label.text = ("%s: %d" % [GameManager.loc("grains"), grains]) if GameManager.current_locale == "ko" else "Grains: %d" % grains
	# S57: Grains earned popup
	if _last_grains >= 0 and grains > _last_grains:
		_show_grains_popup(grains - _last_grains)
	_last_grains = grains

	# Items
	var total_items: int = 0
	var items_dict: Dictionary = GameManager.player_data.get("items", {})
	for item_id in items_dict:
		total_items += items_dict[item_id]
	items_label.text = ("아이템: %d" % total_items) if GameManager.current_locale == "ko" else "Items: %d" % total_items

	var recovery_stock := 0
	var tool_stock := 0
	var witness_stock := 0
	for item_id_value in items_dict.keys():
		var item_id := String(item_id_value)
		var count := int(items_dict[item_id_value])
		match String(GameManager.ITEMS.get(item_id, {}).get("type", "")):
			"heal", "cure": recovery_stock += count
			"witness": witness_stock += count
			_: tool_stock += count
	items_label.visible = total_items > 0 and not OptionsMenu.is_clean_gameplay_visuals()
	var kit_copy := ("전술 키트 · 회복 %d · 도구 %d · 증언 %d" % [recovery_stock, tool_stock, witness_stock]) if GameManager.current_locale == "ko" else "TACTICAL KIT · Recovery %d · Tools %d · Witness %d" % [recovery_stock, tool_stock, witness_stock]
	if recovery_stock > 0 and int(GameManager.player_data.get("hp", 0)) < int(GameManager.player_data.get("max_hp", 0)):
		kit_copy += (" · [%s] 자동 회복" % InputManager.get_icon("quick_item")) if GameManager.current_locale == "ko" else " · [%s] Smart Heal" % InputManager.get_icon("quick_item")
	items_label.text = kit_copy
	items_label.add_theme_color_override("font_color", Color(0.74, 0.58, 1.0) if witness_stock > 0 else Color(0.55, 0.75, 0.55))

	_update_memory_pulse_status()

	# S41: Equipment summary
	var weapon_name = ""
	var wid = GameManager.equipped.get("weapon", "")
	if wid != "" and GameManager.EQUIPMENT.has(wid):
		weapon_name = GameManager.EQUIPMENT[wid].name
	equip_label.text = (("무기: %s" if GameManager.current_locale == "ko" else "Weapon: %s") % weapon_name) if weapon_name != "" else ""
	equip_label.visible = weapon_name != "" and not OptionsMenu.is_clean_gameplay_visuals()

	# S41/S57: Active quest tracker with progress bar
	_update_quest_tracker()

	# S57: Status effect icons
	_update_status_icons()

## S41/S57: 활성 퀘스트 트래커 with progress bar
func _update_quest_tracker() -> void:
	var active_quest = ""
	var quest_step: int = 0
	var quest_total: int = 1

	# S216: 진행 중인 의뢰는 이야기 목표와 나란히 보여 준다.
	# 예전에는 사이드 퀘스트를 찾으면 곧바로 break하고 이야기 목표 계산을 건너뛰어서,
	# 의뢰를 하나 수락하는 순간 메인 목표가 화면에서 사라졌다.
	var side_lines: Array[String] = []
	for q: Dictionary in SideQuest.get_all_quests():
		var qid := String(q.get("id", ""))
		if not SideQuest.is_active(qid):
			continue
		var title := SideQuest.loc(q, "title")
		var step_text := SideQuest.get_current_step_text(qid)
		var steps_array: Array = q.get("steps", [])
		var total := maxi(steps_array.size(), 1)
		var done := SideQuest.get_current_step(qid)
		if step_text != "":
			side_lines.append("· %s (%d/%d)
   %s" % [title, done, total, step_text])
		else:
			side_lines.append("· %s (%d/%d)" % [title, done, total])

	# Main-story objective, synchronized with the flags used by map exits.
	if true:
		var ch = GameManager.current_chapter
		match ch:
			1:
				if not GameManager.get_flag("ch1_elia_appeared"):
					active_quest = "숲에서 엘리아 찾기" if GameManager.current_locale == "ko" else "Find Elia in the forest"
				elif not GameManager.get_flag("ch1_void_beast_defeated"):
					active_quest = "숲을 배회하는 공허수 찾기" if GameManager.current_locale == "ko" else "Find what hunts these woods"
				elif not GameManager.get_flag("ch1_camp_done"):
					active_quest = "남쪽 야영지로 이동하기" if GameManager.current_locale == "ko" else "Reach the southern camp"
			2:
				if not GameManager.get_flag("ch2_malet_done"):
					active_quest = "시장에서 말렛 만나기" if GameManager.current_locale == "ko" else "Meet Malet at the market"
			3:
				active_quest = ("북쪽 출구로 이동하기" if GameManager.get_flag("tobias_in_party") else "토비아스와 대화하기") if GameManager.current_locale == "ko" else ("Take the north exit" if GameManager.get_flag("tobias_in_party") else "Speak with Tobias")
			4:
				active_quest = ("북쪽 출구로 이동하기" if GameManager.get_flag("ch4_anchoring") else "피난처의 기록 조사하기") if GameManager.current_locale == "ko" else ("Take the north exit" if GameManager.get_flag("ch4_anchoring") else "Investigate the shelter records")
			5:
				active_quest = "북쪽 균열에서 심에 도달하기" if GameManager.current_locale == "ko" else "Reach the Seam through the northern fracture"
			6:
				active_quest = ("남쪽의 BL-07 입구로 이동하기" if GameManager.get_flag("ch6_briefing_done") else "세이블과 대화하기") if GameManager.current_locale == "ko" else ("Enter BL-07 to the south" if GameManager.get_flag("ch6_briefing_done") else "Speak with Sable")
			7:
				active_quest = ("북쪽 출구로 이동하기" if GameManager.get_flag("ch7_trial_complete") else "세이블의 시련 완료하기") if GameManager.current_locale == "ko" else ("Take the north exit" if GameManager.get_flag("ch7_trial_complete") else "Complete Sable's trial")
			8:
				active_quest = "기억 기생림을 지나 북쪽으로 이동하기" if GameManager.current_locale == "ko" else "Follow the parasite forest north"
			9:
				active_quest = "무색 황무지의 북쪽 경계로 이동하기" if GameManager.current_locale == "ko" else "Reach the northern edge of the Waste"
			10:
				active_quest = "BL-07의 심장부에 도달하기" if GameManager.current_locale == "ko" else "Reach the heart of BL-07"
		quest_step = 0
		quest_total = 1

	var is_ko := GameManager.current_locale == "ko"
	if active_quest != "":
		quest_tag_label.text = "◆  이야기 흐름" if is_ko else "◆  STORY THREAD"
		quest_label.text = active_quest
		quest_label.visible = true
		quest_progress_bar.max_value = quest_total
		quest_progress_bar.value = quest_step
		quest_progress_bar.visible = quest_total > 1
	else:
		quest_label.visible = false
		quest_progress_bar.visible = false

	if quest_side_label != null:
		if side_lines.is_empty():
			quest_side_label.visible = false
		else:
			var header := "진행 중인 의뢰" if is_ko else "ACTIVE REQUESTS"
			quest_side_label.text = "%s
%s" % [header, "
".join(side_lines)]
			quest_side_label.visible = true
			# 의뢰만 남고 이야기 목표가 없을 때도 카드는 떠 있어야 한다.
			if active_quest == "":
				quest_tag_label.text = "◆  %s" % header

	quest_card.visible = active_quest != "" or not side_lines.is_empty()

func _update_memory_pulse_status() -> void:
	if pulse_label == null:
		return
	var player = _get_player_node()
	if player == null or not player.has_method("get_memory_pulse_status"):
		pulse_label.visible = false
		return

	var status: Dictionary = player.get_memory_pulse_status()
	var focus := int(status.get("field_focus", 0))
	var focus_max := int(status.get("field_focus_max", 3))
	var clean_view := OptionsMenu.is_clean_gameplay_visuals()
	if GameManager.current_locale == "ko":
		var focus_suffix_ko := " · 집중 %d/%d" % [focus, focus_max]
		if bool(status.get("ready", false)):
			pulse_label.text = "파동: 준비 [Q]" + focus_suffix_ko
			pulse_label.add_theme_color_override("font_color", Color(0.86, 0.76, 0.42))
		else:
			var cooldown_ko: float = float(status.get("cooldown", 0.0))
			var max_cooldown_ko: float = maxf(float(status.get("max_cooldown", 1.0)), 1.0)
			var filled_ko := int(round((1.0 - cooldown_ko / max_cooldown_ko) * 5.0))
			var meter_ko := ""
			for i in range(5):
				meter_ko += "|" if i < filled_ko else "."
			pulse_label.text = "파동: %s %.1fs" % [meter_ko, cooldown_ko] + focus_suffix_ko
			pulse_label.add_theme_color_override("font_color", Color(0.52, 0.55, 0.64))
		return
	pulse_label.visible = not clean_view or focus > 0 or not bool(status.get("ready", false))
	var focus_suffix := (" · 집중 %d/%d" if GameManager.current_locale == "ko" else " · Focus %d/%d") % [focus, focus_max]
	if bool(status.get("ready", false)):
		pulse_label.text = ("펄스: 준비 [Q]" if GameManager.current_locale == "ko" else "Pulse: Ready [Q]") + focus_suffix
		pulse_label.add_theme_color_override("font_color", Color(0.86, 0.76, 0.42))
	else:
		var cooldown: float = float(status.get("cooldown", 0.0))
		var max_cooldown: float = maxf(float(status.get("max_cooldown", 1.0)), 1.0)
		var marks := 5
		var filled := int(round((1.0 - cooldown / max_cooldown) * marks))
		var meter := ""
		for i in range(marks):
			meter += "|" if i < filled else "."
		pulse_label.text = (("펄스: %s %.1fs" if GameManager.current_locale == "ko" else "Pulse: %s %.1fs") % [meter, cooldown]) + focus_suffix
		pulse_label.add_theme_color_override("font_color", Color(0.52, 0.55, 0.64))

func _get_player_node() -> Node:
	var tree = get_tree()
	if tree == null:
		return null
	var players = tree.get_nodes_in_group("player")
	if players.is_empty():
		return null
	return players[0]

func _update_decorative_visibility() -> void:
	if hud_plate_art == null:
		return
	var suppress_overlay := OptionsMenu != null and OptionsMenu.is_clean_gameplay_visuals()
	hud_plate_art.visible = panel != null and panel.visible and not suppress_overlay

func _update_location_card() -> void:
	if OptionsMenu != null and OptionsMenu.is_clean_gameplay_visuals():
		if location_card:
			location_card.visible = false
		return
	var key := _get_location_key()
	if key == "" or key == _last_location_key:
		return
	_last_location_key = key
	var art_path: String = MAP_ART.get(key, "")
	if art_path == "" or not ResourceLoader.exists(art_path):
		return

	location_art.texture = load(art_path)
	location_title.text = _get_display_map_name(key)
	location_subtitle.text = ("%d장 / 기록된 지역" % GameManager.current_chapter) if GameManager.current_locale == "ko" else "Ch.%d / %s" % [GameManager.current_chapter, "illustrated region"]

	if GameManager.current_locale == "ko":
		location_subtitle.text = "제%d장 / 기록된 지역" % GameManager.current_chapter

	if _location_card_tween and _location_card_tween.is_valid():
		_location_card_tween.kill()
	location_card.visible = true
	location_card.modulate.a = 0.0
	location_card.position.x = 36
	_location_card_tween = create_tween()
	_location_card_tween.set_parallel(true)
	_location_card_tween.tween_property(location_card, "modulate:a", 1.0, 0.28).set_trans(Tween.TRANS_SINE)
	_location_card_tween.tween_property(location_card, "position:x", 0.0, 0.32).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	_location_card_tween.set_parallel(false)
	_location_card_tween.tween_interval(3.2)
	_location_card_tween.tween_property(location_card, "modulate:a", 0.0, 0.55).set_trans(Tween.TRANS_SINE)
	_location_card_tween.tween_callback(func(): location_card.visible = false)

## 현재 맵 이름 가져오기
func _get_location_name() -> String:
	var key := _get_location_key()
	if key != "":
		return _get_display_map_name(key)
	return ""

func _get_display_map_name(key: String) -> String:
	if GameManager.current_locale == "ko":
		return MAP_NAMES_KO_CLEAN.get(key, MAP_NAMES.get(key, key.capitalize()))
	return MAP_NAMES.get(key, "")

func _get_location_key() -> String:
	var scene := get_tree().current_scene
	if scene == null:
		return ""
	var scene_name: String = scene.name.to_lower()
	# Try direct match first
	if MAP_NAMES.has(scene_name):
		return scene_name
	# Try matching against scene file path
	if scene.scene_file_path:
		var file_name: String = scene.scene_file_path.get_file().get_basename().to_lower()
		if MAP_NAMES.has(file_name):
			return file_name
	return ""
