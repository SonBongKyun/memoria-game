## SaveManager (Autoload)
## 세이브/로드 시스템. JSON 파일로 게임 상태 저장.
## F6 = 퀵세이브(슬롯 1), F7 = 퀵로드(슬롯 1)
## S56: Autosave + Save Backup + Corruption Recovery
## S58: Steam Cloud save hooks (GodotSteam integration-ready)
extends Node

const PRODUCTION_SAVE_ROOT: String = "user://saves"
const TEST_SAVE_ROOT_PREFIX: String = "user://test_tmp/smoke_saves"
# Kept for existing callers and comments. Runtime path resolution uses
# _active_save_root so production and synthetic test storage cannot overlap.
const SAVE_DIR: String = PRODUCTION_SAVE_ROOT + "/"
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
var _checkpoint_scene: String = ""
var _pending_checkpoint_scene: String = ""
var _checkpoint_delay: float = 0.0

## S56: Last save timestamp (unix) for "Last Saved: X minutes ago"
var _last_save_time: float = 0.0

## S56: Save indicator UI
var _save_indicator: Control = null
var _save_indicator_tween: Tween = null

## Save-path dependency injection. Production retains the historical
## user://saves root. Smoke processes start with no usable root and must inject
## a descendant of TEST_SAVE_ROOT_PREFIX before any save read or write.
var _active_save_root: String = PRODUCTION_SAVE_ROOT
var _smoke_test_mode: bool = false
var _test_save_root_configured: bool = false
var _save_path_guard_failed: bool = false

func _ready() -> void:
	_smoke_test_mode = _detect_smoke_test_mode()
	if _smoke_test_mode:
		# A smoke process must not even create or probe the production directory.
		_active_save_root = ""
		_autosave_enabled = false
		print("[SaveManager] Smoke path guard active; test save root not configured")
	else:
		_active_save_root = PRODUCTION_SAVE_ROOT
		_ensure_save_root(_active_save_root)
	_build_save_indicator()
	# Connect to chapter transitions and boss battles for autosave
	_connect_autosave_signals()
	print("[SaveManager] Ready, save root: %s (autosave every %ds)" % [
		_active_save_root if not _smoke_test_mode else "<smoke-guarded>",
		int(AUTOSAVE_INTERVAL),
	])

func _process(delta: float) -> void:
	_update_map_checkpoint(delta)
	# S56: Autosave timer (only during exploration)
	if _autosave_enabled and GameManager.current_state == GameManager.GameState.EXPLORATION:
		_autosave_timer += delta
		if _autosave_timer >= AUTOSAVE_INTERVAL:
			_autosave_timer = 0.0
			autosave("timer")

## Every newly entered exploration map becomes a reliable retry point. The
## short delay lets map scripts restore the loaded position and story state
## before serialization. Returning from battle to the same map does not save.
func _update_map_checkpoint(delta: float) -> void:
	var scene_path := _get_current_scene_path()
	if scene_path == "res://scenes/main/main.tscn":
		_checkpoint_scene = ""
		_pending_checkpoint_scene = ""
		return
	if GameManager.current_state != GameManager.GameState.EXPLORATION or not scene_path.begins_with("res://scenes/maps/"):
		return
	if scene_path == _checkpoint_scene:
		_pending_checkpoint_scene = ""
		return
	if scene_path != _pending_checkpoint_scene:
		_pending_checkpoint_scene = scene_path
		_checkpoint_delay = 1.0
		return
	_checkpoint_delay -= delta
	if _checkpoint_delay > 0.0:
		return
	_checkpoint_scene = scene_path
	_pending_checkpoint_scene = ""
	autosave("map_checkpoint")

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
	# Headless smoke/capture scenes mark themselves as synthetic. Never let
	# developer validation overwrite a player's real autosave.
	if Codex.suppress_recording:
		return
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
		"world_state": _export_world_state_for_save(),
		"memory": MemoryManager.export_data(),
		"scene_flow": SceneFlow.export_data(),
		"elia_diary": EliaDiary.export_data(),
		"tutorial_hints": TutorialHints.export_data(),
		"player_pos": player_pos,
		"is_autosave": slot == AUTOSAVE_SLOT,
	}

	var path = _get_save_path(slot)
	if path == "":
		return false

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
	if path == "":
		return false
	if not FileAccess.file_exists(path):
		print("[SaveManager] No save in slot %d" % slot)
		return false

	var save_data = _load_json_with_recovery(path, slot)
	if save_data == null:
		return false

	# S72: Codex 살린 부분, 구버전 세이브 마이그레이션
	save_data = _migrate_save_data(save_data)

	# Validate the destination before importing mutable game state. A damaged or
	# obsolete path must not leave the current session only partially loaded.
	var scene_path: String = String(save_data.get("scene", ""))
	if scene_path.is_empty() or not ResourceLoader.exists(scene_path, "PackedScene"):
		var scene_error := "Save slot %d points to a missing scene: %s" % [slot, scene_path]
		push_warning("[SaveManager] %s" % scene_error)
		save_failed.emit(scene_error)
		NotificationToast.show_toast("Save could not be loaded: missing scene", NotificationToast.ToastType.WARNING)
		return false

	# 게임 데이터 복원
	if save_data.has("game"):
		GameManager.import_data(save_data.game)

	_restore_world_state_from_save_data(save_data)

	if save_data.has("memory"):
		MemoryManager.import_data(save_data.memory)

	if save_data.has("elia_diary"):
		EliaDiary.import_data(save_data.elia_diary)

	if save_data.has("tutorial_hints"):
		TutorialHints.import_data(save_data.tutorial_hints)

	# Restore VN identifiers/indices before changing scenes so VNHost can resume safely.
	var scene_flow_data: Dictionary = save_data.get("scene_flow", {})
	if scene_path == "res://scenes/main/vn_host.tscn":
		SceneFlow.prepare_resume_from_save(scene_flow_data)
	else:
		SceneFlow.import_data(scene_flow_data)

	# 플레이어 위치 복원 준비
	loaded_player_pos = save_data.get("player_pos", {})

	# Update last save time from loaded data
	_last_save_time = save_data.get("unix_time", Time.get_unix_time_from_system())

	# 씬 전환
	SceneTransition.change_scene_styled(scene_path)

	print("[SaveManager] Loaded slot %d (saved: %s)" % [slot, save_data.get("timestamp", "?")])
	load_completed.emit(slot)
	return true

