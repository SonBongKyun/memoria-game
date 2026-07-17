## MemoryResonance - exploration events that trade an owned memory for a field bonus.
class_name MemoryResonance
extends RefCounted

const TILE_SIZE: int = 32
const MEMORY_RESONANCE_CG_PATH: String = "res://assets/cg/generated/memory_compass_resonance_cinematic.png"
const RESONANCE_CHOICE_ART: Dictionary = {
	"bind": "res://assets/cg/generated/resonance_choice_bind_v2.png",
	"kindle": "res://assets/cg/generated/resonance_choice_kindle_v2.png",
	"leave": "res://assets/cg/generated/resonance_choice_leave_v2.png",
}
const FIELD_FOCUS_CG_BY_MAP: Dictionary = {
	"rim_forest": {
		"path": "res://assets/cg/generated/resonance_rim_forest_echo.png",
		"caption": "The forest remembers a set of footsteps the traveler no longer owns.",
		"deep_path": "res://assets/cg/generated/illustration_expansion_v2/resonance_rim_forest_deep_v2.png",
		"deep_caption": "Two trails converge. The forest remembers companionship as direction.",
		"deep_caption_ko": "두 발자국이 합쳐진다. 숲은 동행을 방향으로 기억한다.",
		"caption_ko": "숲은 여행자가 더는 간직하지 못한 발자국을 기억하고 있다.",
	},
	"verdan_market": {
		"path": "res://assets/cg/generated/resonance_verdan_market_echo.png",
		"caption": "Warm steam rises from a meal that vanished before anyone finished it.",
		"deep_path": "res://assets/cg/generated/illustration_expansion_v2/resonance_verdan_market_deep_v2.png",
		"deep_caption": "A remembered kindness points toward the courtyard where the sword began.",
		"deep_caption_ko": "기억된 친절이 검이 시작된 안뜰을 가리킨다.",
		"caption_ko": "아무도 다 먹지 못한 채 사라진 식사에서 따뜻한 김이 피어오른다.",
	},
	"belt_waystation": {
		"path": "res://assets/cg/generated/resonance_belt_waystation_echo.png",
		"caption": "The dead earth keeps one footprint while spilled ink points back to the record.",
		"deep_path": "res://assets/cg/generated/illustration_expansion_v2/resonance_belt_waystation_deep_v2.png",
		"deep_caption": "Dead soil, spilled ink, and an unwritten tree reconnect the record.",
		"deep_caption_ko": "죽은 흙과 흘린 잉크, 쓰이지 않은 나무가 기록을 다시 잇는다.",
		"caption_ko": "죽은 땅은 발자국 하나를 붙들고, 흘러나온 잉크는 기록으로 돌아가는 길을 가리킨다.",
	},
	"drift_shelter": {
		"path": "res://assets/cg/generated/resonance_drift_shelter_echo.png",
		"caption": "Warmth remains on the table after the hands themselves have moved on.",
		"caption_ko": "손은 떠났지만 그 온기는 아직 탁자 위에 남아 있다.",
	},
	"crumbling_coast": {
		"path": "res://assets/cg/generated/resonance_crumbling_coast_echo.png",
		"caption": "Salt keeps the shape of a hand after the person has gone.",
		"caption_ko": "사람이 떠난 뒤에도 소금은 손의 형태를 붙들고 있다.",
	},
	"the_seam": {
		"path": "res://assets/cg/generated/resonance_the_seam_echo.png",
		"caption": "Trust survives as a route traced by touch through white flowers.",
		"deep_path": "res://assets/cg/generated/illustration_expansion_v2/resonance_the_seam_deep_v2.png",
		"deep_caption": "The eighteenth lantern turns trust into a route through the flowers.",
		"deep_caption_ko": "열여덟 번째 등불이 꽃 사이의 신뢰를 길로 바꾼다.",
		"caption_ko": "신뢰는 흰 꽃 사이로 손끝이 더듬어 그린 길이 되어 살아남는다.",
	},
	"seam_outskirts": {
		"path": "res://assets/cg/generated/resonance_seam_outskirts_echo.png",
		"caption": "The Echo Shell bridges a broken road with one remembered note.",
		"caption_ko": "에코 셸이 기억한 음 하나로 끊어진 길을 잇는다.",
	},
	"forgotten_forest": {
		"path": "res://assets/cg/generated/resonance_forgotten_forest_echo.png",
		"caption": "A hollow tree tries to finish the sentence buried inside it.",
		"deep_path": "res://assets/cg/generated/illustration_expansion_v2/resonance_forgotten_forest_deep_v2.png",
		"deep_caption": "A second hand rises from the roots to finish the witness.",
		"deep_caption_ko": "뿌리에서 두 번째 손이 올라와 증언을 끝맺는다.",
		"caption_ko": "속이 빈 나무가 제 안에 묻힌 문장을 끝맺으려 한다.",
	},
	"colorless_waste": {
		"path": "res://assets/cg/generated/resonance_colorless_waste_echo.png",
		"caption": "The compass gives color back to only the stones it can name.",
		"caption_ko": "나침반은 이름 붙일 수 있는 돌에만 잠시 색을 돌려준다.",
	},
	"bl07_void": {
		"path": "res://assets/cg/generated/resonance_bl07_void_echo.png",
		"caption": "The Void bends around one human footprint it failed to erase.",
		"caption_ko": "보이드는 끝내 지우지 못한 인간의 발자국 하나를 피해 휘어진다.",
	},
}

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
		if GameManager.get_flag(flag) or bool(area.get_meta("choice_open", false)):
			return

		var mem: MemoryManager.Memory = MemoryManager._get_memory(memory_id)
		if mem == null or mem.is_burned:
			return

		# Entering an echo must never spend a memory before the player has had a
		# chance to weigh its cost. The trigger stays in the world on "Leave", so
		# returning later is a real decision rather than a missed collectible.
		area.set_meta("choice_open", true)
		area.set_deferred("monitoring", false)
		_show_resonance_choice(area, tween, flag, mem, bonus_type, bonus_value, bonus_desc)
	)
	map_node.add_child(area)

