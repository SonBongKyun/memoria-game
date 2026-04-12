## BattleScene — 턴제 전투 화면 (S44: 사이드뷰 오버홀)
## BattleManager의 시그널을 받아 UI 표시.
## S44: 사이드뷰 레이아웃, 캐릭터/적 128x128 스프라이트, 전투 애니메이션
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
var item_list_container: VBoxContainer
var enemy_sprite: Control  # 적 스프라이트
var enemy_sprite_container: Control  # 적 아이들 모션용 컨테이너

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
var player_status_container: HBoxContainer
var slash_rect: ColorRect  # 공격 슬래시 VFX
var burn_vfx_container: Control  # 연소 VFX 컨테이너

# Limit Break UI
var limit_bar: ProgressBar
var limit_label: Label
var limit_btn: Button

# S44: 사이드뷰 캐릭터 스프라이트
var player_sprite: Control  # 아렐 스프라이트
var player_sprite_container: Control  # 아이들 모션용
var ally_sprite: Control  # 동행자 스프라이트 (엘리아/세이블)
var ally_sprite_container: Control
var _player_base_pos: Vector2 = Vector2.ZERO  # 돌진 복귀용
var _enemy_base_pos: Vector2 = Vector2.ZERO
var _ally_base_pos: Vector2 = Vector2.ZERO
var player_shadow: ColorRect
var enemy_shadow: ColorRect
var ally_shadow: ColorRect
var _ground_rect: ColorRect  # 전투 지면
var player_portrait_rect: TextureRect  # HP 옆 포트레이트

# 적 아이들 모션
var _idle_time: float = 0.0
var _enemy_base_y: float = 0.0

# S42: 전투 분위기 컬러 그레이딩
var _color_grade_rect: ColorRect
var _battle_particles: GPUParticles2D  # 배경 파티클
var _battle_parallax_layers: Array = []  # S53: 전투 패럴랙스

# S46: 타격감 강화
var _enemy_shader_mat: ShaderMaterial  # 적 VFX 셰이더
var _player_shader_mat: ShaderMaterial  # 플레이어 VFX 셰이더
var ally_cmd_container: HBoxContainer  # 세이블 명령 UI
var tobias_cmd_container: HBoxContainer  # 토비아스 명령 UI

func _ready() -> void:
	_build_ui()
	_connect_signals()
	# 인트로 연출 후 HP 표시
	_play_intro()

func _process(delta: float) -> void:
	_idle_time += delta
	# 적 아이들 모션 (호흡 — 상하 + 미세 스케일)
	if enemy_sprite_container and enemy_sprite_container.visible:
		enemy_sprite_container.position.y = _enemy_base_pos.y + sin(_idle_time * 1.5) * 3.0
		enemy_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.5) * 0.008, 1.0 - sin(_idle_time * 1.5) * 0.006)
	# 플레이어 아이들 모션 (호흡 + 미세 스케일)
	if player_sprite_container:
		player_sprite_container.position.y = _player_base_pos.y + sin(_idle_time * 1.8 + 0.5) * 2.0
		player_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.8 + 0.5) * 0.006, 1.0 - sin(_idle_time * 1.8 + 0.5) * 0.005)
	# 동행자 아이들
	if ally_sprite_container and ally_sprite_container.visible:
		ally_sprite_container.position.y = _ally_base_pos.y + sin(_idle_time * 1.3 + 1.2) * 2.5
		ally_sprite_container.scale = Vector2(1.0 + sin(_idle_time * 1.3 + 1.2) * 0.007, 1.0 - sin(_idle_time * 1.3 + 1.2) * 0.005)
	# S53: 전투 패럴랙스 미세 이동
	for layer in _battle_parallax_layers:
		if layer and is_instance_valid(layer):
			var speed = layer.get_meta("parallax_speed", 0.5)
			layer.position.x = sin(_idle_time * speed * 0.3) * 15 * speed

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
	# S42: 배경 분위기 파티클 + 컬러 그레이딩
	_add_battle_atmosphere()
	# S53: 전투 패럴랙스 레이어
	_add_battle_parallax()

	# S44: 전투 지면 (그라운드 플랫폼)
	_build_battle_ground()

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

	# S44: 사이드뷰 — 플레이어 스프라이트 (왼쪽)
	_build_player_sprite(root)

	# S44: 사이드뷰 — 동행자 스프라이트 (왼쪽, 플레이어 뒤)
	_build_ally_sprite(root)

	# S44: 적 스프라이트 (오른쪽) — 128x128 대형
	_build_enemy_sprite(root)

	# 적 이름 + HP (상단 오른쪽)
	_build_enemy_panel(root)

	# 상태 아이콘 (적 패널 아래)
	_build_enemy_status(root)

	# 전투 로그 (중앙)
	_build_log_panel(root)

	# 플레이어 HP (좌하단)
	_build_player_panel(root)

	# 플레이어 상태 아이콘 (플레이어 패널 아래)
	_build_player_status(root)

	# Limit Break 게이지 (플레이어 패널 우측)
	_build_limit_gauge(root)

	# 행동 버튼 (하단)
	_build_action_buttons(root)

	# 기억 연소 목록 (숨김 상태)
	_build_burn_list(root)

	# 아이템 목록 (숨김 상태)
	_build_item_list(root)

	# 턴 표시 라벨
	_build_turn_label(root)

	# S41: 턴 순서 미리보기
	_build_turn_preview(root)

	# S46: 세이블 명령 UI
	_build_ally_command_ui(root)
	# S53: 토비아스 명령 UI
	_build_tobias_command_ui(root)

	# S51: 스탠스 전환 UI + 에코 표시 + 엘리아 기술 UI
	_build_stance_ui(root)
	_build_echo_display(root)
	_build_elia_skill_ui(root)

	# VFX 레이어 추가
	root.add_child(burn_vfx_container)
	root.add_child(slash_rect)
	root.add_child(hit_flash_rect)

	# 인트로 오버레이 (최상단)
	_build_intro_overlay(root)

## ===================== 배경 비네트 =====================

