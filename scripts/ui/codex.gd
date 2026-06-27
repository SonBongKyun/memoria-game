## Codex (Autoload)
## 도감 시스템 — Bestiary (만난 적) + Memory Archive (수집한 기억)
## PauseMenu에서 접근. 영구 저장 (user://codex.json)
## S59: Visual overhaul — enemy sprite preview, star ratings, defeat badges, animated reveal
extends CanvasLayer

const SAVE_PATH: String = "user://codex.json"
const CODEX_BACKDROP_PATH: String = "res://assets/cg/generated/ui_codex_archive_backdrop.png"

var is_open: bool = false

# ── 도감 데이터 ──
var enemy_entries: Dictionary = {}   # {enemy_name: {encounters: N, defeated: N, is_void: bool, max_hp: int, atk: int}}
var memory_entries: Dictionary = {}  # {memory_id: {title, desc, grade, burned: bool}}

# ── UI 노드 ──
var backdrop: TextureRect
var overlay: ColorRect
var main_panel: PanelContainer
var tab_bestiary: Button
var tab_memories: Button
var item_scroll: ScrollContainer
var item_list: VBoxContainer
var detail_title: Label
var detail_body: RichTextLabel
var close_hint: Label
var archive_status: Label
var _current_tab: String = "bestiary"

# S59: Animated reveal tracking
var _revealed_entries: Dictionary = {}  # {entry_key: true} — already-seen entries
var _detail_vbox: VBoxContainer  # reference to detail panel vbox for sprite preview
var _enemy_preview: Control  # current enemy sprite preview node

signal codex_closed()

func _ready() -> void:
	layer = 58  # StoryJournal(57) 위
	_load_data()
	_build_ui()
	_hide_ui()
	process_mode = Node.PROCESS_MODE_ALWAYS
	# 전투 시그널 연결
	BattleManager.battle_started.connect(_on_battle_started)
	BattleManager.battle_ended.connect(_on_battle_ended)
	MemoryManager.memory_added.connect(_on_memory_added)
	MemoryManager.memory_burned.connect(_on_memory_burned)
	print("[Codex] Ready — %d enemies, %d memories" % [enemy_entries.size(), memory_entries.size()])

func _unhandled_input(event: InputEvent) -> void:
	if is_open and event.is_action_pressed("cancel"):
		close()
		get_viewport().set_input_as_handled()

## ===================== 데이터 기록 =====================

func _on_battle_started(enemy: BattleManager.Enemy) -> void:
	if not enemy_entries.has(enemy.name):
		enemy_entries[enemy.name] = {"encounters": 0, "defeated": 0, "is_void": enemy.is_void_beast, "max_hp": enemy.max_hp, "atk": enemy.attack, "is_boss": enemy.is_boss}
	enemy_entries[enemy.name]["encounters"] += 1
	# S59: Store enemy image path for bestiary preview
	var image_path: String = BattleManager.enemy_image
	if image_path == "":
		image_path = BattleManager.resolve_enemy_image_by_name(enemy.name)
	if image_path != "":
		enemy_entries[enemy.name]["image_path"] = image_path
	_save_data()

func _on_battle_ended(result: BattleManager.BattleState) -> void:
	if result == BattleManager.BattleState.VICTORY and BattleManager.current_enemy:
		var name = BattleManager.current_enemy.name
		if enemy_entries.has(name):
			enemy_entries[name]["defeated"] += 1
			_save_data()

func _on_memory_added(memory: MemoryManager.Memory) -> void:
	if not memory_entries.has(memory.id):
		memory_entries[memory.id] = {
			"title": memory.title,
			"desc": memory.description,
			"grade": memory.grade,
			"burned": false,
		}
		_save_data()

func _on_memory_burned(memory: MemoryManager.Memory) -> void:
	if memory_entries.has(memory.id):
		memory_entries[memory.id]["burned"] = true
		_save_data()

## ===================== 열기/닫기 =====================

