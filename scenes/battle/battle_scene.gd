## BattleScene — 턴제 전투 화면
## BattleManager의 시그널을 받아 UI 표시.
extends Node2D

# UI 노드
var bg: ColorRect
var enemy_name_label: Label
var enemy_hp_bar: ProgressBar
var enemy_hp_label: Label
var player_hp_bar: ProgressBar
var player_hp_label: Label
var log_label: RichTextLabel
var action_container: HBoxContainer
var burn_list_container: VBoxContainer
var enemy_sprite: ColorRect  # 플레이스홀더

var log_lines: Array = []
const MAX_LOG_LINES: int = 6

func _ready() -> void:
	_build_ui()
	_connect_signals()
	_update_hp_displays()

## UI 전체 구축
func _build_ui() -> void:
	# 배경 (이미지 또는 단색)
	bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.06, 0.06, 0.08)
	add_child(bg)

	if BattleManager.battle_bg_image != "" and ResourceLoader.exists(BattleManager.battle_bg_image):
		var bg_tex = TextureRect.new()
		bg_tex.set_anchors_preset(Control.PRESET_FULL_RECT)
		bg_tex.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		bg_tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		bg_tex.texture = load(BattleManager.battle_bg_image)
		bg_tex.modulate = Color(0.5, 0.45, 0.4, 0.7)  # 어둡게
		add_child(bg_tex)

	var canvas = CanvasLayer.new()
	canvas.layer = 10
	add_child(canvas)

	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(root)

	# 적 이름 + HP (상단)
	_build_enemy_panel(root)

	# 적 스프라이트 (화면 중앙 상단)
	_build_enemy_sprite(root)

	# 전투 로그 (중앙)
	_build_log_panel(root)

	# 플레이어 HP (좌하단)
	_build_player_panel(root)

	# 행동 버튼 (하단)
	_build_action_buttons(root)

	# 기억 연소 목록 (숨김 상태)
	_build_burn_list(root)

func _build_enemy_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.55
	panel.anchor_right = 0.95
	panel.anchor_top = 0.02
	panel.anchor_bottom = 0.02
	panel.offset_bottom = 70

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.1, 0.08, 0.08, 0.85)
	style.border_color = Color(0.5, 0.2, 0.2, 0.5)
	style.set_border_width_all(1)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	panel.add_child(vbox)

	enemy_name_label = Label.new()
	enemy_name_label.add_theme_font_size_override("font_size", 14)
	enemy_name_label.add_theme_color_override("font_color", Color(0.8, 0.4, 0.35))
	vbox.add_child(enemy_name_label)

	enemy_hp_bar = ProgressBar.new()
	enemy_hp_bar.custom_minimum_size = Vector2(0, 16)
	enemy_hp_bar.show_percentage = false
	var bar_style = StyleBoxFlat.new()
	bar_style.bg_color = Color(0.6, 0.15, 0.15)
	bar_style.set_corner_radius_all(2)
	enemy_hp_bar.add_theme_stylebox_override("fill", bar_style)
	var bar_bg = StyleBoxFlat.new()
	bar_bg.bg_color = Color(0.15, 0.1, 0.1)
	bar_bg.set_corner_radius_all(2)
	enemy_hp_bar.add_theme_stylebox_override("background", bar_bg)
	vbox.add_child(enemy_hp_bar)

	enemy_hp_label = Label.new()
	enemy_hp_label.add_theme_font_size_override("font_size", 11)
	enemy_hp_label.add_theme_color_override("font_color", Color(0.6, 0.35, 0.3))
	enemy_hp_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(enemy_hp_label)

func _build_enemy_sprite(root: Control) -> void:
	# 이미지가 있으면 TextureRect, 없으면 ColorRect fallback
	if BattleManager.enemy_image != "" and ResourceLoader.exists(BattleManager.enemy_image):
		var tex_rect = TextureRect.new()
		tex_rect.anchor_left = 0.25
		tex_rect.anchor_right = 0.75
		tex_rect.anchor_top = 0.08
		tex_rect.anchor_bottom = 0.48
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = load(BattleManager.enemy_image)
		root.add_child(tex_rect)
		# enemy_sprite를 참조용으로 유지
		enemy_sprite = ColorRect.new()
		enemy_sprite.visible = false
		root.add_child(enemy_sprite)
	else:
		enemy_sprite = ColorRect.new()
		enemy_sprite.anchor_left = 0.35
		enemy_sprite.anchor_right = 0.65
		enemy_sprite.anchor_top = 0.12
		enemy_sprite.anchor_bottom = 0.45
		enemy_sprite.color = Color(0.3, 0.1, 0.15)
		root.add_child(enemy_sprite)

		var eye_l = ColorRect.new()
		eye_l.anchor_left = 0.3
		eye_l.anchor_top = 0.3
		eye_l.offset_right = 12
		eye_l.offset_bottom = 12
		eye_l.color = Color(0.9, 0.2, 0.15)
		enemy_sprite.add_child(eye_l)

		var eye_r = ColorRect.new()
		eye_r.anchor_left = 0.6
		eye_r.anchor_top = 0.3
		eye_r.offset_right = 12
		eye_r.offset_bottom = 12
		eye_r.color = Color(0.9, 0.2, 0.15)
		enemy_sprite.add_child(eye_r)

