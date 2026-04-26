## VNScene — 비주얼 노벨 씬 UI
## S60: SceneFlow 오토로드가 구동하는 CG + 포트레이트 + 텍스트 렌더러.
## 클릭/Enter/Space/E로 진행. 선택지는 마우스 클릭.
extends CanvasLayer

const PORTRAIT_SIZE: int = 384  # VN 스탠딩 크기 (포트레이트 원본을 크게 보여줌)
const TYPEWRITER_SPEED: float = 0.025
const CG_FADE_DURATION: float = 0.8
const PORTRAIT_DIM: Color = Color(0.45, 0.45, 0.5, 1.0)
const PORTRAIT_BRIGHT: Color = Color(1, 1, 1, 1)

# 노드
var _bg: ColorRect
var _cg_current: TextureRect
var _cg_next: TextureRect  # 크로스페이드용
var _portrait_left: TextureRect
var _portrait_right: TextureRect
var _name_label: Label
var _name_panel: PanelContainer
var _text_label: RichTextLabel
var _text_panel: PanelContainer
var _continue_indicator: Label
var _choice_container: VBoxContainer
var _letterbox_top: ColorRect
var _letterbox_bottom: ColorRect

# S61: 기억 왜곡 VFX
var _glitch_overlay: ColorRect          # 전체화면 플래시/색상 틴트
var _chroma_r: TextureRect              # 색수차 레이어 (붉은 채널)
var _chroma_b: TextureRect              # 색수차 레이어 (푸른 채널)
var _is_distorted_line: bool = false    # 현재 대사가 왜곡 상태인지

# 상태
var _current_step: Dictionary = {}
var _typing_done: bool = true
var _typed_chars: int = 0
var _full_text: String = ""
var _type_timer: float = 0.0
var _waiting_for_input: bool = false
var _active_side: String = ""  # "left" or "right" (말하는 쪽)

# 포트레이트 슬롯 상태
var _left_portrait_id: String = ""
var _right_portrait_id: String = ""

# 포트레이트 맵 참조 (DialogueBox의 것과 동일)
var _portrait_map: Dictionary = {}

