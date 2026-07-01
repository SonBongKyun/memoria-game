## AchievementManager (Autoload)
## 업적 시스템. 도전과제 추적 + 달성 시 Steam-style 팝업 알림.
## 영구 저장 (user://achievements.json)
## S56: Steam-ready achievement IDs, popup notification, completion stats
extends Node

const SAVE_PATH: String = "user://achievements.json"

# ── 업적 정의 (S56: steam_id 추가) ──
const ACHIEVEMENTS: Dictionary = {
	# 전투 관련
	"first_blood": {"title": "First Blood", "desc": "Win your first battle.", "icon": "sword", "steam_id": "ACH_FIRST_BLOOD"},
	"void_slayer": {"title": "Void Slayer", "desc": "Defeat a Void Beast.", "icon": "skull", "steam_id": "ACH_VOID_SLAYER"},
	"boss_hunter": {"title": "Boss Hunter", "desc": "Defeat a boss enemy.", "icon": "crown", "steam_id": "ACH_BOSS_HUNTER"},
	"battle_veteran": {"title": "Battle Veteran", "desc": "Win 10 battles.", "icon": "shield", "steam_id": "ACH_BATTLE_VETERAN"},
	"survivor": {"title": "Survivor", "desc": "Win a battle with 10 HP or less.", "icon": "heart", "steam_id": "ACH_SURVIVOR"},
	"item_master": {"title": "Item Master", "desc": "Use 10 items in battle.", "icon": "potion", "steam_id": "ACH_ITEM_MASTER"},
	"perfect_tactics": {"title": "Field Tactician", "desc": "Complete a tactical objective in battle.", "icon": "shield", "steam_id": "ACH_FIELD_TACTICIAN"},
	"resonance_master": {"title": "Overbright", "desc": "Reach high combat resonance in a battle.", "icon": "flame", "steam_id": "ACH_RESONANCE_MASTER"},

	# 기억 관련
	"first_burn": {"title": "First Burn", "desc": "Burn your first memory.", "icon": "flame", "steam_id": "ACH_FIRST_BURN"},
	"pyromaniac": {"title": "Pyromaniac", "desc": "Burn 5 memories.", "icon": "flame", "steam_id": "ACH_PYROMANIAC"},
	"identity_crisis": {"title": "Identity Crisis", "desc": "Burn a Grade 2 (Identity) memory.", "icon": "flame", "steam_id": "ACH_IDENTITY_CRISIS"},
	"zero_burn": {"title": "Zero Burn", "desc": "Burn the Core memory — your name.", "icon": "skull", "steam_id": "ACH_ZERO_BURN"},

	# 탐색 관련
	"hidden_stump": {"title": "Old Growth", "desc": "Find the hidden stump in Rim Forest.", "icon": "eye", "steam_id": "ACH_OLD_GROWTH"},
	"hidden_garden": {"title": "Secret Garden", "desc": "Find the hidden garden in The Seam.", "icon": "eye", "steam_id": "ACH_SECRET_GARDEN"},
	"explorer": {"title": "Explorer", "desc": "Visit all 5 maps.", "icon": "map", "steam_id": "ACH_EXPLORER"},

	# 스토리 관련
	"chapter_complete_1": {"title": "Rim Forest", "desc": "Complete Chapter 1.", "icon": "book", "steam_id": "ACH_CH1"},
	"chapter_complete_2": {"title": "Verdan Market", "desc": "Complete Chapter 2.", "icon": "book", "steam_id": "ACH_CH2"},
	"chapter_complete_3": {"title": "Weight of Pages", "desc": "Complete Chapter 3.", "icon": "book", "steam_id": "ACH_CH3"},
	"chapter_complete_4": {"title": "Drift", "desc": "Complete Chapter 4.", "icon": "book", "steam_id": "ACH_CH4"},
	"chapter_complete_5": {"title": "The Classifier", "desc": "Complete Chapter 5.", "icon": "book", "steam_id": "ACH_CH5"},
	"chapter_complete_6": {"title": "Thread That Holds", "desc": "Complete Chapter 6.", "icon": "book", "steam_id": "ACH_CH6"},
	"chapter_complete_7": {"title": "The Threshold", "desc": "Complete Chapter 7.", "icon": "book", "steam_id": "ACH_CH7"},
	"chapter_complete_8": {"title": "Forest That Forgets", "desc": "Complete Chapter 8.", "icon": "book", "steam_id": "ACH_CH8"},
	"chapter_complete_9": {"title": "Where Colors Stop", "desc": "Complete Chapter 9.", "icon": "book", "steam_id": "ACH_CH9"},
	"chapter_complete_10": {"title": "Into the Void", "desc": "Complete Chapter 10.", "icon": "book", "steam_id": "ACH_CH10"},
	"ending_seal": {"title": "The Seal Holds", "desc": "Reach the Seal ending.", "icon": "star", "steam_id": "ACH_ENDING_SEAL"},
	"ending_zero": {"title": "Nothing Remains", "desc": "Reach the Zero Burn ending.", "icon": "star", "steam_id": "ACH_ENDING_ZERO"},
	"ending_ash": {"title": "Ash Ending", "desc": "Reach the Ash ending.", "icon": "star", "steam_id": "ACH_ENDING_ASH"},
	"ending_seam": {"title": "The Seam Holds", "desc": "Reach the Seam ending.", "icon": "star", "steam_id": "ACH_ENDING_SEAM"},
	"ending_preservation": {"title": "Preservation", "desc": "Keep your name and continue the search.", "icon": "star", "steam_id": "ACH_ENDING_PRESERVATION"},
	"ending_tobias": {"title": "The Record Remains", "desc": "Help Tobias carry the record beyond Authority control.", "icon": "star", "steam_id": "ACH_ENDING_TOBIAS"},
	"ending_hollow": {"title": "Hollow", "desc": "Reach the Hollow ending.", "icon": "star", "steam_id": "ACH_ENDING_HOLLOW"},
	"ending_weave": {"title": "The Weave", "desc": "Reach the Weave ending — seal BL-07 without burning your name.", "icon": "crown", "steam_id": "ACH_ENDING_WEAVE"},
	"all_endings": {"title": "Every Path", "desc": "See all 7 endings.", "icon": "crown", "steam_id": "ACH_ALL_ENDINGS"},

	# 경제 관련
	"merchant": {"title": "Merchant", "desc": "Complete a trade with Malet.", "icon": "coin", "steam_id": "ACH_MERCHANT"},
	"wealthy": {"title": "Wealthy", "desc": "Accumulate 100 Grains.", "icon": "coin", "steam_id": "ACH_WEALTHY"},

	# 사이드 퀘스트
	"all_quests": {"title": "Memory Hunter", "desc": "Complete all side quests.", "icon": "star", "steam_id": "ACH_ALL_QUESTS"},

	# NG+
	"new_game_plus": {"title": "New Game+", "desc": "Start a New Game+ run.", "icon": "cycle", "steam_id": "ACH_NEW_GAME_PLUS"},
}