static func pulse_scan(map_node: Node, origin: Vector2, radius: float) -> Dictionary:
	if map_node == null or map_node.get_tree() == null:
		return {"count": 0}

	var nearest: Area2D = null
	var nearest_distance: float = INF
	var count: int = 0
	var new_discoveries: int = 0
	var discovery_capacity := maxi(GameManager.FIELD_FOCUS_MAX - GameManager.get_field_focus(), 0)
	for node in map_node.get_tree().get_nodes_in_group("memory_resonance"):
		if not is_instance_valid(node) or not (node is Area2D):
			continue
		if node.get_parent() != map_node:
			continue
		var area: Area2D = node as Area2D
		var dist: float = origin.distance_to(area.global_position)
		if dist <= radius:
			count += 1
			var point_flag := String(area.get_meta("flag", "echo"))
			var discovery_flag := "pulse_found_%s" % point_flag
			var is_new := not GameManager.get_flag(discovery_flag) and new_discoveries < discovery_capacity
			if is_new:
				GameManager.set_flag(discovery_flag)
				new_discoveries += 1
			_flash_scan_target(area, radius, dist, is_new)
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
		"direction": nearest.global_position - origin,
		"new_discoveries": new_discoveries,
	}

static func show_field_focus_discovery() -> void:
	var tree := Engine.get_main_loop() as SceneTree
	if tree == null or tree.current_scene == null:
		return
	var map_key := tree.current_scene.scene_file_path.get_file().get_basename()
	if not FIELD_FOCUS_CG_BY_MAP.has(map_key):
		return
	var entry: Dictionary = FIELD_FOCUS_CG_BY_MAP[map_key]
	var base_seen_flag := "field_focus_cg_seen_%s" % map_key
	var deep_seen_flag := "field_focus_cg_deep_seen_%s" % map_key
	var discovered_count := 0
	for point: Dictionary in RESONANCE_POINTS.get(map_key, []):
		if GameManager.get_flag("pulse_found_%s" % String(point.get("flag", ""))):
			discovered_count += 1
	var show_deep := GameManager.get_flag(base_seen_flag) and discovered_count >= 2 and entry.has("deep_path")
	var seen_flag := deep_seen_flag if show_deep else base_seen_flag
	if GameManager.get_flag(seen_flag):
		return
	var path_key := "deep_path" if show_deep else "path"
	var path := String(entry.get(path_key, ""))
	if path == "" or not ResourceLoader.exists(path) or not is_instance_valid(CgViewer):
		return
	GameManager.set_flag(seen_flag)
	var caption_key := ("deep_caption_ko" if GameManager.current_locale == "ko" else "deep_caption") if show_deep else ("caption_ko" if GameManager.current_locale == "ko" else "caption")
	CgViewer.show_cg(path, String(entry.get(caption_key, entry.get("caption", ""))), 2.2)

