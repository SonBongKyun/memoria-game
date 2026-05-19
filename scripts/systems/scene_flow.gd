## SceneFlow (Autoload) — VN/Hybrid Scene Runner
## S60: 삽화 중심 VN 시퀀스 재생기. CG + 포트레이트 + 나레이션으로 스토리를 흐름.
## 핵심 앵커(goto_map / goto_battle)에서 탐색/전투 씬으로 핸드오프 후 복귀.
extends Node

signal step_changed(step: Dictionary)
signal scene_started(scene_id: String)
signal scene_ended(scene_id: String)

const SCENE_DIR: String = "res://data/vn_scenes/"

var is_active: bool = false
var current_id: String = ""
var current_scene: Dictionary = {}
var current_steps: Array = []
var current_index: int = 0

# 탐색/전투 복귀 큐
var resume_queue: Array = []  # [{scene_id, index}]

# VNHost _ready에서 자동 재생할 씬 (씬 전환 직후 race 방지용)
var pending_scene_id: String = ""
var pending_start_index: int = 0

# 로드 캐시
var _cache: Dictionary = {}

# VN UI 인스턴스
var _vn_ui: Node = null

func _ready() -> void:
	print("[SceneFlow] Ready")

## 외부 진입점 — scene id로 시퀀스 재생
func play(scene_id: String, start_index: int = 0) -> void:
	var data = _load_scene(scene_id)
	if data.is_empty():
		push_error("[SceneFlow] Scene not found: %s" % scene_id)
		return

	current_id = scene_id
	current_scene = data
	current_steps = data.get("steps", [])
	current_index = start_index
	is_active = true

	# BGM
	if data.has("bgm") and has_node("/root/AudioManager"):
		AudioManager.play_bgm(data.bgm)

	_ensure_vn_ui()
	GameManager.change_state(GameManager.GameState.DIALOGUE)
	scene_started.emit(scene_id)
	print("[SceneFlow] Playing: %s (%d steps)" % [scene_id, current_steps.size()])
	_run_step()

## 다음 단계로 진행 (VN UI에서 호출)
func advance() -> void:
	if not is_active:
		return
	current_index += 1
	_run_step()

## 현재 단계 실행
func _run_step() -> void:
	if current_index >= current_steps.size():
		_end_scene()
		return

	var step: Dictionary = current_steps[current_index]

	# 플래그/기억 처리 (즉시 실행, UI 안 건드림)
	if step.has("set_flag"):
		GameManager.set_flag(step.set_flag)
	if step.has("set_chapter"):
		GameManager.current_chapter = int(step.set_chapter)
	if step.has("complete_chapter") and has_node("/root/AchievementManager"):
		AchievementManager.record_chapter_complete(int(step.complete_chapter))
	if step.get("autosave_chapter_transition", false) and has_node("/root/SaveManager"):
		SaveManager.autosave_on_chapter_transition()
	if step.has("burn_memory"):
		MemoryManager.burn_memory(step.burn_memory)

	# 조건부 건너뛰기 (requires_flag / requires_not_flag)
	if step.has("requires_flag") and not GameManager.story_flags.get(step.requires_flag, false):
		advance()
		return
	if step.has("requires_not_flag") and GameManager.story_flags.get(step.requires_not_flag, false):
		advance()
		return

	# 액션 처리 (UI와 무관한 전환)
	if step.has("action"):
		_handle_action(step)
		return

	# S61: 기억 왜곡 (Katana ZERO 패턴) — 태운 기억이 있으면 이 씬의 텍스트/CG/포트레이트 교체
	if step.has("distort_if_burned"):
		var mid: String = step.distort_if_burned
		if MemoryManager.is_memory_burned(mid):
			step = step.duplicate(true)
			if step.has("distorted_text"):
				step["text"] = step.distorted_text
			if step.has("distorted_narrate"):
				step["narrate"] = step.distorted_narrate
			if step.has("distorted_speaker"):
				step["speaker"] = step.distorted_speaker
			if step.has("distorted_portrait"):
				step["portrait"] = step.distorted_portrait
			if step.has("distorted_cg"):
				step["cg"] = step.distorted_cg
			step["_distorted"] = true  # VN UI에 왜곡 상태 신호

	# 일반 스텝 — VN UI에 위임
	step_changed.emit(step)

