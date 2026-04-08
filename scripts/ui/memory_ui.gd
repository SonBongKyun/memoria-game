## MemoryUI (Autoload) — 아렐의 서고
## 보유 기억을 서고(書庫) 모티프로 표시하는 인벤토리 화면.
## Tab/M 키로 토글.
extends CanvasLayer

var is_open: bool = false

# UI 노드
var overlay: ColorRect
var main_panel: PanelContainer
var grade_tabs: VBoxContainer      # 좌측 등급 선반
var card_list: VBoxContainer       # 중앙 기억 카드 목록
var detail_panel: PanelContainer   # 우측 상세 정보
var detail_title: Label
var detail_grade: Label
var detail_desc: RichTextLabel
var detail_power: Label
var detail_npc: Label
var detail_effect: Label
var detail_status: Label
var count_label: Label             # 하단 연소 수
var close_hint: Label

var selected_grade_filter: int = -1  # -1 = 전체
var selected_memory = null           # 현재 선택된 기억
var synthesis_mode: bool = false     # 합성 모드 활성 여부
var synthesis_first = null           # 합성 첫 번째 기억
var synth_btn: Button                # 합성 버튼
var synth_status_label: Label        # 합성 모드 상태 표시

const GRADE_NAMES = ["Grade 5 — Sensory", "Grade 4 — Daily", "Grade 3 — Relational", "Grade 2 — Identity", "Grade 1 — Core"]
const GRADE_COLORS = [
	Color(0.5, 0.5, 0.45),     # Grade 5 — 회색
	Color(0.55, 0.5, 0.35),    # Grade 4 — 갈색
	Color(0.4, 0.5, 0.6),      # Grade 3 — 청색
	Color(0.6, 0.45, 0.55),    # Grade 2 — 보라
	Color(0.7, 0.55, 0.3),     # Grade 1 — 금색
]

func _ready() -> void:
	layer = 40
	_build_ui()
	_hide_ui()
	print("[MemoryUI] Ready — Arrel's Archive")

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("memory_menu"):
		if is_open:
			close_archive()
		elif GameManager.current_state == GameManager.GameState.EXPLORATION:
			open_archive()
		get_viewport().set_input_as_handled()

	if is_open and event.is_action_pressed("cancel"):
		close_archive()
		get_viewport().set_input_as_handled()

func open_archive() -> void:
	is_open = true
	GameManager.change_state(GameManager.GameState.MENU)
	_refresh_cards()
	_show_ui()

func close_archive() -> void:
	is_open = false
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	_hide_ui()

## UI 전체 구축
func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# 반투명 오버레이
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.02, 0.02, 0.04, 0.85)
	root.add_child(overlay)

	# 메인 패널 (중앙 서고)
	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.05
	main_panel.anchor_right = 0.95
	main_panel.anchor_top = 0.05
	main_panel.anchor_bottom = 0.92

	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.1, 0.08, 0.06, 0.95)  # 어두운 나무색
	panel_style.border_color = Color(0.3, 0.22, 0.15, 0.7)
	panel_style.set_border_width_all(2)
	panel_style.set_corner_radius_all(6)
	panel_style.set_content_margin_all(16)
	main_panel.add_theme_stylebox_override("panel", panel_style)
	root.add_child(main_panel)

	# 메인 VBox (타이틀 + 내용 + 하단)
	var main_vbox = VBoxContainer.new()
	main_vbox.add_theme_constant_override("separation", 12)
	main_panel.add_child(main_vbox)

	# 타이틀 바
	var title_bar = _create_title_bar()
	main_vbox.add_child(title_bar)

	# 내용 영역 (HBox: 등급탭 | 카드목록 | 상세)
	var content_hbox = HBoxContainer.new()
	content_hbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content_hbox.add_theme_constant_override("separation", 12)
	main_vbox.add_child(content_hbox)

	# 좌측: 등급 필터 탭
	_build_grade_tabs(content_hbox)

	# 중앙: 기억 카드 스크롤 목록
	_build_card_list(content_hbox)

	# 우측: 상세 정보
	_build_detail_panel(content_hbox)

	# 하단: 연소 수 + 닫기 힌트
	var bottom_bar = HBoxContainer.new()
	main_vbox.add_child(bottom_bar)

	count_label = Label.new()
	count_label.add_theme_font_size_override("font_size", 12)
	count_label.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	count_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bottom_bar.add_child(count_label)

	synth_status_label = Label.new()
	synth_status_label.add_theme_font_size_override("font_size", 12)
	synth_status_label.add_theme_color_override("font_color", Color(0.6, 0.4, 0.7))
	synth_status_label.visible = false
	bottom_bar.add_child(synth_status_label)

	close_hint = Label.new()
	close_hint.text = "[Tab / M] Close    [ESC] Close"
	close_hint.add_theme_font_size_override("font_size", 11)
	close_hint.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	bottom_bar.add_child(close_hint)

