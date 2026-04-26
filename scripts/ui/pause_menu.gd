## PauseMenu (Autoload) — 일시정지 메뉴
## ESC 키로 토글. Resume / Save / Load / Title / Quit.
extends CanvasLayer

var is_open: bool = false
var _panel_original_x: float = 0.0  # S53: 슬라이드 애니메이션용
var _anim_tween: Tween = null  # S53

# UI 노드
var overlay: ColorRect
var panel: PanelContainer
var btn_container: VBoxContainer
var save_info_label: Label
var title_label: Label

func _ready() -> void:
	layer = 55  # DialogueBox(50)와 SystemLog(60) 사이
	_build_ui()
	_hide_ui()
	process_mode = Node.PROCESS_MODE_ALWAYS
	print("[PauseMenu] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menu"):
		# 메뉴/대화/전투/컷씬 중에는 열지 않음
		if is_open:
			_close()
			get_viewport().set_input_as_handled()
		elif GameManager.current_state == GameManager.GameState.EXPLORATION and not MemoryUI.is_open:
			_open()
			get_viewport().set_input_as_handled()

func _open() -> void:
	if is_open:
		return
	is_open = true
	get_tree().paused = true
	_update_save_info()
	overlay.visible = true
	panel.visible = true
	# S53: 메뉴 슬라이드 인 애니메이션
	_panel_original_x = panel.position.x
	panel.modulate.a = 0.0
	panel.position.x = _panel_original_x - 300
	if _anim_tween and _anim_tween.is_valid():
		_anim_tween.kill()
	_anim_tween = create_tween().set_parallel(true)
	_anim_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	_anim_tween.tween_property(panel, "modulate:a", 1.0, 0.25).set_ease(Tween.EASE_OUT)
	_anim_tween.tween_property(panel, "position:x", _panel_original_x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	AudioManager.play_sfx("ui_open")
	# 첫 버튼 포커스
	if btn_container.get_child_count() > 0:
		btn_container.get_child(0).grab_focus()

func _close() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	# S53: 메뉴 슬라이드 아웃 애니메이션
	if _anim_tween and _anim_tween.is_valid():
		_anim_tween.kill()
	_anim_tween = create_tween().set_parallel(true)
	_anim_tween.set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
	_anim_tween.tween_property(panel, "modulate:a", 0.0, 0.2)
	_anim_tween.tween_property(panel, "position:x", _panel_original_x - 300, 0.2).set_ease(Tween.EASE_IN)
	_anim_tween.chain().tween_callback(func():
		_hide_ui()
		get_tree().paused = false
	)

func _hide_ui() -> void:
	if overlay:
		overlay.visible = false
	if panel:
		panel.visible = false

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# 어두운 오버레이
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.7)  # S57: darker blur for cinematic feel (40% brightness)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	# 중앙 패널
	panel = PanelContainer.new()
	panel.anchor_left = 0.3
	panel.anchor_right = 0.7
	panel.anchor_top = 0.15
	panel.anchor_bottom = 0.85
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.95)
	style.border_color = Color(0.4, 0.3, 0.2, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	panel.add_child(vbox)

	# 타이틀
	title_label = Label.new()
	title_label.text = GameManager.loc("paused")
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.add_theme_font_size_override("font_size", 22)
	title_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))
	vbox.add_child(title_label)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 8)
	vbox.add_child(sep)

	# 게임 상태 정보
	var info_panel = PanelContainer.new()
	var info_style = StyleBoxFlat.new()
	info_style.bg_color = Color(0.08, 0.07, 0.1, 0.8)
	info_style.set_content_margin_all(12)
	info_style.set_corner_radius_all(3)
	info_panel.add_theme_stylebox_override("panel", info_style)
	vbox.add_child(info_panel)

	save_info_label = Label.new()
	save_info_label.add_theme_font_size_override("font_size", 13)
	save_info_label.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
	info_panel.add_child(save_info_label)

	# 구분선
	var sep2 = HSeparator.new()
	sep2.add_theme_constant_override("separation", 8)
	vbox.add_child(sep2)

	# 버튼들
	btn_container = VBoxContainer.new()
	btn_container.add_theme_constant_override("separation", 8)
	vbox.add_child(btn_container)

	# S65 (A안 피벗): VN 정체성에 맞게 메뉴 슬림화.
	# 숨김: Fast Travel, Stats, Load Autosave (RPG 기능 — 스토리 몰입 방해)
	# 유지: Resume, Journal, Codex, Achievements (Steam 기대치), Endings, Options, Save/Load, Title, Quit
	var buttons = [
		{"text": GameManager.loc("resume"), "callback": _close},
		{"text": GameManager.loc("journal"), "callback": _on_journal},
		{"text": GameManager.loc("codex"), "callback": _on_codex},
		{"text": GameManager.loc("achievements"), "callback": _on_achievements},
	]
	# S54: Endings button (only if at least 1 ending seen)
	if GameManager.seen_endings.size() > 0:
		buttons.append({"text": GameManager.loc("endings"), "callback": _on_endings})
	buttons.append_array([
		{"text": GameManager.loc("options"), "callback": _on_options},
		{"text": GameManager.loc("save"), "callback": _on_save},
		{"text": GameManager.loc("load"), "callback": _on_load},
		{"text": GameManager.loc("title_return"), "callback": _on_title},
		{"text": GameManager.loc("quit"), "callback": _on_quit},
	])

	for data in buttons:
		var btn = Button.new()
		btn.text = data.text
		btn.custom_minimum_size = Vector2(0, 40)

		var btn_style = StyleBoxFlat.new()
		btn_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
		btn_style.border_color = Color(0.35, 0.28, 0.2, 0.5)
		btn_style.set_border_width_all(1)
		btn_style.set_corner_radius_all(3)
		btn_style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", btn_style)

		var hover_style = btn_style.duplicate()
		hover_style.bg_color = Color(0.15, 0.12, 0.18, 0.95)
		hover_style.border_color = Color(0.7, 0.55, 0.3, 0.8)
		btn.add_theme_stylebox_override("hover", hover_style)
		btn.add_theme_stylebox_override("focus", hover_style)

		var press_style = btn_style.duplicate()
		press_style.bg_color = Color(0.18, 0.14, 0.1, 0.95)
		press_style.border_color = Color(0.85, 0.65, 0.3, 1.0)
		btn.add_theme_stylebox_override("pressed", press_style)

		btn.add_theme_font_size_override("font_size", 15)
		btn.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.82, 0.5))

		btn.pressed.connect(data.callback)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		# S57: Hover sound on mouse enter + button press scale feedback
		btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		btn.pivot_offset = Vector2(btn.custom_minimum_size.x / 2.0, btn.custom_minimum_size.y / 2.0)
		btn.button_down.connect(func():
			var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
			t.tween_property(btn, "scale", Vector2(0.95, 0.95), 0.05)
		)
		btn.button_up.connect(func():
			var t = create_tween().set_pause_mode(Tween.TWEEN_PAUSE_PROCESS)
			t.tween_property(btn, "scale", Vector2(1.0, 1.0), 0.08).set_ease(Tween.EASE_OUT)
		)
		btn_container.add_child(btn)

	# S56: Last saved indicator
	var last_saved_label = Label.new()
	last_saved_label.name = "LastSavedLabel"
	last_saved_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	last_saved_label.add_theme_font_size_override("font_size", 11)
	last_saved_label.add_theme_color_override("font_color", Color(0.45, 0.55, 0.35))
	vbox.add_child(last_saved_label)

	# 하단 조작법 — S56: Dynamic hints based on input mode
	var hint = Label.new()
	hint.name = "HintLabel"
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 11)
	hint.add_theme_color_override("font_color", Color(0.4, 0.38, 0.35))
	vbox.add_child(hint)
	_update_hint_text(hint, last_saved_label)

