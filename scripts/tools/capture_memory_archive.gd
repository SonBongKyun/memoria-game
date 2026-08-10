extends Node

## S234: 아렐의 서고 실물 렌더 캡처.
## 이번 네 가지 변경(파수 트랙, 기억 고정, 연쇄, 한국어 텍스트)이 모두 착지하는 화면이다.

const OUTPUT_PATH := "res://tmp/visual_audit/memory_archive_ko.png"

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "ko"
	GameManager.current_chapter = 6
	GameManager.player_data["grains"] = 240

	# 중간 지점의 플레이어 상태를 만든다: 몇 번 태웠고, 앵커는 지켰고, 하나는 고정해 뒀다.
	MemoryManager.memories.clear()
	MemoryManager.burned_memories.clear()
	MemoryManager.burn_passives.clear()
	MemoryManager.anchor_passives.clear()
	MemoryManager.erosion_guarded.clear()
	MemoryManager.extracted_memories.clear()
	MemoryManager.active_loan = {}
	MemoryManager.anchor_vigil = 0
	MemoryManager._guard_slots_used = 0
	MemoryManager._vigil_chapters_counted.clear()
	MemoryManager._init_starting_memories()
	for chapter in range(2, 7):
		MemoryManager.add_chapter_memories(chapter)

	MemoryManager.burn_memory_silent("sense_forest_smell", true)
	MemoryManager.burn_memory_silent("sense_warm_light", true)
	MemoryManager.guard_memory("daily_campfire_song")

	# 관리국 감지 팝업은 연소 직후 잠깐 뜨는 연출이다. 서고 자체를 찍기 위해 비운다.
	SystemLog.queue.clear()
	SystemLog.log_panel.visible = false
	await get_tree().process_frame
	MemoryUI.open_archive()
	await get_tree().process_frame
	var focus = MemoryManager.find_memory("identity_first_sword")
	MemoryUI.call("_show_detail", focus)
	await get_tree().process_frame
	await get_tree().process_frame

	assert(MemoryUI.is_open, "서고가 열려 있어야 한다")
	assert(MemoryUI.detail_title.text == MemoryManager.localized_memory_title(focus), "상세 제목이 표시 텍스트를 따라야 한다")
	assert(MemoryUI.track_summary_label != null and MemoryUI.track_summary_label.text != "", "두 진행축 요약이 보여야 한다")
	assert(MemoryUI.guard_btn != null and MemoryUI.guard_btn.visible, "고정 버튼이 보여야 한다")

	await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	assert(image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH)) == OK, "서고 캡처를 저장하지 못했다")
	print("MEMORY_ARCHIVE_CAPTURE_PASS path=%s vigil=%d anchor_passives=%d guards_left=%d tracks=%s" % [
		OUTPUT_PATH, MemoryManager.anchor_vigil,
		MemoryManager.get_active_anchor_passives().size(),
		MemoryManager.guard_slots_remaining(),
		MemoryUI.track_summary_label.text,
	])
	get_tree().quit(0)
