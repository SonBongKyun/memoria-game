extends Node

## S250: 삽화가 없는 적이 실제로 어떻게 보이는지 한 장 찍는다.
## 코드 주석은 "보라색 마름모"라고 하지만, 고치기 전에 눈으로 확인한다.

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
	GameManager.change_state(GameManager.GameState.BATTLE)
	BattleManager.start_battle(
		{"name": "Void Wraith", "hp": 90, "atk": 18, "is_void": true, "abilities": ["drain"]},
		"res://scenes/maps/the_seam.tscn")
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(2.0).timeout
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	image.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/artless_enemy.png"))
	print("ARTLESS_CAPTURE_PASS image=%s" % BattleManager.enemy_image)
	get_tree().quit(0)
