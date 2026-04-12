## EliaDiary (Autoload) — 엘리아의 일지
## 기억 연소 시 엘리아의 반응을 기록. 특정 기억 연소 시 전투 기술 해금.
## MemoryUI에서 Diary 탭으로 접근.
extends Node

# --- 일지 엔트리 데이터 ---
# memory_id → {text, unlock_skill, read}
# unlock_skill이 빈 문자열이면 기술 해금 없음
var entries: Dictionary = {}

# --- 엘리아 기술 (전투용, 기억 연소 없이 사용 가능) ---
# skill_id → {name, desc, cooldown_max, current_cooldown, effect}
var skills: Dictionary = {}

# --- 엔트리 정의 ---
const DIARY_ENTRIES: Dictionary = {
	"sense_forest_smell": {
		"text": "He burned the smell of rain on earth today. I watched his face — he didn't even flinch. I wonder if he knows what he just lost. The forest after rain. That quiet, breathing moment. Gone.\n\nI still remember it. I'll hold it for him.",
		"unlock_skill": "",
	},
	"daily_campfire_song": {
		"text": "The campfire song. He burned it without hesitating. I hummed it last night and he didn't react at all. Not a flinch, not a twitch.\n\nHe used to close his eyes when I sang it.\n\nI'll keep humming. Even if it means nothing to him now.",
		"unlock_skill": "humming_shield",
	},
	"daily_market_food": {
		"text": "The taste of street food in Verdan. He walked past a vendor today and didn't even slow down. He used to crane his neck at every stall.\n\nThe smaller memories go first, Tobias says. But they're the ones that make a person feel like a person.",
		"unlock_skill": "",
	},
	"rel_hand_reaching": {
		"text": "His hand stopped reaching for me today. Just stopped. Mid-gesture, his arm went slack, and he looked confused — like he couldn't remember why his hand was there.\n\nI caught it. I held it. He let me.\n\nBut next time, will there be a hand to catch?",
		"unlock_skill": "desperate_reach",
	},
	"identity_first_sword": {
		"text": "The sword memory. Malet's price. Whether he sold it or burned it himself — either way, his stance changed. Subtly. The grip shifted, the footwork simplified.\n\nHe fights now like someone who learned fighting from a manual, not from a hand guiding his.",
		"unlock_skill": "remembered_strike",
	},
	"rel_sable_trust": {
		"text": "He burned his trust in Sable. I saw it happen — one moment he was listening to her, the next he was just... hearing words. No weight behind them.\n\nSable noticed. She didn't say anything. She's used to being forgotten.",
		"unlock_skill": "",
	},
	"daily_elia_hands": {
		"text": "He burned the anchoring memory. My hands on his. The warmth.\n\nI can still anchor him — the technique doesn't need his memory of it. But it's harder now. Like pressing a page into a book that's forgotten it was a book.\n\nI'll press harder.",
		"unlock_skill": "anchor_pulse",
	},
	"identity_compass": {
		"text": "The compass is gone. The pull that guided him through the Waste — he burned it for power. Now we navigate by guess and prayer.\n\nBut there's something strange. His body still turns toward BL-07. The muscle memory remains even when the memory doesn't.\n\nIs that hope? Or habit?",
		"unlock_skill": "",
	},
}

# --- 기술 정의 ---
const SKILL_DEFS: Dictionary = {
	"humming_shield": {
		"name": "Humming Shield",
		"desc": "Elia hums a half-remembered melody. Damage halved for 1 turn.",
		"cooldown_max": 3,
		"effect": "defend",
		"power": 50,  # 50% 데미지 감소
	},
	"desperate_reach": {
		"name": "Desperate Reach",
		"desc": "A gesture older than memory. Stuns the enemy for 1 turn.",
		"cooldown_max": 4,
		"effect": "stun_enemy",
		"power": 1,
	},
	"remembered_strike": {
		"name": "Remembered Strike",
		"desc": "A strike from a courtyard that no longer exists. Damage scales with total burned memories.",
		"cooldown_max": 3,
		"effect": "damage",
		"power": 0,  # 동적 계산
	},
	"anchor_pulse": {
		"name": "Anchor Pulse",
		"desc": "Elia stabilizes the architecture. Heal 15% max HP + cure status effects.",
		"cooldown_max": 4,
		"effect": "heal_cure",
		"power": 15,
	},
}

signal diary_entry_added(memory_id: String)
signal skill_unlocked(skill_id: String, skill_name: String)

func _ready() -> void:
	# 기억 연소 시그널 연결
	MemoryManager.memory_burned.connect(_on_memory_burned)
	print("[EliaDiary] Ready — %d potential entries" % DIARY_ENTRIES.size())

