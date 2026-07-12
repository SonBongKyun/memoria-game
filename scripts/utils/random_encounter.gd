## RandomEncounter — 랜덤 인카운터 유틸리티
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

## 매 프레임 호출 — 플레이어 이동 거리 기반 인카운터 체크
## 반환: true면 전투 발생됨
static func update(data: EncounterData, player_pos: Vector2, tile_size: int) -> bool:
	if not data.enabled:
		return false
	if GameManager.current_state != GameManager.GameState.EXPLORATION:
		data.last_player_pos = player_pos
		return false

	# 이동 거리 계산 (타일 단위)
	if data.last_player_pos == Vector2.ZERO:
		data.last_player_pos = player_pos
		return false

	var distance = player_pos.distance_to(data.last_player_pos) / tile_size
	data.last_player_pos = player_pos

	if distance < 0.01:  # 정지 상태
		return false

	data.step_count += distance
	if not data.warning_emitted and data.step_count >= data.threshold * 0.72:
		data.warning_emitted = true
		var warning := "기억의 소음이 가까워진다… 전투에서 도주는 항상 가능하다." if GameManager.current_locale == "ko" else "Memory noise is closing in… ambient battles can always be fled."
		NotificationToast.show_toast(warning, NotificationToast.ToastType.WARNING)

	if data.step_count >= data.threshold:
		data.step_count = 0.0
		data.threshold = randf_range(data.min_steps, data.max_steps)
		data.warning_emitted = false
		_trigger_encounter(data)
		return true

	return false

## 랜덤 적 선택 + 전투 시작
static func _trigger_encounter(data: EncounterData) -> void:
	if data.enemy_pool.is_empty():
		return

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
