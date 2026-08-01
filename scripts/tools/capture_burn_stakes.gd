## S226: Real-renderer capture of the moment the loop hinges on: the burn
## preview under a directive that forbids burning.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/burn_preview_stakes.png"
const OUTPUT_PATH_KO := "res://tmp/visual_audit/burn_preview_stakes_ko.png"
const OUTPUT_PATH_APPROACH := "res://tmp/visual_audit/approach_entry_banner.png"
const OUTPUT_PATH_AFTERGLOW := "res://tmp/visual_audit/burn_afterglow.png"

func _ready() -> void:
	Codex.suppress_recording = true  # 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings.clean_gameplay_visuals = true
	OptionsMenu.settings.reduce_motion = true
	GameManager.current_locale = "en"
	GameManager.current_chapter = 2
	GameManager.player_data.hp = 78
	GameManager.player_data.max_hp = 115
	GameManager.player_data["elia_with_party"] = true
	for hint in ["first_battle", "first_directive", "first_burn", "first_approach"]:
		if hint not in TutorialHints.shown_hints:
			TutorialHints.shown_hints.append(hint)

	var enemy := BattleManager.Enemy.new("Ash Crawler", 96, 14, false)
	enemy.weakness = "fire"
	BattleManager.prepare_field_entry("ambush", 88)
	BattleManager.start_battle(
		enemy,
		"res://scenes/maps/verdan_market.tscn",
		"res://assets/cg/generated/chapter_splash_verdan_market.png"
	)
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

	var battle_packed := load("res://scenes/battle/battle_scene.tscn") as PackedScene
	var battle: Node = battle_packed.instantiate()
	add_child(battle)

	# 진입 방식이 전투 시작 몇 초 안에 실제로 보이는지 먼저 확인한다.
	var approach_captured := false
	for attempt in range(30):
		await get_tree().create_timer(0.2).timeout
		var cue := battle.get("combat_cue_panel") as PanelContainer
		if cue != null and cue.visible and cue.modulate.a > 0.8:
			var cue_detail := battle.get("combat_cue_detail") as Label
			assert("BREAK" in cue_detail.text, "The approach banner must state the opening it bought")
			if DisplayServer.get_name() != "headless":
				DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
				assert(get_viewport().get_texture().get_image().save_png(OUTPUT_PATH_APPROACH) == OK, "Approach capture must save")
			approach_captured = true
			break
	assert(approach_captured, "The ambush entry must announce itself in the opening seconds")
	await get_tree().create_timer(2.0).timeout

	var memory = MemoryManager._get_memory("daily_campfire_song")
	assert(memory != null, "The early Elia memory must be available for the capture")
	battle.call("_show_burn_preview", memory)
	await get_tree().create_timer(1.4).timeout

	var overflow := _measure_overflow(battle)
	if DisplayServer.get_name() == "headless":
		print("BURN_STAKES_CAPTURE_SKIP renderer=headless preview=1 overflow=%.1f" % overflow)
		get_tree().quit(0)
		return
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	assert(get_viewport().get_texture().get_image().save_png(OUTPUT_PATH) == OK, "Burn stakes capture must save")

	# 같은 패널을 한국어로 다시 세운다. 한글 줄바꿈에서 프레임이 터지지 않아야 한다.
	GameManager.current_locale = "ko"
	battle.call("_show_burn_preview", memory)
	await get_tree().create_timer(1.2).timeout
	var overflow_ko := _measure_overflow(battle)
	assert(get_viewport().get_texture().get_image().save_png(OUTPUT_PATH_KO) == OK, "Korean burn stakes capture must save")

	# 연소 직후의 잔향. 전투는 계속 조작 가능하지만 색은 바로 돌아오지 않는다.
	GameManager.current_locale = "en"
	battle.call("_hide_burn_preview")
	await get_tree().create_timer(0.6).timeout
	battle.call("_play_burn_afterglow", MemoryManager.MemoryGrade.GRADE_2)
	await get_tree().create_timer(0.9).timeout
	var glow := battle.get("_burn_afterglow_rect") as ColorRect
	assert(glow != null and is_instance_valid(glow), "The absence wash must exist after a burn")
	assert(glow.mouse_filter == Control.MOUSE_FILTER_IGNORE, "The wash must never take input")
	var desaturation: float = glow.material.get_shader_parameter("desaturation")
	assert(desaturation > 0.2, "The world must stay drained for a moment, desaturation=%.2f" % desaturation)
	assert(get_viewport().get_texture().get_image().save_png(OUTPUT_PATH_AFTERGLOW) == OK, "Afterglow capture must save")

	print("BURN_STAKES_CAPTURE_PASS path=%s ko_path=%s approach_path=%s afterglow_path=%s preview=1 overflow=%.1f overflow_ko=%.1f desaturation=%.2f" % [
		OUTPUT_PATH, OUTPUT_PATH_KO, OUTPUT_PATH_APPROACH, OUTPUT_PATH_AFTERGLOW, overflow, overflow_ko, desaturation
	])
	get_tree().quit(0)

func _measure_overflow(battle: Node) -> float:
	var panel := battle.get("_burn_preview_panel") as PanelContainer
	assert(panel != null and panel.visible, "The burn preview must be on screen for the capture")
	var vbox := panel.get_child(0) as VBoxContainer
	assert(vbox != null, "The preview must have built its content")
	var overflow: float = vbox.get_combined_minimum_size().y - panel.size.y
	assert(overflow <= 0.0, "The preview content must fit its frame, overflow=%.1f" % overflow)
	return overflow
