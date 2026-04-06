## BattleScene — 턴제 전투 화면
## BattleManager의 시그널을 받아 UI 표시.
## S22: 인트로 연출, 공격/번 VFX, 적 아이들 모션, 턴 표시, 상태 아이콘
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
var enemy_sprite: Control  # 적 스프라이트 (TextureRect 또는 ColorRect)
var enemy_sprite_container: Control  # 아이들 애니메이션용 컨테이너

var log_lines: Array = []
const MAX_LOG_LINES: int = 6
var hp_tween_player: Tween
var hp_tween_enemy: Tween
var canvas_root: Control  # 전투 UI 루트 (셰이크용)
var hit_flash_rect: ColorRect  # 히트 플래시 오버레이

# 전투 인트로 / VFX
var intro_overlay: ColorRect
var turn_label: Label
var enemy_status_container: HBoxContainer
var slash_rect: ColorRect  # 공격 슬래시 VFX
var burn_vfx_container: Control  # 연소 VFX 컨테이너

# 적 아이들 모션
var _idle_time: float = 0.0
var _enemy_base_y: float = 0.0

func _ready() -> void:
	_build_ui()
	_connect_signals()
	# 인트로 연출 후 HP 표시
	_play_intro()

func _process(delta: float) -> void:
	# 적 아이들 모션 (부드러운 상하 흔들림)
	_idle_time += delta
	if enemy_sprite_container and enemy_sprite_container.visible:
		enemy_sprite_container.position.y = _enemy_base_y + sin(_idle_time * 1.5) * 3.0

## ===================== UI 빌드 =====================

func _build_ui() -> void:
	# 배경 (이미지 또는 단색)
	bg = ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.04, 0.04, 0.06)
	add_child(bg)

	if BattleManager.battle_bg_image != "" and ResourceLoader.exists(BattleManager.battle_bg_image):
		var bg_tex = TextureRect.new()
		bg_tex.set_anchors_preset(Control.PRESET_FULL_RECT)
		bg_tex.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		bg_tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		bg_tex.texture = load(BattleManager.battle_bg_image)
		bg_tex.modulate = Color(0.45, 0.4, 0.35, 0.6)
		add_child(bg_tex)

	# 배경 비네트 오버레이
	_add_battle_vignette()

	var canvas = CanvasLayer.new()
	canvas.layer = 10
	add_child(canvas)

	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(root)
	canvas_root = root

	# 히트 플래시 오버레이 (최상단에 나중에 추가)
	hit_flash_rect = ColorRect.new()
	hit_flash_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	hit_flash_rect.color = Color(1, 0.2, 0.15, 0)
	hit_flash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hit_flash_rect.z_index = 100

	# 슬래시 VFX 레이어 (투명 초기)
	slash_rect = ColorRect.new()
	slash_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	slash_rect.color = Color(1, 1, 1, 0)
	slash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	slash_rect.z_index = 50

	# 연소 VFX 컨테이너
	burn_vfx_container = Control.new()
	burn_vfx_container.set_anchors_preset(Control.PRESET_FULL_RECT)
	burn_vfx_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	burn_vfx_container.z_index = 45

	# 적 스프라이트 (화면 중앙 상단)
	_build_enemy_sprite(root)

	# 적 이름 + HP (상단)
	_build_enemy_panel(root)

	# 상태 아이콘 (적 패널 아래)
	_build_enemy_status(root)

	# 전투 로그 (중앙)
	_build_log_panel(root)

	# 플레이어 HP (좌하단)
	_build_player_panel(root)

	# 행동 버튼 (하단)
	_build_action_buttons(root)

	# 기억 연소 목록 (숨김 상태)
	_build_burn_list(root)

	# 턴 표시 라벨
	_build_turn_label(root)

	# VFX 레이어 추가
	root.add_child(burn_vfx_container)
	root.add_child(slash_rect)
	root.add_child(hit_flash_rect)

	# 인트로 오버레이 (최상단)
	_build_intro_overlay(root)

## ===================== 배경 비네트 =====================

