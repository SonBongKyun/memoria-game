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
	assert(GameManager.ITEMS.has("witness_ink"), "The preservation route needs a dedicated tactical item")
	assert(ResourceLoader.exists(GameManager.ITEMS["witness_ink"]["icon"]), "Witness Ink must resolve to a shipped illustrated icon")
	for atlas_item in ["root_balm", "signal_jammer", "lantern_salve", "name_thread", "compass_shard", "seed_capsule"]:
		assert(GameManager.ITEMS.has(atlas_item), "%s must be a usable atlas reward" % atlas_item)
		assert(ResourceLoader.exists(GameManager.ITEMS[atlas_item]["icon"]), "%s must resolve to a shipped illustrated icon" % atlas_item)

	# Item balance probe: consumables must create a tactical decision rather than
	# functioning as dead inventory between shop visits.
	var item_enemy := BattleManager.Enemy.new("Item Probe", 120, 0, false)
	_prepare_item_probe(item_enemy)
	GameManager.player_data.items = {"antidote": 1}
	GameManager.player_data.hp = 60
	BattleManager.player_statuses.append(BattleManager.StatusEntry.new(BattleManager.StatusEffect.POISON, 2, 6))
	BattleManager.player_use_item("antidote")
	assert(GameManager.player_data.hp == 72 and BattleManager.player_statuses.is_empty(), "Antidote must both clear pressure and restore a small recovery buffer")
	await _wait_for_player_turn()

	item_enemy = BattleManager.Enemy.new("Item Probe", 120, 0, false)
	_prepare_item_probe(item_enemy)
	GameManager.player_data.items = {"root_balm": 1}
	GameManager.player_data.hp = 40
	BattleManager.player_statuses.append(BattleManager.StatusEntry.new(BattleManager.StatusEffect.BURN, 2, 8))
	BattleManager.player_use_item("root_balm")
	assert(GameManager.player_data.hp == 68 and BattleManager.player_statuses.is_empty(), "Root Balm must be a stronger field-earned cure rather than cosmetic inventory")
	await _wait_for_player_turn()

	item_enemy = BattleManager.Enemy.new("Item Probe", 120, 0, false)
	_prepare_item_probe(item_enemy)
	GameManager.player_data.items = {"firebomb": 1}
	BattleManager.player_use_item("firebomb")
	assert(item_enemy.hp < 105 and not BattleManager.enemy_statuses.is_empty(), "Firebomb must apply immediate impact as well as its first burn tick")
	await _wait_for_player_turn()

	item_enemy = BattleManager.Enemy.new("Item Probe", 120, 0, false)
	_prepare_item_probe(item_enemy)
	GameManager.player_data.items = {"witness_ink": 1}
	BattleManager.player_use_item("witness_ink")
	assert(BattleManager._witness_progress == 1 and GameManager.get_item_count("witness_ink") == 0, "Witness Ink must advance a reading without bypassing it")
	await _wait_for_player_turn()
	assert(BattleManager.state == BattleManager.BattleState.PLAYER_TURN, "Witness Ink must preserve the normal turn flow")

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

func _prepare_item_probe(enemy: BattleManager.Enemy) -> void:
	BattleManager.current_enemy = enemy
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = ""
	BattleManager.tactical_objective = {}
	BattleManager.player_statuses.clear()
	BattleManager.enemy_statuses.clear()
	BattleManager.player_defending = false
	BattleManager._witness_progress = 0
	BattleManager._witness_required = BattleManager._get_witness_requirement(enemy)
	BattleManager._witness_completed_this_battle = false
	BattleManager._resolved_by_witness = false
	BattleManager._witness_boss_insight = false
	BattleManager._battle_started_as_boss_rush = true

func _on_rewards(rewards: Dictionary) -> void:
	captured_rewards = rewards.duplicate(true)
	BattleManager.dismiss_victory()