func _add_battle_vignette() -> void:
	# 셰이더 기반 원형 비네트 (S40)
	var vignette = ColorRect.new()
	vignette.set_anchors_preset(Control.PRESET_FULL_RECT)
	vignette.color = Color(0, 0, 0, 0)  # 셰이더가 알파를 제어
	vignette.z_index = -1
	vignette.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var shader_path = "res://assets/shaders/vignette.gdshader"
	if ResourceLoader.exists(shader_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(shader_path)
		mat.set_shader_parameter("intensity", 0.6)
		mat.set_shader_parameter("outer_radius", 0.9)
		mat.set_shader_parameter("inner_radius", 0.3)
		vignette.material = mat
	add_child(vignette)

## S53: 전투 배경 패럴랙스 레이어
func _add_battle_parallax() -> void:
	# Layer 1: 먼 실루엣 (느린 이동)
	var far_layer = ColorRect.new()
	far_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	far_layer.color = Color(0.05, 0.03, 0.08, 0.3)
	far_layer.z_index = -2
	far_layer.set_meta("parallax_speed", 0.3)
	far_layer.set_meta("base_x", 0.0)
	far_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(far_layer)
	move_child(far_layer, 0)

	# Layer 2: 안개/먼지 (중간 이동)
	var mid_layer = ColorRect.new()
	mid_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	mid_layer.color = Color(0.1, 0.08, 0.12, 0.15)
	mid_layer.z_index = -1
	mid_layer.set_meta("parallax_speed", 0.8)
	mid_layer.set_meta("base_x", 0.0)
	mid_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(mid_layer)
	move_child(mid_layer, 1)

	_battle_parallax_layers = [far_layer, mid_layer]

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

## ===================== S41: 상태이상 비주얼 (적 스프라이트 틴트) =====================

var _status_tween: Tween
var _status_overlay: ColorRect  # 상태이상 오버레이 (적 스프라이트 위)

func _update_enemy_status_visual() -> void:
	if not enemy_sprite:
		return
	# 상태이상에 따라 적 스프라이트에 시각적 틴트 적용
	var has_poison = false
	var has_burn = false
	var has_weaken = false
	for entry in BattleManager.get_statuses("enemy"):
		if entry.effect == BattleManager.StatusEffect.POISON:
			has_poison = true
		elif entry.effect == BattleManager.StatusEffect.BURN:
			has_burn = true
		elif entry.effect == BattleManager.StatusEffect.WEAKEN:
			has_weaken = true

	if _status_tween and _status_tween.is_running():
		_status_tween.kill()

	if has_poison:
		# 독: 초록 틴트 맥동
		_status_tween = create_tween().set_loops()
		_status_tween.tween_property(enemy_sprite, "modulate", Color(0.6, 1.2, 0.6, 1.0), 0.5)
		_status_tween.tween_property(enemy_sprite, "modulate", Color(0.8, 1.0, 0.8, 1.0), 0.5)
	elif has_burn:
		# 화상: 주황 깜빡임
		_status_tween = create_tween().set_loops()
		_status_tween.tween_property(enemy_sprite, "modulate", Color(1.3, 0.7, 0.4, 1.0), 0.3)
		_status_tween.tween_property(enemy_sprite, "modulate", Color(1.0, 0.85, 0.7, 1.0), 0.4)
	elif has_weaken:
		# 약화: 파란 톤
		enemy_sprite.modulate = Color(0.7, 0.7, 1.2, 1.0)
	else:
		enemy_sprite.modulate = Color(1, 1, 1, 1)

## ===================== S41: 콤보 버스트 VFX =====================

func _play_combo_burst(combo: int) -> void:
	if combo < 2:
		return
	var center = Vector2(400, 200)
	# 콤보 텍스트 (큰 금색)
	var lbl = Label.new()
	lbl.text = "COMBO x%d!" % combo
	lbl.add_theme_font_size_override("font_size", 22 + combo * 2)
	lbl.add_theme_color_override("font_color", Color(1.0, 0.85, 0.2))
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.position = center - Vector2(80, 20)
	lbl.z_index = 70
	lbl.modulate.a = 0.0
	lbl.scale = Vector2(0.5, 0.5)
	lbl.pivot_offset = Vector2(80, 20)
	canvas_root.add_child(lbl)

	var lt = create_tween().set_parallel(true)
	lt.tween_property(lbl, "modulate:a", 1.0, 0.1)
	lt.tween_property(lbl, "scale", Vector2(1.2, 1.2), 0.15).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	lt.chain().tween_property(lbl, "scale", Vector2(1.0, 1.0), 0.1)
	lt.chain().tween_interval(0.4)
	lt.chain().tween_property(lbl, "modulate:a", 0.0, 0.3)
	lt.chain().tween_callback(lbl.queue_free)

	# 파티클 버스트 (금색 방사형)
	for i in range(8 + combo * 2):
		var spark = ColorRect.new()
		spark.size = Vector2(4, 4)
		spark.position = center
		spark.color = Color(1.0, 0.8 + randf() * 0.2, 0.2, 0.9)
		spark.z_index = 68
		canvas_root.add_child(spark)
		var angle = randf() * TAU
		var dist = randf_range(40, 100 + combo * 15)
		var target_pos = center + Vector2(cos(angle), sin(angle)) * dist
		var st = create_tween().set_parallel(true)
		st.tween_property(spark, "position", target_pos, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		st.tween_property(spark, "modulate:a", 0.0, 0.3).set_delay(0.15)
		st.chain().tween_callback(spark.queue_free)

## ===================== S41: 턴 순서 미리보기 =====================

var turn_preview_container: HBoxContainer

func _build_turn_preview(root: Control) -> void:
	turn_preview_container = HBoxContainer.new()
	turn_preview_container.anchor_left = 0.3
	turn_preview_container.anchor_right = 0.7
	turn_preview_container.anchor_top = 0.0
	turn_preview_container.offset_top = 4
	turn_preview_container.offset_bottom = 24
	turn_preview_container.alignment = BoxContainer.ALIGNMENT_CENTER
	turn_preview_container.add_theme_constant_override("separation", 8)
	turn_preview_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(turn_preview_container)

func _update_turn_preview() -> void:
	if not turn_preview_container:
		return
	for child in turn_preview_container.get_children():
		child.queue_free()

	# 현재 턴 + 다음 2턴 예측
	var turns: Array = []
	if BattleManager.state == BattleManager.BattleState.PLAYER_TURN:
		turns = ["PLAYER", "ENEMY", "PLAYER"]
	elif BattleManager.state == BattleManager.BattleState.ENEMY_TURN:
		turns = ["ENEMY", "PLAYER", "ENEMY"]
	else:
		return

	for i in range(turns.size()):
		var is_current = (i == 0)
		var lbl = Label.new()
		lbl.text = turns[i]
		lbl.add_theme_font_size_override("font_size", 9 if not is_current else 11)
		var col = Color(0.5, 0.65, 0.85) if turns[i] == "PLAYER" else Color(0.8, 0.4, 0.35)
		if not is_current:
			col = col.darkened(0.4)
		lbl.add_theme_color_override("font_color", col)
		lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE

		var panel = PanelContainer.new()
		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.05, 0.04, 0.07, 0.7 if is_current else 0.4)
		style.border_color = col * Color(1, 1, 1, 0.5 if is_current else 0.2)
		style.set_border_width_all(1)
		style.set_corner_radius_all(2)
		style.set_content_margin_all(3)
		panel.add_theme_stylebox_override("panel", style)
		panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
		panel.add_child(lbl)
		turn_preview_container.add_child(panel)

		if is_current:
			# 현재 턴 표시자에 화살표
			var arrow = Label.new()
			arrow.text = ">"
			arrow.add_theme_font_size_override("font_size", 9)
			arrow.add_theme_color_override("font_color", col.darkened(0.2))
			arrow.mouse_filter = Control.MOUSE_FILTER_IGNORE
			turn_preview_container.add_child(arrow)

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

func _build_player_status(root: Control) -> void:
	player_status_container = HBoxContainer.new()
	player_status_container.anchor_left = 0.05
	player_status_container.anchor_right = 0.4
	player_status_container.anchor_top = 0.68
	player_status_container.anchor_bottom = 0.68
	player_status_container.offset_top = 74
	player_status_container.offset_bottom = 94
	player_status_container.add_theme_constant_override("separation", 6)
	root.add_child(player_status_container)

func _update_status_icons() -> void:
	# 적 상태 아이콘
	for child in enemy_status_container.get_children():
		child.queue_free()

	var enemy = BattleManager.current_enemy
	if enemy:
		if BattleManager.enemy_shielded:
			_add_status_icon(enemy_status_container, "SHIELD", Color(0.3, 0.5, 0.8, 0.9))
		if enemy.is_boss and enemy.phase > 1:
			_add_status_icon(enemy_status_container, "PHASE %d" % enemy.phase, Color(0.8, 0.3, 0.2, 0.9))
		if enemy.is_void_beast:
			_add_status_icon(enemy_status_container, "VOID", Color(0.5, 0.15, 0.6, 0.9))
		# 약점/저항 표시
		if enemy.weakness != "":
			_add_status_icon(enemy_status_container, "WEAK:%s" % enemy.weakness.to_upper(), Color(0.2, 0.8, 0.3, 0.8))
		if enemy.resistance != "":
			_add_status_icon(enemy_status_container, "RESIST:%s" % enemy.resistance.to_upper(), Color(0.8, 0.4, 0.2, 0.8))
		# 적 상태이상
		for entry in BattleManager.get_statuses("enemy"):
			var info = _get_status_display(entry.effect)
			_add_status_icon(enemy_status_container, "%s %d" % [info.text, entry.turns_left], info.color)

	# 플레이어 상태 아이콘
	for child in player_status_container.get_children():
		child.queue_free()

	for entry in BattleManager.get_statuses("player"):
		var info = _get_status_display(entry.effect)
		_add_status_icon(player_status_container, "%s %d" % [info.text, entry.turns_left], info.color)

	# 콤보 표시
	if BattleManager.combo_count >= 2:
		_add_status_icon(player_status_container, "COMBO x%d" % BattleManager.combo_count, Color(0.9, 0.7, 0.2, 0.9))

	# 세이블 동행 표시
	if BattleManager.sable_in_party:
		_add_status_icon(player_status_container, "SABLE", Color(0.5, 0.6, 0.8, 0.8))

func _get_status_display(effect: int) -> Dictionary:
	if effect == BattleManager.StatusEffect.POISON:
		return {"text": "POISON", "color": Color(0.3, 0.7, 0.2, 0.9)}
	elif effect == BattleManager.StatusEffect.WEAKEN:
		return {"text": "WEAK", "color": Color(0.7, 0.5, 0.2, 0.9)}
	elif effect == BattleManager.StatusEffect.BURN:
		return {"text": "BURN", "color": Color(0.9, 0.4, 0.1, 0.9)}
	return {"text": "???", "color": Color(0.5, 0.5, 0.5, 0.9)}

func _add_status_icon(container: HBoxContainer, text: String, color: Color) -> void:
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
	container.add_child(panel)

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

## S44: 전투 지면 (그라운드 플랫폼 — 원근감)
func _build_battle_ground() -> void:
	_ground_rect = ColorRect.new()
	_ground_rect.anchor_left = 0.0
	_ground_rect.anchor_right = 1.0
	_ground_rect.anchor_top = 0.58
	_ground_rect.anchor_bottom = 1.0
	_ground_rect.color = Color(0.06, 0.05, 0.08, 0.85)
	add_child(_ground_rect)
	# 지면 경계선 (밝은 라인)
	var line = ColorRect.new()
	line.anchor_left = 0.0
	line.anchor_right = 1.0
	line.anchor_top = 0.58
	line.anchor_bottom = 0.58
	line.offset_bottom = 2
	line.color = Color(0.2, 0.18, 0.25, 0.6)
	add_child(line)
	# 지면 그라데이션 (위에서 아래로 점점 밝아짐)
	var gradient = ColorRect.new()
	gradient.anchor_left = 0.0
	gradient.anchor_right = 1.0
	gradient.anchor_top = 0.58
	gradient.anchor_bottom = 0.72
	gradient.color = Color(0.08, 0.07, 0.12, 0.4)
	add_child(gradient)

## S44: 플레이어 스프라이트 (왼쪽 — 사이드뷰)
func _build_player_sprite(root: Control) -> void:
	player_sprite_container = Control.new()
	player_sprite_container.position = Vector2(120, 260)
	player_sprite_container.size = Vector2(200, 200)
	root.add_child(player_sprite_container)
	_player_base_pos = player_sprite_container.position

	# 그림자
	player_shadow = ColorRect.new()
	player_shadow.size = Vector2(120, 16)
	player_shadow.position = Vector2(40, 188)
	player_shadow.color = Color(0, 0, 0, 0.3)
	player_sprite_container.add_child(player_shadow)

	# 포트레이트 이미지가 있으면 사용, 없으면 128x128 픽셀 스프라이트
	var portrait_path = "res://assets/portraits/arrel_neutral.jpg"
	if ResourceLoader.exists(portrait_path):
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(10, 0)
		tex_rect.size = Vector2(180, 180)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = load(portrait_path)
		tex_rect.modulate = Color(1, 1, 1, 0.92)
		player_sprite_container.add_child(tex_rect)
		player_sprite = tex_rect
	else:
		var tex = PixelSprite.create_battle_sprite("arrel")
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(20, 5)
		tex_rect.size = Vector2(160, 160)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = tex
		tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		player_sprite_container.add_child(tex_rect)
		player_sprite = tex_rect

	# 발밑 광원 (은은한 파란 빛)
	var glow = ColorRect.new()
	glow.size = Vector2(140, 20)
	glow.position = Vector2(30, 176)
	glow.color = Color(0.2, 0.35, 0.6, 0.15)
	player_sprite_container.add_child(glow)

## S44: 동행자 스프라이트 (왼쪽 뒤)
func _build_ally_sprite(root: Control) -> void:
	ally_sprite_container = Control.new()
	ally_sprite_container.position = Vector2(20, 230)
	ally_sprite_container.size = Vector2(160, 160)
	ally_sprite_container.visible = false
	root.add_child(ally_sprite_container)
	_ally_base_pos = ally_sprite_container.position

	# 동행자가 있는지 확인
	var has_ally = BattleManager.sable_in_party or GameManager.get_flag("elia_companion")
	if not has_ally:
		return

	ally_sprite_container.visible = true
	var who = "sable" if BattleManager.sable_in_party else "elia"

	# 그림자
	ally_shadow = ColorRect.new()
	ally_shadow.size = Vector2(90, 12)
	ally_shadow.position = Vector2(35, 150)
	ally_shadow.color = Color(0, 0, 0, 0.25)
	ally_sprite_container.add_child(ally_shadow)

	# 동행자 포트레이트 체크
	var portrait_map = {"elia": "res://assets/portraits/elia_neutral.jpg", "sable": "res://assets/portraits/sable_neutral.jpg"}
	var p_path = portrait_map.get(who, "")
	if p_path != "" and ResourceLoader.exists(p_path):
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(15, 5)
		tex_rect.size = Vector2(130, 140)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = load(p_path)
		tex_rect.modulate = Color(0.9, 0.9, 0.9, 0.85)
		ally_sprite_container.add_child(tex_rect)
		ally_sprite = tex_rect
	else:
		var tex = PixelSprite.create_battle_sprite(who)
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(20, 10)
		tex_rect.size = Vector2(120, 120)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = tex
		tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		ally_sprite_container.add_child(tex_rect)
		ally_sprite = tex_rect

	# 발밑 광원
	var glow = ColorRect.new()
	glow.size = Vector2(100, 14)
	glow.position = Vector2(30, 142)
	var glow_color = Color(0.5, 0.3, 0.6, 0.12) if who == "sable" else Color(0.6, 0.5, 0.2, 0.12)
	glow.color = glow_color
	ally_sprite_container.add_child(glow)

## S44: 적 스프라이트 (오른쪽 — 128x128 대형)
func _build_enemy_sprite(root: Control) -> void:
	enemy_sprite_container = Control.new()
	enemy_sprite_container.position = Vector2(820, 180)
	enemy_sprite_container.size = Vector2(260, 260)
	root.add_child(enemy_sprite_container)
	_enemy_base_pos = enemy_sprite_container.position

	# 그림자
	enemy_shadow = ColorRect.new()
	enemy_shadow.size = Vector2(160, 18)
	enemy_shadow.position = Vector2(50, 240)
	enemy_shadow.color = Color(0, 0, 0, 0.35)
	enemy_sprite_container.add_child(enemy_shadow)

	if BattleManager.enemy_image != "" and ResourceLoader.exists(BattleManager.enemy_image):
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(10, 10)
		tex_rect.size = Vector2(240, 230)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = load(BattleManager.enemy_image)
		enemy_sprite_container.add_child(tex_rect)
		enemy_sprite = tex_rect
	else:
		# S44: 128x128 대형 적 스프라이트
		var enemy_type = BattleManager.current_enemy.name if BattleManager.current_enemy else "generic"
		var tex = PixelSprite.create_battle_enemy(enemy_type)
		var tex_rect = TextureRect.new()
		tex_rect.position = Vector2(20, 10)
		tex_rect.size = Vector2(220, 220)
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.texture = tex
		tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		enemy_sprite_container.add_child(tex_rect)
		enemy_sprite = tex_rect

	# 발밑 광원 (적은 빨간/보라 톤)
	var enemy_name = BattleManager.current_enemy.name.to_lower() if BattleManager.current_enemy else ""
	var glow_c = Color(0.5, 0.15, 0.5, 0.15) if "void" in enemy_name or "shade" in enemy_name else Color(0.5, 0.2, 0.15, 0.12)
	var glow = ColorRect.new()
	glow.size = Vector2(180, 22)
	glow.position = Vector2(40, 232)
	glow.color = glow_c
	enemy_sprite_container.add_child(glow)

func _build_log_panel(root: Control) -> void:
	var panel = PanelContainer.new()
	# S44: 로그를 화면 하단 중앙으로 이동
	panel.anchor_left = 0.28
	panel.anchor_right = 0.72
	panel.anchor_top = 0.62
	panel.anchor_bottom = 0.78

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.04, 0.06, 0.88)
	style.set_content_margin_all(10)
	style.set_corner_radius_all(4)
	style.border_color = Color(0.2, 0.18, 0.15, 0.35)
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
	panel.anchor_left = 0.02
	panel.anchor_right = 0.35
	panel.anchor_top = 0.62
	panel.anchor_bottom = 0.62
	panel.offset_bottom = 74

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.06, 0.1, 0.92)
	style.border_color = Color(0.2, 0.3, 0.5, 0.5)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 10)
	panel.add_child(hbox)

	# S44: 미니 포트레이트 (HP 옆)
	var portrait_path = "res://assets/portraits/arrel_neutral.jpg"
	if ResourceLoader.exists(portrait_path):
		player_portrait_rect = TextureRect.new()
		player_portrait_rect.custom_minimum_size = Vector2(52, 52)
		player_portrait_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		player_portrait_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
		player_portrait_rect.texture = load(portrait_path)
		hbox.add_child(player_portrait_rect)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 3)
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(vbox)

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
	action_container.anchor_top = 0.86
	action_container.anchor_bottom = 0.86
	action_container.offset_bottom = 50
	action_container.add_theme_constant_override("separation", 10)
	action_container.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(action_container)

	var actions = [
		{"text": "ATTACK", "callback": _on_attack, "icon": "⚔"},
		{"text": "BURN", "callback": _on_burn_menu, "icon": "🔥"},
		{"text": "ITEM", "callback": _on_item_menu, "icon": ""},
		{"text": "DEFEND", "callback": _on_defend, "icon": "🛡"},
		{"text": "LIMIT", "callback": _on_limit_break, "icon": "💥"},
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
	BattleManager.status_changed.connect(_on_status_changed)
	BattleManager.limit_changed.connect(_on_limit_changed)
	BattleManager.phase_changed.connect(_on_phase_changed)  # S46
	BattleManager.echo_activated.connect(_on_echo_activated)  # S51
	BattleManager.stance_changed.connect(_on_stance_changed)  # S51

	if BattleManager.current_enemy:
		_setup_enemy_display()

func _exit_tree() -> void:
	# 오토로드 시그널 연결 해제 — 씬 재진입 시 freed 객체 참조 방지
	if BattleManager.battle_log.is_connected(_on_battle_log):
		BattleManager.battle_log.disconnect(_on_battle_log)
	if BattleManager.damage_dealt.is_connected(_on_damage_dealt):
		BattleManager.damage_dealt.disconnect(_on_damage_dealt)
	if BattleManager.player_turn_started.is_connected(_on_player_turn):
		BattleManager.player_turn_started.disconnect(_on_player_turn)
	if BattleManager.enemy_turn_started.is_connected(_on_enemy_turn):
		BattleManager.enemy_turn_started.disconnect(_on_enemy_turn)
	if BattleManager.battle_ended.is_connected(_on_battle_ended):
		BattleManager.battle_ended.disconnect(_on_battle_ended)
	if BattleManager.status_changed.is_connected(_on_status_changed):
		BattleManager.status_changed.disconnect(_on_status_changed)
	if BattleManager.limit_changed.is_connected(_on_limit_changed):
		BattleManager.limit_changed.disconnect(_on_limit_changed)
	if BattleManager.phase_changed.is_connected(_on_phase_changed):
		BattleManager.phase_changed.disconnect(_on_phase_changed)

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

	# S46: 히트스톱 — 강한 공격일수록 더 긴 프리즈
	var hit_stop_dur = 0.0
	if amount >= 200:
		hit_stop_dur = 0.12
	elif amount >= 80:
		hit_stop_dur = 0.08
	elif amount >= 30:
		hit_stop_dur = 0.04
	if hit_stop_dur > 0:
		get_tree().paused = true
		await get_tree().create_timer(hit_stop_dur, true, false, true).timeout
		get_tree().paused = false

	# S44: 공격 돌진 애니메이션
	if target != "Arrel" and player_sprite_container:
		_player_attack_rush()

	_show_damage_number(target, amount, skill_name)
	# S46: VFX Library 셰이더 피격 플래시 (flash_white)
	_apply_hit_shader(target, amount)
	_hit_flash(target)
	# S46: 셰이크 스케일링 — 데미지에 비례
	var shake_intensity = clampf(float(amount) / 60.0, 0.5, 3.0)
	_screen_shake(shake_intensity)

	# S52: 크리티컬 히트 줌 펀치 (200+ 데미지)
	if amount >= 200 and target != "Arrel":
		_critical_zoom_punch()

	# 스킬별 VFX
	if skill_name != "" and target != "Arrel":
		_play_attack_vfx(skill_name)
	elif target != "Arrel":
		_play_slash_vfx()
		_play_speed_lines()  # S44: 속도선

func _on_player_turn() -> void:
	_show_turn_indicator("— YOUR TURN —", Color(0.5, 0.65, 0.85))
	_update_turn_preview()  # S41
	# S41: 콤보 버스트 VFX
	if BattleManager.combo_count >= 2:
		_play_combo_burst(BattleManager.combo_count)
	await get_tree().create_timer(0.5).timeout
	action_container.visible = true
	if ally_cmd_container:
		ally_cmd_container.visible = BattleManager.sable_in_party
	if tobias_cmd_container:
		tobias_cmd_container.visible = BattleManager.tobias_in_party
	if stance_container:
		stance_container.visible = true
	if elia_skill_container:
		elia_skill_container.visible = GameManager.player_data.elia_with_party
		_refresh_elia_skills()
	_refresh_echo_display()
	_update_limit_button()
	if action_container.get_child_count() > 0:
		action_container.get_child(0).grab_focus()

func _on_enemy_turn() -> void:
	_show_turn_indicator("— ENEMY TURN —", Color(0.8, 0.4, 0.35))
	_update_turn_preview()  # S41
	if tobias_cmd_container:
		tobias_cmd_container.visible = false
	if stance_container:
		stance_container.visible = false
	if elia_skill_container:
		elia_skill_container.visible = false

func _on_status_changed() -> void:
	_update_status_icons()
	_update_enemy_status_visual()  # S41: 상태이상 스프라이트 틴트
	_update_status_shaders()       # S46: VFX Library 상태이상 셰이더

func _on_battle_ended(_result) -> void:
	action_container.visible = false
	if ally_cmd_container:
		ally_cmd_container.visible = false
	if tobias_cmd_container:
		tobias_cmd_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	# S40: 승리 시 적 디졸브 효과
	if _result == BattleManager.BattleState.VICTORY and enemy_sprite:
		_play_enemy_dissolve()

## ===================== 행동 콜백 =====================

func _on_attack() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	BattleManager.player_attack()

func _on_burn_menu() -> void:
	AudioManager.play_sfx("ui_select")
	_hide_item_list()
	_toggle_burn_list()

func _on_item_menu() -> void:
	AudioManager.play_sfx("ui_select")
	_hide_burn_list()
	_toggle_item_list()

func _on_defend() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	BattleManager.player_defend()

func _on_flee() -> void:
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	BattleManager.player_flee()

## ===================== 기억 연소 목록 =====================

func _toggle_burn_list() -> void:
	var scroll = burn_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_burn_list()
		return

	for child in burn_list_container.get_children():
		child.queue_free()

	var available = MemoryManager.get_available_memories().filter(func(m): return not m.is_faded)
	var residues = MemoryManager.get_residue_memories()
	if available.is_empty() and residues.is_empty():
		var empty_label = Label.new()
		empty_label.text = "No memories left to burn."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", Color(0.5, 0.4, 0.35))
		burn_list_container.add_child(empty_label)
	else:
		if not available.is_empty():
			var title = Label.new()
			title.text = "— Select a memory to burn —"
			title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			title.add_theme_font_size_override("font_size", 13)
			title.add_theme_color_override("font_color", Color(0.75, 0.5, 0.35))
			burn_list_container.add_child(title)

			for memory in available:
				var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
				var elem = skill.get("element", "fire").to_upper()
				var eff_power = MemoryManager.get_effective_burn_power(memory)
				var erosion_tag = "" if memory.erosion == 0 else " ⚠"
				var btn = Button.new()
				btn.text = "[%s|%s] %s — Grade %d (DMG: %d+%d)%s" % [
					skill.name, elem, memory.title,
					memory.grade,
					skill.base_damage, eff_power, erosion_tag
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

		# 잔존 기억 (Residue) — 50% 데미지로 재사용
		if not residues.is_empty():
			var res_title = Label.new()
			res_title.text = "— Residue (50% power, no loss) —"
			res_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			res_title.add_theme_font_size_override("font_size", 12)
			res_title.add_theme_color_override("font_color", Color(0.5, 0.4, 0.6))
			burn_list_container.add_child(res_title)

			for memory in residues:
				var skill = BattleManager.BURN_SKILLS.get(memory.grade, BattleManager.BURN_SKILLS[0])
				var half_dmg = int((skill.base_damage + memory.burn_power) * 0.5)
				var btn = Button.new()
				btn.text = "[RESIDUE] %s — %s (DMG: ~%d)" % [skill.name, memory.title, half_dmg]
				btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

				var style = StyleBoxFlat.new()
				style.bg_color = Color(0.06, 0.05, 0.1, 0.85)
				style.set_content_margin_all(8)
				style.set_corner_radius_all(3)
				btn.add_theme_stylebox_override("normal", style)
				var hover_s = style.duplicate()
				hover_s.bg_color = Color(0.12, 0.08, 0.18, 0.95)
				hover_s.border_color = Color(0.5, 0.3, 0.6, 0.7)
				hover_s.set_border_width_all(1)
				btn.add_theme_stylebox_override("hover", hover_s)
				btn.add_theme_stylebox_override("focus", hover_s)
				btn.add_theme_font_size_override("font_size", 12)
				btn.add_theme_color_override("font_color", Color(0.5, 0.4, 0.6))
				btn.add_theme_color_override("font_hover_color", Color(0.75, 0.55, 0.8))

				var mid = memory.id
				btn.pressed.connect(func():
					AudioManager.play_sfx("ui_select")
					action_container.visible = false
					_hide_burn_list()
					BattleManager.player_burn_residue(mid)
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

## ===================== 아이템 목록 =====================

func _build_item_list(root: Control) -> void:
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
	style.bg_color = Color(0.06, 0.08, 0.04, 0.96)
	style.border_color = Color(0.3, 0.5, 0.2, 0.7)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(12)
	panel.add_theme_stylebox_override("panel", style)
	scroll.add_child(panel)

	item_list_container = VBoxContainer.new()
	item_list_container.add_theme_constant_override("separation", 6)
	panel.add_child(item_list_container)

	root.add_child(scroll)
	item_list_container.set_meta("scroll_parent", scroll)

func _toggle_item_list() -> void:
	var scroll = item_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll.visible:
		_hide_item_list()
		return

	for child in item_list_container.get_children():
		child.queue_free()

	var items = GameManager.player_data.items
	if items.is_empty():
		var empty_label = Label.new()
		empty_label.text = "No items."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", Color(0.5, 0.4, 0.35))
		item_list_container.add_child(empty_label)
	else:
		var title = Label.new()
		title.text = "— Select an item —"
		title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		title.add_theme_font_size_override("font_size", 13)
		title.add_theme_color_override("font_color", Color(0.4, 0.65, 0.35))
		item_list_container.add_child(title)

		for item_id in items:
			var count = items[item_id]
			var item_def = GameManager.ITEMS.get(item_id)
			if item_def == null:
				continue
			var btn = Button.new()
			btn.text = "%s x%d — %s" % [item_def["name"], count, item_def["desc"]]
			btn.alignment = HORIZONTAL_ALIGNMENT_LEFT

			var s = StyleBoxFlat.new()
			s.bg_color = Color(0.06, 0.08, 0.05, 0.85)
			s.set_content_margin_all(8)
			s.set_corner_radius_all(3)
			btn.add_theme_stylebox_override("normal", s)
			var hover_s = s.duplicate()
			hover_s.bg_color = Color(0.12, 0.18, 0.1, 0.95)
			hover_s.border_color = Color(0.4, 0.65, 0.3, 0.7)
			hover_s.set_border_width_all(1)
			btn.add_theme_stylebox_override("hover", hover_s)
			btn.add_theme_stylebox_override("focus", hover_s)
			btn.add_theme_font_size_override("font_size", 12)
			btn.add_theme_color_override("font_color", Color(0.6, 0.7, 0.55))
			btn.add_theme_color_override("font_hover_color", Color(0.8, 0.95, 0.5))

			var iid = item_id
			btn.pressed.connect(func():
				AudioManager.play_sfx("ui_select")
				action_container.visible = false
				_hide_item_list()
				BattleManager.player_use_item(iid)
			)
			btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
			item_list_container.add_child(btn)

	var cancel_btn = Button.new()
	cancel_btn.text = "[ Cancel ]"
	cancel_btn.add_theme_font_size_override("font_size", 12)
	cancel_btn.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	cancel_btn.pressed.connect(func():
		AudioManager.play_sfx("cancel")
		_hide_item_list()
	)
	item_list_container.add_child(cancel_btn)

	scroll.visible = true

func _hide_item_list() -> void:
	var scroll = item_list_container.get_meta("scroll_parent") as ScrollContainer
	if scroll:
		scroll.visible = false

## ===================== 시각 피드백 =====================

## 데미지 숫자 표시 (떠오르며 사라짐 — 크기 스케일링)
func _show_damage_number(target: String, amount: int, skill_name: String = "") -> void:
	var label = Label.new()
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER

	# 회복인 경우 (음수 amount = 힐)
	var is_heal = amount < 0
	if is_heal:
		label.text = "+%d" % abs(amount)
		_play_heal_vfx()  # S42: 힐 파티클
	else:
		label.text = str(amount)

	# 데미지 크기에 따른 폰트 스케일
	var font_size = 22
	if abs(amount) >= 100:
		font_size = 30
	elif abs(amount) >= 50:
		font_size = 26
	label.add_theme_font_size_override("font_size", font_size)

	# S40: 스킬/상황별 색상 분류
	var dmg_color: Color
	if is_heal:
		dmg_color = Color(0.3, 1.0, 0.4)  # 회복 = 초록
	elif target == "Arrel":
		dmg_color = Color(1.0, 0.3, 0.25)  # 플레이어 피격 = 빨강
	else:
		# 스킬별 색상
		var sn = skill_name.to_lower()
		if sn.find("burn") >= 0 or sn.find("flame") >= 0 or sn.find("ember") >= 0 or sn.find("fire") >= 0 or sn.find("scorch") >= 0:
			dmg_color = Color(1.0, 0.5, 0.15)  # 화염 = 주황
		elif sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("residue") >= 0:
			dmg_color = Color(0.7, 0.3, 1.0)  # 보이드 = 보라
		elif sn.find("drain") >= 0:
			dmg_color = Color(0.5, 0.9, 0.5)  # 드레인 = 연초록
		elif sn.find("poison") >= 0:
			dmg_color = Color(0.4, 0.85, 0.3)  # 독 = 독녹색
		elif sn.find("combo") >= 0:
			dmg_color = Color(1.0, 0.85, 0.2)  # 콤보 = 금색
		else:
			dmg_color = Color(1.0, 0.9, 0.4)  # 기본 = 연노랑
	label.add_theme_color_override("font_color", dmg_color)

	# S44: 사이드뷰 위치 기반 데미지 숫자
	if target == "Arrel":
		label.position = Vector2(180 + randf_range(-20, 20), 280 + randf_range(-10, 10))
	else:
		label.position = Vector2(900 + randf_range(-30, 30), 260 + randf_range(-10, 10))

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

	# S44: 스프라이트 깜빡임 + 피격 밀림
	if target != "Arrel" and enemy_sprite:
		var flash_t = create_tween()
		flash_t.tween_property(enemy_sprite, "modulate", Color(3, 3, 3, 1), 0.05)
		flash_t.tween_property(enemy_sprite, "modulate", Color(1, 1, 1, 1), 0.15)
		# 피격 밀림 (오른쪽으로 살짝)
		if enemy_sprite_container:
			var push_t = create_tween()
			push_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x + 15, 0.06).set_ease(Tween.EASE_OUT)
			push_t.tween_property(enemy_sprite_container, "position:x", _enemy_base_pos.x, 0.2).set_ease(Tween.EASE_IN_OUT)
	elif target == "Arrel" and player_sprite:
		var flash_t = create_tween()
		flash_t.tween_property(player_sprite, "modulate", Color(2, 0.5, 0.5, 1), 0.05)
		flash_t.tween_property(player_sprite, "modulate", Color(1, 1, 1, 1), 0.15)
		# 피격 밀림 (왼쪽으로)
		if player_sprite_container:
			var push_t = create_tween()
			push_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x - 12, 0.06).set_ease(Tween.EASE_OUT)
			push_t.tween_property(player_sprite_container, "position:x", _player_base_pos.x, 0.2).set_ease(Tween.EASE_IN_OUT)

## 스크린 셰이크 (S42 강화: 더 많은 프레임 + 회전 흔들림)
func _screen_shake(intensity: float = 1.0) -> void:
	# S53: 접근성 — 화면 흔들림 비활성화 옵션
	if not OptionsMenu.settings.get("screen_shake", true):
		return
	var original_pos = canvas_root.position
	var t = create_tween()
	var frames = int(6 + intensity * 2)
	for i in range(frames):
		var decay = 1.0 - float(i) / frames
		var offset = Vector2(
			randf_range(-7, 7) * intensity * decay,
			randf_range(-5, 5) * intensity * decay
		)
		t.tween_property(canvas_root, "position", original_pos + offset, 0.03)
	t.tween_property(canvas_root, "position", original_pos, 0.04)

## ===================== S44: 전투 애니메이션 =====================

## 플레이어 공격 돌진 (적 방향으로 빠르게 이동 후 복귀)
func _player_attack_rush() -> void:
	if not player_sprite_container:
		return
	var rush_target = Vector2(_enemy_base_pos.x - 180, _player_base_pos.y - 10)
	var t = create_tween()
	t.tween_property(player_sprite_container, "position", rush_target, 0.12).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_BACK)
	t.tween_interval(0.1)  # 잠깐 멈춤 (임팩트)
	t.tween_property(player_sprite_container, "position", _player_base_pos, 0.25).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

