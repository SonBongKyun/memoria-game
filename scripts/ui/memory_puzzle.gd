## MemoryPuzzle (Autoload)
## 기억 매칭 퍼즐 미니게임. 카드 뒤집기로 기억 쌍 맞추기.
## 클리어 시 Grains 보상 + 업적.
extends CanvasLayer

var is_open: bool = false

# ── 게임 데이터 ──
var _cards: Array[Dictionary] = []  # {id, title, idx, flipped, matched, node}
var _first_pick: int = -1
var _second_pick: int = -1
var _locked: bool = false  # 매칭 체크 중 입력 차단
var _matches_found: int = 0
var _total_pairs: int = 0
var _attempts: int = 0
var _reward_grains: int = 0

# ── UI 노드 ──
var overlay: ColorRect
var main_panel: PanelContainer
var grid: GridContainer
var info_label: Label
var close_btn: Button

signal puzzle_closed()

func _ready() -> void:
	layer = 43  # MemoryShop(42) 위
	_build_ui()
	_hide_ui()
	print("[MemoryPuzzle] Ready")

func _unhandled_input(event: InputEvent) -> void:
	if is_open and event.is_action_pressed("cancel"):
		close_puzzle()
		get_viewport().set_input_as_handled()

## ===================== 열기/닫기 =====================

## 퍼즐 시작 — pair_count: 매칭할 쌍 수 (3~6), reward: 클리어 보상 Grains
func open_puzzle(pair_count: int = 4, reward: int = 15) -> void:
	if is_open:
		return
	is_open = true
	_reward_grains = reward
	_matches_found = 0
	_attempts = 0
	_first_pick = -1
	_second_pick = -1
	_locked = false
	GameManager.change_state(GameManager.GameState.MENU)
	AudioManager.play_sfx("ui_open")
	_setup_cards(pair_count)
	_update_info()
	_show_ui()

func close_puzzle() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	_hide_ui()
	puzzle_closed.emit()

func _show_ui() -> void:
	overlay.visible = true
	main_panel.visible = true

func _hide_ui() -> void:
	if overlay:
		overlay.visible = false
	if main_panel:
		main_panel.visible = false

## ===================== 카드 세팅 =====================

func _setup_cards(pair_count: int) -> void:
	# 그리드 초기화
	for c in grid.get_children():
		c.queue_free()
	_cards.clear()

	pair_count = clampi(pair_count, 3, 8)
	_total_pairs = pair_count

	# 기억 풀에서 랜덤 선택
	var memories = MemoryManager.memories.duplicate()
	memories.shuffle()
	var selected: Array[Dictionary] = []
	for i in range(mini(pair_count, memories.size())):
		selected.append({"id": memories[i].id, "title": memories[i].title})

	# 부족하면 더미 추가
	while selected.size() < pair_count:
		selected.append({"id": "dummy_%d" % selected.size(), "title": "Memory Fragment %d" % (selected.size() + 1)})

	# 쌍 만들기 + 셔플
	var card_data: Array = []
	for s in selected:
		card_data.append(s.duplicate())
		card_data.append(s.duplicate())
	card_data.shuffle()

	# 그리드 열 수 결정
	var cols = 4
	if pair_count <= 3:
		cols = 3
	elif pair_count >= 6:
		cols = 4
	grid.columns = cols

	# 카드 생성
	for i in range(card_data.size()):
		var card = card_data[i]
		card["idx"] = i
		card["flipped"] = false
		card["matched"] = false
		var btn = _create_card_button(i)
		card["node"] = btn
		grid.add_child(btn)
		_cards.append(card)

func _create_card_button(idx: int) -> Button:
	var btn = Button.new()
	btn.text = "?"
	btn.custom_minimum_size = Vector2(120, 80)
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.12, 0.1, 0.15, 0.95)
	style.border_color = Color(0.4, 0.35, 0.25, 0.6)
	style.set_border_width_all(2)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	btn.add_theme_stylebox_override("normal", style)
	var hover = style.duplicate()
	hover.border_color = Color(0.7, 0.55, 0.3, 0.9)
	hover.bg_color = Color(0.15, 0.12, 0.18, 0.95)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
	btn.add_theme_color_override("font_hover_color", Color(0.85, 0.7, 0.45))
	btn.pressed.connect(func(): _on_card_pressed(idx))
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 카드 로직 =====================

func _on_card_pressed(idx: int) -> void:
	if _locked:
		return
	if idx < 0 or idx >= _cards.size():
		return
	var card = _cards[idx]
	if card["flipped"] or card["matched"]:
		return

	# 카드 뒤집기
	card["flipped"] = true
	_reveal_card(idx)
	AudioManager.play_sfx("ui_select")

	if _first_pick == -1:
		_first_pick = idx
	elif _second_pick == -1:
		_second_pick = idx
		_attempts += 1
		_update_info()
		_locked = true
		# 매칭 체크 (짧은 딜레이 후)
		await get_tree().create_timer(0.8).timeout
		_check_match()

