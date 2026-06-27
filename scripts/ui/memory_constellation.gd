## MemoryConstellation (Autoload)
## S62: 기억 성좌 UI — 기억들을 네트워크 노드로 시각화. 연결된 기억 간 선, 태워진 기억 금/잠금.
## MemoryUI(Tab/M) 안에서 "Constellation" 버튼으로 토글.
extends CanvasLayer

const CONSTELLATION_BACKDROP_PATH: String = "res://assets/cg/generated/ui_memory_constellation_backdrop.png"

const GRADE_RADIUS: Dictionary = {
	0: 380.0,  # GRADE_5 (최외곽)
	1: 320.0,
	2: 250.0,
	3: 170.0,
	4: 80.0,   # GRADE_1 (중심)
}

const GRADE_COLOR: Dictionary = {
	0: Color(0.55, 0.6, 0.5, 0.9),        # 감각
	1: Color(0.65, 0.75, 0.55, 0.95),     # 일상
	2: Color(0.75, 0.7, 0.5, 1.0),        # 관계
	3: Color(0.9, 0.75, 0.4, 1.0),        # 정체성
	4: Color(1.0, 0.85, 0.55, 1.0),       # 핵심
}

const NPC_COLORS: Dictionary = {
	"Elia": Color(0.6, 0.85, 0.7, 0.75),
	"Malet": Color(0.8, 0.7, 0.9, 0.7),
	"Kairos": Color(0.9, 0.5, 0.45, 0.75),
	"Sable": Color(0.55, 0.75, 0.95, 0.7),
	"Tobias": Color(0.85, 0.8, 0.65, 0.7),
	"Unknown": Color(0.7, 0.65, 0.6, 0.6),
}

const NODE_RADIUS: float = 22.0

var is_open: bool = false
var _root: Control
var _canvas: Control  # 커스텀 _draw
var _tooltip: PanelContainer
var _tooltip_label: RichTextLabel
var _bg: TextureRect
var _shade: ColorRect
var _close_btn: Button
var _node_positions: Dictionary = {}  # memory_id → Vector2
var _hovered_id: String = ""
var _time: float = 0.0

func _ready() -> void:
	layer = 42  # MemoryUI(40)보다 위, DialogueBox(50)보다 아래
	_build_ui()
	visible = false
	set_process(false)