## 속도선 (공격 시 화면에 빗금)
func _play_speed_lines() -> void:
	for i in range(6):
		var line = ColorRect.new()
		line.size = Vector2(randf_range(200, 500), 2)
		line.position = Vector2(-100, randf_range(50, 500))
		line.rotation = -0.15
		line.color = Color(1, 1, 1, randf_range(0.08, 0.2))
		line.z_index = 40
		line.mouse_filter = Control.MOUSE_FILTER_IGNORE
		canvas_root.add_child(line)
		var t = create_tween().set_parallel(true)
		t.tween_property(line, "position:x", 1400.0, randf_range(0.15, 0.3)).set_delay(randf_range(0, 0.05))
		t.tween_property(line, "modulate:a", 0.0, 0.2).set_delay(0.1)
		t.chain().tween_callback(line.queue_free)

## 임팩트 버스트 (타격 순간 방사형 빛)
func _play_impact_burst(pos: Vector2) -> void:
	# 중앙 원형 플래시
	var burst = ColorRect.new()
	burst.size = Vector2(8, 8)
	burst.position = pos - Vector2(4, 4)
	burst.color = Color(1, 0.95, 0.8, 0.9)
	burst.z_index = 62
	canvas_root.add_child(burst)
	var bt = create_tween().set_parallel(true)
	bt.tween_property(burst, "size", Vector2(80, 80), 0.15).set_ease(Tween.EASE_OUT)
	bt.tween_property(burst, "position", pos - Vector2(40, 40), 0.15).set_ease(Tween.EASE_OUT)
	bt.tween_property(burst, "color:a", 0.0, 0.25)
	bt.chain().tween_callback(burst.queue_free)
	# 방사 선 4개
	for j in range(4):
		var ray = ColorRect.new()
		ray.size = Vector2(3, 0)
		ray.position = pos
		ray.rotation = j * PI / 4.0 + randf_range(-0.2, 0.2)
		ray.pivot_offset = Vector2(1.5, 0)
		ray.color = Color(1, 0.9, 0.7, 0.7)
		ray.z_index = 61
		canvas_root.add_child(ray)
		var rt = create_tween().set_parallel(true)
		rt.tween_property(ray, "size:y", randf_range(60, 120), 0.12).set_ease(Tween.EASE_OUT)
		rt.tween_property(ray, "modulate:a", 0.0, 0.2).set_delay(0.08)
		rt.chain().tween_callback(ray.queue_free)

