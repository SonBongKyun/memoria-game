## S214: exploration-to-battle Field Flow contract.
extends Node


func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	var previous_state := GameManager.current_state
	var previous_locale := GameManager.current_locale
	var previous_chapter := GameManager.current_chapter
	var previous_flags := GameManager.story_flags.duplicate(true)
	var previous_clean := bool(OptionsMenu.settings.get("clean_gameplay_visuals", false))
	var previous_reduce := bool(OptionsMenu.settings.get("reduce_motion", false))
	GameManager.current_locale = "en"
	GameManager.current_chapter = 5
	GameManager.story_flags = {}
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	assert(InputMap.has_action("field_dash"), "Field Flow requires a dedicated Phase Step action")
	assert(InputManager.get_icon("field_dash") == "Ctrl", "Keyboard HUD must expose the Phase Step key")

	var player: Node = load("res://scenes/player/player.tscn").instantiate()
	add_child(player)
	await get_tree().process_frame
	var flow := player.get_node_or_null("FieldFlow") as FieldFlow
	assert(flow != null, "Player must own the live FieldFlow controller")
	assert(flow.flow > FieldFlow.STARTING_FLOW - 1.0 and flow.flow <= FieldFlow.STARTING_FLOW, "Field Flow must start with a small usable bank")

	# Actual travel, not held input, is what builds the resource.
	var starting_flow := flow.flow
	flow.update_motion(0.5, 64.0, true, true, 180.0)
	assert(flow.flow > starting_flow, "Real movement distance must build Flow")

	# Visible threats expose a high-flow ambush forecast.
	flow.flow = 84.0
	player.call("set_field_threat_source", "visible:smoke:shade", 0.66)
	var forecast: Dictionary = player.call("get_field_flow_status")
	assert(forecast.mode == "ambush_ready", "High Flow near a visible threat must advertise AMBUSH READY")

	# A Phase Step is movement, graphics, and combat intent in one action.
	assert(flow.try_phase_step(Vector2.RIGHT), "A full Flow bank must allow Phase Step")
	assert(player.call("is_field_dashing"), "Player must expose the live Phase Step state")
	var entry: Dictionary = player.call("prepare_field_entry_for_battle", "visible")
	assert(entry.mode == "ambush", "Crossing contact during Phase Step must become an ambush")
	assert(BattleManager.pending_field_entry_mode == "ambush", "Field entry must be handed to BattleManager")

	# Random pursuit can be actively broken rather than only endured.
	flow.flow = 80.0
	assert(flow.try_phase_step(Vector2.RIGHT) == false, "Phase Step cooldown must prevent button mashing")
	flow._dash_cooldown = 0.0
	flow._dash_timer = 0.0
	assert(flow.try_phase_step(Vector2.RIGHT), "Phase Step must be reusable after its cooldown")
	var encounter := RandomEncounter.setup([
		{"name": "Smoke Shade", "hp": 40, "atk": 8, "is_void": false},
	], "res://scenes/maps/rim_forest.tscn", "", "", 100, 100)
	encounter.threshold = 100.0
	encounter.step_count = 80.0
	encounter.warning_emitted = true
	encounter.last_player_pos = Vector2(32, 32)
	RandomEncounter.update(encounter, Vector2(64, 32), 32)
	assert(encounter.step_count < 80.0, "Phase Step must break a closing random-encounter trail")

	# Other approach routes remain explicit choices.
	var guard_flow := FieldFlow.new()
	add_child(guard_flow)
	guard_flow.flow = 58.0
	guard_flow.set_threat_source("random_encounter", 0.78)
	assert(guard_flow.get_forecast_mode() == "guarded", "Holding composure under pressure must forecast a guarded entry")
	assert(guard_flow.consume_battle_entry("ambient").mode == "guarded", "Ambient pressure must carry a guarded opening")

	var witness_flow := FieldFlow.new()
	add_child(witness_flow)
	witness_flow.flow = 58.0
	witness_flow.set_threat_source("visible:smoke:witness", 0.72)
	witness_flow.open_witness_window()
	assert(witness_flow.get_forecast_mode() == "witness", "Memory Pulse must override the forecast with WITNESS")
	assert(witness_flow.consume_battle_entry("visible").mode == "witness", "Pulse response must carry a witness opening")

	# Visible hunts are now readable threat actors instead of anonymous Areas.
	var threat := FieldThreat.new()
	threat.name = "WorldThreat_smoke"
	threat.configure("rim_forest", {"id": "smoke", "name": "Smoke Shade", "hp": 40, "atk": 8}, "world_hunt_smoke")
	add_child(threat)
	await get_tree().process_frame
	assert(threat.is_in_group("field_threat"), "Visible hunts must register as FieldThreat actors")
	assert(threat.source_id.begins_with("visible:"), "Visible threats need a stable pressure source identity")

	# The field decision changes the first turn numerically and visually.
	BattleManager.prepare_field_entry("ambush", 82)
	var enemy := BattleManager.Enemy.new("Smoke Shade", 90, 12, false)
	BattleManager.start_battle(enemy, "res://scenes/maps/rim_forest.tscn")
	assert(BattleManager.field_entry_mode == "ambush", "Battle must retain its field entry mode")
	assert(BattleManager.momentum >= 30.0, "Ambush must begin with real Momentum")
	assert(BattleManager.enemy_break_gauge >= 24.0, "Ambush must prime BREAK pressure")
	assert(BattleManager.limit_gauge >= 12.0, "Ambush must prime Limit")

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(BattleManager.paced(1.82)).timeout
	battle.call("_choose_tactical_objective", 0)
	await get_tree().create_timer(BattleManager.paced(0.28)).timeout
	var cue_title := battle.get("combat_cue_title") as Label
	assert(cue_title != null and "AMBUSH" in cue_title.text, "Battle scene must present the carried approach as an opening beat")

	var flow_panel := ExplorationHUD.get("flow_panel") as PanelContainer
	assert(flow_panel != null and flow_panel.name == "FieldFlowPanel", "Exploration HUD must own the persistent Flow/pressure panel")
	assert(ExplorationHUD.get("flow_bar") is ProgressBar, "Flow must be readable without opening a menu")
	assert(ExplorationHUD.get("pressure_bar") is ProgressBar, "Threat pressure must be readable before contact")

	battle.queue_free()
	threat.queue_free()
	player.queue_free()
	guard_flow.queue_free()
	witness_flow.queue_free()
	BattleManager.state = BattleManager.BattleState.IDLE
	BattleManager.current_enemy = null
	BattleManager.field_entry_mode = "neutral"
	BattleManager.field_entry_power = 0
	GameManager.story_flags = previous_flags
	GameManager.current_locale = previous_locale
	GameManager.current_chapter = previous_chapter
	OptionsMenu.settings["clean_gameplay_visuals"] = previous_clean
	OptionsMenu.settings["reduce_motion"] = previous_reduce
	GameManager.change_state(previous_state)
	print("FIELD_FLOW_SMOKE_PASS routes=3 phase_step=active pursuit_break=active battle_handoff=active")
	get_tree().quit(0)
