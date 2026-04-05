## DialogueManager (Autoload)
## 대화 시스템 기초. 대화 데이터 로드 및 진행 관리.
extends Node

signal dialogue_started()
signal dialogue_line(speaker: String, text: String, portrait: String)
signal dialogue_choice(choices: Array)
signal dialogue_ended()

var is_active: bool = false
var current_dialogue: Array = []
var current_index: int = 0

func _ready() -> void:
	print("[DialogueManager] Ready")

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
		dialogue_choice.emit(line.choices)
		return

	# 일반 대사
	var speaker = line.get("speaker", "")
	var text = line.get("text", "")
	var portrait = line.get("portrait", "")

	# 기억 연소 여부에 따른 대사 변화
	if line.has("requires_memory"):
		if MemoryManager.is_memory_burned(line.requires_memory):
			text = line.get("burned_text", text)
			portrait = line.get("burned_portrait", portrait)

	dialogue_line.emit(speaker, text, portrait)

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
