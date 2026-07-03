## GameOver — 패배 화면
## "Arrel falls..." + 재시도 / 로드 / 타이틀 선택.
extends Control

var return_scene: String = ""
const GAME_OVER_BACKDROP_PATH: String = "res://assets/cg/generated/ui_game_over_void_backdrop.png"

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	return_scene = BattleManager.return_scene
	_build_ui()
	AudioManager.stop_bgm(true)

func _build_ui() -> void:
	# 어두운 배경
	if ResourceLoader.exists(GAME_OVER_BACKDROP_PATH):
		var backdrop = TextureRect.new()
		backdrop.texture = load(GAME_OVER_BACKDROP_PATH)
		backdrop.set_anchors_preset(PRESET_FULL_RECT)
		backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
		backdrop.modulate = Color(0.92, 0.88, 0.82, 0.86)
		add_child(backdrop)

	var bg = ColorRect.new()
	bg.set_anchors_preset(PRESET_FULL_RECT)
	bg.color = Color(0.02, 0.015, 0.03, 0.58)
	add_child(bg)

	var panel = PanelContainer.new()
	panel.set_anchors_preset(PRESET_CENTER)
	panel.offset_left = -230
	panel.offset_right = 230
	panel.offset_top = -190
	panel.offset_bottom = 190
	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = Color(0.025, 0.020, 0.034, 0.84)
	panel_style.border_color = Color(0.62, 0.28, 0.22, 0.72)
	panel_style.set_border_width_all(2)
	panel_style.set_corner_radius_all(6)
	panel_style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", panel_style)
	add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 16)
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.add_child(vbox)

	# 타이틀 텍스트
	var title = Label.new()
	title.text = GameManager.loc("game_over_title")
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.6, 0.2, 0.2))
	vbox.add_child(title)

	# 서브 텍스트
	var sub = Label.new()
	sub.text = GameManager.loc("game_over_subtitle")
	sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	sub.add_theme_font_size_override("font_size", 14)
	sub.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	sub.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vbox.add_child(sub)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_constant_override("separation", 20)
	vbox.add_child(sep)

	# 버튼들
	var buttons = [
		{"text": GameManager.loc("retry"), "callback": _on_retry},
		{"text": GameManager.loc("load_save"), "callback": _on_load},
		{"text": GameManager.loc("title_return"), "callback": _on_title},
	]

	var first_btn: Button = null
	for data in buttons:
		var btn = Button.new()
		btn.text = data.text
		btn.custom_minimum_size = Vector2(0, 44)

		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.08, 0.06, 0.1, 0.9)
		style.border_color = Color(0.4, 0.2, 0.2, 0.6)
		style.set_border_width_all(1)
		style.set_corner_radius_all(4)
		style.set_content_margin_all(10)
		btn.add_theme_stylebox_override("normal", style)

		var hover = style.duplicate()
		hover.bg_color = Color(0.14, 0.1, 0.16, 0.95)
		hover.border_color = Color(0.7, 0.4, 0.3, 0.8)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_stylebox_override("focus", hover)

		btn.add_theme_font_size_override("font_size", 15)
		btn.add_theme_color_override("font_color", Color(0.7, 0.6, 0.55))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.75, 0.45))

		btn.pressed.connect(data.callback)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		vbox.add_child(btn)
		if first_btn == null:
			first_btn = btn

	# 첫 버튼 포커스 (짧은 딜레이로 빌드 완료 후)
	await get_tree().create_timer(0.1).timeout
	if is_instance_valid(first_btn):
		first_btn.grab_focus()

func _reset_battle() -> void:
	BattleManager.current_enemy = null
	BattleManager.state = BattleManager.BattleState.IDLE

func _on_retry() -> void:
	# HP 30% 회복 후 맵 복귀
	GameManager.player_data.hp = int(GameManager.player_data.max_hp * 0.3)
	_reset_battle()
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	if return_scene != "":
		SceneTransition.change_scene_styled(return_scene)
	else:
		SceneTransition.change_scene("res://scenes/main/main.tscn")

func _on_load() -> void:
	if SaveManager.has_save(1):
		_reset_battle()
		SaveManager.load_game(1)
	else:
		AudioManager.play_sfx("cancel")

func _on_title() -> void:
	_reset_battle()
	SceneTransition.change_scene("res://scenes/main/main.tscn")
