extends Node

## S230 개발용 계측기. 전투 HUD 영역과 전투원 판의 실제 사각형을 찍어,
## 겹침을 눈이 아니라 좌표로 확인한다. 마지막에 전 요소를 켠 화면 한 장을 남긴다.

const OUTPUT_PATH := "res://tmp/visual_audit/battle_layout_probe.png"

const REGIONS: Array[String] = [
	"objective_panel", "objective_art", "combat_cue_panel",
	"_battle_speed_btn", "auto_label", "turn_preview_container",
	"enemy_panel", "player_panel", "party_orders_panel", "turn_banner",
	"enemy_status_container", "player_status_container",
	"enemy_hp_bar", "player_hp_bar", "limit_bar",
	"field_readout_art", "action_ribbon_art", "action_container",
	"stance_container", "elia_skill_container", "ally_cmd_container", "tobias_cmd_container",
	"turn_label", "_combo_display_label",
	"player_sprite", "ally_sprite", "tobias_sprite", "enemy_sprite",
]

func _ready() -> void:
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
	battle.call("_on_player_turn")
	await get_tree().create_timer(0.8).timeout

	var rects: Dictionary = {}
	for key in REGIONS:
		var node := battle.get(key) as Control
		if node == null:
			print("PROBE %-26s <missing>" % key)
			continue
		var r := node.get_global_rect()
		rects[key] = {"rect": r, "visible": node.is_visible_in_tree()}
		print("PROBE %-26s vis=%s x=[%7.1f %7.1f] y=[%7.1f %7.1f]" % [
			key, str(node.is_visible_in_tree()), r.position.x, r.end.x, r.position.y, r.end.y
		])

	print("--- overlaps (visible only) ---")
	var keys: Array = rects.keys()
	for i in range(keys.size()):
		for j in range(i + 1, keys.size()):
			var a: Dictionary = rects[keys[i]]
			var b: Dictionary = rects[keys[j]]
			if not a.visible or not b.visible:
				continue
			var ra: Rect2 = a.rect
			var rb: Rect2 = b.rect
			if ra.size.x <= 1.0 or rb.size.x <= 1.0:
				continue
			if ra.intersects(rb):
				var o := ra.intersection(rb)
				print("OVERLAP %s <> %s  (%.0fx%.0f)" % [keys[i], keys[j], o.size.x, o.size.y])
	# 잠깐만 나타나는 연출(턴 배너, 연계 표시)을 켠 채로 한 장 남긴다.
	battle.call("_show_turn_indicator", "당신의 턴", Color(0.72, 0.88, 1.0))
	var banner := battle.get("turn_banner") as Control
	var combo := battle.get("_combo_display_label") as Label
	if combo:
		combo.text = "연계 x3"
	await get_tree().create_timer(0.25).timeout
	if banner:
		banner.modulate.a = 1.0
	if combo:
		combo.modulate.a = 1.0
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	if image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK:
		print("BATTLE_LAYOUT_PROBE_SHOT %s" % OUTPUT_PATH)
	print("BATTLE_LAYOUT_PROBE_DONE")
	get_tree().quit(0)
