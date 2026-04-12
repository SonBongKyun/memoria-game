## SaveManager (Autoload)
## 세이브/로드 시스템. JSON 파일로 게임 상태 저장.
## F6 = 퀵세이브(슬롯 1), F7 = 퀵로드(슬롯 1)
extends Node

const SAVE_DIR: String = "user://saves/"
const MAX_SLOTS: int = 3

signal save_completed(slot: int)
signal load_completed(slot: int)
signal save_failed(reason: String)

## 로드 시 플레이어 위치 복원용 (맵 스크립트에서 참조)
var loaded_player_pos: Dictionary = {}

func _ready() -> void:
	# 세이브 디렉토리 생성
	if not DirAccess.dir_exists_absolute(SAVE_DIR):
		DirAccess.make_dir_recursive_absolute(SAVE_DIR)
	print("[SaveManager] Ready — save dir: %s" % SAVE_DIR)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		# F6 = 퀵세이브, F7 = 퀵로드 (탐색 중에만)
		if GameManager.current_state != GameManager.GameState.EXPLORATION:
			return
		if event.physical_keycode == KEY_F6:
			save_game(1)
			get_viewport().set_input_as_handled()
		elif event.physical_keycode == KEY_F7:
			load_game(1)
			get_viewport().set_input_as_handled()

## 게임 저장
func save_game(slot: int) -> bool:
	if slot < 1 or slot > MAX_SLOTS:
		save_failed.emit("Invalid slot: %d" % slot)
		return false

	# 플레이어 위치 저장
	var player_pos: Dictionary = {}
	var players = get_tree().get_nodes_in_group("player")
	if players.size() > 0:
		var pos = players[0].position
		player_pos = {"x": pos.x, "y": pos.y}

	var save_data: Dictionary = {
		"version": "0.1.0",
		"timestamp": Time.get_datetime_string_from_system(),
		"scene": _get_current_scene_path(),
		"game": GameManager.export_data(),
		"memory": MemoryManager.export_data(),
		"elia_diary": EliaDiary.export_data(),
		"tutorial_hints": TutorialHints.export_data(),
		"player_pos": player_pos,
	}

	var path = SAVE_DIR + "save_%d.json" % slot
	var file = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		var err_msg = "Failed to open save file: %s" % path
		push_error("[SaveManager] %s" % err_msg)
		save_failed.emit(err_msg)
		return false

	file.store_string(JSON.stringify(save_data, "\t"))
	file.close()

	print("[SaveManager] Saved to slot %d" % slot)
	save_completed.emit(slot)
	return true

## 게임 로드
func load_game(slot: int) -> bool:
	if slot < 1 or slot > MAX_SLOTS:
		return false

	var path = SAVE_DIR + "save_%d.json" % slot
	if not FileAccess.file_exists(path):
		print("[SaveManager] No save in slot %d" % slot)
		return false

	var file = FileAccess.open(path, FileAccess.READ)
	var json = JSON.new()
	var error = json.parse(file.get_as_text())
	file.close()

	if error != OK:
		push_error("[SaveManager] Parse error in slot %d: %s" % [slot, json.get_error_message()])
		return false

	var save_data = json.data

	# 게임 데이터 복원
	if save_data.has("game"):
		GameManager.import_data(save_data.game)

	if save_data.has("memory"):
		MemoryManager.import_data(save_data.memory)

	if save_data.has("elia_diary"):
		EliaDiary.import_data(save_data.elia_diary)

	if save_data.has("tutorial_hints"):
		TutorialHints.import_data(save_data.tutorial_hints)

	# 플레이어 위치 복원 준비
	loaded_player_pos = save_data.get("player_pos", {})

	# 씬 전환
	var scene_path = save_data.get("scene", "")
	if scene_path != "" and ResourceLoader.exists(scene_path):
		SceneTransition.change_scene_styled(scene_path)

	print("[SaveManager] Loaded slot %d (saved: %s)" % [slot, save_data.get("timestamp", "?")])
	load_completed.emit(slot)
	return true

## 슬롯에 세이브가 있는지 확인
func has_save(slot: int) -> bool:
	return FileAccess.file_exists(SAVE_DIR + "save_%d.json" % slot)

## 세이브 정보 가져오기 (슬롯 선택 UI용)
func get_save_info(slot: int) -> Dictionary:
	var path = SAVE_DIR + "save_%d.json" % slot
	if not FileAccess.file_exists(path):
		return {}

	var file = FileAccess.open(path, FileAccess.READ)
	var json = JSON.new()
	if json.parse(file.get_as_text()) != OK:
		file.close()
		return {}
	file.close()

	var data = json.data
	var game_data = data.get("game", {})
	var mem_data = data.get("memory", {})
	# S41: 세이브 슬롯에 더 많은 정보 표시
	var scene_path: String = data.get("scene", "")
	var location: String = ""
	if scene_path != "":
		location = scene_path.get_file().get_basename().replace("_", " ").capitalize()
	var hp_val: int = game_data.get("player_data", {}).get("hp", 0)
	var max_hp_val: int = game_data.get("player_data", {}).get("max_hp", 100)
	var grains_val: int = game_data.get("player_data", {}).get("grains", 0)
	return {
		"timestamp": data.get("timestamp", ""),
		"chapter": game_data.get("current_chapter", 1),
		"burn_count": mem_data.get("burned", []).size(),
		"location": location,
		"hp": hp_val,
		"max_hp": max_hp_val,
		"grains": grains_val,
		"equipped": game_data.get("equipped", {}),
	}

## 현재 씬 경로
func _get_current_scene_path() -> String:
	var scene = get_tree().current_scene
	if scene and scene.scene_file_path != "":
		return scene.scene_file_path
	return ""