func open() -> void:
	if is_open:
		return
	is_open = true
	AudioManager.play_sfx("ui_open")
	_current_tab = "bestiary"
	_refresh_list()
	_show_ui()

func close() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	_hide_ui()
	codex_closed.emit()

func _show_ui() -> void:
	backdrop.visible = true
	overlay.visible = true
	main_panel.visible = true
	main_panel.modulate.a = 0.0
	var resting_y := main_panel.position.y
	main_panel.position.y += 14.0
	var tween := create_tween().set_parallel(true)
	tween.tween_property(main_panel, "modulate:a", 1.0, 0.24).set_ease(Tween.EASE_OUT)
	tween.tween_property(main_panel, "position:y", resting_y, 0.32).set_trans(Tween.TRANS_QUART).set_ease(Tween.EASE_OUT)
	tween.tween_property(backdrop, "modulate:a", 1.0, 0.35).from(0.0)

func _hide_ui() -> void:
	if backdrop:
		backdrop.visible = false
	if overlay:
		overlay.visible = false
	if main_panel:
		main_panel.visible = false

## ===================== UI 구축 =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	backdrop = TextureRect.new()
	backdrop.set_anchors_preset(Control.PRESET_FULL_RECT)
	backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if ResourceLoader.exists(CODEX_BACKDROP_PATH):
		backdrop.texture = load(CODEX_BACKDROP_PATH)
	root.add_child(backdrop)

	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.015, 0.012, 0.025, 0.46)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.1
	main_panel.anchor_right = 0.9
	main_panel.anchor_top = 0.05
	main_panel.anchor_bottom = 0.95
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.035, 0.03, 0.05, 0.82), Color(0.5, 0.38, 0.22, 0.78), 2, 6, 18
	))
	root.add_child(main_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	main_panel.add_child(vbox)

	# 헤더
	var header = Label.new()
	header.text = "CODEX"
	header.add_theme_font_size_override("font_size", 24)
	header.add_theme_color_override("font_color", Color(0.8, 0.7, 0.5))
	header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	UITheme.apply_title_font(header)
	vbox.add_child(header)

	var subtitle := Label.new()
	subtitle.text = "조우한 존재와 잃어버린 기억의 기록" if GameManager.current_locale == "ko" else "Creatures encountered. Memories carried and spent."
	subtitle.add_theme_font_size_override("font_size", 12)
	subtitle.add_theme_color_override("font_color", Color(0.56, 0.52, 0.49))
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(subtitle)

	archive_status = Label.new()
	archive_status.add_theme_font_size_override("font_size", 11)
	archive_status.add_theme_color_override("font_color", Color(0.55, 0.62, 0.72))
	archive_status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(archive_status)

	# 탭 행
	var tab_row = HBoxContainer.new()
	tab_row.add_theme_constant_override("separation", 8)
	tab_row.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_child(tab_row)

	tab_bestiary = _make_tab_btn("Bestiary", "bestiary")
	tab_row.add_child(tab_bestiary)
	tab_memories = _make_tab_btn("Memory Archive", "memories")
	tab_row.add_child(tab_memories)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# 본문
	var content = HBoxContainer.new()
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_theme_constant_override("separation", 12)
	vbox.add_child(content)

	# 좌측: 스크롤 리스트
	item_scroll = ScrollContainer.new()
	item_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	content.add_child(item_scroll)

	item_list = VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 3)
	item_scroll.add_child(item_list)

	# 우측: 상세
	var detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(350, 0)
	detail_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.05, 0.04, 0.06, 0.8), UITheme.BORDER_DIM, 1, 4, 14
	))
	content.add_child(detail_panel)

	var dvbox = VBoxContainer.new()
	dvbox.add_theme_constant_override("separation", 8)
	detail_panel.add_child(dvbox)
	_detail_vbox = dvbox  # S59: store reference for sprite preview

	detail_title = Label.new()
	detail_title.text = "Select an entry..."
	detail_title.add_theme_font_size_override("font_size", 16)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.7, 0.55))
	dvbox.add_child(detail_title)

	var sep2 = HSeparator.new()
	dvbox.add_child(sep2)

	detail_body = RichTextLabel.new()
	detail_body.bbcode_enabled = false
	detail_body.fit_content = true
	detail_body.scroll_active = true
	detail_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	detail_body.add_theme_font_size_override("normal_font_size", 13)
	detail_body.add_theme_color_override("default_color", Color(0.65, 0.6, 0.55))
	dvbox.add_child(detail_body)

	# 닫기 힌트
	close_hint = Label.new()
	close_hint.text = "[ESC] Close"
	close_hint.add_theme_font_size_override("font_size", 11)
	close_hint.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	close_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(close_hint)

