## Credits — 엔딩 크레딧 화면
## 스크롤 텍스트 + 분기별 에필로그 한 줄 + 타이틀 복귀
extends Control

const SCROLL_SPEED: float = 40.0  # px/sec
const CREDITS_DATA: Array = [
	{"type": "title", "text": "MEMORIA"},
	{"type": "subtitle", "text": "The Price of Oblivion"},
	{"type": "spacer"},
	{"type": "heading", "text": "Created By"},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": "Story & Writing"},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": "Game Design"},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": "Programming"},
	{"type": "name", "text": "GDScript / Godot 4.6"},
	{"type": "name", "text": "with Claude (Anthropic)"},
	{"type": "spacer"},
	{"type": "heading", "text": "Art"},
	{"type": "name", "text": "Leonardo AI — CG & Portraits"},
	{"type": "name", "text": "Procedural Pixel Art — Code Generated"},
	{"type": "spacer"},
	{"type": "heading", "text": "Music"},
	{"type": "name", "text": "Suno / Udio AI"},
	{"type": "spacer"},
	{"type": "heading", "text": "Sound Effects"},
	{"type": "name", "text": "Procedural Audio — Code Generated"},
	{"type": "spacer"},
	{"type": "spacer"},
	{"type": "heading", "text": "Engine"},
	{"type": "name", "text": "Godot Engine 4.6"},
	{"type": "name", "text": "godotengine.org"},
	{"type": "spacer"},
	{"type": "spacer"},
	{"type": "divider"},
	{"type": "spacer"},
]

var scroll_container: Control
var _finished: bool = false
var _total_height: float = 0.0

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	AudioManager.play_bgm("res://assets/audio/bgm/epilogue.mp3")
	# NG+ 해금 + 업적 기록
	GameManager.mark_game_completed()
	_record_ending_achievements()
	_build_ui()

func _record_ending_achievements() -> void:
	if GameManager.get_flag("zero_burn_path"):
		AchievementManager.record_ending("ending_zero")
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 4:
		AchievementManager.record_ending("ending_ash")
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("hidden_ch1_stump") and GameManager.get_flag("hidden_ch6_garden"):
		AchievementManager.record_ending("ending_seam")
	else:
		AchievementManager.record_ending("ending_seal")

func _build_ui() -> void:
	# 배경
	var bg = ColorRect.new()
	bg.set_anchors_preset(PRESET_FULL_RECT)
	bg.color = Color(0.02, 0.02, 0.03)
	add_child(bg)

	# 스크롤 컨테이너 (화면 하단에서 시작해 위로 올라감)
	scroll_container = Control.new()
	scroll_container.position = Vector2(0, 720)  # 화면 아래에서 시작
	add_child(scroll_container)

	var y_offset: float = 0.0
	var credits = CREDITS_DATA.duplicate()

	# 분기별 에필로그 한 줄 추가
	if GameManager.get_flag("zero_burn_path"):
		credits.append({"type": "quote", "text": "He burned everything. Even his name."})
		credits.append({"type": "quote_sub", "text": "But something remained — a shape where a person used to be."})
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 4:
		credits.append({"type": "quote", "text": "What remains is not a man. Just ash, drifting."})
		credits.append({"type": "quote_sub", "text": "The name survived. Nothing else did."})
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("hidden_ch1_stump") and GameManager.get_flag("hidden_ch6_garden"):
		credits.append({"type": "quote", "text": "In the cracks between loss, something green still grows."})
		credits.append({"type": "quote_sub", "text": "The smallest moments became the strongest shield."})
	else:
		credits.append({"type": "quote", "text": "He kept his name. The seal held."})
		credits.append({"type": "quote_sub", "text": "Whether that was enough... only time would tell."})

	credits.append({"type": "spacer"})
	credits.append({"type": "spacer"})
	credits.append({"type": "thanks", "text": "Thank you for playing."})
	credits.append({"type": "spacer"})
	credits.append({"type": "spacer"})
	credits.append({"type": "spacer"})

	for entry in credits:
		var label: Label
		match entry.type:
			"title":
				label = _make_label(entry.text, 36, Color(0.85, 0.75, 0.55))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 50
			"subtitle":
				label = _make_label(entry.text, 16, Color(0.55, 0.5, 0.45))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 40
			"heading":
				label = _make_label(entry.text, 14, Color(0.5, 0.45, 0.4))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 25
			"name":
				label = _make_label(entry.text, 18, Color(0.8, 0.75, 0.7))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 30
			"quote":
				label = _make_label(entry.text, 16, Color(0.7, 0.55, 0.4))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 28
			"quote_sub":
				label = _make_label(entry.text, 13, Color(0.5, 0.45, 0.4))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 28
			"thanks":
				label = _make_label(entry.text, 22, Color(0.85, 0.75, 0.55))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 40
			"divider":
				var line = ColorRect.new()
				line.size = Vector2(200, 1)
				line.position = Vector2(540, y_offset + 5)
				line.color = Color(0.4, 0.35, 0.3, 0.5)
				scroll_container.add_child(line)
				y_offset += 20
			"spacer":
				y_offset += 30

	_total_height = y_offset + 100  # 마지막 텍스트 + 여유분

	# 스킵 안내
	var skip_label = Label.new()
	skip_label.text = "Press SPACE or ENTER to skip"
	skip_label.set_anchors_preset(PRESET_BOTTOM_WIDE)
	skip_label.offset_top = -40
	skip_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	skip_label.add_theme_font_size_override("font_size", 11)
	skip_label.add_theme_color_override("font_color", Color(0.35, 0.3, 0.28, 0.6))
	add_child(skip_label)

func _make_label(text: String, size: int, color: Color) -> Label:
	var label = Label.new()
	label.text = text
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.size = Vector2(1280, 50)
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_color", color)
	return label

func _process(delta: float) -> void:
	if _finished:
		return

	scroll_container.position.y -= SCROLL_SPEED * delta

	# 모든 텍스트가 화면 위로 사라지면 타이틀로
	if scroll_container.position.y < -_total_height:
		_go_to_title()

func _unhandled_input(event: InputEvent) -> void:
	if _finished:
		return
	if event.is_action_pressed("interact") or event.is_action_pressed("cancel"):
		_go_to_title()
		get_viewport().set_input_as_handled()

func _go_to_title() -> void:
	_finished = true
	AudioManager.stop_bgm(true)
	SceneTransition.change_scene("res://scenes/main/main.tscn")