## 액션 (goto_map / goto_battle / goto_scene / end)
func _handle_action(step: Dictionary) -> void:
	var action: String = step.action

	match action:
		"goto_scene":
			var next_id: String = step.get("id", "")
			if next_id != "":
				# 현재 씬 종료 후 바로 다음 씬
				current_index += 1
				play(next_id, int(step.get("start_index", 0)))
		"goto_map":
			var path: String = step.get("path", "")
			if step.has("resume_scene"):
				resume_queue.append({
					"scene_id": step.resume_scene,
					"index": int(step.get("resume_index", 0)),
				})
			_close_vn_ui()
			is_active = false
			GameManager.change_state(GameManager.GameState.EXPLORATION)
			if path != "":
				SceneTransition.change_scene_styled(path)
		"goto_battle":
			# 전투 시작 — 전투 종료 후 복귀
			if step.has("resume_scene"):
				resume_queue.append({
					"scene_id": step.resume_scene,
					"index": int(step.get("resume_index", 0)),
				})
			_close_vn_ui()
			is_active = false
			# TODO: 배틀 호출은 BattleManager API에 의존
			if has_node("/root/BattleManager"):
				var enemy_id = step.get("enemy", "")
				BattleManager.start_battle(enemy_id)
		"end":
			_end_scene()
		"demo_end":
			# S66: A안 데모 빌드 종료 — 위시리스트 CTA 화면으로
			_close_vn_ui()
			is_active = false
			GameManager.change_state(GameManager.GameState.MENU)
			SceneTransition.change_scene_styled("res://scenes/ui/demo_end.tscn")
		"wait":
			# 자동 진행 대기 (UI가 타이머 후 advance)
			step_changed.emit(step)
		_:
			push_warning("[SceneFlow] Unknown action: %s" % action)
			advance()

## 탐색/전투에서 VN 씬으로 복귀 (맵/전투 스크립트에서 호출)
func resume_if_queued() -> bool:
	if resume_queue.is_empty():
		return false
	var entry = resume_queue.pop_front()
	play(entry.scene_id, int(entry.index))
	return true

## 선택지 처리 (VN UI에서 호출)
func select_choice(choice_index: int) -> void:
	var step = current_steps[current_index]
	if not step.has("choice"):
		return

	var choices: Array = step.choice
	if choice_index < 0 or choice_index >= choices.size():
		return

	var choice: Dictionary = choices[choice_index]

	if choice.has("set_flag"):
		GameManager.set_flag(choice.set_flag)
	if choice.has("burn_memory"):
		MemoryManager.burn_memory(choice.burn_memory)
	# S63: Memory Leverage — cost_memory는 burn_memory의 의미적 별칭 (UI에서 구분 강조됨)
	if choice.has("cost_memory"):
		MemoryManager.burn_memory(choice.cost_memory)
	if choice.has("add_grains"):
		GameManager.player_data.grains = int(GameManager.player_data.get("grains", 0)) + int(choice.add_grains)
		if has_node("/root/NotificationToast"):
			NotificationToast.show_toast("+%d Grains" % int(choice.add_grains), NotificationToast.ToastType.SUCCESS)
	if choice.has("goto"):
		current_index = int(choice.goto) - 1

	advance()

## ===================== 내부 =====================

func _load_scene(scene_id: String) -> Dictionary:
	if _cache.has(scene_id):
		return _cache[scene_id]

	var path = SCENE_DIR + scene_id + ".json"
	if not FileAccess.file_exists(path):
		return {}

	var file = FileAccess.open(path, FileAccess.READ)
	var json = JSON.new()
	var err = json.parse(file.get_as_text())
	file.close()

	if err != OK:
		push_error("[SceneFlow] JSON parse error: %s — %s" % [path, json.get_error_message()])
		return {}

	var data = json.data
	if not (data is Dictionary):
		return {}

	_cache[scene_id] = data
	return data

func _ensure_vn_ui() -> void:
	if _vn_ui != null and is_instance_valid(_vn_ui):
		_vn_ui.visible = true
		return
	var scene: PackedScene = load("res://scenes/ui/vn_scene.tscn")
	if scene == null:
		push_error("[SceneFlow] vn_scene.tscn not found")
		return
	_vn_ui = scene.instantiate()
	get_tree().root.add_child(_vn_ui)

func _close_vn_ui() -> void:
	if _vn_ui != null and is_instance_valid(_vn_ui):
		# 즉시 숨김 + 입력 차단 해제 (queue_free는 다음 프레임이라 그 사이 입력 가로챔 방지)
		_vn_ui.visible = false
		_vn_ui.set_process_input(false)
		_vn_ui.set_process_unhandled_input(false)
		_vn_ui.queue_free()
		_vn_ui = null

func _end_scene() -> void:
	var ended_id = current_id
	is_active = false
	current_steps = []
	current_index = 0
	current_id = ""
	_close_vn_ui()
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	scene_ended.emit(ended_id)
	print("[SceneFlow] Ended: %s" % ended_id)