## ===================== 공격 VFX =====================

## 물리 공격 슬래시 이펙트 (S42 개선: GPU 파티클 추가)
func _play_slash_vfx() -> void:
	_play_gpu_slash_particles()  # S42: GPU 파티클
	# S44: 임팩트 버스트
	_play_impact_burst(Vector2(900, 320))
	var center = Vector2(900, 320)  # S44: 사이드뷰 적 위치

	# 메인 슬래시 — 길이 확장 애니메이션
	var slash = ColorRect.new()
	slash.size = Vector2(0, 4)
	slash.position = center + Vector2(-60, -30)
	slash.rotation = -0.55
	slash.pivot_offset = Vector2(0, 2)
	slash.color = Color(1, 1, 1, 0.9)
	slash.z_index = 60
	canvas_root.add_child(slash)

	var t = create_tween()
	t.tween_property(slash, "size:x", 220.0, 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(slash, "modulate:a", 0.0, 0.2).set_ease(Tween.EASE_IN)
	t.tween_callback(slash.queue_free)

	# 크로스 슬래시 (지연)
	var slash2 = ColorRect.new()
	slash2.size = Vector2(0, 3)
	slash2.position = center + Vector2(-40, 10)
	slash2.rotation = 0.45
	slash2.pivot_offset = Vector2(0, 1.5)
	slash2.color = Color(1, 0.9, 0.7, 0.7)
	slash2.z_index = 60
	canvas_root.add_child(slash2)

	var t2 = create_tween()
	t2.tween_interval(0.06)
	t2.tween_property(slash2, "size:x", 190.0, 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t2.tween_property(slash2, "modulate:a", 0.0, 0.18).set_ease(Tween.EASE_IN)
	t2.tween_callback(slash2.queue_free)

	# 충격 파편 (흰색 입자 4~6개)
	for i in range(randi_range(4, 6)):
		var spark = ColorRect.new()
		spark.size = Vector2(3, 3)
		spark.position = center + Vector2(randf_range(-20, 20), randf_range(-15, 15))
		spark.color = Color(1, 1, 1, 0.8)
		spark.z_index = 58
		canvas_root.add_child(spark)
		var angle = randf() * TAU
		var dist = randf_range(25, 60)
		var target_pos = spark.position + Vector2(cos(angle), sin(angle)) * dist
		var st = create_tween().set_parallel(true)
		st.tween_property(spark, "position", target_pos, 0.3).set_ease(Tween.EASE_OUT)
		st.tween_property(spark, "modulate:a", 0.0, 0.25).set_delay(0.08)
		st.chain().tween_callback(spark.queue_free)

## 기억 연소 VFX — 스킬별 분류 (S40)
func _play_attack_vfx(skill_name: String) -> void:
	var sn = skill_name.to_lower()
	# 보이드 스킬 → 보라 파티클
	if sn.find("void") >= 0 or sn.find("cascade") >= 0 or sn.find("residue") >= 0:
		_play_void_vfx()
	else:
		# 연소/기본 → 불꽃 VFX
		_play_burn_vfx()

## 불꽃 VFX (S42: GPU 파티클 추가, S52: 화면 가장자리 불꽃)
func _play_burn_vfx() -> void:
	_play_gpu_burn_particles()  # S42: GPU 파티클
	_burn_edge_flare()  # S52: 화면 가장자리 화염
	var center = Vector2(920, 310)

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

## ===================== S40: 적 디졸브 사망 이펙트 =====================

func _play_enemy_dissolve() -> void:
	var shader_path = "res://assets/shaders/dissolve.gdshader"
	if not ResourceLoader.exists(shader_path) or not enemy_sprite:
		return
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	mat.set_shader_parameter("progress", 0.0)
	mat.set_shader_parameter("edge_color", Color(0.6, 0.2, 0.8, 1.0))
	mat.set_shader_parameter("edge_width", 0.08)
	enemy_sprite.material = mat
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("progress", val), 0.0, 1.0, 1.2).set_ease(Tween.EASE_IN).set_trans(Tween.TRANS_QUAD)

## ===================== S40: 색수차 이펙트 (Limit Break) =====================

var _chromatic_overlay: ColorRect

func _play_chromatic_aberration(duration: float = 1.5) -> void:
	var shader_path = "res://assets/shaders/chromatic_aberration.gdshader"
	if not ResourceLoader.exists(shader_path):
		return
	if _chromatic_overlay and is_instance_valid(_chromatic_overlay):
		_chromatic_overlay.queue_free()
	_chromatic_overlay = ColorRect.new()
	_chromatic_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_chromatic_overlay.color = Color(0, 0, 0, 0)  # 셰이더가 screen_texture에서 직접 샘플링
	_chromatic_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_chromatic_overlay.z_index = 80
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	mat.set_shader_parameter("strength", 0.008)
	mat.set_shader_parameter("pulse_speed", 5.0)
	mat.set_shader_parameter("use_pulse", true)
	_chromatic_overlay.material = mat
	canvas_root.add_child(_chromatic_overlay)
	# 강도 페이드: 강하게 시작 → 점점 감소
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("strength", val), 0.012, 0.0, duration).set_ease(Tween.EASE_OUT)
	t.tween_callback(_chromatic_overlay.queue_free)

