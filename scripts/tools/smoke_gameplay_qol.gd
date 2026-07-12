extends Node

func _ready() -> void:
	var previous_flags := GameManager.story_flags.duplicate(true)
	var previous_chapter := GameManager.current_chapter
	var previous_state := GameManager.current_state

	GameManager.current_locale = "ko"
	GameManager.current_chapter = 1
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	GameManager.story_flags = {
		"ch1_elia_appeared": true,
		"ch1_void_beast_defeated": false,
	}
	ExplorationHUD.call("_update_quest_tracker")
	assert("공허수" in ExplorationHUD.quest_label.text, "Chapter 1 objective must advance beyond finding Elia")
	GameManager.set_flag("ch1_void_beast_defeated")
	ExplorationHUD.call("_update_quest_tracker")
	assert("야영지" in ExplorationHUD.quest_label.text, "Chapter 1 objective must point to camp after the mandatory battle")

	GameManager.current_chapter = 2
	GameManager.story_flags = {}
	var verdan: Node2D = load("res://scenes/maps/verdan_market.tscn").instantiate()
	var target: Variant = Minimap._resolve_story_target(verdan)
	assert(target is Vector2 and (target as Vector2).distance_to(Vector2(448, 384)) < 1.0, "Verdan guidance must target Malet before his story beat")
	GameManager.set_flag("ch2_malet_done")
	assert(Minimap._resolve_story_target(verdan) == null, "Completed Malet objective must disappear from the minimap")
	verdan.free()

	var encounter := RandomEncounter.setup([], "res://scenes/maps/rim_forest.tscn", "", "", 10, 10)
	encounter.threshold = 10.0
	encounter.last_player_pos = Vector2(1, 1)
	RandomEncounter.update(encounter, Vector2(8.5, 1), 1)
	assert(encounter.warning_emitted and encounter.step_count < encounter.threshold, "Ambient encounter warning must arrive before combat")

	var ambient_enemy := BattleManager.Enemy.new("Ambient Echo", 20, 2, true)
	ambient_enemy.is_ambient_encounter = true
	BattleManager.current_enemy = ambient_enemy
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.player_flee()
	assert(BattleManager.state == BattleManager.BattleState.FLED, "Movement-based repeat encounters must allow guaranteed pacing-friendly escape")

	assert(SaveManager.AUTOSAVE_SLOT == 0, "Checkpoint retry must retain the dedicated autosave slot")
	assert(SaveManager.has_method("_update_map_checkpoint"), "SaveManager must maintain map-entry checkpoints")

	GameManager.story_flags = previous_flags
	GameManager.current_chapter = previous_chapter
	GameManager.change_state(previous_state)
	print("GAMEPLAY_QOL_SMOKE_PASS objective_progression=3 minimap_target=1 checkpoint_hook=1 encounter_warning=1 ambient_flee=1")
	get_tree().quit(0)
