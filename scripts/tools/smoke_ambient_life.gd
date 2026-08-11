extends Node2D

## S235: 서 있는 사람과 걷는 사람이 살아 있는가.
##
## 실측으로 시작한 작업이다. 배회 NPC는 다리를 평균 속도 한 값으로 고정한 채
## SINE 가감속으로 움직여서, 걷기 시작과 끝에서 발이 제자리를 긁었다.
## 월드 인구 NPC는 3초 동안 5명 전원이 0.00px 움직였고 호흡조차 없었다.
##
## 여기서 고정하는 계약:
##   1. 배회 중 다리 회전은 그 순간의 실제 이동 속도를 따라간다.
##   2. 멈추면 다리도 멈춘다 (speed_scale이 원상 복구된다).
##   3. 월드 인구는 ambient 드라이버를 달고 실제로 호흡한다.
##   4. 플레이어가 다가오면 몸을 돌리고, 지나가면 원래 방향으로 돌아온다.
##   5. 알아채는 반경과 놓아주는 반경이 달라 경계에서 깜빡이지 않는다.
##   6. 단일 삽화 NPC는 없는 자세를 지어내지 않고 좌우 반전으로 돌아본다.

var _notice_radius: float = 0.0
var _release_radius: float = 0.0
var _slip_measured: bool = false
var _mean_slip: float = 0.0
var _breath_ground_px: float = 0.0
var _threat_drift: float = 0.0
var _rim_split: float = 0.0

func _ready() -> void:
	Codex.suppress_recording = true
	OptionsMenu.settings["reduce_motion"] = false
	var previous_state := GameManager.current_state
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	await get_tree().process_frame

	await _check_wander_gait_follows_body()
	await _check_population_is_alive()
	await _check_presence_hysteresis()
	await _check_static_plate_turns_by_flip()
	await _check_threats_patrol()
	await _check_silhouette_rim()

	GameManager.change_state(previous_state)
	var slip_report := ("%.1f" % _mean_slip) if _slip_measured else "not_measured(headless)"
	print("AMBIENT_LIFE_SMOKE_PASS gait=analytic idle_reset=1 drivers=attached breathing=1 notice=%d release=%d static_plate=flip mean_footslip=%s breath_px=%.2f threat_patrol=%.1fpx rim_split=%.2f" % [
		int(_notice_radius), int(_release_radius), slip_report, _breath_ground_px, _threat_drift, _rim_split,
	])
	get_tree().quit(0)