## ===================== S40: 보이드 스킬 VFX (보라색 파티클 폭발) =====================

func _play_void_vfx() -> void:
	_play_gpu_void_particles()  # S42: GPU 파티클
	var center = Vector2(920, 310)
	for i in range(16):
		var particle = ColorRect.new()
		var s = randf_range(3, 8)
		particle.size = Vector2(s, s)
		particle.position = center + Vector2(randf_range(-50, 50), randf_range(-40, 40))
		particle.z_index = 55
		var void_colors = [
			Color(0.5, 0.15, 0.8, 0.9),
			Color(0.3, 0.1, 0.6, 0.85),
			Color(0.7, 0.3, 1.0, 0.8),
			Color(0.2, 0.05, 0.4, 0.7),
		]
		particle.color = void_colors[randi_range(0, void_colors.size() - 1)]
		canvas_root.add_child(particle)
		# 방사형으로 퍼지며 사라짐
		var angle = randf() * TAU
		var dist = randf_range(40, 100)
		var target_pos = particle.position + Vector2(cos(angle), sin(angle)) * dist
		var delay = randf_range(0, 0.1)
		var t = create_tween().set_parallel(true)
		t.tween_property(particle, "position", target_pos, randf_range(0.5, 0.9)).set_delay(delay).set_ease(Tween.EASE_OUT)
		t.tween_property(particle, "modulate:a", 0.0, randf_range(0.4, 0.7)).set_delay(delay + 0.15)
		t.tween_property(particle, "size", Vector2(1, 1), 0.7).set_delay(delay)
		t.chain().tween_callback(particle.queue_free)
	# 보라색 플래시
	var flash = ColorRect.new()
	flash.size = Vector2(120, 120)
	flash.position = center - Vector2(60, 60)
	flash.color = Color(0.4, 0.1, 0.6, 0.35)
	flash.z_index = 54
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "modulate:a", 0.0, 0.4)
	ft.tween_callback(flash.queue_free)

## ===================== Limit Break UI =====================

