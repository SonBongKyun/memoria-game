## ExplorationHUD (Autoload)
## 탐색 중 좌상단에 HP/챕터/기억 정보를 표시하는 HUD.
## S57: Steam-quality upgrade — ghost HP bar, status icons, quest progress bar,
##      memory burn glow, grains popup, slide-in animation.
extends CanvasLayer

# ── 맵 이름 매핑 ──
const MAP_NAMES := {
	"rim_forest": "Rim Forest",
	"verdan_market": "Verdan Market",
	"crumbling_coast": "Crumbling Coast",
	"the_seam": "The Seam",
	"bl07_void": "BL-07 Void",
}

# ── 노드 참조 ──
var panel: PanelContainer
var hp_label: Label
var hp_bar: ProgressBar
var hp_ghost_bar: ProgressBar  # S57: ghost drain bar
var hp_value_label: Label
var chapter_label: Label
var memory_label: Label
var grains_label: Label
var items_label: Label
var equip_label: Label    # S41
var quest_label: Label    # S41
var quest_progress_bar: ProgressBar  # S57: visual quest progress
var status_icons_row: HBoxContainer  # S57: status effect icons
var update_timer: Timer
var hp_tween: Tween
var hp_ghost_tween: Tween  # S57
var _last_hp: int = -1
var _last_grains: int = -1  # S57: track grains for popup
var _last_burned: int = -1  # S57: track burned count for glow
var _memory_glow_tween: Tween  # S57
var _slide_in_done: bool = false  # S57

func _ready() -> void:
	layer = 10
	_build_ui()
	_start_timer()
	_connect_signals()
	_update_hud()
	print("[ExplorationHUD] Ready")

# ── UI 구성 ──
func _build_ui() -> void:
	panel = PanelContainer.new()
	panel.position = Vector2(12, 12)
	panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.04, 0.03, 0.06, 0.75),  # semi-transparent dark bg
		UITheme.BORDER,                   # subtle amber border
		1,                                # thin border
		4,                                # corner radius
		8                                 # content margin
	))
	add_child(panel)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 3)
	panel.add_child(vbox)

	# ── HP Row ──
	var hp_row := HBoxContainer.new()
	hp_row.add_theme_constant_override("separation", 6)
	vbox.add_child(hp_row)

	hp_label = Label.new()
	hp_label.text = "HP:"
	hp_label.add_theme_font_size_override("font_size", 12)
	hp_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	hp_row.add_child(hp_label)

	# S57: Stacked HP bars — ghost underneath, real on top
	var hp_stack := Control.new()
	hp_stack.custom_minimum_size = Vector2(100, 12)
	hp_row.add_child(hp_stack)

	# Ghost bar (lighter, trails behind)
	hp_ghost_bar = ProgressBar.new()
	hp_ghost_bar.custom_minimum_size = Vector2(100, 12)
	hp_ghost_bar.position = Vector2.ZERO
	hp_ghost_bar.size = Vector2(100, 12)
	hp_ghost_bar.max_value = 100
	hp_ghost_bar.value = 100
	hp_ghost_bar.show_percentage = false
	var ghost_fill := StyleBoxFlat.new()
	ghost_fill.bg_color = Color(0.85, 0.35, 0.3, 0.6)  # lighter red/orange ghost
	ghost_fill.set_corner_radius_all(2)
	hp_ghost_bar.add_theme_stylebox_override("fill", ghost_fill)
	var ghost_bg := StyleBoxFlat.new()
	ghost_bg.bg_color = Color(0.08, 0.07, 0.1, 0.9)
	ghost_bg.set_corner_radius_all(2)
	hp_ghost_bar.add_theme_stylebox_override("background", ghost_bg)
	hp_stack.add_child(hp_ghost_bar)

	# Real HP bar (on top)
	hp_bar = ProgressBar.new()
	hp_bar.custom_minimum_size = Vector2(100, 12)
	hp_bar.position = Vector2.ZERO
	hp_bar.size = Vector2(100, 12)
	hp_bar.max_value = 100
	hp_bar.value = 100
	hp_bar.show_percentage = false
	var fill_style := StyleBoxFlat.new()
	fill_style.bg_color = UITheme.HP_PLAYER
	fill_style.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("fill", fill_style)
	# Transparent background so ghost shows through
	var real_bg := StyleBoxFlat.new()
	real_bg.bg_color = Color(0, 0, 0, 0)
	real_bg.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("background", real_bg)
	hp_stack.add_child(hp_bar)

	hp_value_label = Label.new()
	hp_value_label.add_theme_font_size_override("font_size", 12)
	hp_value_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	hp_row.add_child(hp_value_label)

	# S57: Status effect icons row (next to HP row)
	status_icons_row = HBoxContainer.new()
	status_icons_row.add_theme_constant_override("separation", 2)
	hp_row.add_child(status_icons_row)

	# ── Chapter Row ──
	chapter_label = Label.new()
	chapter_label.add_theme_font_size_override("font_size", 12)
	chapter_label.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
	vbox.add_child(chapter_label)

	# ── Memory Row ──
	memory_label = Label.new()
	memory_label.add_theme_font_size_override("font_size", 12)
	memory_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	vbox.add_child(memory_label)

	# ── Grains Row ──
	grains_label = Label.new()
	grains_label.add_theme_font_size_override("font_size", 12)
	grains_label.add_theme_color_override("font_color", Color(0.85, 0.7, 0.35))
	vbox.add_child(grains_label)

	# ── Items Row ──
	items_label = Label.new()
	items_label.add_theme_font_size_override("font_size", 12)
	items_label.add_theme_color_override("font_color", Color(0.55, 0.75, 0.55))
	vbox.add_child(items_label)

	# ── S41: Equipment Row ──
	equip_label = Label.new()
	equip_label.add_theme_font_size_override("font_size", 11)
	equip_label.add_theme_color_override("font_color", Color(0.65, 0.55, 0.8))
	vbox.add_child(equip_label)

	# ── S41/S57: Active Quest Tracker with progress bar ──
	quest_label = Label.new()
	quest_label.add_theme_font_size_override("font_size", 11)
	quest_label.add_theme_color_override("font_color", Color(0.7, 0.6, 0.4))
	quest_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	quest_label.custom_minimum_size.x = 180
	vbox.add_child(quest_label)

	# S57: Quest progress bar
	quest_progress_bar = ProgressBar.new()
	quest_progress_bar.custom_minimum_size = Vector2(160, 6)
	quest_progress_bar.max_value = 1
	quest_progress_bar.value = 0
	quest_progress_bar.show_percentage = false
	quest_progress_bar.visible = false
	var quest_fill := StyleBoxFlat.new()
	quest_fill.bg_color = Color(0.7, 0.6, 0.35, 0.9)
	quest_fill.set_corner_radius_all(2)
	quest_progress_bar.add_theme_stylebox_override("fill", quest_fill)
	var quest_bg_style := StyleBoxFlat.new()
	quest_bg_style.bg_color = Color(0.12, 0.1, 0.08, 0.7)
	quest_bg_style.set_corner_radius_all(2)
	quest_progress_bar.add_theme_stylebox_override("background", quest_bg_style)
	vbox.add_child(quest_progress_bar)

