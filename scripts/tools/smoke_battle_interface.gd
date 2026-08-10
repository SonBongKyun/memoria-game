extends Node

## S230: 전투 화면의 "읽히는가" 계약을 좌표로 검사한다.
##
## 이 화면이 무너지는 방식은 늘 같았다. 새 패널이 추가될 때마다 각자 앵커 숫자를
## 들고 오고, 실제로 겹치는지는 눈으로만 확인했다. 실측해 보니 겹침이 열 군데였고
## 그중 셋은 글자를 잘라 먹고 있었다. 여기서는 세 가지를 고정한다.
##
##   1. 서로 다른 HUD 카드는 겹치지 않는다 (부모-자식 관계는 예외).
##   2. 어떤 카드도 전투원 판이 서는 대역을 침범하지 않는다.
##   3. 전투 화면의 모든 글자는 타입 스케일 하한 위에 있다.

const BATTLER_KEYS: Array[String] = ["player_sprite", "ally_sprite", "tobias_sprite", "enemy_sprite"]

## 겹치면 안 되는 독립 카드들.
const HUD_KEYS: Array[String] = [
	"objective_panel",
	"combat_cue_panel",
	"party_orders_panel",
	"enemy_panel",
	"player_panel",
	"turn_banner",
	"_battle_speed_btn",
	"turn_preview_container",
	"action_container",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	var previous_locale: String = GameManager.current_locale

	for locale in ["ko", "en"]:
		GameManager.current_locale = locale
		await _check_locale(locale)

	GameManager.current_locale = previous_locale
	print("BATTLE_INTERFACE_SMOKE_PASS locales=2 cards=%d battlers=%d type_scale=enforced" % [HUD_KEYS.size(), BATTLER_KEYS.size()])
	get_tree().quit(0)

func _check_locale(locale: String) -> void:
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

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	# 전투원 판은 3D 무대 표식에 맞춰 자리를 잡는다. 인트로 암전이 걷힌 뒤의
	# 안정된 배치를 검사해야 의미가 있다.
	await get_tree().create_timer(1.2).timeout

	# 최대 파티 + 전투 큐 + 턴 배너까지 전부 켠, 가장 붐비는 상태로 검사한다.
	battle.call("_show_combat_cue", "FIELD", "The enemy prepares a basic attack.", "", Color(0.6, 0.8, 1.0), 4.0)
	for row_key in ["stance_container", "elia_skill_container", "ally_cmd_container", "tobias_cmd_container"]:
		var row := battle.get(row_key) as Control
		if row != null:
			row.visible = true
	battle.call("_refresh_party_orders_panel")
	battle.call("_show_turn_indicator", "TURN", Color(0.8, 0.8, 0.6))
	var banner := battle.get("turn_banner") as Control
	if banner:
		banner.modulate.a = 1.0
	# 상태 칩이 가장 많이 붙은 상태까지 밀어붙인다.
	BattleManager.enemy_shielded = true
	BattleManager.player_defending = true
	BattleManager.combo_count = 3
	BattleManager.scanned_enemies.append("Shade Sentinel")
	battle.call("_update_status_icons")
	await get_tree().process_frame
	await get_tree().process_frame

	_assert_no_card_overlap(battle, locale)
	_assert_battler_lane_is_clear(battle, locale)
	_assert_type_scale(battle, locale)
	_assert_localized(battle, locale)

	BattleManager.enemy_shielded = false
	BattleManager.player_defending = false
	BattleManager.combo_count = 0
	BattleManager.scanned_enemies.erase("Shade Sentinel")
	battle.queue_free()
	await get_tree().process_frame

func _visible_rect(battle: Node, key: String) -> Rect2:
	var node := battle.get(key) as Control
	if node == null or not node.is_visible_in_tree():
		return Rect2()
	return node.get_global_rect()

func _assert_no_card_overlap(battle: Node, locale: String) -> void:
	for i in range(HUD_KEYS.size()):
		var a := _visible_rect(battle, HUD_KEYS[i])
		if a.size.x <= 1.0:
			continue
		for j in range(i + 1, HUD_KEYS.size()):
			var b := _visible_rect(battle, HUD_KEYS[j])
			if b.size.x <= 1.0:
				continue
			assert(not a.intersects(b), "[%s] %s and %s overlap: %s vs %s" % [locale, HUD_KEYS[i], HUD_KEYS[j], a, b])

func _assert_battler_lane_is_clear(battle: Node, locale: String) -> void:
	# 커맨드 덱과 판독 리본은 무대 아래 자기 자리를 갖고 있으므로 제외한다.
	var stage_cards := ["objective_panel", "combat_cue_panel", "party_orders_panel", "enemy_panel", "player_panel", "turn_banner"]
	for battler_key in BATTLER_KEYS:
		var battler := _visible_rect(battle, battler_key)
		if battler.size.x <= 1.0:
			continue
		for card_key in stage_cards:
			var card := _visible_rect(battle, card_key)
			if card.size.x <= 1.0:
				continue
			assert(not battler.intersects(card), "[%s] %s %s must not cover the %s plate %s" % [locale, card_key, card, battler_key, battler])

func _assert_type_scale(battle: Node, locale: String) -> void:
	var canvas := battle.get("canvas_root") as Control
	assert(canvas != null, "The battle UI root must exist")
	var checked := 0
	for node in _walk(canvas):
		if node is Label:
			var label := node as Label
			if label.text.strip_edges() == "":
				continue
			assert(label.get_theme_font_size("font_size") >= UITheme.MIN_META_FONT_SIZE,
				"[%s] battle label '%s' is below the readable floor" % [locale, label.text])
			checked += 1
		elif node is Button:
			var button := node as Button
			assert(button.get_theme_font_size("font_size") >= UITheme.MIN_META_FONT_SIZE,
				"[%s] battle command '%s' is below the readable floor" % [locale, button.text])
			# 비활성 커맨드도 자기 배경을 가져야 장식 위에서 읽힌다.
			var disabled_style := button.get_theme_stylebox("disabled") as StyleBoxFlat
			assert(disabled_style != null and disabled_style.bg_color.a > 0.85,
				"[%s] '%s' needs an opaque disabled surface" % [locale, button.text])
			checked += 1
	assert(checked >= 20, "[%s] the battle interface should expose far more text than %d nodes" % [locale, checked])

func _assert_localized(battle: Node, locale: String) -> void:
	if locale != "ko":
		return
	# 한국어 판에서 영어 잔재가 남기 쉬운 자리들.
	var break_label := battle.get("enemy_break_label") as Label
	assert(break_label != null and break_label.text == "브레이크", "Korean battle must not show a raw BREAK tag")
	for row_key in ["ally_cmd_container", "tobias_cmd_container"]:
		var row := battle.get(row_key) as Container
		if row == null:
			continue
		for child in row.get_children():
			if child is Button:
				assert(not (child as Button).text.is_valid_identifier(), "Korean party orders must not stay in English: %s" % (child as Button).text)
	var player_chips := battle.get("player_status_container") as Container
	assert(player_chips != null and player_chips.get_child_count() > 0, "Party and guard chips must be built")
	for child in player_chips.get_children():
		var label := child.get_child(0) as Label
		assert(label != null and not label.text.is_valid_identifier(), "Korean status chip must not stay in English: %s" % label.text)

func _walk(node: Node) -> Array[Node]:
	var found: Array[Node] = []
	for child in node.get_children():
		found.append(child)
		found.append_array(_walk(child))
	return found