func _build_ui() -> void:
	_root = Control.new()
	_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(_root)

	_bg = TextureRect.new()
	_bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	_bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if ResourceLoader.exists(CONSTELLATION_BACKDROP_PATH):
		_bg.texture = load(CONSTELLATION_BACKDROP_PATH)
	_root.add_child(_bg)

	_shade = ColorRect.new()
	_shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	_shade.color = Color(0.01, 0.008, 0.02, 0.28)
	_shade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.add_child(_shade)

	# 타이틀
	var title = Label.new()
	title.text = "기억 성좌" if GameManager.current_locale == "ko" else "MEMORY CONSTELLATION"
	title.anchor_left = 0.0
	title.anchor_right = 1.0
	title.anchor_top = 0.0
	title.offset_top = 20
	title.offset_bottom = 60
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.95, 0.85, 0.55))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	UITheme.apply_title_font(title)
	_root.add_child(title)

	var subtitle = Label.new()
	subtitle.text = "지닌 것과 태워버린 것이 같은 하늘에 남는다." if GameManager.current_locale == "ko" else "What you carry. What you have spent."
	subtitle.anchor_left = 0.0
	subtitle.anchor_right = 1.0
	subtitle.offset_top = 58
	subtitle.offset_bottom = 80
	subtitle.add_theme_font_size_override("font_size", 14)
	subtitle.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_root.add_child(subtitle)

	# 메인 캔버스 (커스텀 _draw)
	_canvas = Control.new()
	_canvas.set_anchors_preset(Control.PRESET_FULL_RECT)
	_canvas.offset_top = 90
	_canvas.offset_bottom = -90
	_canvas.mouse_filter = Control.MOUSE_FILTER_STOP
	_canvas.draw.connect(_on_draw)
	_canvas.gui_input.connect(_on_canvas_input)
	_root.add_child(_canvas)

	# 툴팁
	_tooltip = PanelContainer.new()
	_tooltip.visible = false
	var tstyle = StyleBoxFlat.new()
	tstyle.bg_color = Color(0.08, 0.06, 0.1, 0.96)
	tstyle.border_color = Color(0.75, 0.6, 0.35, 1.0)
	tstyle.set_border_width_all(2)
	tstyle.set_content_margin_all(12)
	tstyle.set_corner_radius_all(4)
	_tooltip.add_theme_stylebox_override("panel", tstyle)
	_tooltip.custom_minimum_size = Vector2(320, 0)
	_tooltip.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_root.add_child(_tooltip)

	_tooltip_label = RichTextLabel.new()
	_tooltip_label.bbcode_enabled = true
	_tooltip_label.fit_content = true
	_tooltip_label.custom_minimum_size = Vector2(296, 0)
	_tooltip_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_tooltip.add_child(_tooltip_label)

	# 닫기 버튼
	_close_btn = Button.new()
	_close_btn.text = "닫기 (Esc)" if GameManager.current_locale == "ko" else "Close (Esc)"
	_close_btn.anchor_left = 1.0
	_close_btn.anchor_right = 1.0
	_close_btn.anchor_top = 0.0
	_close_btn.offset_left = -140
	_close_btn.offset_right = -20
	_close_btn.offset_top = 20
	_close_btn.offset_bottom = 52
	var close_style := UITheme.make_button_style(Color(0.045, 0.038, 0.06, 0.84), Color(0.5, 0.4, 0.25, 0.68))
	_close_btn.add_theme_stylebox_override("normal", close_style)
	_close_btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(close_style))
	_close_btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	_close_btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	_close_btn.pressed.connect(close)
	_root.add_child(_close_btn)

	# 범례 (하단)
	var legend = Label.new()
	legend.anchor_left = 0.0
	legend.anchor_right = 1.0
	legend.anchor_top = 1.0
	legend.anchor_bottom = 1.0
	legend.offset_top = -70
	legend.offset_bottom = -20
	legend.add_theme_font_size_override("font_size", 12)
	legend.add_theme_color_override("font_color", Color(0.7, 0.65, 0.55))
	legend.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	legend.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	legend.text = ("바깥 고리: 감각 기억  →  중심: 핵심 기억    선: 인물·범주의 연결    균열: 연소의 흔적    X: 완전 소실" if GameManager.current_locale == "ko" else "Rings: sensory memories outside → core memories within.   Lines: shared bonds.   Cracks: burn residue.   X: gone.")
	_root.add_child(legend)

func open() -> void:
	if is_open:
		return
	is_open = true
	visible = true
	set_process(true)
	_root.modulate.a = 0.0
	var tween := create_tween()
	tween.tween_property(_root, "modulate:a", 1.0, 0.32).set_ease(Tween.EASE_OUT)
	_compute_positions()
	_canvas.queue_redraw()
	# MemoryUI가 열려 있으면 겹침 방지 위해 잠깐 가림
	if has_node("/root/MemoryUI") and MemoryUI.visible:
		MemoryUI.visible = false
		set_meta("reopen_memory_ui", true)
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_open")

func close() -> void:
	if not is_open:
		return
	is_open = false
	visible = false
	set_process(false)
	_tooltip.visible = false
	_hovered_id = ""
	if get_meta("reopen_memory_ui", false) and has_node("/root/MemoryUI"):
		MemoryUI.visible = true
		set_meta("reopen_memory_ui", false)
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_close")

func _process(delta: float) -> void:
	_time += delta
	_canvas.queue_redraw()  # 맥동 애니메이션

func _unhandled_input(event: InputEvent) -> void:
	if not is_open:
		return
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ESCAPE:
			close()
			get_viewport().set_input_as_handled()

## ===================== 레이아웃 =====================

func _scaled_grade_radius(grade: int) -> float:
	# 1280x720의 실제 캔버스 높이에서도 바깥 등급이 화면 밖으로 잘리지 않게 맞춘다.
	var max_radius := minf(_canvas.size.x * 0.42, _canvas.size.y * 0.42)
	var normalized := float(GRADE_RADIUS.get(grade, 250.0)) / float(GRADE_RADIUS[0])
	return maxf(46.0, max_radius * normalized)