func _check_wander_gait_follows_body() -> void:
	var walker := PixelSprite.create_npc_sprite("villager_f")
	walker.position = Vector2(300, 300)
	add_child(walker)
	await get_tree().process_frame
	MapEffects.add_npc_wander(walker, 70.0)
	await get_tree().process_frame

	var sync := walker.get_node_or_null("WanderGaitSync")
	assert(sync != null, "배회를 시작하면 보폭 동기화가 붙어야 한다")

	while not String(walker.animation).begins_with("walk_"):
		await get_tree().process_frame
	await get_tree().process_frame

	# 다리 회전이 실제로 매 프레임 바뀌는가. 예전에는 한 값으로 고정돼 있었다.
	var previous := walker.position
	var cadences: Array[float] = []
	var speeds: Array[float] = []
	for _frame in range(90):
		await get_tree().process_frame
		if not String(walker.animation).begins_with("walk_"):
			break
		var delta := get_process_delta_time()
		speeds.append(walker.position.distance_to(previous) / maxf(delta, 0.0001))
		previous = walker.position
		cadences.append(walker.speed_scale)
	assert(speeds.size() >= 20, "걷는 구간을 충분히 관측해야 한다 (%d)" % speeds.size())

	var distinct: Dictionary = {}
	for cadence in cadences:
		distinct[snappedf(cadence, 0.01)] = true
	assert(distinct.size() >= 4, "다리 회전이 몸 속도를 따라가지 않는다 (관측된 보폭 값 %d개)" % distinct.size())

	# 진짜 검사해야 하는 것은 발이 지면을 긁는 양이다.
	# 다리는 그 프레임에 speed_scale * 기준속도만큼 "지면을 밀었다"고 주장하고,
	# 몸은 실제로 간 거리만큼만 간다. 그 차이가 미끄러짐이다.
	#
	# 단일 프레임의 최대/최소를 비교하면 안 된다. 이동 곡선의 양 극단은 측정 오차가
	# 가장 큰 지점이다. 구간 평균으로 재야 의도한 성질을 본다.
	# (S235 실측: 개선 전 10.2 px/s -> 해석적 보폭 적용 후 0.4~1.7 px/s)
	var total_slip := 0.0
	for i in range(speeds.size()):
		total_slip += absf(cadences[i] * MapEffects.WANDER_GAIT_REFERENCE - speeds[i])
	var mean_slip := total_slip / float(speeds.size())
	# 미끄러짐은 실제 프레임 시간에서만 의미가 있다. 헤드리스는 최대 속도로 돌기 때문에
	# delta가 들쭉날쭉해서 이 수치가 무의미해진다. 그럴 때는 조용히 통과시키는 대신
	# 검사하지 않았다고 밝힌다. 구조 계약(위 세 개)은 어느 쪽에서도 그대로 검사한다.
	_slip_measured = DisplayServer.get_name() != "headless"
	_mean_slip = mean_slip
	if _slip_measured:
		assert(mean_slip < 4.0,
			"걷는 동안 발이 지면을 긁는다 (평균 %.1f px/s). 다리가 몸 속도를 따라가지 않는다." % mean_slip)

	# 걸음이 끝나면 다리도 멈춘다.
	while String(walker.animation).begins_with("walk_"):
		await get_tree().process_frame
	await get_tree().process_frame
	assert(walker.get_node_or_null("WanderGaitSync") == null, "멈추면 보폭 동기화가 떨어져야 한다")
	assert(is_equal_approx(walker.speed_scale, 1.0), "정지 상태의 speed_scale이 원래대로 돌아와야 한다 (%.2f)" % walker.speed_scale)
	walker.set_meta("wander_active", false)
	walker.queue_free()
	await get_tree().process_frame

func _make_voice(art_path: String, at: Vector2) -> Node2D:
	var npc: Node2D = load("res://scenes/npc/npc.tscn").instantiate()
	npc.set("npc_name", "Witness")
	npc.set("field_art_path", art_path)
	npc.position = at
	add_child(npc)
	_notice_radius = float(npc.PRESENCE_NOTICE_RADIUS)
	_release_radius = float(npc.PRESENCE_RELEASE_RADIUS)
	return npc

func _check_population_is_alive() -> void:
	# 시트 기반(네 방향이 실제로 다른) NPC로 검사한다.
	var npc := _make_voice("", Vector2(600, 300))
	await get_tree().process_frame
	await get_tree().process_frame

	var sprite := npc.get_node_or_null("CharacterSprite") as AnimatedSprite2D
	assert(sprite != null, "월드 인구도 캐릭터 스프라이트를 가져야 한다")
	assert(sprite.get_node_or_null("FieldMotionDriver") != null, "서 있는 사람도 ambient 드라이버를 달아야 한다")

	# 정지 삽화는 위치가 아니라 자세로 산다. 호흡 폭을 잰다.
	var low := sprite.offset
	var high := sprite.offset
	for _frame in range(150):
		await get_tree().process_frame
		low = Vector2(minf(low.x, sprite.offset.x), minf(low.y, sprite.offset.y))
		high = Vector2(maxf(high.x, sprite.offset.x), maxf(high.y, sprite.offset.y))
	# 로컬 offset 값이 0이 아닌 것과 화면에서 보이는 것은 다르다.
	# offset은 sprite.scale이 다시 곱해지고, 필드 배우의 스케일은 원본 그림 크기에 따라
	# 0.04에서 0.34까지 벌어진다. 실측 당시 호흡은 화면상 0.02~0.18px, 즉 보이지 않았다.
	# 검사할 것은 "지면 위 픽셀"이다.
	var local_span := (high - low).length()
	var ground_span := local_span * absf(sprite.scale.y)
	assert(ground_span > 0.3,
		"서 있는 사람의 호흡이 화면에서 보이지 않는다 (지면 %.3fpx, 로컬 %.3f, 스케일 %.3f)" % [
			ground_span, local_span, sprite.scale.y
		])
	assert(ground_span < 2.0, "호흡이 과하다. 서 있는 사람이 출렁이면 안 된다 (%.3fpx)" % ground_span)
	_breath_ground_px = ground_span
	npc.queue_free()
	await get_tree().process_frame