func _make_tab_btn(text: String, tab_id: String) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.custom_minimum_size = Vector2(150, 30)
	var style = UITheme.make_button_style()
	btn.add_theme_stylebox_override("normal", style)
	btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(style))
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	btn.pressed.connect(func():
		_current_tab = tab_id
		_refresh_list()
		AudioManager.play_sfx("ui_select")
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 리스트 갱신 =====================

func _refresh_list() -> void:
	if archive_status:
		archive_status.text = ("생물 기록 %d  ·  기억 기록 %d" if GameManager.current_locale == "ko" else "CREATURE RECORDS %d  ·  MEMORY RECORDS %d") % [enemy_entries.size(), memory_entries.size()]
	# 탭 활성 스타일
	_style_tab(tab_bestiary, _current_tab == "bestiary")
	_style_tab(tab_memories, _current_tab == "memories")

	for c in item_list.get_children():
		c.queue_free()
	detail_title.text = "Select an entry..."
	detail_body.text = ""
	_clear_enemy_preview()  # S59: clean up preview on tab switch

	if _current_tab == "bestiary":
		_populate_bestiary()
	else:
		_populate_memory_archive()

func _style_tab(btn: Button, active: bool) -> void:
	if active:
		btn.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	else:
		btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)

func _populate_bestiary() -> void:
	if enemy_entries.is_empty():
		var lbl = Label.new()
		lbl.text = "No enemies encountered yet."
		lbl.add_theme_font_size_override("font_size", 13)
		lbl.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(lbl)
		return

	for enemy_name in enemy_entries:
		var data = enemy_entries[enemy_name]
		var color = Color(0.6, 0.3, 0.5) if data.get("is_void", false) else Color(0.5, 0.55, 0.45)
		if data.get("is_boss", false):
			color = Color(0.7, 0.5, 0.3)
		# S59: Defeat milestone badge suffix
		var badge_text = _get_defeat_badge(data.get("defeated", 0))
		var display_name = enemy_name + badge_text
		var btn = _make_list_btn(display_name, color)
		btn.pressed.connect(func(): _show_enemy_detail(enemy_name, data))
		item_list.add_child(btn)

func _populate_memory_archive() -> void:
	if memory_entries.is_empty():
		var lbl = Label.new()
		lbl.text = "No memories collected yet."
		lbl.add_theme_font_size_override("font_size", 13)
		lbl.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(lbl)
		return

	const GRADE_COLORS_LOCAL = [
		Color(0.5, 0.5, 0.45),
		Color(0.55, 0.5, 0.35),
		Color(0.4, 0.5, 0.6),
		Color(0.6, 0.45, 0.55),
		Color(0.7, 0.55, 0.3),
	]
	for mem_id in memory_entries:
		var data = memory_entries[mem_id]
		var grade = data.get("grade", 0)
		var color = GRADE_COLORS_LOCAL[grade] if grade < GRADE_COLORS_LOCAL.size() else Color(0.5, 0.5, 0.5)
		var suffix = " [BURNED]" if data.get("burned", false) else ""
		# S59: Star rating based on grade
		var stars = _get_star_rating(grade)
		var btn = _make_list_btn(stars + " " + data.get("title", "???") + suffix, color)
		btn.pressed.connect(func(): _show_memory_detail(mem_id, data))
		item_list.add_child(btn)