func _ready() -> void:
	layer = 50
	_load_portrait_map()
	_build_ui()
	_build_glitch_layer()
	SceneFlow.step_changed.connect(_on_step_changed)
	SceneFlow.scene_ended.connect(_on_scene_ended)
	# S61: 기억 연소 시 글리치 VFX 트리거
	if has_node("/root/MemoryManager"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	set_process_input(true)
	set_process(true)

func _load_portrait_map() -> void:
	# DialogueBox 오토로드에서 포트레이트 매핑 공유
	if has_node("/root/DialogueBox") and "PORTRAIT_MAP" in DialogueBox:
		_portrait_map = DialogueBox.PORTRAIT_MAP.duplicate()

## ===================== UI BUILD =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_PASS
	add_child(root)

	# 배경 (검은색)
	_bg = ColorRect.new()
	_bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	_bg.color = Color(0.02, 0.02, 0.03, 1)
	_bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_bg)

	# CG 레이어 — 현재 + 다음(크로스페이드)
	_cg_current = _make_cg_rect()
	_cg_current.modulate = Color(1, 1, 1, 0)
	root.add_child(_cg_current)

	_cg_next = _make_cg_rect()
	_cg_next.modulate = Color(1, 1, 1, 0)
	root.add_child(_cg_next)

	# 레터박스 (연출용)
	_letterbox_top = ColorRect.new()
	_letterbox_top.anchor_left = 0.0
	_letterbox_top.anchor_right = 1.0
	_letterbox_top.anchor_top = 0.0
	_letterbox_top.anchor_bottom = 0.0
	_letterbox_top.offset_bottom = 60
	_letterbox_top.color = Color(0, 0, 0, 0.85)
	_letterbox_top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_letterbox_top)

	_letterbox_bottom = ColorRect.new()
	_letterbox_bottom.anchor_left = 0.0
	_letterbox_bottom.anchor_right = 1.0
	_letterbox_bottom.anchor_top = 1.0
	_letterbox_bottom.anchor_bottom = 1.0
	_letterbox_bottom.offset_top = -60
	_letterbox_bottom.color = Color(0, 0, 0, 0.85)
	_letterbox_bottom.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_letterbox_bottom)

	# 포트레이트 (좌/우)
	_portrait_left = _make_portrait_rect(true)
	root.add_child(_portrait_left)
	_portrait_right = _make_portrait_rect(false)
	root.add_child(_portrait_right)

	# 대화 박스 패널
	_text_panel = PanelContainer.new()
	_text_panel.anchor_left = 0.08
	_text_panel.anchor_right = 0.92
	_text_panel.anchor_top = 1.0
	_text_panel.anchor_bottom = 1.0
	_text_panel.offset_top = -200
	_text_panel.offset_bottom = -30
	_text_panel.mouse_filter = Control.MOUSE_FILTER_PASS
	var tstyle = StyleBoxFlat.new()
	tstyle.bg_color = Color(0.04, 0.04, 0.08, 0.88)
	tstyle.border_color = Color(0.6, 0.5, 0.35, 0.7)
	tstyle.set_border_width_all(2)
	tstyle.set_content_margin_all(20)
	tstyle.set_corner_radius_all(6)
	_text_panel.add_theme_stylebox_override("panel", tstyle)
	root.add_child(_text_panel)

	_text_label = RichTextLabel.new()
	_text_label.bbcode_enabled = true
	_text_label.fit_content = false
	_text_label.scroll_active = false
	_text_label.add_theme_font_size_override("normal_font_size", 20)
	_text_label.add_theme_color_override("default_color", Color(0.94, 0.91, 0.84))
	_text_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_text_panel.add_child(_text_label)

	# 이름 패널 (대화박스 위)
	_name_panel = PanelContainer.new()
	_name_panel.anchor_left = 0.08
	_name_panel.anchor_right = 0.08
	_name_panel.anchor_top = 1.0
	_name_panel.anchor_bottom = 1.0
	_name_panel.offset_left = 10
	_name_panel.offset_top = -236
	_name_panel.offset_bottom = -200
	_name_panel.offset_right = 180
	var nstyle = StyleBoxFlat.new()
	nstyle.bg_color = Color(0.12, 0.09, 0.05, 0.95)
	nstyle.border_color = Color(0.75, 0.6, 0.35, 0.9)
	nstyle.set_border_width_all(2)
	nstyle.set_content_margin_all(6)
	nstyle.set_corner_radius_all(4)
	_name_panel.add_theme_stylebox_override("panel", nstyle)
	_name_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(_name_panel)

	_name_label = Label.new()
	_name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_name_label.add_theme_font_size_override("font_size", 18)
	_name_label.add_theme_color_override("font_color", Color(0.95, 0.85, 0.55))
	_name_panel.add_child(_name_label)
	_name_panel.visible = false

	# 계속 표시 화살표
	_continue_indicator = Label.new()
	_continue_indicator.text = "▼"
	_continue_indicator.anchor_left = 1.0
	_continue_indicator.anchor_right = 1.0
	_continue_indicator.anchor_top = 1.0
	_continue_indicator.anchor_bottom = 1.0
	_continue_indicator.offset_left = -130
	_continue_indicator.offset_right = -90
	_continue_indicator.offset_top = -55
	_continue_indicator.offset_bottom = -25
	_continue_indicator.add_theme_font_size_override("font_size", 20)
	_continue_indicator.add_theme_color_override("font_color", Color(0.9, 0.8, 0.5, 0.85))
	_continue_indicator.visible = false
	root.add_child(_continue_indicator)

	# 선택지 컨테이너
	_choice_container = VBoxContainer.new()
	_choice_container.anchor_left = 0.25
	_choice_container.anchor_right = 0.75
	_choice_container.anchor_top = 0.28
	_choice_container.anchor_bottom = 0.75
	_choice_container.add_theme_constant_override("separation", 12)
	_choice_container.alignment = BoxContainer.ALIGNMENT_CENTER
	root.add_child(_choice_container)
	_choice_container.visible = false