func _build_log_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.05
	panel.anchor_right = 0.95
	panel.anchor_top = 0.48
	panel.anchor_bottom = 0.65

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.05, 0.05, 0.07, 0.8)
	style.set_content_margin_all(10)
	style.set_corner_radius_all(3)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	log_label = RichTextLabel.new()
	log_label.bbcode_enabled = true
	log_label.scroll_active = false
	log_label.fit_content = false
	log_label.add_theme_font_size_override("normal_font_size", 13)
	log_label.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	panel.add_child(log_label)

func _build_player_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.05
	panel.anchor_right = 0.4
	panel.anchor_top = 0.68
	panel.anchor_bottom = 0.68
	panel.offset_bottom = 70

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.08, 0.1, 0.85)
	style.border_color = Color(0.2, 0.3, 0.5, 0.5)
	style.set_border_width_all(1)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	panel.add_child(vbox)

	var name_label = Label.new()
	name_label.text = "Arrel"
	name_label.add_theme_font_size_override("font_size", 14)
	name_label.add_theme_color_override("font_color", Color(0.4, 0.5, 0.7))
	vbox.add_child(name_label)

	player_hp_bar = ProgressBar.new()
	player_hp_bar.custom_minimum_size = Vector2(0, 16)
	player_hp_bar.show_percentage = false
	var fill = StyleBoxFlat.new()
	fill.bg_color = Color(0.2, 0.45, 0.6)
	fill.set_corner_radius_all(2)
	player_hp_bar.add_theme_stylebox_override("fill", fill)
	var bg_s = StyleBoxFlat.new()
	bg_s.bg_color = Color(0.1, 0.1, 0.15)
	bg_s.set_corner_radius_all(2)
	player_hp_bar.add_theme_stylebox_override("background", bg_s)
	vbox.add_child(player_hp_bar)

	player_hp_label = Label.new()
	player_hp_label.add_theme_font_size_override("font_size", 11)
	player_hp_label.add_theme_color_override("font_color", Color(0.35, 0.45, 0.6))
	vbox.add_child(player_hp_label)

func _build_action_buttons(root: Control) -> void:
	action_container = HBoxContainer.new()
	action_container.anchor_left = 0.15
	action_container.anchor_right = 0.85
	action_container.anchor_top = 0.82
	action_container.anchor_bottom = 0.82
	action_container.offset_bottom = 50
	action_container.add_theme_constant_override("separation", 12)
	action_container.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(action_container)

	var actions = [
		{"text": "ATTACK", "callback": _on_attack},
		{"text": "BURN", "callback": _on_burn_menu},
		{"text": "DEFEND", "callback": _on_defend},
		{"text": "FLEE", "callback": _on_flee},
	]

	for action in actions:
		var btn = Button.new()
		btn.text = action.text
		btn.custom_minimum_size = Vector2(120, 40)

		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.12, 0.1, 0.14, 0.9)
		style.border_color = Color(0.4, 0.3, 0.25, 0.6)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", style)

		var hover = style.duplicate()
		hover.bg_color = Color(0.2, 0.15, 0.22, 0.95)
		hover.border_color = Color(0.7, 0.55, 0.35, 0.8)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_stylebox_override("focus", hover)

		btn.add_theme_font_size_override("font_size", 14)
		btn.add_theme_color_override("font_color", Color(0.75, 0.7, 0.65))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.8, 0.5))

		btn.pressed.connect(action.callback)
		action_container.add_child(btn)

	# 첫 버튼 포커스
	action_container.get_child(0).grab_focus()

func _build_burn_list(root: Control) -> void:
	var scroll = ScrollContainer.new()
	scroll.anchor_left = 0.2
	scroll.anchor_right = 0.8
	scroll.anchor_top = 0.4
	scroll.anchor_bottom = 0.78
	scroll.visible = false

	var panel = PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.06, 0.1, 0.95)
	style.border_color = Color(0.5, 0.3, 0.2, 0.7)
	style.set_border_width_all(1)
	style.set_content_margin_all(10)
	panel.add_theme_stylebox_override("panel", style)
	scroll.add_child(panel)

	burn_list_container = VBoxContainer.new()
	burn_list_container.add_theme_constant_override("separation", 6)
	panel.add_child(burn_list_container)

	root.add_child(scroll)
	# scroll 노드를 burn_list_container의 메타로 저장 (표시/숨김용)
	burn_list_container.set_meta("scroll_parent", scroll)

