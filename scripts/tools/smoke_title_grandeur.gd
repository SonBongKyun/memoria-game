extends Node

func _ready() -> void:
	var previous_locale := GameManager.current_locale
	var previous_reduce := bool(OptionsMenu.settings.get("reduce_motion", false))
	GameManager.current_locale = "ko"
	OptionsMenu.settings["reduce_motion"] = true

	var title_scene := load("res://scenes/main/main.tscn") as PackedScene
	assert(title_scene != null, "Title scene must load")
	var title := title_scene.instantiate() as Control
	add_child(title)
	await get_tree().process_frame
	await get_tree().process_frame

	var background := title.get_node_or_null("CinematicBackground") as TextureRect
	var rift_glow := title.get_node_or_null("MemoryRiftGlow") as TextureRect
	var vignette := title.get_node_or_null("CinematicVignette") as ColorRect
	var ash := title.get_node_or_null("TitleAsh") as GPUParticles2D
	var wordmark := title.get_node_or_null("TitleStack/TitleWordmark") as Label
	var eyebrow := title.get_node_or_null("TitleStack/TitleEyebrow") as Label
	var title_rail := title.get_node_or_null("TitleGoldRail") as TextureRect
	var menu_panel := title.get_node_or_null("MenuBackdrop") as PanelContainer
	var menu_heading := title.get_node_or_null("MenuHeading") as Label
	var menu_footer := title.get_node_or_null("MenuFooter") as Label

	assert(background != null and background.texture != null, "Title must retain its authored painterly background")
	assert(background.texture.resource_path == title.TITLE_BG_PATH, "Title background must use the live premium key art")
	assert(rift_glow != null and rift_glow.material is CanvasItemMaterial, "The memory rupture needs an additive depth layer")
	assert(vignette != null and vignette.material is ShaderMaterial, "The title needs a shaped cinematic vignette")
	assert(ash != null and not ash.emitting, "Reduce Motion must stop decorative title ash")
	assert(wordmark != null and wordmark.text == "MEMORIA", "The MEMORIA wordmark must remain prominent")
	assert(wordmark.get_theme_font_size("font_size") >= 72, "The title wordmark must carry the screen hierarchy")
	assert(eyebrow != null and eyebrow.get_theme_font_size("font_size") >= UITheme.MIN_META_FONT_SIZE, "The title eyebrow must remain readable")
	assert(title_rail != null and title_rail.texture is GradientTexture2D, "The title needs an authored gold rail")
	assert(menu_panel != null and menu_panel.anchor_right - menu_panel.anchor_left >= 0.27, "The command panel must frame the menu")
	assert(menu_heading != null and menu_heading.text != "", "The command panel needs a destination heading")
	assert(menu_footer != null and "ENTER" in menu_footer.text, "Keyboard guidance must remain visible")
	var resting_background_position := background.position
	await get_tree().process_frame
	assert(background.position.is_equal_approx(resting_background_position), "Reduce Motion must stop ambient title parallax")

	var menu := title.get_node("VBoxContainer") as VBoxContainer
	var buttons: Array[Button] = []
	for child in menu.get_children():
		if child is Button:
			buttons.append(child)
	assert(buttons.size() == 5, "The title must preserve all five existing destinations")
	for i in range(buttons.size()):
		assert(buttons[i].text.begins_with("%02d" % (i + 1)), "Menu numbering must follow visual order")
		assert(buttons[i].custom_minimum_size.x >= 270.0, "Title commands must remain easy to scan")
		assert(buttons[i].get_theme_font_size("font_size") >= UITheme.MIN_UI_FONT_SIZE, "Title commands must remain readable")

	var title_size := wordmark.get_theme_font_size("font_size")
	title.queue_free()
	await get_tree().process_frame
	GameManager.current_locale = previous_locale
	OptionsMenu.settings["reduce_motion"] = previous_reduce
	print("TITLE_GRANDEUR_SMOKE_PASS layers=7 commands=%d reduce_motion=1 title_size=%d" % [buttons.size(), title_size])
	get_tree().quit(0)
