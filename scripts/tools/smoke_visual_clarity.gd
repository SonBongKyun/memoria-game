extends Node

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["screen_shake"] = false

	ExplorationHUD.call("_on_state_changed", GameManager.GameState.EXPLORATION)
	assert(ExplorationHUD.controls_panel != null and ExplorationHUD.controls_panel.visible, "Exploration controls strip must be visible during exploration")
	assert("[Q]" in ExplorationHUD.controls_label.text, "Keyboard control strip must expose the Memory Pulse action")
	assert(not ExplorationHUD._hud_uses_full_overlay or not ExplorationHUD.hud_plate_art.visible, "Clean view must not show a full-screen HUD decoration")
	DialogueBox.is_typing = true
	DialogueBox.call("_refresh_indicator_text")
	var skip_hint := DialogueBox.indicator.text
	DialogueBox.is_typing = false
	DialogueBox.call("_refresh_indicator_text")
	var continue_hint := DialogueBox.indicator.text
	assert(skip_hint != continue_hint and "[Space/Enter]" in skip_hint and "[Space/Enter]" in continue_hint, "Dialogue hints must distinguish skip from line advance")
	assert(CgViewer.continue_panel != null and CgViewer.continue_label != null, "CG viewer must expose an input-aware continue chip")
	var previous_mode = InputManager.current_mode
	InputManager.current_mode = InputManager.InputMode.CONTROLLER
	PauseMenu.call("_refresh_footer_hints")
	assert("Quick Save" not in PauseMenu.pause_hint_label.text, "Controller footer must not advertise unsupported quick-save buttons")
	InputManager.current_mode = previous_mode
	PauseMenu.call("_refresh_footer_hints")

	var map := Node2D.new()
	add_child(map)
	assert(MapEffects.add_fog(map).is_empty(), "Clean view must suppress screen fog")
	assert(MapEffects.add_heavy_fog(map).is_empty(), "Clean view must suppress heavy fog")
	assert(MapEffects.add_pollen_particles(map).is_empty(), "Clean view must suppress pollen")
	assert(MapEffects.add_void_tendrils(map).is_empty(), "Clean view must suppress void tendrils")
	assert(MapEffects.add_fog_layer(map).is_empty(), "Clean view must suppress procedural fog")
	assert(MapEffects.add_vignette(map).get_child_count() == 0, "Clean view must suppress vignette")
	assert(MapEffects.add_premium_map_lens(map).get_child_count() == 0, "Clean view must suppress lens overlays")
	assert(MapEffects.add_rain(map).get_child_count() == 0, "Clean view must suppress weather overlays")
	assert(MapEffects.add_depth_gradient(map).get_child_count() == 0, "Clean view must suppress depth wash")
	var ambient := MapEffects.add_ambient_lighting(map, Color(0.2, 0.2, 0.25))
	assert(ambient.color == Color.WHITE, "Clean view must keep the playfield neutrally lit")
	var void_particles := MapEffects.add_void_particles(map)
	assert(not void_particles.emitting, "Clean view must suppress ambient particles")

	BattleManager.current_enemy = BattleManager.Enemy.new("Clarity Dummy", 20, 1, false)
	BattleManager.return_scene = "res://scenes/maps/rim_forest.tscn"
	var battle_packed: PackedScene = load("res://scenes/battle/battle_scene.tscn")
	var battle_scene: Node = battle_packed.instantiate()
	add_child(battle_scene)
	await get_tree().process_frame
	assert(battle_scene.get("_battle_particles") == null, "Clean battle view must suppress ambient dust")
	assert((battle_scene.get("_battle_parallax_layers") as Array).is_empty(), "Clean battle view must suppress parallax haze")
	var actor := battle_scene.get("player_sprite") as AnimatedSprite2D
	assert(actor != null, "Battle smoke must build Arrel's animated sprite")
	battle_scene.call("_play_actor_anim", actor, "attack")
	battle_scene.call("_play_actor_anim", actor, "hurt")
	assert(actor.animation_finished.get_connections().size() == 1, "One-shot battle verbs must share one completion callback")

	print("VISUAL_CLARITY_SMOKE_PASS fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1 ui_hints=1")
	get_tree().quit(0)
