## EventBus (Autoload)
## Memory World Engine이 확정한 deterministic world event만 전달한다.
## 상태를 소유하거나 변경하지 않으며, 소비자는 전달받은 복사본만 읽는다.
extends Node

signal world_event_committed(event: Dictionary)


func emit_world_event_committed(event: Dictionary) -> bool:
	if String(event.get("event_id", "")) == "":
		push_warning("[EventBus] Rejected world event without event_id")
		return false
	if String(event.get("type", "")) == "":
		push_warning("[EventBus] Rejected world event without type")
		return false
	if int(event.get("sequence", 0)) <= 0:
		push_warning("[EventBus] Rejected world event without a positive sequence")
		return false
	world_event_committed.emit(event.duplicate(true))
	return true