func _make_list_btn(text: String, color: Color) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 32)
	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r * 0.2, color.g * 0.2, color.b * 0.2, 0.5)
	style.border_color = Color(color.r * 0.4, color.g * 0.4, color.b * 0.4, 0.3)
	style.border_width_left = 3
	style.set_content_margin_all(6)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)
	var hover = style.duplicate()
	hover.bg_color = Color(color.r * 0.35, color.g * 0.35, color.b * 0.35, 0.7)
	hover.border_color = color
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", Color(0.7, 0.65, 0.6))
	btn.add_theme_color_override("font_hover_color", color.lightened(0.3))
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 상세 표시 =====================

func _show_enemy_detail(enemy_name: String, data: Dictionary) -> void:
	# S59: Remove old enemy preview if exists
	_clear_enemy_preview()

	detail_title.text = enemy_name

	# S59: Add enemy sprite preview at top of detail panel
	_add_enemy_preview(enemy_name, data)

	var lines = ""
	if data.get("is_boss", false):
		lines += "Type: BOSS\n"
	elif data.get("is_void", false):
		lines += "Type: Void Beast\n"
	else:
		lines += "Type: Normal\n"
	lines += "Base HP: %d\n" % data.get("max_hp", 0)
	lines += "Base ATK: %d\n" % data.get("atk", 0)
	# S59: Defeat milestone badge in detail
	var defeated = data.get("defeated", 0)
	var badge = _get_defeat_badge(defeated)
	lines += "\nEncounters: %d\n" % data.get("encounters", 0)
	lines += "Defeated: %d %s\n" % [defeated, badge]
	# Scan data (from Tobias analyze)
	if data.get("scanned", false):
		var weakness = data.get("weakness", "")
		var resistance = data.get("resistance", "")
		lines += "\n--- SCAN DATA ---\n"
		lines += "Weakness: %s\n" % (weakness.to_upper() if weakness != "" else "None")
		lines += "Resistance: %s\n" % (resistance.to_upper() if resistance != "" else "None")
	else:
		lines += "\n[Not yet scanned — use Tobias: Analyze]\n"
	detail_body.text = lines

	# S59: Animated entry reveal for first-time views
	_animate_entry_reveal("enemy_" + enemy_name)

func _show_memory_detail(mem_id: String, data: Dictionary) -> void:
	# S59: Clear enemy preview when switching to memory
	_clear_enemy_preview()

	detail_title.text = data.get("title", "???")
	const GRADE_NAMES_LOCAL = ["Grade 5 — Sensory", "Grade 4 — Daily", "Grade 3 — Relational", "Grade 2 — Identity", "Grade 1 — Core"]
	var grade = data.get("grade", 0)
	var grade_name = GRADE_NAMES_LOCAL[grade] if grade < GRADE_NAMES_LOCAL.size() else "Unknown"
	var status = "BURNED" if data.get("burned", false) else "Held"
	# S59: Star rating in detail view
	var stars = _get_star_rating(grade)
	var lines = "%s  %s\nStatus: %s\n\n%s" % [grade_name, stars, status, data.get("desc", "")]
	detail_body.text = lines

	# S59: Animated entry reveal for first-time views
	_animate_entry_reveal("memory_" + mem_id)

## ===================== S59: Visual Enhancements =====================

## Star rating based on memory grade (grade 5=1star, grade 1=5stars)
func _get_star_rating(grade: int) -> String:
	var star_count = 5 - grade  # grade 0→5 stars, grade 4→1 star
	star_count = clampi(star_count, 1, 5)
	var filled = ""
	var empty = ""
	for i in range(star_count):
		filled += "\u2605"
	for i in range(5 - star_count):
		empty += "\u2606"
	return filled + empty