func _build_glitch_layer() -> void:
	# 색수차 복사본 (CG 위에 R/B 채널 오프셋)
	_chroma_r = _make_cg_rect()
	_chroma_r.modulate = Color(1, 0, 0, 0)
	_chroma_r.z_index = 1
	add_child(_chroma_r)

	_chroma_b = _make_cg_rect()
	_chroma_b.modulate = Color(0, 0.4, 1, 0)
	_chroma_b.z_index = 1
	add_child(_chroma_b)

	# 전체화면 글리치 오버레이
	_glitch_overlay = ColorRect.new()
	_glitch_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	_glitch_overlay.color = Color(0.85, 0.2, 0.15, 0)
	_glitch_overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_glitch_overlay.z_index = 2
	add_child(_glitch_overlay)

func _make_cg_rect() -> TextureRect:
	var tr = TextureRect.new()
	tr.set_anchors_preset(Control.PRESET_FULL_RECT)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return tr

func _make_portrait_rect(is_left: bool) -> TextureRect:
	var tr = TextureRect.new()
	tr.custom_minimum_size = Vector2(PORTRAIT_SIZE, PORTRAIT_SIZE)
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT
	tr.anchor_top = 1.0
	tr.anchor_bottom = 1.0
	tr.offset_top = -PORTRAIT_SIZE - 180
	tr.offset_bottom = -180
	if is_left:
		tr.anchor_left = 0.0
		tr.anchor_right = 0.0
		tr.offset_left = 20
		tr.offset_right = PORTRAIT_SIZE + 20
	else:
		tr.anchor_left = 1.0
		tr.anchor_right = 1.0
		tr.offset_left = -PORTRAIT_SIZE - 20
		tr.offset_right = -20
	tr.modulate = PORTRAIT_DIM
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tr.visible = false
	return tr

## ===================== STEP HANDLER =====================

func _on_step_changed(step: Dictionary) -> void:
	_current_step = step
	_is_distorted_line = step.get("_distorted", false)

	# CG 변경
	if step.has("cg"):
		_change_cg(step.cg, float(step.get("fade", CG_FADE_DURATION)))

	# S61: 왜곡 라인은 약한 색수차 + 대사 사전 글리치
	if _is_distorted_line:
		_play_subtle_distortion()

	# 포트레이트 변경/제거
	if step.has("hide_left"):
		_set_portrait_side("left", "")
	if step.has("hide_right"):
		_set_portrait_side("right", "")

	# System log (시스템 메시지)
	if step.has("system_log"):
		_show_system_log(step.system_log)
		_waiting_for_input = true
		_show_continue(true)
		return

	# 선택지
	if step.has("choice"):
		_show_choices(step.choice)
		return

	# 일반 대사 / 나레이션
	var speaker: String = step.get("speaker", "")
	var text: String = step.get("text", step.get("narrate", ""))
	var portrait: String = step.get("portrait", "")
	var side: String = step.get("side", "")

	if portrait != "" and side != "":
		_set_portrait_side(side, portrait)
		_active_side = side
		_highlight_speaking_side(side)
	elif speaker == "":
		# 나레이션 — 양쪽 포트레이트 어둡게
		_active_side = ""
		_highlight_speaking_side("")

	_display_line(speaker, text)

func _on_scene_ended(_id: String) -> void:
	# UI는 SceneFlow가 _close_vn_ui에서 제거
	pass

## ===================== CG =====================

