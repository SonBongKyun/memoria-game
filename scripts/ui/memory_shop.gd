## MemoryShop (Autoload)
## 기억 거래 상점 UI. 말렛 등 상인 NPC와의 기억 매매.
## Grains 화폐로 기억을 사고팔 수 있음.
extends CanvasLayer

var is_open: bool = false

# ── 노드 참조 ──
var overlay: ColorRect
var main_panel: PanelContainer
var shop_title: Label
var grains_label: Label
var tab_buy: Button
var tab_sell: Button
var tab_items: Button
var item_list: VBoxContainer
var item_scroll: ScrollContainer
var detail_panel: PanelContainer
var detail_title: Label
var detail_grade: Label
var detail_desc: RichTextLabel
var detail_price: Label
var detail_effect: Label
var action_btn: Button
var close_btn: Button
var close_hint: Label

# ── 상점 데이터 ──
var _current_mode: String = "sell"  # "buy", "sell", or "items"
var _shop_inventory: Array[Dictionary] = []  # 상점 구매 가능 아이템
var _selected_item: Dictionary = {}
var _merchant_name: String = "Merchant"

const GRADE_NAMES = ["Grade 5 — Sensory", "Grade 4 — Daily", "Grade 3 — Relational", "Grade 2 — Identity", "Grade 1 — Core"]
const GRADE_COLORS = [
	Color(0.5, 0.5, 0.45),
	Color(0.55, 0.5, 0.35),
	Color(0.4, 0.5, 0.6),
	Color(0.6, 0.45, 0.55),
	Color(0.7, 0.55, 0.3),
]

# ── 기억 등급별 판매/구매 가격 ──
const SELL_PRICES := {0: 5, 1: 15, 2: 30, 3: 60, 4: 150}   # 판매가 (플레이어→상인)
const BUY_PRICES := {0: 10, 1: 25, 2: 50, 3: 100, 4: 300}   # 구매가 (상인→플레이어)

signal shop_closed()
signal grains_changed(amount: int)

func _ready() -> void:
	layer = 42  # MemoryUI(40) 위, CgViewer(45) 아래
	_build_ui()
	_hide_ui()
	print("[MemoryShop] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if not is_open:
		return
	if event.is_action_pressed("cancel"):
		close_shop()
		get_viewport().set_input_as_handled()

## 상점 열기 — merchant_name: 상인 이름, inventory: 구매 가능 아이템
func open_shop(merchant_name: String = "Merchant", inventory: Array[Dictionary] = []) -> void:
	if is_open:
		return
	is_open = true
	_merchant_name = merchant_name
	_shop_inventory = inventory
	_current_mode = "sell"
	GameManager.change_state(GameManager.GameState.MENU)
	AudioManager.play_sfx("ui_open")
	shop_title.text = "%s — Memory Exchange" % merchant_name
	_update_grains()
	_refresh_items()
	_show_ui()

func close_shop() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	_hide_ui()
	shop_closed.emit()

## ===================== UI 구축 =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# 오버레이
	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = UITheme.BG_OVERLAY
	root.add_child(overlay)

	# 메인 패널
	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.08
	main_panel.anchor_right = 0.92
	main_panel.anchor_top = 0.06
	main_panel.anchor_bottom = 0.94
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.09, 0.07, 0.06, 0.95),
		Color(0.5, 0.4, 0.25, 0.7),
		2, 6, 16
	))
	root.add_child(main_panel)

	var main_vbox = VBoxContainer.new()
	main_vbox.add_theme_constant_override("separation", 10)
	main_panel.add_child(main_vbox)

	# ── 헤더: 타이틀 + Grains ──
	var header = HBoxContainer.new()
	main_vbox.add_child(header)

	shop_title = Label.new()
	shop_title.text = "Memory Exchange"
	shop_title.add_theme_font_size_override("font_size", 18)
	shop_title.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	shop_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(shop_title)

	grains_label = Label.new()
	grains_label.add_theme_font_size_override("font_size", 16)
	grains_label.add_theme_color_override("font_color", Color(0.85, 0.7, 0.35))
	header.add_child(grains_label)

	# ── 탭 (Buy / Sell) ──
	var tab_row = HBoxContainer.new()
	tab_row.add_theme_constant_override("separation", 8)
	main_vbox.add_child(tab_row)

	tab_sell = _create_tab("Sell Memories", "sell")
	tab_row.add_child(tab_sell)

	tab_buy = _create_tab("Buy Memories", "buy")
	tab_row.add_child(tab_buy)

	tab_items = _create_tab("Items", "items")
	tab_row.add_child(tab_items)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", UITheme.BORDER_DIM)
	main_vbox.add_child(sep)

	# ── 본문: 아이템 목록 + 상세 ──
	var content = HBoxContainer.new()
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_theme_constant_override("separation", 12)
	main_vbox.add_child(content)

	# 좌측: 아이템 스크롤
	item_scroll = ScrollContainer.new()
	item_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	content.add_child(item_scroll)

	item_list = VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 4)
	item_scroll.add_child(item_list)

	# 우측: 상세 + 액션
	_build_detail_panel(content)

	# ── 하단: 닫기 힌트 ──
	close_hint = Label.new()
	close_hint.text = "[ESC] Close Shop"
	close_hint.add_theme_font_size_override("font_size", 11)
	close_hint.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	close_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	main_vbox.add_child(close_hint)

