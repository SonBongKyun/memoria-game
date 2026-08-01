## S226: Guards the early-game loop that the demo lives or dies on:
## threat -> objective -> burn temptation -> immediate payoff and cost -> lingering consequence.
extends Node

const CH1_DIALOGUE := "res://data/chapter1_dialogue.json"

func _ready() -> void:
	Codex.suppress_recording = true  # 가짜 적을 개발자 도감에 남기지 않는다
	GameManager.current_locale = "en"
	GameManager.current_chapter = 1
	GameManager.player_data["elia_with_party"] = true
	for hint in ["first_battle", "first_directive", "first_burn", "first_approach"]:
		if hint not in TutorialHints.shown_hints:
			TutorialHints.shown_hints.append(hint)

	_check_burn_preview_stakes()
	await _check_directive_conflict_surfaces()
	_check_approach_payoff()
	_check_phase_bypass_reward()
	_check_revisit_lines()
	_check_npc_burn_reactions()

	print("EARLY_LOOP_SMOKE_PASS preview=1 conflict=1 forecast=1 approach=1 bypass=1 revisit=3 npc_reaction=1 aftermath=1")
	get_tree().quit(0)

## 연소 미리보기가 이득, 영구 대가, 세계 변화, 목표 충돌을 모두 드러내는가.
func _check_burn_preview_stakes() -> void:
	var memory = MemoryManager._get_memory("daily_campfire_song")
	assert(memory != null, "The early Elia memory must exist for the burn preview")
	var report := WorldRewriteDirector.get_rewrite_report("daily_campfire_song")
	assert(String(report.get("line", "")) != "", "An authored world consequence must back the burn preview")

## 미리보기와 예보와 커맨드 버튼이 Clean Hands 충돌을 동시에 말하는가.
func _check_directive_conflict_surfaces() -> void:
	var enemy := BattleManager.Enemy.new("Early Loop Probe", 120, 8, false)
	enemy.weakness = "physical"
	BattleManager.start_battle(enemy, "")
	BattleManager.tactical_objective = {
		"id": "keep_memory",
		"title": "Clean Hands",
		"desc": "Win without burning a memory.",
		"reward_grains": 5,
		"reward_item": "potion",
		"reward_heal": 0,
		"status": "active",
		"complete": false,
		"failed": false,
	}

	var relation := BattleManager.get_objective_action_relation("burn")
	assert(String(relation.get("kind", "")) == "fail", "Burning must read as a directive failure under Clean Hands")
	assert("Clean Hands" in String(relation.get("text", "")), "The conflict must name the directive it breaks")
	assert(String(BattleManager.get_objective_action_relation("attack").get("kind", "")) == "", "A neutral command must not invent a relation")

	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle_scene: Node = battle_packed.instantiate()
	add_child(battle_scene)
	await get_tree().process_frame
	await get_tree().process_frame

	var memory = MemoryManager._get_memory("daily_campfire_song")
	battle_scene.call("_show_burn_preview", memory)
	await get_tree().process_frame
	var preview_text := _collect_text(battle_scene.get("_burn_preview_panel"))
	assert("Clean Hands" in preview_text, "The preview must warn that this burn fails the directive")
	assert("cannot be restored" in preview_text, "The preview must state irreversibility on every burn")
	assert("Elia" in preview_text, "The preview must name the person the memory belongs to")
	assert("ELIA:" in preview_text, "Elia must speak up before an important memory of hers is spent")
	assert("WORLD:" in preview_text, "The preview must show the authored world consequence")
	assert("POWER GAINED" in preview_text, "The preview must still show what the burn buys")

	# 감각 잔편은 조용한 소모 자원으로 남아야 한다.
	var minor = MemoryManager._get_memory("sense_warm_light")
	assert(minor != null and not BattleManager.is_significant_memory(minor), "A plain sensation must stay a quiet resource")
	battle_scene.call("_show_burn_preview", minor)
	await get_tree().process_frame
	var minor_text := _collect_text(battle_scene.get("_burn_preview_panel"))
	assert("ELIA:" not in minor_text, "Elia must not narrate every burn")
	assert("cannot be restored" in minor_text, "Irreversibility still applies to the smallest memory")
	battle_scene.call("_hide_burn_preview")

	battle_scene.call("_show_action_forecast", "burn")
	var forecast: RichTextLabel = battle_scene.get("log_label")
	assert(forecast != null and forecast.text.begins_with(String(relation.get("text", ""))), "The forecast must lead with the objective relation")

	battle_scene.call("_update_objective_action_warnings")
	var burn_btn: Button = battle_scene.get("_action_buttons")["burn"]
	assert("Clean Hands" in burn_btn.text, "The burn command itself must carry the warning")
	var burn_style := burn_btn.get_theme_stylebox("normal") as StyleBoxFlat
	assert(burn_style != null and burn_style.border_color.r > 0.8, "The conflicting command must wear the warning colour")
	var attack_btn: Button = battle_scene.get("_action_buttons")["attack"]
	assert("Clean Hands" not in attack_btn.text, "Only the conflicting command may be marked")

	BattleManager._fail_tactical_objective("Smoke burn.")
	await get_tree().process_frame
	assert(String(battle_scene.get("_last_objective_status")) == "failed", "Losing the directive must fire its own moment once")

	BattleManager._memory_burns_this_battle = 1
	MemoryManager.burn_memory("daily_campfire_song")
	var aftermath := BattleManager._build_battle_aftermath_line()
	assert(aftermath != "", "Victory must close on what the fight cost")
	battle_scene.queue_free()
	await get_tree().process_frame