## S56: Load JSON with corruption recovery, tries .bak if main fails
func _load_json_with_recovery(path: String, slot: int) -> Variant:
	if not _guard_save_path(path, "load_with_recovery"):
		return null
	# Try primary file first
	var data = _try_parse_json(path)
	if data != null:
		return data

	# Primary file failed, try backup
	var bak_path = path + ".bak"
	push_warning("[SaveManager] Primary save corrupted for slot %d, trying backup..." % slot)

	if FileAccess.file_exists(bak_path):
		data = _try_parse_json(bak_path)
		if data != null:
			# Recovery successful, notify player
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
	push_error("[SaveManager] Failed to load slot %d, both primary and backup corrupted" % slot)
	return null

## S56: Try to parse a JSON file, return null on failure
func _try_parse_json(path: String) -> Variant:
	if not _guard_save_path(path, "read_json"):
		return null
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

## S72: 구버전 세이브 호환, 누락 키 보강 + 버전 스탬프 갱신
const SAVE_VERSION: String = "0.4.0"

func _migrate_save_data(data: Dictionary) -> Dictionary:
	var migrated: Dictionary = data.duplicate(true)
	var version: String = str(migrated.get("version", "0.0.0"))

	# Always normalize the schema, including saves stamped with the current version.
	if not migrated.has("game") or not (migrated["game"] is Dictionary):
		migrated["game"] = {}
	if not migrated.has("memory") or not (migrated["memory"] is Dictionary):
		migrated["memory"] = {}
	migrated["world_state"] = _compatible_world_state_or_default(migrated.get("world_state", null))
	if not migrated.has("elia_diary") or not (migrated["elia_diary"] is Dictionary):
		migrated["elia_diary"] = {}
	if not migrated.has("tutorial_hints") or not (migrated["tutorial_hints"] is Dictionary):
		migrated["tutorial_hints"] = {}
	if not migrated.has("player_pos") or not (migrated["player_pos"] is Dictionary):
		migrated["player_pos"] = {}
	if not migrated.has("scene_flow") or not (migrated["scene_flow"] is Dictionary):
		migrated["scene_flow"] = {}
	if not migrated.has("scene"):
		migrated["scene"] = ""

	migrated["version"] = SAVE_VERSION
	if version != SAVE_VERSION:
		print("[SaveManager] Migrated save from %s to %s" % [version, SAVE_VERSION])
	return migrated


## Memory World Engine save boundary. Kept small so headless validation can
## exercise the same export/import path without changing a player's save slot.
func _export_world_state_for_save() -> Dictionary:
	return WorldState.export_data()


func _restore_world_state_from_save_data(save_data: Dictionary) -> bool:
	var world_data := _compatible_world_state_or_default(save_data.get("world_state", null))
	return WorldState.import_data(world_data)


