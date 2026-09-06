extends "res://oracle.gd"

# Execute the unchanged source initializer outside the tree. The inherited
# observer/comparator harness is the accepted Phase 1A Godot oracle.
func _run() -> void:
	var args := OS.get_cmdline_user_args()
	if args.size() != 1:
		push_error("Starting catalog exporter requires one output path"); get_tree().quit(2); return
	domain = load("res://memory_manager.gd").new()
	domain._init_starting_memories()
	if domain.memories.is_empty():
		push_error("Source initializer produced no memories"); get_tree().quit(2); return
	var fields: Array = ["id", "title", "description", "grade", "burn_power", "story_effect", "related_npc", "is_burned", "is_residue", "is_faded", "erosion", "connections"]
	var entries: Array = []
	var rows: Array = []
	for m in domain.memories:
		var actual: Array = []
		for p in m.get_property_list():
			if int(p.usage) & PROPERTY_USAGE_SCRIPT_VARIABLE:
				actual.append(String(p.name))
		actual.sort(); fields.sort()
		if actual != fields or m.is_burned or m.is_residue or m.is_faded or m.erosion != 0:
			push_error("Unreviewed Memory fields or authored initial state"); get_tree().quit(2); return
		var row := {"id": m.id, "title": m.title, "description": m.description, "grade": m.grade, "burn_power": m.burn_power, "story_effect": m.story_effect, "related_npc": m.related_npc}
		rows.append(row.duplicate(true))
		var ko: Dictionary = domain.MEMORY_TEXT_KO.get(m.id, {})
		for key in ko:
			if key not in ["title", "desc", "effect"]:
				push_error("Unreviewed localization field"); get_tree().quit(2); return
		if not ko.has("title") or not ko.has("desc"):
			push_error("Missing authored Korean memory text"); get_tree().quit(2); return
		row["localization_ko"] = {"title": ko.title, "description": ko.desc, "story_effect": ko.get("effect", null)}
		entries.append(row)
	var initial := _snapshot()
	domain.free(); domain = null
	var commands: Array = [{"op": "erode", "chapter": 2}]
	for i in range(rows.size()):
		if rows[i].grade < 4:
			commands.append({"op": "silent" if i == 1 else "burn", "id": rows[i].id, "allow_faded": true})
	commands.append({"op": "passives"})
	var behavior := _run_case({"id": "starting_catalog_commands", "context": {"chapter": 1, "elia": true, "still": false}, "memories": rows, "commands": commands})
	var file := FileAccess.open(args[0], FileAccess.WRITE)
	if file == null:
		push_error("Cannot write catalog extraction"); get_tree().quit(2); return
	file.store_string(JSON.stringify({"entries": entries, "initial": initial, "behavior_input": {"commands": commands}, "behavior": behavior}))
	file.close()
	print("MEMORIA_STARTING_CATALOG_EXTRACTED entries=%d" % entries.size())
	get_tree().quit(0)
