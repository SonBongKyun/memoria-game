extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/recurring_enemy_battle_stage_v1.png"
const CAPTURE_SIZE := Vector2i(1280, 720)
const SHEET_COLUMNS := 2

const ENCOUNTERS: Array[Dictionary] = [
	{
		"name": "Ash Hound",
		"art": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png",
		"action_art": "res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_battle_v1.png",
		"background": "res://assets/cg/generated/chapter_splash_crumbling_coast.png",
		"return_scene": "res://scenes/maps/crumbling_coast.tscn",
	},
	{
		"name": "Signal Wisp",
		"art": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_stage_v1.png",
		"action_art": "res://assets/cg/generated/battle_stage_v2/enemy_signal_wisp_battle_v1.png",
		"background": "res://assets/cg/generated/chapter_splash_drift_shelter.png",
		"return_scene": "res://scenes/maps/drift_shelter.tscn",
	},
	{
		"name": "Rootbound Echo",
		"art": "res://assets/cg/generated/battle_stage_v2/enemy_rootbound_echo_stage_v1.png",
		"action_art": "res://assets/cg/generated/battle_stage_v2/enemy_rootbound_echo_battle_v1.png",
		"background": "res://assets/cg/generated/chapter_splash_forgotten_forest.png",
		"return_scene": "res://scenes/maps/forgotten_forest.tscn",
	},
	{
		"name": "Void Fragment",
		"art": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_stage_v1.png",
		"action_art": "res://assets/cg/generated/battle_stage_v2/enemy_void_fragment_battle_v1.png",
		"background": "res://assets/cg/generated/chapter_splash_bl07_void.png",
		"return_scene": "res://scenes/maps/bl07_void.tscn",
	},
]


func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "en"
	GameManager.current_chapter = 10
	GameManager.player_data.hp = 82
	GameManager.player_data.max_hp = 100
	GameManager.player_data.elia_with_party = false
	GameManager.change_state(GameManager.GameState.BATTLE)
	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false

	var shots: Array[Image] = []
	for encounter: Dictionary in ENCOUNTERS:
		await _capture_encounter(encounter, shots)

	var rows := ceili(float(shots.size()) / float(SHEET_COLUMNS))
	var sheet := Image.create(CAPTURE_SIZE.x * SHEET_COLUMNS, CAPTURE_SIZE.y * rows, false, Image.FORMAT_RGBA8)
	for i in range(shots.size()):
		var shot := shots[i]
		if shot.get_size() != CAPTURE_SIZE:
			shot.resize(CAPTURE_SIZE.x, CAPTURE_SIZE.y, Image.INTERPOLATE_LANCZOS)
		var destination := Vector2i((i % SHEET_COLUMNS) * CAPTURE_SIZE.x, (i / SHEET_COLUMNS) * CAPTURE_SIZE.y)
		sheet.blit_rect(shot, Rect2i(Vector2i.ZERO, CAPTURE_SIZE), destination)

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var result := sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK, "Could not write the recurring-enemy battle-stage visual audit")
	print("RECURRING_ENEMY_BATTLE_STAGE_CAPTURE_PASS path=%s encounters=%d resolver=canonical texture=TextureRect action_cutins=wide" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)


func _capture_encounter(encounter: Dictionary, shots: Array[Image]) -> void:
	var enemy_name := String(encounter.name)
	var expected_art := String(encounter.art)
	var expected_action_art := String(encounter.action_art)
	BattleManager.current_enemy = BattleManager.Enemy.new(enemy_name, 120, 18, true)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = String(encounter.return_scene)
	BattleManager.battle_bg_image = String(encounter.background)
	BattleManager.enemy_image = BattleManager.resolve_enemy_image_by_name(enemy_name)
	assert(BattleManager.enemy_image == expected_art, "%s must enter capture through its canonical resolver path" % enemy_name)

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	# The live encounter intro owns a full-screen black plate for roughly 1.45s.
	# Capture only after that authored reveal has finished so this audit measures
	# the persistent combat tableau, not a transitional frame.
	await get_tree().create_timer(1.75).timeout
	var intro_overlay: ColorRect = battle.get("intro_overlay") as ColorRect
	assert(intro_overlay == null or not intro_overlay.visible, "%s intro must clear before the stage audit" % enemy_name)
	var resolved_action_art := String(battle.call("_resolve_enemy_action_cutin", enemy_name))
	assert(resolved_action_art == expected_action_art, "%s must use its distinct wide action cut-in" % enemy_name)
	var enemy := battle.get("enemy_sprite") as TextureRect
	assert(enemy != null and enemy.texture != null, "%s must build a painterly TextureRect battler" % enemy_name)
	assert(enemy.texture.resource_path == expected_art, "%s stage texture must preserve its resolved battle plate" % enemy_name)
	battle.call("_set_battle_stage_focus", "enemy")
	battle.call("_play_actor_anim", enemy, "idle", true)
	await get_tree().process_frame
	await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())
	battle.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
