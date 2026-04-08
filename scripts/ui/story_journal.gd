## StoryJournal — 스토리 저널 / 코덱스
## PauseMenu에서 접근. 게임 진행 중 자동으로 기록되는 이벤트/NPC/선택 목록.
## ESC로 닫기.
class_name StoryJournal
extends CanvasLayer

var is_open: bool = false

# ── UI 노드 ──
var overlay: ColorRect
var main_panel: PanelContainer
var tab_events: Button
var tab_npcs: Button
var tab_choices: Button
var item_list: VBoxContainer
var item_scroll: ScrollContainer
var detail_title: Label
var detail_body: RichTextLabel
var close_hint: Label

var _current_tab: String = "events"

# ── 저널 엔트리 ──
# 자동으로 story_flags 기반으로 생성
const CHAPTER_NAMES := {1: "Rim Forest", 2: "Verdan Market", 3: "Crumbling Coast", 4: "The Seam", 5: "BL-07 Void", 6: "Epilogue"}

# 이벤트 엔트리 (flag → 표시 정보)
const EVENT_ENTRIES := [
	{"flag": "ch1_opening_done", "chapter": 1, "title": "Awakening in the Forest", "desc": "Arrel came to in the Rim Forest, blade drawn, a dead Void Beast dissolving at his feet. No memory of the fight."},
	{"flag": "ch1_elia_appeared", "chapter": 1, "title": "Elia Appears", "desc": "A woman called Elia found him — or rather, had been following him. She spoke as if she knew him. He couldn't remember."},
	{"flag": "ch1_ash_rain_seen", "chapter": 1, "title": "Ash Rain", "desc": "Gray flakes began to fall. Not snow — ash. The residue of dissolved memories, raining from the empty sky."},
	{"flag": "hidden_ch1_stump", "chapter": 1, "title": "[Hidden] The Old Stump", "desc": "A tree stump with rings too numerous to count. Something about it felt important — older than the Collapse."},
	{"flag": "ch1_camp_done", "chapter": 1, "title": "Camp at the Forest Edge", "desc": "Night. Elia hummed a melody. Arrel's hand trembled. In the morning, they headed south toward Verdan."},
	{"flag": "ch2_arrived", "chapter": 2, "title": "Verdan Market", "desc": "The Gray Belt's largest settlement. Smoke, noise, and the smell of things being traded that shouldn't be."},
	{"flag": "malet_deal_accepted", "chapter": 2, "title": "Malet's Price — Paid", "desc": "Malet extracted the memory of holding a sword for the first time. A courtyard, dust, a hand closing fingers around a grip. Gone."},
	{"flag": "malet_deal_refused", "chapter": 2, "title": "Malet's Price — Refused", "desc": "Arrel refused to sell his sword memory. The amber-eyed dealer's dismissal was absolute."},
	{"flag": "ch2_malet_done", "chapter": 2, "title": "Information Acquired", "desc": "Three things from Malet: a route through the Coast, a name (Sable), and a warning — Editor Kairos. Four days."},
	{"flag": "ch3_arrived", "chapter": 3, "title": "The Crumbling Coast", "desc": "Where land forgets how to be solid. Salt air and dissolved memory leaching into the sea."},
	{"flag": "ch3_kairos_seen", "chapter": 3, "title": "Kairos Observed", "desc": "A figure on the ridge. Still as stone. Not chasing — classifying. Elia said that was worse."},
	{"flag": "elia_separates", "chapter": 3, "title": "Elia Took the Coastal Path", "desc": "To split Kairos's attention, Elia went south alone. Without her, burned memories would leave no residue. Just absence."},
	{"flag": "elia_stays", "chapter": 3, "title": "Together Through the Coast", "desc": "They stayed together. Two signatures, easier to track — but harder to break."},
	{"flag": "ch4_arrived", "chapter": 4, "title": "The Seam", "desc": "Color between gray cliffs. Amber, crimson, green. A settlement in the cracks of what was and what will be."},
	{"flag": "elia_reunited", "chapter": 4, "title": "Reunion", "desc": "Elia stood at the Seam's entrance. The coastal path worked — Kairos went south. The anchor tightened."},
	{"flag": "hidden_ch4_garden", "chapter": 4, "title": "[Hidden] The Impossible Garden", "desc": "White petals veined with gold. Warm to the touch. A fragment of someone handing a flower — small hands, a child's laugh."},
	{"flag": "ch4_briefing_done", "chapter": 4, "title": "Sable's Briefing", "desc": "BL-07 forming south. If it opens, the Seam dies. A Shade Sentinel guards the entrance. Investigation required."},
	{"flag": "ch4_bl07_entered", "chapter": 4, "title": "The Shade Sentinel", "desc": "Dark. Wrong. The Void's immune response. Between them and BL-07, it coalesced."},
	{"flag": "ch5_complete", "chapter": 5, "title": "The Seal", "desc": "BL-07's core. A decision: burn everything to close it, or keep your name and find another way."},
	{"flag": "zero_burn_path", "chapter": 5, "title": "Zero Burn — Name Consumed", "desc": "He burned 'Arrel.' The name that meant something. The Void Hole collapsed. He didn't know who he was anymore."},
	{"flag": "seal_refused", "chapter": 5, "title": "Preservation — Name Kept", "desc": "He refused to burn his name. BL-07 remains unsolved. But he remembers who he is."},
	{"flag": "epilogue_complete", "chapter": 6, "title": "Epilogue", "desc": "The Seam. Aftermath. What remains after everything is either burned or saved."},
]

