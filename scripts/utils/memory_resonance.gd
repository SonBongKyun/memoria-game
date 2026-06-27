## MemoryResonance - exploration events that trade an owned memory for a field bonus.
class_name MemoryResonance
extends RefCounted

const TILE_SIZE: int = 32
const MEMORY_RESONANCE_CG_PATH: String = "res://assets/cg/generated/memory_compass_resonance_cinematic.png"

const RESONANCE_POINTS: Dictionary = {
	"rim_forest": [
		{"pos_x": 5, "pos_y": 4, "memory_id": "sense_forest_smell", "bonus_type": "max_hp", "bonus_value": 10, "bonus_desc": "The earth remembers your footsteps. +10 Max HP.", "flag": "resonance_rim_smell"},
		{"pos_x": 18, "pos_y": 8, "memory_id": "daily_campfire_song", "bonus_type": "grains", "bonus_value": 25, "bonus_desc": "The melody dissolves into currency. +25 Grains.", "flag": "resonance_rim_song"},
	],
	"verdan_market": [
		{"pos_x": 12, "pos_y": 6, "memory_id": "daily_market_food", "bonus_type": "grains", "bonus_value": 30, "bonus_desc": "The vendor's ghost tips his hat. +30 Grains.", "flag": "resonance_verdan_food"},
		{"pos_x": 8, "pos_y": 14, "memory_id": "identity_first_sword", "bonus_type": "item", "bonus_value": "firebomb", "bonus_desc": "The courtyard echoes. Found a Firebomb.", "flag": "resonance_verdan_sword"},
	],
	"belt_waystation": [
		{"pos_x": 15, "pos_y": 5, "memory_id": "sense_dead_soil", "bonus_type": "encounter_reduce", "bonus_value": 50, "bonus_desc": "The dead earth accepts you. Encounters reduced.", "flag": "resonance_belt_soil"},
		{"pos_x": 6, "pos_y": 12, "memory_id": "rel_tobias_records", "bonus_type": "grains", "bonus_value": 20, "bonus_desc": "Ink stains fade into coins. +20 Grains.", "flag": "resonance_belt_tobias"},
	],
	"drift_shelter": [
		{"pos_x": 10, "pos_y": 8, "memory_id": "daily_elia_hands", "bonus_type": "max_hp", "bonus_value": 15, "bonus_desc": "Warmth remembered. Your body strengthens. +15 Max HP.", "flag": "resonance_drift_hands"},
	],
	"crumbling_coast": [
		{"pos_x": 14, "pos_y": 4, "memory_id": "sense_salt_wind", "bonus_type": "item", "bonus_value": "hi_potion", "bonus_desc": "Salt crystallizes into medicine. Found a Hi-Potion.", "flag": "resonance_coast_salt"},
		{"pos_x": 7, "pos_y": 12, "memory_id": "daily_elia_walking", "bonus_type": "grains", "bonus_value": 20, "bonus_desc": "Footsteps dissolve into currency. +20 Grains.", "flag": "resonance_coast_walk"},
	],
	"the_seam": [
		{"pos_x": 6, "pos_y": 3, "memory_id": "daily_garden_flowers", "bonus_type": "max_hp", "bonus_value": 10, "bonus_desc": "Petals become strength. +10 Max HP.", "flag": "resonance_seam_flowers"},
		{"pos_x": 16, "pos_y": 10, "memory_id": "rel_sable_trust", "bonus_type": "item", "bonus_value": "smoke_bomb", "bonus_desc": "Trust dissolves into shadows. Found a Smoke Bomb.", "flag": "resonance_seam_trust"},
	],
	"seam_outskirts": [
		{"pos_x": 12, "pos_y": 8, "memory_id": "rel_echo_shell", "bonus_type": "grains", "bonus_value": 35, "bonus_desc": "Echoes crystallize. +35 Grains.", "flag": "resonance_outskirts_shell"},
	],
	"forgotten_forest": [
		{"pos_x": 8, "pos_y": 6, "memory_id": "sense_hollow_trees", "bonus_type": "encounter_reduce", "bonus_value": 50, "bonus_desc": "The forest recognizes you. Encounters reduced.", "flag": "resonance_forest_trees"},
		{"pos_x": 16, "pos_y": 14, "memory_id": "rel_ghost_words", "bonus_type": "max_hp", "bonus_value": 12, "bonus_desc": "A ghost's sentence finishes inside you. +12 Max HP.", "flag": "resonance_forest_ghost"},
	],
	"colorless_waste": [
		{"pos_x": 10, "pos_y": 6, "memory_id": "sense_no_color", "bonus_type": "grains", "bonus_value": 40, "bonus_desc": "Absence turns to currency. +40 Grains.", "flag": "resonance_waste_color"},
		{"pos_x": 6, "pos_y": 12, "memory_id": "identity_compass", "bonus_type": "max_hp", "bonus_value": 20, "bonus_desc": "Direction becomes constitution. +20 Max HP.", "flag": "resonance_waste_compass"},
	],
	"bl07_void": [
		{"pos_x": 12, "pos_y": 8, "memory_id": "identity_void_walker", "bonus_type": "max_hp", "bonus_value": 25, "bonus_desc": "What you saw strengthens you. +25 Max HP.", "flag": "resonance_bl07_void"},
	],
}