func _reveal_card(idx: int) -> void:
	var card = _cards[idx]
	var btn: Button = card["node"]
	var title: String = card["title"]
	# 짧게 표시 (최대 12자)
	if title.length() > 14:
		title = title.substr(0, 12) + ".."
	btn.text = title
	btn.add_theme_color_override("font_color", Color(0.85, 0.75, 0.5))
	var style: StyleBoxFlat = btn.get_theme_stylebox("normal").duplicate()
	style.bg_color = Color(0.18, 0.14, 0.1, 0.95)
	style.border_color = Color(0.7, 0.55, 0.3, 0.8)
	btn.add_theme_stylebox_override("normal", style)

func _hide_card(idx: int) -> void:
	var card = _cards[idx]
	var btn: Button = card["node"]
	btn.text = "?"
	btn.add_theme_color_override("font_color", Color(0.6, 0.55, 0.5))
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.12, 0.1, 0.15, 0.95)
	style.border_color = Color(0.4, 0.35, 0.25, 0.6)
	style.set_border_width_all(2)
	style.set_corner_radius_all(4)
	style.set_content_margin_all(8)
	btn.add_theme_stylebox_override("normal", style)

func _mark_matched(idx: int) -> void:
	var card = _cards[idx]
	card["matched"] = true
	var btn: Button = card["node"]
	btn.disabled = true
	btn.add_theme_color_override("font_color", Color(0.4, 0.65, 0.4))
	var style: StyleBoxFlat = btn.get_theme_stylebox("normal").duplicate()
	style.bg_color = Color(0.08, 0.15, 0.08, 0.8)
	style.border_color = Color(0.3, 0.5, 0.3, 0.6)
	btn.add_theme_stylebox_override("normal", style)

func _check_match() -> void:
	var a = _cards[_first_pick]
	var b = _cards[_second_pick]

	if a["id"] == b["id"]:
		# 매칭 성공
		_mark_matched(_first_pick)
		_mark_matched(_second_pick)
		_matches_found += 1
		AudioManager.play_sfx("confirm")

		if _matches_found >= _total_pairs:
			_on_puzzle_complete()
	else:
		# 매칭 실패 — 다시 뒤집기
		_hide_card(_first_pick)
		_hide_card(_second_pick)
		a["flipped"] = false
		b["flipped"] = false

	_first_pick = -1
	_second_pick = -1
	_locked = false
	_update_info()

func _on_puzzle_complete() -> void:
	# 보상 계산 (시도 횟수가 적을수록 보너스)
	var bonus = maxi(0, (_total_pairs * 3) - _attempts) * 2
	var total_reward = _reward_grains + bonus
	GameManager.player_data.grains += total_reward
	AchievementManager.check_grains()

	info_label.text = "COMPLETE! +%d Grains (bonus: %d)" % [total_reward, bonus]
	info_label.add_theme_color_override("font_color", Color(0.5, 0.8, 0.45))
	NotificationToast.show_toast("+%d Grains (Puzzle)" % total_reward, NotificationToast.ToastType.SUCCESS)

	# 자동 닫기
	await get_tree().create_timer(2.0).timeout
	close_puzzle()

func _update_info() -> void:
	info_label.text = "Matches: %d / %d  |  Attempts: %d" % [_matches_found, _total_pairs, _attempts]
	info_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))

## ===================== UI 구축 =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.7)
	overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	root.add_child(overlay)

	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.15
	main_panel.anchor_right = 0.85
	main_panel.anchor_top = 0.08
	main_panel.anchor_bottom = 0.92
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.07, 0.06, 0.08, 0.96), Color(0.45, 0.35, 0.25, 0.7), 2, 6, 20
	))
	root.add_child(main_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	main_panel.add_child(vbox)

	# 타이틀
	var title = Label.new()
	title.text = "MEMORY MATCH"
	title.add_theme_font_size_override("font_size", 20)
	title.add_theme_color_override("font_color", Color(0.8, 0.7, 0.5))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(title)

	var subtitle = Label.new()
	subtitle.text = "Find matching pairs of memories"
	subtitle.add_theme_font_size_override("font_size", 12)
	subtitle.add_theme_color_override("font_color", Color(0.5, 0.45, 0.4))
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(subtitle)

	# 정보 레이블
	info_label = Label.new()
	info_label.add_theme_font_size_override("font_size", 14)
	info_label.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	info_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(info_label)

	var sep = HSeparator.new()
	vbox.add_child(sep)

	# 카드 그리드 (스크롤 가능)
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	vbox.add_child(scroll)

	var center = CenterContainer.new()
	center.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	center.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.add_child(center)

	grid = GridContainer.new()
	grid.columns = 4
	grid.add_theme_constant_override("h_separation", 10)
	grid.add_theme_constant_override("v_separation", 10)
	center.add_child(grid)

	# 닫기 힌트
	var hint = Label.new()
	hint.text = "[ESC] Close"
	hint.add_theme_font_size_override("font_size", 11)
	hint.add_theme_color_override("font_color", Color(0.4, 0.35, 0.3))
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	vbox.add_child(hint)
