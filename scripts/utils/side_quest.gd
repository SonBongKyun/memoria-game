## SideQuest — 사이드 퀘스트 유틸리티 (class_name, 비-오토로드)
## 퀘스트 정의, 상태 확인, 보상 지급. 상태는 GameManager.story_flags에 저장.
class_name SideQuest

# 퀘스트 정의 목록
const QUESTS: Array = [
	{
		"id": "echoes_ash",
		"title": "Echoes in the Ash",
		"desc": "A ghostly figure near the old stump asks you to find scattered memory fragments in the forest.",
		"map": "rim_forest",
		"chapter_req": 2,  # Ch1 완료 후 접근 가능
		"steps": [
			{"flag": "sq_echoes_ash_started", "desc": "Talk to the Ashen Figure in Rim Forest."},
			{"flag": "sq_echoes_ash_frag1", "desc": "Find memory fragment near the mossy stones."},
			{"flag": "sq_echoes_ash_frag2", "desc": "Find memory fragment near the fallen tree."},
			{"flag": "sq_echoes_ash_complete", "desc": "Return to the Ashen Figure."},
		],
		"reward_grains": 25,
		"reward_items": {"potion": 1},
		"reward_memory": {
			"id": "sq_childs_song",
			"title": "A Child's Counting Song",
			"desc": "One, two, three... the voice trails off. You know this melody. You knew it before you knew anything.",
			"grade": 1,  # MemoryGrade.GRADE_4
			"burn_power": 30,
			"effect": "Lose the ability to count past ten without pausing.",
		},
		"npc": "Ashen Figure",
	},
	{
		"id": "sump_ledger",
		"title": "The Sump Ledger",
		"desc": "A nervous trader wants a hidden ledger found in the market's back alleys.",
		"map": "verdan_market",
		"chapter_req": 3,  # Ch2 완료 후
		"steps": [
			{"flag": "sq_sump_ledger_started", "desc": "Talk to the Nervous Trader near the alley."},
			{"flag": "sq_sump_ledger_found", "desc": "Find the hidden ledger in the Sump."},
			{"flag": "sq_sump_ledger_done", "desc": "Decide: return the ledger or burn it."},
		],
		"reward_grains": 40,
		"reward_items": {"hi_potion": 1},
		"reward_memory": {
			"id": "sq_debt_ash",
			"title": "Debt Written in Ash",
			"desc": "Names and numbers, all owed, all forgotten. Someone paid dearly for this silence.",
			"grade": 2,  # MemoryGrade.GRADE_3
			"burn_power": 55,
			"effect": "Lose awareness of financial transactions around you.",
		},
		"npc": "Nervous Trader",
	},
	{
		"id": "sable_vigil",
		"title": "Sable's Vigil",
		"desc": "Sable asks for help clearing a Void Watcher near the BL-07 entrance.",
		"map": "the_seam",
		"chapter_req": 4,
		"prereq_flag": "sable_joined",
		"steps": [
			{"flag": "sq_sable_vigil_started", "desc": "Talk to Sable about void patrols."},
			{"flag": "sq_sable_vigil_killed", "desc": "Defeat the Void Watcher near the rift."},
			{"flag": "sq_sable_vigil_complete", "desc": "Report back to Sable."},
		],
		"reward_grains": 30,
		"reward_items": {"firebomb": 2},
		"reward_memory": {
			"id": "sq_soldiers_oath",
			"title": "A Soldier's Oath",
			"desc": "She swore to come back. She did. The oath didn't say anything about coming back whole.",
			"grade": 2,  # MemoryGrade.GRADE_3
			"burn_power": 60,
			"effect": "Sable's presence feels distant. Trust without understanding.",
			"npc": "Sable",
		},
		"npc": "Sable",
	},
]

## 퀘스트 사용 가능 여부 (챕터 + 전제조건 충족, 아직 시작 안 함)
static func is_available(quest_id: String) -> bool:
	var quest = _find_quest(quest_id)
	if quest.is_empty():
		return false
	if GameManager.current_chapter < quest["chapter_req"]:
		return false
	if quest.has("prereq_flag") and not GameManager.get_flag(quest["prereq_flag"]):
		return false
	if GameManager.get_flag("sq_%s_started" % quest_id):
		return false  # 이미 시작됨
	return true

## 퀘스트 진행 중 여부
static func is_active(quest_id: String) -> bool:
	return GameManager.get_flag("sq_%s_started" % quest_id) and not is_complete(quest_id)

## 퀘스트 완료 여부
static func is_complete(quest_id: String) -> bool:
	var quest = _find_quest(quest_id)
	if quest.is_empty():
		return false
	var steps = quest["steps"] as Array
	var last_flag = steps[steps.size() - 1]["flag"]
	return GameManager.get_flag(last_flag)

## 현재 진행 단계 인덱스 (0-based)
static func get_current_step(quest_id: String) -> int:
	var quest = _find_quest(quest_id)
	if quest.is_empty():
		return -1
	var steps = quest["steps"] as Array
	for i in range(steps.size()):
		if not GameManager.get_flag(steps[i]["flag"]):
			return i
	return steps.size()  # 모두 완료

## 단계 진행 (플래그 설정)
static func advance_step(quest_id: String, step_flag: String) -> void:
	GameManager.set_flag(step_flag, true)
	var quest = _find_quest(quest_id)
	if quest.is_empty():
		return
	# 완료 여부 체크
	var steps = quest["steps"] as Array
	var last_flag = steps[steps.size() - 1]["flag"]
	if step_flag == last_flag:
		_grant_rewards(quest)

## 보상 지급
static func _grant_rewards(quest: Dictionary) -> void:
	# Grains
	var grains = quest.get("reward_grains", 0)
	if grains > 0:
		GameManager.player_data.grains += grains
		NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)

	# 아이템
	var items = quest.get("reward_items", {}) as Dictionary
	for item_id in items:
		for i in range(items[item_id]):
			GameManager.add_item(item_id)

	# 기억 보상
	var mem_data = quest.get("reward_memory")
	if mem_data != null and mem_data is Dictionary:
		var mem = MemoryManager.Memory.new(
			mem_data["id"],
			mem_data["title"],
			mem_data["desc"],
			mem_data["grade"],
			mem_data["burn_power"],
			mem_data.get("effect", ""),
			mem_data.get("npc", "")
		)
		MemoryManager.add_memory(mem)

	NotificationToast.show_toast("Quest Complete: %s" % quest["title"], NotificationToast.ToastType.SUCCESS)
	AchievementManager.check_quest_complete()

## 전체 퀘스트 목록 (상태 포함)
static func get_all_quests() -> Array:
	var result: Array = []
	for quest in QUESTS:
		var qid = quest["id"]
		var status = "locked"
		var step_desc = ""
		if is_complete(qid):
			status = "complete"
		elif is_active(qid):
			status = "active"
			var step_idx = get_current_step(qid)
			var steps = quest["steps"] as Array
			if step_idx < steps.size():
				step_desc = steps[step_idx]["desc"]
		elif is_available(qid):
			status = "available"
		result.append({
			"id": qid,
			"title": quest["title"],
			"desc": quest["desc"],
			"status": status,
			"step_desc": step_desc,
			"npc": quest.get("npc", ""),
			"map": quest.get("map", ""),
		})
	return result

## 퀘스트 정의 검색
static func _find_quest(quest_id: String) -> Dictionary:
	for quest in QUESTS:
		if quest["id"] == quest_id:
			return quest
	return {}