func _create_title_bar() -> HBoxContainer:
	var bar = HBoxContainer.new()

	var title = Label.new()
	title.text = "ARREL'S ARCHIVE"
	title.add_theme_font_size_override("font_size", 20)
	title.add_theme_color_override("font_color", Color(0.75, 0.6, 0.4))
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bar.add_child(title)

	var subtitle = Label.new()
	subtitle.text = "— The things you carry. The things you've lost."
	subtitle.add_theme_font_size_override("font_size", 11)
	subtitle.add_theme_color_override("font_color", Color(0.45, 0.4, 0.35))
	subtitle.vertical_alignment = VERTICAL_ALIGNMENT_BOTTOM
	bar.add_child(subtitle)

	return bar

func _build_grade_tabs(parent: HBoxContainer) -> void:
	var tab_panel = PanelContainer.new()
	tab_panel.custom_minimum_size = Vector2(180, 0)

	var tab_style = StyleBoxFlat.new()
	tab_style.bg_color = Color(0.08, 0.06, 0.05, 0.8)
	tab_style.border_color = Color(0.25, 0.2, 0.15, 0.5)
	tab_style.border_width_right = 1
	tab_style.set_content_margin_all(8)
	tab_panel.add_theme_stylebox_override("panel", tab_style)
	parent.add_child(tab_panel)

	grade_tabs = VBoxContainer.new()
	grade_tabs.add_theme_constant_override("separation", 4)
	tab_panel.add_child(grade_tabs)

	# "전체" 탭
	var all_btn = _create_tab_button("All Memories", -1)
	grade_tabs.add_child(all_btn)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", Color(0.25, 0.2, 0.15, 0.4))
	grade_tabs.add_child(sep)

	# 등급별 탭
	for i in range(5):
		var btn = _create_tab_button(GRADE_NAMES[i], i)
		grade_tabs.add_child(btn)

func _create_tab_button(text: String, grade_filter: int) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.12, 0.1, 0.08, 0.6)
	style.set_content_margin_all(6)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)

	var hover = style.duplicate()
	hover.bg_color = Color(0.18, 0.14, 0.1, 0.8)
	hover.border_color = Color(0.6, 0.45, 0.3, 0.5)
	hover.set_border_width_all(1)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	btn.add_theme_font_size_override("font_size", 12)
	btn.add_theme_color_override("font_color", Color(0.65, 0.55, 0.45))
	btn.add_theme_color_override("font_hover_color", Color(0.85, 0.7, 0.5))

	btn.pressed.connect(func(): _on_grade_filter(grade_filter))
	return btn

func _build_card_list(parent: HBoxContainer) -> void:
	var scroll = ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	parent.add_child(scroll)

	card_list = VBoxContainer.new()
	card_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	card_list.add_theme_constant_override("separation", 6)
	scroll.add_child(card_list)

