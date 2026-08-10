## StoryLog (Autoload) — S209
## 지나간 대사를 되짚어 보는 회상 기록.
##
## MEMORIA는 20개 VN 파일 504스텝 + 필드 대화 1400줄 규모의 이야기 중심 게임인데,
## 실수로 한 줄 넘겨 버리면 그 문장을 다시 볼 방법이 전혀 없었다. 이 오토로드는
## DialogueBox(필드 대화)와 VN 씬(파트 II 이후) 양쪽의 대사를 한 기록으로 모으고,
## 이미 읽은 문장을 영구 저장해 빨리감기 범위를 판단하는 근거로도 쓴다.
##
## 조작: L (또는 패널의 닫기), Esc로 닫기. 일시정지 중에도 동작한다.
extends CanvasLayer

const MAX_ENTRIES: int = 300
const READ_REGISTRY_PATH: String = "user://read_lines.json"
const MAX_READ_KEYS: int = 8000
const BACKDROP_PATH: String = "res://assets/cg/generated/ui_story_log_archive_v1.png"

## [{speaker, text, chapter, source}]
var entries: Array = []
var is_open: bool = false

var _read_keys: Dictionary = {}
var _read_dirty: bool = false
## 스모크 테스트가 합성 문장으로 플레이어의 읽음 등록부를 오염시키지 않도록 하는 스위치.
var suppress_persistence: bool = false

## 마지막으로 표시된 대사가 "처음 보는 문장"이었는지.
## 빨리감기가 새 이야기를 삼키지 않도록 판단하는 근거.
var last_line_was_new: bool = false

var _overlay: Control
var _panel: PanelContainer
var _scroll: ScrollContainer
var _list: VBoxContainer
var _empty_label: Label
var _was_paused: bool = false

func _ready() -> void:
	# StoryJournal(57)보다 위. 일시정지 메뉴에서 열어도 그 위에 덮인다.
	layer = 58
	process_mode = Node.PROCESS_MODE_ALWAYS
	_load_read_registry()
	_build_ui()
	print("[StoryLog] Ready, %d read lines remembered" % _read_keys.size())

func _exit_tree() -> void:
	_save_read_registry()

# ===================== 기록 =====================

## 화면에 표시된 대사 한 줄을 기록한다. `source`는 "field" 또는 "vn".
func record(speaker: String, text: String, source: String = "field") -> void:
	var clean := text.strip_edges()
	if clean == "":
		return
	last_line_was_new = not is_read(speaker, clean)
	# 같은 줄이 연속으로 다시 표시되는 경우(재입력, 리프레시)는 쌓지 않는다.
	if not entries.is_empty():
		var last: Dictionary = entries[entries.size() - 1]
		if String(last.get("text", "")) == clean and String(last.get("speaker", "")) == speaker:
			return
	entries.append({
		"speaker": speaker,
		"text": clean,
		"chapter": GameManager.current_chapter,
		"source": source,
	})
	if entries.size() > MAX_ENTRIES:
		entries = entries.slice(entries.size() - MAX_ENTRIES)
	mark_read(speaker, clean)

## 플레이어가 고른 선택지를 기록한다.
func record_choice(choice_text: String) -> void:
	var clean := choice_text.strip_edges()
	if clean == "":
		return
	entries.append({
		"speaker": "__choice__",
		"text": clean,
		"chapter": GameManager.current_chapter,
		"source": "choice",
	})
	if entries.size() > MAX_ENTRIES:
		entries = entries.slice(entries.size() - MAX_ENTRIES)

func clear() -> void:
	entries.clear()

# ===================== 읽은 대사 등록부 =====================

static func line_key(speaker: String, text: String) -> String:
	return str(hash("%s|%s" % [speaker, text.strip_edges()]))

## 이 문장을 이미 읽었는지. 빨리감기 범위 판단에 쓰인다.
func is_read(speaker: String, text: String) -> bool:
	return _read_keys.has(line_key(speaker, text))