## S56: Update hint text based on input mode
func _update_hint_text(hint_label: Label, last_saved: Label) -> void:
	if InputManager and InputManager.is_controller_mode():
		hint_label.text = "[LB] Quick Save  |  [RB] Quick Load  |  [B] Close"
	else:
		hint_label.text = "F6: Quick Save  |  F7: Quick Load"
	last_saved.text = SaveManager.get_last_saved_text()

func _update_save_info() -> void:
	var chapter_name = {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}
	var ch = GameManager.current_chapter
	var hp = GameManager.player_data.hp
	var max_hp = GameManager.player_data.max_hp
	var burn_count = MemoryManager.get_burn_count()
	var memory_count = MemoryManager.memories.size()

	var ng_text = ""
	if GameManager.ng_plus_cycle > 0:
		ng_text = " (NG+%d)" % GameManager.ng_plus_cycle
	var ch_name = chapter_name.get(ch, "Unknown")
	var text = "Chapter %d — %s%s\n" % [ch, ch_name, ng_text]
	text += "HP: %d / %d\n" % [hp, max_hp]
	text += "Memories: %d held, %d burned" % [memory_count - burn_count, burn_count]

	# S57: Enhanced save slot display with chapter name, HP, grains, and playtime
	var ch_names = {1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter", 5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest", 9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue"}

	var save = SaveManager.get_save_info(1)
	if save.is_empty():
		text += "\n\nSlot 1: [Empty]"
	else:
		var s_ch = save.get("chapter", 1)
		var s_ch_name = ch_names.get(s_ch, "Unknown")
		var s_hp = save.get("hp", 0)
		var s_max_hp = save.get("max_hp", 100)
		var s_grains = save.get("grains", 0)
		var s_location = save.get("location", "")
		text += "\n\nSlot 1: Ch%d - %s" % [s_ch, s_ch_name]
		if s_location != "":
			text += " (%s)" % s_location
		text += "\n    HP: %d/%d | Grains: %d | %s" % [s_hp, s_max_hp, s_grains, save.get("timestamp", "?")]

	# S56/S57: Autosave slot info (enhanced)
	var auto_save = SaveManager.get_save_info(0)
	if not auto_save.is_empty():
		var a_ch = auto_save.get("chapter", 1)
		var a_ch_name = ch_names.get(a_ch, "Unknown")
		var a_hp = auto_save.get("hp", 0)
		var a_max_hp = auto_save.get("max_hp", 100)
		text += "\nAutosave: Ch%d - %s | HP: %d/%d | %s" % [a_ch, a_ch_name, a_hp, a_max_hp, auto_save.get("timestamp", "?")]

	save_info_label.text = text

func _on_save() -> void:
	SaveManager.save_game(1)
	AudioManager.play_sfx("confirm")
	_update_save_info()
	# 세이브 완료 피드백
	title_label.text = "SAVED!"
	title_label.add_theme_color_override("font_color", Color(0.4, 0.7, 0.45))
	await get_tree().create_timer(0.8).timeout
	if not is_open:
		return
	title_label.text = GameManager.loc("paused")
	title_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))

