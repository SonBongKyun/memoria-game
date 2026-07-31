## SideQuest, 사이드 퀘스트 유틸리티 (class_name, 비-오토로드)
## 퀘스트 정의, 상태 확인, 보상 지급. 상태는 GameManager.story_flags에 저장.
class_name SideQuest

# 퀘스트 정의 목록
const QUESTS: Array = [
	{
		"id": "echoes_ash",
		"title": "Echoes in the Ash",
		"title_ko": "재 속의 메아리",
		"art": "res://assets/cg/generated/quest_echoes_ash_v1.png",
		"desc": "A ghostly figure near the old stump asks you to find scattered memory fragments in the forest.",
		"desc_ko": "오래된 그루터기 옆의 희미한 형체가, 숲에 흩어진 기억 조각을 찾아 달라고 부탁한다.",
		"map": "rim_forest",
		"chapter_req": 2,  # Ch1 완료 후 접근 가능
		"steps": [
			{"flag": "sq_echoes_ash_started", "desc": "Talk to the Ashen Figure in Rim Forest.", "desc_ko": "림 외곽 숲의 잿빛 형체와 대화하기"},
			{"flag": "sq_echoes_ash_frag1", "desc": "Find memory fragment near the mossy stones.", "desc_ko": "이끼 낀 돌무더기 근처에서 기억 조각 찾기"},
			{"flag": "sq_echoes_ash_frag2", "desc": "Find memory fragment near the fallen tree.", "desc_ko": "쓰러진 나무 근처에서 기억 조각 찾기"},
			{"flag": "sq_echoes_ash_complete", "desc": "Return to the Ashen Figure.", "desc_ko": "잿빛 형체에게 돌아가기"},
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
		"title_ko": "웅덩이의 장부",
		"art": "res://assets/cg/generated/quest_sump_ledger_v1.png",
		"desc": "A nervous trader wants a hidden ledger found in the market's back alleys.",
		"desc_ko": "불안해하는 상인이 시장 뒷골목에 숨겨진 장부를 찾아 달라고 한다.",
		"map": "verdan_market",
		"chapter_req": 3,  # Ch2 완료 후
		"steps": [
			{"flag": "sq_sump_ledger_started", "desc": "Talk to the Nervous Trader near the alley.", "desc_ko": "골목 근처의 불안한 상인과 대화하기"},
			{"flag": "sq_sump_ledger_found", "desc": "Find the hidden ledger in the Sump.", "desc_ko": "웅덩이에 숨겨진 장부 찾기"},
			{"flag": "sq_sump_ledger_done", "desc": "Decide: return the ledger or burn it.", "desc_ko": "선택하기: 장부를 돌려주거나 태우거나"},
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
		"title_ko": "세이블의 불침번",
		"art": "res://assets/cg/generated/quest_sable_vigil_v2.png",
		"desc": "Sable asks for help clearing a Void Watcher near the BL-07 entrance.",
		"desc_ko": "세이블이 BL-07 입구 근처의 공허 감시자를 처리해 달라고 청한다.",
		"map": "the_seam",
		"chapter_req": 4,
		"prereq_flag": "sable_joined",
		"steps": [
			{"flag": "sq_sable_vigil_started", "desc": "Talk to Sable about void patrols.", "desc_ko": "세이블과 공허 순찰에 대해 이야기하기"},
			{"flag": "sq_sable_vigil_killed", "desc": "Defeat the Void Watcher near the rift.", "desc_ko": "균열 근처의 공허 감시자 처치하기"},
			{"flag": "sq_sable_vigil_complete", "desc": "Report back to Sable.", "desc_ko": "세이블에게 보고하기"},
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
	{
		"id": "echo_fragments",
		"title": "Echoes of the Threshold",
		"title_ko": "문턱의 메아리",
		"art": "res://assets/cg/generated/quest_echo_threshold_v1.png",
		"desc": "Collect echo fragments scattered across the Seam Outskirts.",
		"map": "seam_outskirts",
		"chapter_req": 7,
		"prereq_flag": "",
		"steps": [
			{"flag": "sq_echo_fragments_started", "desc": "Search the Seam Outskirts for echo shards.", "desc_ko": "심 외곽에서 메아리 파편 찾기"},
			{"flag": "sq_echo_frag1", "desc": "Find the first echo shard near the cliff.", "desc_ko": "절벽 근처에서 첫 번째 파편 찾기"},
			{"flag": "sq_echo_frag2", "desc": "Find the second echo shard in the ruins.", "desc_ko": "폐허에서 두 번째 파편 찾기"},
			{"flag": "sq_echo_complete", "desc": "Return to the resonance point.", "desc_ko": "공명 지점으로 돌아가기"},
		],
		"reward_grains": 80,
		"reward_items": {"potion": 2},
		"reward_memory": {
			"id": "echo_threshold",
			"title": "Echo of the Threshold",
			"desc": "A memory that reverberates at the boundary.",
			"grade": 2,
			"burn_power": 18,
			"effect": "A faint hum lingers.",
		},
		"npc": "Sable",
	},
	{
		"id": "forest_parasite",
		"title": "The Parasite's Root",
		"title_ko": "기생하는 뿌리",
		"art": "res://assets/cg/generated/quest_forest_parasite_v1.png",
		"desc": "Trace the memory-parasitic vines to their source in the Forgotten Forest.",
		"map": "forgotten_forest",
		"chapter_req": 8,
		"prereq_flag": "",
		"steps": [
			{"flag": "sq_forest_parasite_started", "desc": "Search the Forgotten Forest for parasitic vines.", "desc_ko": "잊힌 숲에서 기생 덩굴 찾기"},
			{"flag": "sq_parasite_found", "desc": "Find the parasitic vine cluster.", "desc_ko": "기생 덩굴 군락 찾기"},
			{"flag": "sq_parasite_burned", "desc": "Burn a memory to purge the root.", "desc_ko": "기억을 태워 뿌리 정화하기"},
			{"flag": "sq_parasite_complete", "desc": "Confirm the vines have withered.", "desc_ko": "덩굴이 시들었는지 확인하기"},
		],
		"reward_grains": 100,
		"reward_items": {"antidote": 3, "fire_bomb": 1},
		"reward_memory": {
			"id": "forest_root",
			"title": "Root of Forgetting",
			"desc": "The forest remembers what you cannot.",
			"grade": 3,
			"burn_power": 22,
			"effect": "Vines recede.",
		},
		"npc": "Tobias",
	},
	{
		"id": "colorless_compass",
		"title": "Calibrating the Compass",
		"title_ko": "나침반 교정",
		"art": "res://assets/cg/generated/quest_colorless_compass_v2.png",
		"desc": "The Memory Compass needs calibration, find three anchor points in the Colorless Waste.",
		"map": "colorless_waste",
		"chapter_req": 9,
		"prereq_flag": "ch9_compass",
		"steps": [
			{"flag": "sq_colorless_compass_started", "desc": "Begin compass calibration.", "desc_ko": "나침반 교정 시작하기"},
			{"flag": "sq_compass_anchor1", "desc": "Find the northern anchor point.", "desc_ko": "북쪽 정박점 찾기"},
			{"flag": "sq_compass_anchor2", "desc": "Find the eastern anchor point.", "desc_ko": "동쪽 정박점 찾기"},
			{"flag": "sq_compass_calibrated", "desc": "Calibrate the compass at the center.", "desc_ko": "중심에서 나침반 교정하기"},
		],
		"reward_grains": 120,
		"reward_items": {"smoke_bomb": 2, "potion": 3},
		"reward_memory": {
			"id": "compass_memory",
			"title": "True North",
			"desc": "A memory that always points toward what matters.",
			"grade": 3,
			"burn_power": 25,
			"effect": "Direction clarifies.",
		},
		"npc": "Elia",
	},
]

## 퀘스트 사용 가능 여부 (챕터 + 전제조건 충족, 아직 시작 안 함)
## S216: 퀘스트 표시용 로케일 헬퍼.
## 이 게임은 한국어가 기본인데 퀘스트 데이터는 영문만 있어서, 사이드 퀘스트를
## 수락하는 순간 한국어 HUD 한가운데 "Echoes in the Ash"가 떴다.
static func loc(source: Dictionary, key: String) -> String:
	if GameManager.current_locale == "ko":
		var ko := String(source.get(key + "_ko", ""))
		if ko != "":
			return ko
	return String(source.get(key, ""))

## 퀘스트의 현재 단계 설명 (로케일 반영). 다 끝났으면 빈 문자열.
static func get_current_step_text(quest_id: String) -> String:
	var quest := _find_quest(quest_id)
	if quest.is_empty():
		return ""
	var steps: Array = quest.get("steps", [])
	for step: Dictionary in steps:
		if not GameManager.get_flag(String(step.get("flag", ""))):
			return loc(step, "desc")
	return ""

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
				step_desc = loc(steps[step_idx], "desc")
		elif is_available(qid):
			status = "available"
		# S216: 로케일 필드와 단계 배열까지 함께 전달한다.
		# 예전에는 여기서 새 딕셔너리를 만들며 title_ko/desc_ko/steps를 버려서,
		# 호출자가 아무리 loc()을 써도 영문 원본밖에 받지 못했다.
		result.append({
			"id": qid,
			"title": quest["title"],
			"title_ko": quest.get("title_ko", ""),
			"art": quest.get("art", ""),
			"desc": quest["desc"],
			"desc_ko": quest.get("desc_ko", ""),
			"status": status,
			"step_desc": step_desc,
			"steps": quest.get("steps", []),
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
