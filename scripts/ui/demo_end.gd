## DemoEnd
## S66: A안 데모 빌드, Ch1(Act I, Ash) 종료 후 표시되는 화면.
## 감사 메시지 + 다음 챕터 티저 + 위시리스트 / 타이틀 복귀 CTA.
extends Control

const STEAM_URL: String = "https://store.steampowered.com/app/000000/MEMORIA"  # 실제 앱 ID로 교체 예정

@onready var _root: Control = self

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_build()
	_play_intro()

func _build() -> void:
	# 배경 이미지 (Cover2 또는 어울리는 CG)
	var bg = TextureRect.new()
	bg.set_anchors_preset(PRESET_FULL_RECT)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	# S243: 예전에는 game_start.png를 깔았다. 그 그림은 타이틀 화면 목업이라
	# NEW GAME / CONTINUE / SETTINGS / EXIT와 PRESS ANY KEY가 삽화에 구워져 있고,
	# 데모 종료 문구 뒤로 가짜 영어 메뉴가 겹쳐 보였다. 본문이 "벨트를 지나"라고
	# 말하므로 그 다음 목적지의 챕터 스플래시를 깐다. 구운 UI가 없다.
	if ResourceLoader.exists("res://assets/cg/generated/chapter_splash_belt_waystation.png"):
		bg.texture = load("res://assets/cg/generated/chapter_splash_belt_waystation.png")
	bg.modulate = Color(0.55, 0.5, 0.55, 1.0)
	add_child(bg)

	# 어두운 비네트 오버레이
	var overlay = ColorRect.new()
	overlay.set_anchors_preset(PRESET_FULL_RECT)
	overlay.color = Color(0.02, 0.015, 0.025, 0.78)
	add_child(overlay)

	# 컨테이너 (중앙 정렬)
	var vbox = VBoxContainer.new()
	vbox.set_anchors_preset(PRESET_CENTER)
	vbox.offset_left = -360
	vbox.offset_right = 360
	vbox.offset_top = -240
	vbox.offset_bottom = 240
	vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_theme_constant_override("separation", 18)
	add_child(vbox)

	# 타이틀
	var title = Label.new()
	title.text = _loc("Act I, Ash", "제1부 · 재")
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 48)
	title.add_theme_color_override("font_color", Color(0.95, 0.85, 0.55))
	title.modulate.a = 0.0
	title.name = "TitleLabel"
	vbox.add_child(title)

	# 부제
	var subtitle = Label.new()
	subtitle.text = _loc("End of Demo", "데모 종료")
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 18)
	subtitle.add_theme_color_override("font_color", Color(0.7, 0.62, 0.5))
	subtitle.modulate.a = 0.0
	subtitle.name = "SubtitleLabel"
	vbox.add_child(subtitle)

	# 간격
	var sp1 = Control.new()
	sp1.custom_minimum_size = Vector2(0, 24)
	vbox.add_child(sp1)

	# 본문
	var body = RichTextLabel.new()
	body.bbcode_enabled = true
	body.fit_content = true
	body.scroll_active = false
	body.custom_minimum_size = Vector2(700, 0)
	body.add_theme_font_size_override("normal_font_size", 16)
	body.add_theme_color_override("default_color", Color(0.85, 0.8, 0.72))
	body.text = _loc("[center]Thank you for playing.\n\nArrel's road continues, through the Belt, the Seam, and the place beyond memory.\nA brother to find. A choice that cannot be unmade.\n\n[i]The full game will release on Steam.[/i][/center]", "[center]플레이해 주셔서 고맙습니다.\n\n아렐의 길은 계속됩니다. 벨트를 지나, 심을 지나, 기억 너머의 장소까지.\n찾아야 할 형제. 되돌릴 수 없는 선택.\n\n[i]정식판은 Steam에 출시됩니다.[/i][/center]")
	body.modulate.a = 0.0
	body.name = "BodyLabel"
	vbox.add_child(body)

	var sp2 = Control.new()
	sp2.custom_minimum_size = Vector2(0, 24)
	vbox.add_child(sp2)

	# 통계, 플레이어가 태운 기억 수, 잔존 수
	var stats = Label.new()
	var burned = MemoryManager.burned_memories.size()
	var residue = 0
	for m in MemoryManager.burned_memories:
		if m.is_residue:
			residue += 1
	stats.text = _loc("You burned %d memories.   %d remain as residue.", "기억 %d개를 태웠습니다.   %d개가 잔존으로 남았습니다.") % [burned, residue]
	stats.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	stats.add_theme_font_size_override("font_size", 13)
	stats.add_theme_color_override("font_color", Color(0.6, 0.55, 0.45))
	stats.modulate.a = 0.0
	stats.name = "StatsLabel"
	vbox.add_child(stats)

	var sp3 = Control.new()
	sp3.custom_minimum_size = Vector2(0, 30)
	vbox.add_child(sp3)

	# 버튼 컨테이너
	var btn_box = HBoxContainer.new()
	btn_box.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_box.add_theme_constant_override("separation", 18)
	btn_box.modulate.a = 0.0
	btn_box.name = "BtnBox"
	vbox.add_child(btn_box)

	var wishlist_btn = _make_btn(_loc("✦  Wishlist on Steam", "✦  Steam 위시리스트에 담기"))
	wishlist_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_select")
		OS.shell_open(STEAM_URL)
	)
	btn_box.add_child(wishlist_btn)

	var title_btn = _make_btn(_loc("Return to Title", "타이틀로 돌아가기"))
	title_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_select")
		SceneTransition.change_scene_styled("res://scenes/main/main.tscn")
	)
	btn_box.add_child(title_btn)

	var quit_btn = _make_btn("Quit")
	quit_btn.pressed.connect(func():
		AudioManager.play_sfx("ui_select")
		get_tree().quit()
	)
	btn_box.add_child(quit_btn)

## S243: 데모를 끝까지 본 플레이어가 마지막으로 보는 화면인데 통째로 영어였다.
func _loc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

func _make_btn(label: String) -> Button:
	var btn = Button.new()
	btn.text = label
	btn.custom_minimum_size = Vector2(220, 48)
	btn.add_theme_font_size_override("font_size", 16)
	var s = StyleBoxFlat.new()
	s.bg_color = Color(0.08, 0.06, 0.1, 0.92)
	s.border_color = Color(0.7, 0.55, 0.3, 0.85)
	s.set_border_width_all(2)
	s.set_corner_radius_all(4)
	s.set_content_margin_all(10)
	btn.add_theme_stylebox_override("normal", s)
	var h = s.duplicate()
	h.bg_color = Color(0.18, 0.13, 0.08, 0.96)
	h.border_color = Color(0.95, 0.78, 0.4, 1.0)
	btn.add_theme_stylebox_override("hover", h)
	btn.add_theme_stylebox_override("focus", h)
	btn.add_theme_color_override("font_color", Color(0.85, 0.78, 0.65))
	btn.add_theme_color_override("font_hover_color", Color(0.98, 0.88, 0.55))
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

func _play_intro() -> void:
	# 순차 페이드인
	var nodes = ["TitleLabel", "SubtitleLabel", "BodyLabel", "StatsLabel", "BtnBox"]
	var delay = 0.0
	for n in nodes:
		var node = find_child(n, true, false)
		if node == null:
			continue
		var tw = create_tween()
		tw.tween_interval(delay)
		tw.tween_property(node, "modulate:a", 1.0, 1.0)
		delay += 0.45
	# BGM 페이드
	if has_node("/root/AudioManager"):
		AudioManager.play_bgm("title_theme") if AudioManager.has_method("play_bgm") else null
