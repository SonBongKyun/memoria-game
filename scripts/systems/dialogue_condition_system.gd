## DialogueConditionSystem (Autoload)
## Read-only condition evaluator for Memory World Engine state.
## DialogueManager calls it only for the opt-in structured `condition` field.
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
	var status_matches := false
	match expected_status:
		WorldState.MEMORY_STATUS_ACTIVE:
			status_matches = MemoryEngine.check_memory(actor_id, memory_id)
		WorldState.MEMORY_STATUS_REMOVED:
			status_matches = not record.is_empty() \
				and String(record.get("status", "")) == WorldState.MEMORY_STATUS_REMOVED
		"missing":
			status_matches = record.is_empty()
		_:
			return false
	if not status_matches:
		return false
	# Optional restored matching reads existing deterministic audit metadata. It
	# does not introduce another memory status or mutate the record.
	if condition.has("restored"):
		var was_restored := int(record.get("restored_revision", 0)) > 0
		return was_restored == bool(condition.get("restored", false))
	return true


func _evaluate_knowledge(condition: Dictionary) -> bool:
	var actor_id := String(condition.get("actor", ""))
	var fact_id := String(condition.get("fact", ""))
	var expected := bool(condition.get("equals", true))
	return MemoryEngine.knows_fact(actor_id, fact_id) == expected
