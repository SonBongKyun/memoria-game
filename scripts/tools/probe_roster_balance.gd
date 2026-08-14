extends Node

## S247: 프리셋 밖 로스터 전체를 잰다.
##
## S246은 ENEMY_PRESETS 6종만 재고 그 위에서 다이얼 세 개를 돌렸다. 그런데
## 맵 스크립트가 인라인으로 정의하는 적이 **28종**이고, 플레이어가 실제로 가장
## 많이 싸우는 것은 그쪽이다. 내가 방금 바꾼 값들이 검증 범위 밖에서 어떻게
## 작동하는지 모르는 상태였다.
##
## 로스터는 손으로 옮겨 적지 않았다. 맵 스크립트에서 뽑아 넣었다.
##
## 판정 기준은 S246과 같다. 치명도 = 누적 피해 ÷ 최대 HP. 1.0을 넘으면 공격만으로는
## 회복 없이 못 버틴다. 잡몹이 그 선을 넘으면 설계가 어긋난 것이다.

const RUNS := 12
const TURN_CAP := 60

const ROSTER: Array[Dictionary] = [
	{"enemy": {"name": "Belt Scavenger", "hp": 55, "atk": 12, "is_void": false, "abilities": ["weaken"]}, "chapter": 3, "scene": "res://scenes/maps/belt_waystation.tscn"},
	{"enemy": {"name": "Void Wisp", "hp": 45, "atk": 14, "is_void": true, "abilities": ["drain"]}, "chapter": 3, "scene": "res://scenes/maps/belt_waystation.tscn"},
	{"enemy": {"name": "Dust Crawler", "hp": 40, "atk": 10, "is_void": false, "abilities": ["poison"]}, "chapter": 3, "scene": "res://scenes/maps/belt_waystation.tscn"},
	{"enemy": {"name": "Void Fragment", "hp": 75, "atk": 16, "is_void": true, "abilities": ["burn_attack"]}, "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
	{"enemy": {"name": "Memory Eater", "hp": 95, "atk": 20, "is_void": true, "abilities": ["drain", "multi_hit", "weaken"]}, "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
	{"enemy": {"name": "Null Wisp", "hp": 60, "atk": 22, "is_void": true, "abilities": ["poison", "burn_attack"]}, "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
	{"enemy": {"name": "Void Fragment", "hp": 110, "atk": 26, "is_void": true, "abilities": ["drain", "shield", "reflect"]}, "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
	{"enemy": {"name": "Colorless Wraith", "hp": 100, "atk": 24, "is_void": true, "abilities": ["drain", "stun"]}, "chapter": 9, "scene": "res://scenes/maps/colorless_waste.tscn"},
	{"enemy": {"name": "Hollow Walker", "hp": 95, "atk": 22, "is_void": false, "abilities": ["stun", "charge"]}, "chapter": 9, "scene": "res://scenes/maps/colorless_waste.tscn"},
	{"enemy": {"name": "Coastal Void Beast", "hp": 100, "atk": 18, "is_void": true, "abilities": ["drain"]}, "chapter": 5, "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"enemy": {"name": "Shore Wraith", "hp": 85, "atk": 14, "is_void": true, "abilities": ["burn_attack", "weaken"]}, "chapter": 5, "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"enemy": {"name": "Cliff Stalker", "hp": 70, "atk": 16, "is_void": false, "abilities": ["poison", "multi_hit"]}, "chapter": 5, "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"enemy": {"name": "Memory Leech", "hp": 50, "atk": 13, "is_void": true, "abilities": ["drain"]}, "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"enemy": {"name": "Ash Walker", "hp": 60, "atk": 11, "is_void": false, "abilities": ["weaken", "burn_attack"]}, "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"enemy": {"name": "Rubble Rat", "hp": 35, "atk": 9, "is_void": false, "abilities": ["poison"]}, "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"enemy": {"name": "Root Shade", "hp": 70, "atk": 16, "is_void": true, "abilities": ["reflect", "weaken"]}, "chapter": 8, "scene": "res://scenes/maps/forgotten_forest.tscn"},
	{"enemy": {"name": "Memory Leech", "hp": 80, "atk": 18, "is_void": true, "abilities": ["drain", "poison"]}, "chapter": 8, "scene": "res://scenes/maps/forgotten_forest.tscn"},
	{"enemy": {"name": "Ash Phantom", "hp": 75, "atk": 18, "is_void": false, "abilities": ["stun", "weaken"]}, "chapter": 8, "scene": "res://scenes/maps/forgotten_forest.tscn"},
	{"enemy": {"name": "Ash Crawler", "hp": 45, "atk": 10, "is_void": false, "abilities": []}, "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"enemy": {"name": "Forest Shade", "hp": 55, "atk": 12, "is_void": false, "abilities": ["poison"]}, "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"enemy": {"name": "Void Wisp", "hp": 50, "atk": 12, "is_void": true, "abilities": ["drain"]}, "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"enemy": {"name": "Threshold Crawler", "hp": 65, "atk": 22, "is_void": true, "abilities": ["charge", "drain"]}, "chapter": 7, "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"enemy": {"name": "Depth Crawler", "hp": 85, "atk": 20, "is_void": false, "abilities": ["charge", "reflect"]}, "chapter": 7, "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"enemy": {"name": "Void Sentinel", "hp": 90, "atk": 20, "is_void": true, "abilities": ["drain", "shield"]}, "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"enemy": {"name": "Void Wraith", "hp": 90, "atk": 18, "is_void": true, "abilities": ["drain", "weaken"]}, "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"enemy": {"name": "Seam Lurker", "hp": 110, "atk": 20, "is_void": true, "abilities": ["poison", "shield"]}, "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"enemy": {"name": "Market Thief", "hp": 50, "atk": 12, "is_void": false, "abilities": ["weaken"]}, "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
	{"enemy": {"name": "Alley Rat", "hp": 35, "atk": 8, "is_void": false, "abilities": ["poison"]}, "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
]

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

	var over_line := 0
	for row in ROSTER:
		var lethality := await _measure(row)
		if lethality >= 1.0:
			over_line += 1
	print("ROSTER_SUMMARY enemies=%d over_lethal=%d" % [ROSTER.size(), over_line])
	print("ROSTER_BALANCE_DONE")
	Engine.time_scale = 1.0
	get_tree().quit(0)

func _measure(row: Dictionary) -> float:
	var turns_total := 0
	var damage_total := 0
	var stalls := 0
	var max_hp := 0
	for run in range(RUNS):
		var result := await _one_battle(row)
		turns_total += int(result["turns"])
		damage_total += int(result["damage"])
		max_hp = int(result["max_hp"])
		if bool(result["stalled"]):
			stalls += 1
	var turns := float(turns_total) / float(RUNS)
	var damage := float(damage_total) / float(RUNS)
	var lethality := damage / maxf(float(max_hp), 1.0)
	var enemy: Dictionary = row["enemy"]
	print("ROSTER %-20s ch%-2d hp=%3d atk=%2d turns=%5.1f lethality=%.2f stalls=%d" % [
		enemy["name"], int(row["chapter"]), int(enemy["hp"]), int(enemy["atk"]), turns, lethality, stalls])
	return lethality

func _one_battle(row: Dictionary) -> Dictionary:
	GameManager.current_chapter = int(row["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false
	GameManager.player_data.max_hp = 100
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.call("_init_starting_memories")
	_turn_ready = false
	_battle_over = false
	BattleManager.start_battle(row["enemy"], String(row["scene"]))
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
