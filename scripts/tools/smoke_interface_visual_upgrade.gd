extends Node

const REQUIRED_INTERFACE_ART: Array[String] = [
	"res://assets/cg/generated/ui_inventory_archive_v2.png",
	"res://assets/cg/generated/ui_character_status_dossier_v1.png",
	"res://assets/cg/generated/ui_codex_archive_backdrop_v2.png",
	"res://assets/cg/generated/ui_story_journal_backdrop_v3.png",
	"res://assets/cg/generated/ui_story_log_archive_v1.png",
	"res://assets/cg/generated/ui_memory_shop_backdrop_v2.png",
	"res://assets/cg/generated/ui_pause_archive_backdrop_v2.png",
	"res://assets/cg/generated/ui_battle_item_tray_v3.png",
	"res://assets/cg/generated/ui_battle_command_deck_v4.png",
	"res://assets/cg/generated/ui_battle_field_readout_v4.png",
	"res://assets/ui/items/root_balm_v2.png",
	"res://assets/ui/items/signal_jammer_v2.png",
	"res://assets/ui/items/lantern_salve_v2.png",
	"res://assets/ui/items/name_thread_v2.png",
	"res://assets/ui/items/compass_shard_v2.png",
	"res://assets/ui/items/seed_capsule_v2.png",
	"res://assets/ui/equipment/slot_weapon_v1.png",
	"res://assets/ui/equipment/slot_armor_v1.png",
	"res://assets/ui/equipment/slot_accessory_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_recover_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_cure_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_ignite_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_withdraw_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_witness_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_anchor_guard_cutin_v1.png",
	"res://assets/cg/generated/gameplay_moments/item_fault_scan_cutin_v1.png",
]

func _ready() -> void:
	for art_path in REQUIRED_INTERFACE_ART:
		assert(ResourceLoader.exists(art_path), "Interface visual is missing: " + art_path)
		if "/gameplay_moments/" in art_path:
			var moment_texture := load(art_path) as Texture2D
			assert(moment_texture != null and moment_texture.get_width() == 1672 and moment_texture.get_height() == 941, "Gameplay moment must keep the authored 16:9 plate: " + art_path)

	assert(PauseMenu.INVENTORY_BACKDROP_PATH.ends_with("ui_inventory_archive_v2.png"))
	assert(PauseMenu.STATUS_BACKDROP_PATH.ends_with("ui_character_status_dossier_v1.png"))
	assert(PauseMenu.PAUSE_BACKDROP_PATH.ends_with("ui_pause_archive_backdrop_v2.png"))
	assert(Codex.CODEX_BACKDROP_PATH.ends_with("ui_codex_archive_backdrop_v2.png"))
	assert(StoryJournal.JOURNAL_BACKDROP_PATH.ends_with("ui_story_journal_backdrop_v3.png"))
	assert(StoryLog.BACKDROP_PATH.ends_with("ui_story_log_archive_v1.png"))
	assert(MemoryShop.SHOP_BACKDROP_PATH.ends_with("ui_memory_shop_backdrop_v2.png"))

	var gallery_items: Array[Dictionary] = PauseMenu.call("_load_artbook_items")
	for art_path in REQUIRED_INTERFACE_ART:
		assert(gallery_items.any(func(item: Dictionary) -> bool: return String(item.get("path", "")) == art_path), "Artbook is missing interface visual: " + art_path)

	var saved_items: Dictionary = GameManager.player_data.get("items", {}).duplicate(true)
	var saved_equipped: Dictionary = GameManager.equipped.duplicate(true)
	GameManager.player_data["items"] = {
		"potion": 2,
		"root_balm": 1,
		"signal_jammer": 1,
		"lantern_salve": 1,
		"name_thread": 1,
		"compass_shard": 1,
		"seed_capsule": 1,
		"anchor_lantern": 1,
	}
	GameManager.equipped = {
		"weapon": "iron_sword",
		"armor": "leather_vest",
		"accessory": "iron_ring",
	}

	PauseMenu.call("_show_inventory_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().create_timer(0.35).timeout
	var inventory_overlay := PauseMenu.get_node_or_null("InventoryOverlay")
	assert(inventory_overlay != null, "Inventory overlay did not build")
	assert(inventory_overlay.get_node_or_null("InventoryPanel") != null, "Inventory panel did not build")
	assert(inventory_overlay.find_child("InventoryStatusStrip", true, false) != null, "Inventory status telemetry is missing")
	assert(inventory_overlay.find_child("InventoryFilters", true, false) != null, "Inventory category filters are missing")
	for control_name in ["InventoryQuickKit", "InventorySearch", "InventorySort", "InventoryUseNow", "InventoryPinQuick"]:
		assert(inventory_overlay.find_child(control_name, true, false) != null, "Inventory QoL control is missing: " + control_name)
	for filter_id in ["ALL", "RECOVERY", "TACTICAL", "WITNESS"]:
		assert(inventory_overlay.find_child("InventoryFilter_" + filter_id, true, false) != null, "Inventory filter is missing: " + filter_id)
	for slot_name in ["Weapon", "Armor", "Accessory"]:
		var icon := inventory_overlay.find_child("InventorySlotIcon_" + slot_name, true, false) as TextureRect
		assert(icon != null and icon.texture != null, "Equipment slot icon is missing: " + slot_name)
	var equipped_copy_found := false
	for label_node in inventory_overlay.find_children("*", "Label", true, false):
		if "Iron Sword" in String((label_node as Label).text):
			equipped_copy_found = true
	assert(equipped_copy_found, "Equipped item copy must remain visible")
	_save_viewport("user://memoria_inventory_visual_upgrade.png")
	inventory_overlay.queue_free()
	await get_tree().process_frame

	PauseMenu.call("_show_stats_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().create_timer(0.35).timeout
	var status_overlay := PauseMenu.get_node_or_null("CharacterStatusOverlay")
	assert(status_overlay != null, "Character status overlay did not build")
	var portrait := status_overlay.find_child("CharacterStatusPortrait", true, false) as TextureRect
	assert(portrait != null and portrait.texture != null, "Character dossier portrait is missing")
	assert(status_overlay.find_child("CharacterStatusResources", true, false) != null, "Character dossier resource summary is missing")
	_save_viewport("user://memoria_character_dossier_upgrade.png")
	status_overlay.queue_free()

	GameManager.player_data["items"] = saved_items
	GameManager.equipped = saved_equipped
	print("INTERFACE_VISUAL_UPGRADE_SMOKE_PASS")
	print("INVENTORY_SCREENSHOT=" + ProjectSettings.globalize_path("user://memoria_inventory_visual_upgrade.png"))
	print("DOSSIER_SCREENSHOT=" + ProjectSettings.globalize_path("user://memoria_character_dossier_upgrade.png"))
	get_tree().quit()

func _save_viewport(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	var image := get_viewport().get_texture().get_image()
	assert(image != null and not image.is_empty(), "Viewport capture failed: " + path)
	var error := image.save_png(path)
	assert(error == OK, "Could not save viewport capture: " + path)