static func _flash_scan_target(area: Area2D, radius: float, distance: float, is_new: bool) -> void:
	var strength: float = clampf(1.0 - (distance / maxf(radius, 1.0)), 0.25, 1.0)
	for child in area.get_children():
		if child is CanvasItem:
			var item: CanvasItem = child as CanvasItem
			var base_modulate: Color = item.modulate
			var tw = area.create_tween()
			tw.tween_property(item, "modulate", Color(1.45, 1.25, 0.55, clampf(base_modulate.a + 0.35 * strength, 0.2, 1.0)), 0.12)
			tw.tween_property(item, "modulate", base_modulate, 0.55).set_trans(Tween.TRANS_SINE)

	var hint = Label.new()
	hint.text = "NEW ECHO" if is_new else "ECHO"
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

static func _show_resonance_choice(area: Area2D, pulse_tween: Tween, flag: String, memory: MemoryManager.Memory, bonus_type: String, bonus_value: Variant, bonus_desc: String) -> void:
	var tree := Engine.get_main_loop() as SceneTree
	if tree == null or tree.root == null:
		_restore_resonance_trigger(area)
		return

	GameManager.change_state(GameManager.GameState.DIALOGUE)
	var is_ko := GameManager.current_locale == "ko"
	var layer := CanvasLayer.new()
	layer.name = "MemoryResonanceChoice"
	layer.layer = 80
	layer.process_mode = Node.PROCESS_MODE_ALWAYS

	var root := Control.new()
	root.name = "ChoiceRoot"
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_STOP
	layer.add_child(root)

	var veil := ColorRect.new()
	veil.set_anchors_preset(Control.PRESET_FULL_RECT)
	veil.color = Color(0.008, 0.007, 0.014, 0.90)
	veil.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(veil)

	var panel := PanelContainer.new()
	panel.anchor_left = 0.07
	panel.anchor_right = 0.93
	panel.anchor_top = 0.12
	panel.anchor_bottom = 0.88
	panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.025, 0.023, 0.040, 0.98), Color(0.72, 0.55, 0.30, 0.90), 1, 10, 16
	))
	root.add_child(panel)

	var margins := MarginContainer.new()
	margins.add_theme_constant_override("margin_left", 20)
	margins.add_theme_constant_override("margin_right", 20)
	margins.add_theme_constant_override("margin_top", 18)
	margins.add_theme_constant_override("margin_bottom", 18)
	panel.add_child(margins)

	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	margins.add_child(box)

	var kicker := Label.new()
	kicker.text = "기억 공명" if is_ko else "MEMORY RESONANCE"
	kicker.add_theme_font_size_override("font_size", 12)
	kicker.add_theme_color_override("font_color", Color(0.90, 0.70, 0.36))
	UITheme.apply_ui_font(kicker)
	box.add_child(kicker)

	var title := Label.new()
	title.text = ("'%s'이(가) 어둠 속에서 응답합니다." % memory.title) if is_ko else ("%s answers from the dark." % memory.title)
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(0.95, 0.90, 0.80))
	UITheme.apply_ui_font(title)
	box.add_child(title)

	var prompt := Label.new()
	prompt.text = "한 번의 선택이 이 잔향의 앞날을 정합니다. 힘은 즉시 얻되, 상실은 반드시 스스로 선택해야 합니다." if is_ko else "One decision gives this echo a future. Power is immediate; the loss must be chosen."
	prompt.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	prompt.add_theme_font_size_override("font_size", 13)
	prompt.add_theme_color_override("font_color", Color(0.68, 0.66, 0.72))
	UITheme.apply_ui_font(prompt)
	box.add_child(prompt)

	var rule := HSeparator.new()
	rule.add_theme_color_override("separator", Color(0.65, 0.50, 0.28, 0.55))
	box.add_child(rule)

	var choices := HBoxContainer.new()
	choices.size_flags_vertical = Control.SIZE_EXPAND_FILL
	choices.add_theme_constant_override("separation", 12)
	box.add_child(choices)

	var bind_title := "붙들기" if is_ko else "BIND"
	var bind_body := "기억을 지킵니다. 다음 전투에 사용할 집중 1을 얻습니다." if is_ko else "Keep the memory. Gain 1 Field Focus for the next battle."
	var kindle_title := "태우기" if is_ko else "KINDLE"
	var kindle_body := ("지금 태웁니다. 보상: %s" % bonus_desc) if is_ko else ("Burn it now. Reward: %s" % bonus_desc)
	var leave_title := "보류하기" if is_ko else "LEAVE"
	var leave_body := "지금은 떠납니다. 이 잔향은 돌아올 때까지 남아 있습니다." if is_ko else "Walk away. The echo remains here until you return."

	var bind_button := _add_choice_card(
		choices,
		bind_title,
		bind_body,
		String(RESONANCE_CHOICE_ART["bind"]),
		Color(0.42, 0.66, 0.96),
		"기억을 지킨다" if is_ko else "Keep the memory",
		func() -> void:
			_resolve_resonance_choice("bind", layer, area, pulse_tween, flag, memory, bonus_type, bonus_value, bonus_desc)
	)
	_add_choice_card(
		choices,
		kindle_title,
		kindle_body,
		String(RESONANCE_CHOICE_ART["kindle"]),
		Color(0.94, 0.50, 0.22),
		"보상을 위해 태운다" if is_ko else "Take the reward",
		func() -> void:
			_resolve_resonance_choice("kindle", layer, area, pulse_tween, flag, memory, bonus_type, bonus_value, bonus_desc)
	)
	_add_choice_card(
		choices,
		leave_title,
		leave_body,
		String(RESONANCE_CHOICE_ART["leave"]),
		Color(0.54, 0.60, 0.70),
		"아직은 아니다" if is_ko else "Not yet",
		func() -> void:
			_resolve_resonance_choice("leave", layer, area, pulse_tween, flag, memory, bonus_type, bonus_value, bonus_desc)
	)

	var footer := Label.new()
	footer.text = "방향키로 카드를 고르고 Enter로 선택할 수 있습니다." if is_ko else "Use the focused card with Enter, or click a choice."
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.add_theme_font_size_override("font_size", 11)
	footer.add_theme_color_override("font_color", Color(0.50, 0.48, 0.54))
	UITheme.apply_ui_font(footer)
	box.add_child(footer)

	tree.root.call_deferred("add_child", layer)
	if bind_button != null:
		bind_button.call_deferred("grab_focus")

