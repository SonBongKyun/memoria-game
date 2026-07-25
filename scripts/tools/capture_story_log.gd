## S209: 회상 기록 오버레이 실측 렌더 캡처.
extends Node

const OUTPUT_PATH := "res://tmp/visual_audit/story_log_ko.png"

func _ready() -> void:
	ExplorationHUD.visible = false
	PauseMenu.visible = false
	GameManager.current_locale = "ko"
	StoryLog.suppress_persistence = true

	var background := TextureRect.new()
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.texture = load("res://assets/cg/game_image/env_bureau_spires.png")
	background.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	background.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	background.modulate = Color(0.42, 0.44, 0.50)
	add_child(background)

	StoryLog.clear()
	GameManager.current_chapter = 1
	StoryLog.record("", "재가 내린다. 숲은 그 아래에서 조용히 이름을 잊는다.", "field")
	StoryLog.record("Elia", "아직 네 이름은 남아 있어. 확인해 봐.", "field")
	StoryLog.record("Arrel", "…아렐. 아직은.", "field")
	StoryLog.record_choice("이름을 지킨다")
	GameManager.current_chapter = 2
	StoryLog.record("Malet", "기억은 사라지지 않아. 다만 누가 그 값을 치르느냐가 달라질 뿐이지.", "field")
	StoryLog.record("", "말렛은 장부를 덮었다. 값은 이미 매겨져 있었다.", "vn")
	StoryLog.record("Tobias", "누군가는 기록해야 해. 기록이 없으면 손실은 사고가 되어 버리니까.", "vn")

	StoryLog.open_log()
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().process_frame

	var panel := StoryLog.get_node_or_null("StoryLogOverlay/StoryLogPanel")
	assert(panel != null, "회상 기록 패널이 있어야 한다")

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp/visual_audit"))
	var image := get_viewport().get_texture().get_image()
	var result := image.save_png(ProjectSettings.globalize_path(OUTPUT_PATH))
	assert(result == OK)
	print("STORY_LOG_CAPTURE_PASS path=%s entries=%d" % [OUTPUT_PATH, StoryLog.entries.size()])
	get_tree().quit(0)