func _check_presence_hysteresis() -> void:
	var npc := _make_voice("", Vector2(600, 300))
	await get_tree().process_frame
	var sprite := npc.get_node_or_null("CharacterSprite") as AnimatedSprite2D
	assert(sprite != null, "NPC 스프라이트가 있어야 한다")

	var player := Node2D.new()
	player.add_to_group("player")
	player.global_position = Vector2(1400, 300)
	add_child(player)
	await _settle_presence(npc)
	assert(not bool(npc.get("_facing_player")), "멀리 있으면 알아채지 않아야 한다")

	# 알아채는 반경 안으로 들어온다.
	player.global_position = npc.global_position + Vector2(_notice_radius - 12.0, 0)
	await _settle_presence(npc)
	assert(bool(npc.get("_facing_player")), "가까이 오면 몸을 돌려야 한다")

	# 알아챈 반경과 놓아주는 반경 사이에서는 상태가 유지된다 (깜빡임 방지).
	player.global_position = npc.global_position + Vector2((_notice_radius + _release_radius) * 0.5, 0)
	await _settle_presence(npc)
	assert(bool(npc.get("_facing_player")), "이력 구간에서 상태가 흔들리면 안 된다")

	# 완전히 멀어지면 원래 방향으로 돌아온다.
	player.global_position = npc.global_position + Vector2(_release_radius + 60.0, 0)
	await _settle_presence(npc)
	assert(not bool(npc.get("_facing_player")), "지나가면 원래 자세로 돌아와야 한다")

	player.queue_free()
	npc.queue_free()
	await get_tree().process_frame

func _check_static_plate_turns_by_flip() -> void:
	# 네 방향이 모두 같은 그림인 NPC는 자세를 바꿔 봐야 화면이 달라지지 않는다.
	# 없는 포즈를 지어내는 대신 좌우 반전으로 "돌아봤다"를 표현해야 한다.
	var plate_art := "res://assets/sprites/field/arrel/down.png"
	if not ResourceLoader.exists(plate_art):
		return
	var npc := _make_voice(plate_art, Vector2(600, 500))
	await get_tree().process_frame
	var sprite := npc.get_node_or_null("CharacterSprite") as AnimatedSprite2D
	assert(sprite != null and bool(sprite.get_meta("field_static_plate", false)), "단일 삽화 NPC는 그 사실을 표시해야 한다")

	var player := Node2D.new()
	player.add_to_group("player")
	player.global_position = npc.global_position + Vector2(-40, 0)  # 왼쪽에 선다
	add_child(player)
	await _settle_presence(npc)
	assert(sprite.flip_h, "왼쪽의 플레이어를 보려면 좌우 반전되어야 한다")

	player.global_position = npc.global_position + Vector2(40, 0)  # 오른쪽으로 돌아간다
	await _settle_presence(npc)
	assert(not sprite.flip_h, "오른쪽으로 오면 반전이 풀려야 한다")

	player.queue_free()
	npc.queue_free()
	await get_tree().process_frame

