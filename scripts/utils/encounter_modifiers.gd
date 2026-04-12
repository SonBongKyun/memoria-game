## EncounterModifier — 보이드 부패 인카운터 수정자
## 기억 연소 횟수에 따라 랜덤 전투에 수정자 부여.
## 많이 태울수록 세계가 반응 — 전투가 더 예측 불가해짐.
class_name EncounterModifier
extends RefCounted

# 수정자 풀 (burn_count 범위별)
const MODIFIERS: Dictionary = {
	# 0-2번 연소: 수정자 없음
	"early": [],
	# 3-5번 연소: 가벼운 수정자
	"mid": [
		{"id": "memory_fog", "name": "Memory Fog", "desc": "The fog of burned memories clouds your aim.", "effect": "player_miss", "value": 20},
		{"id": "residue_storm", "name": "Residue Storm", "desc": "Residue burns surge — echoes amplified.", "effect": "echo_boost", "value": 50},
		{"id": "void_whisper", "name": "Void Whisper", "desc": "The void notices you. Enemy attacks have a sting.", "effect": "enemy_atk_up", "value": 15},
	],
	# 6-8번 연소: 중간 수정자
	"high": [
		{"id": "void_surge", "name": "Void Surge", "desc": "Dark energy surges — enemies gain new abilities.", "effect": "enemy_ability_add", "value": 1},
		{"id": "fractured_time", "name": "Fractured Time", "desc": "Time fractures. The enemy acts twice every 3rd round.", "effect": "enemy_double_turn", "value": 3},
		{"id": "burning_ground", "name": "Burning Ground", "desc": "The ground smolders with memory residue.", "effect": "dot_both", "value": 5},
		{"id": "memory_weight", "name": "Memory Weight", "desc": "Unburned memories drag you down.", "effect": "player_def_down", "value": 20},
	],
	# 9+번 연소: 위험 수정자
	"extreme": [
		{"id": "reality_tear", "name": "Reality Tear", "desc": "Reality fractures — the enemy becomes a Void Beast.", "effect": "void_convert", "value": 0},
		{"id": "echo_overload", "name": "Echo Overload", "desc": "Echoes cascade — memory burns trigger double echoes.", "effect": "double_echo", "value": 0},
		{"id": "the_watcher", "name": "The Watcher", "desc": "The Bureau observes. End the fight in 6 turns.", "effect": "turn_limit", "value": 6},
		{"id": "void_hunger", "name": "Void Hunger", "desc": "BL-07 reaches for you. Lose HP each turn.", "effect": "player_dot", "value": 8},
	],
}

# 추가 능력 풀 (void_surge에서 적에게 부여)
const EXTRA_ABILITIES: Array = ["stun", "reflect", "charge", "poison", "drain", "shield"]

## 인카운터에 수정자 적용
## 반환: 적용된 수정자 정보 (빈 딕셔너리 = 수정자 없음)
static func apply(enemy: BattleManager.Enemy) -> Dictionary:
	var burn_count = MemoryManager.get_burn_count()

	# 보스전에는 수정자 미적용
	if enemy.is_boss:
		return {}

	# 연소 수에 따른 풀 선택
	var pool: Array
	if burn_count <= 2:
		return {}
	elif burn_count <= 5:
		pool = MODIFIERS["mid"]
		# 40% 확률
		if randf() > 0.4:
			return {}
	elif burn_count <= 8:
		pool = MODIFIERS["mid"] + MODIFIERS["high"]
		# 60% 확률
		if randf() > 0.6:
			return {}
	else:
		pool = MODIFIERS["high"] + MODIFIERS["extreme"]
		# 80% 확률
		if randf() > 0.8:
			return {}

	if pool.is_empty():
		return {}

	var modifier = pool[randi_range(0, pool.size() - 1)]

	# 수정자 효과 적용
	match modifier["effect"]:
		"player_miss":
			pass  # BattleManager에서 체크
		"echo_boost":
			pass  # BattleManager에서 체크
		"enemy_atk_up":
			enemy.attack += modifier["value"]
		"enemy_ability_add":
			var available = EXTRA_ABILITIES.filter(func(a): return a not in enemy.abilities)
			if not available.is_empty():
				var new_ability = available[randi_range(0, available.size() - 1)]
				enemy.abilities.append(new_ability)
		"enemy_double_turn":
			pass  # BattleManager에서 턴 카운팅
		"dot_both":
			pass  # BattleManager에서 턴마다 처리
		"player_def_down":
			pass  # BattleManager에서 체크
		"void_convert":
			if not enemy.is_void_beast:
				enemy.is_void_beast = true
				enemy.weakness = "void"
				enemy.name = "Void " + enemy.name
		"double_echo":
			pass  # BattleManager에서 에코 생성 시 2배
		"turn_limit":
			pass  # BattleManager에서 턴 카운팅
		"player_dot":
			pass  # BattleManager에서 턴마다 처리

	return modifier
