## S209 회귀 테스트
## 회상 기록, 읽은 대사 기반 빨리감기, 전투 배속, 필드 오브젝트 스프라이트,
## 그리고 전투 무대 기준선 정렬을 실제 런타임에서 검증한다.
extends Node

func _ready() -> void:
	var previous_locale := GameManager.current_locale
	var previous_chapter := GameManager.current_chapter
	var previous_state := GameManager.current_state
	var previous_speed: int = int(OptionsMenu.settings.get("battle_speed", 0))
	var previous_skip: bool = bool(OptionsMenu.settings.get("skip_read_only", true))

	GameManager.current_locale = "ko"
	GameManager.current_chapter = 3
	# 합성 문장이 플레이어의 읽음 등록부에 남지 않게 한다.
	StoryLog.suppress_persistence = true

	_check_story_log()
	_check_fast_forward_gate()
	_check_battle_speed()
	_check_prop_sprites()
	await _check_battle_stage()

	GameManager.current_locale = previous_locale
	GameManager.current_chapter = previous_chapter
	GameManager.change_state(previous_state)
	OptionsMenu.settings["battle_speed"] = previous_speed
	OptionsMenu.settings["skip_read_only"] = previous_skip
	# cycle_battle_speed()가 설정 파일에 썼으므로, 원래 값으로 되돌린 뒤 다시 저장한다.
	OptionsMenu.save_settings()

	print("STORY_QOL_SMOKE_PASS log=%d read=%d speed_steps=%d props=%d" % [
		StoryLog.entries.size(),
		StoryLog.read_line_count(),
		BattleManager.BATTLE_SPEED_STEPS.size(),
		6,
	])
	get_tree().quit(0)

# ===================== 회상 기록 =====================

func _check_story_log() -> void:
	StoryLog.clear()
	StoryLog.record("Elia", "이름을 기억해.", "field")
	StoryLog.record("", "재가 내린다.", "field")
	StoryLog.record("Elia", "이름을 기억해.", "field")
	assert(StoryLog.entries.size() == 3, "필드 대사와 나레이션이 모두 기록되어야 한다")

	# 같은 줄이 연속으로 다시 표시되면(리프레시) 중복 적재하지 않는다.
	StoryLog.record("", "재가 내린다.", "field")
	StoryLog.record("", "재가 내린다.", "field")
	assert(StoryLog.entries.size() == 4, "연속 중복 표시는 한 번만 쌓여야 한다")

	StoryLog.record("Tobias", "기록은 남는다.", "vn")
	assert(String(StoryLog.entries[StoryLog.entries.size() - 1].get("source", "")) == "vn", "VN 대사도 같은 기록으로 모여야 한다")

	StoryLog.record_choice("이름을 지킨다")
	var last: Dictionary = StoryLog.entries[StoryLog.entries.size() - 1]
	assert(String(last.get("speaker", "")) == "__choice__", "플레이어 선택도 기록에 남아야 한다")

	# 상한을 넘겨도 최근 대사가 유지된다.
	for i in range(StoryLog.MAX_ENTRIES + 20):
		StoryLog.record("Arrel", "대사 %d" % i, "field")
	assert(StoryLog.entries.size() == StoryLog.MAX_ENTRIES, "기록은 상한을 넘지 않아야 한다")
	assert(String(StoryLog.entries[StoryLog.entries.size() - 1].get("text", "")).ends_with(str(StoryLog.MAX_ENTRIES + 19)), "가장 최근 대사가 남아야 한다")

	# 오버레이가 실제로 구성되는지 (목록 + 장 구분 헤더).
	var was_paused := get_tree().paused
	StoryLog.open_log()
	var panel := StoryLog.get_node_or_null("StoryLogOverlay/StoryLogPanel")
	assert(panel != null, "회상 기록 패널이 생성되어야 한다")
	var backdrop := StoryLog.get_node_or_null("StoryLogOverlay/StoryLogBackdrop") as TextureRect
	assert(backdrop != null and backdrop.texture != null, "회상 기록은 전용 아카이브 배경을 사용해야 한다")
	assert(backdrop.texture.resource_path == StoryLog.BACKDROP_PATH, "회상 기록 배경 경로가 선택된 에셋과 일치해야 한다")
	assert(StoryLog.is_open, "회상 기록이 열려야 한다")
	assert(get_tree().paused, "필드에서 회상 기록을 열면 뒤의 게임 진행이 멈춰야 한다")
	StoryLog.close_log()
	assert(not StoryLog.is_open, "회상 기록이 닫혀야 한다")
	assert(get_tree().paused == was_paused, "회상 기록을 닫으면 기존 일시정지 상태가 복원되어야 한다")

# ===================== 빨리감기 범위 =====================

