extends Node

func _ready() -> void:
	GameManager.player_data["field_focus"] = 0
	GameManager.story_flags.erase("pulse_found_smoke_echo")
	assert(MemoryResonance.FIELD_FOCUS_CG_BY_MAP.size() == MemoryResonance.RESONANCE_POINTS.size(), "Every resonance map should register a Field Focus CG")
	for map_key in MemoryResonance.RESONANCE_POINTS:
		assert(MemoryResonance.FIELD_FOCUS_CG_BY_MAP.has(map_key), "Field Focus CG missing for %s" % map_key)
	for entry in MemoryResonance.FIELD_FOCUS_CG_BY_MAP.values():
		assert(ResourceLoader.exists(String(entry.path)), "Every Field Focus CG path must resolve")

	var map := Node2D.new()
	map.name = "FieldFocusSmokeMap"
	add_child(map)
	var echo := Area2D.new()
	echo.position = Vector2(64, 0)
	echo.add_to_group("memory_resonance")
	echo.set_meta("flag", "smoke_echo")
	echo.set_meta("memory_id", "sense_forest_smell")
	map.add_child(echo)

	var scan := MemoryResonance.pulse_scan(map, Vector2.ZERO, 150.0)
	assert(int(scan.get("count", 0)) == 1, "Memory Pulse should find the nearby echo")
	assert(int(scan.get("new_discoveries", 0)) == 1, "First scan should map one new echo")
	assert(GameManager.add_field_focus(int(scan.new_discoveries)) == 1, "Mapped echo should grant focus")

	var enemy := BattleManager.Enemy.new("Field Focus Dummy", 20, 1, false)
	BattleManager.start_battle(enemy, "res://scenes/maps/rim_forest.tscn")
	assert(GameManager.get_field_focus() == 0, "Normal battle should consume one focus")
	assert(BattleManager.field_focus_opening, "Battle should mark the focus opening")
	assert(is_equal_approx(BattleManager.momentum, 25.0), "Focus should start at 25 resonance")
	assert(is_equal_approx(BattleManager.limit_gauge, 20.0), "Focus should start at 20 limit")

	print("FIELD_FOCUS_SMOKE_PASS maps=%d count=1 resonance=25 limit=20" % MemoryResonance.FIELD_FOCUS_CG_BY_MAP.size())
	get_tree().quit(0)