## Current MVP has one supported WorldState schema and no prior world-state
## payload to transform. Missing, corrupt, or unknown schemas therefore recover
## to deterministic defaults without touching legacy story_flags or memory.
func _compatible_world_state_or_default(value: Variant) -> Dictionary:
	if value == null:
		return WorldState.make_default_data()
	if not (value is Dictionary):
		push_warning("[SaveManager] Corrupt WorldState payload; using schema v%d defaults" % WorldState.SCHEMA_VERSION)
		return WorldState.make_default_data()
	var snapshot: Dictionary = value
	if not WorldState.is_supported_snapshot(snapshot):
		push_warning("[SaveManager] Unsupported WorldState schema; using schema v%d defaults" % WorldState.SCHEMA_VERSION)
		return WorldState.make_default_data()
	return snapshot.duplicate(true)


## Injects a save root only for a smoke process. The target must be a strict
## descendant of user://test_tmp/smoke_saves after absolute-path normalization.
## Invalid targets trigger the fatal path guard before any filesystem access.
func configure_test_save_root(root: String) -> bool:
	if not _smoke_test_mode:
		push_warning("[SaveManager] Test save root injection rejected outside smoke mode")
		return false
	var normalized_root := _normalize_virtual_root(root)
	if not _is_descendant_path(normalized_root, TEST_SAVE_ROOT_PREFIX):
		_fail_save_path_guard("configure_test_root", root)
		return false
	_active_save_root = normalized_root
	_test_save_root_configured = true
	if not _ensure_save_root(_active_save_root):
		_fail_save_path_guard("create_test_root", _active_save_root)
		return false
	print("[SaveManager] Isolated test save root: %s" % _active_save_root)
	return true


func is_smoke_test_mode() -> bool:
	return _smoke_test_mode


func is_test_save_root_configured() -> bool:
	return _smoke_test_mode and _test_save_root_configured


func get_active_save_root() -> String:
	return _active_save_root


func get_save_path_for_test(slot: int) -> String:
	if not _smoke_test_mode:
		push_warning("[SaveManager] Test save path requested outside smoke mode")
		return ""
	return _get_save_path(slot)


## Every synthetic helper must call this immediately before a direct write.
## SaveManager's own slot operations pass through the same path validator.
func guard_test_write_target(target_path: String) -> bool:
	if not _smoke_test_mode or not _is_allowed_test_path(target_path):
		_fail_save_path_guard("test_write", target_path)
		return false
	return true


func has_save_path_guard_failed() -> bool:
	return _save_path_guard_failed


func _detect_smoke_test_mode() -> bool:
	if "--smoke-test" in OS.get_cmdline_user_args():
		return true
	# Protect direct `--scene res://scripts/tools/smoke_*.tscn` invocations even
	# when a developer forgets the explicit user argument.
	for argument in OS.get_cmdline_args():
		var normalized := String(argument).replace("\\", "/").to_lower()
		if "/scripts/tools/smoke_" in normalized or normalized.begins_with("res://scripts/tools/smoke_"):
			return true
	return false


func _ensure_save_root(root: String) -> bool:
	if root == "":
		return false
	if DirAccess.dir_exists_absolute(root):
		return true
	return DirAccess.make_dir_recursive_absolute(root) == OK


func _join_root(root: String, filename: String) -> String:
	if root == "":
		return filename
	return root.trim_suffix("/") + "/" + filename


func _normalize_virtual_root(path: String) -> String:
	return path.strip_edges().replace("\\", "/").simplify_path().trim_suffix("/")


func _normalized_absolute_path(path: String) -> String:
	var resolved := ProjectSettings.globalize_path(path)
	return resolved.replace("\\", "/").simplify_path().trim_suffix("/").to_lower()


func _is_descendant_path(candidate: String, root: String) -> bool:
	var candidate_absolute := _normalized_absolute_path(candidate)
	var root_absolute := _normalized_absolute_path(root)
	return candidate_absolute.begins_with(root_absolute + "/")


func _is_allowed_test_path(path: String) -> bool:
	if not _smoke_test_mode or not _test_save_root_configured or path == "":
		return false
	var resolved := _normalized_absolute_path(path)
	var production := _normalized_absolute_path(PRODUCTION_SAVE_ROOT)
	var active_root := _normalized_absolute_path(_active_save_root)
	var allowed_prefix := _normalized_absolute_path(TEST_SAVE_ROOT_PREFIX)
	if resolved == production or resolved.begins_with(production + "/"):
		return false
	return resolved.begins_with(active_root + "/") and active_root.begins_with(allowed_prefix + "/")