## S236: 위협은 사냥한다고 말한다. 제자리에 굳어 있으면 지형지물과 구분되지 않는다.
func _check_threats_patrol() -> void:
	var threat := FieldThreat.new()
	threat.configure("probe_map", {"id": "probe"}, "probe_flag")
	threat.position = Vector2(900, 400)
	add_child(threat)
	await get_tree().process_frame

	var player := Node2D.new()
	player.add_to_group("player")
	player.global_position = Vector2(1600, 400)  # 감지 밖
	add_child(player)

	var start := threat.position
	var far_drift := 0.0
	for _frame in range(120):
		await get_tree().process_frame
		far_drift = maxf(far_drift, threat.position.distance_to(start))
	assert(far_drift > 3.0, "위협이 제자리에 굳어 있다 (이동 %.2fpx)" % far_drift)
	assert(far_drift < FieldThreat.PATROL_RADIUS * 2.5,
		"순찰이 자기 자리를 벗어나면 배치 의도가 깨진다 (%.2fpx)" % far_drift)

	# 플레이어가 다가오면 그쪽으로 기운다.
	# 순찰 궤도는 진동이므로 한 순간의 좌표를 보면 위상에 따라 답이 뒤집힌다.
	# (발 미끄러짐 때와 같은 함정이다.) 궤도와 무관한 "이번 프레임의 목표"를 읽어
	# 기울기 성분만 검사한다.
	player.global_position = threat.global_position + Vector2(70, 0)
	await get_tree().process_frame
	await get_tree().process_frame
	var right_target: float = float(threat.get("_patrol_target").x) - sin(float(threat.get("_patrol_phase"))) * FieldThreat.PATROL_RADIUS
	player.global_position = threat.global_position + Vector2(-70, 0)
	await get_tree().process_frame
	await get_tree().process_frame
	var left_target: float = float(threat.get("_patrol_target").x) - sin(float(threat.get("_patrol_phase"))) * FieldThreat.PATROL_RADIUS
	assert(right_target > left_target,
		"압박이 걸렸는데 플레이어 쪽으로 기울지 않는다 (오른쪽 %.2f, 왼쪽 %.2f)" % [right_target, left_target])
	_threat_drift = far_drift

	player.queue_free()
	threat.queue_free()
	await get_tree().process_frame

## S237: 실루엣은 밝은 지형과 어두운 지형 양쪽에서 읽혀야 한다.
## 어두운 테두리 하나만 쓰면 어두운 맵에서 사라진다(실측: forgotten_forest 0.0014).
## 바깥은 어둡게, 안쪽은 밝게 두 겹을 갖춰야 배경이 어느 쪽이든 한쪽이 대비를 만든다.
func _check_silhouette_rim() -> void:
	var npc := _make_voice("", Vector2(300, 700))
	await get_tree().process_frame
	var sprite := npc.get_node_or_null("CharacterSprite") as AnimatedSprite2D
	assert(sprite != null, "필드 배우 스프라이트가 있어야 한다")
	var material := sprite.material as ShaderMaterial
	assert(material != null, "필드 배우는 실루엣 마감 머티리얼을 써야 한다")

	# 마감 헬퍼가 계약을 직접 실어야 한다. 셰이더 기본값에 기대면 조용히 어긋난다.
	var outline: Color = material.get_shader_parameter("outline_color")
	var rim: Color = material.get_shader_parameter("rim_color")
	var rim_strength: float = float(material.get_shader_parameter("rim_strength"))
	assert(rim_strength > 0.0, "안쪽 밝은 테가 꺼져 있으면 어두운 지형에서 실루엣이 사라진다")
	# 두 겹이 실제로 반대 방향이어야 의미가 있다. 둘 다 어두우면 한 겹과 다를 바 없다.
	var outline_luma := 0.2126 * outline.r + 0.7152 * outline.g + 0.0722 * outline.b
	var rim_luma := 0.2126 * rim.r + 0.7152 * rim.g + 0.0722 * rim.b
	assert(rim_luma - outline_luma > 0.5,
		"바깥 테와 안쪽 테의 밝기가 갈라져야 한다 (바깥 %.2f, 안쪽 %.2f)" % [outline_luma, rim_luma])
	_rim_split = rim_luma - outline_luma

	npc.queue_free()
	await get_tree().process_frame

## 존재 감지는 0.2초마다 한 번 돈다. 그 주기를 넘겨 준다.
func _settle_presence(npc: Node) -> void:
	npc.set("_presence_scan", 0.0)
	for _frame in range(4):
		await get_tree().process_frame
