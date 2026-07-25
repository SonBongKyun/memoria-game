extends Node

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "en"
	GameManager.current_chapter = 6
	GameManager.player_data.items = {"potion": 2, "antidote": 1, "firebomb": 1}
	BattleManager.current_enemy = BattleManager.Enemy.new("Shade Sentinel", 160, 18, true)
	BattleManager.current_enemy.weakness = "void"
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/the_seam.tscn"
	BattleManager.enemy_image = "res://assets/cg/character_shots/shade_sentinel_guard_v3.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/story_ch5_seam_first_light.png"

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame

	var command_art := battle.get("action_ribbon_art") as TextureRect
	var readout_art := battle.get("field_readout_art") as TextureRect
	var readout_header := battle.get("field_readout_header") as Label
	var readout_text := battle.get("log_label") as RichTextLabel
	var action_buttons := battle.get("_action_buttons") as Dictionary
	assert(command_art != null and command_art.texture is AtlasTexture, "Battle command deck must use the cropped generated frame")
	assert((command_art.texture as AtlasTexture).atlas.resource_path.ends_with("ui_battle_command_deck_v4.png"), "Battle command deck must use the selected v4 art")
	assert(readout_art != null and readout_art.texture is AtlasTexture, "Battle readout must use the compact generated frame")
	assert((readout_art.texture as AtlasTexture).atlas.resource_path.ends_with("ui_battle_field_readout_v4.png"), "Battle readout must use the selected v4 art")
	assert(readout_art.size.y < 90.0, "Battle readout must preserve the center of the arena")
	assert(action_buttons.size() == 8, "Command deck must retain all eight battle actions")
	for action_id in action_buttons:
		var button := action_buttons[action_id] as Button
		assert(button != null and button.get_meta("action_id", "") == action_id, "Each command must expose a stable action identity")
		assert("\n" in button.text and button.tooltip_text != "", "Each command must explain its tactical role")
	for index in range(battle.action_container.get_child_count()):
		assert((battle.action_container.get_child(index) as Button).text.begins_with("%d ·" % (index + 1)), "Command hotkeys must be visible in deck order")

	assert(battle.get("player_shadow") is Polygon2D, "Arrel must use an elliptical contact shadow instead of a rectangular UI block")
	assert(battle.get("enemy_shadow") is Polygon2D, "Enemies must use an elliptical contact shadow instead of a rectangular UI block")
	battle.call("_show_action_forecast", "burn")
	assert(readout_header.text == "IRREVERSIBLE COST" and "gone after battle" in readout_text.text, "BURN focus must preview its permanent story cost")
	battle.call("_show_action_forecast", "witness")
	assert(readout_header.text == "PRESERVATION ROUTE" and "without burning" in readout_text.text, "WITNESS focus must preview the preservation route")
	battle.call("_on_battle_log", "First beat")
	battle.call("_on_battle_log", "Second beat")
	battle.call("_on_battle_log", "Third beat")
	assert((battle.get("log_lines") as Array).size() == 2, "The field readout must not accumulate an obstructive wall of text")
	assert(battle.get("_last_battle_message") == "Third beat", "The newest combat consequence must remain readable")

	print("BATTLE_COMMAND_DECK_SMOKE_PASS actions=%d readout_height=%.1f player_scale=%s" % [
		action_buttons.size(),
		readout_art.size.y,
		str((battle.get("player_sprite") as AnimatedSprite2D).scale),
	])
	get_tree().quit(0)
