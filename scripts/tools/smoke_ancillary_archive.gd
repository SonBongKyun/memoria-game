extends Node

func _ready() -> void:
	assert(ResourceLoader.exists(PauseMenu.SAVE_ARCHIVE_BACKDROP_PATH), "Save Archive backdrop is missing")
	assert(ResourceLoader.exists(PauseMenu.FIELD_GUIDE_BACKDROP_PATH), "Field Guide backdrop is missing")
	assert(ResourceLoader.exists(PauseMenu.EMPTY_SAVE_RECORD_PATH), "Empty save-record thumbnail is missing")
	assert(ResourceLoader.exists(Minimap.MINIMAP_FRAME_PATH), "Minimap compass frame is missing")
	assert(SaveManager.MAX_SLOTS == 3, "Save Archive expects three manual records")

	var previous_locale := GameManager.current_locale
	GameManager.current_locale = "ko"
	PauseMenu.call("_show_save_archive_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	var save_overlay := PauseMenu.get_node_or_null("SaveArchiveOverlay")
	assert(save_overlay != null, "Save Archive overlay did not build")
	assert(_count_nodes_of_type(save_overlay, "Button") >= 7, "Save Archive must expose four records, save/load, and close controls")
	var archive_buttons := save_overlay.find_children("*", "Button", true, false)
	if archive_buttons.size() >= 2:
		(archive_buttons[0] as Button).pressed.emit()
		await get_tree().process_frame
		(archive_buttons[1] as Button).pressed.emit()
		await get_tree().process_frame
	_save_viewport("user://memoria_save_archive_smoke.png")
	save_overlay.queue_free()
	await get_tree().process_frame

	PauseMenu.call("_show_field_guide_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	var guide_overlay := PauseMenu.get_node_or_null("FieldGuideOverlay")
	assert(guide_overlay != null, "Field Guide overlay did not build")
	assert(_count_nodes_of_type(guide_overlay, "RichTextLabel") == 6, "Field Guide must expose six concise reference blocks")
	_save_viewport("user://memoria_field_guide_smoke.png")
	assert(bool(PauseMenu.call("_close_active_archive_modal")), "Pause Esc routing must close the active archive surface first")
	await get_tree().process_frame
	assert(PauseMenu.get_node_or_null("FieldGuideOverlay") == null, "Field Guide must be removed before the pause menu closes")

	GameManager.current_state = GameManager.GameState.EXPLORATION
	var map_data: Array = []
	for y in range(18):
		var row: Array = []
		for x in range(20):
			row.append(0 if x < 10 else 1)
		map_data.append(row)
	var tile_defs: Array = [{"detail": "grass"}, {"detail": "path"}]
	var minimap_data := Minimap.create_minimap(self, map_data, tile_defs, 20, 18)
	await get_tree().process_frame
	var minimap_container := minimap_data.get("container") as Control
	assert(minimap_container != null and minimap_container.size.x <= 112.0, "Minimap frame must remain compact")
	var has_generated_frame := false
	for child in minimap_container.get_children():
		if child is TextureRect and child.texture != null and child.texture.resource_path == Minimap.MINIMAP_FRAME_PATH:
			has_generated_frame = true
	assert(has_generated_frame, "Minimap must render the generated Memory Compass frame")
	_save_viewport("user://memoria_minimap_frame_smoke.png")

	GameManager.current_locale = previous_locale
	print("ANCILLARY_ARCHIVE_SMOKE_PASS")
	print("SAVE_ARCHIVE_SCREENSHOT=" + ProjectSettings.globalize_path("user://memoria_save_archive_smoke.png"))
	print("FIELD_GUIDE_SCREENSHOT=" + ProjectSettings.globalize_path("user://memoria_field_guide_smoke.png"))
	print("MINIMAP_SCREENSHOT=" + ProjectSettings.globalize_path("user://memoria_minimap_frame_smoke.png"))
	get_tree().quit()

func _count_nodes_of_type(root: Node, type_name: String) -> int:
	var count := 0
	for child in root.get_children():
		if child.is_class(type_name):
			count += 1
		count += _count_nodes_of_type(child, type_name)
	return count

func _save_viewport(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	var image := get_viewport().get_texture().get_image()
	assert(image != null and not image.is_empty(), "Viewport capture failed: " + path)
	var error := image.save_png(path)
	assert(error == OK, "Could not save viewport capture: " + path)
