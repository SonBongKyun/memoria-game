## WorldRewriteDirector (Autoload)
## Turns memory loss into visible scene-level consequences.
extends Node

signal rewrite_manifested(memory, report: Dictionary)

const MEMORY_REWRITE_RULES := {
	"daily_market_food": {
		"flag": "world_rewrite_verdan_taste_blurred",
		"title": "Verdan Taste Blurred",
		"line": "A vendor's face slips out of every market smell.",
		"compass": "Verdan taste map erased.",
		"color": Color(0.95, 0.62, 0.30),
		"art": "res://assets/cg/generated/world_rewrite_verdan_market.png"
	},
	"daily_campfire_song": {
		"flag": "world_rewrite_elia_hum_unmoored",
		"title": "Campfire Song Unmoored",
		"line": "Elia's humming still exists, but it no longer knows where to land.",
		"compass": "Anchor melody destabilized.",
		"color": Color(0.68, 0.74, 0.92),
		"art": "res://assets/cg/generated/memory_burn_elia_song.png"
	},
	"rel_hand_reaching": {
		"flag": "world_rewrite_reaching_hand_absent",
		"title": "The Reaching Hand Removed",
		"line": "When someone reaches for him, the world hesitates before drawing the hand.",
		"compass": "Relationship contour torn.",
		"color": Color(0.80, 0.66, 0.94),
		"art": "res://assets/cg/generated/memory_burn_reaching_hand.png"
	},
	"identity_first_sword": {
		"flag": "world_rewrite_first_sword_cut",
		"title": "First Sword Excised",
		"line": "The body remembers the stance. The self no longer remembers why.",
		"compass": "Identity architecture fractured.",
		"color": Color(0.95, 0.38, 0.26),
		"art": "res://assets/cg/generated/memory_burn_first_sword.png"
	},
	"core_name_origin": {
		"flag": "world_rewrite_name_origin_void",
		"title": "Name Origin Consumed",
		"line": "The sound 'Arrel' keeps its letters and loses its owner.",
		"compass": "Name-bearing thread severed.",
		"color": Color(1.00, 0.28, 0.22),
		"art": "res://assets/cg/generated/memory_burn_arrel_name.png"
	},
	"rel_tobias_records": {
		"flag": "world_rewrite_record_ink_fades",
		"title": "Record Ink Fades",
		"line": "Tobias can still write the facts. The meaning dries before the ink does.",
		"compass": "Record-tree contour retained.",
		"color": Color(0.75, 0.62, 0.48),
		"art": "res://assets/cg/generated/world_rewrite_tobias_record_tree.png"
	},
	"daily_elia_hands": {
		"flag": "world_rewrite_elia_anchor_thinned",
		"title": "Anchor Warmth Thinned",
		"line": "Warm hands remain in the scene like heat after a body has left.",
		"compass": "Elia anchor pressure reduced.",
		"color": Color(0.72, 0.78, 0.96),
		"art": "res://assets/cg/generated/world_rewrite_elia_anchor.png"
	},
	"rel_sable_voidwalk": {
		"flag": "world_rewrite_sable_witness_dimmed",
		"title": "Witness Dimmed",
		"line": "Sable's certainty loses one scar's worth of weight.",
		"compass": "Void-walker witness weakened.",
		"color": Color(0.62, 0.54, 0.72),
		"art": "res://assets/cg/generated/world_rewrite_sable_witness.png"
	},
}

const DEFAULT_LINES := {
	MemoryManager.MemoryGrade.GRADE_5: "A small sensation leaves the weather of the room.",
	MemoryManager.MemoryGrade.GRADE_4: "A daily habit disappears, and the world quietly edits around it.",
	MemoryManager.MemoryGrade.GRADE_3: "A relationship contour collapses into afterimage.",
	MemoryManager.MemoryGrade.GRADE_2: "A piece of identity breaks alignment with the body.",
	MemoryManager.MemoryGrade.GRADE_1: "The core thread burns white. Reality pauses before continuing.",
}

var _last_scene_path := ""
var _scene_residue_cooldown := 0.0