func _build_limit_gauge(root: Control) -> void:
	var panel = PanelContainer.new()
	panel.anchor_left = 0.38
	panel.anchor_right = 0.62
	panel.anchor_top = 0.78
	panel.anchor_bottom = 0.78
	panel.offset_bottom = 28

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.04, 0.08, 0.85)
	style.border_color = Color(0.5, 0.3, 0.6, 0.5)
	style.set_border_width_all(1)
	style.set_corner_radius_all(3)
	style.set_content_margin_all(4)
	panel.add_theme_stylebox_override("panel", style)
	root.add_child(panel)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 6)
	panel.add_child(hbox)

	limit_label = Label.new()
	limit_label.text = "LIMIT"
	limit_label.add_theme_font_size_override("font_size", 10)
	limit_label.add_theme_color_override("font_color", Color(0.6, 0.4, 0.7))
	hbox.add_child(limit_label)

	limit_bar = ProgressBar.new()
	limit_bar.custom_minimum_size = Vector2(100, 14)
	limit_bar.max_value = 100.0
	limit_bar.value = 0.0
	limit_bar.show_percentage = false
	limit_bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var fill = StyleBoxFlat.new()
	fill.bg_color = Color(0.7, 0.3, 0.8)
	fill.set_corner_radius_all(2)
	limit_bar.add_theme_stylebox_override("fill", fill)
	var bg_s = StyleBoxFlat.new()
	bg_s.bg_color = Color(0.08, 0.06, 0.1)
	bg_s.set_corner_radius_all(2)
	limit_bar.add_theme_stylebox_override("background", bg_s)
	hbox.add_child(limit_bar)

func _on_limit_changed(value: float) -> void:
	if limit_bar:
		limit_bar.value = value
		# 게이지 꽉 차면 색상 변경
		var fill = limit_bar.get_theme_stylebox("fill") as StyleBoxFlat
		if fill:
			if value >= BattleManager.LIMIT_MAX:
				fill.bg_color = Color(1.0, 0.6, 0.9)
				limit_label.add_theme_color_override("font_color", Color(1.0, 0.7, 0.9))
			else:
				fill.bg_color = Color(0.7, 0.3, 0.8)
				limit_label.add_theme_color_override("font_color", Color(0.6, 0.4, 0.7))
	# LIMIT 버튼 활성화/비활성화
	_update_limit_button()

func _update_limit_button() -> void:
	if not action_container:
		return
	for child in action_container.get_children():
		if child is Button and child.text == "LIMIT":
			child.disabled = BattleManager.limit_gauge < BattleManager.LIMIT_MAX
			if child.disabled:
				child.modulate = Color(0.5, 0.5, 0.5, 0.7)
			else:
				child.modulate = Color(1.0, 0.8, 1.0, 1.0)

func _on_limit_break() -> void:
	if BattleManager.limit_gauge < BattleManager.LIMIT_MAX:
		AudioManager.play_sfx("cancel")
		return
	AudioManager.play_sfx("ui_select")
	action_container.visible = false
	_hide_burn_list()
	_hide_item_list()
	# S40: Limit Break 색수차 연출
	_play_chromatic_aberration(2.0)
	_screen_shake(2.5)
	# S42: 리밋 브레이크 폭발 파티클
	_play_limit_burst_vfx()
	BattleManager.player_limit_break()

## ===================== S42: 전투 분위기 + 강화된 VFX =====================

## 배경 분위기 파티클 (떠다니는 먼지/잿가루)
func _add_battle_atmosphere() -> void:
	# 떠다니는 먼지 파티클
	_battle_particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0.5, -0.3, 0)
	mat.spread = 60.0
	mat.initial_velocity_min = 5.0
	mat.initial_velocity_max = 15.0
	mat.gravity = Vector3(2, -3, 0)
	mat.scale_min = 0.5
	mat.scale_max = 2.0

	# 적에 따라 파티클 색상 결정
	var enemy_name = BattleManager.current_enemy.name.to_lower() if BattleManager.current_enemy else ""
	var p_color: Color
	if "void" in enemy_name or "shade" in enemy_name:
		p_color = Color(0.4, 0.15, 0.6, 0.25)
	elif "sentinel" in enemy_name:
		p_color = Color(0.3, 0.1, 0.5, 0.3)
	else:
		p_color = Color(0.5, 0.45, 0.4, 0.15)

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(p_color.r, p_color.g, p_color.b, 0.0))
	g.add_point(0.3, p_color)
	g.add_point(0.7, Color(p_color.r, p_color.g, p_color.b, p_color.a * 0.6))
	g.set_color(1, Color(p_color.r, p_color.g, p_color.b, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(640, 360, 0)

	_battle_particles.process_material = mat
	_battle_particles.amount = 20
	_battle_particles.lifetime = 6.0
	_battle_particles.position = Vector2(640, 360)
	_battle_particles.z_index = -1
	_battle_particles.visibility_rect = Rect2(-700, -400, 1400, 800)
	add_child(_battle_particles)

	# 컬러 그레이딩 오버레이 (전투 분위기)
	_color_grade_rect = ColorRect.new()
	_color_grade_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	_color_grade_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_color_grade_rect.z_index = -2
	if "void" in enemy_name or "shade" in enemy_name:
		_color_grade_rect.color = Color(0.1, 0.05, 0.15, 0.15)
	else:
		_color_grade_rect.color = Color(0.05, 0.03, 0.0, 0.1)
	add_child(_color_grade_rect)

## 물리 공격 GPUParticles2D 이펙트 (S42: 기존 ColorRect 대체)
func _play_gpu_slash_particles() -> void:
	var center = Vector2(900, 300)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(-1, 0, 0)
	mat.spread = 25.0
	mat.initial_velocity_min = 150.0
	mat.initial_velocity_max = 300.0
	mat.gravity = Vector3(0, 50, 0)
	mat.scale_min = 0.5
	mat.scale_max = 2.0
	mat.damping_min = 100.0
	mat.damping_max = 200.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1, 1, 1, 1.0))
	g.add_point(0.3, Color(1, 0.9, 0.7, 0.8))
	g.set_color(1, Color(1, 0.7, 0.3, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 5.0
	particles.process_material = mat
	particles.amount = 24
	particles.lifetime = 0.4
	particles.one_shot = true
	particles.explosiveness = 0.9
	particles.position = center
	particles.z_index = 60
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 자동 정리
	var timer = get_tree().create_timer(1.0)
	timer.timeout.connect(particles.queue_free)

## 불꽃 GPUParticles2D (S42)
func _play_gpu_burn_particles() -> void:
	var center = Vector2(920, 310)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)
	mat.spread = 40.0
	mat.initial_velocity_min = 40.0
	mat.initial_velocity_max = 120.0
	mat.gravity = Vector3(0, -80, 0)
	mat.scale_min = 1.0
	mat.scale_max = 4.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1, 0.9, 0.3, 1.0))
	g.add_point(0.2, Color(1, 0.6, 0.1, 0.9))
	g.add_point(0.5, Color(0.9, 0.3, 0.05, 0.7))
	g.set_color(1, Color(0.3, 0.1, 0.05, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(40, 20, 0)
	particles.process_material = mat
	particles.amount = 40
	particles.lifetime = 0.7
	particles.one_shot = true
	particles.explosiveness = 0.7
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 열기 왜곡 오버레이
	var heat = ColorRect.new()
	heat.size = Vector2(120, 80)
	heat.position = center - Vector2(60, 50)
	heat.color = Color(1.0, 0.5, 0.1, 0.2)
	heat.z_index = 54
	canvas_root.add_child(heat)
	var ht = create_tween()
	ht.tween_property(heat, "color:a", 0.0, 0.5)
	ht.tween_callback(heat.queue_free)

	var timer = get_tree().create_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 보이드 GPUParticles2D (S42)
func _play_gpu_void_particles() -> void:
	var center = Vector2(920, 310)
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 0, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 60.0
	mat.initial_velocity_max = 150.0
	mat.gravity = Vector3(0, 0, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.5
	mat.damping_min = 50.0
	mat.damping_max = 100.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.7, 0.3, 1.0, 1.0))
	g.add_point(0.3, Color(0.5, 0.15, 0.8, 0.8))
	g.add_point(0.6, Color(0.3, 0.1, 0.6, 0.5))
	g.set_color(1, Color(0.15, 0.05, 0.3, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 10.0
	particles.process_material = mat
	particles.amount = 50
	particles.lifetime = 0.8
	particles.one_shot = true
	particles.explosiveness = 0.85
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 중앙 에너지 링
	var ring = ColorRect.new()
	ring.size = Vector2(8, 8)
	ring.position = center - Vector2(4, 4)
	ring.color = Color(0.6, 0.2, 1.0, 0.8)
	ring.z_index = 56
	canvas_root.add_child(ring)
	var rt = create_tween().set_parallel(true)
	rt.tween_property(ring, "size", Vector2(100, 100), 0.3).set_ease(Tween.EASE_OUT)
	rt.tween_property(ring, "position", center - Vector2(50, 50), 0.3).set_ease(Tween.EASE_OUT)
	rt.tween_property(ring, "color:a", 0.0, 0.4)
	rt.chain().tween_callback(ring.queue_free)

	var timer = get_tree().create_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 힐 GPUParticles2D (S42: 새로운 힐 이펙트)
func _play_heal_vfx() -> void:
	var center = Vector2(200, 360)  # S44: 플레이어 위치 근처
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, -1, 0)
	mat.spread = 30.0
	mat.initial_velocity_min = 30.0
	mat.initial_velocity_max = 80.0
	mat.gravity = Vector3(0, -40, 0)
	mat.scale_min = 1.0
	mat.scale_max = 3.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(0.3, 1.0, 0.5, 0.9))
	g.add_point(0.4, Color(0.5, 1.0, 0.7, 0.7))
	g.set_color(1, Color(0.7, 1.0, 0.9, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(30, 5, 0)
	particles.process_material = mat
	particles.amount = 25
	particles.lifetime = 0.8
	particles.one_shot = true
	particles.explosiveness = 0.5
	particles.position = center
	particles.z_index = 55
	particles.visibility_rect = Rect2(-200, -200, 400, 400)
	canvas_root.add_child(particles)
	particles.emitting = true

	var timer = get_tree().create_timer(1.5)
	timer.timeout.connect(particles.queue_free)

## 리밋 브레이크 폭발 VFX (S42)
func _play_limit_burst_vfx() -> void:
	var center = Vector2(920, 310)
	# 큰 폭발 파티클
	var particles = GPUParticles2D.new()
	var mat = ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 0, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 100.0
	mat.initial_velocity_max = 350.0
	mat.gravity = Vector3(0, 30, 0)
	mat.scale_min = 2.0
	mat.scale_max = 6.0
	mat.damping_min = 50.0
	mat.damping_max = 150.0

	var gradient = GradientTexture1D.new()
	var g = Gradient.new()
	g.set_color(0, Color(1.0, 0.9, 1.0, 1.0))
	g.add_point(0.2, Color(1.0, 0.6, 0.9, 0.9))
	g.add_point(0.5, Color(0.8, 0.3, 0.7, 0.6))
	g.set_color(1, Color(0.5, 0.1, 0.4, 0.0))
	gradient.gradient = g
	mat.color_ramp = gradient

	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	mat.emission_sphere_radius = 15.0
	particles.process_material = mat
	particles.amount = 80
	particles.lifetime = 1.0
	particles.one_shot = true
	particles.explosiveness = 0.95
	particles.position = center
	particles.z_index = 65
	particles.visibility_rect = Rect2(-400, -400, 800, 800)
	canvas_root.add_child(particles)
	particles.emitting = true

	# 화면 백색 플래시
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1, 0.9, 1, 0.6)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 90
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.8).set_ease(Tween.EASE_OUT)
	ft.tween_callback(flash.queue_free)

	var timer = get_tree().create_timer(2.0)
	timer.timeout.connect(particles.queue_free)

## ===================== S46: 타격감 강화 + VFX Library 셰이더 =====================

## VFX Library — flash_white 피격 셰이더 (적/플레이어 스프라이트에 흰색 플래시)
func _apply_hit_shader(target: String, amount: int) -> void:
	var shader_path = "res://addons/vfx_lib/shaders/flash_white.gdshader"
	if not ResourceLoader.exists(shader_path):
		return
	var sprite_node: Control = null
	if target != "Arrel" and enemy_sprite:
		sprite_node = enemy_sprite
	elif target == "Arrel" and player_sprite:
		sprite_node = player_sprite
	if not sprite_node:
		return
	var mat = ShaderMaterial.new()
	mat.shader = load(shader_path)
	var flash_strength = clampf(float(amount) / 100.0, 0.4, 1.0)
	mat.set_shader_parameter("flash_amount", flash_strength)
	mat.set_shader_parameter("flash_color", Color(1, 1, 1, 1) if target != "Arrel" else Color(1, 0.4, 0.3, 1))
	sprite_node.material = mat
	# 플래시 페이드아웃
	var t = create_tween()
	t.tween_method(func(val): mat.set_shader_parameter("flash_amount", val), flash_strength, 0.0, 0.25)
	t.tween_callback(func(): sprite_node.material = null)

## VFX Library — 상태이상 셰이더 (독/화상/약화) 적 스프라이트에 적용
func _apply_status_shader() -> void:
	if not enemy_sprite:
		return
	# 독 — poison 셰이더
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.POISON):
		var shader_path = "res://addons/vfx_lib/shaders/poison.gdshader"
		if ResourceLoader.exists(shader_path):
			var mat = ShaderMaterial.new()
			mat.shader = load(shader_path)
			mat.set_shader_parameter("poison_amount", 0.6)
			mat.set_shader_parameter("poison_color", Color(0.3, 1.0, 0.3, 1.0))
			mat.set_shader_parameter("pulse_speed", 3.0)
			enemy_sprite.material = mat
			return
	# 화상 — burning 셰이더
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.BURN):
		var shader_path = "res://addons/vfx_lib/shaders/burning.gdshader"
		if ResourceLoader.exists(shader_path):
			var mat = ShaderMaterial.new()
			mat.shader = load(shader_path)
			mat.set_shader_parameter("burn_amount", 0.5)
			mat.set_shader_parameter("fire_color1", Color(1.0, 0.8, 0.2, 1.0))
			mat.set_shader_parameter("fire_color2", Color(1.0, 0.3, 0.0, 1.0))
			mat.set_shader_parameter("distortion_strength", 0.02)
			enemy_sprite.material = mat
			return
	# 약화 — 그레이스케일 틴트 (기존 VFX lib grayscale 사용)
	if BattleManager.has_status("enemy", BattleManager.StatusEffect.WEAKEN):
		enemy_sprite.modulate = Color(0.7, 0.6, 0.8, 1.0)
		return
	# 상태이상 없으면 클리어
	enemy_sprite.material = null
	enemy_sprite.modulate = Color.WHITE