func _start_timer() -> void:
	update_timer = Timer.new()
	update_timer.wait_time = 0.5
	update_timer.autostart = true
	update_timer.timeout.connect(_update_hud)
	add_child(update_timer)

func _connect_signals() -> void:
	GameManager.state_changed.connect(_on_state_changed)
	# S57: Listen for memory burned to trigger glow
	if MemoryManager.has_signal("memory_burned"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	# S58: Stat gain popup
	GameManager.stat_gained.connect(_on_stat_gained)
	# Set initial visibility based on current state
	_on_state_changed(GameManager.current_state)

# ── 상태 변경 시 표시/숨김 ──
func _on_state_changed(new_state: GameManager.GameState) -> void:
	var should_show = (new_state == GameManager.GameState.EXPLORATION)
	panel.visible = should_show
	# S57: Slide-in animation when entering exploration
	if should_show and not _slide_in_done:
		_play_slide_in()
		_slide_in_done = true
	elif not should_show:
		_slide_in_done = false

# ── S57: Slide-in animation ──
func _play_slide_in() -> void:
	# Temporarily make all children invisible, then animate them in
	var children_to_animate = []
	var vbox = panel.get_child(0) as VBoxContainer
	if not vbox:
		return
	# Save original positions and animate
	panel.modulate.a = 0.0
	var t = create_tween()
	t.tween_property(panel, "modulate:a", 1.0, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	# HP slides from left
	var orig_pos = panel.position
	panel.position.x = orig_pos.x - 80
	t.parallel().tween_property(panel, "position:x", orig_pos.x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

# ── S57: Memory burn glow effect ──
func _on_memory_burned(_memory) -> void:
	if _memory_glow_tween and _memory_glow_tween.is_valid():
		_memory_glow_tween.kill()
	# Flash memory label gold
	memory_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
	_memory_glow_tween = create_tween()
	_memory_glow_tween.tween_property(memory_label, "theme_override_colors/font_color", UITheme.TEXT_DIM, 0.5).set_ease(Tween.EASE_OUT)

# ── S57: Grains earned popup ──
func _show_grains_popup(amount: int) -> void:
	if amount <= 0:
		return
	var popup = Label.new()
	popup.text = "+%d Grains" % amount
	popup.add_theme_font_size_override("font_size", 14)
	popup.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4))
	popup.add_theme_color_override("font_outline_color", Color(0.15, 0.1, 0.05))
	popup.add_theme_constant_override("outline_size", 2)
	# Position near grains label
	popup.position = grains_label.global_position + Vector2(grains_label.size.x + 8, -4)
	popup.z_index = 100
	add_child(popup)

	var t = create_tween()
	t.tween_property(popup, "position:y", popup.position.y - 30, 0.8).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.parallel().tween_property(popup, "modulate:a", 0.0, 0.8).set_ease(Tween.EASE_IN).set_delay(0.3)
	t.tween_callback(popup.queue_free)

# ── S58: Stat gain floating popup ──
func _on_stat_gained(stat_name: String, amount: int) -> void:
	if amount == 0:
		return
	var prefix = "+" if amount > 0 else ""
	var popup = Label.new()
	popup.text = "%s%d %s" % [prefix, amount, stat_name]
	popup.add_theme_font_size_override("font_size", 16)
	var col = Color(0.3, 1.0, 0.5) if amount > 0 else Color(1.0, 0.4, 0.3)
	popup.add_theme_color_override("font_color", col)
	popup.add_theme_color_override("font_outline_color", Color(0.05, 0.05, 0.05))
	popup.add_theme_constant_override("outline_size", 3)
	# Position near the HUD panel, offset right
	popup.position = Vector2(panel.position.x + panel.size.x + 16, panel.position.y + 8)
	popup.z_index = 120
	popup.modulate.a = 0.0
	add_child(popup)
	# Animate: fade in, float up, fade out
	var t = create_tween()
	t.tween_property(popup, "modulate:a", 1.0, 0.15)
	t.tween_property(popup, "position:y", popup.position.y - 40, 1.0).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	t.parallel().tween_property(popup, "modulate:a", 0.0, 0.5).set_ease(Tween.EASE_IN).set_delay(0.6)
	t.tween_callback(popup.queue_free)
	# Play a subtle SFX
	AudioManager.play_sfx("ui_select")

# ── S57: Update status effect icons ──
func _update_status_icons() -> void:
	# Clear existing icons
	for child in status_icons_row.get_children():
		child.queue_free()

	# Only show if in battle-related context or status persists
	# Check BattleManager player_statuses (may be empty outside battle)
	if not BattleManager or BattleManager.player_statuses.is_empty():
		return

	var status_colors := {
		"poison": Color(0.3, 0.8, 0.2),
		"burn": Color(0.9, 0.4, 0.1),
		"weaken": Color(0.6, 0.3, 0.7),
		"stun": Color(0.9, 0.9, 0.3),
	}

	for entry in BattleManager.player_statuses:
		var status_type: String = ""
		if entry is Dictionary:
			status_type = entry.get("type", "")
		elif "type" in entry:
			status_type = entry.type

		if status_type == "":
			continue

		var icon = ColorRect.new()
		icon.custom_minimum_size = Vector2(8, 8)
		icon.size = Vector2(8, 8)
		icon.color = status_colors.get(status_type, Color(0.5, 0.5, 0.5))
		status_icons_row.add_child(icon)

		# Pulse animation
		var pulse_tween = create_tween().set_loops()
		pulse_tween.tween_property(icon, "modulate:a", 0.4, 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
		pulse_tween.tween_property(icon, "modulate:a", 1.0, 0.6).set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)

# ── HUD 갱신 ──
func _update_hud() -> void:
	if not panel.visible:
		return

	var pd: Dictionary = GameManager.player_data
	var hp: int = pd.get("hp", 0)
	var max_hp: int = pd.get("max_hp", 100)

	# HP bar — S57: ghost drain effect
	hp_bar.max_value = max_hp
	hp_ghost_bar.max_value = max_hp
	if hp != _last_hp:
		var prev_hp = _last_hp
		_last_hp = hp

		# Real HP drops instantly
		if hp_tween and hp_tween.is_valid():
			hp_tween.kill()
		hp_tween = create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		hp_tween.tween_property(hp_bar, "value", float(hp), 0.15)

		# Ghost bar follows slowly (drain effect)
		if hp_ghost_tween and hp_ghost_tween.is_valid():
			hp_ghost_tween.kill()
		hp_ghost_tween = create_tween().set_ease(Tween.EASE_IN_OUT).set_trans(Tween.TRANS_SINE)
		hp_ghost_tween.tween_property(hp_ghost_bar, "value", float(hp), 0.5).set_delay(0.15)

		# Update fill color based on HP threshold
		var fill: StyleBoxFlat = hp_bar.get_theme_stylebox("fill")
		if float(hp) / float(max_hp) <= 0.25:
			fill.bg_color = UITheme.HP_LOW
		else:
			fill.bg_color = UITheme.HP_PLAYER

	hp_value_label.text = "%d/%d" % [hp, max_hp]

	# Chapter & location
	var chapter_num: int = GameManager.current_chapter
	var location_name: String = _get_location_name()
	var ng_suffix = " (NG+%d)" % GameManager.ng_plus_cycle if GameManager.ng_plus_cycle > 0 else ""
	if location_name.is_empty():
		chapter_label.text = "Ch.%d%s" % [chapter_num, ng_suffix]
	else:
		chapter_label.text = "Ch.%d — %s%s" % [chapter_num, location_name, ng_suffix]

	# Memories
	var held: int = MemoryManager.memories.size()
	var burned: int = MemoryManager.burned_memories.size()
	memory_label.text = "Memories: %d held, %d burned" % [held, burned]

	# S57: Memory burn counter glow (detect change via burned count)
	if _last_burned >= 0 and burned > _last_burned:
		_on_memory_burned(null)
	_last_burned = burned

	# Grains
	var grains: int = GameManager.player_data.get("grains", 0)
	grains_label.text = "Grains: %d" % grains
	# S57: Grains earned popup
	if _last_grains >= 0 and grains > _last_grains:
		_show_grains_popup(grains - _last_grains)
	_last_grains = grains

	# Items
	var total_items: int = 0
	var items_dict: Dictionary = GameManager.player_data.get("items", {})
	for item_id in items_dict:
		total_items += items_dict[item_id]
	items_label.text = "Items: %d" % total_items

	# S41: Equipment summary
	var weapon_name = ""
	var wid = GameManager.equipped.get("weapon", "")
	if wid != "" and GameManager.EQUIPMENT.has(wid):
		weapon_name = GameManager.EQUIPMENT[wid].name
	equip_label.text = "Weapon: %s" % weapon_name if weapon_name != "" else ""
	equip_label.visible = weapon_name != ""

	# S41/S57: Active quest tracker with progress bar
	_update_quest_tracker()

	# S57: Status effect icons
	_update_status_icons()

## S41/S57: 활성 퀘스트 트래커 with progress bar
func _update_quest_tracker() -> void:
	var active_quest = ""
	var quest_step: int = 0
	var quest_total: int = 1

	# SideQuest에서 활성 퀘스트 검색
	var all_quests = SideQuest.get_all_quests()
	for q in all_quests:
		var qid = q.get("id", "")
		if SideQuest.is_active(qid):
			active_quest = q.get("name", "")
			quest_step = SideQuest.get_current_step(qid)
			var quest_data = q
			# Get total steps from quest steps array
			var steps = q.get("steps", [])
			quest_total = max(steps.size(), 1) if steps is Array else 1
			break

	# 스토리 기반 힌트
	if active_quest == "":
		var ch = GameManager.current_chapter
		if ch == 1 and not GameManager.get_flag("met_elia"):
			active_quest = "Find Elia in the forest"
			quest_step = 0; quest_total = 1
		elif ch == 2 and not GameManager.get_flag("malet_deal"):
			active_quest = "Meet Malet at the market"
			quest_step = 0; quest_total = 1
		elif ch == 3 and not GameManager.get_flag("reached_seam"):
			active_quest = "Reach The Seam"
			quest_step = 0; quest_total = 1
		elif ch == 4 and not GameManager.get_flag("shade_sentinel_defeated"):
			active_quest = "Defeat the Shade Sentinel"
			quest_step = 0; quest_total = 1

	if active_quest != "":
		quest_label.text = "> %s  (%d/%d)" % [active_quest, quest_step, quest_total]
		quest_label.visible = true
		# S57: Progress bar
		quest_progress_bar.max_value = quest_total
		quest_progress_bar.value = quest_step
		quest_progress_bar.visible = true
	else:
		quest_label.visible = false
		quest_progress_bar.visible = false

## 현재 맵 이름 가져오기
func _get_location_name() -> String:
	var scene := get_tree().current_scene
	if scene == null:
		return ""
	var scene_name: String = scene.name.to_lower()
	# Try direct match first
	if MAP_NAMES.has(scene_name):
		return MAP_NAMES[scene_name]
	# Try matching against scene file path
	if scene.scene_file_path:
		var file_name: String = scene.scene_file_path.get_file().get_basename().to_lower()
		if MAP_NAMES.has(file_name):
			return MAP_NAMES[file_name]
	return ""