func _on_load() -> void:
	if not SaveManager.has_save(1):
		AudioManager.play_sfx("cancel")
		return
	_close()
	SaveManager.load_game(1)

## S56: Load autosave
func _on_load_autosave() -> void:
	if not SaveManager.has_save(0):
		AudioManager.play_sfx("cancel")
		NotificationToast.show_toast("No autosave found", NotificationToast.ToastType.WARNING)
		return
	_close()
	SaveManager.load_game(0)

func _on_title() -> void:
	_close()
	SceneTransition.change_scene("res://scenes/main/main.tscn")

func _on_journal() -> void:
	AudioManager.play_sfx("ui_select")
	StoryJournal.open_journal()

func _on_options() -> void:
	AudioManager.play_sfx("ui_select")
	OptionsMenu.open()

func _on_codex() -> void:
	AudioManager.play_sfx("ui_select")
	Codex.open()

func _on_achievements() -> void:
	AudioManager.play_sfx("ui_select")
	_show_achievements_panel()

func _show_achievements_panel() -> void:
	# 업적 패널 (PauseMenu 위에 오버레이)
	var ach_overlay = ColorRect.new()
	ach_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	ach_overlay.color = Color(0, 0, 0, 0.7)
	ach_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(ach_overlay)

	var ach_panel = PanelContainer.new()
	ach_panel.anchor_left = 0.15
	ach_panel.anchor_right = 0.85
	ach_panel.anchor_top = 0.05
	ach_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.5, 0.4, 0.25, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(16)
	ach_panel.add_theme_stylebox_override("panel", style)
	ach_overlay.add_child(ach_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	ach_panel.add_child(vbox)

	# 타이틀
	var header = Label.new()
	var all_achs = AchievementManager.get_all_achievements()
	var unlocked_count = 0
	for a in all_achs:
		if a["unlocked"]:
			unlocked_count += 1
	header.text = "ACHIEVEMENTS  (%d / %d)" % [unlocked_count, all_achs.size()]
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", Color(0.85, 0.7, 0.45))
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# 스크롤 리스트
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(scroll)

	var list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 4)
	scroll.add_child(list)

	for ach in all_achs:
		var row = HBoxContainer.new()
		row.add_theme_constant_override("separation", 10)
		list.add_child(row)

		# 아이콘 (간단한 텍스트)
		var icon_map = {"sword": "⚔", "skull": "💀", "crown": "👑", "shield": "🛡", "heart": "❤", "potion": "🧪", "flame": "🔥", "eye": "👁", "map": "🗺", "book": "📖", "star": "⭐", "coin": "🪙", "cycle": "🔄"}
		var icon_label = Label.new()
		icon_label.text = icon_map.get(ach.get("icon", ""), "•")
		icon_label.add_theme_font_size_override("font_size", 16)
		icon_label.custom_minimum_size = Vector2(28, 0)
		row.add_child(icon_label)

		var info = VBoxContainer.new()
		info.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(info)

		var title_lbl = Label.new()
		title_lbl.add_theme_font_size_override("font_size", 14)
		info.add_child(title_lbl)

		var desc_lbl = Label.new()
		desc_lbl.add_theme_font_size_override("font_size", 11)
		info.add_child(desc_lbl)

		if ach["unlocked"]:
			title_lbl.text = ach["title"]
			title_lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
			desc_lbl.text = ach["desc"]
			desc_lbl.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
		else:
			title_lbl.text = "???"
			title_lbl.add_theme_color_override("font_color", Color(0.35, 0.3, 0.28))
			desc_lbl.text = ach["desc"]
			desc_lbl.add_theme_color_override("font_color", Color(0.3, 0.28, 0.25))

	# 닫기 힌트
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	# ESC로 닫기
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			ach_overlay.queue_free()
			get_viewport().set_input_as_handled()
	ach_overlay.gui_input.connect(close_handler)
	# 패널 클릭으로도 닫기
	ach_overlay.mouse_filter = Control.MOUSE_FILTER_STOP

