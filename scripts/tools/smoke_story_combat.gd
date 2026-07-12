extends Node

var captured_rewards: Dictionary = {}

func _ready() -> void:
	GameManager.current_locale = "ko"
	GameManager.player_data.hp = 100
	GameManager.player_data.max_hp = 100
	GameManager.player_data.field_focus = 0
	GameManager.play_stats.enemies_witnessed = 0
	GameManager.set_flag("listened_to_humming", true)

	var void_enemy := BattleManager.Enemy.new("Void Beast", 80, 1, true)
	assert(BattleManager._get_witness_requirement(void_enemy) == 2, "Elia's remembered humming must shorten a void reading")
	var boss := BattleManager.Enemy.new("Shade Sentinel", 180, 20, true)
	boss.is_boss = true
	assert(BattleManager._get_witness_requirement(boss) == 3, "Story bosses must still require a full reading")

	var enemy := BattleManager.Enemy.new("Ash Crawler", 80, 1, false)
	BattleManager.current_enemy = enemy
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = ""
	BattleManager.tactical_objective = {}
	BattleManager.scanned_enemies.clear()
	BattleManager._witness_progress = 0
	BattleManager._witness_required = BattleManager._get_witness_requirement(enemy)
	BattleManager._witness_completed_this_battle = false
	BattleManager._resolved_by_witness = false
	BattleManager._witness_boss_insight = false
	BattleManager._battle_started_as_boss_rush = true
	BattleManager.victory_rewards_ready.connect(_on_rewards, CONNECT_ONE_SHOT)

	BattleManager.player_witness()
	assert(BattleManager._witness_progress == 1, "First WITNESS must advance the reading")
	await _wait_for_player_turn()
	assert(BattleManager.state == BattleManager.BattleState.PLAYER_TURN, "WITNESS must return control after the enemy response")

	BattleManager.player_witness()
	await get_tree().create_timer(0.1).timeout
	assert(BattleManager._resolved_by_witness, "Second WITNESS must release an ordinary echo")
	assert(GameManager.play_stats.enemies_witnessed == 1, "Released echoes must be tracked")
	assert(captured_rewards.get("resolution", "") == "witness", "Victory rewards must identify a WITNESS resolution")
	assert(int(captured_rewards.get("preservation_bonus", 0)) > 0, "WITNESS resolution must grant a preservation bonus")
	assert(int(captured_rewards.get("field_focus_gained", 0)) == 1, "WITNESS resolution must bank Field Focus")

	print("STORY_COMBAT_SMOKE_PASS witness=2 release=1 choice_echo=1 preservation_bonus=%d focus=1" % int(captured_rewards.preservation_bonus))
	get_tree().quit(0)

func _wait_for_player_turn() -> void:
	var elapsed := 0.0
	while BattleManager.state != BattleManager.BattleState.PLAYER_TURN and elapsed < 4.0:
		await get_tree().create_timer(0.05).timeout
		elapsed += 0.05

func _on_rewards(rewards: Dictionary) -> void:
	captured_rewards = rewards.duplicate(true)
	BattleManager.dismiss_victory()
