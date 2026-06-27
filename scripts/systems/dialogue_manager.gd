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

	# 선택지인 경우
	if line.has("choices"):
		dialogue_choice.emit(_localized_choices(line.choices))
		return

	# 일반 대사
	var speaker = line.get("speaker", "")
	var text = GameManager.localized_value(line, "text", "")
	var portrait = line.get("portrait", "")

	# 기억 연소 여부에 따른 대사 변화
	if line.has("requires_memory"):
		if MemoryManager.is_memory_burned(line.requires_memory):
			text = GameManager.localized_value(line, "burned_text", text)
			portrait = line.get("burned_portrait", portrait)

	dialogue_line.emit(speaker, text, portrait)

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

## 선택지 결과 처리
func select_choice(choice_index: int) -> void:
	var line = current_dialogue[current_index]
	if line.has("choices") and choice_index < line.choices.size():
		var choice = line.choices[choice_index]

		# 플래그 설정
		if choice.has("set_flag"):
			GameManager.set_flag(choice.set_flag)

		# 기억 연소 트리거
		if choice.has("burn_memory"):
			MemoryManager.burn_memory(choice.burn_memory)

		# Grains 지급
		if choice.has("add_grains"):
			GameManager.player_data.grains += int(choice.add_grains)
			NotificationToast.show_toast("+%d Grains" % int(choice.add_grains), NotificationToast.ToastType.SUCCESS)

		# 다음 대사로 점프
		if choice.has("jump_to"):
			current_index = choice.jump_to - 1  # advance()에서 +1 되므로

	advance()

## 대화 종료
func end_dialogue() -> void:
	is_active = false
	current_dialogue = []
	current_index = 0
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	dialogue_ended.emit()
