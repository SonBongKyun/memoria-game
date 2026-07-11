extends Node

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["screen_shake"] = false

	ExplorationHUD.call("_on_state_changed", GameManager.GameState.EXPLORATION)
	assert(ExplorationHUD.controls_panel != null and not ExplorationHUD.controls_panel.visible, "Clean exploration must not reserve a permanent controls strip")
	MemoryCompass.call("_refresh_visibility")
	assert(MemoryCompass.panel != null and not MemoryCompass.panel.visible, "Clean exploration must keep the full compass hidden until a memory event")
	assert("[Q]" in ExplorationHUD.controls_label.text, "Keyboard control strip must expose the Memory Pulse action")
	assert(ExplorationHUD.hud_plate_art == null or not ExplorationHUD.hud_plate_art.visible, "Clean view must not show a decorative HUD plate")
	assert(ExplorationHUD.location_card == null or not ExplorationHUD.location_card.visible, "Clean view must not cover the opening screen with a location-art card")
	ExplorationHUD.call("_update_hud")
	assert(not ExplorationHUD.memory_label.visible and not ExplorationHUD.grains_label.visible, "Clean view must keep archive resources out of the persistent field HUD")
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

	var player_scene: PackedScene = load("res://scenes/player/player.tscn")
	var field_player: CharacterBody2D = player_scene.instantiate()
	add_child(field_player)
	await get_tree().process_frame
	var field_player_sprite := field_player.get_node("AnimatedSprite2D") as AnimatedSprite2D
	assert(field_player_sprite != null and field_player_sprite.sprite_frames != null, "Exploration must build Arrel's sprite")
	var arrel_texture := field_player_sprite.sprite_frames.get_frame_texture("idle_down", 0)
	assert(arrel_texture != null and "arrel_sheet" in arrel_texture.resource_path, "Exploration Arrel must use the authored character sheet")
	field_player.queue_free()
	await get_tree().process_frame

	var npc_scene: PackedScene = load("res://scenes/npc/npc.tscn")
	var field_elia = npc_scene.instantiate()
	field_elia.npc_name = "Elia"
	add_child(field_elia)
	await get_tree().process_frame
	var field_elia_sprite := field_elia.get_node("CharacterSprite") as AnimatedSprite2D
	var elia_texture := field_elia_sprite.sprite_frames.get_frame_texture("idle_down", 0)
	assert(elia_texture != null and "elia_sheet" in elia_texture.resource_path, "Opening Elia must use the authored character sheet")
	field_elia.queue_free()
	await get_tree().process_frame
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
	BattleManager.sable_in_party = true
	BattleManager.tobias_in_party = false
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
	var sable_stage := battle_scene.get("ally_sprite") as TextureRect
	assert(sable_stage != null and sable_stage.texture.resource_path == "res://assets/cg/game_image/sable_battle_fullbody.png", "Sable battle support must use the canonical transparent full-body art")

	battle_scene.queue_free()
	await get_tree().process_frame
	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = true
	var tobias_battle: Node = battle_packed.instantiate()
	add_child(tobias_battle)
	await get_tree().process_frame
	var tobias_stage := tobias_battle.get("tobias_sprite") as TextureRect
	assert(tobias_stage != null and tobias_stage.texture.resource_path == "res://assets/cg/game_image/tobias_battle_fullbody.png", "Tobias battle support must use the transparent record-ward art")

	print("VISUAL_CLARITY_SMOKE_PASS fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1 ui_hints=1 support_art=2 exploration_sheets=2 compact_hud=1")
	get_tree().quit(0)