## Defeat milestone badge text
func _get_defeat_badge(defeated: int) -> String:
	if defeated >= 50:
		return " \u25cf"  # Gold dot
	elif defeated >= 25:
		return " \u25cb"  # Silver circle
	elif defeated >= 10:
		return " \u25e6"  # Bronze ring
	return ""

## Add enemy sprite preview to detail panel
func _add_enemy_preview(enemy_name: String, data: Dictionary) -> void:
	var preview_container = HBoxContainer.new()
	preview_container.name = "EnemyPreview"
	preview_container.custom_minimum_size = Vector2(0, 72)
	preview_container.add_theme_constant_override("separation", 8)

	var image_path: String = data.get("image_path", "")
	if image_path == "":
		image_path = BattleManager.resolve_enemy_image_by_name(enemy_name)
	if image_path != "" and ResourceLoader.exists(image_path):
		# Use actual enemy image
		var tex_rect = TextureRect.new()
		tex_rect.texture = load(image_path)
		tex_rect.custom_minimum_size = Vector2(64, 64)
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.expand_mode = TextureRect.EXPAND_FIT_WIDTH_PROPORTIONAL
		preview_container.add_child(tex_rect)
	else:
		# Colored placeholder with enemy initial
		var placeholder = ColorRect.new()
		placeholder.custom_minimum_size = Vector2(64, 64)
		# Color based on type
		if data.get("is_boss", false):
			placeholder.color = Color(0.35, 0.2, 0.1, 0.8)
		elif data.get("is_void", false):
			placeholder.color = Color(0.25, 0.1, 0.3, 0.8)
		else:
			placeholder.color = Color(0.15, 0.2, 0.15, 0.8)
		preview_container.add_child(placeholder)
		# Initial letter overlay
		var initial_lbl = Label.new()
		initial_lbl.text = enemy_name.substr(0, 1).to_upper()
		initial_lbl.add_theme_font_size_override("font_size", 28)
		initial_lbl.add_theme_color_override("font_color", Color(0.8, 0.75, 0.6, 0.7))
		initial_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		initial_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		initial_lbl.set_anchors_preset(Control.PRESET_FULL_RECT)
		initial_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
		placeholder.add_child(initial_lbl)

	# Insert preview after detail_title (index 0 is title, 1 is separator)
	if _detail_vbox and _detail_vbox.get_child_count() > 1:
		_detail_vbox.add_child(preview_container)
		_detail_vbox.move_child(preview_container, 1)  # after title, before separator
	_enemy_preview = preview_container

## Clear enemy preview node
func _clear_enemy_preview() -> void:
	if _enemy_preview and is_instance_valid(_enemy_preview):
		_enemy_preview.queue_free()
		_enemy_preview = null

## Animated entry reveal — slide in from right with flash on first view
func _animate_entry_reveal(entry_key: String) -> void:
	if _revealed_entries.has(entry_key):
		return  # Already seen, no animation
	_revealed_entries[entry_key] = true

	# Slide detail body from right
	if detail_body:
		var original_pos = detail_body.position
		detail_body.position.x += 60
		detail_body.modulate = Color(1.2, 1.1, 0.9, 0.0)
		var t = create_tween()
		t.set_parallel(true)
		t.tween_property(detail_body, "position:x", original_pos.x, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
		t.tween_property(detail_body, "modulate", Color(1, 1, 1, 1), 0.25).set_ease(Tween.EASE_OUT)

	# Brief white flash on detail title
	if detail_title:
		var original_color = detail_title.get_theme_color("font_color")
		detail_title.add_theme_color_override("font_color", Color(1.0, 0.95, 0.8))
		var t2 = create_tween()
		t2.tween_property(detail_title, "theme_override_colors/font_color", original_color, 0.4).set_delay(0.1)

	AudioManager.play_sfx("ui_select")

## ===================== 영구 저장 =====================

func _save_data() -> void:
	var data = {"enemies": enemy_entries.duplicate(true), "memories": memory_entries.duplicate(true)}
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
		enemy_entries = data.get("enemies", {})
		memory_entries = data.get("memories", {})
