extends Node

## S248: 한 번도 계측된 적 없는 세 축을 잰다 — 난이도, NG+, 장비.
##
## S246~247이 기본 조건(보통 난이도, NG+0, 장비 없음)에서만 쟀다. 그런데
## 난이도는 적 HP와 공격력에 0.7/1.0/1.4를 곱하고, NG+는 회당 +30%를 얹으며,
## 최고 장비 조합은 공격 +20 / 방어 +15를 준다. 어느 것도 실제 효과가 확인된
## 적이 없다.
##
## 묻는 것:
##  - 난이도 세 종류가 실제로 다른 게임인가, 아니면 이름만 다른가.
##  - NG+가 회를 거듭할수록 넘을 수 없어지는가.
##  - 장비를 갖추는 것이 체감되는가, 아니면 장식인가.
##
## 전수 조합(18가지)은 돌리지 않는다. 축을 하나씩 쓸어 기준선과 비교한다.
## 판정 잣대는 S246과 같은 치명도(누적 피해 ÷ 최대 HP)다.

const RUNS := 12
const TURN_CAP := 60

## 범위를 대표하는 넷. 잡몹 / 중간 / 중반 보스 / 최종 보스.
const ENCOUNTERS: Array[Dictionary] = [
	{"preset": "ash_crawler", "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"preset": "void_beast", "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"preset": "shade_sentinel", "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"preset": "kairos", "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
]

## 상점에서 살 수 있는 최고 조합(NG++ 전용 제외). 공격 +20, 방어 +15.
const FULL_KIT := {"weapon": "void_edge", "armor": "memory_weave", "accessory": "iron_ring"}

var _turn_ready: bool = false
var _battle_over: bool = false

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "en"
	Engine.time_scale = 25.0
	BattleManager.player_turn_started.connect(func(): _turn_ready = true)
	BattleManager.battle_ended.connect(func(_r): _battle_over = true)
	await get_tree().process_frame

	await _sweep("difficulty", [
		{"label": "easy", "difficulty": 0}, {"label": "normal", "difficulty": 1}, {"label": "hard", "difficulty": 2}])
	await _sweep("ng_plus", [
		{"label": "ng0", "ng": 0}, {"label": "ng1", "ng": 1}, {"label": "ng2", "ng": 2}])
	await _sweep("equipment", [
		{"label": "bare", "kit": false}, {"label": "full_kit", "kit": true}])
	# NG+ 플레이어는 장비를 이어받는다(S34). 맨몸 NG+ 수치만 보고 판정하면
	# 실제로 존재하지 않는 상황을 재는 셈이다. 장비를 갖춘 NG+가 실제 조건이다.
	await _sweep("ng_equipped", [
		{"label": "ng0+kit", "ng": 0, "kit": true},
		{"label": "ng1+kit", "ng": 1, "kit": true},
		{"label": "ng2+kit", "ng": 2, "kit": true}])

	print("BALANCE_AXES_DONE")
	Engine.time_scale = 1.0
	get_tree().quit(0)

func _sweep(axis: String, conditions: Array) -> void:
	for condition in conditions:
		for encounter in ENCOUNTERS:
			var result := await _measure(encounter, condition)
			print("AXIS %-10s %-10s %-16s turns=%5.1f lethality=%.2f stalls=%d" % [
				axis, condition["label"], encounter["preset"],
				result["turns"], result["lethality"], result["stalls"]])

func _measure(encounter: Dictionary, condition: Dictionary) -> Dictionary:
	var turns_total := 0
	var damage_total := 0
	var stalls := 0
	var max_hp := 0
	for run in range(RUNS):
		var result := await _one_battle(encounter, condition)
		turns_total += int(result["turns"])
		damage_total += int(result["damage"])
		max_hp = int(result["max_hp"])
		if bool(result["stalled"]):
			stalls += 1
	var turns := float(turns_total) / float(RUNS)
	var damage := float(damage_total) / float(RUNS)
	return {"turns": turns, "lethality": damage / maxf(float(max_hp), 1.0), "stalls": stalls}

func _one_battle(encounter: Dictionary, condition: Dictionary) -> Dictionary:
	GameManager.current_chapter = int(encounter["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.max_hp = 100
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.call("_init_starting_memories")
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false

	# 축 조건. 지정되지 않은 축은 기본값으로 되돌려 서로 섞이지 않게 한다.
	OptionsMenu.settings["difficulty"] = int(condition.get("difficulty", 1))
	GameManager.ng_plus_cycle = int(condition.get("ng", 0))
	GameManager.equipped = {"weapon": "", "armor": "", "accessory": ""}
	if bool(condition.get("kit", false)):
		GameManager.equipped = FULL_KIT.duplicate()

	_turn_ready = false
	_battle_over = false
	BattleManager.start_battle(String(encounter["preset"]), String(encounter["scene"]))
	BattleManager.return_scene = ""
	BattleManager.select_tactical_objective(0, false)

	var max_hp: int = int(GameManager.player_data.max_hp)
	GameManager.player_data.hp = max_hp
	var turns := 0
	var damage := 0
	var stalled := false
	while turns < TURN_CAP and not _battle_over:
		if BattleManager.state != BattleManager.BattleState.PLAYER_TURN:
			if not await _wait_for_turn():
				stalled = not _battle_over
				break
			continue
		_turn_ready = false
		BattleManager.player_attack()
		turns += 1
		if not await _wait_for_turn():
			stalled = not _battle_over
			break
		var lost: int = max_hp - int(GameManager.player_data.hp)
		if lost > 0:
			damage += lost
			GameManager.player_data.hp = max_hp
	if turns >= TURN_CAP:
		stalled = true
	BattleManager.dismiss_victory()
	var settle := 0
	while BattleManager.current_enemy != null and settle < 400:
		await get_tree().process_frame
		settle += 1
	BattleManager.state = BattleManager.BattleState.IDLE
	return {"turns": turns, "damage": damage, "max_hp": max_hp, "stalled": stalled}

func _wait_for_turn() -> bool:
	var guard := 0
	while guard < 2000:
		if _battle_over:
			return false
		if _turn_ready:
			return true
		await get_tree().process_frame
		guard += 1
	return false