# NPC 엔트리
const NPC_ENTRIES := [
	{"flag": "ch1_elia_appeared", "name": "Elia", "role": "Anchor / Companion",
	 "desc": "Silver-haired woman who follows Arrel. Her presence slows memory decay — an 'anchoring' effect. Knows more than she says."},
	{"flag": "ch2_arrived", "name": "Malet", "role": "Memory Dealer",
	 "desc": "Small, thin, dressed in gray. Amber eyes from high-grade Memory Ampoules. Trades information for memories in the Sump beneath Verdan."},
	{"flag": "ch4_arrived", "name": "Sable", "role": "Seam Leader / Void Walker",
	 "desc": "Short dark hair, scarred. Walked into a Void Hole and walked out. Leads the Seam settlement. Pragmatic, steady."},
	{"flag": "ch3_kairos_seen", "name": "Kairos", "role": "Editor / Pursuer",
	 "desc": "An Authority Editor. Quiet. Methodical. Classifies rather than chases. His presence means the Authority knows about Arrel's burning."},
]

func _ready() -> void:
	layer = 57  # OptionsMenu(56) 위
	_build_ui()
	_hide_ui()
	print("[StoryJournal] Ready — Codex")

func _unhandled_input(event: InputEvent) -> void:
	if not is_open:
		return
	if event.is_action_pressed("cancel"):
		close_journal()
		get_viewport().set_input_as_handled()

func open_journal() -> void:
	if is_open:
		return
	is_open = true
	AudioManager.play_sfx("ui_open")
	_current_tab = "events"
	_refresh_list()
	_show_ui()

func close_journal() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	_hide_ui()

## ===================== UI 구축 =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = UITheme.BG_OVERLAY
	root.add_child(overlay)

	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.06
	main_panel.anchor_right = 0.94
	main_panel.anchor_top = 0.04
	main_panel.anchor_bottom = 0.96
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.07, 0.06, 0.05, 0.95),
		Color(0.4, 0.32, 0.22, 0.7),
		2, 6, 16
	))
	root.add_child(main_panel)

	var main_vbox = VBoxContainer.new()
	main_vbox.add_theme_constant_override("separation", 10)
	main_panel.add_child(main_vbox)

	# ── 헤더 ──
	var header = Label.new()
	header.text = "JOURNAL — Field Notes of a Memory Carrier"
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	main_vbox.add_child(header)

	# ── 탭 ──
	var tab_row = HBoxContainer.new()
	tab_row.add_theme_constant_override("separation", 8)
	main_vbox.add_child(tab_row)

	tab_events = _create_tab("Events", "events")
	tab_row.add_child(tab_events)
	tab_npcs = _create_tab("People", "npcs")
	tab_row.add_child(tab_npcs)
	tab_choices = _create_tab("Choices", "choices")
	tab_row.add_child(tab_choices)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", UITheme.BORDER_DIM)
	main_vbox.add_child(sep)

	# ── 본문 ──
	var content = HBoxContainer.new()
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_theme_constant_override("separation", 12)
	main_vbox.add_child(content)

	# 좌측: 목록
	item_scroll = ScrollContainer.new()
	item_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	content.add_child(item_scroll)

	item_list = VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 4)
	item_scroll.add_child(item_list)

	# 우측: 상세
	var detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(380, 0)
	detail_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.06, 0.05, 0.04, 0.8), UITheme.BORDER_DIM, 1, 4, 16
	))
	content.add_child(detail_panel)

	var detail_vbox = VBoxContainer.new()
	detail_vbox.add_theme_constant_override("separation", 10)
	detail_panel.add_child(detail_vbox)

	detail_title = Label.new()
	detail_title.text = "Select an entry..."
	detail_title.add_theme_font_size_override("font_size", 16)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.72, 0.58))
	detail_vbox.add_child(detail_title)

	var sep2 = HSeparator.new()
	sep2.add_theme_color_override("separator", UITheme.BORDER_DIM)
	detail_vbox.add_child(sep2)

	detail_body = RichTextLabel.new()
	detail_body.bbcode_enabled = false
	detail_body.fit_content = true
	detail_body.scroll_active = true
	detail_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	detail_body.add_theme_font_size_override("normal_font_size", 14)
	detail_body.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	detail_vbox.add_child(detail_body)

	# ── 하단 ──
	close_hint = Label.new()
	close_hint.text = "[ESC] Close Journal"
	close_hint.add_theme_font_size_override("font_size", 11)
	close_hint.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	close_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	main_vbox.add_child(close_hint)

