## Credits, 엔딩 크레딧 화면
## 스크롤 텍스트 + 분기별 에필로그 한 줄 + 타이틀 복귀
extends Control

const SCROLL_SPEED: float = 40.0  # px/sec
const GAME_VERSION: String = "v0.9.0"  # S59: Shown at the end of credits
## S243: 항목 이름이 로케일에 따라 달라지므로 상수로 둘 수 없다. 런타임에 만든다.
func _credits_data() -> Array:
	return [
	{"type": "title", "text": "MEMORIA"},
	{"type": "subtitle", "text": "The Price of Oblivion"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Created By", "제작")},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Story & Writing", "각본")},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Game Design", "게임 디자인")},
	{"type": "name", "text": "Son Bong Kyun"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Programming", "프로그래밍")},
	{"type": "name", "text": "GDScript / Godot 4.6"},
	{"type": "name", "text": _loc("with Claude (Anthropic)", "Claude(Anthropic)와 함께")},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Art", "아트")},
	{"type": "name", "text": _loc("Leonardo AI, CG & Portraits", "Leonardo AI · CG 및 포트레이트")},
	{"type": "name", "text": _loc("Procedural Pixel Art, Code Generated", "절차적 픽셀 아트 · 코드 생성")},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Music", "음악")},
	{"type": "name", "text": "Suno / Udio AI"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Sound Effects", "효과음")},
	{"type": "name", "text": _loc("Procedural Audio, Code Generated", "절차적 오디오 · 코드 생성")},
	{"type": "spacer"},
	{"type": "spacer"},
	{"type": "heading", "text": _loc("Engine", "엔진")},
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

## S243: 크레딧은 전체 플레이의 마지막 화면인데 한국어 로케일에서도 통째로
## 영어였다. 분기별 에필로그 한 줄까지 포함해서.
func _loc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

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
	var credits = _credits_data()

	# 분기별 에필로그 한 줄 추가
	if GameManager.get_flag("zero_burn_path"):
		credits.append({"type": "quote", "text": _loc("He burned everything. Even his name.", "그는 전부 태웠다. 자기 이름까지.")})
		credits.append({"type": "quote_sub", "text": _loc("But something remained, a shape where a person used to be.", "그래도 무언가는 남았다. 사람이 서 있던 자리의 모양이.")})
	elif GameManager.get_flag("seal_refused") and MemoryManager.get_burn_count() >= 4:
		credits.append({"type": "quote", "text": _loc("What remains is not a man. Just ash, drifting.", "남은 것은 사람이 아니다. 떠도는 재일 뿐.")})
		credits.append({"type": "quote_sub", "text": _loc("The name survived. Nothing else did.", "이름은 살아남았다. 그 밖에는 아무것도.")})
	elif GameManager.get_flag("seal_refused") and GameManager.get_flag("hidden_ch1_stump") and GameManager.get_flag("hidden_ch6_garden"):
		credits.append({"type": "quote", "text": _loc("In the cracks between loss, something green still grows.", "상실과 상실 사이의 틈에서, 아직 푸른 것이 자란다.")})
		credits.append({"type": "quote_sub", "text": _loc("The smallest moments became the strongest shield.", "가장 작은 순간들이 가장 단단한 방패가 되었다.")})
	else:
		credits.append({"type": "quote", "text": _loc("He kept his name. The seal held.", "그는 이름을 지켰다. 봉인은 버텼다.")})
		credits.append({"type": "quote_sub", "text": _loc("Whether that was enough... only time would tell.", "그것으로 충분했는지는, 시간만이 답할 것이다.")})

	credits.append({"type": "spacer"})
	credits.append({"type": "spacer"})

	# S59: Special Thanks section
	credits.append({"type": "divider"})
	credits.append({"type": "spacer"})
	credits.append({"type": "heading", "text": _loc("Special Thanks", "감사의 말")})
	credits.append({"type": "name", "text": _loc("The Godot Community", "Godot 커뮤니티")})
	credits.append({"type": "name", "text": _loc("Everyone who playtested and gave feedback", "플레이테스트와 피드백을 준 모든 분")})
	credits.append({"type": "name", "text": _loc("To The Moon & LISA, for the inspiration", "영감을 준 To The Moon과 LISA에게")})
	credits.append({"type": "name", "text": _loc("All memory keepers who refuse to forget", "잊기를 거부하는 모든 기억 지킴이에게")})
	credits.append({"type": "spacer"})
	credits.append({"type": "divider"})
	credits.append({"type": "spacer"})

	credits.append({"type": "thanks", "text": _loc("Thank you for playing.", "플레이해 주셔서 고맙습니다.")})
	credits.append({"type": "spacer"})
	# S56: Steam wishlist reminder (subtle, non-intrusive)
	credits.append({"type": "steam_wishlist", "text": _loc("MEMORIA is coming to Steam", "MEMORIA가 Steam에 출시됩니다")})
	credits.append({"type": "steam_sub", "text": _loc("Wishlist now to be notified at launch", "위시리스트에 담으면 출시 알림을 받습니다")})
	credits.append({"type": "spacer"})
	# S59: Version number at the end
	credits.append({"type": "version", "text": "MEMORIA %s" % GAME_VERSION})
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
			"steam_wishlist":
				label = _make_label(entry.text, 15, Color(0.4, 0.55, 0.7))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 24
			"steam_sub":
				label = _make_label(entry.text, 12, Color(0.35, 0.45, 0.55))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 24
			"version":
				label = _make_label(entry.text, 11, Color(0.35, 0.33, 0.3, 0.5))
				label.position = Vector2(0, y_offset)
				scroll_container.add_child(label)
				y_offset += 24
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
	skip_label.text = _loc("Press SPACE or ENTER to skip", "SPACE 또는 ENTER로 건너뛰기")
	skip_label.set_anchors_preset(PRESET_BOTTOM_WIDE)
	skip_label.offset_top = -40
	skip_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	skip_label.add_theme_font_size_override("font_size", 13)
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