func _add_battle_vignette() -> void:
	# 화면 가장자리를 어둡게 — 전투 집중감
	var vignette = ColorRect.new()
	vignette.set_anchors_preset(Control.PRESET_FULL_RECT)
	vignette.color = Color(0, 0, 0, 0)
	vignette.z_index = -1
	add_child(vignette)

	# 상단 그라데이션
	var top = ColorRect.new()
	top.anchor_right = 1.0
	top.offset_bottom = 80
	top.color = Color(0, 0, 0, 0.4)
	top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(top)

	# 하단 그라데이션
	var bottom = ColorRect.new()
	bottom.anchor_right = 1.0
	bottom.anchor_top = 1.0
	bottom.anchor_bottom = 1.0
	bottom.offset_top = -80
	bottom.color = Color(0, 0, 0, 0.4)
	bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(bottom)

## ===================== 인트로 시스템 =====================

func _build_intro_overlay(root: Control) -> void:
	intro_overlay = ColorRect.new()
	intro_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	intro_overlay.color = Color(0, 0, 0, 1.0)
	intro_overlay.z_index = 200
	intro_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(intro_overlay)

func _play_intro() -> void:
	# 액션 버튼 숨김
	action_container.visible = false

	var enemy = BattleManager.current_enemy
	if not enemy:
		_finish_intro()
		return

	# 1단계: 검은 화면에서 적 이름 표시
	var name_display = Label.new()
	name_display.text = enemy.name
	name_display.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	name_display.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	name_display.set_anchors_preset(Control.PRESET_CENTER)
	name_display.offset_left = -300
	name_display.offset_right = 300
	name_display.offset_top = -40
	name_display.offset_bottom = 40
	name_display.add_theme_font_size_override("font_size", 32)
	name_display.modulate.a = 0.0

	# 보스/공허수는 다른 색
	if enemy.is_boss:
		name_display.add_theme_color_override("font_color", Color(0.9, 0.3, 0.2))
	elif enemy.is_void_beast:
		name_display.add_theme_color_override("font_color", Color(0.6, 0.2, 0.7))
	else:
		name_display.add_theme_color_override("font_color", Color(0.85, 0.75, 0.6))

	intro_overlay.add_child(name_display)

	# 부제 (보스/공허수일 때)
	var sub_label = Label.new()
	sub_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	sub_label.set_anchors_preset(Control.PRESET_CENTER)
	sub_label.offset_left = -300
	sub_label.offset_right = 300
	sub_label.offset_top = 30
	sub_label.offset_bottom = 60
	sub_label.add_theme_font_size_override("font_size", 14)
	sub_label.modulate.a = 0.0

	if enemy.is_boss:
		sub_label.text = "— BOSS —"
		sub_label.add_theme_color_override("font_color", Color(0.7, 0.2, 0.15))
	elif enemy.is_void_beast:
		sub_label.text = "Void Beast"
		sub_label.add_theme_color_override("font_color", Color(0.45, 0.15, 0.5))
	else:
		sub_label.text = "Hostile Creature"
		sub_label.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))

	intro_overlay.add_child(sub_label)

	# 구분선 효과 (좌우로 펼쳐지는 선)
	var line_left = ColorRect.new()
	line_left.set_anchors_preset(Control.PRESET_CENTER)
	line_left.offset_left = 0
	line_left.offset_right = 0
	line_left.offset_top = 22
	line_left.offset_bottom = 24
	line_left.color = Color(0.6, 0.4, 0.3, 0.6)
	intro_overlay.add_child(line_left)

	var line_right = ColorRect.new()
	line_right.set_anchors_preset(Control.PRESET_CENTER)
	line_right.offset_left = 0
	line_right.offset_right = 0
	line_right.offset_top = 22
	line_right.offset_bottom = 24
	line_right.color = Color(0.6, 0.4, 0.3, 0.6)
	intro_overlay.add_child(line_right)

	# 애니메이션 시퀀스
	var t = create_tween()

	# 선 펼침
	t.set_parallel(true)
	t.tween_property(line_left, "offset_left", -160, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(line_right, "offset_right", 160, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

	# 이름 페이드 인
	t.tween_property(name_display, "modulate:a", 1.0, 0.3).set_delay(0.15)
	t.tween_property(sub_label, "modulate:a", 0.7, 0.3).set_delay(0.3)

	t.set_parallel(false)

	# 잠시 유지
	t.tween_interval(1.0)

	# 전체 페이드 아웃
	t.tween_property(intro_overlay, "color:a", 0.0, 0.5).set_ease(Tween.EASE_IN)
	t.tween_property(name_display, "modulate:a", 0.0, 0.4)
	t.tween_property(sub_label, "modulate:a", 0.0, 0.4)
	t.tween_callback(_finish_intro)

func _finish_intro() -> void:
	if intro_overlay:
		intro_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
		intro_overlay.visible = false
	_update_hp_displays()
	# 약간의 딜레이 후 플레이어 턴 시작
	await get_tree().create_timer(0.2).timeout
	action_container.visible = true
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

## ===================== 턴 표시 =====================

func _build_turn_label(root: Control) -> void:
	turn_label = Label.new()
	turn_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	turn_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	turn_label.set_anchors_preset(Control.PRESET_CENTER)
	turn_label.offset_left = -200
	turn_label.offset_right = 200
	turn_label.offset_top = -100
	turn_label.offset_bottom = -60
	turn_label.add_theme_font_size_override("font_size", 20)
	turn_label.add_theme_color_override("font_color", Color(0.85, 0.75, 0.55))
	turn_label.modulate.a = 0.0
	turn_label.z_index = 80
	turn_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(turn_label)

func _show_turn_indicator(text: String, color: Color = Color(0.85, 0.75, 0.55)) -> void:
	turn_label.text = text
	turn_label.add_theme_color_override("font_color", color)
	turn_label.modulate.a = 0.0
	turn_label.position = Vector2(0, 0)

	var t = create_tween()
	t.tween_property(turn_label, "modulate:a", 0.9, 0.15)
	t.tween_interval(0.4)
	t.tween_property(turn_label, "modulate:a", 0.0, 0.25)

## ===================== 상태 아이콘 =====================

func _build_enemy_status(root: Control) -> void:
	enemy_status_container = HBoxContainer.new()
	enemy_status_container.anchor_left = 0.55
	enemy_status_container.anchor_right = 0.95
	enemy_status_container.anchor_top = 0.02
	enemy_status_container.anchor_bottom = 0.02
	enemy_status_container.offset_top = 75
	enemy_status_container.offset_bottom = 95
	enemy_status_container.add_theme_constant_override("separation", 6)
	root.add_child(enemy_status_container)

func _update_status_icons() -> void:
	# 기존 아이콘 제거
	for child in enemy_status_container.get_children():
		child.queue_free()

	var enemy = BattleManager.current_enemy
	if not enemy:
		return

	# 방어 상태
	if BattleManager.enemy_shielded:
		_add_status_icon("SHIELD", Color(0.3, 0.5, 0.8, 0.9))

	# 보스 페이즈
	if enemy.is_boss and enemy.phase > 1:
		_add_status_icon("PHASE %d" % enemy.phase, Color(0.8, 0.3, 0.2, 0.9))

	# 공허수 표시
	if enemy.is_void_beast:
		_add_status_icon("VOID", Color(0.5, 0.15, 0.6, 0.9))

func _add_status_icon(text: String, color: Color) -> void:
	var label = Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", 10)
	label.add_theme_color_override("font_color", color)

	var panel = PanelContainer.new()
	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r, color.g, color.b, 0.15)
	style.border_color = Color(color.r, color.g, color.b, 0.4)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(3)
	panel.add_theme_stylebox_override("panel", style)
	panel.add_child(label)
	enemy_status_container.add_child(panel)

## ===================== UI 패널 빌드 =====================

func _build_enemy_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.55
	panel.anchor_right = 0.95
	panel.anchor_top = 0.02
	panel.anchor_bottom = 0.02
	panel.offset_bottom = 70

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.06, 0.06, 0.9)
	style.border_color = Color(0.5, 0.2, 0.2, 0.5)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	panel.add_child(vbox)

	enemy_name_label = Label.new()
	enemy_name_label.add_theme_font_size_override("font_size", 15)
	enemy_name_label.add_theme_color_override("font_color", Color(0.85, 0.4, 0.35))
	vbox.add_child(enemy_name_label)

	enemy_hp_bar = ProgressBar.new()
	enemy_hp_bar.custom_minimum_size = Vector2(0, 18)
	enemy_hp_bar.show_percentage = false
	var bar_style = StyleBoxFlat.new()
	bar_style.bg_color = Color(0.6, 0.15, 0.15)
	bar_style.set_corner_radius_all(3)
	enemy_hp_bar.add_theme_stylebox_override("fill", bar_style)
	var bar_bg = StyleBoxFlat.new()
	bar_bg.bg_color = Color(0.12, 0.08, 0.08)
	bar_bg.set_corner_radius_all(3)
	enemy_hp_bar.add_theme_stylebox_override("background", bar_bg)
	vbox.add_child(enemy_hp_bar)

	enemy_hp_label = Label.new()
	enemy_hp_label.add_theme_font_size_override("font_size", 11)
	enemy_hp_label.add_theme_color_override("font_color", Color(0.6, 0.35, 0.3))
	enemy_hp_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(enemy_hp_label)