func _change_cg(cg_ref: String, fade: float) -> void:
	var path = _resolve_cg_path(cg_ref)
	if path == "":
		return
	if not ResourceLoader.exists(path):
		push_warning("[VNScene] CG not found: %s" % path)
		return
	var tex = load(path)
	if tex == null:
		return

	# 크로스페이드: next에 새 이미지 올리고 페이드인, 끝나면 current와 교체
	_cg_next.texture = tex
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(_cg_next, "modulate:a", 1.0, fade)
	tw.tween_property(_cg_current, "modulate:a", 0.0, fade)
	tw.chain().tween_callback(Callable(self, "_swap_cg"))

func _swap_cg() -> void:
	_cg_current.texture = _cg_next.texture
	_cg_current.modulate.a = 1.0
	_cg_next.texture = null
	_cg_next.modulate.a = 0.0

func _resolve_cg_path(ref: String) -> String:
	if ref == "":
		return ""
	if ref.begins_with("res://"):
		return ref
	# 짧은 이름 → cg 폴더에서 jpg/png 자동 탐색
	for ext in [".jpg", ".png"]:
		var p = "res://assets/cg/" + ref + ext
		if ResourceLoader.exists(p):
			return p
	return "res://assets/cg/" + ref + ".jpg"

## ===================== PORTRAIT =====================

func _set_portrait_side(side: String, portrait_id: String) -> void:
	var target: TextureRect = _portrait_left if side == "left" else _portrait_right

	if portrait_id == "":
		target.visible = false
		if side == "left":
			_left_portrait_id = ""
		else:
			_right_portrait_id = ""
		return

	if (side == "left" and _left_portrait_id == portrait_id) or \
	   (side == "right" and _right_portrait_id == portrait_id):
		target.visible = true
		return

	var path = _portrait_map.get(portrait_id, "")
	if path == "" or not ResourceLoader.exists(path):
		# 폴백
		target.visible = false
		return

	target.texture = load(path)
	target.visible = true

	if side == "left":
		_left_portrait_id = portrait_id
	else:
		_right_portrait_id = portrait_id

func _highlight_speaking_side(side: String) -> void:
	if side == "left":
		_portrait_left.modulate = PORTRAIT_BRIGHT
		_portrait_right.modulate = PORTRAIT_DIM
	elif side == "right":
		_portrait_right.modulate = PORTRAIT_BRIGHT
		_portrait_left.modulate = PORTRAIT_DIM
	else:
		_portrait_left.modulate = PORTRAIT_DIM
		_portrait_right.modulate = PORTRAIT_DIM

## ===================== S61: 기억 왜곡 VFX =====================

## 기억 연소 순간 — 강한 글리치 (붉은 플래시 + 색수차 분리 + 셰이크)
func _on_memory_burned(_memory) -> void:
	if not visible:
		return
	_play_burn_glitch()

func _play_burn_glitch() -> void:
	# 1. 붉은 플래시
	_glitch_overlay.color = Color(0.95, 0.25, 0.15, 0.55)
	var tw_flash = create_tween()
	tw_flash.tween_property(_glitch_overlay, "color:a", 0.0, 0.9).set_trans(Tween.TRANS_EXPO)

	# 2. 색수차 분리 (CG가 있을 때만)
	if _cg_current.texture != null:
		_chroma_r.texture = _cg_current.texture
		_chroma_b.texture = _cg_current.texture
		_chroma_r.modulate.a = 0.55
		_chroma_b.modulate.a = 0.55
		_chroma_r.position = Vector2(-8, -2)
		_chroma_b.position = Vector2(8, 2)
		var tw_ch = create_tween()
		tw_ch.set_parallel(true)
		tw_ch.tween_property(_chroma_r, "position", Vector2.ZERO, 0.7).set_trans(Tween.TRANS_EXPO)
		tw_ch.tween_property(_chroma_b, "position", Vector2.ZERO, 0.7).set_trans(Tween.TRANS_EXPO)
		tw_ch.tween_property(_chroma_r, "modulate:a", 0.0, 0.7)
		tw_ch.tween_property(_chroma_b, "modulate:a", 0.0, 0.7)

	# 3. 글리치 사운드 (있을 때만)
	if has_node("/root/AudioManager"):
		if AudioManager.has_method("play_sfx"):
			AudioManager.play_sfx("memory_burn")

	# 4. 텍스트 스크램블 (표시 중인 라인이 있으면 잠깐 깨진 문자로 치환 후 복원)
	if _text_label.text != "":
		var original = _text_label.text
		_text_label.text = _scramble_text(original)
		await get_tree().create_timer(0.12).timeout
		if is_instance_valid(_text_label):
			_text_label.text = original