static func _add_choice_card(parent: HBoxContainer, title_text: String, body_text: String, art_path: String, accent: Color, action_text: String, on_pressed: Callable) -> Button:
	var card := PanelContainer.new()
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	card.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(accent.r * 0.07, accent.g * 0.07, accent.b * 0.09, 0.94), Color(accent.r, accent.g, accent.b, 0.72), 1, 7, 8
	))
	parent.add_child(card)

	var padding := MarginContainer.new()
	padding.add_theme_constant_override("margin_left", 8)
	padding.add_theme_constant_override("margin_right", 8)
	padding.add_theme_constant_override("margin_top", 8)
	padding.add_theme_constant_override("margin_bottom", 8)
	card.add_child(padding)

	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 6)
	padding.add_child(box)

	var art := TextureRect.new()
	art.custom_minimum_size = Vector2(0, 116)
	art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if art_path != "" and ResourceLoader.exists(art_path):
		art.texture = load(art_path)
	box.add_child(art)

	var title := Label.new()
	title.text = title_text
	title.add_theme_font_size_override("font_size", 14)
	title.add_theme_color_override("font_color", accent.lightened(0.22))
	UITheme.apply_ui_font(title)
	box.add_child(title)

	var body := Label.new()
	body.text = body_text
	body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.custom_minimum_size = Vector2(0, 54)
	body.add_theme_font_size_override("font_size", 11)
	body.add_theme_color_override("font_color", Color(0.76, 0.73, 0.72))
	UITheme.apply_ui_font(body)
	box.add_child(body)

	var button := Button.new()
	button.text = action_text
	button.custom_minimum_size = Vector2(0, 34)
	button.add_theme_font_size_override("font_size", 12)
	button.add_theme_color_override("font_color", Color(0.94, 0.92, 0.84))
	button.add_theme_color_override("font_hover_color", Color.WHITE)
	button.add_theme_stylebox_override("normal", UITheme.make_button_style(Color(accent.r * 0.24, accent.g * 0.24, accent.b * 0.24, 0.78), Color(accent.r, accent.g, accent.b, 0.68)))
	button.add_theme_stylebox_override("hover", UITheme.make_button_style(Color(accent.r * 0.44, accent.g * 0.44, accent.b * 0.44, 0.92), accent.lightened(0.20)))
	button.add_theme_stylebox_override("focus", UITheme.make_button_style(Color(accent.r * 0.44, accent.g * 0.44, accent.b * 0.44, 0.92), Color(1.0, 0.9, 0.58, 0.95)))
	UITheme.apply_ui_font(button)
	button.pressed.connect(on_pressed)
	button.mouse_entered.connect(func() -> void: AudioManager.play_sfx("ui_hover"))
	box.add_child(button)
	return button

