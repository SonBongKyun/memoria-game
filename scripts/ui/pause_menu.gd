## PauseMenu (Autoload) — 일시정지 메뉴
## ESC 키로 토글. Resume / Save / Load / Title / Quit.
extends CanvasLayer

var is_open: bool = false

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
		elif GameManager.current_state == GameManager.GameState.EXPLORATION:
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
	AudioManager.play_sfx("confirm")
	# 첫 버튼 포커스
	if btn_container.get_child_count() > 0:
		btn_container.get_child(0).grab_focus()

func _close() -> void:
	if not is_open:
		return
	is_open = false
	get_tree().paused = false
	AudioManager.play_sfx("cancel")
	_hide_ui()

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
		btn_container.add_child(btn)

	# 하단 조작법
	var hint = Label.new()
	hint.text = "F6: Quick Save  |  F7: Quick Load"
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.add_theme_font_size_override("font_size", 11)
	hint.add_theme_color_override("font_color", Color(0.4, 0.38, 0.35))
	vbox.add_child(hint)

func _update_save_info() -> void:
	var chapter_name = ["", "Rim Forest", "Verdan Market", "Crumbling Coast", "The Seam", "BL-07 Void", "Epilogue"]
	var ch = clampi(GameManager.current_chapter, 1, 6)
	var hp = GameManager.player_data.hp
	var max_hp = GameManager.player_data.max_hp
	var burn_count = MemoryManager.get_burn_count()
	var memory_count = MemoryManager.memories.size()

	var text = "Chapter %d — %s\n" % [ch, chapter_name[ch]]
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

func _on_quit() -> void:
	get_tree().quit()