## 왜곡된 대사에 붙는 약한 효과 (색수차만 약하게)
func _play_subtle_distortion() -> void:
	if _cg_current.texture == null:
		return
	_chroma_r.texture = _cg_current.texture
	_chroma_b.texture = _cg_current.texture
	_chroma_r.modulate.a = 0.25
	_chroma_b.modulate.a = 0.25
	_chroma_r.position = Vector2(-3, 0)
	_chroma_b.position = Vector2(3, 0)
	# 상주형: 다음 스텝에서 해제
	await get_tree().create_timer(1.2).timeout
	if not is_instance_valid(_chroma_r):
		return
	var tw = create_tween()
	tw.set_parallel(true)
	tw.tween_property(_chroma_r, "modulate:a", 0.0, 0.4)
	tw.tween_property(_chroma_b, "modulate:a", 0.0, 0.4)
	tw.tween_property(_chroma_r, "position", Vector2.ZERO, 0.4)
	tw.tween_property(_chroma_b, "position", Vector2.ZERO, 0.4)

func _scramble_text(source: String) -> String:
	var glitch_chars = "▓▒░█▄▀#@%&*?!"
	var out = ""
	for i in source.length():
		var c = source[i]
		if c == " " or c == "\n":
			out += c
		elif randf() < 0.7:
			out += glitch_chars[randi() % glitch_chars.length()]
		else:
			out += c
	return out

## ===================== TEXT =====================

func _display_line(speaker: String, text: String) -> void:
	_full_text = text
	_typed_chars = 0
	_typing_done = false
	_type_timer = 0.0
	_waiting_for_input = false
	_show_continue(false)

	if speaker == "":
		_name_panel.visible = false
	else:
		_name_label.text = speaker
		_name_label.add_theme_color_override("font_color", _color_for_speaker(speaker))
		_name_panel.visible = true

	_text_label.text = ""

func _color_for_speaker(speaker: String) -> Color:
	match speaker:
		"Arrel":
			return Color(0.95, 0.85, 0.55)
		"Elia":
			return Color(0.7, 0.9, 0.75)
		"Malet":
			return Color(0.85, 0.75, 0.9)
		"Kairos":
			return Color(0.9, 0.6, 0.5)
		"Sable":
			return Color(0.6, 0.75, 0.95)
		"Tobias":
			return Color(0.85, 0.8, 0.7)
		_:
			return Color(0.9, 0.9, 0.9)

func _process(delta: float) -> void:
	if _typing_done or _full_text == "":
		return
	_type_timer += delta
	while _type_timer >= TYPEWRITER_SPEED and _typed_chars < _full_text.length():
		_type_timer -= TYPEWRITER_SPEED
		_typed_chars += 1
		_text_label.text = _full_text.substr(0, _typed_chars)
	if _typed_chars >= _full_text.length():
		_typing_done = true
		_waiting_for_input = true
		_show_continue(true)

func _show_continue(on: bool) -> void:
	_continue_indicator.visible = on

## ===================== SYSTEM LOG =====================

func _show_system_log(msg: String) -> void:
	# 대화박스를 시스템 로그 스타일로 표시 (SYSTEM 라벨 + 청록색)
	_name_panel.visible = true
	_name_label.text = "SYSTEM"
	_name_label.add_theme_color_override("font_color", Color(0.5, 0.85, 0.95))
	_full_text = msg
	_typed_chars = msg.length()
	_typing_done = true
	_text_label.text = msg

## ===================== CHOICES =====================