func _build_detail_panel(parent: HBoxContainer) -> void:
	detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(300, 0)
	detail_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.07, 0.05, 0.04, 0.8),
		UITheme.BORDER_DIM,
		1, 4, 14
	))
	parent.add_child(detail_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	detail_panel.add_child(vbox)

	detail_title = Label.new()
	detail_title.text = "Select a memory..."
	detail_title.add_theme_font_size_override("font_size", 16)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.7, 0.55))
	vbox.add_child(detail_title)

	detail_grade = Label.new()
	detail_grade.add_theme_font_size_override("font_size", 12)
	vbox.add_child(detail_grade)

	var sep2 = HSeparator.new()
	sep2.add_theme_color_override("separator", UITheme.BORDER_DIM)
	vbox.add_child(sep2)

	detail_desc = RichTextLabel.new()
	detail_desc.bbcode_enabled = false
	detail_desc.fit_content = true
	detail_desc.scroll_active = false
	detail_desc.add_theme_font_size_override("normal_font_size", 13)
	detail_desc.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	detail_desc.custom_minimum_size = Vector2(0, 50)
	vbox.add_child(detail_desc)

	detail_price = Label.new()
	detail_price.add_theme_font_size_override("font_size", 14)
	detail_price.add_theme_color_override("font_color", Color(0.85, 0.7, 0.35))
	vbox.add_child(detail_price)

	detail_effect = Label.new()
	detail_effect.add_theme_font_size_override("font_size", 11)
	detail_effect.add_theme_color_override("font_color", Color(0.6, 0.45, 0.4))
	detail_effect.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vbox.add_child(detail_effect)

	# 액션 버튼
	action_btn = Button.new()
	action_btn.text = "Sell"
	action_btn.custom_minimum_size = Vector2(0, 36)
	var btn_style = UITheme.make_button_style(Color(0.15, 0.12, 0.08, 0.9), Color(0.6, 0.45, 0.25, 0.7))
	action_btn.add_theme_stylebox_override("normal", btn_style)
	action_btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(btn_style))
	var pressed_style = btn_style.duplicate()
	pressed_style.bg_color = Color(0.2, 0.15, 0.1, 0.95)
	action_btn.add_theme_stylebox_override("pressed", pressed_style)
	action_btn.add_theme_font_size_override("font_size", 14)
	action_btn.add_theme_color_override("font_color", Color(0.85, 0.7, 0.35))
	action_btn.add_theme_color_override("font_hover_color", Color(1.0, 0.85, 0.5))
	action_btn.pressed.connect(_on_action_pressed)
	action_btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	action_btn.visible = false
	vbox.add_child(action_btn)

func _create_tab(text: String, mode: String) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.custom_minimum_size = Vector2(140, 30)
	var style = UITheme.make_button_style()
	btn.add_theme_stylebox_override("normal", style)
	btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(style))
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	btn.pressed.connect(func():
		_current_mode = mode
		_refresh_items()
		AudioManager.play_sfx("ui_select")
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 아이템 목록 갱신 =====================

func _refresh_items() -> void:
	# 탭 스타일 업데이트
	_update_tab_style(tab_sell, _current_mode == "sell")
	_update_tab_style(tab_buy, _current_mode == "buy")
	_update_tab_style(tab_items, _current_mode == "items")

	# 목록 클리어
	for child in item_list.get_children():
		child.queue_free()
	_clear_detail()

	if _current_mode == "sell":
		_populate_sell_list()
	elif _current_mode == "buy":
		_populate_buy_list()
	else:
		_populate_items_list()

func _update_tab_style(btn: Button, active: bool) -> void:
	if active:
		btn.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
		var style = btn.get_theme_stylebox("normal").duplicate()
		style.border_color = UITheme.TEXT_ACCENT
		style.set_border_width_all(1)
		btn.add_theme_stylebox_override("normal", style)
	else:
		btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		btn.add_theme_stylebox_override("normal", UITheme.make_button_style())

## 판매 목록 (플레이어 보유 기억 중 미연소)
func _populate_sell_list() -> void:
	var available = MemoryManager.get_available_memories()
	if available.is_empty():
		var empty_label = Label.new()
		empty_label.text = "No memories available to sell."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty_label)
		return

	for memory in available:
		# Grade 1 (핵심 기억)은 판매 불가
		if memory.grade == MemoryManager.MemoryGrade.GRADE_1:
			continue
		var price = SELL_PRICES.get(memory.grade, 5)
		var item = {"type": "sell", "memory": memory, "price": price}
		_add_item_button(memory.title, GRADE_COLORS[memory.grade], price, item)

