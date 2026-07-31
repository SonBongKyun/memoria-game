extends Node

const APPROACH_OUTPUT := "res://tmp/visual_audit/field_flow_approach.png"
const PHASE_OUTPUT := "res://tmp/visual_audit/field_flow_phase_step.png"
const BATTLE_OUTPUT := "res://tmp/visual_audit/field_flow_battle_handoff.png"


func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	OptionsMenu.settings["screen_shake"] = false
	GameManager.story_flags = {}
	GameManager.current_chapter = 3
	GameManager.current_locale = "ko"
	GameManager.set_flag("ch3_arrived")
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var map: Node = (load("res://scenes/maps/belt_waystation.tscn") as PackedScene).instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame
	var player := map.get_node("Player") as CharacterBody2D
	var population := map.get_node("WorldPopulation") as Node2D
	var threat: FieldThreat = null
	for actor in population.get_children():
		if actor is FieldThreat:
			threat = actor as FieldThreat
			break
	assert(player != null and threat != null, "Capture requires one live player and visible FieldThreat")

	# Stage the audit in the map's visual center so the threat telegraph is not
	# obscured by the persistent compass/minimap overlays.
	threat.global_position = Vector2(400, 270)
	player.global_position = Vector2(400, 412)
	player.collision_mask = 0
	var flow := player.get_node("FieldFlow") as FieldFlow
	flow.flow = 86.0
	player.call("set_field_threat_source", "visible:capture", 0.34)
	await get_tree().create_timer(0.48).timeout
	await RenderingServer.frame_post_draw
	_save_viewport(APPROACH_OUTPUT)

	Input.action_press("move_up")
	Input.action_press("field_dash")
	await get_tree().physics_frame
	Input.action_release("field_dash")
	await get_tree().create_timer(0.11).timeout
	await RenderingServer.frame_post_draw
	_save_viewport(PHASE_OUTPUT)
	Input.action_release("move_up")

	var status: Dictionary = player.call("get_field_flow_status")
	BattleManager.prepare_field_entry("ambush", 86)
	var enemy := BattleManager.Enemy.new("Belt Tag Raider", 94, 13, false)
	enemy.weakness = "fire"
	BattleManager.start_battle(
		enemy,
		"res://scenes/maps/belt_waystation.tscn",
		"",
		"res://assets/sprites/world_population/hostiles/belt_tag_raider_field_v1.png"
	)
	var battle: Node = (load("res://scenes/battle/battle_scene.tscn") as PackedScene).instantiate()
	add_child(battle)
	await get_tree().create_timer(BattleManager.paced(1.82)).timeout
	battle.call("_choose_tactical_objective", 0)
	await get_tree().create_timer(BattleManager.paced(0.34)).timeout
	await RenderingServer.frame_post_draw
	_save_viewport(BATTLE_OUTPUT)

	print("FIELD_FLOW_CAPTURE_PASS approach=%s phase=%s battle=%s mode=%s pressure=%.2f" % [
		APPROACH_OUTPUT,
		PHASE_OUTPUT,
		BATTLE_OUTPUT,
		String(status.mode),
		float(status.pressure),
	])
	get_tree().quit(0)


func _save_viewport(path: String) -> void:
	var output_dir := ProjectSettings.globalize_path("res://tmp/visual_audit")
	var mkdir_error := DirAccess.make_dir_recursive_absolute(output_dir)
	assert(mkdir_error == OK or mkdir_error == ERR_ALREADY_EXISTS, "Could not create Field Flow capture directory")
	var image := get_viewport().get_texture().get_image()
	assert(image != null and not image.is_empty(), "Field Flow capture is empty: " + path)
	var save_error := image.save_png(ProjectSettings.globalize_path(path))
	assert(save_error == OK, "Could not save Field Flow capture: " + path)