func _on_travel() -> void:
	AudioManager.play_sfx("ui_select")
	_show_travel_panel()

func _show_travel_panel() -> void:
	var travel_overlay = ColorRect.new()
	travel_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	travel_overlay.color = Color(0, 0, 0, 0.7)
	travel_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(travel_overlay)

	var travel_panel = PanelContainer.new()
	travel_panel.anchor_left = 0.25
	travel_panel.anchor_right = 0.75
	travel_panel.anchor_top = 0.15
	travel_panel.anchor_bottom = 0.85
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.4, 0.5, 0.3, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	travel_panel.add_theme_stylebox_override("panel", style)
	travel_overlay.add_child(travel_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	travel_panel.add_child(vbox)

	var header = Label.new()
	header.text = "FAST TRAVEL"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", Color(0.7, 0.8, 0.55))
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	var desc = Label.new()
	desc.text = "Select a destination. Travel is instant."
	desc.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	desc.add_theme_font_size_override("font_size", 12)
	desc.add_theme_color_override("font_color", Color(0.5, 0.5, 0.45))
	vbox.add_child(desc)

	# 맵 목록 — 챕터에 따라 해금
	var maps = [
		{"name": "Rim Forest", "scene": "res://scenes/maps/rim_forest.tscn", "chapter": 1, "desc": "Where it all began."},
		{"name": "Verdan Market", "scene": "res://scenes/maps/verdan_market.tscn", "chapter": 2, "desc": "A place of trade and memory."},
		{"name": "Belt Waystation", "scene": "res://scenes/maps/belt_waystation.tscn", "chapter": 3, "desc": "Bureau Relay Station 14. The dead road."},
		{"name": "Drift Shelter", "scene": "res://scenes/maps/drift_shelter.tscn", "chapter": 4, "desc": "Where the architecture crumbles."},
		{"name": "Crumbling Coast", "scene": "res://scenes/maps/crumbling_coast.tscn", "chapter": 5, "desc": "Cliffs falling into the void."},
		{"name": "The Seam", "scene": "res://scenes/maps/the_seam.tscn", "chapter": 6, "desc": "Where color bleeds through."},
		{"name": "Seam Outskirts", "scene": "res://scenes/maps/seam_outskirts.tscn", "chapter": 7, "desc": "The Threshold. BL-07's edge."},
		{"name": "Forgotten Forest", "scene": "res://scenes/maps/forgotten_forest.tscn", "chapter": 8, "desc": "Trees that remember being trees."},
		{"name": "Colorless Waste", "scene": "res://scenes/maps/colorless_waste.tscn", "chapter": 9, "desc": "Where the concept of color withdrew."},
		{"name": "BL-07 Void", "scene": "res://scenes/maps/bl07_void.tscn", "chapter": 10, "desc": "The space between spaces."},
	]

	var current_ch = GameManager.current_chapter
	for map_data in maps:
		var btn = Button.new()
		var unlocked = current_ch >= map_data["chapter"]
		btn.custom_minimum_size = Vector2(0, 44)

		var btn_style = StyleBoxFlat.new()
		btn_style.set_content_margin_all(10)
		btn_style.set_corner_radius_all(4)

		if unlocked:
			btn.text = "Ch%d — %s\n    %s" % [map_data["chapter"], map_data["name"], map_data["desc"]]
			btn_style.bg_color = Color(0.08, 0.1, 0.06, 0.9)
			btn_style.border_color = Color(0.35, 0.45, 0.25, 0.5)
			btn_style.set_border_width_all(1)
			btn.add_theme_color_override("font_color", Color(0.65, 0.75, 0.5))
			btn.add_theme_color_override("font_hover_color", Color(0.85, 0.95, 0.6))
		else:
			btn.text = "Ch%d — ???" % map_data["chapter"]
			btn_style.bg_color = Color(0.06, 0.06, 0.06, 0.7)
			btn_style.border_color = Color(0.2, 0.2, 0.2, 0.3)
			btn_style.set_border_width_all(1)
			btn.add_theme_color_override("font_color", Color(0.3, 0.3, 0.3))
			btn.disabled = true

		btn.add_theme_stylebox_override("normal", btn_style)
		var hover_s = btn_style.duplicate()
		hover_s.bg_color = Color(0.12, 0.16, 0.08, 0.95)
		hover_s.border_color = Color(0.6, 0.7, 0.35, 0.8)
		btn.add_theme_stylebox_override("hover", hover_s)
		btn.add_theme_stylebox_override("focus", hover_s)
		btn.add_theme_font_size_override("font_size", 13)

		if unlocked:
			var scene_path = map_data["scene"]
			btn.pressed.connect(func():
				AudioManager.play_sfx("confirm")
				travel_overlay.queue_free()
				_close()
				SceneTransition.change_scene_styled(scene_path)
			)
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))

		vbox.add_child(btn)

	# 닫기
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			travel_overlay.queue_free()
			get_viewport().set_input_as_handled()
	travel_overlay.gui_input.connect(close_handler)