## 시그널 연결
func _connect_signals() -> void:
	BattleManager.battle_log.connect(_on_battle_log)
	BattleManager.damage_dealt.connect(_on_damage_dealt)
	BattleManager.player_turn_started.connect(_on_player_turn)
	BattleManager.battle_ended.connect(_on_battle_ended)

	if BattleManager.current_enemy:
		_setup_enemy_display()

func _setup_enemy_display() -> void:
	var enemy = BattleManager.current_enemy
	enemy_name_label.text = enemy.name
	enemy_hp_bar.max_value = enemy.max_hp
	enemy_hp_bar.value = enemy.hp

	# 공허수는 다른 색
	if enemy.is_void_beast:
		enemy_sprite.color = Color(0.15, 0.05, 0.2)
		enemy_name_label.add_theme_color_override("font_color", Color(0.6, 0.2, 0.6))

func _update_hp_displays() -> void:
	# 플레이어 HP
	player_hp_bar.max_value = GameManager.player_data.max_hp
	player_hp_bar.value = GameManager.player_data.hp
	player_hp_label.text = "HP: %d / %d" % [GameManager.player_data.hp, GameManager.player_data.max_hp]

	# 적 HP
	if BattleManager.current_enemy:
		var e = BattleManager.current_enemy
		enemy_hp_bar.value = e.hp
		enemy_hp_label.text = "HP: %d / %d" % [e.hp, e.max_hp]

## 전투 로그 표시
func _on_battle_log(message: String) -> void:
	log_lines.append(message)
	if log_lines.size() > MAX_LOG_LINES:
		log_lines = log_lines.slice(-MAX_LOG_LINES)
	log_label.text = "\n".join(log_lines)

func _on_damage_dealt(_target: String, _amount: int, _skill: String) -> void:
	_update_hp_displays()

func _on_player_turn() -> void:
	action_container.visible = true
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

func _on_battle_ended(_result) -> void:
	action_container.visible = false
	_hide_burn_list()

## 행동 콜백
func _on_attack() -> void:
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_attack()

func _on_burn_menu() -> void:
	_toggle_burn_list()

func _on_defend() -> void:
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_defend()

func _on_flee() -> void:
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_flee()

## 기억 연소 목록
func _toggle_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_burn_list()
		return

	# 기존 항목 제거
	for child in burn_list_container.get_children():
		child.queue_free()

	# 사용 가능한 기억 표시
	var available = MemoryManager.get_available_memories()
	if available.is_empty():
		var empty_label = Label.new()
		empty_label.text = "No memories left to burn."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", Color(0.5, 0.4, 0.35))
		burn_list_container.add_child(empty_label)
	else:
		# 타이틀
		var title = Label.new()
		title.text = "Select a memory to burn:"
		title.add_theme_font_size_override("font_size", 12)
		title.add_theme_color_override("font_color", Color(0.7, 0.5, 0.35))
		burn_list_container.add_child(title)

		for memory in available:
			var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
			var btn = Button.new()
			btn.text = "[%s] %s — %s (DMG: %d+%d)" % [
				skill.name, memory.title,
				["Grade 5", "Grade 4", "Grade 3", "Grade 2", "Grade 1"][memory.grade],
				skill.base_damage, memory.burn_power
			]
			btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

			var style = StyleBoxFlat.new()
			style.bg_color = Color(0.1, 0.08, 0.12, 0.8)
			style.set_content_margin_all(6)
			btn.add_theme_stylebox_override("normal", style)
			var hover = style.duplicate()
			hover.bg_color = Color(0.2, 0.12, 0.18, 0.9)
			hover.border_color = Color(0.7, 0.4, 0.3, 0.7)
			hover.set_border_width_all(1)
			btn.add_theme_stylebox_override("hover", hover)
			btn.add_theme_stylebox_override("focus", hover)
			btn.add_theme_font_size_override("font_size", 12)
			btn.add_theme_color_override("font_color", Color(0.7, 0.6, 0.55))
			btn.add_theme_color_override("font_hover_color", Color(0.95, 0.7, 0.4))

			var mid = memory.id
			btn.pressed.connect(func():
				action_container.visible = false
				_hide_burn_list()
				BattleManager.player_burn(mid)
			)
			burn_list_container.add_child(btn)

	# 취소 버튼
	var cancel_btn = Button.new()
	cancel_btn.text = "[ Cancel ]"
	cancel_btn.add_theme_font_size_override("font_size", 12)
	cancel_btn.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	cancel_btn.pressed.connect(_hide_burn_list)
	burn_list_container.add_child(cancel_btn)

	scroll.visible = true

func _hide_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll:
		scroll.visible = false