func _compute_positions() -> void:
	_node_positions.clear()
	var center = _canvas.size / 2.0
	# 등급별 동심원 배치. 각 등급 내에서 NPC별로 각도 클러스터링.
	var by_grade: Dictionary = {}
	for m in MemoryManager.memories:
		if not by_grade.has(m.grade):
			by_grade[m.grade] = []
		by_grade[m.grade].append(m)
	# 잔존(is_residue)은 연결 유지, 완전 소실된(is_burned && !is_residue) 기억도 포함 (어두운 자리로 표시)
	for m in MemoryManager.burned_memories:
		if not m.is_residue:
			if not by_grade.has(m.grade):
				by_grade[m.grade] = []
			# 중복 방지
			var exists = false
			for n in by_grade[m.grade]:
				if n.id == m.id:
					exists = true
					break
			if not exists:
				by_grade[m.grade].append(m)

	for grade in by_grade.keys():
		var group: Array = by_grade[grade]
		var radius := _scaled_grade_radius(int(grade))
		var count = group.size()
		# 각 등급이 모두 12시 방향에서 시작하면 적은 수의 기억이 한 줄로 겹친다.
		# 등급마다 위상을 돌려 성좌가 화면 전체에 자연스럽게 펼쳐지게 한다.
		var grade_phase := -PI / 2.0 + float(int(grade)) * 0.78
		for i in range(count):
			var angle := grade_phase if count == 1 else grade_phase + TAU * float(i) / float(count)
			# NPC가 같으면 서로 약간 가깝게 몰기 — 간단히 id 해시 오프셋
			var npc_offset = 0.0
			if group[i].related_npc != "":
				npc_offset = sin(group[i].related_npc.hash() * 0.001) * 0.15
			angle += npc_offset
			var pos = center + Vector2(cos(angle), sin(angle)) * radius
			_node_positions[group[i].id] = pos

## ===================== 드로잉 =====================

func _on_draw() -> void:
	var center = _canvas.size / 2.0

	# 배경 링 (등급 가이드)
	for grade_key in GRADE_RADIUS.keys():
		var r := _scaled_grade_radius(int(grade_key))
		_canvas.draw_arc(center, r, 0, TAU, 64, Color(0.3, 0.28, 0.25, 0.18), 1.0, false)

	# 연결선 먼저 (노드 아래에 깔림)
	_draw_connections()

	# 노드
	for m in MemoryManager.memories:
		if _node_positions.has(m.id):
			_draw_node(m, _node_positions[m.id])
	for m in MemoryManager.burned_memories:
		if _node_positions.has(m.id) and not m.is_residue:
			_draw_node(m, _node_positions[m.id])

func _draw_connections() -> void:
	var drawn: Dictionary = {}  # "a|b" 쌍 중복 방지
	for m in MemoryManager.memories + MemoryManager.burned_memories:
		if not _node_positions.has(m.id):
			continue
		for cid in m.connections:
			var key = m.id + "|" + cid if m.id < cid else cid + "|" + m.id
			if drawn.has(key):
				continue
			drawn[key] = true
			if not _node_positions.has(cid):
				continue
			var other = MemoryManager.find_memory(cid)
			if other == null:
				continue
			var color = _connection_color(m, other)
			var width = 1.5
			# 둘 중 하나라도 태워졌으면 선이 끊긴 듯 점선 + 붉은 톤
			if m.is_burned or other.is_burned:
				color = Color(0.7, 0.3, 0.25, 0.45)
				width = 1.0
				_draw_broken_line(_node_positions[m.id], _node_positions[cid], color, width)
			else:
				_canvas.draw_line(_node_positions[m.id], _node_positions[cid], color, width, true)

func _connection_color(a, b) -> Color:
	# 두 기억이 공통 NPC를 가지면 그 NPC 색, 아니면 옅은 회색
	if a.related_npc != "" and a.related_npc == b.related_npc:
		return NPC_COLORS.get(a.related_npc, Color(0.6, 0.55, 0.5, 0.55))
	return Color(0.45, 0.42, 0.38, 0.35)

func _draw_broken_line(a: Vector2, b: Vector2, color: Color, width: float) -> void:
	# 짧은 조각들로 점선 효과
	var dir = (b - a)
	var dist = dir.length()
	if dist < 1:
		return
	dir = dir / dist
	var t = 0.0
	var dash = 8.0
	var gap = 6.0
	while t < dist:
		var s = a + dir * t
		var e = a + dir * min(t + dash, dist)
		_canvas.draw_line(s, e, color, width, true)
		t += dash + gap

