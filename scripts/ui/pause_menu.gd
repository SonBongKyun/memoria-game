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
	overlay.color = Color(0, 0, 0, 0.6)
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
	title_label.text = "PAUSED"
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

	var buttons = [
		{"text": "Resume", "callback": _close},
		{"text": "Journal", "callback": _on_journal},
		{"text": "Travel", "callback": _on_travel},
		{"text": "Codex", "callback": _on_codex},
		{"text": "Achievements", "callback": _on_achievements},
		{"text": "Options", "callback": _on_options},
		{"text": "Save (Slot 1)", "callback": _on_save},
		{"text": "Load (Slot 1)", "callback": _on_load},
		{"text": "Return to Title", "callback": _on_title},
		{"text": "Quit Game", "callback": _on_quit},
	]

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
		btn_container.add_child(btn)

	# 하단 조작법
	var hint = Label.new()
	hint.text = "F6: Quick Save  |  F7: Quick Load"
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 11)
	hint.add_theme_color_override("font_color", Color(0.4, 0.38, 0.35))
	vbox.add_child(hint)

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

	# 세이브 슬롯 정보
	var save = SaveManager.get_save_info(1)
	if save.is_empty():
		text += "\n\nSlot 1: Empty"
	else:
		text += "\n\nSlot 1: Ch%d, %s" % [save.get("chapter", 1), save.get("timestamp", "?")]

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
	title_label.text = "PAUSED"
	title_label.add_theme_color_override("font_color", Color(0.75, 0.65, 0.45))

func _on_load() -> void:
	if not SaveManager.has_save(1):
		AudioManager.play_sfx("cancel")
		return
	_close()
	SaveManager.load_game(1)

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
				SceneTransition.change_scene(scene_path)
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

func _on_quit() -> void:
	get_tree().quit()
