extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/hybrid_battle_stage.png"
const ARREL_BATTLE_FULLBODY_PATH := "res://assets/portraits/character_shots/arrel_battle_v3.png"

func _ready() -> void:
	# Synthetic presentation runs must never contribute to player-facing records.
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.hp = 82
	GameManager.player_data.max_hp = 100
	GameManager.player_data.elia_with_party = true
	BattleManager.sable_in_party = true
	BattleManager.tobias_in_party = true
	BattleManager.current_enemy = BattleManager.Enemy.new("Shade Sentinel", 160, 18, true)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/the_seam.tscn"
	BattleManager.enemy_image = "res://assets/cg/character_shots/shade_sentinel_guard_v3.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/story_ch5_seam_first_light.png"

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.2).timeout
	var player := battle.get("player_sprite") as TextureRect
	assert(player != null and player.texture != null and player.texture.resource_path == ARREL_BATTLE_FULLBODY_PATH, "Hybrid capture must exercise the canonical painterly Arrel plate")
	for property_name in ["ally_sprite", "tobias_sprite", "enemy_sprite"]:
		assert(battle.get(property_name) is CanvasItem, "Hybrid capture requires the complete CanvasItem battler line: %s" % property_name)

	battle.call("_on_player_turn")
	await get_tree().create_timer(0.75).timeout
	var relief := battle.get_node_or_null("HybridDepthStage") as HybridDepthStage
	assert(relief != null and relief.profile_id == "the_seam", "Battle must resolve the returning map into the correct 3D biome")
	assert(battle.get("_current_battler_focus") == "player", "Hybrid capture must preserve player-primary stage focus")
	print("HYBRID_BATTLE_LAYOUT bg=%s relief=%s viewport=%s canonical_arrel=%s party=max" % [battle.bg.size, relief.size, get_viewport().get_visible_rect().size, player.texture.resource_path])
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK, "Could not write the hybrid battle visual audit")
	print("HYBRID_BATTLE_CAPTURE_PASS path=%s profile=%s focus=player canonical=TextureRect" % [OUTPUT_PATH, relief.profile_id])
	get_tree().quit(0)
