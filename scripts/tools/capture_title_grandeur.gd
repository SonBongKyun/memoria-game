extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/title_grandeur.png"

func _ready() -> void:
	GameManager.current_locale = "ko"
	OptionsMenu.settings["reduce_motion"] = false
	ExplorationHUD.visible = false
	PauseMenu.visible = false

	var title_scene := load("res://scenes/main/main.tscn") as PackedScene
	assert(title_scene != null)
	var title := title_scene.instantiate()
	add_child(title)
	await get_tree().create_timer(1.65).timeout
	await get_tree().process_frame

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("TITLE_GRANDEUR_CAPTURE_PASS path=%s size=%dx%d" % [OUTPUT_PATH, image.get_width(), image.get_height()])
	get_tree().quit(0)
