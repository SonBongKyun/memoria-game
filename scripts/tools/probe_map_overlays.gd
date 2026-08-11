extends Node

## S236 개발용. 맵이 실제로 세운 전면 오버레이를 전부 나열한다.
## "왜 어두워졌는가"를 코드를 읽어 추측하는 대신 런타임 상태로 비교하기 위한 도구.

const MAP_ID := "verdan_market"

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	var map: Node = load("res://scenes/maps/%s.tscn" % MAP_ID).instantiate()
	add_child(map)
	await get_tree().create_timer(1.2).timeout

	var rows: Array[String] = []
	_walk(map, rows)
	rows.sort()
	print("OVERLAY_INVENTORY %s count=%d" % [MAP_ID, rows.size()])
	for row in rows:
		print("OVL " + row)
	print("OVERLAY_INVENTORY_DONE")
	get_tree().quit(0)

func _walk(node: Node, rows: Array[String]) -> void:
	for child in node.get_children():
		if child is CanvasModulate:
			var cm := child as CanvasModulate
			rows.append("CanvasModulate            color=%s" % _c(cm.color))
		elif child is ColorRect:
			var cr := child as ColorRect
			# 전면을 덮는 것만 센다. 작은 장식은 제외.
			if cr.size.x >= 600.0 or cr.anchor_right == 1.0:
				var mat := "shader" if cr.material is ShaderMaterial else "flat"
				rows.append("ColorRect  %-22s color=%s mod=%s %s" % [
					String(cr.name).substr(0, 22), _c(cr.color), _c(cr.modulate), mat
				])
		elif child is TextureRect:
			var tr := child as TextureRect
			if tr.anchor_right == 1.0 or tr.size.x >= 600.0:
				rows.append("TextureRect %-21s mod=%s" % [String(tr.name).substr(0, 21), _c(tr.modulate)])
		_walk(child, rows)

func _c(color: Color) -> String:
	return "(%.2f,%.2f,%.2f,%.3f)" % [color.r, color.g, color.b, color.a]