## 접근 방식이 전투 도입부에 구체적인 숫자로 드러나는가.
func _check_approach_payoff() -> void:
	BattleManager.field_entry_mode = "ambush"
	BattleManager.field_entry_power = 88
	var ambush := BattleManager.get_field_entry_bonus_text()
	assert("BREAK" in ambush and "Resonance" in ambush, "An ambush must state the opening it bought")
	BattleManager.field_entry_mode = "guarded"
	assert(BattleManager.get_field_entry_bonus_text() != "", "A guarded entry must state its opening")
	BattleManager.field_entry_mode = "witness"
	assert(BattleManager.get_field_entry_bonus_text() != "", "A witness entry must state its opening")
	BattleManager.field_entry_mode = "neutral"
	assert(BattleManager.get_field_entry_bonus_text() == "", "A plain approach stays quiet")

## 회피가 그냥 '전투 안 함'이 아니라 흐름 일부를 남기는가.
func _check_phase_bypass_reward() -> void:
	var flow := FieldFlow.new()
	add_child(flow)
	flow.flow = 20.0
	var after := flow.reward_phase_bypass()
	assert(is_equal_approx(after, 20.0 + FieldFlow.PHASE_STEP_COST * 0.5), "A clean bypass must return half the Phase Step")
	flow.flow = FieldFlow.FLOW_MAX
	assert(flow.reward_phase_bypass() <= FieldFlow.FLOW_MAX, "Flow must stay inside its bank")
	flow.queue_free()

## 초반 동선에서 다시 마주치는 잔향이 실제로 준비되어 있는가.
func _check_revisit_lines() -> void:
	var early := {
		"daily_campfire_song": "rim_forest",
		"sense_forest_smell": "rim_forest",
		"daily_market_food": "verdan_market",
	}
	for memory_id in early:
		var map_id := String(early[memory_id])
		assert(WorldRewriteDirector.get_revisit_line(memory_id, map_id) != "", "%s must be answered again on %s" % [memory_id, map_id])
	assert(WorldRewriteDirector.get_revisit_line("daily_campfire_song", "bl07_void") == "", "A revisit line must stay on its own route")
	assert(WorldRewriteDirector.get_map_id_for_scene("res://scenes/maps/rim_forest.tscn") == "rim_forest", "Map keys come from the scene path")

	# daily_campfire_song은 위에서 이미 태웠다. 같은 맵에서는 한 번만 말한다.
	var picked = WorldRewriteDirector._pick_revisit_memory("rim_forest")
	assert(picked != null and String(picked.id) == "daily_campfire_song", "A burned early memory must be waiting on its map")

	# 맵으로 돌아왔을 때 실제로 잔향이 발화되는지 확인한다.
	var stand_in := Node.new()
	stand_in.scene_file_path = "res://scenes/maps/rim_forest.tscn"
	get_tree().root.add_child(stand_in)  # current_scene은 루트의 직계 자식이어야 한다
	var previous_scene := get_tree().current_scene
	var previous_state := GameManager.current_state
	get_tree().current_scene = stand_in
	GameManager.current_state = GameManager.GameState.EXPLORATION
	WorldRewriteDirector._manifest_scene_residue()
	get_tree().current_scene = previous_scene
	GameManager.current_state = previous_state
	assert(GameManager.get_flag(WorldRewriteDirector.revisit_flag("daily_campfire_song", "rim_forest")), "Returning to the map must spend the authored absence line")
	assert(stand_in.get_child_count() > 0, "The absence must actually appear in the scene")
	stand_in.queue_free()

	assert(WorldRewriteDirector._pick_revisit_memory("rim_forest") == null, "The same absence must not repeat on the same map")

## 연소 이후 인물이 한 번 반응하고, 그 뒤로는 반복하지 않는가.
func _check_npc_burn_reactions() -> void:
	var speaker := Node.new()
	speaker.set_meta("burn_reaction_daily_campfire_song", "elia_song_burned")
	add_child(speaker)
	var first := PerceptionFilter.take_burn_reaction(speaker, CH1_DIALOGUE)
	assert(String(first.get("key", "")) == "elia_song_burned", "Elia must answer the burn the next time she is met")
	assert(String(first.get("file", "")) == CH1_DIALOGUE, "A bare key uses the character's own dialogue file")
	assert(PerceptionFilter.take_burn_reaction(speaker, CH1_DIALOGUE).is_empty(), "The reaction must be a one-time beat")

	var cross := Node.new()
	cross.set_meta("burn_reaction_daily_campfire_song", "%s::elia_sword_burned" % CH1_DIALOGUE)
	add_child(cross)
	var reused := PerceptionFilter.take_burn_reaction(cross, "res://data/chapter2_dialogue.json")
	assert(String(reused.get("file", "")) == CH1_DIALOGUE, "An explicit file lets later chapters reuse the reaction")

	var broken := Node.new()
	broken.set_meta("burn_reaction_daily_campfire_song", "no_such_dialogue_key")
	add_child(broken)
	assert(PerceptionFilter.take_burn_reaction(broken, CH1_DIALOGUE).is_empty(), "A missing reaction must be skipped, not silently eaten")
	assert(not GameManager.get_flag("burn_reaction_heard_no_such_dialogue_key"), "A skipped reaction must not burn its flag")
	speaker.queue_free()
	cross.queue_free()
	broken.queue_free()

func _collect_text(node: Node) -> String:
	if node == null:
		return ""
	var text := ""
	if node is Label:
		text += (node as Label).text + "\n"
	elif node is Button:
		text += (node as Button).text + "\n"
	elif node is RichTextLabel:
		text += (node as RichTextLabel).text + "\n"
	for child in node.get_children():
		text += _collect_text(child)
	return text
