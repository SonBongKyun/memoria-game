## DialogueConditionSystem (Autoload)
## Read-only condition evaluator for Memory World Engine state.
## It is intentionally not wired into existing dialogue/VN pipelines in MVP 1.
extends Node


func evaluate(condition: Dictionary, _context: Dictionary = {}) -> bool:
	if condition.has("all"):
		var all_conditions: Variant = condition["all"]
		if not (all_conditions is Array):
			return false
		for child in all_conditions:
			if not (child is Dictionary) or not evaluate(child):
				return false
		return true

	if condition.has("any"):
		var any_conditions: Variant = condition["any"]
		if not (any_conditions is Array):
			return false
		for child in any_conditions:
			if child is Dictionary and evaluate(child):
				return true
		return false

	if condition.has("not"):
		var negated: Variant = condition["not"]
		return negated is Dictionary and not evaluate(negated)

	match String(condition.get("type", "")):
		"memory":
			return _evaluate_memory(condition)
		"knowledge":
			return _evaluate_knowledge(condition)
		_:
			return false


func _evaluate_memory(condition: Dictionary) -> bool:
	var actor_id := String(condition.get("actor", ""))
	var memory_id := String(condition.get("memory", ""))
	var expected_status := String(condition.get("status", WorldState.MEMORY_STATUS_ACTIVE))
	var record := WorldState.get_memory_record(actor_id, memory_id)
	match expected_status:
		WorldState.MEMORY_STATUS_ACTIVE:
			return MemoryEngine.check_memory(actor_id, memory_id)
		WorldState.MEMORY_STATUS_REMOVED:
			return not record.is_empty() and String(record.get("status", "")) == WorldState.MEMORY_STATUS_REMOVED
		"missing":
			return record.is_empty()
		_:
			return false


func _evaluate_knowledge(condition: Dictionary) -> bool:
	var actor_id := String(condition.get("actor", ""))
	var fact_id := String(condition.get("fact", ""))
	var expected := bool(condition.get("equals", true))
	return WorldState.knows_fact(actor_id, fact_id) == expected