func _on_memory_burned(memory: MemoryManager.Memory) -> void:
	# 엘리아가 파티에 없으면 기록 안 함
	if not GameManager.player_data.elia_with_party:
		return
	if memory.id in DIARY_ENTRIES and memory.id not in entries:
		var def = DIARY_ENTRIES[memory.id]
		entries[memory.id] = {"text": def["text"], "read": false}
		diary_entry_added.emit(memory.id)
		NotificationToast.show_toast("Elia wrote in her diary...", NotificationToast.ToastType.INFO)
		# 기술 해금
		if def["unlock_skill"] != "" and def["unlock_skill"] not in skills:
			var skill_id = def["unlock_skill"]
			var skill_def = SKILL_DEFS.get(skill_id, {})
			if not skill_def.is_empty():
				skills[skill_id] = {
					"name": skill_def["name"],
					"desc": skill_def["desc"],
					"cooldown_max": skill_def["cooldown_max"],
					"current_cooldown": 0,
					"effect": skill_def["effect"],
					"power": skill_def["power"],
				}
				skill_unlocked.emit(skill_id, skill_def["name"])
				NotificationToast.show_toast("Elia Technique unlocked: %s" % skill_def["name"], NotificationToast.ToastType.SUCCESS)
				print("[EliaDiary] Skill unlocked: %s" % skill_def["name"])

## 전투에서 엘리아 기술 사용
func use_skill(skill_id: String) -> Dictionary:
	if skill_id not in skills:
		return {"success": false, "msg": "Skill not available."}
	var skill = skills[skill_id]
	if skill["current_cooldown"] > 0:
		return {"success": false, "msg": "%s is on cooldown (%d turns)." % [skill["name"], skill["current_cooldown"]]}
	if not GameManager.player_data.elia_with_party:
		return {"success": false, "msg": "Elia is not with you."}

	# 쿨다운 시작
	skill["current_cooldown"] = skill["cooldown_max"]

	var result = {"success": true, "msg": "", "effect": skill["effect"], "power": skill["power"], "name": skill["name"]}

	match skill["effect"]:
		"defend":
			result["msg"] = "Elia hums softly. The melody shields you."
		"stun_enemy":
			result["msg"] = "A hand reaches through absence — the enemy freezes."
		"damage":
			# 데미지 = 10 + (연소 수 * 8)
			var burn_dmg = 10 + MemoryManager.get_burn_count() * 8
			result["power"] = burn_dmg
			result["msg"] = "A remembered blade from a forgotten courtyard. %d damage." % burn_dmg
		"heal_cure":
			var heal = int(GameManager.player_data.max_hp * skill["power"] / 100.0)
			result["power"] = heal
			result["msg"] = "Elia stabilizes the architecture. +%d HP." % heal

	return result

## 전투 턴 종료 시 쿨다운 감소
func tick_cooldowns() -> void:
	for skill_id in skills:
		if skills[skill_id]["current_cooldown"] > 0:
			skills[skill_id]["current_cooldown"] -= 1

## 전투 시작 시 쿨다운 리셋
func reset_cooldowns() -> void:
	for skill_id in skills:
		skills[skill_id]["current_cooldown"] = 0

## 사용 가능한 기술 목록
func get_available_skills() -> Array:
	var available: Array = []
	for skill_id in skills:
		var skill = skills[skill_id]
		available.append({
			"id": skill_id,
			"name": skill["name"],
			"desc": skill["desc"],
			"cooldown": skill["current_cooldown"],
			"ready": skill["current_cooldown"] <= 0,
		})
	return available

## 일지 엔트리 목록 (UI 표시용)
func get_entries() -> Array:
	var result: Array = []
	for memory_id in entries:
		var entry = entries[memory_id]
		var mem_title = ""
		for m in MemoryManager.memories:
			if m.id == memory_id:
				mem_title = m.title
				break
		if mem_title == "":
			mem_title = memory_id.replace("_", " ").capitalize()
		result.append({
			"memory_id": memory_id,
			"memory_title": mem_title,
			"text": entry["text"],
			"read": entry["read"],
		})
	return result

func mark_read(memory_id: String) -> void:
	if memory_id in entries:
		entries[memory_id]["read"] = true

## 세이브/로드
func export_data() -> Dictionary:
	var data = {"entries": {}, "skills": {}}
	for key in entries:
		data.entries[key] = {"read": entries[key]["read"]}
	for key in skills:
		data.skills[key] = {"current_cooldown": skills[key]["current_cooldown"]}
	return data

func import_data(data: Dictionary) -> void:
	if data.has("entries"):
		for key in data.entries:
			if key in DIARY_ENTRIES:
				entries[key] = {"text": DIARY_ENTRIES[key]["text"], "read": data.entries[key].get("read", false)}
				# 기술도 복원
				var skill_id = DIARY_ENTRIES[key].get("unlock_skill", "")
				if skill_id != "" and skill_id in SKILL_DEFS and skill_id not in skills:
					var sd = SKILL_DEFS[skill_id]
					skills[skill_id] = {
						"name": sd["name"], "desc": sd["desc"],
						"cooldown_max": sd["cooldown_max"],
						"current_cooldown": data.get("skills", {}).get(skill_id, {}).get("current_cooldown", 0),
						"effect": sd["effect"], "power": sd["power"],
					}
	print("[EliaDiary] Imported — %d entries, %d skills" % [entries.size(), skills.size()])
