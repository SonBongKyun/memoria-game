## S217 회귀 테스트 — 장비 비교, 도감 힌트, 상점 시세, 저널 미해결 단서
extends Node

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 도감 저장을 건드리지 않는다
	var prev_flags := GameManager.story_flags.duplicate(true)
	var prev_locale := GameManager.current_locale
	var prev_grains: int = GameManager.player_data.get("grains", 0)
	var prev_equipped := GameManager.equipped.duplicate(true)
	GameManager.current_locale = "ko"

	_check_equipment_comparison()
	_check_shop_rates()
	await _check_bestiary_hints()
	await _check_journal_leads()

	GameManager.story_flags = prev_flags
	GameManager.current_locale = prev_locale
	GameManager.player_data["grains"] = prev_grains
	GameManager.equipped = prev_equipped
	print("RPG_DEPTH_SMOKE_PASS compare=1 bestiary=1 rates=1 leads=1")
	get_tree().quit(0)

## 절대값만 보면 이미 더 좋은 장비를 차고 있어도 이득처럼 읽힌다.
func _check_equipment_comparison() -> void:
	GameManager.equipped["weapon"] = ""
	var fresh := GameManager.compare_equipment("iron_sword")
	assert(not fresh.is_empty(), "장비 비교 결과가 있어야 한다")
	assert(int(fresh.get("atk_delta", 0)) == int(GameManager.EQUIPMENT["iron_sword"].get("atk", 0)),
		"빈 슬롯이면 장비 성능이 곧 증가분이다")
	assert(fresh.get("is_upgrade", false), "빈 슬롯에 무기를 끼면 상승이어야 한다")

	# 더 좋은 것을 이미 차고 있으면 하락으로 나와야 한다.
	GameManager.equipped["weapon"] = "void_edge"   # ATK 15
	var worse := GameManager.compare_equipment("rusty_blade")  # ATK 3
	assert(int(worse.get("atk_delta", 0)) < 0,
		"더 나쁜 무기는 하락으로 표시되어야 한다 (%d)" % int(worse.get("atk_delta", 0)))
	assert(not worse.get("is_upgrade", true), "하락을 상승으로 표시하면 안 된다")
	var line := GameManager.format_equipment_delta("rusty_blade")
	assert("ATK" in line and "-" in line, "증감이 문장으로 보여야 한다: %s" % line)
	assert("교체" in line, "무엇을 교체하는지 한국어로 알려야 한다: %s" % line)

	# 강화 수치도 비교에 반영되어야 한다.
	GameManager.upgrade_levels["void_edge"] = 2
	var vs_upgraded := GameManager.compare_equipment("ember_brand")
	assert(int(vs_upgraded.get("atk_delta", 0)) < int(GameManager.EQUIPMENT["ember_brand"].get("atk", 0)),
		"강화된 현재 장비를 기준으로 비교해야 한다")
	GameManager.upgrade_levels.erase("void_edge")

## 가격 숫자만으로는 비싼지 싼지 알 수 없다.
func _check_shop_rates() -> void:
	GameManager.player_data["grains"] = 10
	var expensive := {"type": "buy_equip", "equip_id": "void_edge", "price": 80}
	var line: String = MemoryShop.call("_price_line", 80)
	assert("부족" in line, "살 수 없으면 얼마가 모자란지 알려야 한다: %s" % line)
	GameManager.player_data["grains"] = 500
	line = MemoryShop.call("_price_line", 80)
	assert("잔액" in line, "살 수 있으면 남는 금액을 알려야 한다: %s" % line)

	var rate: String = MemoryShop.call("_market_rate_label", expensive)
	assert(rate != "", "시세 표기가 있어야 한다")
	var cheap := {"type": "buy_equip", "equip_id": "rusty_blade", "price": 15}
	var cheap_rate: String = MemoryShop.call("_market_rate_label", cheap)
	assert(cheap_rate != rate, "싼 물건과 비싼 물건의 시세 표기는 달라야 한다 (%s vs %s)" % [cheap_rate, rate])

