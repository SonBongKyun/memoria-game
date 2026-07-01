## SaveManager (Autoload)
## 세이브/로드 시스템. JSON 파일로 게임 상태 저장.
## F6 = 퀵세이브(슬롯 1), F7 = 퀵로드(슬롯 1)
## S56: Autosave + Save Backup + Corruption Recovery
## S58: Steam Cloud save hooks (GodotSteam integration-ready)
extends Node

const SAVE_DIR: String = "user://saves/"
const MAX_SLOTS: int = 3
const AUTOSAVE_SLOT: int = 0  # Dedicated autosave slot
const AUTOSAVE_INTERVAL: float = 300.0  # 5 minutes

signal save_completed(slot: int)
signal load_completed(slot: int)
signal save_failed(reason: String)
signal autosave_completed()

## 로드 시 플레이어 위치 복원용 (맵 스크립트에서 참조)
var loaded_player_pos: Dictionary = {}

## S56: Autosave timer
var _autosave_timer: float = 0.0
var _autosave_enabled: bool = true

## S56: Last save timestamp (unix) for "Last Saved: X minutes ago"
var _last_save_time: float = 0.0

## S56: Save indicator UI
var _save_indicator: Control = null
var _save_indicator_tween: Tween = null

func _ready() -> void:
	# 세이브 디렉토리 생성
	if not DirAccess.dir_exists_absolute(SAVE_DIR):
		DirAccess.make_dir_recursive_absolute(SAVE_DIR)
	_build_save_indicator()
	# Connect to chapter transitions and boss battles for autosave
	_connect_autosave_signals()
	print("[SaveManager] Ready — save dir: %s (autosave every %ds)" % [SAVE_DIR, int(AUTOSAVE_INTERVAL)])

func _process(delta: float) -> void:
	# S56: Autosave timer (only during exploration)
	if _autosave_enabled and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_autosave_timer += delta
		if _autosave_timer >= AUTOSAVE_INTERVAL:
			_autosave_timer = 0.0
			autosave("timer")

func _connect_autosave_signals() -> void:
	# Autosave before boss battles
	if BattleManager.has_signal("battle_started"):
		BattleManager.battle_started.connect(_on_battle_started_autosave)

func _on_battle_started_autosave(enemy) -> void:
	if enemy and enemy.is_boss:
		autosave("boss_battle")

## S56: Trigger autosave on chapter transition (call from map scripts)
func autosave_on_chapter_transition() -> void:
	autosave("chapter_transition")

## S56: Autosave logic
func autosave(reason: String = "auto") -> void:
	if GameManager.current_state == GameManager.GameState.MENU:
		return  # Don't autosave on title screen
	var success = save_game(AUTOSAVE_SLOT)
	if success:
		_autosave_timer = 0.0  # Reset timer after any autosave
		autosave_completed.emit()
		print("[SaveManager] Autosave (%s) complete" % reason)

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
	if slot < 0 or slot > MAX_SLOTS:
		save_failed.emit("Invalid slot: %d" % slot)
		return false

	# 플레이어 위치 저장
	var player_pos: Dictionary = {}
	var players = get_tree().get_nodes_in_group("player")
	if players.size() > 0:
		var pos = players[0].position
		player_pos = {"x": pos.x, "y": pos.y}

	var save_data: Dictionary = {
		"version": SAVE_VERSION,
		"timestamp": Time.get_datetime_string_from_system(),
		"unix_time": Time.get_unix_time_from_system(),
		"scene": _get_current_scene_path(),
		"game": GameManager.export_data(),
		"memory": MemoryManager.export_data(),
		"elia_diary": EliaDiary.export_data(),
		"tutorial_hints": TutorialHints.export_data(),
		"player_pos": player_pos,
		"is_autosave": slot == AUTOSAVE_SLOT,
	}

	var path = _get_save_path(slot)

	# S56: Create backup of existing save before overwriting
	_create_backup(path)

	var file = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		var err_msg = "Failed to open save file: %s" % path
		push_error("[SaveManager] %s" % err_msg)
		save_failed.emit(err_msg)
		return false

	file.store_string(JSON.stringify(save_data, "\t"))
	file.close()

	# Update last save time
	_last_save_time = Time.get_unix_time_from_system()

	# Show save indicator
	_show_save_indicator()

	print("[SaveManager] Saved to slot %d" % slot)
	save_completed.emit(slot)
	return true

