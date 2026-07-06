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

# S149: 챕터 원장 — set_chapter 시점의 연소 수 스냅샷
var _ledger_burn_snapshot: int = 0

# S168: VN AUTO 모드 — 씬 인스턴스가 바뀌어도 유지되는 세션 설정
var vn_auto_mode: bool = false

# VN UI 인스턴스
var _vn_ui: Node = null

func _ready() -> void:
	print("[SceneFlow] Ready")

## Save only identifiers and indices. Scene JSON and UI nodes are rebuilt on load.
func export_data() -> Dictionary:
	var queue_data: Array = []
	for entry in resume_queue:
		if entry is Dictionary:
			var scene_id := String(entry.get("scene_id", ""))
			if scene_id != "":
				queue_data.append({
					"scene_id": scene_id,
					"index": maxi(0, int(entry.get("index", 0))),
				})
	return {
		"current_id": current_id,
		"current_index": maxi(0, current_index),
		"pending_scene_id": pending_scene_id,
		"pending_start_index": maxi(0, pending_start_index),
		"resume_queue": queue_data,
		"is_active": is_active,
		"ledger_burn_snapshot": _ledger_burn_snapshot,
	}

## Restore serializable flow state without reviving stale JSON or UI objects.
func import_data(data: Dictionary) -> void:
	_close_vn_ui()
	current_scene = {}
	current_steps = []
	current_id = String(data.get("current_id", ""))
	current_index = maxi(0, int(data.get("current_index", 0)))
	pending_scene_id = String(data.get("pending_scene_id", ""))
	pending_start_index = maxi(0, int(data.get("pending_start_index", 0)))
	# Older saves predate the ledger snapshot. Starting from the currently loaded
	# burn count prevents old chapter losses from being reported as new ones.
	_ledger_burn_snapshot = clampi(
		int(data.get("ledger_burn_snapshot", MemoryManager.burned_memories.size())),
		0,
		MemoryManager.burned_memories.size()
	)
	resume_queue.clear()
	var saved_queue: Variant = data.get("resume_queue", [])
	if saved_queue is Array:
		for entry in saved_queue:
			if not (entry is Dictionary):
				continue
			var scene_id := String(entry.get("scene_id", ""))
			if scene_id == "":
				continue
			resume_queue.append({
				"scene_id": scene_id,
				"index": maxi(0, int(entry.get("index", 0))),
			})
	is_active = bool(data.get("is_active", false)) and current_id != ""

## Convert an active saved VN step into the pending state consumed by VNHost._ready().
func prepare_resume_from_save(data: Dictionary) -> void:
	import_data(data)
	var resume_id := ""
	var resume_index := 0
	if is_active and current_id != "":
		resume_id = current_id
		resume_index = current_index
	elif pending_scene_id != "":
		resume_id = pending_scene_id
		resume_index = pending_start_index

	if resume_id == "" or _load_scene(resume_id).is_empty():
		push_warning("[SceneFlow] VN save had no valid scene id; restarting from ch1_prologue")
		resume_id = "ch1_prologue"
		resume_index = 0

	pending_scene_id = resume_id
	pending_start_index = maxi(0, resume_index)
	is_active = false
	current_scene = {}
	current_steps = []

