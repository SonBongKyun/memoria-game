extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/dialogue_interface_ko.png"

func _ready() -> void:
	ExplorationHUD.visible = false
	PauseMenu.visible = false
	GameManager.current_locale = "ko"

	var background := TextureRect.new()
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.texture = load("res://assets/cg/game_image/env_bureau_spires.png")
	background.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	background.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	background.modulate = Color(0.46, 0.48, 0.54)
	add_child(background)

	var veil := ColorRect.new()
	veil.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	veil.color = Color(0.025, 0.035, 0.055, 0.38)
	add_child(veil)

	# DialogueBox starts with a short hide tween; let that initialization finish
	# before presenting the audit line.
	await get_tree().create_timer(0.2).timeout
	DialogueBox.visible = true
	DialogueBox.call("_on_dialogue_line", "Malet", "기억은 사라지지 않아. 다만 누가 그 값을 치르느냐가 달라질 뿐이지.", "malet_neutral")
	DialogueBox.call("show_box")
	await get_tree().create_timer(0.45).timeout
	DialogueBox.is_typing = false
	DialogueBox.text_label.text = DialogueBox._bbcode_text
	DialogueBox.call("_refresh_indicator_text")
	await get_tree().process_frame

	assert(DialogueBox.text_label.get_theme_font("normal_font").font_names[0] == "Noto Serif KR")
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("DIALOGUE_INTERFACE_CAPTURE_PASS path=%s font=%s" % [OUTPUT_PATH, DialogueBox.text_label.get_theme_font("normal_font").font_names[0]])
	get_tree().quit(0)
