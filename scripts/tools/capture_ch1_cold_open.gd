extends Node

const OMEN_PATH := "res://tmp/visual_audit/ch1_cold_open_omen.png"
const CHOICE_PATH := "res://tmp/visual_audit/ch1_cold_open_choice.png"

func _ready() -> void:
	ExplorationHUD.visible = false
	PauseMenu.visible = false
	GameManager.current_locale = "ko"
	GameManager.story_flags.erase("vb_read")
	OptionsMenu.settings.clean_gameplay_visuals = true
	OptionsMenu.settings.reduce_motion = false
	OptionsMenu.settings.screen_shake = false
	# SceneTree root is still attaching this harness during _ready(). Let that
	# finish before SceneFlow adds the VN CanvasLayer to the root.
	await get_tree().process_frame
	SceneFlow.play("ch1_cold_open")

	await get_tree().create_timer(2.1).timeout
	var vn = SceneFlow._vn_ui
	assert(vn != null and is_instance_valid(vn), "Cold-open VN UI must exist")
	vn._typed_chars = vn._full_text.length()
	vn._text_label.text = vn._full_text
	vn._typing_done = true
	await get_tree().process_frame
	var rendered := _save_viewport(OMEN_PATH)
	assert(vn._cg_current.texture != null, "Cold-open omen CG must be visible")
	assert(vn._cg_current.texture.resource_path.ends_with("story_ch1_rim_omen.png"), "Cold open must use the new omen CG")
	assert(not vn._film_grain.visible, "VN presentation must remain noise-free")

	SceneFlow.advance()
	await get_tree().create_timer(0.8).timeout
	SceneFlow.advance()
	await get_tree().create_timer(0.25).timeout
	assert(vn._choice_container.visible, "Opening instinct choice must be visible")
	assert(vn._choice_container.get_child_count() == 3, "Opening instinct must offer three readable routes")
	rendered = _save_viewport(CHOICE_PATH) and rendered
	SceneFlow.select_choice(2)
	await get_tree().process_frame
	assert(GameManager.get_flag("vb_read"), "Reading instinct must carry into the next Void Beast fight")
	assert(SceneFlow.current_index == 5, "Reading instinct must route to its authored strike response")
	print("CH1_COLD_OPEN_CAPTURE_PASS omen=%s choice=%s routes=3 read_carry=1 grain=0 rendered=%d" % [OMEN_PATH, CHOICE_PATH, int(rendered)])
	get_tree().quit(0)

func _save_viewport(path: String) -> bool:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	if image == null:
		# Headless display drivers can validate the UI tree but cannot read back a
		# framebuffer. A normal renderer still writes the same audit captures.
		return false
	var absolute_path := ProjectSettings.globalize_path(path)
	var result := image.save_png(absolute_path)
	assert(result == OK, "Cold-open capture must save: %s" % path)
	assert(FileAccess.file_exists(absolute_path), "Cold-open capture file must exist: %s" % absolute_path)
	return true