func _guard_save_path(path: String, operation: String) -> bool:
	if not _smoke_test_mode:
		return true
	if _is_allowed_test_path(path):
		return true
	_fail_save_path_guard(operation, path)
	return false


func _fail_save_path_guard(operation: String, target_path: String) -> void:
	_save_path_guard_failed = true
	var resolved := _normalized_absolute_path(target_path) if target_path != "" else "<empty>"
	push_error("[SAVE_PATH_GUARD][FATAL] operation=%s target=%s resolved=%s" % [
		operation,
		target_path if target_path != "" else "<empty>",
		resolved,
	])
	if is_inside_tree():
		get_tree().quit(1)

## S56: Create .bak backup before overwriting
func _create_backup(path: String) -> void:
	if not _guard_save_path(path, "backup"):
		return
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
	var filename := "autosave.json" if slot == AUTOSAVE_SLOT else "save_%d.json" % slot
	var path := _join_root(_active_save_root, filename)
	if not _guard_save_path(path, "resolve_slot_%d" % slot):
		return ""
	return path

## 슬롯에 세이브가 있는지 확인
func has_save(slot: int) -> bool:
	var path := _get_save_path(slot)
	return path != "" and FileAccess.file_exists(path)

## 세이브 정보 가져오기 (슬롯 선택 UI용)
func get_save_info(slot: int) -> Dictionary:
	var path = _get_save_path(slot)
	if path == "":
		return {}
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
	var raw_flow_data: Variant = data.get("scene_flow", {})
	var flow_data: Dictionary = raw_flow_data if raw_flow_data is Dictionary else {}
	var vn_scene_id := ""
	var vn_step := 0
	if scene_path == "res://scenes/main/vn_host.tscn":
		if bool(flow_data.get("is_active", false)):
			vn_scene_id = String(flow_data.get("current_id", ""))
			vn_step = maxi(0, int(flow_data.get("current_index", 0)))
		else:
			vn_scene_id = String(flow_data.get("pending_scene_id", ""))
			vn_step = maxi(0, int(flow_data.get("pending_start_index", 0)))
		if vn_scene_id != "":
			location = vn_scene_id.replace("_", " ").capitalize()
	var hp_val: int = game_data.get("player_data", {}).get("hp", 0)
	var max_hp_val: int = game_data.get("player_data", {}).get("max_hp", 100)
	var grains_val: int = game_data.get("player_data", {}).get("grains", 0)
	return {
		"timestamp": data.get("timestamp", ""),
		"unix_time": data.get("unix_time", 0.0),
		"chapter": game_data.get("current_chapter", 1),
		"burn_count": mem_data.get("burned", []).size(),
		"location": location,
		"vn_scene_id": vn_scene_id,
		"vn_step": vn_step,
		"hp": hp_val,
		"max_hp": max_hp_val,
		"grains": grains_val,
		"equipped": game_data.get("equipped", {}),
		"is_autosave": data.get("is_autosave", false),
	}

## S56: Get "Last Saved: X minutes ago" text
func get_last_saved_text() -> String:
	# S242: 일시정지 화면에 그대로 나오는 문자열인데 한국어 로케일에서도 영어였다.
	var is_ko := GameManager.current_locale == "ko"
	if _last_save_time <= 0.0:
		return "아직 저장하지 않음" if is_ko else "Not saved yet"
	var elapsed = Time.get_unix_time_from_system() - _last_save_time
	if elapsed < 60:
		return "마지막 저장: 방금" if is_ko else "Last saved: just now"
	elif elapsed < 3600:
		var mins = int(elapsed / 60)
		return ("마지막 저장: %d분 전" if is_ko else "Last saved: %d min ago") % mins
	else:
		var hours = int(elapsed / 3600)
		return ("마지막 저장: %d시간 전" if is_ko else "Last saved: %dh ago") % hours

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
	icon_label.add_theme_font_size_override("font_size", 13)
	icon_label.add_theme_color_override("font_color", Color(0.5, 0.75, 0.4))
	hbox.add_child(icon_label)

	var text_label = Label.new()
	text_label.text = "Saving..."
	text_label.add_theme_font_size_override("font_size", 13)
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

	print("[SaveManager] Cloud save stub, local save only (slot %d)" % slot)
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

	print("[SaveManager] Cloud load stub, local load only (slot %d)" % slot)
	return load_game(slot)

## Get cloud save info (for save slot UI, show cloud icon if synced).
func has_cloud_save(slot: int) -> bool:
	if not is_cloud_available():
		return false
	# --- GodotSteam Integration Point ---
	# var cloud_filename = "memoria_save_%d.json" % slot
	# return Steam.fileExists(cloud_filename)
	return false
