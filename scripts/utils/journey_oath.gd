## JourneyOath, 여정 맹세.
##
## 전투 디렉티브(전술 목표)를 여정 단위로 확장한 자기부여 제약. 플레이어는
## 언제든 세 가지 맹세 중 하나를 세울 수 있고, 지키는 동안 작은 상수 보상을
## 얻는다. 깨진 맹세는 이번 여정에서 되돌릴 수 없다(숨은 벌칙은 없다 — S226
## 원칙: 대가는 항상 미리 이름을 붙여 보여 준다).
##
## 상태는 GameManager.story_flags(oath_*_sworn / oath_*_broken)와
## player_data["oath_chapters_kept"]에 저장된다. 세이브 스키마 변경 없이
## 기존 직렬화를 타고 다니며, NG+의 story_flags.clear()로 자연 리셋된다.
class_name JourneyOath

const ASH := "ash"          # 잿불의 맹세: 기억을 팔지 않는다
const WITNESS := "witness"  # 증인의 맹세: 목격한 위협을 건너뛰지 않는다
const STILL := "still"      # 고요한 손: 엘리아와 얽힌 기억을 직접 태우지 않는다

const CHAPTER_KEEP_GRAINS: int = 40
const WITNESS_FLOW_MULT: float = 1.15

## 맹세 정의. desc의 규칙은 한 문장으로 읽히야 한다 — 위반 판정과 정확히 같은 범위.
const OATHS: Array[Dictionary] = [
	{
		"id": ASH,
		"name_en": "Oath of Ash",
		"name_ko": "잿불의 맹세",
		"vow_en": "Sell no memory on this journey. What I carry, I carry or I burn.",
		"vow_ko": "이 여정에서 기억을 팔지 않는다. 지닌 것은 지니거나, 내 손으로 태운다.",
		"reward_en": "+40 Grains each chapter kept",
		"reward_ko": "지킨 챕터마다 Grains +40",
	},
	{
		"id": WITNESS,
		"name_en": "Oath of Witness",
		"name_ko": "증인의 맹세",
		"vow_en": "Slip past no threat I have seen. What hunts me deserves a witness.",
		"vow_ko": "목격한 위협을 건너뛰지 않는다. 나를 사냥하는 것도 증언할 가치가 있다.",
		"reward_en": "Field Flow builds +15% faster while kept",
		"reward_ko": "지키는 동안 필드 흐름 축적 +15%",
	},
	{
		"id": STILL,
		"name_en": "Oath of Still Hands",
		"name_ko": "고요한 손",
		"vow_en": "Never burn a memory tied to Elia by my own hand.",
		"vow_ko": "엘리아와 얽힌 기억을 내 손으로 직접 태우지 않는다.",
		"reward_en": "Elia-tied memories stop eroding while kept",
		"reward_ko": "지키는 동안 엘리아 관련 기억이 침식되지 않는다",
	},
]


static func _flag_sworn(id: String) -> String:
	return "oath_%s_sworn" % id


static func _flag_broken(id: String) -> String:
	return "oath_%s_broken" % id


static func definition(id: String) -> Dictionary:
	for oath in OATHS:
		if oath.id == id:
			return oath
	return {}


static func is_sworn(id: String) -> bool:
	return GameManager.get_flag(_flag_sworn(id))


static func is_broken(id: String) -> bool:
	return GameManager.get_flag(_flag_broken(id))


static func is_active(id: String) -> bool:
	return is_sworn(id) and not is_broken(id)


## 맹세하기. 이미 서약했거나 깨진 맹세는 다시 세울 수 없다.
static func swear(id: String) -> bool:
	if id == "" or definition(id).is_empty() or is_sworn(id):
		return false
	GameManager.set_flag(_flag_sworn(id))
	if not GameManager.player_data.has("oath_chapters_kept"):
		GameManager.player_data["oath_chapters_kept"] = {}
	GameManager.player_data["oath_chapters_kept"][id] = 0
	var oath := definition(id)
	var name_text: String = oath.name_ko if GameManager.current_locale == "ko" else oath.name_en
	NotificationToast.show_toast(
		("맹세했다: %s" % name_text) if GameManager.current_locale == "ko" else ("Sworn: %s" % name_text),
		NotificationToast.ToastType.SUCCESS
	)
	print("[JourneyOath] Sworn: %s" % id)
	return true


## 맹세 깨기. 이유는 항상 토스트로 이름을 붙여 알린다.
static func break_oath(id: String, reason_ko: String, reason_en: String) -> void:
	if not is_active(id):
		return
	GameManager.set_flag(_flag_broken(id))
	var oath := definition(id)
	var name_text: String = oath.name_ko if GameManager.current_locale == "ko" else oath.name_en
	var reason: String = reason_ko if GameManager.current_locale == "ko" else reason_en
	NotificationToast.show_toast(
		("맹세가 깨졌다: %s — %s" % [name_text, reason]) if GameManager.current_locale == "ko" else ("Oath broken: %s — %s" % [name_text, reason]),
		NotificationToast.ToastType.WARNING
	)
	print("[JourneyOath] BROKEN: %s (%s)" % [id, reason])


## 위반 진입점. 각 시스템은 자기 맹세만 신고한다.
static func on_memory_sold() -> void:
	break_oath(ASH, "기억을 팔았다.", "a memory was sold.")


static func on_threat_bypassed() -> void:
	break_oath(WITNESS, "목격한 위협을 건너뛰었다.", "a witnessed threat was bypassed.")


static func on_player_burn(memory: MemoryManager.Memory) -> void:
	if memory == null:
		return
	if memory.related_npc == "Elia":
		break_oath(STILL, "엘리아와 얽힌 기억을 태웠다.", "a memory tied to Elia was burned.")


## 챕터 정산. add_chapter_memories의 챕터당 한 번 블록에서 호출된다.
static func on_chapter_advanced(chapter: int) -> void:
	if not is_active(ASH):
		return
	var kept: Dictionary = GameManager.player_data.get("oath_chapters_kept", {})
	kept[ASH] = int(kept.get(ASH, 0)) + 1
	GameManager.player_data["oath_chapters_kept"] = kept
	GameManager.player_data.grains = int(GameManager.player_data.get("grains", 0)) + CHAPTER_KEEP_GRAINS
	NotificationToast.show_toast(
		("잿불의 맹세 유지 — Grains +%d" % CHAPTER_KEEP_GRAINS) if GameManager.current_locale == "ko" else ("Oath of Ash kept — Grains +%d" % CHAPTER_KEEP_GRAINS),
		NotificationToast.ToastType.SUCCESS
	)


## 증인의 맹세 상수 보상. FieldFlow 이동 축적에 곱해진다.
static func witness_flow_multiplier() -> float:
	return WITNESS_FLOW_MULT if is_active(WITNESS) else 1.0


## 고요한 손 상수 보상. 침식 루프에서 엘리아 관련 기억 면역 판정에 쓰인다.
static func still_hands_shields(memory: MemoryManager.Memory) -> bool:
	return memory != null and memory.related_npc == "Elia" and is_active(STILL)


static func get_status() -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	for oath in OATHS:
		var id := String(oath.id)
		out.append({
			"id": id,
			"sworn": is_sworn(id),
			"broken": is_broken(id),
			"chapters_kept": int(GameManager.player_data.get("oath_chapters_kept", {}).get(id, 0)),
		})
	return out