## 외부 진입점 — scene id로 시퀀스 재생
func play(scene_id: String, start_index: int = 0) -> void:
	var data = _load_scene(scene_id)
	if data.is_empty():
		push_error("[SceneFlow] Scene not found: %s" % scene_id)
		return

	current_id = scene_id
	current_scene = data
	current_steps = data.get("steps", [])
	current_index = clampi(start_index, 0, current_steps.size())
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
	if current_index < 0:
		current_index = 0
	if current_index >= current_steps.size():
		_end_scene()
		return

	var step: Dictionary = current_steps[current_index]

	# 플래그/기억 처리 (즉시 실행, UI 안 건드림)
	if step.has("set_flag"):
		GameManager.set_flag(step.set_flag)
	if step.has("set_chapter"):
		var next_chapter := int(step.set_chapter)
		if next_chapter != GameManager.current_chapter:
			GameManager.current_chapter = next_chapter
			MemoryManager.add_chapter_memories(next_chapter)
		# S149: 챕터 원장 — 연소 스냅샷 (complete_chapter에서 델타 계산)
		_ledger_burn_snapshot = MemoryManager.burned_memories.size()
	if step.has("complete_chapter"):
		if has_node("/root/AchievementManager"):
			AchievementManager.record_chapter_complete(int(step.complete_chapter))
		# S149: 챕터 원장 오버레이 — 이번 장의 대차대조
		_show_chapter_ledger(int(step.complete_chapter))
	if step.get("autosave_chapter_transition", false) and has_node("/root/SaveManager"):
		SaveManager.autosave_on_chapter_transition()
	if step.has("burn_memory"):
		MemoryManager.burn_memory(step.burn_memory, bool(step.get("allow_faded_burn", false)))
	# S147: 데이터 주도 엔딩 기록 (DialogueManager 패리티)
	if step.has("record_ending"):
		GameManager.record_ending(String(step.record_ending))
	# S147: Weave 해금 상태를 플래그로 노출 (VN 선택지 requires_flag 필터에서 사용)
	if step.get("check_weave", false):
		GameManager.story_flags["p3_weave_ready"] = MemoryManager.weave_unlocked()
		print("[SceneFlow] check_weave → p3_weave_ready = %s" % MemoryManager.weave_unlocked())

	# 조건부 건너뛰기 (requires_flag / requires_not_flag)
	if step.has("requires_flag") and not GameManager.story_flags.get(step.requires_flag, false):
		advance()
		return
	if step.has("requires_not_flag") and GameManager.story_flags.get(step.requires_not_flag, false):
		advance()
		return

	_apply_reward_fields(step)

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
			if step.has("distorted_text_ko"):
				step["text_ko"] = step.distorted_text_ko
			if step.has("distorted_narrate"):
				step["narrate"] = step.distorted_narrate
			if step.has("distorted_narrate_ko"):
				step["narrate_ko"] = step.distorted_narrate_ko
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
			# Deprecated after the VN pivot. Never pass a string into the legacy battle API.
			push_warning("[SceneFlow] Deprecated goto_battle ignored in '%s' at step %d" % [current_id, current_index])
			advance()
		"resolve_part3_ending":
			# S147: Part III 엔딩 허브 — 기억 상태 + 플래그로 최종 엔딩 판정.
			# p3e_<id> 플래그를 세워 ch23/ch24의 requires_flag 블록이 라우팅되게 함.
			var ending_id: String = GameManager.evaluate_part3_ending()
			GameManager.set_flag("p3e_%s" % ending_id)
			GameManager.record_ending(ending_id)
			print("[SceneFlow] Part III ending resolved: %s" % ending_id)
			advance()
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
	if not is_active or current_index < 0 or current_index >= current_steps.size():
		return
	var step = current_steps[current_index]
	if not step.has("choice"):
		return

	var choices: Array = step.choice
	if choice_index < 0 or choice_index >= choices.size():
		return

	var choice: Dictionary = choices[choice_index]
	# Re-check gates at selection time. UI filtering alone is not authoritative:
	# a save/load, test call, or another overlay can change memory state.
	if choice.has("requires_memory_intact") and not MemoryManager.is_intact(String(choice.requires_memory_intact)):
		push_warning("[SceneFlow] Choice requires an intact memory: %s" % choice.requires_memory_intact)
		return
	if choice.has("requires_flag") and not GameManager.story_flags.get(choice.requires_flag, false):
		return
	if choice.has("requires_not_flag") and GameManager.story_flags.get(choice.requires_not_flag, false):
		return

	# Pay explicit memory costs before applying flags or rewards.
	if choice.has("cost_memory"):
		var burned = MemoryManager.burn_memory(choice.cost_memory, bool(choice.get("allow_faded_burn", false)))
		if burned == null:
			push_warning("[SceneFlow] Memory cost could not be paid: %s" % choice.cost_memory)
			return
	if choice.has("set_flag"):
		GameManager.set_flag(choice.set_flag)
	# S149: 복수 플래그 지원 (기억 열쇠 선택지 등 — 분기 플래그 + 열쇠 플래그 동시 설정)
	if choice.has("set_flags") and choice.set_flags is Array:
		for f in choice.set_flags:
			GameManager.set_flag(String(f))
	if choice.has("burn_memory"):
		MemoryManager.burn_memory(choice.burn_memory, bool(choice.get("allow_faded_burn", false)))
	_apply_reward_fields(choice)
	if choice.has("goto"):
		current_index = int(choice.goto) - 1

	advance()

func _apply_reward_fields(data: Dictionary) -> void:
	if data.has("add_grains"):
		var grains := int(data.add_grains)
		GameManager.player_data.grains = int(GameManager.player_data.get("grains", 0)) + grains
		GameManager.add_stat("total_grains_earned", grains)
		if has_node("/root/NotificationToast"):
			NotificationToast.show_toast("+%d Grains" % grains, NotificationToast.ToastType.SUCCESS)
	if data.has("add_item"):
		var item_id := String(data.add_item)
		var count := int(data.get("add_item_count", 1))
		GameManager.add_item(item_id, count)
	if data.has("heal_player"):
		var heal := int(data.heal_player)
		var current_hp := int(GameManager.player_data.get("hp", 100))
		var max_hp := int(GameManager.player_data.get("max_hp", 100))
		var actual := mini(heal, max_hp - current_hp)
		if actual > 0:
			GameManager.player_data.hp = current_hp + actual
			if has_node("/root/NotificationToast"):
				NotificationToast.show_toast("+%d HP" % actual, NotificationToast.ToastType.SUCCESS)

