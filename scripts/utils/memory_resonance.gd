## MemoryResonance — 기억 공명 탐색 이벤트
## 맵 위 특정 위치에서 보유 기억이 공명. 현장 연소로 탐색 보너스 획득.
## 각 맵의 _setup_exploration_events()에서 호출.
class_name MemoryResonance
extends RefCounted

const TILE_SIZE: int = 32

# 맵별 공명 지점 데이터
# map_name → [{pos_x, pos_y, memory_id, bonus_type, bonus_value, bonus_desc, flag}]
const RESONANCE_POINTS: Dictionary = {
	"rim_forest": [
		{"pos_x": 5, "pos_y": 4, "memory_id": "sense_forest_smell", "bonus_type": "max_hp", "bonus_value": 10, "bonus_desc": "The earth remembers your footsteps. +10 Max HP.", "flag": "resonance_rim_smell"},
		{"pos_x": 18, "pos_y": 8, "memory_id": "daily_campfire_song", "bonus_type": "grains", "bonus_value": 25, "bonus_desc": "The melody dissolves into currency. +25 Grains.", "flag": "resonance_rim_song"},
	],
	"verdan_market": [
		{"pos_x": 12, "pos_y": 6, "memory_id": "daily_market_food", "bonus_type": "grains", "bonus_value": 30, "bonus_desc": "The vendor's ghost tips his hat. +30 Grains.", "flag": "resonance_verdan_food"},
		{"pos_x": 8, "pos_y": 14, "memory_id": "identity_first_sword", "bonus_type": "item", "bonus_value": "firebomb", "bonus_desc": "The courtyard echoes. Found a Firebomb.", "flag": "resonance_verdan_sword"},
	],
	"belt_waystation": [
		{"pos_x": 15, "pos_y": 5, "memory_id": "sense_dead_soil", "bonus_type": "encounter_reduce", "bonus_value": 50, "bonus_desc": "The dead earth accepts you. Encounters reduced.", "flag": "resonance_belt_soil"},
		{"pos_x": 6, "pos_y": 12, "memory_id": "rel_tobias_records", "bonus_type": "grains", "bonus_value": 20, "bonus_desc": "Ink stains fade into coins. +20 Grains.", "flag": "resonance_belt_tobias"},
	],
	"drift_shelter": [
		{"pos_x": 10, "pos_y": 8, "memory_id": "daily_elia_hands", "bonus_type": "max_hp", "bonus_value": 15, "bonus_desc": "Warmth remembered. Your body strengthens. +15 Max HP.", "flag": "resonance_drift_hands"},
	],
	"crumbling_coast": [
		{"pos_x": 14, "pos_y": 4, "memory_id": "sense_salt_wind", "bonus_type": "item", "bonus_value": "hi_potion", "bonus_desc": "Salt crystallizes into medicine. Found a Hi-Potion.", "flag": "resonance_coast_salt"},
		{"pos_x": 7, "pos_y": 12, "memory_id": "daily_elia_walking", "bonus_type": "grains", "bonus_value": 20, "bonus_desc": "Footsteps dissolve into currency. +20 Grains.", "flag": "resonance_coast_walk"},
	],
	"the_seam": [
		{"pos_x": 6, "pos_y": 3, "memory_id": "daily_garden_flowers", "bonus_type": "max_hp", "bonus_value": 10, "bonus_desc": "Petals become strength. +10 Max HP.", "flag": "resonance_seam_flowers"},
		{"pos_x": 16, "pos_y": 10, "memory_id": "rel_sable_trust", "bonus_type": "item", "bonus_value": "smoke_bomb", "bonus_desc": "Trust dissolves into shadows. Found a Smoke Bomb.", "flag": "resonance_seam_trust"},
	],
	"seam_outskirts": [
		{"pos_x": 12, "pos_y": 8, "memory_id": "rel_echo_shell", "bonus_type": "grains", "bonus_value": 35, "bonus_desc": "Echoes crystallize. +35 Grains.", "flag": "resonance_outskirts_shell"},
	],
	"forgotten_forest": [
		{"pos_x": 8, "pos_y": 6, "memory_id": "sense_hollow_trees", "bonus_type": "encounter_reduce", "bonus_value": 50, "bonus_desc": "The forest recognizes you. Encounters reduced.", "flag": "resonance_forest_trees"},
		{"pos_x": 16, "pos_y": 14, "memory_id": "rel_ghost_words", "bonus_type": "max_hp", "bonus_value": 12, "bonus_desc": "A ghost's sentence finishes inside you. +12 Max HP.", "flag": "resonance_forest_ghost"},
	],
	"colorless_waste": [
		{"pos_x": 10, "pos_y": 6, "memory_id": "sense_no_color", "bonus_type": "grains", "bonus_value": 40, "bonus_desc": "Absence turns to currency. +40 Grains.", "flag": "resonance_waste_color"},
		{"pos_x": 6, "pos_y": 12, "memory_id": "identity_compass", "bonus_type": "max_hp", "bonus_value": 20, "bonus_desc": "Direction becomes constitution. +20 Max HP.", "flag": "resonance_waste_compass"},
	],
	"bl07_void": [
		{"pos_x": 12, "pos_y": 8, "memory_id": "identity_void_walker", "bonus_type": "max_hp", "bonus_value": 25, "bonus_desc": "What you saw strengthens you. +25 Max HP.", "flag": "resonance_bl07_void"},
	],
}