func _check_fast_forward_gate() -> void:
	OptionsMenu.settings["skip_read_only"] = true
	var fresh := "이 문장은 이번 실행에서 처음 등장한다 %d" % Time.get_ticks_usec()
	assert(not StoryLog.is_read("Arrel", fresh), "새 문장은 아직 읽지 않은 상태여야 한다")

	StoryLog.record("Arrel", fresh, "field")
	assert(StoryLog.last_line_was_new, "처음 보는 문장은 새 대사로 표시되어야 한다")
	assert(not StoryLog.can_fast_forward(), "읽은 대사만 빨리감기면 새 문장 앞에서 멈춰야 한다")
	assert(StoryLog.is_read("Arrel", fresh), "표시된 문장은 읽음으로 등록되어야 한다")

	# 두 번째 표시(= 재플레이)에서는 통과한다.
	StoryLog.record("Arrel", "사이에 낀 다른 줄", "field")
	StoryLog.record("Arrel", fresh, "field")
	assert(not StoryLog.last_line_was_new, "이미 읽은 문장은 새 대사가 아니어야 한다")
	assert(StoryLog.can_fast_forward(), "이미 읽은 문장은 빨리감기로 넘어가야 한다")

	# 옵션을 끄면 새 문장도 넘긴다.
	OptionsMenu.settings["skip_read_only"] = false
	StoryLog.record("Arrel", "또 다른 새 문장 %d" % Time.get_ticks_usec(), "field")
	assert(StoryLog.can_fast_forward(), "전체 빨리감기 설정에서는 새 문장도 넘어가야 한다")
	OptionsMenu.settings["skip_read_only"] = true

# ===================== 전투 배속 =====================

func _check_battle_speed() -> void:
	OptionsMenu.settings["battle_speed"] = 0
	assert(is_equal_approx(BattleManager.get_battle_speed(), 1.0), "기본 배속은 x1.0")
	assert(is_equal_approx(BattleManager.paced(1.0), 1.0), "x1.0에서는 대기 시간이 그대로")

	OptionsMenu.settings["battle_speed"] = 2
	assert(is_equal_approx(BattleManager.get_battle_speed(), 2.0), "x2.0 배속이 적용되어야 한다")
	assert(is_equal_approx(BattleManager.paced(1.0), 0.5), "x2.0에서는 연출 대기가 절반")
	assert(BattleManager.get_battle_speed_label() == "x2.0", "배속 표기가 일치해야 한다")

	var timer := BattleManager.pace_timer(1.0)
	assert(timer != null and timer.time_left <= 0.51, "pace_timer도 배속을 반영해야 한다")

	# 순환은 처음으로 되돌아온다.
	OptionsMenu.settings["battle_speed"] = 2
	var cycled := BattleManager.cycle_battle_speed()
	assert(is_equal_approx(cycled, 1.0), "마지막 단계 다음은 x1.0으로 돌아와야 한다")

# ===================== 필드 오브젝트 스프라이트 =====================

func _check_prop_sprites() -> void:
	for prop_type: String in ["barrel", "crate", "campfire", "sign", "chest", "clue"]:
		var tex := PixelSprite.create_prop_texture(prop_type, false)
		assert(tex != null, "%s 스프라이트가 생성되어야 한다" % prop_type)
		assert(tex.get_width() == PixelSprite.PROP_SIZE and tex.get_height() == PixelSprite.PROP_SIZE, "%s 스프라이트는 32x32여야 한다" % prop_type)
		var image := tex.get_image()
		var opaque := 0
		for y in range(image.get_height()):
			for x in range(image.get_width()):
				if image.get_pixel(x, y).a > 0.5:
					opaque += 1
		assert(opaque > 60, "%s 스프라이트에 실제 그림이 있어야 한다 (불투명 픽셀 %d)" % [prop_type, opaque])
		# 같은 요청은 캐시를 재사용한다 (맵마다 다시 그리지 않는다).
		assert(PixelSprite.create_prop_texture(prop_type, false) == tex, "%s 스프라이트는 캐시되어야 한다" % prop_type)

	# 사용한 상태는 다른 그림이어야 한다.
	assert(PixelSprite.create_prop_texture("chest", true) != PixelSprite.create_prop_texture("chest", false), "연 상자는 다른 스프라이트여야 한다")

	for kind: String in ["chest", "clue"]:
		var marker := MapEffects.make_discovery_marker(kind)
		assert(marker != null, "%s 마커가 생성되어야 한다" % kind)
		var sprite: Sprite2D = null
		for child in marker.get_children():
			if child is Sprite2D:
				sprite = child
		assert(sprite != null and sprite.texture != null, "%s 마커는 색깔 사각형이 아니라 스프라이트여야 한다" % kind)
		assert(sprite.modulate.a > 0.5, "Clean Gameplay View에서도 발견물 마커가 보여야 한다")
		marker.free()

	# 실제 맵에 배치되는 경로도 확인한다.
	var map := Node2D.new()
	add_child(map)
	var prop := MapEffects.add_interactive_prop(map, Vector2(64, 64), "barrel")
	var prop_visual: Sprite2D = null
	for child in prop.get_children():
		if child is Sprite2D:
			prop_visual = child
	assert(prop_visual != null, "필드 상호작용 오브젝트는 Sprite2D로 그려져야 한다")
	for child in prop.get_children():
		assert(not (child is ColorRect), "필드 오브젝트에 색깔 사각형 플레이스홀더가 남아 있으면 안 된다")
	map.queue_free()