func _create_tab(text: String, tab_name: String) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.custom_minimum_size = Vector2(120, 28)
	var style = UITheme.make_button_style()
	btn.add_theme_stylebox_override("normal", style)
	btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(style))
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	btn.pressed.connect(func():
		_current_tab = tab_name
		_refresh_list()
		AudioManager.play_sfx("ui_select")
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 목록 갱신 =====================

func _refresh_list() -> void:
	_update_tab_styles()

	for child in item_list.get_children():
		child.queue_free()

	detail_title.text = "Select an entry..."
	detail_body.text = ""

	match _current_tab:
		"events":
			_populate_events()
		"npcs":
			_populate_npcs()
		"choices":
			_populate_choices()

func _update_tab_styles() -> void:
	for tab_data in [{"btn": tab_events, "name": "events"}, {"btn": tab_npcs, "name": "npcs"}, {"btn": tab_choices, "name": "choices"}]:
		var btn: Button = tab_data.btn
		var active: bool = (_current_tab == tab_data.name)
		if active:
			btn.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
			var style = btn.get_theme_stylebox("normal").duplicate()
			style.border_color = UITheme.TEXT_ACCENT
			style.set_border_width_all(1)
			btn.add_theme_stylebox_override("normal", style)
		else:
			btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
			btn.add_theme_stylebox_override("normal", UITheme.make_button_style())

func _populate_events() -> void:
	var last_chapter := 0
	for entry in EVENT_ENTRIES:
		if not GameManager.get_flag(entry.flag):
			continue
		# 챕터 헤더
		if entry.chapter != last_chapter:
			last_chapter = entry.chapter
			var header = Label.new()
			header.text = "— Chapter %d: %s —" % [entry.chapter, CHAPTER_NAMES.get(entry.chapter, "")]
			header.add_theme_font_size_override("font_size", 12)
			header.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
			item_list.add_child(header)
		var is_hidden = entry.title.begins_with("[Hidden]")
		var color = Color(0.6, 0.5, 0.7) if is_hidden else Color(0.75, 0.7, 0.65)
		_add_list_button(entry.title, color, entry.title, entry.desc)

func _populate_npcs() -> void:
	for npc in NPC_ENTRIES:
		if not GameManager.get_flag(npc.flag):
			continue
		var speaker_color = UITheme.get_speaker_color(npc.name)
		var full_desc = "%s\n\n%s" % [npc.role, npc.desc]
		_add_list_button(npc.name, speaker_color, npc.name, full_desc)

func _populate_choices() -> void:
	# 선택 기록 — 주요 분기점
	var choice_entries := [
		{"flag": "malet_deal_accepted", "title": "Accepted Malet's Deal", "desc": "Traded the memory of first holding a sword for information. The courtyard, the dust, the hand — gone."},
		{"flag": "malet_deal_refused", "title": "Refused Malet's Deal", "desc": "Kept the sword memory. Left the Sump without Malet's help. (But returned later.)"},
		{"flag": "elia_separates", "title": "Sent Elia South", "desc": "Split up at the Crumbling Coast to confuse Kairos. Burned memories during separation left no residue."},
		{"flag": "elia_stays", "title": "Kept Elia Close", "desc": "Traveled the Coast together. The anchor stayed. Memories burned still left traces."},
		{"flag": "zero_burn_path", "title": "Burned Your Name", "desc": "Zero Burn. The ultimate sacrifice. BL-07 closed, but the person called 'Arrel' ceased to exist."},
		{"flag": "seal_refused", "title": "Kept Your Name", "desc": "Refused the Seal. BL-07 remains, but so does the person who remembers."},
	]
	for entry in choice_entries:
		if not GameManager.get_flag(entry.flag):
			continue
		_add_list_button(entry.title, Color(0.7, 0.6, 0.45), entry.title, entry.desc)

	if item_list.get_child_count() == 0:
		var empty = Label.new()
		empty.text = "No major choices recorded yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _add_list_button(text: String, color: Color, title: String, desc: String) -> void:
	var btn = Button.new()
	btn.text = text
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 32)

	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r * 0.2, color.g * 0.2, color.b * 0.2, 0.4)
	style.border_width_left = 3
	style.border_color = Color(color.r * 0.4, color.g * 0.4, color.b * 0.4, 0.3)
	style.set_content_margin_all(6)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)

	var hover = style.duplicate()
	hover.bg_color = Color(color.r * 0.3, color.g * 0.3, color.b * 0.3, 0.6)
	hover.border_color = color
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", Color(0.72, 0.68, 0.62))
	btn.add_theme_color_override("font_hover_color", color.lightened(0.3))

	btn.pressed.connect(func():
		detail_title.text = title
		detail_body.text = desc
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	item_list.add_child(btn)

func _show_ui() -> void:
	overlay.visible = true
	main_panel.visible = true

func _hide_ui() -> void:
	overlay.visible = false
	main_panel.visible = false
