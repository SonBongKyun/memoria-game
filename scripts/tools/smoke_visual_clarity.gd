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
	assert(ExplorationHUD.identity_label != null and ("ARREL" in ExplorationHUD.identity_label.text or "아렐" in ExplorationHUD.identity_label.text), "Field HUD must lead with a clear player identity header")
	assert(ExplorationHUD.chapter_label != null and ExplorationHUD.chapter_label.text.length() > 0, "Field HUD must retain chapter and location context in its header")
	assert(ExplorationHUD.quest_card != null and ExplorationHUD.quest_tag_label != null, "Exploration HUD must frame the active story objective")
	assert("STORY" in ExplorationHUD.quest_tag_label.text or "이야기" in ExplorationHUD.quest_tag_label.text, "Story objective card must expose a readable hierarchy label")
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
	assert(arrel_texture != null and "sprites/field/arrel" in PixelSprite.get_texture_source(arrel_texture), "Exploration Arrel must use the four-direction field sprite")
	assert(arrel_texture.resource_path != "", "Field sprites must resolve as imported project assets")
	var arrel_right := field_player_sprite.sprite_frames.get_frame_texture("idle_right", 0)
	var arrel_left := field_player_sprite.sprite_frames.get_frame_texture("idle_left", 0)
	var arrel_up := field_player_sprite.sprite_frames.get_frame_texture("idle_up", 0)
	assert("/right.png" in PixelSprite.get_texture_source(arrel_right) and "/left.png" in PixelSprite.get_texture_source(arrel_left), "Arrel must visibly turn left and right using dedicated field poses")
	assert("/up.png" in PixelSprite.get_texture_source(arrel_up), "Arrel's upward pose must use the dedicated rear-facing frame")
	assert(PixelSprite.get_texture_source(arrel_texture) != PixelSprite.get_texture_source(arrel_up), "Front and rear silhouettes must not reuse one texture")
	var interact_chip := field_player.get("_interact_indicator") as Label
	assert(interact_chip != null and interact_chip.custom_minimum_size.x >= 72.0, "Interaction feedback must use a readable action chip instead of a bare key")
	field_player.call("_spawn_step_echo")
	var has_step_echo := false
	for child in get_children():
		if child is Line2D:
			has_step_echo = true
			break
	assert(has_step_echo, "Clean movement must retain a subtle footfall echo")
	field_player.velocity = Vector2.RIGHT * 120.0
	field_player.call("_update_camera_look_ahead", 0.25)
	assert(field_player.get_node("Camera2D").offset.x > 0.0, "Clean camera must retain restrained movement anticipation")
	field_player.queue_free()
	await get_tree().process_frame

	var npc_scene: PackedScene = load("res://scenes/npc/npc.tscn")
	var field_elia = npc_scene.instantiate()
	field_elia.npc_name = "Elia"
	add_child(field_elia)
	await get_tree().process_frame
	var field_elia_sprite := field_elia.get_node("CharacterSprite") as AnimatedSprite2D
	var elia_texture := field_elia_sprite.sprite_frames.get_frame_texture("idle_down", 0)
	assert(elia_texture != null and "sprites/field/elia" in PixelSprite.get_texture_source(elia_texture), "Opening Elia must use the four-direction field sprite")
	MapEffects.update_npc_idle_motion(field_elia, 0.5)
	assert(absf(field_elia_sprite.scale.x - 0.36) < 0.01, "NPC idle motion must preserve the authored field-cast scale")
	field_elia.queue_free()
	await get_tree().process_frame

	var companion_scene: PackedScene = load("res://scenes/npc/companion.tscn")
	var field_companion = companion_scene.instantiate()
	field_companion.npc_name = "Elia"
	add_child(field_companion)
	await get_tree().process_frame
	assert(field_companion.get("_presence_ring") is Line2D and field_companion.get("_presence_shadow") is Polygon2D, "Companions must use a restrained ground-presence treatment")
	field_companion.queue_free()
	await get_tree().process_frame

	for npc_data in [
		{"name": "Malet", "sheet": "field/malet"},
		{"name": "Tobias", "sheet": "field/tobias"},
		{"name": "Kairos", "sheet": "field/kairos"},
		{"name": "Nera", "sheet": "field/nera"},
		{"name": "Veil", "sheet": "field/veil"},
	]:
		var authored_npc = npc_scene.instantiate()
		authored_npc.npc_name = npc_data.name
		add_child(authored_npc)
		await get_tree().process_frame
		var authored_sprite := authored_npc.get_node("CharacterSprite") as AnimatedSprite2D
		var authored_down := authored_sprite.sprite_frames.get_frame_texture("idle_down", 0)
		var authored_right := authored_sprite.sprite_frames.get_frame_texture("walk_right", 0)
		var authored_left := authored_sprite.sprite_frames.get_frame_texture("walk_left", 0)
		assert(npc_data.sheet in PixelSprite.get_texture_source(authored_down), "%s must use its authored field sheet" % npc_data.name)
		assert("/right.png" in PixelSprite.get_texture_source(authored_right) and "/left.png" in PixelSprite.get_texture_source(authored_left), "%s must expose dedicated left/right field poses" % npc_data.name)
		assert(is_equal_approx(authored_sprite.scale.x, 0.36), "%s must share the unified field-cast scale" % npc_data.name)
		authored_npc.queue_free()
		await get_tree().process_frame
	assert(PixelSprite.has_field_sprite_frames("sable"), "Sable must share the same four-direction field-cast contract")
	for direction in ["down", "up", "left", "right"]:
		assert(ResourceLoader.exists("res://assets/sprites/field/sable/%s.png" % direction), "Sable is missing a %s field direction" % direction)
	assert(UITheme.make_body_font().font_names[0] == "Noto Serif KR", "Korean dialogue must use the literary Hangul serif-first font chain")
	for canvas_path in [
		"res://assets/environment/map_canvases/map_rim_forest_canvas_v1.png",
		"res://assets/environment/map_canvases/map_verdan_market_canvas_v1.png",
		"res://assets/environment/map_canvases/map_belt_waystation_canvas_v1.png",
		"res://assets/environment/map_canvases/map_crumbling_coast_canvas_v1.png",
		"res://assets/environment/map_canvases/map_the_seam_canvas_v1.png",
		"res://assets/environment/map_canvases/map_bl07_void_canvas_v1.png",
		"res://assets/environment/map_canvases/map_drift_shelter_canvas_v2.png",
		"res://assets/environment/map_canvases/map_forgotten_forest_canvas_v2.png",
		"res://assets/environment/map_canvases/map_colorless_waste_canvas_v2.png",
		"res://assets/environment/map_canvases/map_seam_outskirts_canvas_v2.png",
		"res://assets/environment/map_canvases/map_rim_root_hollow_canvas_v1.png",
		"res://assets/environment/map_canvases/map_verdan_ledger_cellar_canvas_v1.png",
		"res://assets/environment/map_canvases/map_belt_signal_yard_canvas_v1.png",
		"res://assets/environment/map_canvases/map_drift_waymarker_shrine_canvas_v1.png",
		"res://assets/environment/map_canvases/map_coast_cinder_harbor_canvas_v1.png",
		"res://assets/environment/map_canvases/map_seam_lantern_ward_canvas_v1.png",
		"res://assets/environment/map_canvases/map_forest_name_hollow_canvas_v1.png",
		"res://assets/environment/map_canvases/map_waste_grey_caravan_canvas_v1.png",
		"res://assets/environment/map_canvases/map_bl07_seed_vault_canvas_v1.png",
	]:
		assert(ResourceLoader.exists(canvas_path), "Map canvas is missing: %s" % canvas_path)
	for site_scene in [
		"res://scenes/maps/rim_root_hollow.tscn", "res://scenes/maps/verdan_ledger_cellar.tscn",
		"res://scenes/maps/belt_signal_yard.tscn", "res://scenes/maps/drift_waymarker_shrine.tscn",
		"res://scenes/maps/coast_cinder_harbor.tscn", "res://scenes/maps/seam_lantern_ward.tscn",
		"res://scenes/maps/forest_name_hollow.tscn", "res://scenes/maps/waste_grey_caravan.tscn",
		"res://scenes/maps/bl07_seed_vault.tscn",
	]:
		assert(ResourceLoader.exists(site_scene), "Optional story site is missing: %s" % site_scene)
	assert("Quick Save" not in PauseMenu.pause_hint_label.text, "Controller footer must not advertise unsupported quick-save buttons")
	InputManager.current_mode = previous_mode
	PauseMenu.call("_refresh_footer_hints")

	var map := Node2D.new()
	add_child(map)
	assert(TilePainter._clean_detail_name("stone") == "masonry_clean" and TilePainter._clean_detail_name("path") == "path_clean", "Clean view must replace per-pixel terrain noise with broad value groups")
	var story_trigger := Area2D.new()
	map.add_child(story_trigger)
	MapEffects.update_trigger_approach_glow(map, Vector2.ZERO, 0.5)
	var has_compact_beacon := false
	for child in story_trigger.get_children():
		if child is ColorRect and child.has_meta("approach_glow"):
			has_compact_beacon = child.size == Vector2(8, 8)
	assert(has_compact_beacon, "Nearby story triggers must use a compact Memory beacon instead of their collision bounds")
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

	MemoryShop.open_shop("Malet")
	MemoryShop._current_mode = "items"
	MemoryShop._refresh_items()
	var illustrated_shop_buttons := 0
	for shop_entry in MemoryShop.item_list.get_children():
		if shop_entry is Button:
			var shop_button := shop_entry as Button
			if shop_button.icon != null:
				illustrated_shop_buttons += 1
	assert(illustrated_shop_buttons == GameManager.ITEMS.size(), "Malet's item tab must use the shared consumable icon family")
	MemoryShop.close_shop()

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
	var command_grid := battle_scene.get("action_container") as GridContainer
	var witness_button := battle_scene.get("witness_btn") as Button
	assert(command_grid != null and command_grid.columns == 4 and command_grid.get_child_count() == 8, "Battle commands must remain a readable 4x2 grid")
	assert(witness_button != null and ("WITNESS" in witness_button.text or "기억 읽기" in witness_button.text), "Story combat must expose the WITNESS route")
	GameManager.player_data.items = {"potion": 1, "firebomb": 1}
	battle_scene.call("_toggle_item_list")
	var item_picker := battle_scene.get("item_list_container") as VBoxContainer
	var illustrated_item_buttons := 0
	for item_entry in item_picker.get_children():
		if item_entry is Button:
			var item_button := item_entry as Button
			if item_button.icon != null:
				illustrated_item_buttons += 1
	assert(illustrated_item_buttons == 2, "Battle items must use the shared consumable icon family")
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
	tobias_battle.queue_free()
	await get_tree().process_frame
	BattleManager.tobias_in_party = false
	var elia_battle: Node = battle_packed.instantiate()
	add_child(elia_battle)
	await get_tree().process_frame
	var elia_stage := elia_battle.get("ally_sprite") as TextureRect
	assert(elia_stage != null and elia_stage.texture.resource_path == "res://assets/cg/game_image/elia_battle_anchor_fullbody.png", "Elia battle support must use the transparent anchor full-body art")

	print("VISUAL_CLARITY_SMOKE_PASS fog=0 particles=0 vignette=0 lens=0 battle_dust=0 actor_callbacks=1 ui_header=1 companion_presence=1 npc_scale=preserved map_canvases=19 support_art=3 item_icons=2 shop_icons=12 exploration_sheets=7 sheet_denoise=1 terrain_noise=low directional_turns=4 footfall_echo=1 camera_lead=1 story_beacon=1 objective_card=1 font_chain=ko command_grid=4x2 witness=1")
	get_tree().quit(0)
