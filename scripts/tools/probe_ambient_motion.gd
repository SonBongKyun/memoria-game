extends Node2D

## S235 개발용 계측기.
## "캐릭터가 뻣뻣하다"를 눈이 아니라 숫자로 확인한다.
##   1. 배회 NPC: 몸의 실제 속도와 다리 회전 속도가 맞는가 (발 미끄러짐)
##   2. 월드 인구 NPC: 애초에 움직이기는 하는가

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	await get_tree().process_frame

	await _probe_wander_footslip()
	await _probe_world_population()

	print("AMBIENT_MOTION_PROBE_DONE")
	get_tree().quit(0)

func _probe_wander_footslip() -> void:
	var walker := PixelSprite.create_npc_sprite("villager_f")
	walker.position = Vector2(200, 200)
	add_child(walker)
	await get_tree().process_frame
	MapEffects.add_npc_wander(walker, 60.0)
	await get_tree().process_frame

	print("--- wander step: body speed vs leg cadence ---")
	# 한 번의 연속된 걸음만 잰다. 여러 번의 이동을 섞으면 이동 사이의 위치 점프가
	# 속도로 잡혀 측정이 무의미해진다. 헤드리스에서는 delta가 들쭉날쭉하므로
	# 이 계측기는 실제 렌더러(60fps)로 돌려야 한다.
	while not String(walker.animation).begins_with("walk_"):
		await get_tree().process_frame
	await get_tree().process_frame  # 시작 프레임의 셋업 점프는 버린다
	var previous := walker.position
	var samples: Array = []
	for _frame in range(240):
		await get_tree().process_frame
		if not String(walker.animation).begins_with("walk_"):
			break
		var delta := get_process_delta_time()
		var speed := walker.position.distance_to(previous) / maxf(delta, 0.0001)
		previous = walker.position
		samples.append({"speed": speed, "scale": walker.speed_scale})
	if samples.is_empty():
		print("PROBE wander: no walking frames sampled")
		walker.set_meta("wander_active", false)
		return

	# 발 미끄러짐은 프레임마다 재야 한다. 다리는 그 순간 speed_scale * 기준속도로
	# "지면을 밀고 있다"고 주장하고, 몸은 그 순간 실제로 간 거리만큼만 간다.
	# 두 값의 차이가 그 프레임에 발이 긁은 양이다.
	var min_speed := 99999.0
	var max_speed := 0.0
	var worst_slip := 0.0
	var total_slip := 0.0
	var cadence: float = float(samples[0]["scale"])
	var cadence_changes := 0
	for sample in samples:
		var speed := float(sample["speed"])
		var claimed := float(sample["scale"]) * MapEffects.WANDER_GAIT_REFERENCE
		min_speed = minf(min_speed, speed)
		max_speed = maxf(max_speed, speed)
		var slip := absf(claimed - speed)
		worst_slip = maxf(worst_slip, slip)
		total_slip += slip
		if not is_equal_approx(float(sample["scale"]), cadence):
			cadence_changes += 1
			cadence = float(sample["scale"])
	print("PROBE wander frames=%d body_speed=[%.1f .. %.1f] px/s cadence_changes=%d" % [
		samples.size(), min_speed, max_speed, cadence_changes
	])
	print("PROBE wander footslip worst=%.1f px/s  mean=%.1f px/s" % [
		worst_slip, total_slip / float(samples.size())
	])
	walker.set_meta("wander_active", false)

func _probe_world_population() -> void:
	var root := Node2D.new()
	add_child(root)
	WorldPopulation.populate(root, "rim_forest")
	await get_tree().process_frame
	await get_tree().process_frame

	var voices: Array[Node2D] = []
	for child in root.get_children():
		for grandchild in child.get_children():
			if String(grandchild.name).begins_with("WorldVoice_"):
				voices.append(grandchild as Node2D)
	if voices.is_empty():
		for child in root.get_children():
			if String(child.name).begins_with("WorldVoice_"):
				voices.append(child as Node2D)
	print("--- world population: does anyone move? ---")
	print("PROBE population voices=%d" % voices.size())
	if voices.is_empty():
		return

	var start: Array[Vector2] = []
	for voice in voices:
		start.append(voice.global_position)
	var offset_min: Array[Vector2] = []
	var offset_max: Array[Vector2] = []
	for voice in voices:
		var s := voice.get_node_or_null("CharacterSprite") as AnimatedSprite2D
		offset_min.append(s.offset if s != null else Vector2.ZERO)
		offset_max.append(s.offset if s != null else Vector2.ZERO)
	for _frame in range(180):
		await get_tree().process_frame
		for i in range(voices.size()):
			var s := voices[i].get_node_or_null("CharacterSprite") as AnimatedSprite2D
			if s == null:
				continue
			offset_min[i] = Vector2(minf(offset_min[i].x, s.offset.x), minf(offset_min[i].y, s.offset.y))
			offset_max[i] = Vector2(maxf(offset_max[i].x, s.offset.x), maxf(offset_max[i].y, s.offset.y))
	for i in range(voices.size()):
		var s := voices[i].get_node_or_null("CharacterSprite") as AnimatedSprite2D
		if s != null:
			s.set_meta("probe_offset_span", (offset_max[i] - offset_min[i]).length())
	var moved := 0
	var max_drift := 0.0
	var has_driver := 0
	var breathing := 0
	for i in range(voices.size()):
		var drift := voices[i].global_position.distance_to(start[i])
		max_drift = maxf(max_drift, drift)
		if drift > 1.0:
			moved += 1
		var sprite := voices[i].get_node_or_null("CharacterSprite")
		if sprite != null and sprite.get_node_or_null("FieldMotionDriver") != null:
			has_driver += 1
		if sprite != null and sprite.has_meta("probe_offset_span") and float(sprite.get_meta("probe_offset_span")) > 0.02:
			breathing += 1
	print("PROBE population breathing=%d/%d" % [breathing, voices.size()])
	# 숫자가 0이 아닌 것과 화면에서 보이는 것은 다르다.
	# 실제 렌더 픽셀 = offset 진폭 * 스프라이트 스케일 * 카메라 줌(2.25)
	for i in range(voices.size()):
		var s := voices[i].get_node_or_null("CharacterSprite") as AnimatedSprite2D
		if s == null:
			continue
		var span := float(s.get_meta("probe_offset_span", 0.0))
		print("PROBE breath[%d] local=%.4f scale=%.2f screen_px=%.3f" % [
			i, span, s.scale.y, span * s.scale.y * 2.25
		])
	print("PROBE population after 3s: moved=%d/%d  max_drift=%.2f px  motion_drivers=%d/%d" % [
		moved, voices.size(), max_drift, has_driver, voices.size()
	])
	var animations: Dictionary = {}
	for voice in voices:
		var sprite := voice.get_node_or_null("CharacterSprite") as AnimatedSprite2D
		if sprite != null:
			animations[String(sprite.animation)] = int(animations.get(String(sprite.animation), 0)) + 1
	print("PROBE population animations=%s" % str(animations))
