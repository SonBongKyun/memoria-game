## S212 회귀 테스트
## 2D 전투원이 3D 무대와 실제로 결합되어 있는지 검증한다.
## 정지 화면만 보면 "3D 배경 위에 2D를 얹은 것"과 구분되지 않으므로,
## 카메라를 움직였을 때 캐릭터가 따라가는지를 직접 확인해야 한다.
extends Node

func _ready() -> void:
	var previous_reduce: Variant = OptionsMenu.settings.get("reduce_motion", true)
	OptionsMenu.settings["reduce_motion"] = true
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

	var stage: HybridDepthStage = battle.get("_hybrid_depth_stage")
	assert(stage != null, "전투에 하이브리드 무대가 있어야 한다")

	# --- 두 좌표계가 같은 자리를 가리키는가 ---
	var anchors: Array = battle.get("_battler_anchors")
	assert(anchors.size() >= 3, "전투원 앵커가 만들어져야 한다 (현재 %d)" % anchors.size())
	var player_anchor: Control = null
	var enemy_anchor: Control = null
	for anchor: Control in anchors:
		if anchor.name == "PlayerAnchor":
			player_anchor = anchor
		elif anchor.name == "EnemyAnchor":
			enemy_anchor = anchor
	assert(player_anchor != null and enemy_anchor != null, "플레이어/적 앵커가 있어야 한다")

	# 앵커의 3D 좌표를 다시 화면으로 투영하면 2D 발 위치로 돌아와야 한다.
	var baseline: float = battle.STAGE_BASELINE_Y
	var player_world: Vector3 = player_anchor.get_meta("world_anchor")
	var projected: Vector2 = stage.world_to_canvas(player_world)
	assert(absf(projected.x - 226.0) < 6.0 and absf(projected.y - baseline) < 6.0,
		"플레이어의 3D 앵커는 2D 발 위치로 되돌아와야 한다 (투영 %s)" % str(projected))

	# 3D 접지 그림자와 포커스 링도 같은 x에 있어야 한다.
	var focus_x: float = stage.battle_player_focus_root.position.x
	assert(absf(focus_x - player_world.x) < 0.05,
		"포커스 링이 플레이어 앵커에서 벗어났다 (%f vs %f)" % [focus_x, player_world.x])

	# --- 기준 자세에서는 레이아웃이 조금도 달라지지 않아야 한다 ---
	assert(player_anchor.position.length() < 0.5,
		"카메라가 기준 자세일 때 앵커 오프셋은 0이어야 한다 (현재 %s)" % str(player_anchor.position))

	# --- 카메라가 움직이면 캐릭터도 따라가야 한다 ---
	stage.set_battle_focus("enemy")
	# 카메라는 지수 감쇠로 목표에 다가간다. 충분히 정착시킨 뒤에 재야
	# 앵커 값과 즉석 계산값이 같은 프레임을 가리킨다.
	for _f in range(90):
		await get_tree().process_frame
	var moved: float = player_anchor.position.length()
	assert(moved > 1.0,
		"카메라가 패닝했는데 캐릭터가 그대로다. 3D가 배경 벽지로 남는다. (이동 %.2f)" % moved)

	# 캐릭터는 자기 3D 접지 그림자에 붙어 있어야 한다.
	# 그림자와 포커스 링은 3D 무대 안에 있어 카메라와 함께 온전히 움직이므로,
	# 캐릭터만 감쇠시키면 자기 그림자에서 미끄러져 나온다.
	var raw_offset: Vector2 = stage.anchor_offset(player_world)
	# 0.5px 여유. 감쇠 계수(예: 0.62)가 들어오면 4px 가까이 벌어지므로 충분히 걸린다.
	assert(absf(moved - raw_offset.length()) < 0.5,
		"캐릭터가 자기 접지 그림자에서 벗어났다 (%.2f vs %.2f)" % [moved, raw_offset.length()])
	# 원근 시차는 깊이에서 나온다. 앞에 선 플레이어가 더 먼 적보다 많이 움직여야 한다.
	var enemy_world: Vector3 = enemy_anchor.get_meta("world_anchor")
	assert(stage.anchor_offset(player_world).length() > 0.0 and enemy_anchor.position.length() > 0.0,
		"두 전투원 모두 카메라 이동을 받아야 한다")
	assert(absf(player_world.z - enemy_world.z) < 2.0,
		"두 앵커가 비슷한 깊이에 있어야 좌우 구도가 유지된다")

	# --- S213: 전경 레이어 ---
	var foreground: HybridDepthStage = battle.get("_foreground_depth_stage")
	assert(foreground != null, "전경 3D 레이어가 있어야 한다 (Clean View에서도 켜진다)")
	assert(foreground.follow_stage == stage,
		"전경이 전투 무대의 카메라를 따라가지 않으면 시차가 어긋난다")
	assert(foreground.z_index > 0,
		"전경은 2D 전투원 위에 합성되어야 한다")
	# 기하는 화면 양 끝에만 있어야 한다. 안쪽으로 들어오면 전투 정보를 가린다.
	var innermost: float = 99.0
	for child in foreground.motion_root.get_children():
		var mesh := child as MeshInstance3D
		if mesh != null:
			innermost = minf(innermost, absf(mesh.position.x))
	assert(innermost > 2.9,
		"전경 기둥이 화면 안쪽으로 들어왔다 (가장 안쪽 x=%.2f). 전투원과 UI를 가린다." % innermost)

	# --- S213: 기억 연소에 무대가 반응하는가 ---
	assert(stage._burn_flare <= 0.01, "연소 전에는 무대가 평온해야 한다")
	stage.play_memory_burn(2)
	assert(stage._burn_flare > 0.5,
		"기억 연소는 이 게임의 심장이다. 무대가 반응하지 않으면 3D는 장식으로 남는다.")
	var flare_at_burn: float = stage._burn_flare
	for _f in range(40):
		await get_tree().process_frame
	assert(stage._burn_flare < flare_at_burn,
		"연소 반응은 시간이 지나면 잦아들어야 한다 (%.2f -> %.2f)" % [flare_at_burn, stage._burn_flare])

	OptionsMenu.settings["reduce_motion"] = previous_reduce
	print("HYBRID_COUPLING_SMOKE_PASS anchors=%d projected=(%.0f, %.0f) pan_shift=%.2f foreground=1 burn_reaction=1" % [
		anchors.size(), projected.x, projected.y, moved,
	])
	get_tree().quit(0)
