## DialogueManager (Autoload)
## 대화 시스템. 대화 데이터 로드 및 진행 관리.
extends Node

signal dialogue_started()
signal dialogue_line(speaker: String, text: String, portrait: String)
signal dialogue_choice(choices: Array)
signal dialogue_ended()

var is_active: bool = false
var current_dialogue: Array = []
var current_index: int = 0

# S146: 현재 화면에 표시 중인(필터링 통과한) 선택지. select_choice는 이 배열을 기준으로 동작.
var _current_choices: Array = []

# JSON에서 로드한 대화 데이터 캐시
var loaded_dialogues: Dictionary = {}

func _ready() -> void:
	print("[DialogueManager] Ready")

## JSON 파일에서 대화 데이터 로드 (캐싱)
func load_dialogue_file(file_path: String) -> bool:
	if loaded_dialogues.has(file_path):
		return true

	if not FileAccess.file_exists(file_path):
		push_error("[DialogueManager] File not found: %s" % file_path)
		return false

	var file = FileAccess.open(file_path, FileAccess.READ)
	var json = JSON.new()
	var error = json.parse(file.get_as_text())
	file.close()

	if error != OK:
		push_error("[DialogueManager] JSON parse error in %s: %s" % [file_path, json.get_error_message()])
		return false

	var data = json.data
	if data is Dictionary and data.has("dialogues"):
		loaded_dialogues[file_path] = data.dialogues
		print("[DialogueManager] Loaded: %s (%d dialogues)" % [file_path, data.dialogues.size()])
		return true

	push_error("[DialogueManager] Invalid dialogue format in %s" % file_path)
	return false

## JSON 파일 로드 + 특정 키로 대화 시작 (NPC에서 호출)
func load_and_start(file_path: String, dialogue_key: String) -> void:
	if not load_dialogue_file(file_path):
		return

	var dialogues = loaded_dialogues[file_path]
	if not dialogues.has(dialogue_key):
		push_error("[DialogueManager] Dialogue key not found: %s" % dialogue_key)
		return

	start_dialogue(dialogues[dialogue_key])

## 대화 시작 — dialogue_data는 Array of Dictionary
## [{"speaker": "Elia", "text": "How bad?", "portrait": "elia_concern"}, ...]
func start_dialogue(dialogue_data: Array) -> void:
	current_dialogue = dialogue_data
	current_index = 0
	is_active = true
	GameManager.change_state(GameManager.GameState.DIALOGUE)
	dialogue_started.emit()
	_show_next_line()

## 다음 대사로 진행
func advance() -> void:
	if not is_active:
		return
	current_index += 1
	_show_next_line()

## 현재 줄 표시
func _show_next_line() -> void:
	if current_index >= current_dialogue.size():
		end_dialogue()
		return

	var line = current_dialogue[current_index]

	# S146: 조건부 라인 — 조건 불충족 시 통째로 건너뜀 (분위기/힌트 라인 게이팅)
	if not _line_condition_met(line):
		advance()
		return

	# S146: 라인 단위 즉시 효과 (플래그/엔딩 기록/보상)
	_apply_line_effects(line)

	# 선택지인 경우
	if line.has("choices"):
		_current_choices = _filter_choices(line.choices)
		if _current_choices.is_empty():
			# 표시 가능한 선택지가 하나도 없으면 선택지 라인을 건너뜀 (안전장치)
			advance()
			return
		dialogue_choice.emit(_localized_choices(_current_choices))
		return

	# 일반 대사
	var speaker = line.get("speaker", "")
	var text = GameManager.localized_value(line, "text", "")
	var portrait = line.get("portrait", "")

	# 기억 연소 여부에 따른 대사 변화 (legacy: requires_memory + burned_text)
	if line.has("requires_memory") and line.has("burned_text"):
		if MemoryManager.is_memory_burned(line.requires_memory):
			text = GameManager.localized_value(line, "burned_text", text)
			portrait = line.get("burned_portrait", portrait)

	dialogue_line.emit(speaker, text, portrait)