## 도감은 "만난 적"만 담아서, 무엇이 남았는지 알 수 없었다.
func _check_bestiary_hints() -> void:
	var roster: Dictionary = Codex.call("_known_enemy_roster")
	assert(roster.size() >= 8, "전체 적 명단이 모여야 한다 (현재 %d)" % roster.size())
	var with_region := 0
	for entry_name: String in roster:
		if String(roster[entry_name]) != "":
			with_region += 1
	assert(with_region >= 3, "지역이 확인되는 적은 그 지역을 힌트로 줘야 한다")

	# S218: 명단은 게임이 한국어 이름을 붙여 둔 모든 적을 담아야 한다.
	# 이 명단이 빠지면 purge_unknown_entries()가 진짜 적을 지운다 (실제로 겪었다).
	for enemy_name: String in GameManager.ENEMY_NAMES_KO:
		assert(roster.has(enemy_name),
			"'%s' 는 실제 게임 적인데 도감 명단에 없다. 정리 기능이 이 항목을 지운다." % enemy_name)

	# 정리 기능은 진짜 적을 건드리면 안 된다.
	# suppress_recording이 켜져 있으므로 이 조작은 저장 파일에 닿지 않는다.
	assert(Codex.suppress_recording, "테스트는 도감 저장을 건드리면 안 된다")
	var saved_entries := Codex.enemy_entries.duplicate(true)
	Codex.enemy_entries = {
		"Void Wraith": {"encounters": 1, "defeated": 0},
		"Cleanup Victory": {"encounters": 1, "defeated": 0},
	}
	var purged := Codex.purge_unknown_entries()
	assert(Codex.enemy_entries.has("Void Wraith"), "실제 적을 정리 대상으로 삼으면 안 된다")
	assert(not Codex.enemy_entries.has("Cleanup Victory"), "테스트 항목은 정리되어야 한다")
	assert(purged.size() == 1, "정리 대상은 테스트 항목 하나여야 한다")
	Codex.enemy_entries = saved_entries

	var hint_known: String = Codex.call("_unmet_hint", "림 숲")
	assert("림 숲" in hint_known, "지역이 있으면 힌트에 지역이 들어가야 한다: %s" % hint_known)
	var hint_unknown: String = Codex.call("_unmet_hint", "")
	assert("림 숲" not in hint_unknown, "근거 없는 지역을 지어내면 안 된다: %s" % hint_unknown)
	await get_tree().process_frame

## 저널은 끝난 일만 기록했다. 남은 것을 세어 줄 필요가 있다.
func _check_journal_leads() -> void:
	GameManager.story_flags = {}
	# 탭 버튼이 화면에 붙어 있어야 플레이어가 이 기능에 닿을 수 있다.
	# (_create_tab은 버튼을 만들기만 하고 부모에 붙이지는 않는다.)
	StoryJournal.call("_build_ui")
	assert(StoryJournal.tab_leads_btn != null and StoryJournal.tab_leads_btn.get_parent() != null,
		"미해결 탭 버튼이 탭 줄에 추가되어야 한다")
	StoryJournal.call("_populate_leads")
	# 아무것도 손대지 않은 상태에서는 지역별 미해결 항목이 나와야 한다.
	var listed := StoryJournal.item_list.get_child_count()
	assert(listed > 0, "미해결 단서가 목록에 나와야 한다")

	# 전부 처리하면 목록이 비어야 한다.
	for map_id: String in WorldPopulation.POPULATIONS:
		var population: Dictionary = WorldPopulation.POPULATIONS[map_id]
		for cache: Dictionary in population.get("caches", []):
			GameManager.set_flag("world_cache_%s_%s" % [map_id, String(cache.get("id", ""))])
		for curio: Dictionary in StoryJournal.call("_curios_for", map_id):
			GameManager.set_flag("world_curio_%s_%s" % [map_id, String(curio.get("id", ""))])
		for point: Dictionary in MemoryResonance.RESONANCE_POINTS.get(map_id, []):
			GameManager.set_flag(String(point.get("flag", "")))
	for child in StoryJournal.item_list.get_children():
		child.free()
	StoryJournal.call("_populate_leads")
	var cleared_text := ""
	for child in StoryJournal.item_list.get_children():
		if child is Label:
			cleared_text = (child as Label).text
	assert("남은 단서가 없습니다" in cleared_text,
		"전부 처리하면 비었다고 말해야 한다: %s" % cleared_text)
	await get_tree().process_frame