static func _resolve_resonance_choice(choice: String, layer: CanvasLayer, area: Area2D, pulse_tween: Tween, flag: String, memory: MemoryManager.Memory, bonus_type: String, bonus_value: Variant, bonus_desc: String) -> void:
	if is_instance_valid(layer):
		layer.queue_free()

	if choice == "leave":
		_restore_resonance_trigger(area)
		GameManager.change_state(GameManager.GameState.EXPLORATION)
		NotificationToast.show_toast("Echo left intact. Return whenever you are ready.", NotificationToast.ToastType.INFO)
		return

	GameManager.set_flag(flag)
	if pulse_tween != null and pulse_tween.is_valid():
		pulse_tween.kill()
	if is_instance_valid(area):
		area.queue_free()

	if choice == "bind":
		var gained := GameManager.add_field_focus(1)
		GameManager.set_flag("%s_bound" % flag)
		var focus_desc := "Field Focus is already full. The memory is still safe." if gained == 0 else "Memory preserved. +1 Field Focus for your next battle."
		NotificationToast.show_toast(focus_desc, NotificationToast.ToastType.SUCCESS)
		_show_resonance_outcome(String(RESONANCE_CHOICE_ART["bind"]), "The lantern keeps what the road tried to take.")
		print("[MemoryResonance] Bound '%s' for Field Focus" % memory.title)
		return

	var burned: MemoryManager.Memory = MemoryManager.burn_memory_silent(memory.id)
	if burned == null:
		GameManager.change_state(GameManager.GameState.EXPLORATION)
		return

	_apply_resonance_bonus(bonus_type, bonus_value)
	NotificationToast.show_toast(bonus_desc, NotificationToast.ToastType.SUCCESS)
	_show_resonance_outcome(String(RESONANCE_CHOICE_ART["kindle"]), "The Memory Compass trembles. A lost direction briefly takes shape.")
	print("[MemoryResonance] Kindled '%s' for: %s" % [memory.title, bonus_desc])

static func _restore_resonance_trigger(area: Area2D) -> void:
	if not is_instance_valid(area):
		return
	area.set_meta("choice_open", false)
	area.monitoring = true

static func _apply_resonance_bonus(bonus_type: String, bonus_value: Variant) -> void:
	match bonus_type:
		"max_hp":
			var amount := int(bonus_value)
			GameManager.player_data.max_hp += amount
			GameManager.player_data.hp = mini(GameManager.player_data.hp + amount, GameManager.player_data.max_hp)
		"grains":
			GameManager.player_data.grains += int(bonus_value)
		"item":
			GameManager.add_item(str(bonus_value), 1)
		"encounter_reduce":
			GameManager.set_flag("resonance_encounter_reduce")

static func _show_resonance_outcome(art_path: String, caption: String) -> void:
	if art_path != "" and ResourceLoader.exists(art_path) and is_instance_valid(CgViewer):
		CgViewer.show_cg(art_path, caption, 2.5, func() -> void:
			GameManager.change_state(GameManager.GameState.EXPLORATION)
		)
		return
	GameManager.change_state(GameManager.GameState.EXPLORATION)
