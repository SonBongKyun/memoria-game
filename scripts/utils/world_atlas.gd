## WorldAtlas places chapter-gated optional routes on the main journey maps.
## The sites always return to their origin, so the ten-chapter story chain
## remains linear while each completed chapter earns a short exploration pocket.
class_name WorldAtlas
extends RefCounted

const GATES: Dictionary = {
	"belt_waystation": [{"id": "belt_signal_yard", "tile": [4, 10], "requires": "ch3_complete", "scene": "res://scenes/maps/belt_signal_yard.tscn", "en": "Trace the signal yard", "ko": "신호 야적장 추적", "accent": Color(0.90, 0.48, 0.26)}],
	"drift_shelter": [{"id": "drift_waymarker_shrine", "tile": [19, 11], "requires": "ch4_complete", "scene": "res://scenes/maps/drift_waymarker_shrine.tscn", "en": "Read the waymarker shrine", "ko": "이정표 성소 읽기", "accent": Color(0.55, 0.68, 0.94)}],
	"crumbling_coast": [{"id": "coast_cinder_harbor", "tile": [20, 13], "requires": "ch5_complete", "scene": "res://scenes/maps/coast_cinder_harbor.tscn", "en": "Enter Cinder Harbor", "ko": "신더 항구로 들어가기", "accent": Color(0.88, 0.62, 0.34)}],
	"the_seam": [{"id": "seam_lantern_ward", "tile": [3, 14], "requires": "ch6_complete", "scene": "res://scenes/maps/seam_lantern_ward.tscn", "en": "Visit Lantern Ward", "ko": "랜턴 구역 방문", "accent": Color(0.94, 0.76, 0.38)}],
	"forgotten_forest": [{"id": "forest_name_hollow", "tile": [18, 11], "requires": "ch8_complete", "scene": "res://scenes/maps/forest_name_hollow.tscn", "en": "Follow the name hollow", "ko": "이름의 움푹한 길 따르기", "accent": Color(0.54, 0.82, 0.46)}],
	"colorless_waste": [{"id": "waste_grey_caravan", "tile": [20, 15], "requires": "ch9_complete", "scene": "res://scenes/maps/waste_grey_caravan.tscn", "en": "Find the grey caravan", "ko": "회색 캐러밴 찾기", "accent": Color(0.72, 0.78, 0.86)}],
	"bl07_void": [{"id": "bl07_seed_vault", "tile": [4, 17], "requires": "ch10_void_entered", "scene": "res://scenes/maps/bl07_seed_vault.tscn", "en": "Enter the seed vault", "ko": "씨앗 금고 진입", "accent": Color(0.76, 0.58, 0.96)}],
}

static func add_gateways(map: Node2D, map_id: String) -> void:
	if map == null or map.has_node("WorldAtlasGateways"):
		return
	var root := Node2D.new()
	root.name = "WorldAtlasGateways"
	root.z_index = 3
	map.add_child(root)
	for gate_data in GATES.get(map_id, []):
		if not GameManager.get_flag(String(gate_data.get("requires", ""))):
			continue
		var gate := MapGateway.new()
		gate.name = "AtlasGateway_%s" % String(gate_data.get("id", "site"))
		var tile: Array = gate_data.get("tile", [1, 1])
		gate.position = Vector2((float(tile[0]) + 0.5) * 32.0, (float(tile[1]) + 0.5) * 32.0)
		gate.destination_scene = String(gate_data.get("scene", ""))
		gate.label_en = String(gate_data.get("en", "Explore"))
		gate.label_ko = String(gate_data.get("ko", "탐색"))
		gate.accent = gate_data.get("accent", Color(0.75, 0.64, 0.35))
		root.add_child(gate)
