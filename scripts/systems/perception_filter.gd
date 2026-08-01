## PerceptionFilter
## S64: 기억 상태에 따라 월드가 다르게 보이도록 필터링. 특정 기억을 태우면 NPC/오브젝트가 사라지거나,
## 반대로 잊혀진 것만이 드러나기도 함. 맵 씬의 _ready 말미에 apply(self)를 호출하면 그룹 기반으로 자동 적용.
##
## 사용법:
##   노드에 다음 메타를 설정:
##     requires_memory_intact: "memory_id" , 이 기억이 태워지면 노드 숨김
##     requires_memory_burned: "memory_id" , 이 기억이 태워져야 노드 표시
##     on_burned_replace_dialogue: "new_dialogue_key" , NPC에 한해 기억 태움 시 대화 키 교체
##     on_burned_tint: Color               , 기억 태움 시 sprite에 color tint 적용 (창백함 등)
##
##   또는 그룹으로:
##     "perception_intact_<memory_id>", 태워지면 숨김
##     "perception_burned_<memory_id>", 태워져야 표시
class_name PerceptionFilter
extends RefCounted

## 씬 루트에 대해 필터 적용
static func apply(scene: Node) -> void:
	_apply_meta_filters(scene)
	_apply_group_filters(scene)
	_apply_npc_dialogue_replacements(scene)

## 메타데이터 기반 처리
static func _apply_meta_filters(node: Node) -> void:
	for child in node.get_children():
		_apply_meta_filters(child)

	if not (node is CanvasItem):
		return

	if node.has_meta("requires_memory_intact"):
		var mid = String(node.get_meta("requires_memory_intact"))
		if MemoryManager.is_memory_burned(mid):
			node.visible = false
			_disable_collisions(node)

	if node.has_meta("requires_memory_burned"):
		var mid = String(node.get_meta("requires_memory_burned"))
		if not MemoryManager.is_memory_burned(mid):
			node.visible = false
			_disable_collisions(node)

	# 태움 시 틴트 적용
	if node.has_meta("on_burned_tint_memory"):
		var mid = String(node.get_meta("on_burned_tint_memory"))
		if MemoryManager.is_memory_burned(mid) and node is CanvasItem:
			var tint = node.get_meta("on_burned_tint", Color(0.6, 0.55, 0.5, 0.75))
			(node as CanvasItem).modulate = tint

## 그룹 기반 처리 (메타 대신 그룹 사용)
static func _apply_group_filters(scene: Node) -> void:
	var tree = scene.get_tree()
	if tree == null:
		return
	# 모든 기억에 대해 그룹 검색
	for m in MemoryManager.memories + MemoryManager.burned_memories:
		var intact_group = "perception_intact_" + m.id
		var burned_group = "perception_burned_" + m.id
		for n in tree.get_nodes_in_group(intact_group):
			if not scene.is_ancestor_of(n):
				continue
			if m.is_burned:
				n.visible = false
				_disable_collisions(n)
		for n in tree.get_nodes_in_group(burned_group):
			if not scene.is_ancestor_of(n):
				continue
			if not m.is_burned:
				n.visible = false
				_disable_collisions(n)

## NPC 대화 키 교체 (기억 태움 시 다른 대화 출력)
static func _apply_npc_dialogue_replacements(scene: Node) -> void:
	var npcs = scene.get_tree().get_nodes_in_group("npcs") if scene.get_tree() else []
	for npc in npcs:
		if not scene.is_ancestor_of(npc):
			continue
		# npc 메타에 기억별 대화 교체가 있으면 순회 검사
		for meta_key in npc.get_meta_list():
			if not meta_key.begins_with("burned_dialogue_"):
				continue
			var mid = meta_key.substr("burned_dialogue_".length())
			if MemoryManager.is_memory_burned(mid):
				# 기억이 태워졌으면 대화 키 교체
				if "dialogue_key" in npc:
					npc.dialogue_key = String(npc.get_meta(meta_key))
				break

## S226: 이 인물이 특정 기억 연소 이후 처음 말을 걸었을 때 한 번만 나오는 반응 대사.
## 연소는 알림 한 번으로 끝나지 않고, 같은 동선에서 사람의 입을 통해 다시 돌아온다.
##
## 메타 형식:
##   burn_reaction_<memory_id> = "<dialogue_key>"
##   burn_reaction_<memory_id> = "res://data/chapterN_dialogue.json::<dialogue_key>"
## 두 번째 형식은 다른 챕터 파일에 있는 대사를 재사용할 때 쓴다.
##
## 반환: {"file": String, "key": String} 또는 빈 Dictionary.
## 실제로 존재하는 대사만 소모 처리하므로, 키가 없으면 플래그도 태우지 않는다.
static func take_burn_reaction(node: Node, default_file: String) -> Dictionary:
	if node == null:
		return {}
	for meta_key in node.get_meta_list():
		if not meta_key.begins_with("burn_reaction_"):
			continue
		var memory_id = meta_key.substr("burn_reaction_".length())
		if not MemoryManager.is_memory_burned(memory_id):
			continue
		var raw = String(node.get_meta(meta_key))
		if raw == "":
			continue
		var file_path = default_file
		var dialogue_key = raw
		var split_at = raw.rfind("::")
		if split_at > 0:
			file_path = raw.substr(0, split_at)
			dialogue_key = raw.substr(split_at + 2)
		if dialogue_key == "" or file_path == "":
			continue
		var flag = "burn_reaction_heard_%s" % dialogue_key
		if GameManager.get_flag(flag):
			continue
		if not DialogueManager.load_dialogue_file(file_path):
			continue
		if not DialogueManager.loaded_dialogues[file_path].has(dialogue_key):
			push_warning("[PerceptionFilter] Missing burn reaction '%s' in %s" % [dialogue_key, file_path])
			continue
		GameManager.set_flag(flag, true)
		return {"file": file_path, "key": dialogue_key}
	return {}

## CollisionObject2D 비활성화 (숨긴 오브젝트를 통과하게)
static func _disable_collisions(node: Node) -> void:
	if node is CollisionObject2D:
		(node as CollisionObject2D).collision_layer = 0
		(node as CollisionObject2D).collision_mask = 0
	for child in node.get_children():
		_disable_collisions(child)