## 맵에 공명 지점 설치
## map_node: 현재 맵 Node2D, map_name: RESONANCE_POINTS 키
static func setup_points(map_node: Node2D, map_name: String) -> void:
	var points = RESONANCE_POINTS.get(map_name, [])
	for point in points:
		# 이미 사용한 공명점은 스킵
		if GameManager.get_flag(point["flag"]):
			continue
		# 해당 기억을 아직 보유 중인지 체크
		var memory = MemoryManager._get_memory(point["memory_id"])
		if memory == null or memory.is_burned:
			continue

		var pos = Vector2(point["pos_x"] * TILE_SIZE, point["pos_y"] * TILE_SIZE)
		_create_resonance_trigger(map_node, pos, point)

static func _create_resonance_trigger(map_node: Node2D, pos: Vector2, point: Dictionary) -> void:
	var area = Area2D.new()
	area.position = pos + Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	area.collision_layer = 0
	area.collision_mask = 2

	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(TILE_SIZE * 1.5, TILE_SIZE * 1.5)
	shape.shape = rect
	area.add_child(shape)

	# 시각 효과 — 맥동하는 빛
	var glow = ColorRect.new()
	glow.size = Vector2(TILE_SIZE, TILE_SIZE)
	glow.position = -Vector2(TILE_SIZE / 2.0, TILE_SIZE / 2.0)
	glow.color = Color(0.7, 0.55, 0.3, 0.15)
	glow.z_index = -1
	area.add_child(glow)

	# 펄스 애니메이션
	var tween = map_node.create_tween().set_loops()
	tween.tween_property(glow, "color:a", 0.35, 1.5).set_trans(Tween.TRANS_SINE)
	tween.tween_property(glow, "color:a", 0.1, 1.5).set_trans(Tween.TRANS_SINE)

	var flag = point["flag"]
	var memory_id = point["memory_id"]
	var bonus_desc = point["bonus_desc"]
	var bonus_type = point["bonus_type"]
	var bonus_value = point["bonus_value"]

	area.body_entered.connect(func(body):
		if body.name != "Player" or GameManager.current_state != GameManager.GameState.EXPLORATION:
			return
		if GameManager.get_flag(flag):
			return
		# 기억이 아직 미연소인지 재확인
		var mem = MemoryManager._get_memory(memory_id)
		if mem == null or mem.is_burned:
			return
		# 공명 발동 — 선택지 대화
		GameManager.set_flag(flag)
		glow.queue_free()
		tween.kill()
		_trigger_resonance_choice(mem, bonus_type, bonus_value, bonus_desc)
	)
	map_node.add_child(area)

static func _trigger_resonance_choice(memory: MemoryManager.Memory, bonus_type: String, bonus_value, bonus_desc: String) -> void:
	# 공명 알림
	NotificationToast.show_toast("Memory Resonance: %s" % memory.title, NotificationToast.ToastType.INFO)

	# 비전투 연소 → 보너스 적용 (선택지 대신 자동 적용으로 간소화)
	# 실제로는 DialogueManager 선택지 활용이 이상적이나, 현재 자동 적용으로 구현
	var burned = MemoryManager.burn_memory_silent(memory.id)
	if burned == null:
		return

	# 보너스 적용
	match bonus_type:
		"max_hp":
			GameManager.player_data.max_hp += bonus_value
			GameManager.player_data.hp = mini(GameManager.player_data.hp + bonus_value, GameManager.player_data.max_hp)
		"grains":
			GameManager.player_data.grains += bonus_value
		"item":
			GameManager.add_item(str(bonus_value), 1)
		"encounter_reduce":
			# 플래그로 저장 — RandomEncounter에서 체크
			GameManager.set_flag("resonance_encounter_reduce")

	NotificationToast.show_toast(bonus_desc, NotificationToast.ToastType.SUCCESS)
	print("[MemoryResonance] Burned '%s' for: %s" % [memory.title, bonus_desc])