static func setup_points(map_node: Node2D, map_name: String) -> void:
	var points: Array = RESONANCE_POINTS.get(map_name, [])
	for point_data in points:
		var point: Dictionary = point_data as Dictionary
		if GameManager.get_flag(point["flag"]):
			continue

		var memory: MemoryManager.Memory = MemoryManager._get_memory(str(point["memory_id"]))
		if memory == null or memory.is_burned:
			continue

		var pos: Vector2 = Vector2(float(point["pos_x"]) * TILE_SIZE, float(point["pos_y"]) * TILE_SIZE)
		_create_resonance_trigger(map_node, pos, point)

static func _create_resonance_trigger(map_node: Node2D, pos: Vector2, point: Dictionary) -> void:
	var area = Area2D.new()
	area.name = "MemoryResonance_%s" % str(point.get("flag", "echo"))
	area.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2
	area.add_to_group("memory_resonance")
	area.set_meta("memory_id", point["memory_id"])
	area.set_meta("bonus_desc", point["bonus_desc"])
	area.set_meta("flag", point["flag"])

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 1.5, TILE_SIZE * 1.5)
	shape.shape = rect
	area.add_child(shape)

	var glow = ColorRect.new()
	glow.size = Vector2(TILE_SIZE * 1.7, TILE_SIZE * 1.7)
	glow.position = -glow.size * 0.5
	glow.pivot_offset = glow.size * 0.5
	glow.color = Color(0.7, 0.55, 0.3, 0.16)
	glow.z_index = -1
	area.add_child(glow)

	var core = ColorRect.new()
	core.size = Vector2(8, 8)
	core.position = -core.size * 0.5
	core.pivot_offset = core.size * 0.5
	core.color = Color(1.0, 0.8, 0.35, 0.72)
	core.z_index = 2
	area.add_child(core)

	for i in range(4):
		var spark = ColorRect.new()
		spark.size = Vector2(3, 10)
		var angle = float(i) * TAU / 4.0
		spark.position = Vector2(cos(angle), sin(angle)) * 20.0 - spark.size * 0.5
		spark.rotation = angle
		spark.color = Color(0.95, 0.72, 0.28, 0.42)
		spark.z_index = 1
		area.add_child(spark)

	var tween = map_node.create_tween().set_loops().set_parallel(true)
	tween.tween_property(glow, "color:a", 0.32, 1.4).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(glow, "scale", Vector2(1.12, 1.12), 1.4).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(core, "scale", Vector2(1.35, 1.35), 0.8).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.chain().tween_property(glow, "color:a", 0.1, 1.4).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.parallel().tween_property(glow, "scale", Vector2.ONE, 1.4).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.parallel().tween_property(core, "scale", Vector2.ONE, 0.8).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)

	var flag: String = str(point["flag"])
	var memory_id: String = str(point["memory_id"])
	var bonus_desc: String = str(point["bonus_desc"])
	var bonus_type: String = str(point["bonus_type"])
	var bonus_value: Variant = point["bonus_value"]

	area.body_entered.connect(func(body):
		if body.name != "Player" or GameManager.current_state != GameManager.GameState.EXPLORATION:
			return
		if GameManager.get_flag(flag):
			return

		var mem: MemoryManager.Memory = MemoryManager._get_memory(memory_id)
		if mem == null or mem.is_burned:
			return

		GameManager.set_flag(flag)
		tween.kill()
		area.queue_free()
		_trigger_resonance_choice(mem, bonus_type, bonus_value, bonus_desc)
	)
	map_node.add_child(area)

