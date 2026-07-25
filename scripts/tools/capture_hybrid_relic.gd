extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_relic_choice.png"

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	var data: Dictionary = WorldPopulation.CURIOS_BY_MAP["bl07_void"]
	var curio := WorldCurio.new()
	curio.map_id = "bl07_void"
	curio.curio_id = String(data.id)
	curio.title = String(data.title)
	curio.title_ko = String(data.title_ko)
	curio.lore = String(data.lore)
	curio.lore_ko = String(data.lore_ko)
	curio.art_path = String(data.art)
	curio.salvage_item = String(data.item)
	add_child(curio)
	await get_tree().process_frame
	curio.call("_show_choice")
	await get_tree().process_frame
	await get_tree().process_frame
	var choice_layer := get_tree().root.get_node_or_null("WorldCurioChoice")
	assert(choice_layer != null, "Curio choice must open")
	var relief := choice_layer.get_node_or_null("RelicDepthDiorama") as HybridDepthStage
	assert(relief != null and relief.focus_root != null, "Curio choice must layer a live 3D relic over the 2D discovery art")
	await get_tree().create_timer(0.8).timeout
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("HYBRID_RELIC_CAPTURE_PASS path=%s" % OUTPUT_PATH)
	get_tree().paused = false
	get_tree().quit(0)