# ===================== 전투 무대 정렬 =====================

func _check_battle_stage() -> void:
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	GameManager.player_data.elia_with_party = true
	BattleManager.current_enemy = BattleManager.Enemy.new("Ash Crawler", 60, 9, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/rim_forest.tscn"
	BattleManager.enemy_image = ""
	BattleManager.battle_bg_image = ""

	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame

	var baseline: float = battle.STAGE_BASELINE_Y
	var player_container := battle.get("player_sprite_container") as Control
	var enemy_container := battle.get("enemy_sprite_container") as Control
	var ally_container := battle.get("ally_sprite_container") as Control
	var player_shadow := battle.get("player_shadow") as Polygon2D
	var enemy_shadow := battle.get("enemy_shadow") as Polygon2D

	# 전투원 그림자는 모두 공통 기준선 근처에 놓인다 (원근 오프셋 20px 이내).
	var player_feet: float = player_container.position.y + player_shadow.position.y + 200.0
	var enemy_feet: float = enemy_container.position.y + enemy_shadow.position.y + 300.0
	assert(absf(player_feet - baseline) < 2.0, "아렐의 발이 무대 기준선에 있어야 한다 (%.1f vs %.1f)" % [player_feet, baseline])
	assert(absf(enemy_feet - baseline) < 2.0, "적의 발이 무대 기준선에 있어야 한다 (%.1f vs %.1f)" % [enemy_feet, baseline])
	assert(ally_container.position.y + 160.0 <= baseline, "동행자는 기준선보다 앞으로 나오지 않는다")

	# 적 판이 상자 안에서 세로 가운데 정렬로 떠 있지 않아야 한다.
	var enemy_plate := battle.get("enemy_sprite") as Control
	assert(enemy_plate != null, "적 표시가 있어야 한다")
	var plate_bottom: float = enemy_plate.position.y + enemy_plate.size.y
	assert(plate_bottom > 240.0, "적 일러스트 아랫변이 지면 가까이 내려와야 한다 (%.1f)" % plate_bottom)

	# 주인공이 무대에서 가장 큰 전투원이어야 한다.
	var player_sprite := battle.get("player_sprite") as AnimatedSprite2D
	assert(player_sprite.scale.x >= 1.3, "아렐이 무대의 시선 기준이어야 한다 (scale %.2f)" % player_sprite.scale.x)

	# 배속 칩과 리밋 라벨.
	var speed_chip := battle.get("_battle_speed_btn") as Button
	assert(speed_chip != null and "Tab" in speed_chip.text, "전투 화면에 배속 칩이 있어야 한다")

	# Tab이 실제로 배속을 순환하는지. `_input`에서 가로채지 않으면 Godot의 포커스
	# 이동이 이벤트를 먼저 먹어버리기 때문에, 이 경로 자체가 회귀 지점이다.
	OptionsMenu.settings["battle_speed"] = 0
	var tab_event := InputEventKey.new()
	tab_event.keycode = KEY_TAB
	tab_event.pressed = true
	battle.call("_input", tab_event)
	assert(int(OptionsMenu.settings.get("battle_speed", 0)) == 1, "Tab이 전투 배속을 다음 단계로 넘겨야 한다")
	assert("x1.5" in speed_chip.text, "배속 칩 표기가 갱신되어야 한다")
	var limit_label := battle.get("limit_label") as Label
	assert(limit_label != null and limit_label.custom_minimum_size.x >= 40.0, "리밋 라벨이 잘리지 않을 폭을 확보해야 한다")

	# 쓸 기술이 없으면 엘리아 레일은 숨는다.
	battle.call("_refresh_elia_skills")
	var elia_rail := battle.get("elia_skill_container") as Control
	if elia_rail != null and EliaDiary.get_available_skills().is_empty():
		assert(not elia_rail.visible, "사용 가능한 기술이 없으면 엘리아 레일은 숨어야 한다")

	# 비활성 커맨드도 불투명 배경을 유지해야 장식 위에서 읽힌다.
	var limit_btn := battle.get("limit_btn") as Button
	assert(limit_btn != null, "리밋 커맨드가 있어야 한다")
	var disabled_style := limit_btn.get_theme_stylebox("disabled") as StyleBoxFlat
	assert(disabled_style != null and disabled_style.bg_color.a > 0.9, "비활성 커맨드 배경이 불투명해야 한다")

	battle.queue_free()
	await get_tree().process_frame