## 보스 페이즈 2 드라마틱 전환 (프리즈 프레임 + 화면 변색 + 적 분노 광원)
func _on_phase_changed(enemy_name: String, phase: int) -> void:
	if phase != 2:
		return
	# 1. 프리즈 프레임 (0.4초)
	get_tree().paused = true
	# 2. 화면 적색 플래시
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(0.8, 0.1, 0.05, 0.6)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	flash.z_index = 95
	canvas_root.add_child(flash)
	# 3. 경고 텍스트
	var warn = Label.new()
	warn.text = "— PHASE 2 —"
	warn.add_theme_font_size_override("font_size", 36)
	warn.add_theme_color_override("font_color", Color(1, 0.3, 0.2))
	warn.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	warn.set_anchors_preset(Control.PRESET_CENTER)
	warn.position = Vector2(640 - 100, 280)
	warn.z_index = 96
	canvas_root.add_child(warn)
	# 프리즈 해제 후 페이드
	await get_tree().create_timer(0.4, true, false, true).timeout
	get_tree().paused = false
	# 강한 셰이크
	_screen_shake(3.0)
	# 색수차
	_play_chromatic_aberration(2.0)
	# 적 분노 틴트 — outline_glow 셰이더
	var glow_path = "res://addons/vfx_lib/shaders/outline_glow.gdshader"
	if enemy_sprite and ResourceLoader.exists(glow_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(glow_path)
		mat.set_shader_parameter("outline_color", Color(1.0, 0.2, 0.1, 1.0))
		mat.set_shader_parameter("outline_width", 3.0)
		mat.set_shader_parameter("glow_intensity", 1.5)
		enemy_sprite.material = mat
		# 2초 후 페이드
		var gt = create_tween()
		gt.tween_interval(2.0)
		gt.tween_callback(func(): enemy_sprite.material = null)
	# 경고 페이드아웃
	var wt = create_tween()
	wt.tween_property(warn, "modulate:a", 0.0, 1.0).set_delay(0.5)
	wt.tween_callback(warn.queue_free)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.6)
	ft.tween_callback(flash.queue_free)

## S46: 상태이상 셰이더 업데이트 (status_changed 시그널에 연동)
func _update_status_shaders() -> void:
	_apply_status_shader()

## ===================== S46: 세이블 명령 UI =====================

func _build_ally_command_ui(root: Control) -> void:
	if not BattleManager.sable_in_party:
		return
	ally_cmd_container = HBoxContainer.new()
	ally_cmd_container.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	ally_cmd_container.anchor_left = 0.0
	ally_cmd_container.anchor_right = 0.35
	ally_cmd_container.anchor_top = 0.78
	ally_cmd_container.anchor_bottom = 0.84
	ally_cmd_container.offset_left = 10
	ally_cmd_container.add_theme_constant_override("separation", 4)

	var lbl = Label.new()
	lbl.text = "Sable:"
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.add_theme_color_override("font_color", Color(0.7, 0.85, 0.95))
	ally_cmd_container.add_child(lbl)

	var cmds = [["Heal", "heal"], ["Strike", "strike"], ["Weaken", "weaken"], ["Guard", "guard"]]
	for cmd in cmds:
		var btn = Button.new()
		btn.text = cmd[0]
		btn.custom_minimum_size = Vector2(52, 22)
		btn.add_theme_font_size_override("font_size", 10)
		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.15, 0.18, 0.25, 0.9)
		style.border_color = Color(0.4, 0.55, 0.7, 0.7)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		btn.add_theme_stylebox_override("normal", style)
		var hover = style.duplicate()
		hover.bg_color = Color(0.25, 0.35, 0.5, 0.95)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_color_override("font_color", Color(0.8, 0.85, 0.9))
		var action_name = cmd[1]
		btn.pressed.connect(func(): _on_ally_cmd(action_name))
		ally_cmd_container.add_child(btn)

	ally_cmd_container.visible = false
	root.add_child(ally_cmd_container)

func _on_ally_cmd(action: String) -> void:
	BattleManager.set_ally_command(action)
	AudioManager.play_sfx("ui_select")
	# 선택 확인 — 버튼 하이라이트
	if ally_cmd_container:
		for i in range(1, ally_cmd_container.get_child_count()):
			var btn = ally_cmd_container.get_child(i)
			if btn is Button:
				var is_selected = (btn.text.to_lower() == action)
				btn.modulate = Color(0.5, 1.0, 0.5) if is_selected else Color.WHITE

## ===================== S53: 토비아스 명령 UI =====================

func _build_tobias_command_ui(root: Control) -> void:
	if not BattleManager.tobias_in_party:
		return
	tobias_cmd_container = HBoxContainer.new()
	tobias_cmd_container.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	tobias_cmd_container.anchor_left = 0.0
	tobias_cmd_container.anchor_right = 0.35
	tobias_cmd_container.anchor_top = 0.84
	tobias_cmd_container.anchor_bottom = 0.90
	tobias_cmd_container.offset_left = 10
	tobias_cmd_container.add_theme_constant_override("separation", 4)

	var lbl = Label.new()
	lbl.text = "Tobias:"
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.add_theme_color_override("font_color", Color(0.85, 0.75, 0.55))
	tobias_cmd_container.add_child(lbl)

	var cmds = [["Analyze", "analyze"], ["Archive", "archive"], ["Protect", "protect"]]
	for cmd in cmds:
		var btn = Button.new()
		btn.text = cmd[0]
		btn.custom_minimum_size = Vector2(52, 22)
		btn.add_theme_font_size_override("font_size", 10)
		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.2, 0.18, 0.12, 0.9)
		style.border_color = Color(0.6, 0.5, 0.3, 0.7)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		btn.add_theme_stylebox_override("normal", style)
		var hover = style.duplicate()
		hover.bg_color = Color(0.35, 0.3, 0.2, 0.95)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_color_override("font_color", Color(0.9, 0.85, 0.7))
		var action_name = cmd[1]
		btn.pressed.connect(func(): _on_tobias_cmd(action_name))
		tobias_cmd_container.add_child(btn)

	tobias_cmd_container.visible = false
	root.add_child(tobias_cmd_container)

func _on_tobias_cmd(action: String) -> void:
	BattleManager.set_tobias_command(action)
	AudioManager.play_sfx("ui_select")
	if tobias_cmd_container:
		for i in range(1, tobias_cmd_container.get_child_count()):
			var btn = tobias_cmd_container.get_child(i)
			if btn is Button:
				var is_selected = (btn.text.to_lower() == action)
				btn.modulate = Color(0.5, 1.0, 0.5) if is_selected else Color.WHITE

## ===================== S51: 스탠스 전환 UI =====================

var stance_container: HBoxContainer
var stance_buttons: Array[Button] = []

