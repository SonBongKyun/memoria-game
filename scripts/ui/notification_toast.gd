## NotificationToast (Autoload)
## 화면 하단 중앙에 간단한 알림 토스트를 표시하는 시스템.
extends CanvasLayer

enum ToastType { INFO, SUCCESS, WARNING }

const TOAST_COLORS := {
	ToastType.INFO: Color(0.7, 0.65, 0.55),
	ToastType.SUCCESS: Color(0.4, 0.7, 0.5),
	ToastType.WARNING: Color(0.8, 0.5, 0.3),
}

const TOAST_ICONS := {
	ToastType.INFO: "ℹ ",
	ToastType.SUCCESS: "✓ ",
	ToastType.WARNING: "⚠ ",
}

const SLIDE_DISTANCE := 20.0
const FADE_IN_TIME := 0.3
const HOLD_TIME := 2.0
const FADE_OUT_TIME := 0.5
const TOAST_FRAME_PATH: String = "res://assets/cg/generated/ui_notification_toast_frame.png"

var _queue: Array[Dictionary] = []
var _showing := false

var _frame: TextureRect
var _panel: PanelContainer
var _label: Label

func _ready() -> void:
	layer = 35
	_build_ui()
	_connect_signals()

func _build_ui() -> void:
	if ResourceLoader.exists(TOAST_FRAME_PATH):
		_frame = TextureRect.new()
		_frame.texture = load(TOAST_FRAME_PATH)
		_frame.anchor_left = 0.5
		_frame.anchor_right = 0.5
		_frame.anchor_top = 1.0
		_frame.anchor_bottom = 1.0
		_frame.grow_horizontal = Control.GROW_DIRECTION_BOTH
		_frame.offset_left = -178
		_frame.offset_right = 178
		_frame.offset_top = -92
		_frame.offset_bottom = -38
		_frame.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		_frame.stretch_mode = TextureRect.STRETCH_SCALE
		_frame.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_frame.modulate = Color(1.0, 0.92, 0.78, 0.0)
		add_child(_frame)

	# Panel container
	_panel = PanelContainer.new()
	_panel.anchor_left = 0.5
	_panel.anchor_right = 0.5
	_panel.anchor_top = 1.0
	_panel.anchor_bottom = 1.0
	_panel.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_panel.offset_left = -150
	_panel.offset_right = 150
	_panel.offset_top = -80
	_panel.offset_bottom = -50
	_panel.modulate.a = 0.0

	# StyleBox
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.035, 0.030, 0.048, 0.64)
	style.border_color = Color(0.8, 0.65, 0.3, 0.32)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(8)
	style.content_margin_left = 14
	style.content_margin_right = 14
	_panel.add_theme_stylebox_override("panel", style)

	# Label
	_label = Label.new()
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.add_theme_font_size_override("font_size", 13)
	_panel.add_child(_label)

	add_child(_panel)

func _connect_signals() -> void:
	if MemoryManager:
		if MemoryManager.has_signal("memory_added"):
			MemoryManager.memory_added.connect(_on_memory_added)
		if MemoryManager.has_signal("memory_burned"):
			MemoryManager.memory_burned.connect(_on_memory_burned)
	if SaveManager:
		if SaveManager.has_signal("save_completed"):
			SaveManager.save_completed.connect(_on_save_completed)
		if SaveManager.has_signal("load_completed"):
			SaveManager.load_completed.connect(_on_load_completed)

func _on_memory_added(memory) -> void:
	show_toast("Memory acquired: %s" % memory.title, ToastType.SUCCESS)

func _on_memory_burned(memory) -> void:
	show_toast("Memory burned: %s" % memory.title, ToastType.WARNING)

func _on_save_completed(slot: int) -> void:
	if slot == 0:
		show_toast("Autosaved", ToastType.INFO)
	else:
		show_toast("Game saved — Slot %d" % slot, ToastType.SUCCESS)

func _on_load_completed(slot: int) -> void:
	if slot == 0:
		show_toast("Autosave loaded", ToastType.INFO)
	else:
		show_toast("Game loaded — Slot %d" % slot, ToastType.INFO)

func show_toast(text: String, type: ToastType = ToastType.INFO) -> void:
	_queue.append({"text": text, "type": type})
	if not _showing:
		_process_queue()

func _process_queue() -> void:
	if _queue.is_empty():
		_showing = false
		return
	_showing = true
	var item: Dictionary = _queue.pop_front()
	_display_toast(item.text, item.type as ToastType)

func _display_toast(text: String, type: ToastType) -> void:
	var icon: String = TOAST_ICONS.get(type, "")
	var color: Color = TOAST_COLORS.get(type, Color.WHITE)

	_label.text = icon + text
	_label.add_theme_color_override("font_color", color)

	# Reset position for slide-up animation
	_panel.modulate.a = 0.0
	var base_top: float = -80.0
	_panel.offset_top = base_top + SLIDE_DISTANCE
	_panel.offset_bottom = -50.0 + SLIDE_DISTANCE
	if _frame:
		_frame.offset_top = base_top + SLIDE_DISTANCE - 12.0
		_frame.offset_bottom = -50.0 + SLIDE_DISTANCE + 12.0
		_frame.modulate.a = 0.0

	# Fade in + slide up
	var tween := create_tween()
	tween.set_parallel(true)
	if _frame:
		tween.tween_property(_frame, "modulate:a", 0.76, FADE_IN_TIME)
		tween.tween_property(_frame, "offset_top", base_top - 12.0, FADE_IN_TIME).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
		tween.tween_property(_frame, "offset_bottom", -38.0, FADE_IN_TIME).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	tween.tween_property(_panel, "modulate:a", 1.0, FADE_IN_TIME)
	tween.tween_property(_panel, "offset_top", base_top, FADE_IN_TIME).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)
	tween.tween_property(_panel, "offset_bottom", -50.0, FADE_IN_TIME).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)

	# Hold
	tween.chain().tween_interval(HOLD_TIME)

	# Fade out
	tween.chain().tween_property(_panel, "modulate:a", 0.0, FADE_OUT_TIME)
	if _frame:
		tween.parallel().tween_property(_frame, "modulate:a", 0.0, FADE_OUT_TIME)

	# Next in queue
	tween.chain().tween_callback(_process_queue)