## 구매 목록 (상점 인벤토리)
func _populate_buy_list() -> void:
	if _shop_inventory.is_empty():
		var empty_label = Label.new()
		empty_label.text = "Nothing available for purchase."
		empty_label.add_theme_font_size_override("font_size", 13)
		empty_label.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty_label)
		return

	for item_data in _shop_inventory:
		if item_data.get("sold", false):
			continue
		var price = item_data.get("price", 10)
		var grade_color = GRADE_COLORS[item_data.get("grade", 0)]
		var item = {"type": "buy", "data": item_data, "price": price}
		_add_item_button(item_data.get("title", "???"), grade_color, price, item)

## 아이템 매매 목록
func _populate_items_list() -> void:
	# 구매 가능한 아이템
	var buy_header = Label.new()
	buy_header.text = "— Buy Items —"
	buy_header.add_theme_font_size_override("font_size", 12)
	buy_header.add_theme_color_override("font_color", Color(0.55, 0.75, 0.55))
	buy_header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	item_list.add_child(buy_header)

	for item_id in GameManager.ITEMS:
		var def = GameManager.ITEMS[item_id]
		var price = def["price"]
		var item = {"type": "buy_item", "item_id": item_id, "price": price}
		_add_item_button(def["name"], Color(0.45, 0.65, 0.45), price, item)

	# 판매 가능한 아이템
	var sell_header = Label.new()
	sell_header.text = "— Sell Items —"
	sell_header.add_theme_font_size_override("font_size", 12)
	sell_header.add_theme_color_override("font_color", Color(0.75, 0.6, 0.4))
	sell_header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	item_list.add_child(sell_header)

	var has_items = false
	for item_id in GameManager.player_data.get("items", {}):
		var count = GameManager.player_data["items"][item_id]
		if count <= 0:
			continue
		has_items = true
		var def = GameManager.ITEMS.get(item_id)
		if def == null:
			continue
		var sell_price = int(def["price"] * 0.6)  # 60% 가격에 판매
		var item = {"type": "sell_item", "item_id": item_id, "price": sell_price, "count": count}
		_add_item_button("%s (×%d)" % [def["name"], count], Color(0.65, 0.55, 0.35), sell_price, item)

	if not has_items:
		var empty = Label.new()
		empty.text = "No items to sell."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _add_item_button(title: String, color: Color, price: int, item_data: Dictionary) -> void:
	var btn = Button.new()
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 36)
	btn.text = "%s — %d G" % [title, price]

	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r * 0.25, color.g * 0.25, color.b * 0.25, 0.5)
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
	btn.add_theme_color_override("font_color", Color(0.75, 0.7, 0.65))
	btn.add_theme_color_override("font_hover_color", color.lightened(0.3))

	btn.pressed.connect(func(): _select_item(item_data))
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	item_list.add_child(btn)

## ===================== 상세 정보 =====================