func _build_stance_ui(root: Control) -> void:
	stance_container = HBoxContainer.new()
	stance_container.anchor_left = 0.35
	stance_container.anchor_right = 0.65
	stance_container.anchor_top = 0.78
	stance_container.anchor_bottom = 0.84
	stance_container.add_theme_constant_override("separation", 6)
	stance_container.alignment = BoxContainer.ALIGNMENT_CENTER

	var lbl = Label.new()
	lbl.text = "Stance:"
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.add_theme_color_override("font_color", Color(0.8, 0.7, 0.5))
	stance_container.add_child(lbl)

	var stances = [
		[BattleManager.Stance.REMNANT, "Remnant", Color(0.6, 0.55, 0.45)],
		[BattleManager.Stance.PYRE, "Pyre", Color(0.85, 0.4, 0.25)],
		[BattleManager.Stance.HOLLOW, "Hollow", Color(0.4, 0.35, 0.7)],
	]

	for s in stances:
		var btn = Button.new()
		btn.text = s[1]
		btn.custom_minimum_size = Vector2(65, 24)
		btn.add_theme_font_size_override("font_size", 10)
		var style = StyleBoxFlat.new()
		style.bg_color = Color(s[2].r * 0.3, s[2].g * 0.3, s[2].b * 0.3, 0.9)
		style.border_color = Color(s[2].r * 0.5, s[2].g * 0.5, s[2].b * 0.5, 0.6)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		btn.add_theme_stylebox_override("normal", style)
		var hover = style.duplicate()
		hover.bg_color = Color(s[2].r * 0.5, s[2].g * 0.5, s[2].b * 0.5, 0.95)
		hover.border_color = s[2]
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_color_override("font_color", Color(0.8, 0.75, 0.7))
		# 해금 체크
		var stance_val = s[0]
		var info = BattleManager.STANCE_INFO[stance_val]
		if GameManager.current_chapter < info["unlock_chapter"]:
			btn.disabled = true
			btn.modulate.a = 0.4
		else:
			btn.pressed.connect(func(): _on_stance_select(stance_val))
		btn.focus_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
		stance_container.add_child(btn)
		stance_buttons.append(btn)

	stance_container.visible = false
	root.add_child(stance_container)
	_update_stance_highlight()

func _on_stance_select(stance: int) -> void:
	BattleManager.switch_stance(stance)
	AudioManager.play_sfx("ui_select")
	_update_stance_highlight()

func _on_stance_changed(_stance: int) -> void:
	_update_stance_highlight()

func _update_stance_highlight() -> void:
	var names = ["Remnant", "Pyre", "Hollow"]
	for i in range(stance_buttons.size()):
		var btn = stance_buttons[i]
		var is_active = (i == BattleManager.current_stance)
		if is_active:
			btn.modulate = Color(1.0, 1.0, 0.7) if not btn.disabled else Color(0.4, 0.4, 0.4, 0.4)
		else:
			btn.modulate = Color.WHITE if not btn.disabled else Color(0.4, 0.4, 0.4, 0.4)

## ===================== S51: 에코 표시 =====================

var echo_display: VBoxContainer

func _build_echo_display(root: Control) -> void:
	echo_display = VBoxContainer.new()
	echo_display.anchor_left = 0.7
	echo_display.anchor_right = 0.98
	echo_display.anchor_top = 0.42
	echo_display.anchor_bottom = 0.65
	echo_display.add_theme_constant_override("separation", 2)
	root.add_child(echo_display)

func _on_echo_activated(_echo_type: String, _desc: String) -> void:
	_refresh_echo_display()

func _refresh_echo_display() -> void:
	if not echo_display:
		return
	for child in echo_display.get_children():
		child.queue_free()
	if BattleManager.active_echoes.is_empty():
		return
	var header = Label.new()
	header.text = "— Active Echoes —"
	header.add_theme_font_size_override("font_size", 10)
	header.add_theme_color_override("font_color", Color(0.7, 0.55, 0.35))
	echo_display.add_child(header)
	var echo_colors = {
		"fading_warmth": Color(0.4, 0.7, 0.4),
		"lingering_habit": Color(0.55, 0.5, 0.35),
		"elia_anchor": Color(0.4, 0.5, 0.7),
		"sable_shadow": Color(0.5, 0.4, 0.6),
		"bond_fracture": Color(0.6, 0.45, 0.45),
		"identity_fracture": Color(0.6, 0.3, 0.7),
		"total_erasure": Color(0.9, 0.6, 0.2),
	}
	for echo in BattleManager.active_echoes:
		var lbl = Label.new()
		var echo_name = echo["type"].replace("_", " ").capitalize()
		lbl.text = "%s (%dt)" % [echo_name, echo.get("turns", 0)]
		lbl.add_theme_font_size_override("font_size", 9)
		lbl.add_theme_color_override("font_color", echo_colors.get(echo["type"], Color(0.6, 0.6, 0.6)))
		echo_display.add_child(lbl)

## ===================== S51: 엘리아 기술 UI =====================

var elia_skill_container: VBoxContainer

func _build_elia_skill_ui(root: Control) -> void:
	if not GameManager.player_data.elia_with_party:
		return
	elia_skill_container = VBoxContainer.new()
	elia_skill_container.anchor_left = 0.0
	elia_skill_container.anchor_right = 0.22
	elia_skill_container.anchor_top = 0.42
	elia_skill_container.anchor_bottom = 0.68
	elia_skill_container.offset_left = 10
	elia_skill_container.add_theme_constant_override("separation", 3)

	var header = Label.new()
	header.text = "— Elia —"
	header.add_theme_font_size_override("font_size", 11)
	header.add_theme_color_override("font_color", Color(0.75, 0.6, 0.85))
	elia_skill_container.add_child(header)

	elia_skill_container.visible = false
	root.add_child(elia_skill_container)

func _refresh_elia_skills() -> void:
	if not elia_skill_container:
		return
	# 헤더 외 기존 버튼 제거
	while elia_skill_container.get_child_count() > 1:
		elia_skill_container.get_child(elia_skill_container.get_child_count() - 1).queue_free()
		elia_skill_container.remove_child(elia_skill_container.get_child(elia_skill_container.get_child_count() - 1))

	var skills = EliaDiary.get_available_skills()
	if skills.is_empty():
		var no_skill = Label.new()
		no_skill.text = "(no techniques)"
		no_skill.add_theme_font_size_override("font_size", 9)
		no_skill.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
		elia_skill_container.add_child(no_skill)
		return

	for skill in skills:
		var btn = Button.new()
		var cd_text = " [READY]" if skill["ready"] else " [%dT]" % skill["cooldown"]
		btn.text = "%s%s" % [skill["name"], cd_text]
		btn.tooltip_text = skill["desc"]
		btn.custom_minimum_size = Vector2(140, 24)
		btn.add_theme_font_size_override("font_size", 10)
		btn.disabled = not skill["ready"]

		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.18, 0.12, 0.25, 0.9) if skill["ready"] else Color(0.1, 0.08, 0.12, 0.6)
		style.border_color = Color(0.6, 0.45, 0.75, 0.7) if skill["ready"] else Color(0.3, 0.25, 0.3, 0.4)
		style.set_border_width_all(1)
		style.set_corner_radius_all(3)
		btn.add_theme_stylebox_override("normal", style)
		var hover = style.duplicate()
		hover.bg_color = Color(0.3, 0.2, 0.4, 0.95)
		hover.border_color = Color(0.8, 0.6, 0.9, 0.9)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_color_override("font_color", Color(0.85, 0.75, 0.95) if skill["ready"] else Color(0.4, 0.35, 0.4))

		var skill_id = skill["id"]
		btn.pressed.connect(func(): _on_elia_skill(skill_id))
		elia_skill_container.add_child(btn)

func _on_elia_skill(skill_id: String) -> void:
	AudioManager.play_sfx("ui_select")
	BattleManager.player_use_elia_skill(skill_id)
	_refresh_elia_skills()

## ===================== S52: 전투 VFX 강화 =====================

## 크리티컬 히트 줌 펀치 — 강력한 공격 시 화면 줌인→복귀
func _critical_zoom_punch() -> void:
	# 전투 루트 스케일로 줌 효과 근사
	var original_scale = canvas_root.scale
	var original_pivot = canvas_root.pivot_offset
	canvas_root.pivot_offset = canvas_root.size / 2.0 if canvas_root.size != Vector2.ZERO else Vector2(640, 360)

	# 빠른 줌인
	var t = create_tween()
	t.tween_property(canvas_root, "scale", Vector2(1.08, 1.08), 0.1).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.tween_property(canvas_root, "scale", Vector2(1.0, 1.0), 0.3).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_ELASTIC)

	# 크리티컬 플래시 — 밝은 임팩트 프레임
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 0.95, 0.8, 0.35)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_root.add_child(flash)
	var ft = create_tween()
	ft.tween_property(flash, "color:a", 0.0, 0.25).set_ease(Tween.EASE_OUT)
	ft.tween_callback(flash.queue_free)

## 연소 임팩트 — 기억 연소 시 화면 가장자리 불타는 효과
func _burn_edge_flare() -> void:
	var flare = ColorRect.new()
	flare.set_anchors_preset(Control.PRESET_FULL_RECT)
	flare.mouse_filter = Control.MOUSE_FILTER_IGNORE

	# 비네트 스타일 화염 테두리
	var shader_path = "res://assets/shaders/vignette.gdshader"
	if ResourceLoader.exists(shader_path):
		var mat = ShaderMaterial.new()
		mat.shader = load(shader_path)
		mat.set_shader_parameter("color", Color(0.9, 0.4, 0.1, 0.5))
		mat.set_shader_parameter("radius", 0.6)
		mat.set_shader_parameter("softness", 0.4)
		flare.material = mat
	else:
		flare.color = Color(0.8, 0.3, 0.1, 0.15)

	canvas_root.add_child(flare)
	var t = create_tween()
	t.tween_property(flare, "modulate:a", 0.0, 0.6).set_ease(Tween.EASE_OUT)
	t.tween_callback(flare.queue_free)

## 속성 임팩트 배경 플래시 (속성별 색상)
func _element_flash(element: String) -> void:
	var flash_color: Color
	match element:
		"fire": flash_color = Color(1.0, 0.4, 0.1, 0.2)
		"void": flash_color = Color(0.5, 0.2, 0.8, 0.2)
		"physical": flash_color = Color(0.9, 0.9, 0.9, 0.15)
		_: flash_color = Color(0.8, 0.7, 0.3, 0.15)

	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = flash_color
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas_root.add_child(flash)
	var t = create_tween()
	t.tween_property(flash, "color:a", 0.0, 0.3).set_ease(Tween.EASE_OUT)
	t.tween_callback(flash.queue_free)