static func pulse_scan(map_node: Node, origin: Vector2, radius: float) -> Dictionary:
	if map_node == null or map_node.get_tree() == null:
		return {"count": 0}

	var nearest: Area2D = null
	var nearest_distance: float = INF
	var count: int = 0
	for node in map_node.get_tree().get_nodes_in_group("memory_resonance"):
		if not is_instance_valid(node) or not (node is Area2D):
			continue
		if node.get_parent() != map_node:
			continue
		var area: Area2D = node as Area2D
		var dist: float = origin.distance_to(area.global_position)
		if dist <= radius:
			count += 1
			_flash_scan_target(area, radius, dist)
			if dist < nearest_distance:
				nearest_distance = dist
				nearest = area

	if nearest == null:
		return {"count": 0}

	var memory_id: String = str(nearest.get_meta("memory_id", ""))
	var memory: MemoryManager.Memory = MemoryManager._get_memory(memory_id)
	var memory_title: String = memory.title if memory != null else "unknown memory"
	return {
		"count": count,
		"distance": nearest_distance,
		"memory_id": memory_id,
		"memory_title": memory_title,
	}

static func _flash_scan_target(area: Area2D, radius: float, distance: float) -> void:
	var strength: float = clampf(1.0 - (distance / maxf(radius, 1.0)), 0.25, 1.0)
	for child in area.get_children():
		if child is CanvasItem:
			var item: CanvasItem = child as CanvasItem
			var base_modulate: Color = item.modulate
			var tw = area.create_tween()
			tw.tween_property(item, "modulate", Color(1.45, 1.25, 0.55, clampf(base_modulate.a + 0.35 * strength, 0.2, 1.0)), 0.12)
			tw.tween_property(item, "modulate", base_modulate, 0.55).set_trans(Tween.TRANS_SINE)

	var hint = Label.new()
	hint.text = "ECHO"
	hint.add_theme_font_size_override("font_size", 9)
	hint.add_theme_color_override("font_color", Color(1.0, 0.86, 0.46, 0.92))
	hint.add_theme_color_override("font_outline_color", Color(0.0, 0.0, 0.0, 0.72))
	hint.add_theme_constant_override("outline_size", 2)
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint.position = Vector2(-28, -34)
	hint.z_index = 8
	hint.modulate.a = 0.0
	area.add_child(hint)
	var ht = area.create_tween()
	ht.set_parallel(true)
	ht.tween_property(hint, "modulate:a", 1.0, 0.12)
	ht.tween_property(hint, "position:y", -44.0, 0.55).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	ht.chain().tween_property(hint, "modulate:a", 0.0, 0.28)
	ht.chain().tween_callback(hint.queue_free)

static func _trigger_resonance_choice(memory: MemoryManager.Memory, bonus_type: String, bonus_value: Variant, bonus_desc: String) -> void:
	NotificationToast.show_toast("Memory Resonance: %s" % memory.title, NotificationToast.ToastType.INFO)

	var burned: MemoryManager.Memory = MemoryManager.burn_memory_silent(memory.id)
	if burned == null:
		return

	if ResourceLoader.exists(MEMORY_RESONANCE_CG_PATH) and is_instance_valid(CgViewer):
		var caption := "기억 나침반이 떨린다. 잃어버린 방향이 잠깐 형태를 되찾았다." if GameManager.current_locale == "ko" else "The Memory Compass trembles. A lost direction briefly takes shape."
		CgViewer.show_cg(MEMORY_RESONANCE_CG_PATH, caption, 2.4)

	match bonus_type:
		"max_hp":
			GameManager.player_data.max_hp += bonus_value
			GameManager.player_data.hp = mini(GameManager.player_data.hp + bonus_value, GameManager.player_data.max_hp)
		"grains":
			GameManager.player_data.grains += bonus_value
		"item":
			GameManager.add_item(str(bonus_value), 1)
		"encounter_reduce":
			GameManager.set_flag("resonance_encounter_reduce")

	NotificationToast.show_toast(bonus_desc, NotificationToast.ToastType.SUCCESS)
	print("[MemoryResonance] Burned '%s' for: %s" % [memory.title, bonus_desc])
