## RandomEncounter, 랜덤 인카운터 유틸리티
## 맵 재방문 시 이동 기반 랜덤 전투 발생.
## 각 맵에서 setup() 호출 후 _process에서 update() 호출.
class_name RandomEncounter

## 인카운터 데이터
class EncounterData:
	var step_count: float = 0.0
	var threshold: float = 0.0  # 다음 인카운터까지 필요한 걸음 수
	var enemy_pool: Array = []  # [{name, hp, atk, is_void, abilities}]
	var map_scene: String = ""
	var bg_image: String = ""
	var enemy_image: String = ""
	var enabled: bool = true
	var warning_emitted: bool = false
	var trail_broken_feedback: bool = false
	var min_steps: int = 40     # 최소 걸음 수
	var max_steps: int = 80     # 최대 걸음 수
	var last_player_pos: Vector2 = Vector2.ZERO

## 인카운터 시스템 초기화
static func setup(enemy_pool: Array, map_scene: String, bg_img: String = "", e_img: String = "", min_steps: int = 40, max_steps: int = 80) -> EncounterData:
	var data = EncounterData.new()
	data.enemy_pool = enemy_pool
	data.map_scene = map_scene
	data.bg_image = bg_img
	data.enemy_image = e_img
	data.min_steps = min_steps
	data.max_steps = max_steps
	data.threshold = randf_range(min_steps, max_steps)
	return data

## 매 프레임 호출, 플레이어 이동 거리 기반 인카운터 체크
## 반환: true면 전투 발생됨
static func update(data: EncounterData, player_pos: Vector2, tile_size: int) -> bool:
	if not data.enabled:
		return false
	var player := _get_player()
	if GameManager.current_state != GameManager.GameState.EXPLORATION:
		_clear_pressure(player)
		data.last_player_pos = player_pos
		return false

	# 이동 거리 계산 (타일 단위)
	if data.last_player_pos == Vector2.ZERO:
		data.last_player_pos = player_pos
		_update_pressure(player, data)
		return false

	var distance = player_pos.distance_to(data.last_player_pos) / tile_size
	data.last_player_pos = player_pos

	if distance < 0.01:  # 정지 상태
		_update_pressure(player, data)
		return false

	data.step_count += distance
	if player and player.has_method("is_field_dashing") and bool(player.call("is_field_dashing")) and data.warning_emitted:
		data.step_count = maxf(0.0, data.step_count - distance * 4.4)
		if data.step_count < data.threshold * 0.54:
			data.warning_emitted = false
			if not data.trail_broken_feedback:
				data.trail_broken_feedback = true
				var broken_text := "위상 흔적이 끊겼다. 추적이 멀어진다." if GameManager.current_locale == "ko" else "The phase trail breaks. Pursuit falls away."
				NotificationToast.show_toast(broken_text, NotificationToast.ToastType.SUCCESS)
		_update_pressure(player, data)
		return false

	if not data.warning_emitted and data.step_count >= data.threshold * 0.72:
		data.warning_emitted = true
		data.trail_broken_feedback = false
		var warning := "기억 소음이 닫힌다 · Ctrl 위상 이동 / Q 증언 파동" if GameManager.current_locale == "ko" else "Memory noise closes in · Ctrl Phase Step / Q Witness Pulse"
		NotificationToast.show_toast(warning, NotificationToast.ToastType.WARNING)
	_update_pressure(player, data)

	if data.step_count >= data.threshold:
		data.step_count = 0.0
		data.threshold = randf_range(data.min_steps, data.max_steps)
		data.warning_emitted = false
		data.trail_broken_feedback = false
		_clear_pressure(player)
		_trigger_encounter(data)
		return true

	return false

## 랜덤 적 선택 + 전투 시작
static func _trigger_encounter(data: EncounterData) -> void:
	if data.enemy_pool.is_empty():
		return
	var player := _get_player()
	if player and player.has_method("prepare_field_entry_for_battle"):
		player.call("prepare_field_entry_for_battle", "ambient")

	var pool_entry: Dictionary = data.enemy_pool[randi_range(0, data.enemy_pool.size() - 1)]
	var enemy = BattleManager.Enemy.new(
		pool_entry["name"],
		pool_entry["hp"],
		pool_entry["atk"],
		pool_entry.get("is_void", false)
	)
	enemy.is_ambient_encounter = true
	if pool_entry.has("abilities"):
		enemy.abilities = pool_entry["abilities"]

	var bg = data.bg_image if data.bg_image != "" else pool_entry.get("bg", "")
	var ei = data.enemy_image if data.enemy_image != "" else pool_entry.get("img", "")

	BattleManager.start_battle(enemy, data.map_scene, bg, ei)
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")

static func _get_player() -> Node:
	var tree := GameManager.get_tree()
	if tree:
		return tree.get_first_node_in_group("player")
	return null

static func _update_pressure(player: Node, data: EncounterData) -> void:
	if player == null or not player.has_method("set_field_threat_source"):
		return
	var ratio := clampf(data.step_count / maxf(data.threshold, 0.001), 0.0, 1.0)
	var pressure := clampf((ratio - 0.42) / 0.58, 0.0, 1.0)
	player.call("set_field_threat_source", "random_encounter", pressure)

static func _clear_pressure(player: Node) -> void:
	if player and player.has_method("clear_field_threat_source"):
		player.call("clear_field_threat_source", "random_encounter")
