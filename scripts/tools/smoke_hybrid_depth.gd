extends Node

func _ready() -> void:
	var previous_reduce: Variant = OptionsMenu.settings.get("reduce_motion", true)
	OptionsMenu.settings["reduce_motion"] = true
	var battle_stage := HybridDepthStage.create_stage("rim_forest", HybridDepthStage.StageMode.BATTLE)
	battle_stage.name = "BattleHybridProbe"
	add_child(battle_stage)
	await get_tree().process_frame
	assert(battle_stage.viewport != null and battle_stage.viewport.own_world_3d, "Hybrid stage must render through an isolated 3D world")
	assert(battle_stage.camera != null and battle_stage.camera.current, "Hybrid stage must own a live perspective camera")
	assert(_count_meshes(battle_stage.scene_root) >= 16, "Battle hybrid must contain a readable low-poly diorama")
	assert(battle_stage.battle_player_focus_root != null and battle_stage.battle_enemy_focus_root != null, "Battle diorama must anchor both sides with reactive focus rings")

	# S211: the battle arena must read as a space, not a few floating sticks.
	assert(battle_stage.arena_floor != null and battle_stage.arena_floor.mesh is PlaneMesh,
		"Battle arena needs a real ground plane, not only dashed traces")
	assert(battle_stage.arena_floor.get_parent() == battle_stage.scene_root,
		"The floor must sit outside the swaying motion root, or the ground rocks under the characters")
	var battle_environment: Environment = null
	for child in battle_stage.scene_root.get_children():
		if child is WorldEnvironment:
			battle_environment = (child as WorldEnvironment).environment
	assert(battle_environment != null and battle_environment.fog_enabled,
		"Depth fog is what turns identical low-poly boxes into distance")
	var floor_depth: float = absf(battle_stage.arena_floor.position.z) + HybridDepthStage.ARENA_FLOOR_SIZE * 0.5
	assert(floor_depth > 30.0, "The arena floor must recede far enough to build a horizon")
	battle_stage.set_battle_focus("player")
	assert(battle_stage._focus_pan < 0.0 and battle_stage._battle_player_focus_material.emission_energy_multiplier > 1.0, "Player turns must focus the left arena without moving 2D actors")
	battle_stage.set_battle_focus("enemy")
	assert(battle_stage._focus_pan > 0.0 and battle_stage._battle_enemy_focus_material.emission_energy_multiplier > 1.0, "Enemy turns must focus the right arena without moving 2D actors")

	var atlas_stage := HybridDepthStage.create_stage("world_map", HybridDepthStage.StageMode.ATLAS)
	atlas_stage.name = "AtlasHybridProbe"
	add_child(atlas_stage)
	await get_tree().process_frame
	assert(atlas_stage._atlas_markers.size() == 10, "Atlas relief must represent all ten Part I routes")
	assert(_count_meshes(atlas_stage.scene_root) >= 30, "Atlas relief must contain route, landmark and contour geometry")
	atlas_stage.focus_route(7)
	assert(atlas_stage._focus_pan > 0.0, "Selecting a later route must pan the live 3D camera")

	var relic_stage := HybridDepthStage.create_stage("bl07_void", HybridDepthStage.StageMode.RELIC)
	relic_stage.name = "RelicHybridProbe"
	add_child(relic_stage)
	await get_tree().process_frame
	assert(relic_stage.focus_root != null and relic_stage.orbit_root != null, "Relic relief must have a floating core and orbit")
	assert(_count_meshes(relic_stage.scene_root) >= 35, "Relic relief must contain a layered memory construct")
	OptionsMenu.settings["reduce_motion"] = previous_reduce
	print("HYBRID_DEPTH_SMOKE_PASS battle=%d atlas=%d relic=%d route_markers=%d" % [
		_count_meshes(battle_stage.scene_root),
		_count_meshes(atlas_stage.scene_root),
		_count_meshes(relic_stage.scene_root),
		atlas_stage._atlas_markers.size(),
	])
	get_tree().quit(0)

func _count_meshes(node: Node) -> int:
	var count := 1 if node is MeshInstance3D else 0
	for child in node.get_children():
		count += _count_meshes(child)
	return count
