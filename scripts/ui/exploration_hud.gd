## ExplorationHUD (Autoload)
## 탐색 중 좌상단에 HP/챕터/기억 정보를 표시하는 HUD.
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
var hp_value_label: Label
var chapter_label: Label
var memory_label: Label
var update_timer: Timer
var hp_tween: Tween
var _last_hp: int = -1

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

	hp_bar = ProgressBar.new()
	hp_bar.custom_minimum_size = Vector2(100, 12)
	hp_bar.max_value = 100
	hp_bar.value = 100
	hp_bar.show_percentage = false
	# Fill style
	var fill_style := StyleBoxFlat.new()
	fill_style.bg_color = UITheme.HP_PLAYER
	fill_style.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("fill", fill_style)
	# Background style
	var bg_style := StyleBoxFlat.new()
	bg_style.bg_color = Color(0.08, 0.07, 0.1, 0.9)
	bg_style.set_corner_radius_all(2)
	hp_bar.add_theme_stylebox_override("background", bg_style)
	hp_row.add_child(hp_bar)

	hp_value_label = Label.new()
	hp_value_label.add_theme_font_size_override("font_size", 12)
	hp_value_label.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY)
	hp_row.add_child(hp_value_label)

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

func _start_timer() -> void:
	update_timer = Timer.new()
	update_timer.wait_time = 0.5
	update_timer.autostart = true
	update_timer.timeout.connect(_update_hud)
	add_child(update_timer)

func _connect_signals() -> void:
	GameManager.state_changed.connect(_on_state_changed)
	# Set initial visibility based on current state
	_on_state_changed(GameManager.current_state)

# ── 상태 변경 시 표시/숨김 ──
func _on_state_changed(new_state: GameManager.GameState) -> void:
	panel.visible = (new_state == GameManager.GameState.EXPLORATION)

# ── HUD 갱신 ──
func _update_hud() -> void:
	if not panel.visible:
		return

	var pd: Dictionary = GameManager.player_data
	var hp: int = pd.get("hp", 0)
	var max_hp: int = pd.get("max_hp", 100)

	# HP bar — smooth tween animation
	hp_bar.max_value = max_hp
	if hp != _last_hp:
		_last_hp = hp
		if hp_tween:
			hp_tween.kill()
		hp_tween = create_tween().set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		hp_tween.tween_property(hp_bar, "value", float(hp), 0.4)

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
	if location_name.is_empty():
		chapter_label.text = "Ch.%d" % chapter_num
	else:
		chapter_label.text = "Ch.%d — %s" % [chapter_num, location_name]

	# Memories
	var held: int = MemoryManager.memories.size()
	var burned: int = MemoryManager.burned_memories.size()
	memory_label.text = "Memories: %d held, %d burned" % [held, burned]

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
