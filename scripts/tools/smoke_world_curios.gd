extends Node

func _ready() -> void:
	assert(WorldPopulation.CURIOS_BY_MAP.size() == 10, "Every core region needs one deliberate RPG landmark")
	var seen_ids: Dictionary = {}
	for map_id: String in WorldPopulation.CURIOS_BY_MAP:
		var data: Dictionary = WorldPopulation.CURIOS_BY_MAP[map_id]
		var curio_id := String(data.get("id", ""))
		assert(curio_id != "" and not seen_ids.has(curio_id), "%s needs a unique curio id" % map_id)
		seen_ids[curio_id] = true
		assert(ResourceLoader.exists(String(data.get("art", ""))), "%s curio needs a live illustrated discovery plate" % map_id)
		assert(GameManager.ITEMS.has(String(data.get("item", ""))), "%s curio salvage must grant a registered item" % map_id)

	var previous_flags := GameManager.story_flags.duplicate(true)
	var previous_focus := GameManager.get_field_focus()
	var previous_grains := int(GameManager.player_data.grains)
	GameManager.story_flags = {}
	GameManager.player_data.field_focus = 0

	var data: Dictionary = WorldPopulation.CURIOS_BY_MAP["rim_forest"]
	var curio := WorldCurio.new()
	curio.map_id = "rim_forest"
	curio.curio_id = String(data.id)
	curio.title = String(data.title)
	curio.title_ko = String(data.title_ko)
	curio.lore = String(data.lore)
	curio.lore_ko = String(data.lore_ko)
	curio.art_path = String(data.art)
	curio.salvage_item = String(data.item)
	add_child(curio)
	await get_tree().process_frame
	curio.interact()
	await get_tree().process_frame
	var choice_layer := get_tree().root.get_node_or_null("WorldCurioChoice") as CanvasLayer
	assert(choice_layer != null and get_tree().paused, "Interacting with a curio should open and safely pause an illustrated choice")
	var buttons := choice_layer.find_children("*", "Button", true, false)
	assert(buttons.size() == 4, "Curio choice should expose study, salvage, attune, and leave")
	curio.call("_resolve_choice", "study")
	await get_tree().process_frame
	assert(not get_tree().paused, "Resolving a curio should restore exploration processing")
	assert(GameManager.get_flag("world_curio_rim_forest_cinder_nest_study"), "Curio choice must be persisted")
	assert(GameManager.get_field_focus() == 1, "Studying a curio should prepare the next battle")

	GameManager.story_flags = previous_flags
	GameManager.player_data.field_focus = previous_focus
	GameManager.player_data.grains = previous_grains
	print("WORLD_CURIOS_SMOKE_PASS maps=10 choices=4 study_focus=1")
	get_tree().quit(0)