## 게임 로드
func load_game(slot: int) -> bool:
	if slot < 0 or slot > MAX_SLOTS:
		return false

	var path = _get_save_path(slot)
	if not FileAccess.file_exists(path):
		print("[SaveManager] No save in slot %d" % slot)
		return false

	var save_data = _load_json_with_recovery(path, slot)
	if save_data == null:
		return false

	# S72: Codex 살린 부분 — 구버전 세이브 마이그레이션
	save_data = _migrate_save_data(save_data)

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

	# Update last save time from loaded data
	_last_save_time = save_data.get("unix_time", Time.get_unix_time_from_system())

	# 씬 전환
	var scene_path = save_data.get("scene", "")
	if scene_path != "" and ResourceLoader.exists(scene_path):
		SceneTransition.change_scene_styled(scene_path)

	print("[SaveManager] Loaded slot %d (saved: %s)" % [slot, save_data.get("timestamp", "?")])
	load_completed.emit(slot)
	return true

## S56: Load JSON with corruption recovery — tries .bak if main fails
func _load_json_with_recovery(path: String, slot: int) -> Variant:
	# Try primary file first
	var data = _try_parse_json(path)
	if data != null:
		return data

	# Primary file failed — try backup
	var bak_path = path + ".bak"
	push_warning("[SaveManager] Primary save corrupted for slot %d, trying backup..." % slot)

	if FileAccess.file_exists(bak_path):
		data = _try_parse_json(bak_path)
		if data != null:
			# Recovery successful — notify player
			NotificationToast.show_toast("Save recovered from backup (slot %d)" % slot, NotificationToast.ToastType.WARNING)
			# Restore the backup as the primary save
			var bak_file = FileAccess.open(bak_path, FileAccess.READ)
			var bak_content = bak_file.get_as_text()
			bak_file.close()
			var restore_file = FileAccess.open(path, FileAccess.WRITE)
			if restore_file:
				restore_file.store_string(bak_content)
				restore_file.close()
			print("[SaveManager] Recovered slot %d from backup" % slot)
			return data
		else:
			push_error("[SaveManager] Backup also corrupted for slot %d" % slot)

	# Both failed
	NotificationToast.show_toast("Save data corrupted (slot %d)" % slot, NotificationToast.ToastType.WARNING)
	push_error("[SaveManager] Failed to load slot %d — both primary and backup corrupted" % slot)
	return null

## S56: Try to parse a JSON file, return null on failure
func _try_parse_json(path: String) -> Variant:
	if not FileAccess.file_exists(path):
		return null
	var file = FileAccess.open(path, FileAccess.READ)
	if file == null:
		return null
	var content = file.get_as_text()
	file.close()
	if content.strip_edges() == "":
		return null
	var json = JSON.new()
	var error = json.parse(content)
	if error != OK:
		return null
	if json.data is Dictionary:
		return json.data
	return null

## S72: 구버전 세이브 호환 — 누락 키 보강 + 버전 스탬프 갱신
const SAVE_VERSION: String = "0.2.0"

func _migrate_save_data(data: Dictionary) -> Dictionary:
	var migrated: Dictionary = data.duplicate(true)
	var version: String = str(migrated.get("version", "0.0.0"))
	if version == SAVE_VERSION:
		return migrated

	# 누락 키 기본값 보강
	if not migrated.has("game") or not (migrated["game"] is Dictionary):
		migrated["game"] = {}
	if not migrated.has("memory") or not (migrated["memory"] is Dictionary):
		migrated["memory"] = {}
	if not migrated.has("elia_diary") or not (migrated["elia_diary"] is Dictionary):
		migrated["elia_diary"] = {}
	if not migrated.has("tutorial_hints") or not (migrated["tutorial_hints"] is Dictionary):
		migrated["tutorial_hints"] = {}
	if not migrated.has("player_pos") or not (migrated["player_pos"] is Dictionary):
		migrated["player_pos"] = {}
	if not migrated.has("scene"):
		migrated["scene"] = ""

	migrated["version"] = SAVE_VERSION
	print("[SaveManager] Migrated save from %s to %s" % [version, SAVE_VERSION])
	return migrated