func _build_detail_panel(parent: HBoxContainer) -> void:
	detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(320, 0)

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.06, 0.05, 0.8)
	style.border_color = Color(0.25, 0.2, 0.15, 0.5)
	style.border_width_left = 1
	style.set_content_margin_all(16)
	detail_panel.add_theme_stylebox_override("panel", style)
	parent.add_child(detail_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	detail_panel.add_child(vbox)

	# 상세 제목
	detail_title = Label.new()
	detail_title.add_theme_font_size_override("font_size", 18)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.7, 0.55))
	vbox.add_child(detail_title)

	# 등급
	detail_grade = Label.new()
	detail_grade.add_theme_font_size_override("font_size", 12)
	vbox.add_child(detail_grade)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", Color(0.25, 0.2, 0.15, 0.3))
	vbox.add_child(sep)

	# 설명
	detail_desc = RichTextLabel.new()
	detail_desc.bbcode_enabled = false
	detail_desc.fit_content = true
	detail_desc.scroll_active = false
	detail_desc.add_theme_font_size_override("normal_font_size", 13)
	detail_desc.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	detail_desc.custom_minimum_size = Vector2(0, 60)
	vbox.add_child(detail_desc)

	# 연소력
	detail_power = Label.new()
	detail_power.add_theme_font_size_override("font_size", 12)
	detail_power.add_theme_color_override("font_color", Color(0.7, 0.4, 0.3))
	vbox.add_child(detail_power)

	# 관련 NPC
	detail_npc = Label.new()
	detail_npc.add_theme_font_size_override("font_size", 12)
	detail_npc.add_theme_color_override("font_color", Color(0.5, 0.55, 0.6))
	vbox.add_child(detail_npc)

	# 스토리 효과
	detail_effect = Label.new()
	detail_effect.add_theme_font_size_override("font_size", 11)
	detail_effect.add_theme_color_override("font_color", Color(0.6, 0.45, 0.4))
	detail_effect.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vbox.add_child(detail_effect)

	# 상태
	detail_status = Label.new()
	detail_status.add_theme_font_size_override("font_size", 14)
	vbox.add_child(detail_status)

	# 합성 버튼
	synth_btn = Button.new()
	synth_btn.text = "SYNTHESIZE"
	synth_btn.custom_minimum_size = Vector2(0, 36)
	synth_btn.visible = false
	var synth_style = StyleBoxFlat.new()
	synth_style.bg_color = Color(0.2, 0.15, 0.3, 0.8)
	synth_style.border_color = Color(0.5, 0.35, 0.6, 0.6)
	synth_style.set_border_width_all(1)
	synth_style.set_corner_radius_all(4)
	synth_style.set_content_margin_all(6)
	synth_btn.add_theme_stylebox_override("normal", synth_style)
	var synth_hover = synth_style.duplicate()
	synth_hover.bg_color = Color(0.3, 0.2, 0.4, 0.9)
	synth_hover.border_color = Color(0.7, 0.5, 0.8, 0.8)
	synth_btn.add_theme_stylebox_override("hover", synth_hover)
	synth_btn.add_theme_stylebox_override("focus", synth_hover)
	synth_btn.add_theme_font_size_override("font_size", 13)
	synth_btn.add_theme_color_override("font_color", Color(0.7, 0.55, 0.8))
	synth_btn.add_theme_color_override("font_hover_color", Color(0.9, 0.7, 1.0))
	synth_btn.pressed.connect(_on_synth_pressed)
	vbox.add_child(synth_btn)

	_clear_detail()

## 카드 목록 새로고침
func _refresh_cards() -> void:
	# 기존 카드 제거
	for child in card_list.get_children():
		child.queue_free()

	var memories = MemoryManager.memories
	var burn_count = MemoryManager.get_burn_count()
	count_label.text = "Burned: %d / %d" % [burn_count, memories.size()]

	for memory in memories:
		# 등급 필터
		if selected_grade_filter >= 0 and memory.grade != selected_grade_filter:
			continue
		_add_memory_card(memory)

	_clear_detail()

func _add_memory_card(memory) -> void:
	var btn = Button.new()
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 40)

	# 카드 텍스트
	var grade_label = GRADE_NAMES[memory.grade].split(" — ")[0]  # "Grade 5"
	var status_mark = ""
	if memory.is_burned:
		status_mark = " [BURNED]" if not memory.is_residue else " [RESIDUE]"
	btn.text = "[%s] %s%s" % [grade_label, memory.title, status_mark]

	# 스타일
	var card_color = GRADE_COLORS[memory.grade]
	var style = StyleBoxFlat.new()
	style.bg_color = Color(card_color.r * 0.3, card_color.g * 0.3, card_color.b * 0.3, 0.6)
	style.border_color = Color(card_color.r * 0.5, card_color.g * 0.5, card_color.b * 0.5, 0.4)
	style.border_width_left = 4
	style.set_content_margin_all(8)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)

	var hover = style.duplicate()
	hover.bg_color = Color(card_color.r * 0.4, card_color.g * 0.4, card_color.b * 0.4, 0.8)
	hover.border_color = card_color
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	# 연소된 기억은 흐리게
	var font_color = Color(0.75, 0.7, 0.65)
	if memory.is_burned:
		font_color = Color(0.4, 0.35, 0.3) if not memory.is_residue else Color(0.5, 0.5, 0.55)
		btn.modulate.a = 0.7 if not memory.is_residue else 0.85

	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", font_color)
	btn.add_theme_color_override("font_hover_color", card_color.lightened(0.3))

	btn.pressed.connect(func(): _show_detail(memory))
	card_list.add_child(btn)