## S54: Ending Gallery
func _on_endings() -> void:
	AudioManager.play_sfx("ui_select")
	_show_endings_gallery()

func _show_endings_gallery() -> void:
	var end_overlay = ColorRect.new()
	end_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	end_overlay.color = Color(0, 0, 0, 0.8)
	end_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(end_overlay)

	var end_panel = PanelContainer.new()
	end_panel.anchor_left = 0.1
	end_panel.anchor_right = 0.9
	end_panel.anchor_top = 0.05
	end_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.03, 0.06, 0.98)
	style.border_color = Color(0.6, 0.45, 0.2, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	end_panel.add_theme_stylebox_override("panel", style)
	end_overlay.add_child(end_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	end_panel.add_child(vbox)

	# Header
	var header = Label.new()
	header.text = "ENDING GALLERY  (%d / %d)" % [GameManager.seen_endings.size(), GameManager.ENDING_DATA.size()]
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 20)
	header.add_theme_color_override("font_color", Color(0.85, 0.7, 0.4))
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# Grid of endings
	var grid = GridContainer.new()
	grid.columns = 3
	grid.add_theme_constant_override("h_separation", 16)
	grid.add_theme_constant_override("v_separation", 16)
	vbox.add_child(grid)

	var ending_ids = ["zero_burn", "preservation", "ash", "seam", "tobias", "hollow"]
	for eid in ending_ids:
		var card = VBoxContainer.new()
		card.custom_minimum_size = Vector2(180, 160)
		card.add_theme_constant_override("separation", 6)
		grid.add_child(card)

		var seen = eid in GameManager.seen_endings
		var data = GameManager.ENDING_DATA.get(eid, {})

		# Thumbnail area
		var thumb = ColorRect.new()
		thumb.custom_minimum_size = Vector2(180, 100)
		if seen:
			# Try to load CG image
			var cg_path = data.get("cg", "")
			if cg_path != "" and ResourceLoader.exists(cg_path):
				var tex_rect = TextureRect.new()
				tex_rect.custom_minimum_size = Vector2(180, 100)
				tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
				tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
				tex_rect.texture = load(cg_path)
				card.add_child(tex_rect)
			else:
				# Fallback colored rect
				thumb.color = Color(0.15, 0.12, 0.18)
				card.add_child(thumb)
		else:
			# Locked — dark with lock icon
			thumb.color = Color(0.06, 0.05, 0.07)
			card.add_child(thumb)
			var lock_label = Label.new()
			lock_label.text = "?"
			lock_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			lock_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			lock_label.add_theme_font_size_override("font_size", 32)
			lock_label.add_theme_color_override("font_color", Color(0.25, 0.2, 0.18))
			lock_label.set_anchors_preset(Control.PRESET_FULL_RECT)
			thumb.add_child(lock_label)

		# Title
		var title_lbl = Label.new()
		title_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		title_lbl.add_theme_font_size_override("font_size", 13)
		if seen:
			title_lbl.text = data.get("name", eid)
			title_lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
		else:
			title_lbl.text = "???"
			title_lbl.add_theme_color_override("font_color", Color(0.3, 0.25, 0.2))
		card.add_child(title_lbl)

		# Description (only if seen)
		var desc_lbl = Label.new()
		desc_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		desc_lbl.add_theme_font_size_override("font_size", 10)
		desc_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD
		desc_lbl.custom_minimum_size = Vector2(180, 0)
		if seen:
			desc_lbl.text = data.get("desc", "")
			desc_lbl.add_theme_color_override("font_color", Color(0.55, 0.5, 0.45))
		else:
			desc_lbl.text = "Reach this ending to unlock."
			desc_lbl.add_theme_color_override("font_color", Color(0.25, 0.22, 0.2))
		card.add_child(desc_lbl)

	# Close hint
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			end_overlay.queue_free()
			get_viewport().set_input_as_handled()
	end_overlay.gui_input.connect(close_handler)

## S55: Statistics Screen
func _on_stats() -> void:
	AudioManager.play_sfx("ui_select")
	_show_stats_panel()

func _show_stats_panel() -> void:
	var stats_overlay = ColorRect.new()
	stats_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	stats_overlay.color = Color(0, 0, 0, 0.7)
	stats_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(stats_overlay)

	var stats_panel = PanelContainer.new()
	stats_panel.anchor_left = 0.2
	stats_panel.anchor_right = 0.8
	stats_panel.anchor_top = 0.05
	stats_panel.anchor_bottom = 0.95
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.45, 0.55, 0.35, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(20)
	stats_panel.add_theme_stylebox_override("panel", style)
	stats_overlay.add_child(stats_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	stats_panel.add_child(vbox)

	# Header
	var header = Label.new()
	header.text = "PLAY STATISTICS"
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	header.add_theme_font_size_override("font_size", 20)
	header.add_theme_color_override("font_color", Color(0.75, 0.85, 0.55))
	vbox.add_child(header)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# Scrollable stat list
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(scroll)

	var list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 6)
	scroll.add_child(list)

	var stats = GameManager.play_stats
	# S56: Completion percentage
	var completion = AchievementManager.get_completion_percentage()
	var stat_display = [
		{"label": "Play Time", "value": GameManager.format_play_time()},
		{"label": "Completion", "value": "%.1f%%" % completion},
		{"label": "Achievements", "value": "%d / %d" % [AchievementManager.unlocked.size(), AchievementManager.ACHIEVEMENTS.size()]},
		{"label": "Endings Seen", "value": "%d / %d" % [GameManager.seen_endings.size(), GameManager.ENDING_DATA.size()]},
		{"label": "", "value": ""},
		{"label": "Total Battles", "value": str(int(stats.total_battles))},
		{"label": "Enemies Defeated", "value": str(int(stats.enemies_defeated))},
		{"label": "Bosses Defeated", "value": str(int(stats.bosses_defeated))},
		{"label": "Memories Burned", "value": str(int(stats.total_burns))},
		{"label": "Memories Collected", "value": str(int(stats.memories_collected))},
		{"label": "Grains Earned", "value": str(int(stats.total_grains_earned))},
		{"label": "Steps Taken", "value": str(int(stats.steps_taken))},
		{"label": "Highest Combo", "value": str(int(stats.highest_combo))},
		{"label": "Items Used", "value": str(int(stats.items_used))},
		{"label": "", "value": ""},
		{"label": "Current Chapter", "value": str(GameManager.current_chapter)},
		{"label": "NG+ Cycle", "value": str(GameManager.ng_plus_cycle)},
		{"label": "Last Saved", "value": SaveManager.get_last_saved_text()},
	]

	for entry in stat_display:
		var row = HBoxContainer.new()
		row.add_theme_constant_override("separation", 8)
		list.add_child(row)

		var name_label = Label.new()
		name_label.text = entry.label
		name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		name_label.add_theme_font_size_override("font_size", 15)
		name_label.add_theme_color_override("font_color", Color(0.6, 0.58, 0.52))
		row.add_child(name_label)

		var val_label = Label.new()
		val_label.text = entry.value
		val_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		val_label.add_theme_font_size_override("font_size", 15)
		val_label.add_theme_color_override("font_color", Color(0.9, 0.8, 0.5))
		val_label.custom_minimum_size = Vector2(120, 0)
		row.add_child(val_label)

	# Close hint
	var close_label = Label.new()
	close_label.text = "[ESC] Close"
	close_label.add_theme_font_size_override("font_size", 11)
	close_label.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_label)

	# ESC close handler
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			stats_overlay.queue_free()
			get_viewport().set_input_as_handled()
	stats_overlay.gui_input.connect(close_handler)

