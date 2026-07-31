## S217: 상점 시세/장비 비교, 도감, 저널 미해결을 한 장으로 확인한다.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/rpg_depth.png"

func _ready() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	OptionsMenu.settings["reduce_motion"] = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 4
	GameManager.player_data["grains"] = 60
	GameManager.equipped["weapon"] = "iron_sword"
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var shots: Array[Image] = []

	# 1) 상점: 장비 비교 + 시세
	MemoryShop.open_shop("Malet", [])
	for _f in range(6):
		await get_tree().process_frame
	MemoryShop.call("_refresh_items")
	await get_tree().process_frame
	MemoryShop.call("_select_item", {"type": "buy_equip", "equip_id": "void_edge", "price": 80})
	# 첫 상점 튜토리얼 힌트가 4초 뒤 스스로 사라진다. 감사 이미지가 가려지지 않게 기다린다.
	for _f in range(300):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())
	MemoryShop.close_shop()
	for _f in range(4):
		await get_tree().process_frame

	# 2) 도감: 기록/미조우
	Codex.open()
	for _f in range(6):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())
	Codex.close()
	for _f in range(4):
		await get_tree().process_frame

	# 3) 저널: 미해결 단서
	StoryJournal.open_journal()
	for _f in range(6):
		await get_tree().process_frame
	StoryJournal.set("_current_tab", "leads")
	StoryJournal.call("_refresh_list")
	for _f in range(4):
		await get_tree().process_frame
	shots.append(get_viewport().get_texture().get_image())
	StoryJournal.close_journal()

	var crop := Rect2i(0, 90, 1280, 480)
	var sheet := Image.create(crop.size.x, crop.size.y * shots.size(), false, Image.FORMAT_RGBA8)
	for i in range(shots.size()):
		sheet.blit_rect(shots[i], crop, Vector2i(0, crop.size.y * i))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	sheet.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	print("RPG_DEPTH_CAPTURE_PASS path=%s shots=%d" % [OUTPUT_PATH, shots.size()])
	get_tree().quit(0)
