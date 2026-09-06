extends Node

var domain: Node
var observed: Array = []

func _ready() -> void:
	call_deferred("_run")

func _snapshot() -> Dictionary:
	var owned: Array = []
	var effective: Array = []
	var grades: Array = []
	var weights: Array = []
	var intact: Array = []
	for m in domain.memories:
		owned.append({"id": m.id, "burned": m.is_burned, "residue": m.is_residue,
			"faded": m.is_faded, "erosion": m.erosion, "connections": m.connections.duplicate()})
		effective.append(domain.get_effective_burn_power(m))
		grades.append(m.grade)
		weights.append(domain._carry_weight_of(m))
		intact.append(domain.is_intact(m.id))
	var history: Array = []
	for m in domain.burned_memories:
		history.append(m.id)
	var available: Array = []
	for m in domain.get_available_memories():
		available.append(m.id)
	var available_faded: Array = []
	for m in domain.get_available_memories(0, true):
		available_faded.append(m.id)
	var residue: Array = []
	for m in domain.get_residue_memories():
		residue.append(m.id)
	return {"owned": owned, "history": history, "passives": domain.burn_passives.duplicate(true),
		"guards": domain.erosion_guarded.keys(), "loan": domain.active_loan.duplicate(true),
		"extracted": domain.extracted_memories.duplicate(), "anchor_vigil": domain.anchor_vigil,
		"anchor_passives": domain.anchor_passives.duplicate(true), "vigil_chapters": domain._vigil_chapters_counted.keys(),
		"guard_slots_used": domain._guard_slots_used, "carry_weight": domain.get_carry_weight(),
		"carry_capacity": domain.get_carry_capacity(), "overload": domain.get_carry_overload_ratio(),
		"voluntary_burn_count": domain.get_voluntary_burn_count(), "available": available,
		"available_allow_faded": available_faded, "residue_ids": residue,
		"effective_powers": effective, "grade_ordinals": grades, "weights": weights, "intact": intact}

func _event(kind: String, memory_id: String = "", amount: int = 0, affected: Array = [], weight: int = 0, capacity: int = 0, passive_name: String = "") -> void:
	var affected_ids: Array = []
	for m in affected:
		affected_ids.append(m.id)
	observed.append({"kind": kind, "memory_id": memory_id, "amount": amount,
		"affected_ids": affected_ids, "weight": weight, "capacity": capacity,
		"passive_name": passive_name, "observation": _snapshot()})

func _make_memory(row: Dictionary):
	var m = MemoryManager.Memory.new(String(row.id), String(row.get("title", row.id)),
		String(row.get("description", "")), int(row.grade), int(row.burn_power),
		String(row.get("story_effect", "")), String(row.get("related_npc", "")))
	m.is_burned = bool(row.get("burned", false))
	m.is_residue = bool(row.get("residue", false))
	m.is_faded = bool(row.get("faded", false))
	m.erosion = int(row.get("erosion", 0))
	return m

func _run_case(test: Dictionary) -> Dictionary:
	GameManager.current_chapter = int(test.context.get("chapter", 1))
	GameManager.player_data.elia_with_party = bool(test.context.get("elia", true))
	GameManager.story_flags = {"oath_still_sworn": bool(test.context.get("still", false))}
	# A second ORIGINAL script instance kept outside the SceneTree: core methods
	# execute unchanged; only presentation-only milestone rendering is naturally off.
	domain = load("res://memory_manager.gd").new()
	for row in test.memories:
		domain.memories.append(_make_memory(row))
	for id in test.get("history", []):
		domain.burned_memories.append(domain.find_memory(String(id)))
	domain.burn_passives = test.get("passives", {}).duplicate(true)
	domain.anchor_vigil = int(test.get("anchor_vigil", 0))
	domain.anchor_passives = test.get("anchor_passives", {}).duplicate(true)
	for chapter in test.get("vigil_chapters", []):
		domain._vigil_chapters_counted[int(chapter)] = true
	for id in test.get("guards", []):
		domain.erosion_guarded[String(id)] = true
	domain._guard_slots_used = int(test.get("guard_slots_used", 0))
	domain.active_loan = test.get("loan", {}).duplicate(true)
	for id in test.get("extracted", []):
		domain.extracted_memories.append(String(id))
	domain._refresh_connections()
	domain.memory_added.connect(func(m): _event("Added", m.id))
	domain.memory_became_residue.connect(func(m): _event("ResidueCreated", m.id))
	domain.memory_burned.connect(func(m): _event("Burned", m.id))
	domain.memory_faded.connect(func(m): _event("Faded", m.id))
	domain.memory_cascaded.connect(func(m, affected, amount): _event("Cascaded", m.id, amount, affected))
	domain.carry_state_changed.connect(func(weight, capacity): _event("CarryChanged", "", 0, [], weight, capacity))
	domain.passive_unlocked.connect(func(name): _event("PassiveUnlocked", "", 0, [], 0, 0, name))
	domain.memories_eroded.connect(func(count): _event("MemoriesEroded", "", count))
	var output := {"id": test.id, "initial": _snapshot(), "steps": []}
	for command in test.commands:
		observed = []
		var success := true
		match String(command.op):
			"burn": success = domain.burn_memory(String(command.id), bool(command.get("allow_faded", false))) != null
			"silent": success = domain.burn_memory_silent(String(command.id), bool(command.get("allow_faded", false))) != null
			"erode": domain.apply_erosion(int(command.chapter))
			"passives": domain.check_unlock_passives()
			"add": domain.add_memory(_make_memory(command.memory))
			"query":
				# Repeated residue reads must remain reads, not generic removal.
				for m in domain.get_residue_memories():
					domain.get_residue_memory(m.id)
					domain.get_residue_memory(m.id)
			_: push_error("Unsupported oracle command: %s" % command.op); get_tree().quit(2); return {}
		output.steps.append({"success": success, "events": observed.duplicate(true), "state": _snapshot()})
	domain.free()
	domain = null
	return output

func _run() -> void:
	var args := OS.get_cmdline_user_args()
	if args.size() != 2:
		push_error("Expected input and output file paths"); get_tree().quit(2); return
	var parsed = JSON.parse_string(FileAccess.get_file_as_string(args[0]))
	if not (parsed is Dictionary) or int(parsed.get("schema_version", 0)) != 1:
		push_error("Invalid input fixture schema"); get_tree().quit(2); return
	var results: Array = []
	for test in parsed.cases:
		results.append(_run_case(test))
	var file := FileAccess.open(args[1], FileAccess.WRITE)
	if file == null:
		push_error("Cannot write oracle output"); get_tree().quit(2); return
	file.store_string(JSON.stringify({"schema_version": 1, "cases": results}, "\t"))
	file.close()
	print("MEMORIA_MEMORY_ORACLE_PASS cases=%d engine=%s" % [results.size(), Engine.get_version_info().string])
	get_tree().quit(0)