# ── 달성 데이터 ──
var unlocked: Dictionary = {}  # {id: true}
var stats: Dictionary = {      # 통계 카운터
	"battles_won": 0,
	"items_used": 0,
	"maps_visited": [],
}

signal achievement_unlocked(id: String)

# S56: Steam-style popup UI
var _popup_canvas: CanvasLayer
var _popup_panel: PanelContainer
var _popup_icon_label: Label
var _popup_title_label: Label
var _popup_desc_label: Label
var _popup_tween: Tween
var _popup_queue: Array[String] = []
var _popup_showing: bool = false

func _ready() -> void:
	_load_data()
	_connect_signals()
	_build_achievement_popup()
	print("[AchievementManager] Ready — %d/%d unlocked" % [unlocked.size(), ACHIEVEMENTS.size()])

func _connect_signals() -> void:
	BattleManager.battle_ended.connect(_on_battle_ended)
	MemoryManager.memory_burned.connect(_on_memory_burned)

## 업적 해금
func unlock(id: String) -> void:
	if unlocked.has(id):
		return
	if not ACHIEVEMENTS.has(id):
		return
	unlocked[id] = true
	var ach = ACHIEVEMENTS[id]

	# S56: Show Steam-style popup instead of generic toast
	_queue_achievement_popup(id)

	# S56: Call Steam API if available (placeholder for GodotSteam integration)
	_steam_set_achievement(ach.get("steam_id", ""))

	achievement_unlocked.emit(id)
	_save_data()
	print("[Achievement] Unlocked: %s (Steam ID: %s)" % [ach["title"], ach.get("steam_id", "N/A")])

	# all_endings 체크
	if id.begins_with("ending_"):
		_check_all_endings()

func is_unlocked(id: String) -> bool:
	return unlocked.has(id)

