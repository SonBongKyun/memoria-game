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
	var generated_paths: Array[String] = [
		HybridDepthStage.ROOT_SPIRE_PATH,
		HybridDepthStage.RELAY_OBELISK_PATH,
		HybridDepthStage.MEMORY_LANTERN_PATH,
		HybridDepthStage.WRECKED_MAST_PATH,
		HybridDepthStage.VOID_MONOLITH_PATH,
	]
	for path in generated_paths:
		assert(ResourceLoader.exists(path), "GPT Image 2 depth landmark is missing: %s" % path)
		var import_text := FileAccess.get_file_as_string(path + ".import")
		assert("mipmaps/generate=true" in import_text and "process/size_limit=1024" in import_text,
			"3D billboard art must use bounded mipmapped imports: %s" % path)
	assert(battle_stage.illustrated_landmarks.size() == 2,
		"Rim Forest battle must gain two painterly Sprite3D root landmarks")
	var illustrated_probe := battle_stage.illustrated_landmarks[0]
	assert(illustrated_probe.texture != null and not illustrated_probe.shaded,
		"Painterly Sprite3D art must preserve its authored light instead of blackening under stage light")
	assert(illustrated_probe.texture_filter == BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		"Depth landmark downscaling must remain mipmapped")
	var landmark_before_burn := illustrated_probe.modulate
	battle_stage.play_memory_burn(3)
	battle_stage._process(0.016)
	assert(illustrated_probe.modulate != landmark_before_burn,
		"Generated depth art must share the memory-burn reaction with the live arena")

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
	assert(atlas_stage.illustrated_landmarks.size() == 5,
		"World atlas must use five illustrated route landmarks without replacing its live 3D route")
	assert(_count_meshes(atlas_stage.scene_root) >= 30, "Atlas relief must contain route, landmark and contour geometry")
	atlas_stage.focus_route(7)
	assert(atlas_stage._focus_pan > 0.0, "Selecting a later route must pan the live 3D camera")

	var relic_stage := HybridDepthStage.create_stage("bl07_void", HybridDepthStage.StageMode.RELIC)
	relic_stage.name = "RelicHybridProbe"
	add_child(relic_stage)
	await get_tree().process_frame
	assert(relic_stage.focus_root != null and relic_stage.orbit_root != null, "Relic relief must have a floating core and orbit")
	assert(relic_stage.illustrated_landmarks.size() == 1,
		"Relic relief must place one profile-matched generated centerpiece in the live orbit")
	assert(_count_meshes(relic_stage.scene_root) >= 35, "Relic relief must contain a layered memory construct")
	OptionsMenu.settings["reduce_motion"] = previous_reduce
	print("HYBRID_DEPTH_SMOKE_PASS battle=%d atlas=%d relic=%d route_markers=%d illustrated=%d/%d/%d burn_tint=1" % [
		_count_meshes(battle_stage.scene_root),
		_count_meshes(atlas_stage.scene_root),
		_count_meshes(relic_stage.scene_root),
		atlas_stage._atlas_markers.size(),
		battle_stage.illustrated_landmarks.size(),
		atlas_stage.illustrated_landmarks.size(),
		relic_stage.illustrated_landmarks.size(),
	])
	get_tree().quit(0)

func _count_meshes(node: Node) -> int:
	var count := 1 if node is MeshInstance3D else 0
	for child in node.get_children():
		count += _count_meshes(child)
	return count
