extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/rim_forest_first_exploration.png"

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["screen_shake"] = false
	GameManager.current_chapter = 1
	GameManager.set_flag("ch1_opening_done")
	GameManager.set_flag("ch1_elia_appeared")
	GameManager.set_flag("ch1_ash_rain_seen")
	SceneFlow.resume_queue = [{"scene": "capture"}]
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var packed: PackedScene = load("res://scenes/maps/rim_forest.tscn")
	var map = packed.instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame
	ExplorationHUD.call("_on_state_changed", GameManager.GameState.EXPLORATION)
	await get_tree().create_timer(0.6).timeout
	map.get_node("Player").call("_spawn_step_echo")
	await get_tree().create_timer(0.06).timeout

	var player_sprite := map.get_node("Player/AnimatedSprite2D") as AnimatedSprite2D
	var elia_sprite := map.get_node("Elia/CharacterSprite") as AnimatedSprite2D
	var player_texture := player_sprite.sprite_frames.get_frame_texture("idle_down", 0)
	var elia_texture := elia_sprite.sprite_frames.get_frame_texture("idle_down", 0)
	assert("arrel_sheet" in PixelSprite.get_texture_source(player_texture))
	assert("elia_sheet" in PixelSprite.get_texture_source(elia_texture))
	assert(ExplorationHUD.hud_plate_art == null or not ExplorationHUD.hud_plate_art.visible)
	assert(not ExplorationHUD.controls_panel.visible)
	assert(not ExplorationHUD.location_card.visible)

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("FIRST_EXPLORATION_CAPTURE_PASS path=%s arrel=%s elia=%s" % [OUTPUT_PATH, PixelSprite.get_texture_source(player_texture), PixelSprite.get_texture_source(elia_texture)])
	get_tree().quit(0)
