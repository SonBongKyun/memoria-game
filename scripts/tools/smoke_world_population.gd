extends Node

const MAPS := [
	{"id": "rim_forest", "width": 25, "height": 18, "voices": 5, "hunts": 3, "caches": 1},
	{"id": "verdan_market", "width": 30, "height": 20, "voices": 6, "hunts": 0, "caches": 1},
	{"id": "belt_waystation", "width": 25, "height": 18, "voices": 5, "hunts": 4},
	{"id": "drift_shelter", "width": 25, "height": 18, "voices": 5, "hunts": 3, "caches": 1},
	{"id": "crumbling_coast", "width": 25, "height": 18, "voices": 5, "hunts": 4},
	{"id": "the_seam", "width": 25, "height": 18, "voices": 5, "hunts": 3},
	{"id": "seam_outskirts", "width": 25, "height": 18, "voices": 5, "hunts": 4},
	{"id": "forgotten_forest", "width": 25, "height": 18, "voices": 5, "hunts": 4},
	{"id": "colorless_waste", "width": 25, "height": 18, "voices": 5, "hunts": 4, "caches": 1},
	{"id": "bl07_void", "width": 20, "height": 20, "voices": 5, "hunts": 4},
	{"id": "rim_root_hollow", "width": 25, "height": 18, "voices": 2, "hunts": 2, "optional_site": true},
	{"id": "verdan_ledger_cellar", "width": 25, "height": 18, "voices": 3, "hunts": 1, "optional_site": true},
	{"id": "belt_signal_yard", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/belt_signal_yard.tscn"},
	{"id": "drift_waymarker_shrine", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/drift_waymarker_shrine.tscn"},
	{"id": "coast_cinder_harbor", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/coast_cinder_harbor.tscn"},
	{"id": "seam_lantern_ward", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/seam_lantern_ward.tscn"},
	{"id": "forest_name_hollow", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/forest_name_hollow.tscn"},
	{"id": "waste_grey_caravan", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/waste_grey_caravan.tscn"},
	{"id": "bl07_seed_vault", "width": 25, "height": 18, "voices": 4, "hunts": 2, "caches": 1, "optional_site": true, "scene": "res://scenes/maps/bl07_seed_vault.tscn"},
]

const BLOCKED_TILE_IDS := {
	"rim_forest": [2, 4],
	"verdan_market": [1, 2],
	"belt_waystation": [2, 3],
	"drift_shelter": [1, 2, 3],
	"crumbling_coast": [2, 3],
	"the_seam": [1, 2, 5],
	"seam_outskirts": [1, 2, 3, 5],
	"forgotten_forest": [1, 2, 5],
	"colorless_waste": [1, 2, 3],
	"bl07_void": [3],
}

func _ready() -> void:
	var previous_flags := GameManager.story_flags.duplicate(true)
	var previous_chapter := GameManager.current_chapter
	var previous_state := GameManager.current_state
	var previous_locale := GameManager.current_locale
	GameManager.current_chapter = 10
	GameManager.current_locale = "ko"
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	GameManager.story_flags = {}
	for chapter in range(1, 11):
		GameManager.set_flag("ch%d_arrived" % chapter)
		GameManager.set_flag("ch%d_complete" % chapter)
	GameManager.set_flag("ch1_opening_done")
	GameManager.set_flag("ch1_elia_appeared")
	GameManager.set_flag("ch1_void_beast_defeated")
	GameManager.set_flag("ch2_malet_done")
	GameManager.set_flag("ch3_tobias_met")
	GameManager.set_flag("ch10_void_entered")

	var total_voices := 0
	var total_hunts := 0
	var total_caches := 0
	var total_curios := 0
	var field_asset_paths: Dictionary = {}
	var special_voice_count := 0
	var special_hunt_count := 0
	var atlas_gate_count := 0
	for map_data in MAPS:
		var is_optional_site := bool(map_data.get("optional_site", false))
		var layout: Node2D = null
		var layout_grid: Array = []
		if is_optional_site:
			var optional_source := FileAccess.get_file_as_string("res://scenes/maps/optional_memory_site.gd")
			assert(optional_source.contains("WorldPopulation.populate(self, site_id)"), "%s must invoke the population system during map setup" % map_data.id)
			if map_data.has("scene"):
				assert(ResourceLoader.exists(String(map_data.scene)), "%s optional scene must exist" % map_data.id)
		else:
			var source := FileAccess.get_file_as_string("res://scenes/maps/%s.gd" % map_data.id)
			assert(source.contains("WorldPopulation.populate(self, \"%s\")" % map_data.id), "%s must invoke the population system during map setup" % map_data.id)
			var map_script := load("res://scenes/maps/%s.gd" % map_data.id) as Script
			layout = map_script.new() as Node2D
			layout_grid = layout.get("map_data")
		var map := Node2D.new()
		map.name = "PopulationFixture_%s" % map_data.id
		add_child(map)
		WorldPopulation.populate(map, String(map_data.id))
		await get_tree().process_frame
		var population := map.get_node_or_null("WorldPopulation") as Node2D
		assert(population != null, "%s must attach the world population root" % map_data.id)
		var voices := 0
		var hunts := 0
		var caches := 0
		var curios := 0
		for actor in population.get_children():
			assert(actor is Node2D and (actor as Node2D).position.x > 0.0 and (actor as Node2D).position.y > 0.0 and (actor as Node2D).position.x < float(map_data.width) * 32.0 and (actor as Node2D).position.y < float(map_data.height) * 32.0, "%s must be placed inside its map bounds" % actor.name)
			if not is_optional_site:
				var tile_x := int(floor((actor as Node2D).position.x / 32.0))
				var tile_y := int(floor((actor as Node2D).position.y / 32.0))
				assert(not (BLOCKED_TILE_IDS.get(map_data.id, []) as Array).has(int(layout_grid[tile_y][tile_x])), "%s must not occupy a collision tile" % actor.name)
			if actor is WorldCache:
				caches += 1
				assert(GameManager.ITEMS.has(actor.item_id), "%s cache item must be registered" % actor.name)
				var cache_sprite := actor.get_node_or_null("ItemIcon") as Sprite2D
				assert(cache_sprite != null and cache_sprite.texture != null, "%s cache needs an item icon" % actor.name)
				assert(cache_sprite.texture.resource_path.contains("assets/ui/items"), "%s cache must use an authored item icon" % actor.name)
			elif actor is WorldCurio:
				curios += 1
				assert(WorldPopulation.CURIOS_BY_MAP.has(map_data.id), "%s must belong to a registered core-region curio" % actor.name)
				assert(ResourceLoader.exists(String(actor.art_path)), "%s needs an illustrated discovery plate" % actor.name)
				assert(GameManager.ITEMS.has(String(actor.salvage_item)), "%s salvage choice must grant a registered item" % actor.name)
			elif actor is StaticBody2D:
				voices += 1
				var field_sprite := actor.get_node_or_null("CharacterSprite") as AnimatedSprite2D
				assert(field_sprite != null and field_sprite.sprite_frames != null, "%s world voice needs a field sprite" % actor.name)
				var texture := field_sprite.sprite_frames.get_frame_texture("idle_down", 0)
				assert(texture != null and texture.resource_path.contains("world_population/npcs"), "%s must resolve a generated NPC field asset" % actor.name)
				var source_image := texture.get_image()
				var used_rect := source_image.get_used_rect()
				var is_child := "child" in String(actor.get("npc_name")).to_lower()
				var expected_height := PixelSprite.FIELD_CHILD_HEIGHT if is_child else PixelSprite.FIELD_ADULT_HEIGHT
				var visible_height := float(used_rect.size.y) * field_sprite.scale.y
				var visible_foot := (float(used_rect.position.y + used_rect.size.y) - float(source_image.get_height()) * 0.5 + field_sprite.offset.y) * field_sprite.scale.y + field_sprite.position.y
				assert(absf(visible_height - expected_height) < 0.5, "%s must use the unified apparent-height profile" % actor.name)
				assert(absf(visible_foot - PixelSprite.FIELD_FOOT_Y) < 0.5, "%s must share the unified foot baseline" % actor.name)
				assert(field_sprite.texture_filter == CanvasItem.TEXTURE_FILTER_LINEAR, "%s must use the unified low-noise texture filter" % actor.name)
				field_asset_paths[texture.resource_path] = true
				var voice_id := String(actor.name).trim_prefix("WorldVoice_")
				if WorldPopulation.SPECIAL_VOICE_ART.has(voice_id):
					assert(texture.resource_path == WorldPopulation.SPECIAL_VOICE_ART[voice_id], "%s must use its distinct specialist field asset" % actor.name)
					special_voice_count += 1
				assert(field_sprite.sprite_frames.has_animation("idle_left") and field_sprite.sprite_frames.has_animation("idle_right"), "%s must preserve the NPC facing contract" % actor.name)
				assert(String(actor.call("_get_runtime_name")) != "", "%s must expose a localized talk name" % actor.name)
			elif actor is Area2D:
				hunts += 1
				assert(actor is FieldThreat, "%s must use the readable FieldThreat approach controller" % actor.name)
				var threat_sprite: Sprite2D = null
				for component in actor.get_children():
					if component is Sprite2D:
						threat_sprite = component as Sprite2D
						break
				assert(threat_sprite != null and threat_sprite.texture != null, "%s visible threat needs a sprite" % actor.name)
				assert(actor.get_node_or_null("ThreatAura") is Line2D and actor.get_node_or_null("ThreatSight") is Line2D, "%s needs pressure aura and sight-line telegraphs" % actor.name)
				assert(String((actor as FieldThreat).hunt_data.get("_resolved_field_art", "")) == threat_sprite.texture.resource_path, "%s must carry the same authored hostile art into battle" % actor.name)
				assert(threat_sprite.texture.resource_path.contains("world_population/hostiles"), "%s must resolve a generated hostile field asset" % actor.name)
				field_asset_paths[threat_sprite.texture.resource_path] = true
				var hunt_id := String(actor.name).trim_prefix("WorldThreat_")
				if WorldPopulation.SPECIAL_HUNT_ART.has(hunt_id):
					assert(threat_sprite.texture.resource_path == WorldPopulation.SPECIAL_HUNT_ART[hunt_id], "%s must use its distinct rare hostile asset" % actor.name)
					special_hunt_count += 1
		assert(voices == int(map_data.voices), "%s expected %d world voices, got %d" % [map_data.id, map_data.voices, voices])
		assert(hunts == int(map_data.hunts), "%s expected %d visible threats, got %d" % [map_data.id, map_data.hunts, hunts])
		var minimap_points := Minimap._collect_points_of_interest(map)
		var minimap_threats := 0
		for minimap_point: Dictionary in minimap_points:
			if String(minimap_point.get("kind", "")) == "threat":
				minimap_threats += 1
		assert(minimap_threats == hunts, "%s minimap must expose every live field threat" % map_data.id)
		assert(caches == int(map_data.get("caches", 0)), "%s expected %d caches, got %d" % [map_data.id, int(map_data.get("caches", 0)), caches])
		var expected_curios := 1 if WorldPopulation.CURIOS_BY_MAP.has(map_data.id) else 0
		assert(curios == expected_curios, "%s expected %d regional curios, got %d" % [map_data.id, expected_curios, curios])
		total_voices += voices
		total_hunts += hunts
		total_caches += caches
		total_curios += curios
		map.free()
		if layout != null:
			layout.free()
		await get_tree().process_frame

	for origin in WorldAtlas.GATES:
		var origin_id := String(origin)
		var origin_source := FileAccess.get_file_as_string("res://scenes/maps/%s.gd" % origin_id)
		assert(origin_source.contains("WorldAtlas.add_gateways(self, \"%s\")" % origin_id), "%s must expose its atlas gateway after completion" % origin_id)
		var gate_map := Node2D.new()
		add_child(gate_map)
		WorldAtlas.add_gateways(gate_map, origin_id)
		var gate_root := gate_map.get_node_or_null("WorldAtlasGateways") as Node2D
		assert(gate_root != null and gate_root.get_child_count() == 1, "%s must spawn one unlocked atlas route" % origin_id)
		var gate := gate_root.get_child(0) as MapGateway
		assert(gate != null and ResourceLoader.exists(gate.destination_scene), "%s atlas route needs a valid destination" % origin_id)
		atlas_gate_count += 1
		gate_map.free()

	assert(total_voices == 84 and total_hunts == 50 and total_caches == 11, "World expansion totals must remain deliberate")
	assert(total_curios == 10, "Every core region should contain one choice-driven illustrated landmark")
	print("[WorldPopulationSmoke] unique live field assets: %d" % field_asset_paths.size())
	assert(field_asset_paths.size() >= 54, "The expanded civilian and hostile field roster must appear in the playable population")
	for expanded_asset in [
		"res://assets/sprites/world_population/npcs/lantern_cartographer_field_v1.png",
		"res://assets/sprites/world_population/npcs/root_tender_field_v1.png",
		"res://assets/sprites/world_population/npcs/verdan_route_map_seller_field_v1.png",
		"res://assets/sprites/world_population/npcs/belt_rail_root_tender_field_v1.png",
		"res://assets/sprites/world_population/npcs/outskirts_edge_cartographer_field_v1.png",
		"res://assets/sprites/world_population/npcs/bl07_void_root_tender_field_v1.png",
		"res://assets/sprites/world_population/hostiles/cinder_antler_field_v2.png",
		"res://assets/sprites/world_population/hostiles/ledger_moth_swarm_field_v2.png",
		"res://assets/sprites/world_population/npcs/verdan_sealed_note_seller_field_v2.png",
		"res://assets/sprites/world_population/npcs/drift_rain_ledger_scribe_field_v2.png",
		"res://assets/sprites/world_population/npcs/coast_ash_netter_field_v2.png",
		"res://assets/sprites/world_population/npcs/seam_quiet_healer_field_v2.png",
		"res://assets/sprites/world_population/npcs/outskirts_threshold_warden_field_v2.png",
		"res://assets/sprites/world_population/npcs/forest_root_listener_field_v2.png",
		"res://assets/sprites/world_population/hostiles/belt_scavenger_field_v2.png",
		"res://assets/sprites/world_population/hostiles/signal_wisp_field_v2.png",
		"res://assets/sprites/world_population/hostiles/ash_hound_field_v2.png",
		"res://assets/sprites/world_population/hostiles/rootbound_echo_field_v2.png",
		"res://assets/sprites/world_population/hostiles/colorless_wraith_field_v2.png",
		"res://assets/sprites/world_population/hostiles/void_fragment_field_v2.png",
	]:
		assert(field_asset_paths.has(expanded_asset), "Expanded generated field asset must be live: %s" % expanded_asset)
	for override_art in WorldPopulation.FIELD_ART_OVERRIDES.values():
		assert(ResourceLoader.exists(String(override_art)), "Every population art override must resolve: %s" % override_art)
	assert(WorldPopulation.FIELD_ART_OVERRIDES.get("verdan_map_seller", "") == "res://assets/sprites/world_population/npcs/verdan_route_map_seller_field_v1.png", "Verdan route seller must use its dedicated generated illustration")
	assert(WorldPopulation.FIELD_ART_OVERRIDES.get("vault_cinder_antler", "") == "res://assets/sprites/world_population/hostiles/cinder_antler_field_v2.png", "BL-07 cinder antler must use its dedicated generated illustration")
	assert(WorldPopulation.FIELD_ART_OVERRIDES.get("sealed_note_seller", "") == "res://assets/sprites/world_population/npcs/verdan_sealed_note_seller_field_v2.png", "Sealed-note seller must use its dedicated generated illustration")
	assert(WorldPopulation.FIELD_ART_OVERRIDES.get("void_fragment", "") == "res://assets/sprites/world_population/hostiles/void_fragment_field_v2.png", "Void fragment must use its dedicated generated illustration")
	assert(special_voice_count == 6 and special_hunt_count == 6, "Specialist and rare variants must remain wired")
	assert(atlas_gate_count == 7, "Every earned atlas destination needs a story-safe return route")
	GameManager.story_flags = previous_flags
	GameManager.current_chapter = previous_chapter
	GameManager.current_locale = previous_locale
	GameManager.change_state(previous_state)
	print("WORLD_POPULATION_SMOKE_PASS maps=19 voices=%d visible_threats=%d caches=%d curios=%d atlas_gates=%d generated_field_assets=%d" % [total_voices, total_hunts, total_caches, total_curios, atlas_gate_count, field_asset_paths.size()])
	get_tree().quit(0)