func _select_item(item: Dictionary) -> void:
	_selected_item = item

	if item.type == "sell":
		var memory = item.memory
		detail_title.text = memory.title
		detail_grade.text = GRADE_NAMES[memory.grade]
		detail_grade.add_theme_color_override("font_color", GRADE_COLORS[memory.grade])
		detail_desc.text = memory.description
		detail_price.text = "Sell Price: %d Grains" % item.price
		if memory.story_effect != "":
			detail_effect.text = "Warning: %s" % memory.story_effect
			detail_effect.visible = true
		else:
			detail_effect.visible = false
		action_btn.text = "Sell for %d G" % item.price
		action_btn.disabled = false
		action_btn.visible = true
	elif item.type == "buy":
		var data = item.data
		detail_title.text = data.get("title", "???")
		var grade = data.get("grade", 0)
		detail_grade.text = GRADE_NAMES[grade]
		detail_grade.add_theme_color_override("font_color", GRADE_COLORS[grade])
		detail_desc.text = data.get("description", "")
		detail_price.text = "Buy Price: %d Grains" % item.price
		detail_effect.visible = false
		var can_afford = GameManager.player_data.grains >= item.price
		action_btn.text = "Buy for %d G" % item.price if can_afford else "Not enough Grains"
		action_btn.disabled = not can_afford
		action_btn.visible = true
	elif item.type == "buy_item":
		var def = GameManager.ITEMS.get(item.item_id, {})
		detail_title.text = def.get("name", "???")
		detail_grade.text = "Consumable Item"
		detail_grade.add_theme_color_override("font_color", Color(0.55, 0.75, 0.55))
		detail_desc.text = def.get("desc", "")
		detail_price.text = "Buy Price: %d Grains" % item.price
		detail_effect.visible = false
		var can_afford = GameManager.player_data.get("grains", 0) >= item.price
		action_btn.text = "Buy for %d G" % item.price if can_afford else "Not enough Grains"
		action_btn.disabled = not can_afford
		action_btn.visible = true
	elif item.type == "sell_item":
		var def = GameManager.ITEMS.get(item.item_id, {})
		detail_title.text = def.get("name", "???")
		detail_grade.text = "Consumable Item (×%d owned)" % item.get("count", 0)
		detail_grade.add_theme_color_override("font_color", Color(0.65, 0.55, 0.35))
		detail_desc.text = def.get("desc", "")
		detail_price.text = "Sell Price: %d Grains" % item.price
		detail_effect.visible = false
		action_btn.text = "Sell for %d G" % item.price
		action_btn.disabled = false
		action_btn.visible = true

func _clear_detail() -> void:
	detail_title.text = "Select a memory..."
	detail_grade.text = ""
	detail_desc.text = ""
	detail_price.text = ""
	detail_effect.text = ""
	detail_effect.visible = false
	action_btn.visible = false
	_selected_item = {}

## ===================== 거래 실행 =====================

func _on_action_pressed() -> void:
	if _selected_item.is_empty():
		return

	if _selected_item.type == "sell":
		_execute_sell()
	elif _selected_item.type == "buy":
		_execute_buy()
	elif _selected_item.type == "buy_item":
		_execute_buy_item()
	elif _selected_item.type == "sell_item":
		_execute_sell_item()

func _execute_sell() -> void:
	var memory = _selected_item.memory
	var price = _selected_item.price

	# 기억 연소 (거래는 연소와 동일한 효과)
	MemoryManager.burn_memory(memory.id)

	# Grains 지급
	GameManager.player_data.grains += price
	grains_changed.emit(GameManager.player_data.grains)
	AudioManager.play_sfx("confirm")

	NotificationToast.show_toast("Sold: %s (+%d G)" % [memory.title, price], NotificationToast.ToastType.WARNING)
	_update_grains()
	_refresh_items()

func _execute_buy() -> void:
	var data = _selected_item.data
	var price = _selected_item.price

	if GameManager.player_data.grains < price:
		return

	# Grains 차감
	GameManager.player_data.grains -= price
	grains_changed.emit(GameManager.player_data.grains)

	# 기억 추가
	var memory = MemoryManager.Memory.new(
		data.id, data.title, data.description,
		data.grade, data.get("burn_power", 10),
		data.get("story_effect", ""), data.get("related_npc", "")
	)
	MemoryManager.add_memory(memory)

	# 상점 인벤토리에서 제거
	data["sold"] = true
	AudioManager.play_sfx("confirm")

	NotificationToast.show_toast("Bought: %s (-%d G)" % [data.title, price], NotificationToast.ToastType.SUCCESS)
	_update_grains()
	_refresh_items()

func _execute_buy_item() -> void:
	var item_id = _selected_item.item_id
	var price = _selected_item.price
	if GameManager.player_data.get("grains", 0) < price:
		return
	GameManager.player_data.grains -= price
	grains_changed.emit(GameManager.player_data.grains)
	GameManager.add_item(item_id)
	AudioManager.play_sfx("confirm")
	_update_grains()
	_refresh_items()

func _execute_sell_item() -> void:
	var item_id = _selected_item.item_id
	var price = _selected_item.price
	if not GameManager.remove_item(item_id):
		return
	GameManager.player_data.grains += price
	grains_changed.emit(GameManager.player_data.grains)
	AudioManager.play_sfx("confirm")
	var def = GameManager.ITEMS.get(item_id, {})
	NotificationToast.show_toast("Sold: %s (+%d G)" % [def.get("name", "?"), price], NotificationToast.ToastType.WARNING)
	_update_grains()
	_refresh_items()

func _update_grains() -> void:
	grains_label.text = "%d Grains" % GameManager.player_data.grains
	AchievementManager.check_grains()

func _show_ui() -> void:
	overlay.visible = true
	main_panel.visible = true

func _hide_ui() -> void:
	overlay.visible = false
	main_panel.visible = false