## S56: Steam API placeholder — will be replaced with actual GodotSteam calls
func _steam_set_achievement(steam_id: String) -> void:
	if steam_id == "":
		return
	# When GodotSteam is integrated, this becomes:
	# if Steam.isSteamRunning():
	#     Steam.setAchievement(steam_id)
	#     Steam.storeStats()
	print("[Steam] Would set achievement: %s" % steam_id)

## S56: Get completion percentage
func get_completion_percentage() -> float:
	if ACHIEVEMENTS.size() == 0:
		return 0.0
	return float(unlocked.size()) / float(ACHIEVEMENTS.size()) * 100.0

## ===================== S56: Steam-style Achievement Popup =====================

func _build_achievement_popup() -> void:
	_popup_canvas = CanvasLayer.new()
	_popup_canvas.layer = 95  # Above most UI
	add_child(_popup_canvas)

	_popup_panel = PanelContainer.new()
	_popup_panel.anchor_left = 1.0
	_popup_panel.anchor_right = 1.0
	_popup_panel.anchor_top = 0.0
	_popup_panel.anchor_bottom = 0.0
	_popup_panel.offset_left = -320
	_popup_panel.offset_right = -12
	_popup_panel.offset_top = -80  # Start above screen (hidden)
	_popup_panel.offset_bottom = -8

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.07, 0.06, 0.95)
	style.border_color = Color(0.7, 0.55, 0.25, 0.9)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(12)
	_popup_panel.add_theme_stylebox_override("panel", style)

	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 12)
	_popup_panel.add_child(hbox)

	# Achievement icon
	_popup_icon_label = Label.new()
	_popup_icon_label.add_theme_font_size_override("font_size", 28)
	_popup_icon_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_popup_icon_label.custom_minimum_size = Vector2(36, 0)
	hbox.add_child(_popup_icon_label)

	# Text area
	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 2)
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(vbox)

	# "ACHIEVEMENT UNLOCKED" header
	var header = Label.new()
	header.text = "ACHIEVEMENT UNLOCKED"
	header.add_theme_font_size_override("font_size", 10)
	header.add_theme_color_override("font_color", Color(0.6, 0.55, 0.4))
	vbox.add_child(header)

	# Achievement title
	_popup_title_label = Label.new()
	_popup_title_label.add_theme_font_size_override("font_size", 16)
	_popup_title_label.add_theme_color_override("font_color", Color(0.95, 0.85, 0.55))
	vbox.add_child(_popup_title_label)

	# Achievement description
	_popup_desc_label = Label.new()
	_popup_desc_label.add_theme_font_size_override("font_size", 11)
	_popup_desc_label.add_theme_color_override("font_color", Color(0.55, 0.5, 0.45))
	vbox.add_child(_popup_desc_label)

	_popup_canvas.add_child(_popup_panel)

func _queue_achievement_popup(id: String) -> void:
	_popup_queue.append(id)
	if not _popup_showing:
		_show_next_popup()