## 상세 정보 표시
func _show_detail(memory) -> void:
	# 합성 모드 — 두 번째 기억 선택
	if synthesis_mode and synthesis_first != null:
		if memory.id == synthesis_first.id:
			return  # 같은 기억 선택 불가
		if memory.is_burned:
			return
		if memory.grade != synthesis_first.grade:
			synth_status_label.text = "Must be same grade! (%s ≠ %s)" % [GRADE_NAMES[synthesis_first.grade].split(" — ")[0], GRADE_NAMES[memory.grade].split(" — ")[0]]
			return
		# 합성 실행
		var result = MemoryManager.synthesize(synthesis_first.id, memory.id)
		if result:
			AudioManager.play_sfx("memory_add")
		_exit_synthesis_mode()
		_refresh_cards()
		if result:
			_show_detail(result)
		return

	selected_memory = memory
	detail_title.text = memory.title
	detail_grade.text = GRADE_NAMES[memory.grade]
	detail_grade.add_theme_color_override("font_color", GRADE_COLORS[memory.grade])
	detail_desc.text = memory.description
	detail_power.text = "Burn Power: %d" % memory.burn_power
	detail_npc.text = "Related: %s" % memory.related_npc if memory.related_npc != "" else ""
	detail_npc.visible = memory.related_npc != ""

	if memory.story_effect != "":
		detail_effect.text = "If burned: %s" % memory.story_effect
		detail_effect.visible = true
	else:
		detail_effect.visible = false

	# 상태 표시
	if memory.is_burned:
		if memory.is_residue:
			detail_status.text = "RESIDUE — A faint trace remains."
			detail_status.add_theme_color_override("font_color", Color(0.5, 0.55, 0.65))
		else:
			detail_status.text = "BURNED — Gone forever."
			detail_status.add_theme_color_override("font_color", Color(0.6, 0.3, 0.25))
	else:
		detail_status.text = "INTACT"
		detail_status.add_theme_color_override("font_color", Color(0.5, 0.6, 0.45))

	# 합성 버튼 표시 (미연소 + Grade 1 미만 + 같은 등급 짝이 있을 때)
	synth_btn.visible = not memory.is_burned and memory.grade < MemoryManager.MemoryGrade.GRADE_1 and _count_same_grade(memory.grade) >= 2

func _clear_detail() -> void:
	detail_title.text = "Select a memory..."
	detail_grade.text = ""
	detail_desc.text = ""
	detail_power.text = ""
	detail_npc.text = ""
	detail_npc.visible = false
	detail_effect.text = ""
	detail_effect.visible = false
	detail_status.text = ""
	synth_btn.visible = false

func _on_grade_filter(grade: int) -> void:
	selected_grade_filter = grade
	_refresh_cards()

## ===================== 합성 시스템 =====================

func _count_same_grade(grade: int) -> int:
	var count = 0
	for m in MemoryManager.memories:
		if not m.is_burned and m.grade == grade:
			count += 1
	return count

func _on_synth_pressed() -> void:
	if selected_memory == null or selected_memory.is_burned:
		return
	synthesis_mode = true
	synthesis_first = selected_memory
	synth_status_label.text = "SYNTHESIS: Select second memory (same grade: %s)" % GRADE_NAMES[synthesis_first.grade].split(" — ")[0]
	synth_status_label.visible = true
	synth_btn.visible = false
	AudioManager.play_sfx("ui_select")

func _exit_synthesis_mode() -> void:
	synthesis_mode = false
	synthesis_first = null
	synth_status_label.visible = false

func _show_ui() -> void:
	overlay.visible = true
	main_panel.visible = true
	_exit_synthesis_mode()

func _hide_ui() -> void:
	overlay.visible = false
	main_panel.visible = false
	_exit_synthesis_mode()