func mark_read(speaker: String, text: String) -> void:
	var key := line_key(speaker, text)
	if _read_keys.has(key):
		return
	# 등록부가 무한정 커지지 않도록 상한을 둔다. 넘치면 가장 오래된 절반을 버린다.
	if _read_keys.size() >= MAX_READ_KEYS:
		var keys: Array = _read_keys.keys()
		for i in range(keys.size() / 2):
			_read_keys.erase(keys[i])
	_read_keys[key] = 1
	_read_dirty = true

func read_line_count() -> int:
	return _read_keys.size()

## 빨리감기가 지금 진행해도 되는지.
## "읽은 대사만 빨리감기"가 켜져 있으면 처음 보는 문장 앞에서 멈춘다.
func can_fast_forward() -> bool:
	var read_only: bool = true
	if OptionsMenu:
		read_only = bool(OptionsMenu.settings.get("skip_read_only", true))
	if not read_only:
		return true
	return not last_line_was_new

func _load_read_registry() -> void:
	if not FileAccess.file_exists(READ_REGISTRY_PATH):
		return
	var file := FileAccess.open(READ_REGISTRY_PATH, FileAccess.READ)
	if file == null:
		return
	var text := file.get_as_text()
	file.close()
	var json := JSON.new()
	if json.parse(text) != OK:
		return
	var data: Variant = json.data
	if data is Array:
		for key: Variant in data:
			_read_keys[String(key)] = 1

func _save_read_registry() -> void:
	if not _read_dirty or suppress_persistence:
		return
	var file := FileAccess.open(READ_REGISTRY_PATH, FileAccess.WRITE)
	if file == null:
		return
	file.store_string(JSON.stringify(_read_keys.keys()))
	file.close()
	_read_dirty = false

# ===================== UI =====================

func _build_ui() -> void:
	_overlay = Control.new()
	_overlay.name = "StoryLogOverlay"
	_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	_overlay.visible = false
	add_child(_overlay)

	var backdrop := TextureRect.new()
	backdrop.name = "StoryLogBackdrop"
	backdrop.set_anchors_preset(Control.PRESET_FULL_RECT)
	backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	backdrop.texture = load(BACKDROP_PATH) as Texture2D
	backdrop.modulate = Color(0.82, 0.84, 0.90, 0.92)
	backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_overlay.add_child(backdrop)

	var dimmer := ColorRect.new()
	dimmer.set_anchors_preset(Control.PRESET_FULL_RECT)
	dimmer.color = Color(0.012, 0.010, 0.020, 0.22)
	dimmer.mouse_filter = Control.MOUSE_FILTER_STOP
	_overlay.add_child(dimmer)

	_panel = PanelContainer.new()
	_panel.name = "StoryLogPanel"
	_panel.anchor_left = 0.12
	_panel.anchor_right = 0.88
	_panel.anchor_top = 0.07
	_panel.anchor_bottom = 0.93
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.018, 0.017, 0.026, 0.78)
	style.border_color = Color(0.62, 0.48, 0.26, 0.34)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(18)
	_panel.add_theme_stylebox_override("panel", style)
	_overlay.add_child(_panel)

	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	_panel.add_child(box)

	var header := Label.new()
	header.name = "StoryLogHeader"
	header.text = _loc("STORY LOG / RECOLLECTION", "회상 기록")
	header.add_theme_font_size_override("font_size", 21)
	header.add_theme_color_override("font_color", Color(0.94, 0.80, 0.48))
	box.add_child(header)

	var hint := Label.new()
	hint.text = _loc(
		"Lines you have already seen, newest at the bottom.  [L] or [Esc] to close.",
		"이미 지나간 대사입니다. 아래쪽이 가장 최근.  [L] 또는 [Esc]로 닫기."
	)
	hint.add_theme_font_size_override("font_size", 13)
	hint.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	box.add_child(hint)

	var rule := ColorRect.new()
	rule.custom_minimum_size = Vector2(0, 1)
	rule.color = Color(0.45, 0.36, 0.22, 0.45)
	box.add_child(rule)

	_scroll = ScrollContainer.new()
	_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	box.add_child(_scroll)

	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 11)
	_scroll.add_child(_list)

	_empty_label = Label.new()
	_empty_label.text = _loc("No recorded lines yet.", "아직 기록된 대사가 없습니다.")
	_empty_label.add_theme_font_size_override("font_size", 14)
	_empty_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	_empty_label.visible = false
	box.add_child(_empty_label)