func _show_choices(choices: Array) -> void:
	# 기존 버튼 제거
	for c in _choice_container.get_children():
		c.queue_free()

	_choice_container.visible = true
	_text_panel.visible = false
	_name_panel.visible = false
	_show_continue(false)

	for i in range(choices.size()):
		var c: Dictionary = choices[i]
		# 조건부 선택지 (기억 필요 등)
		if c.has("requires_memory_intact"):
			if MemoryManager.is_memory_burned(c.requires_memory_intact):
				continue

		# S63: Memory Leverage — cost_memory 필드가 있으면 태울 기억 상태 체크
		var cost_mem_id: String = c.get("cost_memory", c.get("burn_memory", ""))
		var cost_mem = null
		var is_cost_choice: bool = c.has("cost_memory")  # 명시적 cost (UI 하이라이트용)
		if cost_mem_id != "":
			cost_mem = MemoryManager.find_memory(cost_mem_id)
			# 이미 태워진 기억이면 이 선택지 비활성화 (leverage 불가)
			if cost_mem != null and cost_mem.is_burned:
				continue
			if cost_mem == null:
				# 소실된 기억도 선택 불가
				continue

		var btn = Button.new()
		var label_text = c.get("text", "...")
		if is_cost_choice and cost_mem != null:
			label_text = "✦  %s\n    [ Burn: %s ]" % [c.get("text", "..."), cost_mem.title]
		btn.text = label_text
		btn.custom_minimum_size = Vector2(560, 60 if is_cost_choice else 50)
		btn.add_theme_font_size_override("font_size", 18)
		var bstyle = StyleBoxFlat.new()
		bstyle.bg_color = Color(0.08, 0.06, 0.1, 0.92)
		bstyle.border_color = Color(0.6, 0.5, 0.35, 0.8)
		if is_cost_choice:
			bstyle.bg_color = Color(0.14, 0.08, 0.06, 0.94)
			bstyle.border_color = Color(0.9, 0.45, 0.3, 0.9)  # 주황/붉은 테두리로 "연소 선택" 강조
		bstyle.set_border_width_all(2)
		bstyle.set_content_margin_all(10)
		bstyle.set_corner_radius_all(4)
		btn.add_theme_stylebox_override("normal", bstyle)
		var hover = bstyle.duplicate()
		hover.bg_color = Color(0.18, 0.13, 0.08, 0.95)
		if is_cost_choice:
			hover.bg_color = Color(0.28, 0.12, 0.08, 0.98)
			hover.border_color = Color(1.0, 0.55, 0.35, 1.0)
		else:
			hover.border_color = Color(0.9, 0.75, 0.45, 1.0)
		btn.add_theme_stylebox_override("hover", hover)
		btn.add_theme_color_override("font_color", Color(0.94, 0.9, 0.82))
		btn.pressed.connect(_on_choice_selected.bind(i))
		_choice_container.add_child(btn)

func _on_choice_selected(index: int) -> void:
	if has_node("/root/AudioManager"):
		AudioManager.play_sfx("ui_select")
	_choice_container.visible = false
	_text_panel.visible = true
	SceneFlow.select_choice(index)

## ===================== INPUT =====================

func _input(event: InputEvent) -> void:
	# S61b: VN 런너가 비활성일 때는 입력 처리 안 함 (탐색 맵 DialogueBox에 입력 양보)
	if not SceneFlow.is_active:
		return
	if _choice_container.visible:
		return  # 선택지 중에는 진행 불가

	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_try_advance()
	elif event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ENTER or event.keycode == KEY_SPACE or event.keycode == KEY_E:
			_try_advance()

func _try_advance() -> void:
	if not _typing_done:
		# 즉시 완성
		_typed_chars = _full_text.length()
		_text_label.text = _full_text
		_typing_done = true
		_waiting_for_input = true
		_show_continue(true)
		return
	if _waiting_for_input:
		_waiting_for_input = false
		_show_continue(false)
		SceneFlow.advance()
