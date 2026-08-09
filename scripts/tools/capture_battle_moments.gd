extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/battle_moments.png"
const ARREL_BATTLE_FULLBODY_PATH := "res://assets/portraits/character_shots/arrel_battle_v3.png"
const CAPTURE_SIZE := Vector2i(1280, 720)

func _ready() -> void:
	# Keep this visual audit outside of achievements, story log, and save state.
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = false
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
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
	await get_tree().create_timer(0.9).timeout
	var player := battle.get("player_sprite") as TextureRect
	var enemy := battle.get("enemy_sprite") as CanvasItem
	assert(player != null and player.texture != null and player.texture.resource_path == ARREL_BATTLE_FULLBODY_PATH, "Moment capture must exercise the canonical painterly Arrel plate")
	assert(enemy != null, "Moment capture requires an enemy CanvasItem")

	var shots: Array[Image] = []
	# Full-frame vertical plates preserve objective, HP, field-readout, and command-deck
	# clearance in every semantic moment rather than cropping those constraints away.
	battle.call("_set_battle_stage_focus", "neutral")
	battle.call("_play_actor_anim", player, "idle", true)
	battle.call("_play_actor_anim", enemy, "idle", true)
	await _capture_viewport(shots) # idle

	battle.call("_on_player_turn")
	await get_tree().create_timer(0.65).timeout
	await _capture_viewport(shots) # player focus / command deck

	battle.call("_on_enemy_turn")
	await _capture_viewport(shots) # enemy focus

	battle.call("_set_battle_stage_focus", "player")
	battle.call("_play_actor_anim", player, "attack", true)
	await _capture_viewport(shots) # player action

	battle.call("_play_actor_anim", player, "idle", true)
	battle.call("_set_battle_stage_focus", "enemy")
	battle.call("_play_actor_anim", enemy, "hurt", true)
	await _capture_viewport(shots) # enemy hurt

	battle.call("_play_actor_anim", enemy, "down", true)
	await _capture_viewport(shots) # enemy down

	var sheet := Image.create(CAPTURE_SIZE.x, CAPTURE_SIZE.y * shots.size(), false, Image.FORMAT_RGBA8)
	for i in range(shots.size()):
		var shot := shots[i]
		if shot.get_size() != CAPTURE_SIZE:
			shot.resize(CAPTURE_SIZE.x, CAPTURE_SIZE.y, Image.INTERPOLATE_LANCZOS)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, CAPTURE_SIZE), Vector2i(0, CAPTURE_SIZE.y * i))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var result := sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK, "Could not write the battle-moments visual audit")
	print("BATTLE_MOMENTS_CAPTURE_PASS path=%s shots=%d states=idle_player_enemy_action_hurt_down canonical=TextureRect" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)

func _capture_viewport(shots: Array[Image]) -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())
