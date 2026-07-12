extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/story_combat_witness.png"

func _ready() -> void:
	OptionsMenu.settings.clean_gameplay_visuals = true
	OptionsMenu.settings.reduce_motion = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 1
	GameManager.player_data.hp = 82
	GameManager.player_data.max_hp = 100
	var enemy := BattleManager.Enemy.new("Ash Crawler", 45, 10, false)
	BattleManager.current_enemy = enemy
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.enemy_image = "res://assets/cg/game_image/void_beast_confrontation.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/story_ch1_twisted_forest_path.png"
	BattleManager._witness_progress = 1
	BattleManager._witness_required = 2
	BattleManager._witness_completed_this_battle = false
	BattleManager.tactical_objective = {
		"id": "witness_echo",
		"title": "Hold the Name",
		"desc": "Complete a WITNESS reading before victory.",
		"status": "active",
		"complete": false,
		"failed": false,
		"reward_grains": 8,
	}
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(3.2).timeout
	battle.call("_on_player_turn")
	await get_tree().create_timer(0.8).timeout
	var image := get_viewport().get_texture().get_image()
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var result := image.save_png(OUTPUT_PATH)
	assert(result == OK, "Story combat capture must save")
	assert(battle.witness_btn != null and "1/2" in battle.witness_btn.text, "WITNESS progress must be visible in the command grid")
	assert(battle.action_container.columns == 4, "Battle commands must use a readable two-row grid")
	assert(battle.action_container.visible, "Battle command grid must be visible on the player turn")
	print("STORY_COMBAT_CAPTURE_PASS path=%s witness=1/2 grid=4x2 pos=%s size=%s" % [OUTPUT_PATH, battle.action_container.position, battle.action_container.size])
	get_tree().quit(0)