func _build_enemy_sprite(root: Control) -> void:
	# 적 스프라이트 컨테이너 (아이들 모션용)
	enemy_sprite_container = Control.new()
	enemy_sprite_container.anchor_left = 0.2
	enemy_sprite_container.anchor_right = 0.8
	enemy_sprite_container.anchor_top = 0.06
	enemy_sprite_container.anchor_bottom = 0.48
	root.add_child(enemy_sprite_container)
	_enemy_base_y = 0.0

	if BattleManager.enemy_image != "" and ResourceLoader.exists(BattleManager.enemy_image):
		var tex_rect = TextureRect.new()
		tex_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = load(BattleManager.enemy_image)
		enemy_sprite_container.add_child(tex_rect)
		enemy_sprite = tex_rect
	else:
		var rect = ColorRect.new()
		rect.set_anchors_preset(Control.PRESET_CENTER)
		rect.offset_left = -80
		rect.offset_right = 80
		rect.offset_top = -60
		rect.offset_bottom = 60
		rect.color = Color(0.25, 0.08, 0.12)
		enemy_sprite_container.add_child(rect)

		# 눈 (빛나는 효과)
		var eye_l = ColorRect.new()
		eye_l.size = Vector2(10, 10)
		eye_l.position = Vector2(30, 35)
		eye_l.color = Color(0.95, 0.2, 0.15)
		rect.add_child(eye_l)

		var eye_r = ColorRect.new()
		eye_r.size = Vector2(10, 10)
		eye_r.position = Vector2(120, 35)
		eye_r.color = Color(0.95, 0.2, 0.15)
		rect.add_child(eye_r)

		enemy_sprite = rect

