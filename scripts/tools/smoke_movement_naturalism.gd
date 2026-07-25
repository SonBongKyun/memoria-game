extends Node2D

func _ready() -> void:
	var previous_state := GameManager.current_state
	var previous_clean_view: Variant = OptionsMenu.settings.get("clean_gameplay_visuals", false)
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var player_scene := load("res://scenes/player/player.tscn") as PackedScene
	var player := player_scene.instantiate() as CharacterBody2D
	add_child(player)
	await get_tree().process_frame
	assert(player.motion_mode == CharacterBody2D.MOTION_MODE_FLOATING, "Top-down movement must use floating collision response")
	var camera := player.get_node("Camera2D") as Camera2D
	assert(camera.position_smoothing_speed >= 8.0 and not camera.drag_horizontal_enabled and not camera.drag_vertical_enabled, "Camera smoothing must not stack drag lag on top of look-ahead")
	var ground_shadow := MapEffects.add_drop_shadow(player)
	assert(ground_shadow.get_child_count() == 2 and ground_shadow.get_child(0) is Polygon2D, "Moving characters must use a layered oval ground shadow, not a rectangular UI block")

	player.global_position = Vector2(100, 100)
	Input.action_press("move_right")
	var start_x := player.global_position.x
	for _frame in range(8):
		await get_tree().physics_frame
	assert(player.velocity.x > 110.0 and player.global_position.x > start_x + 5.0, "Arrel must reach walking speed responsively")

	Input.action_release("move_right")
	Input.action_press("move_left")
	for _frame in range(7):
		await get_tree().physics_frame
	assert(player.velocity.x < -30.0, "Opposite-direction input must turn decisively instead of drifting")
	Input.action_release("move_left")
	for _frame in range(7):
		await get_tree().physics_frame
	assert(player.velocity.length() < 2.0, "Released movement must settle without a long ice-slide tail")

	player.call("_update_animation", Vector2(0.72, 0.69), true)
	var held_suffix := String(player.get("_anim_suffix"))
	player.call("_update_animation", Vector2(0.69, 0.72), true)
	assert(String(player.get("_anim_suffix")) == held_suffix, "Near-diagonal input must not flicker between cardinal poses")
	player.set("_step_distance", 0.0)
	player.call("_update_footfalls", 25.0, true)
	assert(is_equal_approx(float(player.get("_step_distance")), 25.0), "Footfalls must wait for real travelled distance")
	player.call("_update_footfalls", 2.0, true)
	assert(float(player.get("_step_distance")) < 2.0, "Footfall cadence must wrap on stride distance")

	var companion_scene := load("res://scenes/npc/companion.tscn") as PackedScene
	var companion := companion_scene.instantiate() as CharacterBody2D
	companion.global_position = Vector2(40, 100)
	add_child(companion)
	await get_tree().process_frame
	await get_tree().process_frame
	assert(companion.get("target") == player, "Companion must resolve the active player")
	player.global_position = Vector2(60, 100)
	companion.call("_reset_target_trail")
	for point in [Vector2(100, 100), Vector2(100, 140), Vector2(100, 180)]:
		player.global_position = point
		companion.call("_record_target_trail")
	var follow_point: Vector2 = companion.call("_get_trail_follow_point")
	assert(absf(follow_point.x - 100.0) < 0.1 and absf(follow_point.y - 132.0) < 1.0, "Companion must follow the player's breadcrumb path through corners")
	assert(follow_point.distance_to(player.global_position) >= 46.0, "Companion formation must preserve personal space")

	var ambient := PixelSprite.create_npc_sprite("villager_f")
	add_child(ambient)
	MapEffects.add_npc_wander(ambient, 20.0)
	assert(String(ambient.animation).begins_with("walk_"), "Ambient walkers must play a directional gait while translating")
	ambient.set_meta("wander_active", false)

	Input.action_release("move_up")
	Input.action_release("move_down")
	Input.action_release("move_left")
	Input.action_release("move_right")
	OptionsMenu.settings["clean_gameplay_visuals"] = previous_clean_view
	GameManager.change_state(previous_state)
	print("MOVEMENT_NATURALISM_SMOKE_PASS acceleration=responsive turn=decisive stop=settled direction_hysteresis=1 distance_footfalls=1 breadcrumb_corner=1 camera_single_lag=1 ambient_gait=1 grounded_shadow=1")
	get_tree().quit(0)