## ===================== S149: 챕터 원장 오버레이 =====================
## 세이블의 장부 모티프 — 챕터가 끝날 때 이번 장의 대차대조를 잠시 보여준다.
## 비차단(클릭 무시), 5초 후 자동 소멸. 누적소실률 게이지 금지 규칙 준수(개수/이름만, 바 없음).
func _show_chapter_ledger(chapter: int) -> void:
	if not is_inside_tree():
		return
	var tree = get_tree()
	if tree == null or tree.root == null:
		return
	var ko := GameManager.current_locale == "ko"

	# 이번 장에서 태운 기억
	var burned_now: Array = []
	for i in range(_ledger_burn_snapshot, MemoryManager.burned_memories.size()):
		burned_now.append(MemoryManager.burned_memories[i])
	# 남은 온전한 기억 수
	var held := 0
	for m in MemoryManager.memories:
		if not m.is_burned and not m.is_faded:
			held += 1
	var anchors := MemoryManager.intact_anchor_count()
	var name_intact := MemoryManager.is_intact(MemoryManager.WEAVE_PRIMARY)

	var overlay = CanvasLayer.new()
	overlay.layer = 115
	tree.root.add_child(overlay)

	var panel = PanelContainer.new()
	panel.set_anchors_preset(Control.PRESET_CENTER_TOP)
	panel.position = Vector2(-260, 46)
	panel.custom_minimum_size = Vector2(520, 0)
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var pstyle = StyleBoxFlat.new()
	pstyle.bg_color = Color(0.035, 0.03, 0.045, 0.92)
	pstyle.border_color = Color(0.62, 0.5, 0.3, 0.7)
	pstyle.set_border_width_all(1)
	pstyle.set_corner_radius_all(4)
	pstyle.set_content_margin_all(14)
	panel.add_theme_stylebox_override("panel", pstyle)
	overlay.add_child(panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 5)
	vbox.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(vbox)

	var title = Label.new()
	title.text = ("장부 — 제%d장" % chapter) if ko else ("THE LEDGER — CHAPTER %d" % chapter)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 16)
	title.add_theme_color_override("font_color", Color(0.9, 0.78, 0.5))
	vbox.add_child(title)

	var lines: Array[String] = []
	if burned_now.is_empty():
		lines.append("이번 장에서 태운 기억: 없음" if ko else "Burned this chapter: nothing")
	else:
		var names: Array[String] = []
		for m in burned_now:
			names.append(str(m.title))
		var joined := ", ".join(names) if names.size() <= 3 else (", ".join(names.slice(0, 3)) + (" 외 %d" % (names.size() - 3) if ko else " +%d more" % (names.size() - 3)))
		lines.append(("이번 장에서 태운 기억 %d — %s" % [burned_now.size(), joined]) if ko else ("Burned this chapter: %d — %s" % [burned_now.size(), joined]))
	lines.append(("아직 온전한 기억: %d" % held) if ko else ("Still held intact: %d" % held))
	var anchor_line := ("닻: %d/4 · 이름: %s" % [anchors, "온전" if name_intact else "소실"]) if ko \
		else ("Anchors: %d/4 · The name: %s" % [anchors, "intact" if name_intact else "gone"])
	lines.append(anchor_line)
	# 실(thread) 상태 — Weave 경로 가능 여부의 간접 표현 (게이지 금지 규칙)
	var thread_ok := MemoryManager.weave_unlocked()
	lines.append(("실은 아직 이어져 있다." if thread_ok else "실이 닳아 가고 있다.") if ko \
		else ("The thread still holds." if thread_ok else "The thread is fraying."))

	for lt in lines:
		var lbl = Label.new()
		lbl.text = lt
		lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		lbl.add_theme_font_size_override("font_size", 13)
		lbl.add_theme_color_override("font_color", Color(0.82, 0.78, 0.72))
		vbox.add_child(lbl)
	# 마지막 실 상태 라인만 색 구분
	var last_lbl: Label = vbox.get_child(vbox.get_child_count() - 1)
	last_lbl.add_theme_color_override("font_color", Color(0.45, 0.85, 0.8) if thread_ok else Color(0.85, 0.55, 0.4))

	panel.modulate.a = 0.0
	var tw = overlay.create_tween()
	tw.tween_property(panel, "modulate:a", 1.0, 0.4).set_ease(Tween.EASE_OUT)
	tw.tween_interval(4.6)
	tw.tween_property(panel, "modulate:a", 0.0, 0.6).set_ease(Tween.EASE_IN)
	tw.tween_callback(overlay.queue_free)
	print("[SceneFlow] Chapter ledger shown — ch%d, burned %d, held %d, anchors %d/4" % [chapter, burned_now.size(), held, anchors])

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
		if _vn_ui.has_method("prepare_for_close"):
			_vn_ui.prepare_for_close()
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
