extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/arena_biomes.png"
const PROFILES: Array[String] = ["rim_forest", "verdan_market", "bl07_void", "crumbling_coast"]

func _ready() -> void:
	OptionsMenu.settings["reduce_motion"] = true
	var backdrop := ColorRect.new()
	backdrop.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	backdrop.color = Color(0.05, 0.045, 0.065)
	add_child(backdrop)
	for i in range(PROFILES.size()):
		var stage := HybridDepthStage.create_stage(PROFILES[i], HybridDepthStage.StageMode.BATTLE)
		stage.set_anchors_preset(Control.PRESET_TOP_LEFT)
		stage.position = Vector2(float(i % 2) * 640.0, float(i / 2) * 360.0)
		stage.size = Vector2(640, 360)
		add_child(stage)
		var label := Label.new()
		label.text = PROFILES[i]
		label.position = stage.position + Vector2(10, 8)
		label.add_theme_font_size_override("font_size", 14)
		add_child(label)
	for _f in range(6):
		await get_tree().process_frame
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("ARENA_BIOMES_CAPTURE_PASS path=%s profiles=%d" % [OUTPUT_PATH, PROFILES.size()])
	get_tree().quit(0)