func _show_next_popup() -> void:
	if _popup_queue.is_empty():
		_popup_showing = false
		return
	_popup_showing = true
	var id = _popup_queue.pop_front()
	var ach = ACHIEVEMENTS.get(id, {})

	# Set content
	var icon_map = {"sword": "X", "skull": "X", "crown": "X", "shield": "X", "heart": "X", "potion": "X", "flame": "X", "eye": "X", "map": "X", "book": "X", "star": "X", "coin": "X", "cycle": "X"}
	_popup_icon_label.text = icon_map.get(ach.get("icon", ""), "X")
	_popup_title_label.text = ach.get("title", "???")
	_popup_desc_label.text = ach.get("desc", "")

	# Play SFX
	AudioManager.play_sfx("memory_add")

	# Controller vibration
	if InputManager:
		InputManager.vibrate("ui_confirm")

	# Animate: slide down from top
	if _popup_tween and _popup_tween.is_valid():
		_popup_tween.kill()
	_popup_panel.offset_top = -80
	_popup_panel.offset_bottom = -8
	_popup_panel.modulate.a = 0.0

	_popup_tween = create_tween()
	# Slide in
	_popup_tween.set_parallel(true)
	_popup_tween.tween_property(_popup_panel, "offset_top", 12.0, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	_popup_tween.tween_property(_popup_panel, "offset_bottom", 80.0, 0.4).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	_popup_tween.tween_property(_popup_panel, "modulate:a", 1.0, 0.3)
	_popup_tween.set_parallel(false)
	# Hold for 4 seconds
	_popup_tween.tween_interval(4.0)
	# Slide out
	_popup_tween.set_parallel(true)
	_popup_tween.tween_property(_popup_panel, "offset_top", -80.0, 0.3).set_ease(Tween.EASE_IN)
	_popup_tween.tween_property(_popup_panel, "offset_bottom", -8.0, 0.3).set_ease(Tween.EASE_IN)
	_popup_tween.tween_property(_popup_panel, "modulate:a", 0.0, 0.3)
	_popup_tween.set_parallel(false)
	# Next in queue
	_popup_tween.tween_callback(_show_next_popup)

## ===================== 이벤트 핸들러 =====================

func _on_battle_ended(result: BattleManager.BattleState) -> void:
	if result == BattleManager.BattleState.VICTORY:
		stats["battles_won"] += 1
		unlock("first_blood")
		if stats["battles_won"] >= 10:
			unlock("battle_veteran")
		# Void Beast / Boss 체크
		if BattleManager.current_enemy:
			if BattleManager.current_enemy.is_void_beast:
				unlock("void_slayer")
			if BattleManager.current_enemy.is_boss:
				unlock("boss_hunter")
		# 생존자 체크
		if GameManager.player_data.get("hp", 100) <= 10:
			unlock("survivor")
		_save_data()

func _on_memory_burned(memory) -> void:
	unlock("first_burn")
	if MemoryManager.get_burn_count() >= 5:
		unlock("pyromaniac")
	if memory.grade == MemoryManager.MemoryGrade.GRADE_2:
		unlock("identity_crisis")
	if memory.id == "core_name_origin":
		unlock("zero_burn")

## 맵 방문 기록 (맵 스크립트에서 호출)
func record_map_visit(map_name: String) -> void:
	if map_name not in stats["maps_visited"]:
		stats["maps_visited"].append(map_name)
		_save_data()
	if stats["maps_visited"].size() >= 5:
		unlock("explorer")

## 아이템 사용 기록 (BattleManager에서 호출)
func record_item_used() -> void:
	stats["items_used"] += 1
	if stats["items_used"] >= 10:
		unlock("item_master")
	_save_data()

## 챕터 완료 기록
func record_chapter_complete(chapter: int) -> void:
	var id = "chapter_complete_%d" % chapter
	if ACHIEVEMENTS.has(id):
		unlock(id)

## 엔딩 기록
func record_ending(ending_id: String) -> void:
	unlock(ending_id)

## Grains 체크
func check_grains() -> void:
	if GameManager.player_data.get("grains", 0) >= 100:
		unlock("wealthy")

func check_quest_complete() -> void:
	# 모든 사이드 퀘스트 완료 체크
	var all_done = true
	for q in SideQuest.QUESTS:
		if not SideQuest.is_complete(q["id"]):
			all_done = false
			break
	if all_done:
		unlock("all_quests")

func _check_all_endings() -> void:
	var endings = ["ending_zero", "ending_preservation", "ending_ash", "ending_seam", "ending_tobias", "ending_hollow", "ending_weave"]
	var all = true
	for e in endings:
		if not unlocked.has(e):
			all = false
			break
	if all:
		unlock("all_endings")

## ===================== 영구 저장 =====================

func _save_data() -> void:
	var data = {
		"unlocked": unlocked.duplicate(),
		"stats": stats.duplicate(true),
	}
	var file = FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(data, "\t"))
		file.close()

func _load_data() -> void:
	if not FileAccess.file_exists(SAVE_PATH):
		return
	var file = FileAccess.open(SAVE_PATH, FileAccess.READ)
	if file == null:
		return
	var json = JSON.new()
	if json.parse(file.get_as_text()) != OK:
		file.close()
		return
	file.close()
	var data = json.data
	if data is Dictionary:
		unlocked = data.get("unlocked", {})
		var loaded_stats = data.get("stats", {})
		for key in loaded_stats:
			stats[key] = loaded_stats[key]

## 업적 목록 반환 (UI용)
func get_all_achievements() -> Array[Dictionary]:
	var result: Array[Dictionary] = []
	for id in ACHIEVEMENTS:
		var ach = ACHIEVEMENTS[id].duplicate()
		ach["id"] = id
		ach["unlocked"] = unlocked.has(id)
		result.append(ach)
	return result

## S56: Get Steam achievement ID map (for Steamworks dashboard setup)
func get_steam_id_map() -> Dictionary:
	var result: Dictionary = {}
	for id in ACHIEVEMENTS:
		result[id] = ACHIEVEMENTS[id].get("steam_id", "")
	return result