func _ready() -> void:
	if MemoryManager and MemoryManager.has_signal("memory_burned"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	if MemoryManager and MemoryManager.has_signal("memory_faded"):
		MemoryManager.memory_faded.connect(_on_memory_faded)
	set_process(true)
	print("[WorldRewriteDirector] Ready")

func _process(delta: float) -> void:
	_scene_residue_cooldown = maxf(_scene_residue_cooldown - delta, 0.0)
	var scene := get_tree().current_scene
	if scene == null:
		return
	var path := scene.scene_file_path
	if path != _last_scene_path:
		_last_scene_path = path
		if _scene_residue_cooldown <= 0.0:
			_scene_residue_cooldown = 1.2
			call_deferred("_manifest_scene_residue")

func _on_memory_burned(memory) -> void:
	var report := _build_report(memory, false)
	_apply_story_flags(memory, report)
	call_deferred("_manifest_rewrite", memory, report)
	rewrite_manifested.emit(memory, report)

func _on_memory_faded(memory) -> void:
	var report := _build_report(memory, true)
	call_deferred("_manifest_rewrite", memory, report)
	rewrite_manifested.emit(memory, report)

func _build_report(memory, faded: bool) -> Dictionary:
	var id := String(memory.id) if memory != null else ""
	var grade := int(memory.grade) if memory != null else MemoryManager.MemoryGrade.GRADE_5
	var rule: Dictionary = MEMORY_REWRITE_RULES.get(id, {})
	var title := String(rule.get("title", "Uncatalogued Absence"))
	var line := String(rule.get("line", DEFAULT_LINES.get(grade, "A contour vanished.")))
	var compass := String(rule.get("compass", _default_compass_line(memory, faded)))
	var color: Color = rule.get("color", _grade_color(grade))
	if faded:
		title = "Fading: " + title
		line = "Before it burns, it thins: " + line
		compass = "Erosion warning: " + compass
	return {
		"id": id,
		"flag": String(rule.get("flag", "world_rewrite_" + id)),
		"title": title,
		"line": line,
		"compass": compass,
		"color": color,
		"grade": grade,
		"faded": faded,
		"memory_title": String(memory.title) if memory != null else "Unknown Memory",
		"art": String(rule.get("art", _fallback_art_for_grade(grade))),
	}

func get_loss_records() -> Array[Dictionary]:
	var records: Array[Dictionary] = []
	if not MemoryManager:
		return records
	for memory in MemoryManager.memories:
		if memory.is_burned:
			records.append(_build_loss_record(memory, false))
		elif memory.is_faded:
			records.append(_build_loss_record(memory, true))
	return records

func get_rewrite_report(memory_id: String) -> Dictionary:
	if not MemoryManager:
		return {}
	for memory in MemoryManager.memories:
		if memory.id == memory_id:
			return _build_report(memory, memory.is_faded and not memory.is_burned)
	return {}

func _build_loss_record(memory, faded: bool) -> Dictionary:
	var report := _build_report(memory, faded)
	var status := "FADING" if faded else "BURNED"
	var grade_name := _grade_name(int(report.grade))
	var body := "%s\n\nMemory: %s\nGrade: %s\n\nWorld consequence:\n%s\n\nCompass reading:\n%s\n\nStory hook: %s" % [
		status,
		String(report.memory_title),
		grade_name,
		String(report.line),
		String(report.compass),
		String(report.flag),
	]
	return {
		"title": "%s - %s" % [status, String(report.memory_title)],
		"body": body,
		"color": report.color,
		"grade": int(report.grade),
		"faded": faded,
		"art": String(report.art),
	}

func _default_compass_line(memory, faded: bool) -> String:
	if memory == null:
		return "Unmapped contour missing."
	var prefix := "Eroding" if faded else "Lost"
	return "%s: %s" % [prefix, String(memory.title)]

func _apply_story_flags(memory, report: Dictionary) -> void:
	if memory == null:
		return
	GameManager.set_flag(String(report.flag), true)
	GameManager.set_flag("world_forgot_" + String(memory.id), true)
	if int(memory.grade) >= MemoryManager.MemoryGrade.GRADE_2:
		GameManager.set_flag("identity_rewrite_active", true)

func _manifest_rewrite(memory, report: Dictionary) -> void:
	var scene := get_tree().current_scene
	if scene == null:
		return
	PerceptionFilter.apply(scene)
	_show_rewrite_art(report)
	_spawn_echo_cluster(scene, report, _find_manifest_origin(scene), memory)
	if NotificationToast:
		NotificationToast.show_toast("World rewrite: %s" % String(report.title), NotificationToast.ToastType.WARNING)

func _manifest_scene_residue() -> void:
	if not GameManager or GameManager.current_state != GameManager.GameState.EXPLORATION:
		return
	if not MemoryManager or MemoryManager.burned_memories.is_empty():
		return
	var scene := get_tree().current_scene
	if scene == null:
		return
	var memory = MemoryManager.burned_memories.back()
	var report := _build_report(memory, false)
	report["line"] = "This place has adjusted around %s." % String(memory.title)
	report["title"] = "Residual Absence"
	_spawn_echo_cluster(scene, report, _find_manifest_origin(scene) + Vector2(0, -18), memory, 1)

func _find_manifest_origin(scene: Node) -> Vector2:
	var players := get_tree().get_nodes_in_group("player")
	for player in players:
		if player is Node2D and (scene == player or scene.is_ancestor_of(player)):
			return (player as Node2D).global_position
	if scene is Node2D:
		return Vector2(640, 360)
	return Vector2(640, 360)

func _spawn_echo_cluster(scene: Node, report: Dictionary, origin: Vector2, memory, count: int = 3) -> void:
	var color: Color = report.get("color", Color(0.86, 0.68, 0.44))
	var line := String(report.get("line", "A contour vanished."))
	var grade := int(report.get("grade", MemoryManager.MemoryGrade.GRADE_5))
	var radius := 34.0 + float(grade) * 10.0
	for i in range(count):
		var angle := (TAU / maxf(float(count), 1.0)) * float(i) + randf_range(-0.42, 0.42)
		var offset := Vector2(cos(angle), sin(angle)) * randf_range(18.0, radius)
		var echo := _make_echo_node(line, color, grade, i == 0)
		if echo is Node2D:
			(echo as Node2D).global_position = origin + offset
		scene.add_child(echo)
		_animate_echo(echo, color, i)

func _make_echo_node(line: String, color: Color, grade: int, show_text: bool) -> Node2D:
	var root := Node2D.new()
	root.name = "MemoryRewriteEcho"
	root.z_index = 96

	var shard := Polygon2D.new()
	var size := 13.0 + float(grade) * 3.0
	shard.polygon = PackedVector2Array([
		Vector2(0, -size),
		Vector2(size * 0.48, 0),
		Vector2(0, size),
		Vector2(-size * 0.48, 0),
	])
	shard.color = Color(color.r, color.g, color.b, 0.48)
	root.add_child(shard)

	var core := ColorRect.new()
	core.position = Vector2(-1, -size)
	core.size = Vector2(2, size * 2.0)
	core.color = Color(1.0, 0.92, 0.72, 0.68)
	root.add_child(core)

	if show_text:
		var label := Label.new()
		label.text = line
		label.position = Vector2(18, -20)
		label.custom_minimum_size = Vector2(260, 42)
		label.autowrap_mode = TextServer.AUTOWRAP_WORD
		label.add_theme_font_size_override("font_size", 12)
		label.add_theme_color_override("font_color", color.lightened(0.28))
		label.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75))
		label.add_theme_constant_override("shadow_offset_x", 1)
		label.add_theme_constant_override("shadow_offset_y", 1)
		root.add_child(label)

	return root

