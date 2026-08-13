extends Node

## S246: 모든 정규 교전을 같은 조건으로 시뮬레이션해 나란히 놓는다.
##
## 화면에는 이 방식이 다섯 번 값을 했다(맵·전투 화면·오버레이·VN·엔딩). 전투
## **밸런스**는 한 번도 계측된 적이 없다. S17이 챕터별 HP/ATK 성장을 손으로
## 정한 것이 마지막이고, 그 뒤로 스탠스·콤보·속성·브레이크·수정자가 얹혔다.
##
## 묻는 것은 하나다. **기억을 태우지 않고 이길 수 있는가.**
## 이 게임의 중심 긴장이 거기 있다. 공격만으로 전부 쉽게 이긴다면 연소는 있으나
## 마나 마찬가지고, 전부 진다면 연소는 선택이 아니라 의무다. 어느 쪽도 설계
## 의도가 아니므로 실제 수치를 봐야 한다.
##
## 계산식을 다시 구현하지 않고 실제 BattleManager를 헤드리스로 돌린다.
##
## HP를 0까지 떨어뜨리지 않는다. 패배 경로가 game_over 씬으로 전환해 시뮬레이터를
## 죽이기 때문이다. 대신 라운드마다 받은 피해를 누적하고 HP를 되돌린다. 누적
## 피해를 최대 HP로 나눈 값이 "이 빌드로 몇 번 죽을 만큼 맞는가"가 된다.
##
## 계측기를 두 번 버렸다. 둘 다 상태(state)를 폴링했는데, 가속 상태에서는 적 턴
## 하나가 프레임 경계 사이에 통째로 끝나 PLAYER_TURN → ENEMY_TURN → PLAYER_TURN
## 전이가 관측되지 않는다. 첫 판에서는 void_beast가 20회 중 10회 "멈춤"으로
## 잡혔고, 두 번째 판에서는 ash_crawler의 받은 피해가 0으로 나왔다. 둘 다 게임이
## 아니라 계측기 문제였다. 지금은 BattleManager가 직접 쏘는 신호에 연결한다.

const RUNS := 20
const TURN_CAP := 60

const ENCOUNTERS: Array[Dictionary] = [
	{"preset": "ash_crawler", "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"preset": "forest_shade", "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
	{"preset": "void_beast", "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"preset": "threshold_shade", "chapter": 7, "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"preset": "shade_sentinel", "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"preset": "kairos", "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
]

var _turn_ready: bool = false
var _battle_over: bool = false
var _burn_total: int = 0

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "en"
	Engine.time_scale = 25.0
	BattleManager.player_turn_started.connect(func(): _turn_ready = true)
	BattleManager.battle_ended.connect(func(_result): _battle_over = true)
	await get_tree().process_frame

	print("BALANCE_HEADER policy preset chapter turns dmg_taken max_hp lethality stalls")
	for policy in ["attack", "burn"]:
		for encounter in ENCOUNTERS:
			await _measure(encounter, policy)
	print("BATTLE_BALANCE_DONE")
	Engine.time_scale = 1.0
	get_tree().quit(0)

func _measure(encounter: Dictionary, policy: String) -> void:
	var turns_total := 0
	var damage_total := 0
	var stalls := 0
	var max_hp := 0
	for run in range(RUNS):
		var result := await _one_battle(encounter, policy)
		turns_total += int(result["turns"])
		damage_total += int(result["damage"])
		max_hp = int(result["max_hp"])
		if bool(result["stalled"]):
			stalls += 1
			print("BALANCE_STALL %-6s %-16s run=%d turns=%d" % [policy, encounter["preset"], run, int(result["turns"])])

	var turns := float(turns_total) / float(RUNS)
	var damage := float(damage_total) / float(RUNS)
	# 받은 피해가 최대 HP의 몇 배인가. 1.0을 넘으면 회복 없이는 못 버틴다.
	var lethality := damage / maxf(float(max_hp), 1.0)
	print("BALANCE %-6s %-16s ch%-2d turns=%5.1f dmg=%6.1f max_hp=%3d lethality=%.2f stalls=%d burns=%.1f" % [
		policy, encounter["preset"], int(encounter["chapter"]), turns, damage, max_hp, lethality, stalls,
		float(_burn_total) / float(RUNS)])
	_burn_total = 0

func _one_battle(encounter: Dictionary, policy: String) -> Dictionary:
	GameManager.current_chapter = int(encounter["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	# 판마다 초기 상태로 되돌린다. 두 가지가 이전 판을 물고 넘어갔었다.
	# 하나는 최대 HP다. start_battle은 챕터 성장치보다 낮을 때만 올리고 절대
	# 내리지 않으므로, 10장을 돌고 나면 1장 시뮬레이션도 235로 시작한다.
	# 다른 하나는 기억이다. 연소는 영구 소모라 첫 교전에서 다 태우고 나면
	# 이후 모든 판이 "태울 게 없어 그냥 공격"이 된다(burns=0.0).
	GameManager.player_data.max_hp = 100
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.call("_init_starting_memories")
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false
	_turn_ready = false
	_battle_over = false
	BattleManager.start_battle(String(encounter["preset"]), String(encounter["scene"]))
	# 씬 전환을 막는다. 환경 보너스는 start_battle이 이미 경로에서 읽어 갔다.
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
		# 연소 정책: 태울 기억이 있으면 태우고, 없으면 벤다. 이 게임의 중심 선택을
		# 그대로 흉내 낸다. 등급이 높은 것부터 쓴다.
		var burned := false
		if policy == "burn":
			var memory_id := _cheapest_memory()
			if memory_id != "":
				BattleManager.player_burn(memory_id)
				_burn_total += 1
				burned = true
		if not burned:
			BattleManager.player_attack()
		turns += 1
		# 다음 플레이어 턴이 열릴 때까지 기다린다. 그 시점이면 적은 이미 행동했다.
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

## 지금 태울 수 있는 기억 중 **가장 값싼** 것. 없으면 빈 문자열.
##
## 처음에는 등급이 가장 높은 것을 골랐는데, 그러자 카이로스(450 HP)까지 전부
## 1턴에 죽었다. 열거형이 {GRADE_5, GRADE_4, GRADE_3, GRADE_2, GRADE_1}이라
## 등급 값이 가장 큰 것은 GRADE_1, 곧 연소력 999의 정체성 기억이었다. 그건
## 게임 전체에 한 번뿐인 최후의 수단(엔딩 "이름을 태우다")이지 매 턴 쓰는
## 자원이 아니다. 계산식은 정상이었고 정책이 틀렸다.
##
## 실제 플레이어는 값싼 것부터 쓴다. 감각 잔편(GRADE_5)이 가장 싸다.
func _cheapest_memory() -> String:
	var best_id := ""
	var best_grade := 99
	for memory in MemoryManager.memories:
		if memory.is_burned or memory.is_faded:
			continue
		if memory.grade < best_grade:
			best_grade = memory.grade
			best_id = memory.id
	return best_id

## 다음 플레이어 턴 신호나 전투 종료를 기다린다. 참이면 계속 진행할 수 있다.
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