## S56: Create .bak backup before overwriting
func _create_backup(path: String) -> void:
	if not FileAccess.file_exists(path):
		return  # Nothing to back up
	var file = FileAccess.open(path, FileAccess.READ)
	if file == null:
		return
	var content = file.get_as_text()
	file.close()
	if content.strip_edges() == "":
		return
	var bak_path = path + ".bak"
	var bak_file = FileAccess.open(bak_path, FileAccess.WRITE)
	if bak_file:
		bak_file.store_string(content)
		bak_file.close()

## S56: Get save file path for a slot
func _get_save_path(slot: int) -> String:
	if slot == AUTOSAVE_SLOT:
		return SAVE_DIR + "autosave.json"
	return SAVE_DIR + "save_%d.json" % slot

## 슬롯에 세이브가 있는지 확인
func has_save(slot: int) -> bool:
	return FileAccess.file_exists(_get_save_path(slot))

## 세이브 정보 가져오기 (슬롯 선택 UI용)
func get_save_info(slot: int) -> Dictionary:
	var path = _get_save_path(slot)
	if not FileAccess.file_exists(path):
		return {}

	var file = FileAccess.open(path, FileAccess.READ)
	var json = JSON.new()
	if json.parse(file.get_as_text()) != OK:
		file.close()
		return {}
	file.close()

	var data = json.data
	if not (data is Dictionary):
		return {}
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
		"unix_time": data.get("unix_time", 0.0),
		"chapter": game_data.get("current_chapter", 1),
		"burn_count": mem_data.get("burned", []).size(),
		"location": location,
		"hp": hp_val,
		"max_hp": max_hp_val,
		"grains": grains_val,
		"equipped": game_data.get("equipped", {}),
		"is_autosave": data.get("is_autosave", false),
	}

## S56: Get "Last Saved: X minutes ago" text
func get_last_saved_text() -> String:
	if _last_save_time <= 0.0:
		return "Not saved yet"
	var elapsed = Time.get_unix_time_from_system() - _last_save_time
	if elapsed < 60:
		return "Last saved: just now"
	elif elapsed < 3600:
		var mins = int(elapsed / 60)
		return "Last saved: %d min ago" % mins
	else:
		var hours = int(elapsed / 3600)
		return "Last saved: %dh ago" % hours

## S56: Build save indicator UI (small icon top-right corner)
func _build_save_indicator() -> void:
	var canvas = CanvasLayer.new()
	canvas.layer = 90  # High layer, above most UI
	add_child(canvas)

	_save_indicator = PanelContainer.new()
	_save_indicator.anchor_left = 1.0
	_save_indicator.anchor_right = 1.0
	_save_indicator.anchor_top = 0.0
	_save_indicator.anchor_bottom = 0.0
	_save_indicator.offset_left = -120
	_save_indicator.offset_right = -12
	_save_indicator.offset_top = 12
	_save_indicator.offset_bottom = 40
	_save_indicator.modulate.a = 0.0

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.05, 0.08, 0.85)
	style.border_color = Color(0.4, 0.6, 0.3, 0.7)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(6)
	_save_indicator.add_theme_stylebox_override("panel", style)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 6)
	_save_indicator.add_child(hbox)

	# Spinning save icon (using text rotation trick)
	var icon_label = Label.new()
	icon_label.text = "[ ]"
	icon_label.add_theme_font_size_override("font_size", 12)
	icon_label.add_theme_color_override("font_color", Color(0.5, 0.75, 0.4))
	hbox.add_child(icon_label)

	var text_label = Label.new()
	text_label.text = "Saving..."
	text_label.add_theme_font_size_override("font_size", 11)
	text_label.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
	hbox.add_child(text_label)

	canvas.add_child(_save_indicator)

