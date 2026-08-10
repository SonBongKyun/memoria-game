extends Node

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	# S230: 서체 역할 분리 — 이야기는 세리프, 인터페이스는 산세리프.
	assert(UITheme.BODY_FONT_PATH.ends_with("NotoSerifKR-VF.ttf"), "Story copy must use the bundled Korean serif")
	assert(UITheme.TITLE_FONT_PATH.ends_with("NotoSerifKR-VF.ttf"), "Titles must use the bundled Korean serif")
	assert(UITheme.UI_FONT_PATH.ends_with("NotoSansKR-VF.ttf"), "Interface copy must use the bundled Korean sans")
	# S230: 굵기는 가짜 embolden이 아니라 실제 wght 축에서 와야 한다.
	# 두 폰트 모두 기본 인스턴스가 가장 얇은 마스터라, 축을 지정하지 않으면 Thin으로 렌더된다.
	for role_font in [UITheme.make_body_font(), UITheme.make_ui_font(), UITheme.make_title_font(), UITheme.make_meta_font()]:
		assert(role_font is FontVariation, "Every text role must resolve to a weighted variable-font instance")
		assert(is_zero_approx((role_font as FontVariation).variation_embolden), "Synthetic embolden must stay off; weight comes from the wght axis")
	assert(UITheme.get_font_weight(UITheme.make_body_font()) >= UITheme.WEIGHT_MEDIUM, "Body copy needs a real medium weight")
	assert(UITheme.get_font_weight(UITheme.make_ui_font()) >= UITheme.WEIGHT_SEMIBOLD, "Compact UI copy needs a real semibold weight")
	assert(UITheme.get_font_weight(UITheme.make_meta_font()) >= UITheme.WEIGHT_MEDIUM, "Meta chips need a real medium weight")
	# 명시적 override가 없는 컨트롤도 같은 굵기로 그려져야 한다.
	var project_theme := load(ProjectSettings.get_setting("gui/theme/custom", "")) as Theme
	assert(project_theme != null, "The project theme must stay loadable")
	assert(UITheme.get_font_weight(project_theme.default_font) >= UITheme.WEIGHT_SEMIBOLD, "The project default font must carry a real weight axis")
	assert(project_theme.default_font_size >= UITheme.MIN_UI_FONT_SIZE, "The project default size must clear the UI floor")
	assert(_contrast_ratio(UITheme.TEXT_PRIMARY, UITheme.BG_PANEL) >= 12.0, "Primary text contrast is below the readability contract")
	assert(_contrast_ratio(UITheme.TEXT_NARRATION, UITheme.BG_PANEL) >= 9.0, "Narration contrast is below the readability contract")
	assert(_contrast_ratio(UITheme.TEXT_DIM, UITheme.BG_PANEL) >= 5.5, "Secondary text contrast is below the readability contract")

	await get_tree().process_frame
	var dialogue_style := DialogueBox.panel.get_theme_stylebox("panel") as StyleBoxFlat
	assert(dialogue_style != null and dialogue_style.bg_color.a >= 0.95, "Dialogue copy needs an opaque reading surface")
	assert(DialogueBox.call("_get_dialogue_font_size") >= UITheme.MIN_BODY_FONT_SIZE, "Dialogue normal size is below the body-copy minimum")
	assert(DialogueBox.text_label.get_theme_font_size("normal_font_size") >= UITheme.MIN_BODY_FONT_SIZE, "Dialogue label did not receive the readable size")
	assert(DialogueBox.text_label.get_theme_constant("outline_size") >= 1, "Dialogue copy needs a one-pixel outline")

	assert(ExplorationHUD.controls_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Exploration controls are too small")
	assert(ExplorationHUD.approach_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Field approach state is too small")
	assert(ExplorationHUD.flow_hint_label.get_theme_font_size("font_size") >= UITheme.MIN_META_FONT_SIZE, "Field Flow hint is too small")
	var flow_style := ExplorationHUD.flow_panel.get_theme_stylebox("panel") as StyleBoxFlat
	assert(flow_style != null and flow_style.bg_color.a >= 0.95, "Field Flow needs an opaque reading surface")
	# S230: 스타일박스만 검사하면 부족하다. 런타임 modulate가 그 위에 곱해지므로,
	# 조용한 상태에서 패널 전체를 흐리게 만들면 검사를 통과한 채로 글자가 사라진다.
	ExplorationHUD.call("_update_field_flow_hud")
	var effective_alpha := flow_style.bg_color.a * ExplorationHUD.flow_panel.modulate.a
	assert(effective_alpha >= 0.80, "Field Flow copy must stay readable in its quiet state (effective alpha %.2f)" % effective_alpha)
	assert(MemoryCompass.compass_title.get_theme_font_size("font_size") >= UITheme.MIN_META_FONT_SIZE, "Memory Compass title is too small")
	assert(MemoryCompass.lore_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Memory Compass lore is too small")
	var compass_style := MemoryCompass.panel.get_theme_stylebox("panel") as StyleBoxFlat
	assert(compass_style != null and compass_style.bg_color.a >= 0.95, "Memory Compass needs an opaque reading surface")

	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
	GameManager.change_state(GameManager.GameState.BATTLE)
	BattleManager.current_enemy = BattleManager.Enemy.new("Readability Sentinel", 160, 18, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/the_seam.tscn"
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame

	assert(battle.objective_title_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Battle objective title is too small")
	assert(battle.objective_desc_label.get_theme_font_size("font_size") >= 14, "Battle objective detail is too small")
	assert(battle.enemy_hp_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Enemy HP is too small")
	assert(battle.player_hp_label.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Player HP is too small")
	assert(battle.field_readout_header.get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Field readout header is too small")
	assert(battle.log_label.get_theme_font_size("normal_font_size") >= 15, "Field readout copy is too small")
	for button in battle._action_buttons.values():
		assert((button as Button).get_theme_font_size("font_size") >= 15, "Battle command is too small")
	var objective_style := battle.objective_panel.get_theme_stylebox("panel") as StyleBoxFlat
	assert(objective_style != null and objective_style.bg_color.a >= 0.90, "Battle objective needs an opaque reading surface")

	print("TEXT_READABILITY_SMOKE_PASS body=20 ui_floor=12 battle=15 contrast=high weight=medium")
	get_tree().quit()

func _contrast_ratio(foreground: Color, background: Color) -> float:
	var lighter := maxf(_relative_luminance(foreground), _relative_luminance(background))
	var darker := minf(_relative_luminance(foreground), _relative_luminance(background))
	return (lighter + 0.05) / (darker + 0.05)

func _relative_luminance(color: Color) -> float:
	return 0.2126 * _linear_channel(color.r) + 0.7152 * _linear_channel(color.g) + 0.0722 * _linear_channel(color.b)

func _linear_channel(value: float) -> float:
	return value / 12.92 if value <= 0.04045 else pow((value + 0.055) / 1.055, 2.4)