func _build_log_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.05
	panel.anchor_right = 0.95
	panel.anchor_top = 0.48
	panel.anchor_bottom = 0.65

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.04, 0.06, 0.85)
	style.set_content_margin_all(10)
	style.set_corner_radius_all(4)
	style.border_color = Color(0.2, 0.18, 0.15, 0.3)
	style.set_border_width_all(1)
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
	style.bg_color = Color(0.06, 0.06, 0.1, 0.9)
	style.border_color = Color(0.2, 0.3, 0.5, 0.5)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	panel.add_child(vbox)

	var name_label = Label.new()
	name_label.text = "Arrel"
	name_label.add_theme_font_size_override("font_size", 15)
	name_label.add_theme_color_override("font_color", Color(0.45, 0.55, 0.75))
	vbox.add_child(name_label)

	player_hp_bar = ProgressBar.new()
	player_hp_bar.custom_minimum_size = Vector2(0, 18)
	player_hp_bar.show_percentage = false
	var fill = StyleBoxFlat.new()
	fill.bg_color = Color(0.2, 0.45, 0.6)
	fill.set_corner_radius_all(3)
	player_hp_bar.add_theme_stylebox_override("fill", fill)
	var bg_s = StyleBoxFlat.new()
	bg_s.bg_color = Color(0.08, 0.08, 0.12)
	bg_s.set_corner_radius_all(3)
	player_hp_bar.add_theme_stylebox_override("background", bg_s)
	vbox.add_child(player_hp_bar)

	player_hp_label = Label.new()
	player_hp_label.add_theme_font_size_override("font_size", 11)
	player_hp_label.add_theme_color_override("font_color", Color(0.35, 0.45, 0.6))
	vbox.add_child(player_hp_label)

