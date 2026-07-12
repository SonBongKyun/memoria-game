extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/verdan_malet_field.png"

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["screen_shake"] = false
	GameManager.current_chapter = 2
	GameManager.set_flag("ch2_arrived")
	GameManager.set_flag("ch2_arrival_vn_seen")
	GameManager.set_flag("prop_campfire_416_352")
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var packed: PackedScene = load("res://scenes/maps/verdan_market.tscn")
	var map = packed.instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame
	var player := map.get_node("Player") as CharacterBody2D
	var malet := map.get_node("Malet") as StaticBody2D
	map.get_node("Elia").visible = false
	player.global_position = malet.global_position + Vector2(-28, 0)
	player.facing_direction = Vector2.RIGHT
	player.call("_update_animation", Vector2.RIGHT, false)
	player.call("_update_raycast_direction")
	malet.call("_face_toward_player")
	await get_tree().create_timer(0.7).timeout

	var malet_sprite := malet.get_node("CharacterSprite") as AnimatedSprite2D
	var texture := malet_sprite.sprite_frames.get_frame_texture(malet_sprite.animation, 0)
	assert("malet_sheet" in PixelSprite.get_texture_source(texture))
	assert(malet_sprite.animation == "idle_left")
	var interaction_chip := player.get("_interact_indicator") as Label
	assert(interaction_chip.visible and ("대화" in interaction_chip.text or "Talk" in interaction_chip.text))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("MALET_FIELD_CAPTURE_PASS path=%s animation=%s interaction=%s texture=%s" % [OUTPUT_PATH, malet_sprite.animation, interaction_chip.text, PixelSprite.get_texture_source(texture)])
	get_tree().quit(0)
