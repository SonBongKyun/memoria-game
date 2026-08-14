extends Node

## S249: 전투 무대와 필드 맵을 같은 잣대로 나란히 잰다.
##
## S236이 맵 열 곳에 대기 예산을 주어 밝기를 0.159~0.368로 올렸다. S241은 전투
## 화면에서 빈 무대를 밝기 0.102 / 구조량 0.0261로 쟀는데, 그건 적의 대비를
## 재기 위한 **바닥**으로만 썼다. 그 바닥 자체가 낮은지는 물은 적이 없다.
##
## 전투 무대는 대기 예산을 받은 적이 없다. 정말 맵보다 어둡고 평평한지, 아니면
## 내 인상인지 같은 방식으로 재서 가른다.
##
## 잣대: 화면에서 UI가 없는 대역만 표본한다. 전투는 HUD가 위아래를 채우므로
## 무대가 보이는 가로 띠(y 0.25~0.58)만 쓰고, 맵도 같은 띠를 쓴다. 그래야
## "화면 전체 평균"이 UI 밝기에 오염되지 않는다.

const BAND_TOP := 0.25
const BAND_BOTTOM := 0.58

const BATTLES: Array[Dictionary] = [
	{"preset": "ash_crawler", "chapter": 1, "scene": "res://scenes/maps/rim_forest.tscn"},
	{"preset": "forest_shade", "chapter": 2, "scene": "res://scenes/maps/verdan_market.tscn"},
	{"preset": "void_beast", "chapter": 4, "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"preset": "shade_sentinel", "chapter": 6, "scene": "res://scenes/maps/the_seam.tscn"},
	{"preset": "kairos", "chapter": 10, "scene": "res://scenes/maps/bl07_void.tscn"},
]

const MAPS: Array[String] = [
	"rim_forest", "verdan_market", "belt_waystation", "drift_shelter", "crumbling_coast",
	"the_seam", "seam_outskirts", "forgotten_forest", "colorless_waste", "bl07_void",
]

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	await get_tree().process_frame

	var field_luma := 0.0
	var field_rms := 0.0
	for map_id in MAPS:
		var stats := await _measure_map(map_id)
		field_luma += float(stats["luma"])
		field_rms += float(stats["rms"])
		print("STAGE_CMP field   %-16s luma=%.4f rms=%.4f" % [map_id, stats["luma"], stats["rms"]])
	field_luma /= float(MAPS.size())
	field_rms /= float(MAPS.size())

	var battle_luma := 0.0
	var battle_rms := 0.0
	for encounter in BATTLES:
		var stats := await _measure_battle(encounter)
		battle_luma += float(stats["luma"])
		battle_rms += float(stats["rms"])
		print("STAGE_CMP battle  %-16s luma=%.4f rms=%.4f" % [encounter["preset"], stats["luma"], stats["rms"]])
	battle_luma /= float(BATTLES.size())
	battle_rms /= float(BATTLES.size())

	print("STAGE_CMP_SUMMARY field luma=%.4f rms=%.4f | battle luma=%.4f rms=%.4f | ratio luma=%.2f rms=%.2f" % [
		field_luma, field_rms, battle_luma, battle_rms,
		battle_luma / maxf(field_luma, 0.0001), battle_rms / maxf(field_rms, 0.0001)])
	print("STAGE_VS_FIELD_DONE")
	get_tree().quit(0)

func _measure_map(map_id: String) -> Dictionary:
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	var map: Node = load("res://scenes/maps/%s.tscn" % map_id).instantiate()
	add_child(map)
	await get_tree().create_timer(1.5).timeout
	await RenderingServer.frame_post_draw
	var stats := _band_stats(get_viewport().get_texture().get_image())
	map.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return stats

func _measure_battle(encounter: Dictionary) -> Dictionary:
	GameManager.current_chapter = int(encounter["chapter"])
	GameManager.change_state(GameManager.GameState.BATTLE)
	GameManager.player_data.elia_with_party = true
	BattleManager.start_battle(String(encounter["preset"]), String(encounter["scene"]))
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().create_timer(1.8).timeout
	await RenderingServer.frame_post_draw
	var stats := _band_stats(get_viewport().get_texture().get_image())
	battle.queue_free()
	await get_tree().process_frame
	await get_tree().process_frame
	return stats

## UI가 없는 가로 띠만 본다. 화면 전체를 평균하면 HUD 밝기가 섞인다.
func _band_stats(image: Image) -> Dictionary:
	var top := int(image.get_height() * BAND_TOP)
	var bottom := int(image.get_height() * BAND_BOTTOM)
	var lumas: Array[float] = []
	var total := 0.0
	for y in range(top, bottom, 2):
		for x in range(0, image.get_width(), 3):
			var c := image.get_pixel(x, y)
			var luma := 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
			lumas.append(luma)
			total += luma
	var mean := total / maxf(float(lumas.size()), 1.0)
	var variance := 0.0
	for luma in lumas:
		variance += (luma - mean) * (luma - mean)
	return {"luma": mean, "rms": sqrt(variance / maxf(float(lumas.size()), 1.0))}
