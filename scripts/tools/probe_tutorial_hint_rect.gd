extends Node

## S241: 튜토리얼 힌트 패널이 실제로 차지하는 사각형을 찍는다.
##
## 전투 서베이 1번 칸(챕터1 첫 교전)에서 어두운 띠가 화면 중앙을 세로로 통째
## 가로지르며 덱의 1·3번 버튼까지 덮었다. 코드상 패널은 offset_top 10,
## offset_bottom 64로 높이 54여야 한다. 둘 중 하나가 틀렸으니 실측한다.

func _ready() -> void:
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	# 가장 긴 한국어 힌트도 함께 본다. 높이가 내용에 따라 늘어나야 잘리지 않는다.
	for hint_id in ["first_battle", "first_approach", "first_directive"]:
		TutorialHints.show_hint(hint_id)
		await get_tree().create_timer(0.8).timeout
		var p := TutorialHints.get("_panel") as Control
		var l := TutorialHints.get("_label") as Label
		print("TUTORIAL_HINT %-16s panel=(%.0f x %.0f) label=(%.0f x %.0f) lines=%d" % [
			hint_id, p.size.x, p.size.y, l.size.x, l.size.y, l.get_line_count()])
		await RenderingServer.frame_post_draw
		var shot := get_viewport().get_texture().get_image()
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
		shot.save_png(ProjectSettings.globalize_path("res://tmp/visual_audit/hint_%s.png" % hint_id))
		TutorialHints.call("_dismiss")
		await get_tree().create_timer(0.4).timeout
	TutorialHints.shown_hints.clear()
	TutorialHints.show_hint("first_battle")
	await get_tree().create_timer(0.8).timeout
	var panel := TutorialHints.get("_panel") as Control
	var viewport := get_viewport().get_visible_rect().size
	if panel == null:
		print("TUTORIAL_HINT_RECT panel=null")
	else:
		var rect := panel.get_global_rect()
		print("TUTORIAL_HINT_RECT pos=(%.0f, %.0f) size=(%.0f x %.0f) viewport=(%.0f x %.0f) grow_v=%d" % [
			rect.position.x, rect.position.y, rect.size.x, rect.size.y,
			viewport.x, viewport.y, panel.grow_vertical])
		print("TUTORIAL_HINT_EXPECTED height=54 (offset_top=10, offset_bottom=64)")
		var label := TutorialHints.get("_label") as Control
		print("TUTORIAL_HINT_MINSIZE panel_combined=%s label_combined=%s label_custom=%s label_size=%s" % [
			panel.get_combined_minimum_size(), label.get_combined_minimum_size(),
			label.custom_minimum_size, label.size])
		var root := TutorialHints.get("_root") as Control
		print("TUTORIAL_HINT_ROOT size=%s offsets=(%.0f, %.0f)" % [
			root.size, panel.offset_top, panel.offset_bottom])
	get_tree().quit(0)