func _build_action_buttons(root: Control) -> void:
	action_container = HBoxContainer.new()
	action_container.anchor_left = 0.1
	action_container.anchor_right = 0.9
	action_container.anchor_top = 0.82
	action_container.anchor_bottom = 0.82
	action_container.offset_bottom = 54
	action_container.add_theme_constant_override("separation", 10)
	action_container.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(action_container)

	var actions = [
		{"text": "ATTACK", "callback": _on_attack, "icon": "⚔"},
		{"text": "BURN", "callback": _on_burn_menu, "icon": "🔥"},
		{"text": "DEFEND", "callback": _on_defend, "icon": "🛡"},
		{"text": "FLEE", "callback": _on_flee, "icon": "💨"},
	]

	for action in actions:
		var btn = Button.new()
		btn.text = action.text
		btn.custom_minimum_size = Vector2(130, 44)

		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.1, 0.08, 0.12, 0.92)
		style.border_color = Color(0.35, 0.28, 0.22, 0.5)
		style.set_border_width_all(1)
		style.set_corner_radius_all(4)
		style.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", style)

		var hover = style.duplicate()
		hover.bg_color = Color(0.18, 0.14, 0.22, 0.95)
		hover.border_color = Color(0.75, 0.55, 0.3, 0.8)
		hover.set_border_width_all(2)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_stylebox_override("focus", hover)

		var pressed_s = style.duplicate()
		pressed_s.bg_color = Color(0.25, 0.18, 0.3, 1.0)
		pressed_s.border_color = Color(0.9, 0.65, 0.35, 1.0)
		btn.add_theme_stylebox_override("pressed", pressed_s)

		btn.add_theme_font_size_override("font_size", 14)
		btn.add_theme_color_override("font_color", Color(0.75, 0.7, 0.65))
		btn.add_theme_color_override("font_hover_color", Color(0.95, 0.8, 0.5))
		btn.add_theme_color_override("font_pressed_color", Color(1.0, 0.9, 0.6))

		btn.pressed.connect(action.callback)
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		action_container.add_child(btn)

func _build_burn_list(root: Control) -> void:
	var scroll = ScrollContainer.new()
	scroll.anchor_left = 0.15
	scroll.anchor_right = 0.85
	scroll.anchor_top = 0.35
	scroll.anchor_bottom = 0.78
	scroll.visible = false

	var panel = PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.04, 0.08, 0.96)
	style.border_color = Color(0.5, 0.3, 0.2, 0.7)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(12)
	panel.add_theme_stylebox_override("panel", style)
	scroll.add_child(panel)

	burn_list_container = VBoxContainer.new()
	burn_list_container.add_theme_constant_override("separation", 6)
	panel.add_child(burn_list_container)

	root.add_child(scroll)
	burn_list_container.set_meta("scroll_parent", scroll)

## ===================== 시그널 연결 =====================

func _connect_signals() -> void:
	BattleManager.battle_log.connect(_on_battle_log)
	BattleManager.damage_dealt.connect(_on_damage_dealt)
	BattleManager.player_turn_started.connect(_on_player_turn)
	BattleManager.enemy_turn_started.connect(_on_enemy_turn)
	BattleManager.battle_ended.connect(_on_battle_ended)

	if BattleManager.current_enemy:
		_setup_enemy_display()

func _setup_enemy_display() -> void:
	var enemy = BattleManager.current_enemy
	enemy_name_label.text = enemy.name
	enemy_hp_bar.max_value = enemy.max_hp
	enemy_hp_bar.value = enemy.hp

	if enemy.is_void_beast:
		enemy_name_label.add_theme_color_override("font_color", Color(0.65, 0.25, 0.65))
	if enemy.is_boss:
		enemy_name_label.add_theme_color_override("font_color", Color(0.9, 0.35, 0.25))

