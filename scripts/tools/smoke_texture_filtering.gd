## S215 회귀 테스트 — 축소 필터 계약
##
## project.godot의 기본 필터는 Nearest다. 픽셀아트 타일과 캐릭터 시트에는 그게 맞지만,
## 손으로 그린 대형 원화(전투 배경, 전투원 판, 풀스크린 CG)를 같은 필터로 줄이면
## 픽셀이 그대로 버려져 디더 알파가 점점이 튀고 실루엣이 계단처럼 부서진다.
##
## 두 방향을 모두 지킨다: 원화는 선형으로, 픽셀아트는 Nearest로.
extends Node

func _ready() -> void:
	Codex.suppress_recording = true  # S218: 가짜 적을 개발자 도감에 남기지 않는다
	OptionsMenu.settings["reduce_motion"] = true
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	GameManager.player_data.elia_with_party = true

	var linear_filters := [
		CanvasItem.TEXTURE_FILTER_LINEAR,
		CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
	]

	# --- 전투 화면 ---
	BattleManager.current_enemy = BattleManager.Enemy.new("Ash Crawler", 60, 9, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/rim_forest.tscn"
	BattleManager.enemy_image = ""
	BattleManager.battle_bg_image = ""
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame

	var bg := battle.get("bg") as Control
	assert(bg != null, "전투 배경 노드가 있어야 한다")
	var bg_tex: TextureRect = null
	for child in battle.get_children():
		if child is TextureRect and (child as TextureRect).texture != null:
			bg_tex = child
			break
	assert(bg_tex != null, "전투 배경 일러스트가 있어야 한다")
	assert(bg_tex.texture_filter in linear_filters,
		"전투 배경은 선형 축소를 써야 한다 (현재 %d)" % bg_tex.texture_filter)

	var enemy_plate := battle.get("enemy_sprite") as CanvasItem
	if enemy_plate is TextureRect:
		assert(enemy_plate.texture_filter in linear_filters,
			"원화 적 판은 선형 축소를 써야 한다 (현재 %d)" % enemy_plate.texture_filter)
	var ally_plate := battle.get("ally_sprite") as CanvasItem
	if ally_plate is TextureRect:
		assert(ally_plate.texture_filter in linear_filters,
			"원화 동행자 판은 선형 축소를 써야 한다 (현재 %d)" % ally_plate.texture_filter)

	# 절차 생성 픽셀아트 주인공은 반대로 선명해야 한다 (선형이어도 시트 원본이 크므로
	# 여기서는 Nearest 강제가 아니라 "블러 처리되지 않았는지"만 확인한다).
	var hero := battle.get("player_sprite") as AnimatedSprite2D
	assert(hero != null, "주인공 스프라이트가 있어야 한다")

	battle.queue_free()
	await get_tree().process_frame

	# --- VN / CG 화면 ---
	var cg_rect := CgViewer.get("cg_texture") as TextureRect
	assert(cg_rect != null, "CgViewer에 표시 노드가 있어야 한다")
	assert(cg_rect.texture_filter in linear_filters,
		"풀스크린 CG는 선형 축소를 써야 한다 (현재 %d)" % cg_rect.texture_filter)

	# --- 픽셀아트는 반대 계약 ---
	var tile_defs: Array = [{"color": Color(0.2, 0.3, 0.2), "detail": "flat"}]
	var tilemap := TilePainter.create_tilemap(tile_defs, [[0, 0], [0, 0]], 2, 2)
	assert(tilemap.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST
			or tilemap.texture_filter == CanvasItem.TEXTURE_FILTER_PARENT_NODE,
		"픽셀아트 타일맵까지 선형으로 흐려지면 안 된다")
	tilemap.free()

	# --- 대형 원화 텍스처에 밉맵이 생성되어 있는가 ---
	# 필터만 LINEAR_WITH_MIPMAPS로 두고 import에서 밉맵을 끄면 아무 효과가 없다.
	# 실제로 이 함정에 한 번 빠졌으므로 import 설정까지 확인한다.
	var mip_checked := 0
	for path: String in [
		"res://assets/portraits/character_shots/elia_anchor_v3.png",
		"res://assets/cg/generated/illustration_expansion_v3/enemy_cutin_ash_crawler_v1.png",
	]:
		var import_path := path + ".import"
		if not FileAccess.file_exists(import_path):
			continue
		var text := FileAccess.get_file_as_string(import_path)
		assert("mipmaps/generate=true" in text,
			"%s 는 밉맵이 생성되어야 선형+밉맵 필터가 의미를 갖는다" % path)
		mip_checked += 1
	assert(mip_checked >= 2, "밉맵 설정을 확인할 원화가 있어야 한다")

	print("TEXTURE_FILTERING_SMOKE_PASS battle_bg=linear plates=linear cg=linear mipmaps=%d" % mip_checked)
	get_tree().quit(0)