## S56: Show save indicator briefly
func _show_save_indicator() -> void:
	if _save_indicator == null:
		return
	if _save_indicator_tween and _save_indicator_tween.is_valid():
		_save_indicator_tween.kill()
	_save_indicator.modulate.a = 0.0
	_save_indicator_tween = create_tween()
	# Fade in
	_save_indicator_tween.tween_property(_save_indicator, "modulate:a", 1.0, 0.2)
	# Hold
	_save_indicator_tween.tween_interval(1.5)
	# Fade out
	_save_indicator_tween.tween_property(_save_indicator, "modulate:a", 0.0, 0.5)

## 현재 씬 경로
func _get_current_scene_path() -> String:
	var scene = get_tree().current_scene
	if scene and scene.scene_file_path != "":
		return scene.scene_file_path
	return ""

## ===================== S58: Steam Cloud Save Hooks =====================
##
## GodotSteam integration stubs. When GodotSteam plugin is installed:
## 1. Replace is_cloud_available() body with: return Steam.isCloudEnabledForAccount()
## 2. In cloud_save(), after local write, call Steam.fileWrite(filename, bytes)
## 3. In cloud_load(), call Steam.fileRead(filename) and parse JSON
## 4. Add Steam.steamInit() in game_manager.gd _ready()
## Reference: https://godotsteam.com/classes/remote_storage/

## Check if Steam Cloud is available. Stub returns false until GodotSteam is connected.
func is_cloud_available() -> bool:
	# --- GodotSteam Integration Point ---
	# Replace with:
	#   if not Steam.isSteamRunning():
	#       return false
	#   return Steam.isCloudEnabledForAccount() and Steam.isCloudEnabledForApp()
	return false

## Write save data to Steam Cloud (falls back to local save).
## Call this instead of save_game() when Steam integration is active.
func cloud_save(slot: int) -> bool:
	# Always save locally first (acts as cache and offline fallback)
	var local_ok = save_game(slot)
	if not local_ok:
		return false

	if not is_cloud_available():
		return local_ok  # Local save succeeded, cloud not available

	# --- GodotSteam Integration Point ---
	# var path = _get_save_path(slot)
	# var file = FileAccess.open(path, FileAccess.READ)
	# if file:
	#     var content = file.get_as_text()
	#     file.close()
	#     var cloud_filename = "memoria_save_%d.json" % slot
	#     var bytes = content.to_utf8_buffer()
	#     var success = Steam.fileWrite(cloud_filename, bytes)
	#     if success:
	#         print("[SaveManager] Cloud save slot %d synced (%d bytes)" % [slot, bytes.size()])
	#     else:
	#         push_warning("[SaveManager] Cloud save failed for slot %d" % slot)
	#     return success

	print("[SaveManager] Cloud save stub — local save only (slot %d)" % slot)
	return local_ok

## Load save data from Steam Cloud (falls back to local if unavailable).
func cloud_load(slot: int) -> bool:
	if not is_cloud_available():
		return load_game(slot)  # Fallback to local

	# --- GodotSteam Integration Point ---
	# var cloud_filename = "memoria_save_%d.json" % slot
	# if not Steam.fileExists(cloud_filename):
	#     print("[SaveManager] No cloud save for slot %d, trying local" % slot)
	#     return load_game(slot)
	#
	# var file_size = Steam.getFileSize(cloud_filename)
	# var cloud_data = Steam.fileRead(cloud_filename, file_size)
	# if cloud_data.is_empty():
	#     push_warning("[SaveManager] Cloud read failed for slot %d" % slot)
	#     return load_game(slot)
	#
	# # Write cloud data to local path, then load normally
	# var local_path = _get_save_path(slot)
	# var file = FileAccess.open(local_path, FileAccess.WRITE)
	# if file:
	#     file.store_string(cloud_data.get_string_from_utf8())
	#     file.close()
	# print("[SaveManager] Cloud load slot %d synced (%d bytes)" % [slot, file_size])
	# return load_game(slot)

	print("[SaveManager] Cloud load stub — local load only (slot %d)" % slot)
	return load_game(slot)

## Get cloud save info (for save slot UI — show cloud icon if synced).
func has_cloud_save(slot: int) -> bool:
	if not is_cloud_available():
		return false
	# --- GodotSteam Integration Point ---
	# var cloud_filename = "memoria_save_%d.json" % slot
	# return Steam.fileExists(cloud_filename)
	return false