func _update_hp_displays(animate: bool = false) -> void:
	# 플레이어 HP
	player_hp_bar.max_value = GameManager.player_data.max_hp
	var p_hp = GameManager.player_data.hp
	player_hp_label.text = "HP: %d / %d" % [p_hp, GameManager.player_data.max_hp]

	var p_ratio = float(p_hp) / max(GameManager.player_data.max_hp, 1)
	var p_fill = player_hp_bar.get_theme_stylebox("fill") as StyleBoxFlat
	if p_fill:
		p_fill.bg_color = UITheme.HP_LOW if p_ratio <= 0.25 else UITheme.HP_PLAYER

	if animate:
		if hp_tween_player:
			hp_tween_player.kill()
		hp_tween_player = create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		hp_tween_player.tween_property(player_hp_bar, "value", float(p_hp), 0.4)
	else:
		player_hp_bar.value = p_hp

	# 적 HP
	if BattleManager.current_enemy:
		var e = BattleManager.current_enemy
		enemy_hp_label.text = "HP: %d / %d" % [e.hp, e.max_hp]

		var e_ratio = float(e.hp) / max(e.max_hp, 1)
		var e_fill = enemy_hp_bar.get_theme_stylebox("fill") as StyleBoxFlat
		if e_fill:
			e_fill.bg_color = UITheme.HP_LOW if e_ratio <= 0.25 else UITheme.HP_ENEMY

		if animate:
			if hp_tween_enemy:
				hp_tween_enemy.kill()
			hp_tween_enemy = create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
			hp_tween_enemy.tween_property(enemy_hp_bar, "value", float(e.hp), 0.4)
		else:
			enemy_hp_bar.value = e.hp

	# 상태 아이콘 업데이트
	_update_status_icons()

## ===================== 전투 로그 =====================

func _on_battle_log(message: String) -> void:
	log_lines.append(message)
	if log_lines.size() > MAX_LOG_LINES:
		log_lines = log_lines.slice(-MAX_LOG_LINES)
	log_label.text = "\n".join(log_lines)

func _on_damage_dealt(target: String, amount: int, skill_name: String) -> void:
	_update_hp_displays(true)
	_show_damage_number(target, amount)
	_hit_flash(target)
	_screen_shake()

	# 스킬별 VFX
	if skill_name != "" and target != "Arrel":
		_play_attack_vfx(skill_name)
	elif target != "Arrel":
		_play_slash_vfx()

func _on_player_turn() -> void:
	_show_turn_indicator("— YOUR TURN —", Color(0.5, 0.65, 0.85))
	await get_tree().create_timer(0.5).timeout
	action_container.visible = true
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

func _on_enemy_turn() -> void:
	_show_turn_indicator("— ENEMY TURN —", Color(0.8, 0.4, 0.35))

func _on_battle_ended(_result) -> void:
	action_container.visible = false
	_hide_burn_list()

## ===================== 행동 콜백 =====================

func _on_attack() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_attack()

func _on_burn_menu() -> void:
	AudioManager.play_sfx("ui_select")
	_toggle_burn_list()

func _on_defend() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_defend()

func _on_flee() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	BattleManager.player_flee()

## ===================== 기억 연소 목록 =====================