## S146: 라인/선택지 표시 조건 판정.
## requires_memory_intact / requires_memory_gone / requires_flag / requires_not_flag / requires_weave
## (legacy requires_memory + burned_text 조합은 라인을 건너뛰지 않으므로 여기서 제외)
func _condition_met(data: Dictionary) -> bool:
	if data.has("requires_memory_intact") and not MemoryManager.is_intact(String(data.requires_memory_intact)):
		return false
	if data.has("requires_memory_gone") and MemoryManager.is_intact(String(data.requires_memory_gone)):
		return false
	if data.has("requires_flag") and not GameManager.get_flag(String(data.requires_flag)):
		return false
	if data.has("requires_not_flag") and GameManager.get_flag(String(data.requires_not_flag)):
		return false
	if data.get("requires_weave", false) and not MemoryManager.weave_unlocked():
		return false
	return true

func _line_condition_met(line: Dictionary) -> bool:
	# legacy requires_memory(+burned_text) 라인은 항상 표시 (텍스트만 교체)
	if line.has("requires_memory") and line.has("burned_text"):
		return true
	return _condition_met(line)

## S146: 조건을 통과한 선택지만 남김
func _filter_choices(choices: Array) -> Array:
	var result: Array = []
	for c in choices:
		if c is Dictionary and not _condition_met(c):
			continue
		result.append(c)
	return result

## S146: 라인/선택지 공통 효과 적용 (SceneFlow와 패리티)
func _apply_line_effects(data: Dictionary) -> void:
	if data.has("set_flag"):
		GameManager.set_flag(String(data.set_flag))
	if data.has("record_ending") and GameManager.has_method("record_ending"):
		GameManager.record_ending(String(data.record_ending))

func _localized_choices(choices: Array) -> Array:
	var localized: Array = []
	for choice in choices:
		if choice is Dictionary:
			var copy: Dictionary = choice.duplicate(true)
			copy["text"] = GameManager.localized_value(copy, "text", String(copy.get("text", "...")))
			if copy.has("effect"):
				copy["effect"] = GameManager.localized_value(copy, "effect", String(copy.effect))
			localized.append(copy)
		else:
			localized.append(choice)
	return localized

## 선택지 결과 처리 — _current_choices(필터링된 목록) 기준
func select_choice(choice_index: int) -> void:
	if not is_active or current_index < 0 or current_index >= current_dialogue.size():
		return
	if choice_index >= 0 and choice_index < _current_choices.size():
		var choice = _current_choices[choice_index]

		# 플래그 설정
		if choice.has("set_flag"):
			GameManager.set_flag(choice.set_flag)

		# 기억 연소 트리거
		if choice.has("burn_memory"):
			MemoryManager.burn_memory(choice.burn_memory)
		# S146: Memory Leverage — cost_memory는 burn_memory의 의미적 별칭 (대가성 강조)
		if choice.has("cost_memory"):
			var lost = MemoryManager.burn_memory(String(choice.cost_memory))
			if lost != null:
				NotificationToast.show_toast("Memory spent: %s" % lost.title, NotificationToast.ToastType.WARNING)

		# S146: 엔딩 기록 (데이터 주도)
		if choice.has("record_ending") and GameManager.has_method("record_ending"):
			GameManager.record_ending(String(choice.record_ending))

		# Grains 지급
		if choice.has("add_grains"):
			GameManager.player_data.grains += int(choice.add_grains)
			NotificationToast.show_toast("+%d Grains" % int(choice.add_grains), NotificationToast.ToastType.SUCCESS)

		# S146: 아이템/회복 보상 (SceneFlow 패리티)
		if choice.has("add_item"):
			GameManager.add_item(String(choice.add_item), int(choice.get("add_item_count", 1)))
		if choice.has("heal_player"):
			var heal := int(choice.heal_player)
			var cur_hp := int(GameManager.player_data.get("hp", 100))
			var max_hp := int(GameManager.player_data.get("max_hp", 100))
			var actual := mini(heal, max_hp - cur_hp)
			if actual > 0:
				GameManager.player_data.hp = cur_hp + actual
				NotificationToast.show_toast("+%d HP" % actual, NotificationToast.ToastType.SUCCESS)

		# 다음 대사로 점프
		if choice.has("jump_to"):
			current_index = choice.jump_to - 1  # advance()에서 +1 되므로

	_current_choices = []
	advance()

## 대화 종료
func end_dialogue() -> void:
	is_active = false
	current_dialogue = []
	current_index = 0
	_current_choices = []
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	dialogue_ended.emit()
