extends Node

func _ready() -> void:
	var saved_player: Dictionary = GameManager.player_data.duplicate(true)
	var saved_state := GameManager.current_state
	var saved_locale := GameManager.current_locale
	GameManager.current_locale = "en"
	GameManager.current_state = GameManager.GameState.PAUSED
	GameManager.player_data = {
		"name": "Arrel", "hp": 25, "max_hp": 100, "grains": 0,
		"field_focus": 0, "directive_streak": 0, "elia_with_party": true,
		"items": {"potion": 2, "hi_potion": 1, "antidote": 1, "witness_ink": 1, "firebomb": 1},
		"item_quick_slots": ["potion", "antidote", "witness_ink"], "recent_items": [],
	}

	assert(InputMap.has_action("quick_item") and InputManager.get_icon("quick_item") == "H", "Field quick-heal input must be mapped and discoverable")
	assert(GameManager.get_item_quick_slots().size() == 3, "Quick Kit must expose three stable slots")
	assert(GameManager.get_best_field_recovery() == "hi_potion", "Smart Heal must choose the smallest recovery that fully covers missing HP")
	var first_heal := GameManager.use_best_field_recovery()
	assert(bool(first_heal.get("success", false)) and int(first_heal.get("healed", 0)) == 75, "Smart Heal must restore only missing HP")
	assert(GameManager.get_item_count("hi_potion") == 0, "Smart Heal must consume exactly one item")
	GameManager.add_item("root_balm", 1)
	assert(GameManager.get_recent_items()[0] == "root_balm", "Recently acquired supplies must be tracked")
	GameManager.toggle_item_quick_slot("firebomb")
	assert(GameManager.get_item_quick_slots()[0] == "firebomb" and GameManager.get_item_quick_slots().size() == 3, "Pinning must place the item first without exceeding three slots")

	PauseMenu.call("_show_inventory_panel")
	await get_tree().process_frame
	await get_tree().process_frame
	var overlay := PauseMenu.get_node_or_null("InventoryOverlay") as Control
	assert(overlay != null, "Inventory overlay must build")
	for required_name in ["InventoryQuickKit", "InventorySearch", "InventorySort", "InventoryUseNow", "InventoryPinQuick"]:
		assert(overlay.find_child(required_name, true, false) != null, "Inventory QoL control missing: " + required_name)
	for slot_index in range(1, 4):
		assert(overlay.find_child("InventoryQuickSlot_%d" % slot_index, true, false) != null, "Quick Kit slot missing")
	var search := overlay.find_child("InventorySearch", true, false) as LineEdit
	search.text = "witness"
	PauseMenu.call("_on_inventory_search_changed", "witness", overlay)
	await get_tree().process_frame
	var visible_matches := 0
	var visible_ids: Array[String] = []
	for node in overlay.find_children("InventoryItem_*", "Button", true, false):
		if (node as Button).visible:
			visible_matches += 1
			visible_ids.append(String((node as Button).get_meta("inventory_id", "")))
	assert(visible_matches == 1 and visible_ids[0] == "witness_ink", "Inventory search must narrow the carried list")
	search.text = ""
	overlay.set_meta("inventory_selected_id", "potion")
	GameManager.player_data["hp"] = 60
	PauseMenu.call("_on_inventory_use_now", overlay)
	assert(int(GameManager.player_data["hp"]) == 100 and GameManager.get_item_count("potion") == 1, "USE NOW must heal and refresh inventory count")
	overlay.queue_free()
	await get_tree().process_frame

	GameManager.current_state = GameManager.GameState.BATTLE
	BattleManager.current_enemy = BattleManager.Enemy.new("Archive Wraith", 90, 12, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame
	battle.call("_toggle_item_list")
	await get_tree().process_frame
	var quick_buttons: Array = battle.get("_battle_quick_item_buttons")
	assert(quick_buttons.size() == 3, "Battle tray must mirror all three Quick Kit slots")
	for slot_index in range(3):
		assert((quick_buttons[slot_index] as Button).name == "BattleQuickItem_%d" % (slot_index + 1), "Battle Quick Kit needs stable slot names")
	assert((quick_buttons[0] as Button).text.begins_with("1 · Firebomb"), "Battle Quick Kit must preserve inventory pin order")

	GameManager.import_data({"player_data": {"items": {}}})
	assert(GameManager.player_data.has("item_quick_slots") and GameManager.player_data.has("recent_items"), "Legacy saves must receive inventory QoL defaults")

	GameManager.player_data = saved_player
	GameManager.current_state = saved_state
	GameManager.current_locale = saved_locale
	print("INVENTORY_QOL_SMOKE_PASS quick=3 search=%d smart_heal=%d" % [visible_matches, int(first_heal.get("healed", 0))])
	get_tree().quit(0)