func _toggle_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_burn_list()
		return

	for child in burn_list_container.get_children():
		child.queue_free()

	var available = MemoryManager.get_available_memories()
	if available.is_empty():
		var empty_label = Label.new()
		empty_label.text = "No memories left to burn."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", Color(0.5, 0.4, 0.35))
		burn_list_container.add_child(empty_label)
	else:
		var title = Label.new()
		title.text = "— Select a memory to burn —"
		title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		title.add_theme_font_size_override("font_size", 13)
		title.add_theme_color_override("font_color", Color(0.75, 0.5, 0.35))
		burn_list_container.add_child(title)

		for memory in available:
			var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
			var btn = Button.new()
			btn.text = "[%s] %s — Grade %d (DMG: %d+%d)" % [
				skill.name, memory.title,
				memory.grade,
				skill.base_damage, memory.burn_power
			]
			btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

			var style = StyleBoxFlat.new()
			style.bg_color = Color(0.08, 0.06, 0.1, 0.85)
			style.set_content_margin_all(8)
			style.set_corner_radius_all(3)
			btn.add_theme_stylebox_override("normal", style)
			var hover_s = style.duplicate()
			hover_s.bg_color = Color(0.18, 0.1, 0.16, 0.95)
			hover_s.border_color = Color(0.7, 0.4, 0.3, 0.7)
			hover_s.set_border_width_all(1)
			btn.add_theme_stylebox_override("hover", hover_s)
			btn.add_theme_stylebox_override("focus", hover_s)
			btn.add_theme_font_size_override("font_size", 12)
			btn.add_theme_color_override("font_color", Color(0.7, 0.6, 0.55))
			btn.add_theme_color_override("font_hover_color", Color(0.95, 0.7, 0.4))

			var mid = memory.id
			btn.pressed.connect(func():
				AudioManager.play_sfx("ui_select")
				action_container.visible = false
				_hide_burn_list()
				BattleManager.player_burn(mid)
			)
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
			burn_list_container.add_child(btn)

	var cancel_btn = Button.new()
	cancel_btn.text = "[ Cancel ]"
	cancel_btn.add_theme_font_size_override("font_size", 12)
	cancel_btn.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	cancel_btn.pressed.connect(func():
		AudioManager.play_sfx("cancel")
		_hide_burn_list()
	)
	burn_list_container.add_child(cancel_btn)

	scroll.visible = true

func _hide_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll:
		scroll.visible = false

## ===================== 시각 피드백 =====================

## 데미지 숫자 표시 (떠오르며 사라짐 — 크기 스케일링)
func _show_damage_number(target: String, amount: int) -> void:
	var label = Label.new()
	label.text = str(amount)
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER

	# 데미지 크기에 따른 폰트 스케일
	var font_size = 22
	if amount >= 100:
		font_size = 30
	elif amount >= 50:
		font_size = 26
	label.add_theme_font_size_override("font_size", font_size)

	if target == "Arrel":
		label.add_theme_color_override("font_color", Color(1.0, 0.3, 0.25))
		label.position = Vector2(200 + randf_range(-20, 20), 500)
	else:
		label.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4))
		label.position = Vector2(600 + randf_range(-30, 30), 180 + randf_range(-10, 10))

	# 드롭 섀도우 효과
	var shadow = Label.new()
	shadow.text = str(amount)
	shadow.add_theme_font_size_override("font_size", font_size)
	shadow.add_theme_color_override("font_color", Color(0, 0, 0, 0.6))
	shadow.position = Vector2(2, 2)
	label.add_child(shadow)

	canvas_root.add_child(label)

	# 떠오르며 사라지는 애니메이션 + 스케일 펀치
	label.scale = Vector2(1.3, 1.3)
	var t = create_tween().set_parallel(true)
	t.tween_property(label, "position:y", label.position.y - 50, 0.9).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(label, "scale", Vector2(1.0, 1.0), 0.2).set_ease(Tween.EASE_OUT)
	t.tween_property(label, "modulate:a", 0.0, 0.5).set_delay(0.4)
	t.chain().tween_callback(label.queue_free)

## 히트 플래시 (적 피격 = 흰색, 플레이어 피격 = 빨간색)
func _hit_flash(target: String) -> void:
	if target == "Arrel":
		hit_flash_rect.color = Color(1, 0.15, 0.1, 0.3)
	else:
		hit_flash_rect.color = Color(1, 1, 1, 0.25)

	var t = create_tween()
	t.tween_property(hit_flash_rect, "color:a", 0.0, 0.2)

	# 적 스프라이트 깜빡임
	if target != "Arrel" and enemy_sprite:
		var flash_t = create_tween()
		flash_t.tween_property(enemy_sprite, "modulate", Color(3, 3, 3, 1), 0.05)
		flash_t.tween_property(enemy_sprite, "modulate", Color(1, 1, 1, 1), 0.15)

