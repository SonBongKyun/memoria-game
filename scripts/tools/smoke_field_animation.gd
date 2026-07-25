## S210 회귀 테스트
## 걷기 애니메이션이 실제로 움직이는지, 배경 시민이 주역과 같은 키 계약을 쓰는지,
## 필드에 각진 플레이스홀더 사각형이 남아 있지 않은지, 그리고 글자 선명도를 좌우하는
## 스트레치 모드가 유지되는지 검증한다.
extends Node

func _ready() -> void:
	var previous_locale := GameManager.current_locale
	GameManager.current_locale = "ko"

	var distinct := _check_walk_cycle()
	_check_walk_anchor()
	await _check_live_playback()
	_check_ambient_scale()
	await _check_no_placeholder_rects()
	_check_text_scaling_contract()

	GameManager.current_locale = previous_locale
	print("FIELD_ANIMATION_SMOKE_PASS walk_frames=%d distinct=%d cast_height=%.0f stretch=canvas_items" % [
		PixelSprite.WALK_FRAME_COUNT, distinct, PixelSprite.FIELD_ADULT_HEIGHT,
	])
	get_tree().quit(0)

# ===================== 걷기 사이클 =====================

## 예전에는 walk 애니메이션이 "같은 정지 그림 두 장"이었다. 프레임 수만 세면
## 그 회귀를 다시 놓치므로, 프레임이 실제로 서로 다른 픽셀인지까지 확인한다.
func _check_walk_cycle() -> int:
	var distinct_pairs := 0
	for who: String in ["arrel", "elia", "malet"]:
		assert(PixelSprite.has_field_sprite_frames(who), "%s의 필드 원화가 있어야 한다" % who)
		var frames := PixelSprite.create_sheet_frames(who)
		for direction: String in ["down", "up", "left", "right"]:
			var anim: String = "walk_" + direction
			assert(frames.has_animation(anim), "%s는 %s 애니메이션이 있어야 한다" % [who, anim])
			var count := frames.get_frame_count(anim)
			assert(count >= 2, "%s/%s는 최소 2프레임이어야 한다 (현재 %d)" % [who, anim, count])

			var first := frames.get_frame_texture(anim, 0).get_image()
			var moved := false
			for i in range(1, count):
				var other := frames.get_frame_texture(anim, i).get_image()
				if not first.get_data() == other.get_data():
					moved = true
					distinct_pairs += 1
			assert(moved, "%s/%s의 프레임이 전부 동일하다. 다리가 움직이지 않는 미끄러짐 회귀." % [who, anim])

			# 걷기 프레임은 idle과도 달라야 한다.
			var idle := frames.get_frame_texture("idle_" + direction, 0).get_image()
			var any_differs_from_idle := false
			for i in range(count):
				if not idle.get_data() == frames.get_frame_texture(anim, i).get_image().get_data():
					any_differs_from_idle = true
			assert(any_differs_from_idle, "%s/%s가 정지 포즈와 완전히 같다" % [who, anim])
	return distinct_pairs

## 데이터로 프레임이 존재하는 것과 실제로 걸을 때 화면이 바뀌는 것은 다른 문제다.
## 플레이어를 실제 입력으로 걷게 한 뒤, 표시 프레임이 진행하는지 확인한다.
## (스프라이트를 직접 play()로 조작하면 플레이어의 애니메이션 상태 기계가 다음
##  프레임에 idle로 덮어써 버리므로, 반드시 진짜 이동 입력으로 검증해야 한다.)
func _check_live_playback() -> void:
	var previous_state := GameManager.current_state
	GameManager.change_state(GameManager.GameState.EXPLORATION)

	var player: Node2D = load("res://scenes/player/player.tscn").instantiate()
	add_child(player)
	await get_tree().process_frame

	var sprite: AnimatedSprite2D = null
	for child in player.get_children():
		if child is AnimatedSprite2D:
			sprite = child
			break
	assert(sprite != null and sprite.sprite_frames != null, "플레이어에 필드 스프라이트가 있어야 한다")

	Input.action_press("move_right")
	for _f in range(4):
		await get_tree().physics_frame
	assert(sprite.animation.begins_with("walk_"),
		"이동 중에는 걷기 애니메이션이어야 한다 (현재 %s)" % sprite.animation)
	assert(sprite.is_playing(), "걷기 애니메이션이 재생 상태여야 한다")
	assert(sprite.sprite_frames.get_frame_count(sprite.animation) > 1,
		"걷기 애니메이션이 한 장짜리면 다리가 절대 움직이지 않는다")

	var start_frame := sprite.frame
	var advanced := false
	# 9fps이므로 한 프레임 전환에 약 0.11초. 넉넉히 40프레임을 흘린다.
	for _f in range(40):
		await get_tree().process_frame
		if sprite.frame != start_frame:
			advanced = true
			break
	Input.action_release("move_right")
	assert(advanced, "걷는 동안에도 표시 프레임이 그대로다. 정지 포즈로 미끄러지는 회귀.")

	player.queue_free()
	await get_tree().process_frame
	GameManager.change_state(previous_state)