func _draw_node(m, pos: Vector2) -> void:
	var grade_color = GRADE_COLOR.get(m.grade, Color(0.7, 0.7, 0.7))
	var is_hovered = (m.id == _hovered_id)
	var pulse = 1.0 + sin(_time * 2.0 + m.id.hash() * 0.001) * 0.06
	var r = NODE_RADIUS * (1.15 if is_hovered else 1.0) * pulse

	# 태워지고 잔존 없음: 어두운 링만
	if m.is_burned and not m.is_residue:
		_canvas.draw_arc(pos, r, 0, TAU, 24, Color(0.35, 0.18, 0.15, 0.85), 2.0, true)
		# X 표시
		var off = r * 0.5
		_canvas.draw_line(pos + Vector2(-off, -off), pos + Vector2(off, off), Color(0.8, 0.35, 0.3, 0.9), 2.5, true)
		_canvas.draw_line(pos + Vector2(-off, off), pos + Vector2(off, -off), Color(0.8, 0.35, 0.3, 0.9), 2.5, true)
		return

	# 잔존(태워졌지만 흔적): 흐릿한 노드
	if m.is_residue:
		_canvas.draw_circle(pos, r, Color(grade_color.r * 0.4, grade_color.g * 0.4, grade_color.b * 0.4, 0.5))
		_canvas.draw_arc(pos, r, 0, TAU, 24, Color(0.7, 0.5, 0.35, 0.7), 1.5, true)
		_canvas.draw_string(ThemeDB.fallback_font, pos + Vector2(-5, 5), "~",
			HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.85, 0.7, 0.5, 0.8))
		return

	# 정상 노드
	_canvas.draw_circle(pos, r, Color(grade_color.r, grade_color.g, grade_color.b, 0.85))
	_canvas.draw_arc(pos, r, 0, TAU, 32, Color(0.95, 0.85, 0.55, 0.95 if is_hovered else 0.6), 2.0 if is_hovered else 1.5, true)

	# 주변 태워진 이웃 수에 따른 금
	var burned_count = MemoryManager.burned_neighbor_count(m.id)
	if burned_count > 0:
		for i in range(min(burned_count, 3)):
			var angle = TAU * float(i) / 3.0 + m.id.hash() * 0.01
			var start = pos + Vector2(cos(angle), sin(angle)) * (r * 0.3)
			var end = pos + Vector2(cos(angle + 0.4), sin(angle + 0.4)) * r
			_canvas.draw_line(start, end, Color(0.9, 0.4, 0.3, 0.7), 1.2, true)

## ===================== 입력 (호버/클릭) =====================

func _on_canvas_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var mp = event.position
		var found = ""
		for mid in _node_positions.keys():
			if _node_positions[mid].distance_to(mp) <= NODE_RADIUS * 1.2:
				found = mid
				break
		if found != _hovered_id:
			_hovered_id = found
			_update_tooltip(mp)
			_canvas.queue_redraw()

func _update_tooltip(mouse_pos: Vector2) -> void:
	if _hovered_id == "":
		_tooltip.visible = false
		return
	var m = MemoryManager.find_memory(_hovered_id)
	if m == null:
		# burned_memories 도 확인
		for bm in MemoryManager.burned_memories:
			if bm.id == _hovered_id:
				m = bm
				break
	if m == null:
		return

	var grade_name = ["Sensory", "Daily", "Relational", "Identity", "Core"][m.grade]
	var state = "Intact"
	if m.is_burned and not m.is_residue:
		state = "[color=#cc4d3d]Burned (gone)[/color]"
	elif m.is_residue:
		state = "[color=#d4a874]Residue[/color]"
	elif m.is_faded:
		state = "[color=#8a8078]Faded[/color]"
	var npc_line = ""
	if m.related_npc != "":
		npc_line = "\n[color=#a8968a]Bond:[/color] %s" % m.related_npc
	var text = "[b][color=#f5d98c]%s[/color][/b]\n[color=#a8968a]%s · %s[/color]%s\n\n%s" % [
		m.title, grade_name, state, npc_line, m.description
	]
	if m.story_effect != "":
		text += "\n\n[color=#c08878][i]If burned: %s[/i][/color]" % m.story_effect
	_tooltip_label.text = text

	# 위치: 마우스 근처, 화면 밖 방지
	var pos = mouse_pos + Vector2(20, 20) + _canvas.position
	var vp_size = get_viewport().get_visible_rect().size
	if pos.x + 340 > vp_size.x:
		pos.x = mouse_pos.x - 340 + _canvas.position.x
	if pos.y + 200 > vp_size.y:
		pos.y = mouse_pos.y - 200 + _canvas.position.y
	_tooltip.position = pos
	_tooltip.visible = true
