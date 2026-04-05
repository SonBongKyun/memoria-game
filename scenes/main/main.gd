## Main Scene — 타이틀 화면
## Cover.png 배경 + New Game / Continue / Quit 메뉴.
extends Control

@onready var continue_btn: Button = $VBoxContainer/ContinueButton

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.MENU)
	_setup_background()
	_setup_menu()
	print("=== MEMORIA: The Price of Oblivion ===")

func _setup_background() -> void:
	# Cover.png 배경
	var bg = TextureRect.new()
	bg.set_anchors_preset(PRESET_FULL_RECT)
	bg.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	bg.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	var tex = load("res://assets/cg/cover.png")
	if tex:
		bg.texture = tex
	else:
		# fallback
		var fallback = ColorRect.new()
		fallback.set_anchors_preset(PRESET_FULL_RECT)
		fallback.color = Color(0.08, 0.08, 0.1)
		add_child(fallback)
		return
	bg.z_index = -1
	bg.mouse_filter = MOUSE_FILTER_IGNORE
	# 배경을 메뉴 뒤로
	add_child(bg)
	move_child(bg, 0)

	# 어두운 오버레이 (메뉴 가독성)
	var overlay = ColorRect.new()
	overlay.set_anchors_preset(PRESET_FULL_RECT)
	overlay.color = Color(0, 0, 0, 0.4)
	overlay.mouse_filter = MOUSE_FILTER_IGNORE
	add_child(overlay)
	move_child(overlay, 1)

func _setup_menu() -> void:
	# Continue 버튼 활성/비활성
	if continue_btn:
		continue_btn.disabled = not SaveManager.has_save(1)
		if continue_btn.disabled:
			continue_btn.modulate.a = 0.4

func _on_new_game_pressed() -> void:
	# 기억 초기화 (새 게임)
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager._init_starting_memories()
	GameManager.story_flags.clear()
	GameManager.current_chapter = 1
	GameManager.player_data.hp = GameManager.player_data.max_hp
	SceneTransition.change_scene("res://scenes/maps/rim_forest.tscn")

func _on_continue_pressed() -> void:
	SaveManager.load_game(1)

func _on_quit_pressed() -> void:
	get_tree().quit()
