extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_world_map.png"

func _ready() -> void:
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_chapter = 7
	GameManager.change_state(GameManager.GameState.MENU)
	PauseMenu.call("_show_travel_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	var overlay := PauseMenu.get_node_or_null("WorldMapOverlay")
	assert(overlay != null, "World map overlay must open")
	var relief := overlay.get_node_or_null("RouteDepthDiorama") as HybridDepthStage
	assert(relief != null and relief._atlas_markers.size() == 10, "World map must contain the live ten-route relief")
	relief.focus_route(7)
	await get_tree().create_timer(0.8).timeout
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("HYBRID_WORLD_MAP_CAPTURE_PASS path=%s" % OUTPUT_PATH)
	get_tree().quit(0)