## 걸음 프레임이 원화의 실루엣 기준(발 높이, 가로 중심)에서 크게 벗어나면
## 캐릭터가 지면에서 떨어지거나 좌우로 튄다.
func _check_walk_anchor() -> void:
	var frames := PixelSprite.create_sheet_frames("arrel")
	var idle_rect := frames.get_frame_texture("idle_down", 0).get_image().get_used_rect()
	for i in range(frames.get_frame_count("walk_down")):
		var rect := frames.get_frame_texture("walk_down", i).get_image().get_used_rect()
		var foot_drift: int = absi((rect.position.y + rect.size.y) - (idle_rect.position.y + idle_rect.size.y))
		var center_drift: int = absi(
			(rect.position.x + rect.size.x / 2) - (idle_rect.position.x + idle_rect.size.x / 2)
		)
		assert(foot_drift <= 6, "걸음 프레임 %d의 발 높이가 %dpx나 벗어났다" % [i, foot_drift])
		assert(center_drift <= 6, "걸음 프레임 %d의 좌우 중심이 %dpx나 벗어났다" % [i, center_drift])

# ===================== 배경 시민 크기 =====================

## 주역은 apply_field_profile로 50px(아이 42px)에 맞춰지는데, 배경 시민만
## scale 0.24가 손으로 박혀 있어 약 35px로 렌더링되었다. 같은 시장 안에서
## 배경 인물이 일제히 30% 작은 사람들처럼 보이던 원인.
func _check_ambient_scale() -> void:
	for preset: String in ["villager_f", "villager_m", "fisherman", "elder", "child", "merchant", "traveler"]:
		var sprite := PixelSprite.create_npc_sprite(preset)
		add_child(sprite)
		var texture := sprite.sprite_frames.get_frame_texture("idle_down", 0)
		var used := texture.get_image().get_used_rect()
		var visible_height: float = float(used.size.y) * sprite.scale.y
		var expected: float = PixelSprite.FIELD_CHILD_HEIGHT if preset == "child" else PixelSprite.FIELD_ADULT_HEIGHT
		assert(absf(visible_height - expected) < 1.0,
			"%s 배경 시민의 표시 높이가 %.1fpx다. 주역과 같은 %.0fpx 계약을 써야 한다." % [preset, visible_height, expected])
		sprite.queue_free()

# ===================== 플레이스홀더 사각형 =====================

## 손으로 그린 맵 캔버스 위에 반투명 색 사각형이 얹히면, 배경과 전혀 다른
## 레이어처럼 보인다. 공명 지점이 마지막으로 남아 있던 사례였다.
func _check_no_placeholder_rects() -> void:
	var map: Node2D = load("res://scenes/maps/verdan_market.tscn").instantiate()
	add_child(map)
	await get_tree().process_frame
	await get_tree().process_frame

	var resonance_points := 0
	for node in get_tree().get_nodes_in_group("memory_resonance"):
		resonance_points += 1
		var glow_sprites := 0
		for child in node.get_children():
			assert(not (child is ColorRect),
				"공명 지점 %s에 각진 ColorRect 플레이스홀더가 남아 있다" % node.name)
			if child is Sprite2D and (child as Sprite2D).texture != null:
				glow_sprites += 1
		assert(glow_sprites >= 2, "공명 지점 %s는 광원과 중심점을 가져야 한다" % node.name)
	assert(resonance_points >= 1, "베르단 시장에는 공명 지점이 있어야 한다")

	# 필드 상호작용 오브젝트도 스프라이트여야 한다 (S209 회귀 방지).
	var prop_sprites := 0
	for child in map.get_children():
		if child is Area2D:
			for sub in child.get_children():
				if sub is Sprite2D and (sub as Sprite2D).has_meta("prop_type"):
					prop_sprites += 1
	assert(prop_sprites >= 3, "시장의 통/상자/모닥불이 스프라이트로 그려져야 한다 (현재 %d)" % prop_sprites)

	map.queue_free()
	await get_tree().process_frame

# ===================== 텍스트 선명도 계약 =====================

## stretch mode가 "viewport"로 되돌아가면 게임 전체가 1280x720으로 렌더링된 뒤
## 창 크기로 확대되어, 전체화면에서 모든 글자가 뭉개진다.
func _check_text_scaling_contract() -> void:
	assert(get_window().content_scale_mode == Window.CONTENT_SCALE_MODE_CANVAS_ITEMS,
		"canvas_items 스트레치가 유지되어야 전체화면에서 글자가 선명하다")
	assert(get_window().content_scale_aspect == Window.CONTENT_SCALE_ASPECT_KEEP,
		"aspect=keep이어야 전투 무대 같은 절대 좌표 레이아웃의 구도가 유지된다")
	assert(get_viewport().get_visible_rect().size.is_equal_approx(Vector2(1280, 720)),
		"논리 좌표계는 1280x720으로 고정되어야 한다")
