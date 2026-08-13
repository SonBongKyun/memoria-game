extends Node

## S245: 튜토리얼 힌트 띠가 전투 HUD의 무엇을 얼마나 가리는지 잰다.
##
## S241에서 힌트 패널의 높이 결함(768x806)을 고치면서 관찰만 하고 남긴 것이 있다.
## 띠가 4초 동안 "전장 관측" 카드와 적 이름을 덮는다. 그때는 눈으로만 봤으므로
## 이번에는 면적으로 잰다.
##
## 노드 이름을 추측하지 않는다. 힌트 사각형과 겹치는 **보이는 Control 전부**를
## 찾아 겹친 면적 순으로 신고한다. 그래야 내가 모르는 요소를 빠뜨리지 않는다.

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 1
	GameManager.change_state(GameManager.GameState.BATTLE)
	BattleManager.start_battle("ash_crawler", "res://scenes/maps/rim_forest.tscn")

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.5).timeout
	battle.call("_on_player_turn")
	await get_tree().create_timer(0.6).timeout

	var panel := TutorialHints.get("_panel") as Control
	if panel == null or not panel.visible:
		print("HINT_OVERLAP panel_visible=false (힌트가 뜨지 않았다)")
		get_tree().quit(1)
		return
	var hint_rect := panel.get_global_rect()
	print("HINT_OVERLAP hint=(%.0f, %.0f)-(%.0f, %.0f) area=%.0f" % [
		hint_rect.position.x, hint_rect.position.y,
		hint_rect.end.x, hint_rect.end.y, hint_rect.get_area()])

	var hits: Array = []
	_collect(battle, hint_rect, hits)
	hits.sort_custom(func(a, b): return a["area"] > b["area"])
	var total := 0.0
	for hit in hits.slice(0, 8):
		total += float(hit["area"])
		print("HINT_OVERLAP covered %7.0fpx  %-28s %s" % [hit["area"], hit["class"], hit["path"]])
	print("HINT_OVERLAP_TOTAL covered_controls=%d top8_area=%.0f" % [hits.size(), total])

	# 어디로 옮길지도 추측하지 않는다. 같은 폭의 띠를 화면 위에서 아래로 훑으며
	# 겹침 면적을 재고, 가장 조용한 자리를 찾는다. 화면 전체를 덮는 배경은
	# 어디에 두어도 겹치므로 후보 비교에서 제외한다.
	var best_y := -1.0
	var best_area := INF
	var viewport := get_viewport().get_visible_rect().size
	for step in range(0, int(viewport.y - hint_rect.size.y), 10):
		var candidate := Rect2(hint_rect.position.x, float(step), hint_rect.size.x, hint_rect.size.y)
		var probe_hits: Array = []
		_collect(battle, candidate, probe_hits, viewport, true)
		var area := 0.0
		for hit in probe_hits:
			area += float(hit["area"])
		if area < best_area:
			best_area = area
			best_y = float(step)
		print("HINT_SCAN y=%3d overlap=%.0f" % [step, area])
	print("HINT_SCAN_BEST y=%.0f overlap=%.0f" % [best_y, best_area])
	get_tree().quit(0)

## 힌트와 겹치는, 실제로 보이는 Control을 모은다. 컨테이너 자체는 배경이 없으면
## 화면에 아무것도 안 그리므로 스타일이나 텍스트가 있는 것만 센다.
## `readable_only`면 글자와 막대만 센다. 4초 동안 무대 그림을 가리는 것과
## 적의 HP를 가리는 것은 값이 다르다. 후보 자리를 고를 때는 정보만 세는 게 맞다.
func _collect(node: Node, hint_rect: Rect2, hits: Array, viewport: Vector2 = Vector2.ZERO,
		readable_only: bool = false) -> void:
	var control := node as Control
	if control != null and control.is_visible_in_tree():
		var draws := control is Label or control is Button or control is ProgressBar
		if not readable_only:
			draws = draws or control is TextureRect \
				or (control is PanelContainer and control.has_theme_stylebox("panel"))
		var rect := control.get_global_rect() if draws else Rect2()
		# 화면의 8할 이상을 덮는 것은 배경이다. 어디에 두어도 겹치므로 후보 비교에서 뺀다.
		if draws and viewport != Vector2.ZERO and rect.get_area() >= viewport.x * viewport.y * 0.8:
			draws = false
		if draws:
			var overlap := rect.intersection(hint_rect)
			if overlap.get_area() > 200.0:
				hits.append({
					"area": overlap.get_area(),
					"class": control.get_class(),
					"path": String(control.name),
				})
	for child in node.get_children():
		_collect(child, hint_rect, hits, viewport, readable_only)
