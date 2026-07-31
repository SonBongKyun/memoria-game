extends Node

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	assert(UITheme.BODY_FONT_PATH.ends_with("NotoSansKR-VF.ttf"), "Body copy must use the screen-readable Korean sans font")
	assert(UITheme.make_body_font() is FontVariation and (UITheme.make_body_font() as FontVariation).variation_embolden >= 0.3, "Body copy needs a medium screen weight")
	assert(UITheme.make_ui_font() is FontVariation and (UITheme.make_ui_font() as FontVariation).variation_embolden >= 0.4, "Compact UI copy needs a firm screen weight")
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