func open_log() -> void:
	if is_open:
		return
	# 필드에서 L로 직접 열어도 뒤에서 플레이어/NPC/대사가 계속 진행되면 기록을
	# 읽는 동안 위치와 이야기 상태가 바뀐다. 기존 일시정지 상태를 기억한 뒤 항상
	# 정지하고, PauseMenu 위에서 열었던 경우에는 닫을 때 그 정지를 그대로 보존한다.
	_was_paused = get_tree().paused
	get_tree().paused = true
	is_open = true
	_rebuild_list()
	_overlay.visible = true
	AudioManager.play_sfx("ui_open")
	# 목록 맨 아래(가장 최근 대사)에서 시작한다.
	await get_tree().process_frame
	if is_instance_valid(_scroll):
		_scroll.scroll_vertical = int(_scroll.get_v_scroll_bar().max_value)

func close_log() -> void:
	if not is_open:
		return
	is_open = false
	_overlay.visible = false
	_save_read_registry()
	get_tree().paused = _was_paused
	AudioManager.play_sfx("ui_close")

func _rebuild_list() -> void:
	for child in _list.get_children():
		child.queue_free()
	_empty_label.visible = entries.is_empty()
	var last_chapter: int = -1
	for entry: Dictionary in entries:
		var chapter := int(entry.get("chapter", 0))
		if chapter != last_chapter:
			last_chapter = chapter
			var chapter_label := Label.new()
			chapter_label.text = _loc("— Chapter %d —", "— %d장 —") % chapter
			chapter_label.add_theme_font_size_override("font_size", 13)
			chapter_label.add_theme_color_override("font_color", Color(0.58, 0.48, 0.34))
			_list.add_child(chapter_label)
		_list.add_child(_make_entry_row(entry))

func _make_entry_row(entry: Dictionary) -> Control:
	var speaker := String(entry.get("speaker", ""))
	var row := VBoxContainer.new()
	row.add_theme_constant_override("separation", 2)
	row.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	if speaker == "__choice__":
		var choice := Label.new()
		choice.text = _loc("▸ chose: %s", "▸ 선택: %s") % String(entry.get("text", ""))
		choice.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		choice.add_theme_font_size_override("font_size", 14)
		choice.add_theme_color_override("font_color", Color(0.62, 0.80, 0.94))
		row.add_child(choice)
		return row

	if speaker != "":
		var name_label := Label.new()
		name_label.text = GameManager.localized_speaker(speaker)
		name_label.add_theme_font_size_override("font_size", 13)
		# 로그는 어두운 판 위에 촘촘히 쌓이므로, 화자색을 그대로 쓰면 이름이 묻힌다.
		name_label.add_theme_color_override("font_color", UITheme.get_speaker_color(speaker).lightened(0.22))
		row.add_child(name_label)

	var body := Label.new()
	body.text = String(entry.get("text", ""))
	body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	body.add_theme_font_size_override("font_size", 15)
	body.add_theme_color_override("font_color", UITheme.TEXT_PRIMARY if speaker != "" else UITheme.TEXT_NARRATION)
	row.add_child(body)
	return row

func _unhandled_input(event: InputEvent) -> void:
	if is_open:
		if event.is_action_pressed("cancel") or _is_log_key(event):
			close_log()
			get_viewport().set_input_as_handled()
		return
	if not _is_log_key(event):
		return
	# 이야기가 흐르는 중에만 의미가 있다. 전투/메뉴에서는 열지 않는다.
	if GameManager.current_state != GameManager.GameState.DIALOGUE and GameManager.current_state != GameManager.GameState.EXPLORATION:
		return
	open_log()
	get_viewport().set_input_as_handled()

func _is_log_key(event: InputEvent) -> bool:
	if not (event is InputEventKey):
		return false
	var key := event as InputEventKey
	return key.pressed and not key.echo and key.keycode == KEY_L

func _loc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en
