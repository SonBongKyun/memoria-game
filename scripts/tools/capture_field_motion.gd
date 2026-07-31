extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/verdan_field_motion.png"

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = false
	OptionsMenu.settings["screen_shake"] = false
	GameManager.current_chapter = 2
	GameManager.set_flag("ch2_arrived")
	GameManager.set_flag("ch2_arrival_vn_seen")
	GameManager.set_flag("prop_campfire_416_352")
	for trigger_flag in [
		"ch2_market_walk",
		"ch2_old_burner",
		"ch2_malet_backstory",
		"ch2_elia_concern",
		"ch2_sump_atmos",
	]:
		GameManager.set_flag(trigger_flag)
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var map := (load("res://scenes/maps/verdan_market.tscn") as PackedScene).instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame
	var player := map.get_node("Player") as CharacterBody2D
	var companion := map.get_node("Elia") as CharacterBody2D
	player.global_position = Vector2(13.0 * 32.0, 9.5 * 32.0)
	companion.global_position = player.global_position + Vector2(-54, 18)
	companion.call("_reset_target_trail")
	await get_tree().create_timer(0.25).timeout

	Input.action_press("move_right")
	Input.action_press("sprint")
	await get_tree().create_timer(0.42).timeout
	var player_sprite := player.get_node("AnimatedSprite2D") as AnimatedSprite2D
	var companion_sprite := companion.get_node("AnimatedSprite2D") as AnimatedSprite2D
	assert(player_sprite.animation.begins_with("walk_"))
	assert(player_sprite.material is ShaderMaterial and companion_sprite.material is ShaderMaterial)
	assert(player.get_node_or_null("FieldGrounding") != null)
	assert(companion.get_node_or_null("FieldGrounding") != null)

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	Input.action_release("move_right")
	Input.action_release("sprint")
	print("FIELD_MOTION_CAPTURE_PASS path=%s player=%s companion=%s speed=%.1f" % [
		OUTPUT_PATH,
		player_sprite.animation,
		companion_sprite.animation,
		player.velocity.length(),
	])
	get_tree().quit(0)