## S59: Quit confirmation dialog
func _on_quit() -> void:
	AudioManager.play_sfx("ui_select")
	_show_quit_confirmation()

func _show_quit_confirmation() -> void:
	var confirm_overlay = ColorRect.new()
	confirm_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	confirm_overlay.color = Color(0, 0, 0, 0.7)
	confirm_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(confirm_overlay)

	var confirm_panel = PanelContainer.new()
	confirm_panel.anchor_left = 0.3
	confirm_panel.anchor_right = 0.7
	confirm_panel.anchor_top = 0.35
	confirm_panel.anchor_bottom = 0.65
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.98)
	style.border_color = Color(0.7, 0.4, 0.3, 0.7)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(24)
	confirm_panel.add_theme_stylebox_override("panel", style)
	confirm_overlay.add_child(confirm_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 16)
	confirm_panel.add_child(vbox)

	var question = Label.new()
	question.text = "Are you sure you want to quit?"
	question.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	question.add_theme_font_size_override("font_size", 18)
	question.add_theme_color_override("font_color", Color(0.85, 0.7, 0.5))
	vbox.add_child(question)

	var hint = Label.new()
	hint.text = "Unsaved progress will be lost."
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 12)
	hint.add_theme_color_override("font_color", Color(0.55, 0.45, 0.4, 0.7))
	vbox.add_child(hint)

	var btn_row = HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 20)
	vbox.add_child(btn_row)

	var yes_btn = Button.new()
	yes_btn.text = "Yes, Quit"
	yes_btn.custom_minimum_size = Vector2(120, 40)
	var yes_style = StyleBoxFlat.new()
	yes_style.bg_color = Color(0.25, 0.1, 0.08, 0.9)
	yes_style.border_color = Color(0.7, 0.35, 0.25, 0.6)
	yes_style.set_border_width_all(1)
	yes_style.set_corner_radius_all(3)
	yes_style.set_content_margin_all(8)
	yes_btn.add_theme_stylebox_override("normal", yes_style)
	var yes_hover = yes_style.duplicate()
	yes_hover.border_color = Color(0.9, 0.45, 0.3, 0.9)
	yes_btn.add_theme_stylebox_override("hover", yes_hover)
	yes_btn.add_theme_stylebox_override("focus", yes_hover)
	yes_btn.add_theme_font_size_override("font_size", 15)
	yes_btn.add_theme_color_override("font_color", Color(0.85, 0.55, 0.4))
	yes_btn.add_theme_color_override("font_hover_color", Color(1.0, 0.7, 0.5))
	yes_btn.pressed.connect(func():
		get_tree().quit()
	)
	yes_btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn_row.add_child(yes_btn)

	var no_btn = Button.new()
	no_btn.text = "No, Stay"
	no_btn.custom_minimum_size = Vector2(120, 40)
	var no_style = StyleBoxFlat.new()
	no_style.bg_color = Color(0.1, 0.08, 0.12, 0.9)
	no_style.border_color = Color(0.35, 0.45, 0.3, 0.6)
	no_style.set_border_width_all(1)
	no_style.set_corner_radius_all(3)
	no_style.set_content_margin_all(8)
	no_btn.add_theme_stylebox_override("normal", no_style)
	var no_hover = no_style.duplicate()
	no_hover.border_color = Color(0.5, 0.7, 0.4, 0.9)
	no_btn.add_theme_stylebox_override("hover", no_hover)
	no_btn.add_theme_stylebox_override("focus", no_hover)
	no_btn.add_theme_font_size_override("font_size", 15)
	no_btn.add_theme_color_override("font_color", Color(0.6, 0.75, 0.5))
	no_btn.add_theme_color_override("font_hover_color", Color(0.75, 0.9, 0.6))
	no_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_close")
		confirm_overlay.queue_free()
	)
	no_btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	btn_row.add_child(no_btn)

	# ESC closes the confirmation (No)
	var close_handler = func(event: InputEvent):
		if event.is_action_pressed("cancel") or event.is_action_pressed("menu"):
			AudioManager.play_sfx("ui_close")
			confirm_overlay.queue_free()
			get_viewport().set_input_as_handled()
	confirm_overlay.gui_input.connect(close_handler)

	# Focus the No button by default
	no_btn.grab_focus()