func _animate_echo(echo: Node, color: Color, index: int) -> void:
	if not is_instance_valid(echo):
		return
	echo.modulate = Color(1, 1, 1, 0.0)
	echo.scale = Vector2(0.72, 0.72)
	var delay := float(index) * 0.08
	var tween := echo.create_tween()
	tween.tween_interval(delay)
	tween.set_parallel(true)
	tween.tween_property(echo, "modulate:a", 1.0, 0.16)
	tween.tween_property(echo, "scale", Vector2(1.12, 1.12), 0.28).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.set_parallel(false)
	tween.tween_property(echo, "position:y", echo.position.y - 24.0, 2.1).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tween.parallel().tween_property(echo, "modulate:a", 0.0, 2.1).set_delay(0.35)
	tween.finished.connect(func():
		if is_instance_valid(echo):
			echo.queue_free()
	)

func _show_rewrite_art(report: Dictionary) -> void:
	var art_path := String(report.get("art", ""))
	if art_path == "" or not ResourceLoader.exists(art_path):
		return
	var layer := CanvasLayer.new()
	layer.layer = 22
	layer.name = "WorldRewriteArtFlash"
	layer.modulate.a = 0.0
	add_child(layer)

	var plate := TextureRect.new()
	plate.set_anchors_preset(Control.PRESET_FULL_RECT)
	plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	plate.texture = load(art_path)
	plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(plate)

	var wash := ColorRect.new()
	wash.set_anchors_preset(Control.PRESET_FULL_RECT)
	var tint: Color = report.get("color", Color(0.86, 0.68, 0.44))
	wash.color = Color(tint.r, tint.g, tint.b, 0.10)
	wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(wash)

	var shade := ColorRect.new()
	shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	shade.color = Color(0, 0, 0, 0.28)
	shade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(shade)

	var tween := create_tween()
	tween.tween_property(layer, "modulate:a", 0.82, 0.16)
	tween.tween_interval(0.62)
	tween.tween_property(layer, "modulate:a", 0.0, 0.46).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tween.finished.connect(func():
		if is_instance_valid(layer):
			layer.queue_free()
	)

func _grade_color(grade: int) -> Color:
	match grade:
		MemoryManager.MemoryGrade.GRADE_1:
			return Color(1.0, 0.36, 0.22)
		MemoryManager.MemoryGrade.GRADE_2:
			return Color(0.9, 0.46, 0.52)
		MemoryManager.MemoryGrade.GRADE_3:
			return Color(0.72, 0.58, 0.9)
		MemoryManager.MemoryGrade.GRADE_4:
			return Color(0.88, 0.70, 0.42)
	return Color(0.70, 0.76, 0.62)

func _fallback_art_for_grade(grade: int) -> String:
	if grade >= MemoryManager.MemoryGrade.GRADE_2:
		return "res://assets/cg/generated/memory_burn_arrel_name.png"
	return "res://assets/cg/generated/ui_loss_record_blank_book.png"

func _grade_name(grade: int) -> String:
	match grade:
		MemoryManager.MemoryGrade.GRADE_1:
			return "Grade 1 / Core"
		MemoryManager.MemoryGrade.GRADE_2:
			return "Grade 2 / Identity"
		MemoryManager.MemoryGrade.GRADE_3:
			return "Grade 3 / Relationship"
		MemoryManager.MemoryGrade.GRADE_4:
			return "Grade 4 / Daily Life"
	return "Grade 5 / Sensation"
