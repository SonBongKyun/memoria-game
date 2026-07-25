extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_depth_board.png"

func _ready() -> void:
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.change_state(GameManager.GameState.MENU)
	var background := ColorRect.new()
	background.set_anchors_preset(Control.PRESET_FULL_RECT)
	background.color = Color(0.22, 0.035, 0.10)
	add_child(background)

	var battle := HybridDepthStage.create_stage("the_seam", HybridDepthStage.StageMode.BATTLE)
	battle.anchor_left = 0.02
	battle.anchor_right = 0.56
	battle.anchor_top = 0.05
	battle.anchor_bottom = 0.95
	add_child(battle)

	var atlas := HybridDepthStage.create_stage("world_map", HybridDepthStage.StageMode.ATLAS)
	atlas.anchor_left = 0.56
	atlas.anchor_right = 0.98
	atlas.anchor_top = 0.04
	atlas.anchor_bottom = 0.54
	add_child(atlas)
	atlas.focus_route(7)

	var relic := HybridDepthStage.create_stage("bl07_void", HybridDepthStage.StageMode.RELIC)
	relic.anchor_left = 0.60
	relic.anchor_right = 0.94
	relic.anchor_top = 0.56
	relic.anchor_bottom = 0.96
	add_child(relic)

	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().create_timer(0.8).timeout
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("HYBRID_DEPTH_CAPTURE_PASS path=%s" % OUTPUT_PATH)
	get_tree().quit(0)