## 스크린 셰이크 (짧은 흔들림 — 개선)
func _screen_shake(intensity: float = 1.0) -> void:
	var original_pos = canvas_root.position
	var t = create_tween()
	for i in range(5):
		var decay = 1.0 - float(i) / 5.0
		var offset = Vector2(
			randf_range(-5, 5) * intensity * decay,
			randf_range(-4, 4) * intensity * decay
		)
		t.tween_property(canvas_root, "position", original_pos + offset, 0.035)
	t.tween_property(canvas_root, "position", original_pos, 0.035)

## ===================== 공격 VFX =====================

## 물리 공격 슬래시 이펙트
func _play_slash_vfx() -> void:
	# 대각선 슬래시 라인 표시
	var slash = ColorRect.new()
	slash.size = Vector2(200, 4)
	slash.position = Vector2(450, 150)
	slash.rotation = -0.6
	slash.color = Color(1, 1, 1, 0.8)
	slash.z_index = 60
	canvas_root.add_child(slash)

	var t = create_tween()
	t.tween_property(slash, "modulate:a", 0.0, 0.25).set_ease(Tween.EASE_IN)
	t.tween_callback(slash.queue_free)

	# 두 번째 슬래시 (약간 지연)
	var slash2 = ColorRect.new()
	slash2.size = Vector2(180, 3)
	slash2.position = Vector2(470, 180)
	slash2.rotation = 0.4
	slash2.color = Color(1, 0.9, 0.7, 0.6)
	slash2.z_index = 60
	canvas_root.add_child(slash2)

	var t2 = create_tween()
	t2.tween_interval(0.08)
	t2.tween_property(slash2, "modulate:a", 0.0, 0.2).set_ease(Tween.EASE_IN)
	t2.tween_callback(slash2.queue_free)

## 기억 연소 VFX — 불꽃 파티클
func _play_attack_vfx(skill_name: String) -> void:
	# 연소 스킬일 때 불꽃 VFX
	var is_burn = skill_name.to_lower().find("burn") >= 0 or skill_name.to_lower().find("flame") >= 0 or skill_name.to_lower().find("ember") >= 0 or skill_name.to_lower().find("pyre") >= 0 or skill_name.to_lower().find("incinerate") >= 0

	if is_burn or true:  # 모든 스킬에 VFX 적용
		_play_burn_vfx()

## 불꽃 VFX (여러 파티클)
func _play_burn_vfx() -> void:
	var center = Vector2(580, 200)

	# 여러 불꽃 입자 생성
	for i in range(12):
		var particle = ColorRect.new()
		particle.size = Vector2(randf_range(4, 10), randf_range(4, 10))
		particle.position = center + Vector2(randf_range(-40, 40), randf_range(-30, 30))
		particle.z_index = 55

		# 불 색상 (주황~빨강~노랑)
		var fire_colors = [
			Color(1.0, 0.6, 0.1, 0.9),
			Color(1.0, 0.35, 0.1, 0.85),
			Color(1.0, 0.85, 0.3, 0.8),
			Color(0.9, 0.2, 0.05, 0.7),
		]
		particle.color = fire_colors[randi_range(0, fire_colors.size() - 1)]

		canvas_root.add_child(particle)

		# 위로 떠오르며 사라짐
		var delay = randf_range(0, 0.15)
		var t = create_tween().set_parallel(true)
		t.tween_property(particle, "position:y", particle.position.y - randf_range(30, 80), randf_range(0.4, 0.8)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(particle, "position:x", particle.position.x + randf_range(-20, 20), randf_range(0.4, 0.8)).set_delay(delay)
		t.tween_property(particle, "modulate:a", 0.0, randf_range(0.3, 0.6)).set_delay(delay + 0.2)
		t.tween_property(particle, "size", Vector2(1, 1), 0.6).set_delay(delay)
		t.chain().tween_callback(particle.queue_free)

	# 중앙 플래시
	var flash = ColorRect.new()
	flash.size = Vector2(100, 100)
	flash.position = center - Vector2(50, 50)
	flash.color = Color(1, 0.7, 0.2, 0.4)
	flash.z_index = 54
	canvas_root.add_child(flash)

	var ft = create_tween()
	ft.tween_property(flash, "modulate:a", 0.0, 0.35)
	ft.tween_callback(flash.queue_free)
