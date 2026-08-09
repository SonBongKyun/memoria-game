extends Node

const ARREL_PATH := "res://assets/portraits/character_shots/arrel_battle_v3.png"

func _ready() -> void:
	Codex.suppress_recording = true
	var previous_clean := bool(OptionsMenu.settings.get("clean_gameplay_visuals", false))
	var previous_reduce := bool(OptionsMenu.settings.get("reduce_motion", false))
	var previous_sable := BattleManager.sable_in_party
	var previous_tobias := BattleManager.tobias_in_party
	var previous_elia := bool(GameManager.player_data.get("elia_with_party", false))

	OptionsMenu.settings["clean_gameplay_visuals"] = false
	OptionsMenu.settings["reduce_motion"] = false
	GameManager.current_locale = "en"
	BattleManager.current_enemy = BattleManager.Enemy.new("Stage Contract Dummy", 180, 16, false)
	BattleManager.state = BattleManager.BattleState.PLAYER_TURN
	BattleManager.return_scene = "res://scenes/maps/the_seam.tscn"
	BattleManager.enemy_image = "res://assets/cg/character_shots/shade_sentinel_guard_v3.png"
	BattleManager.battle_bg_image = "res://assets/cg/generated/story_ch5_seam_first_light.png"

	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false
	GameManager.player_data.elia_with_party = false
	var solo := await _spawn_battle()
	_validate_canonical_player(solo)
	_validate_role_profiles_and_anchors(solo, ["player", "enemy"])
	_validate_visible_bounds(solo, ["player", "enemy"])
	await _validate_explicit_fallback_contract(solo)
	solo.queue_free()
	await get_tree().process_frame

	BattleManager.sable_in_party = false
	BattleManager.tobias_in_party = false
	GameManager.player_data.elia_with_party = true
	var elia_only := await _spawn_battle()
	await _validate_displayed_ally_identity(elia_only, "Elia")
	elia_only.queue_free()
	await get_tree().process_frame

	BattleManager.sable_in_party = true
	BattleManager.tobias_in_party = true
	GameManager.player_data.elia_with_party = true
	var party := await _spawn_battle()
	_validate_role_profiles_and_anchors(party, ["player", "ally", "support", "enemy"])
	_validate_visible_bounds(party, ["player", "ally", "support", "enemy"])
	_validate_ui_clearance(party, ["player", "ally", "support", "enemy"])
	_validate_stage_order(party)
	_validate_focus_and_material_restore(party)
	await _validate_displayed_ally_identity(party, "Sable", "Elia")
	await _validate_authored_modulate_restore(party)
	await _validate_accessibility_and_semantics(party)
	party.queue_free()
	await get_tree().process_frame

	OptionsMenu.settings["clean_gameplay_visuals"] = previous_clean
	OptionsMenu.settings["reduce_motion"] = previous_reduce
	BattleManager.sable_in_party = previous_sable
	BattleManager.tobias_in_party = previous_tobias
	GameManager.player_data.elia_with_party = previous_elia

	print("CINEMATIC_BATTLE_STAGE_SMOKE_PASS canonical=TextureRect profiles=4 anchors=4 solo=visible max_party=visible ui_clear=1 focus=player_enemy neutral_brightness=1 material_restore=1 displayed_ally=guarded modulation_restore=1 reduce_motion=static_settle stale_guard=1 normal_mechanics=1 clean_view=1 semantics=attack_cast_hurt_down animated_fallback=static fallback_init=static fallbacks=explicit")
	get_tree().quit(0)

func _spawn_battle() -> Node:
	var battle: Node = load("res://scenes/battle/battle_scene.tscn").instantiate()
	add_child(battle)
	await get_tree().process_frame
	await get_tree().process_frame
	return battle

func _validate_canonical_player(battle: Node) -> void:
	var player := battle.get("player_sprite") as TextureRect
	assert(player != null, "Canonical Arrel must be a CanvasItem-compatible TextureRect")
	assert(player.texture != null and player.texture.resource_path == ARREL_PATH, "Canonical Arrel must use the exact battle plate path")
	assert(player.texture_filter == CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, "Painterly Arrel must use mipmapped linear filtering")
	assert(String(player.get_meta("battle_art_kind", "")) == "painterly", "Canonical Arrel must identify its painterly stage path")
	assert(not bool(player.get_meta("canonical_fallback", false)), "The canonical path must not report itself as fallback")

func _validate_role_profiles_and_anchors(battle: Node, roles: Array[String]) -> void:
	var profiles: Dictionary = battle.BATTLE_ROLE_PROFILES
	var stage := battle.get("_hybrid_depth_stage") as HybridDepthStage
	assert(stage != null, "Cinematic stage requires HybridDepthStage")
	for role in roles:
		assert(profiles.has(role), "Missing battle role profile: %s" % role)
		var profile: Dictionary = profiles[role]
		for key in ["anchor_name", "stage_key", "stage_anchor", "canvas_foot", "container_size", "local_foot", "plate_max_size", "plate_local_foot", "stage_order"]:
			assert(profile.has(key), "%s role profile must own %s" % [role, key])
		var container := _role_container(battle, role)
		assert(container != null, "Missing %s battler container" % role)
		var expected_position: Vector2 = profile["canvas_foot"] - profile["local_foot"]
		# Idle breathing may offset the live container between frames. The stored
		# base is the authoritative action-return position and must come directly
		# from the shared role profile.
		var base_position := _role_base_position(battle, role)
		assert(base_position.distance_to(expected_position) < 0.01, "%s container base must derive from its role profile" % role)
		var anchor := _role_anchor(battle, role)
		assert(anchor != null and anchor.name == String(profile["anchor_name"]), "%s anchor must derive from its role profile" % role)
		var projected: Vector3 = stage.canvas_to_floor(profile["canvas_foot"])
		var stored: Vector3 = anchor.get_meta("world_anchor", Vector3.ZERO)
		assert(stored.distance_to(projected) < 0.02, "%s 3D anchor must project the same canvas foot" % role)
		var contact_shadow: MeshInstance3D = stage._contact_shadows.get(String(profile["stage_key"]))
		assert(contact_shadow != null and contact_shadow.position.distance_to(Vector3(projected.x, HybridDepthStage.ARENA_FLOOR_Y + 0.012, projected.z)) < 0.03, "%s contact shadow must couple to the profile anchor" % role)

func _validate_visible_bounds(battle: Node, roles: Array[String]) -> void:
	var viewport_rect := get_viewport().get_visible_rect()
	for role in roles:
		var actor := _role_actor(battle, role) as Control
		assert(actor != null and actor.visible, "%s actor must be visible" % role)
		var rect := actor.get_global_rect()
		assert(rect.position.x >= -1.0 and rect.end.x <= viewport_rect.size.x + 1.0, "%s stage plate must stay inside the 1280-wide tableau" % role)
		assert(rect.position.y >= 72.0 and rect.end.y <= 438.0, "%s stage plate must stay inside the open battle tableau" % role)

func _validate_ui_clearance(battle: Node, roles: Array[String]) -> void:
	var ui_regions: Array[Control] = [
		battle.get("objective_panel") as Control,
		battle.get("enemy_hp_bar") as Control,
		battle.get("player_hp_bar") as Control,
		battle.get("field_readout_art") as Control,
		battle.get("action_ribbon_art") as Control,
	]
	for role in roles:
		var actor := _role_actor(battle, role) as Control
		var actor_rect := actor.get_global_rect()
		for region in ui_regions:
			if region == null or not region.visible or region.get_global_rect().size.length() <= 1.0:
				continue
			assert(not actor_rect.intersects(region.get_global_rect()), "%s actor must not obscure %s" % [role, region.name])

func _validate_stage_order(battle: Node) -> void:
	var profiles: Dictionary = battle.BATTLE_ROLE_PROFILES
	assert(int(profiles["ally"]["stage_order"]) < int(profiles["player"]["stage_order"]), "Companion rear line must render behind primary Arrel")
	assert(int(profiles["support"]["stage_order"]) < int(profiles["player"]["stage_order"]), "Support rear line must render behind primary Arrel")
	assert((_role_actor(battle, "player") as Control).size.y > (_role_actor(battle, "ally") as Control).size.y, "Arrel must retain player-primary visual weight")

func _validate_focus_and_material_restore(battle: Node) -> void:
	var player := _role_actor(battle, "player") as CanvasItem
	var enemy := _role_actor(battle, "enemy") as CanvasItem
	var ally := _role_actor(battle, "ally") as CanvasItem
	var support := _role_actor(battle, "support") as CanvasItem
	var player_container := _role_container(battle, "player")
	var enemy_container := _role_container(battle, "enemy")
	var player_container_before := player_container.position
	var enemy_container_before := enemy_container.position
	var stage := battle.get("_hybrid_depth_stage") as HybridDepthStage
	battle.call("_set_battle_stage_focus", "player")
	assert(battle.get("_current_battler_focus") == "player" and player.self_modulate.r > enemy.self_modulate.r, "Player turn must add painterly emphasis alongside HybridDepthStage focus")
	var player_focus := _assert_neutral_focus_tint(player, "player")
	var enemy_focus := _assert_neutral_focus_tint(enemy, "enemy")
	var ally_focus := _assert_neutral_focus_tint(ally, "ally")
	var support_focus := _assert_neutral_focus_tint(support, "support")
	assert(_focus_brightness(player_focus) > _focus_brightness(enemy_focus) and _focus_brightness(player_focus) > _focus_brightness(ally_focus) and _focus_brightness(player_focus) > _focus_brightness(support_focus), "Player focus must brighten the active side above inactive and rear battlers")
	assert(player_container.position.distance_to(player_container_before) < 0.01 and enemy_container.position.distance_to(enemy_container_before) < 0.01, "Focus emphasis must never reposition battler containers")
	assert(stage._focus_pan < 0.0, "Player focus must retain HybridDepthStage camera direction")
	battle.call("_set_battle_stage_focus", "enemy")
	assert(battle.get("_current_battler_focus") == "enemy" and enemy.self_modulate.r > player.self_modulate.r, "Enemy turn must move emphasis without moving battler containers")
	player_focus = _assert_neutral_focus_tint(player, "player")
	enemy_focus = _assert_neutral_focus_tint(enemy, "enemy")
	ally_focus = _assert_neutral_focus_tint(ally, "ally")
	support_focus = _assert_neutral_focus_tint(support, "support")
	assert(_focus_brightness(enemy_focus) > _focus_brightness(player_focus) and _focus_brightness(enemy_focus) > _focus_brightness(ally_focus) and _focus_brightness(enemy_focus) > _focus_brightness(support_focus), "Enemy focus must brighten the active side above inactive and rear battlers")
	assert(player_container.position.distance_to(player_container_before) < 0.01 and enemy_container.position.distance_to(enemy_container_before) < 0.01, "Enemy focus must keep battler containers position-static")
	assert(stage._focus_pan > 0.0, "Enemy focus must retain HybridDepthStage camera direction")
	for actor in [player, enemy]:
		var stage_blend: Material = actor.get_meta("stage_blend_material", null) as Material
		assert(stage_blend != null, "Painterly actor must retain its stage blend material")
		actor.material = ShaderMaterial.new()
		battle.call("_restore_plate_material", actor)
		assert(actor.material == stage_blend, "Temporary material must restore the actor's current stage blend")

func _validate_displayed_ally_identity(battle: Node, expected_identity: String, hidden_identity: String = "") -> void:
	var displayed := _role_actor(battle, "ally") as CanvasItem
	assert(displayed != null, "%s party must build a visible ally actor" % expected_identity)
	assert(String(battle.get("_displayed_ally_identity")) == expected_identity, "Battle must explicitly record the displayed ally identity")
	assert(String(displayed.get_meta("displayed_character", "")) == expected_identity, "The shared ally plate must identify its displayed character")
	if hidden_identity != "":
		var before_hidden := String(displayed.get_meta("semantic_state", ""))
		battle.call("_on_ally_action", hidden_identity, "anchor_pulse", 0)
		assert(String(displayed.get_meta("semantic_state", "")) == before_hidden, "%s action must not animate the displayed %s plate" % [hidden_identity, expected_identity])
	var displayed_action := "strike" if expected_identity == "Sable" else "anchor_pulse"
	var expected_semantic := "attack" if displayed_action == "strike" else "cast"
	battle.call("_on_ally_action", expected_identity, displayed_action, 0)
	assert(String(displayed.get_meta("semantic_state", "")) == expected_semantic, "%s action must animate its own displayed plate" % expected_identity)
	await get_tree().create_timer(0.45).timeout
	assert(String(displayed.get_meta("semantic_state", "")) == "idle", "Displayed ally semantics must settle after their normal-motion action")
	if expected_identity == "Sable":
		var tobias := _role_actor(battle, "support") as CanvasItem
		assert(tobias != null and String(tobias.get_meta("displayed_character", "")) == "Tobias", "Tobias must remain mapped to the support plate")
		battle.call("_on_ally_action", "Tobias", "analyze", 0)
		assert(String(tobias.get_meta("semantic_state", "")) == "cast", "Tobias actions must animate Tobias rather than the shared ally plate")
		await get_tree().create_timer(0.45).timeout

func _validate_authored_modulate_restore(battle: Node) -> void:
	var player := _role_actor(battle, "player") as CanvasItem
	var enemy := _role_actor(battle, "enemy") as CanvasItem
	var player_base: Color = player.get_meta("battle_base_modulate", Color.WHITE)
	var enemy_base: Color = enemy.get_meta("battle_base_modulate", Color.WHITE)
	var player_self := player.self_modulate
	var enemy_self := enemy.self_modulate
	assert(_colors_match(player.modulate, player_base) and _colors_match(enemy.modulate, enemy_base), "Painterly profile colors must be registered as authored base modulation")
	battle.call("_hit_flash", "Arrel")
	await get_tree().create_timer(0.36).timeout
	assert(_colors_match(player.modulate, player_base), "Player hit feedback must restore the exact recorded base modulation")
	assert(_colors_match(player.self_modulate, player_self), "Player hit feedback must not disturb focus or semantic self_modulate")
	battle.call("_hit_flash", "Stage Contract Dummy")
	await get_tree().create_timer(0.26).timeout
	assert(_colors_match(enemy.modulate, enemy_base), "Enemy hit feedback must restore the exact recorded base modulation")
	assert(_colors_match(enemy.self_modulate, enemy_self), "Enemy hit feedback must not disturb focus or semantic self_modulate")
	var previous_enemy_statuses: Array = BattleManager.enemy_statuses.duplicate()
	BattleManager.enemy_statuses.clear()
	enemy.modulate = Color(0.2, 0.3, 0.4, 0.5)
	battle.call("_update_enemy_status_visual")
	assert(_colors_match(enemy.modulate, enemy_base), "No-status visual cleanup must restore the enemy's authored base modulation")
	enemy.material = ShaderMaterial.new()
	enemy.modulate = Color(0.5, 0.2, 0.3, 1.0)
	battle.call("_apply_status_shader")
	assert(_colors_match(enemy.modulate, enemy_base), "Status-shader clear must restore the enemy's authored base modulation")
	assert(enemy.material == enemy.get_meta("stage_blend_material", null), "Status-shader clear must retain the current stage blend material")
	BattleManager.enemy_statuses = previous_enemy_statuses

func _colors_match(left: Color, right: Color) -> bool:
	return is_equal_approx(left.r, right.r) and is_equal_approx(left.g, right.g) and is_equal_approx(left.b, right.b) and is_equal_approx(left.a, right.a)

func _assert_neutral_focus_tint(actor: CanvasItem, label: String) -> Color:
	var tint: Color = actor.get_meta("focus_tint", Color.WHITE)
	assert(is_equal_approx(tint.r, tint.g) and is_equal_approx(tint.g, tint.b), "%s focus tint must be brightness-only with equal RGB channels" % label)
	return tint

func _focus_brightness(tint: Color) -> float:
	return (tint.r + tint.g + tint.b) / 3.0

func _validate_accessibility_and_semantics(battle: Node) -> void:
	var player := _role_actor(battle, "player") as CanvasItem
	var player_container := _role_container(battle, "player")
	var player_base: Vector2 = battle.get("_player_base_pos")
	var player_base_scale: Vector2 = player.get_meta("semantic_base_scale", player.scale)
	var player_base_rotation := float(player.get_meta("semantic_base_rotation", player.rotation))
	OptionsMenu.settings["reduce_motion"] = true
	battle.call("_process", 0.25)
	assert(player_container.position.distance_to(player_base) < 0.01, "Reduce Motion must settle decorative battler movement at its base")
	for state in ["attack", "cast", "hurt"]:
		battle.call("_play_actor_anim", player, state)
		assert(player.get_meta("semantic_state", "") == state, "Painterly actor must expose %s semantics without transition motion" % state)
		assert(not (battle.get("_painterly_semantic_tweens") as Dictionary).has(player.get_instance_id()), "Reduce Motion must not run semantic transition tweens")
		await get_tree().create_timer(0.30).timeout
		assert(player.get_meta("semantic_state", "") == "idle", "Reduce Motion must settle the static %s cue back to idle" % state)
		assert(player.scale.distance_to(player_base_scale) < 0.001 and is_equal_approx(player.rotation, player_base_rotation), "Reduce Motion must restore idle scale and rotation after %s" % state)
	# These real battle callbacks deliberately skip their mechanical tween under
	# Reduce Motion, so their static semantic cues must still settle themselves.
	battle.call("_on_pre_attack", "Arrel", "Stage Contract Dummy", "Attack")
	assert(player.get_meta("semantic_state", "") == "attack", "Reduce Motion player actions must retain a readable static attack cue")
	assert(not (battle.get("_painterly_semantic_tweens") as Dictionary).has(player.get_instance_id()), "Reduce Motion player actions must not create semantic transition tweens")
	await get_tree().create_timer(0.30).timeout
	assert(player.get_meta("semantic_state", "") == "idle" and player.scale.distance_to(player_base_scale) < 0.001 and is_equal_approx(player.rotation, player_base_rotation), "Reduce Motion player action callbacks must restore the idle pose")
	battle.call("_on_damage_dealt", "Arrel", 12, "")
	assert(player.get_meta("semantic_state", "") == "hurt", "Reduce Motion damage callbacks must retain a readable static hurt cue")
	assert(not (battle.get("_painterly_semantic_tweens") as Dictionary).has(player.get_instance_id()), "Reduce Motion damage callbacks must not create semantic transition tweens")
	await get_tree().create_timer(0.24).timeout
	assert(player.get_meta("semantic_state", "") == "idle" and player.scale.distance_to(player_base_scale) < 0.001 and is_equal_approx(player.rotation, player_base_rotation), "Reduce Motion damage callbacks must restore the idle pose")
	# The first static cue's deferred cleanup must not erase the later cue.
	battle.call("_play_actor_anim", player, "attack")
	var first_generation := int(battle.call("_painterly_semantic_generation", player))
	await get_tree().create_timer(0.04).timeout
	battle.call("_play_actor_anim", player, "cast")
	var second_generation := int(battle.call("_painterly_semantic_generation", player))
	assert(second_generation > first_generation, "Each painterly semantic state must advance its lifecycle generation")
	await get_tree().create_timer(0.17).timeout
	assert(player.get_meta("semantic_state", "") == "cast", "A stale Reduce Motion cleanup must not overwrite a newer semantic cue")
	await get_tree().create_timer(0.10).timeout
	assert(player.get_meta("semantic_state", "") == "idle" and player.scale.distance_to(player_base_scale) < 0.001 and is_equal_approx(player.rotation, player_base_rotation), "The current Reduce Motion cue must settle cleanly after its own hold")
	battle.call("_play_actor_anim", player, "attack")
	var pre_down_generation := int(battle.call("_painterly_semantic_generation", player))
	await get_tree().create_timer(0.04).timeout
	battle.call("_play_actor_anim", player, "down")
	var down_generation := int(battle.call("_painterly_semantic_generation", player))
	assert(down_generation > pre_down_generation, "A persistent down state must supersede an earlier transient lifecycle")
	assert(player.get_meta("semantic_state", "") == "down" and player.scale.y < float(player.get_meta("semantic_base_scale", Vector2.ONE).y), "Down state must be persistent and readable")
	await get_tree().create_timer(0.24).timeout
	assert(player.get_meta("semantic_state", "") == "down", "A stale Reduce Motion cleanup must not overwrite a newer persistent down state")
	OptionsMenu.settings["reduce_motion"] = false
	OptionsMenu.settings["clean_gameplay_visuals"] = true
	player_container.position = player_base + Vector2(7.0, -3.0)
	battle.call("_process", 0.5)
	assert(player_container.position.distance_to(player_base) < 0.01, "Clean Gameplay Visuals must retain the profile-based static actor hierarchy without idle drift")
	OptionsMenu.settings["clean_gameplay_visuals"] = false
	battle.call("_on_pre_attack", "Arrel", "Stage Contract Dummy", "Attack")
	assert(player.get_meta("semantic_state", "") == "attack", "Normal-motion player actions must retain their readable attack semantic")
	await get_tree().create_timer(0.60).timeout
	var normal_state := String(player.get_meta("semantic_state", ""))
	var normal_delta := player_container.position - player_base
	print("CINEMATIC_NORMAL_MECHANICS state=%s delta=(%.3f,%.3f)" % [normal_state, normal_delta.x, normal_delta.y])
	assert(normal_state == "idle", "Normal-motion mechanical return must settle the held painterly action at idle")
	assert(absf(normal_delta.x) < 0.01 and absf(normal_delta.y) <= 2.01, "Normal-mode idle breathing may offset only the authored vertical two-pixel amplitude after the mechanical return")

func _validate_explicit_fallback_contract(battle: Node) -> void:
	assert(battle.call("_make_painterly_battle_plate", "player", "res://missing_canonical_arrel.png") == null, "Missing canonical art must enter the explicit fallback path")
	var arrel_frames: SpriteFrames = PixelSprite.load_sheet_frames("arrel")
	if arrel_frames == null:
		arrel_frames = PixelSprite.create_battle_sprite_frames("arrel")
	assert(arrel_frames != null and arrel_frames.has_animation("idle") and arrel_frames.has_animation("attack") and arrel_frames.has_animation("cast") and arrel_frames.has_animation("hurt") and arrel_frames.has_animation("down"), "Arrel's animated fallback frames must remain available")
	var prior_reduce := bool(OptionsMenu.settings.get("reduce_motion", false))
	OptionsMenu.settings["reduce_motion"] = true
	var fallback := AnimatedSprite2D.new()
	fallback.sprite_frames = arrel_frames
	battle.add_child(fallback)
	# Registration owns live fallback initialization, so test the same path with
	# Reduce Motion enabled before any semantic verb is requested.
	battle.call("_register_battler_actor", fallback, "player", "animated_fallback")
	fallback.set_meta("canonical_fallback", true)
	assert(String(fallback.get_meta("semantic_state", "")) == "idle" and String(fallback.animation) == "idle" and not fallback.is_playing(), "Reduce Motion fallback registration must immediately select a static authored idle frame")
	for state in ["attack", "cast", "hurt"]:
		battle.call("_play_actor_anim", fallback, state)
		assert(String(fallback.get_meta("semantic_state", "")) == state and String(fallback.animation) == state, "Reduce Motion fallback %s must select a readable static semantic frame" % state)
		assert(not fallback.is_playing(), "Reduce Motion fallback %s must not run SpriteFrames animation" % state)
		assert(not (battle.get("_painterly_semantic_tweens") as Dictionary).has(fallback.get_instance_id()), "Reduce Motion fallback %s must not create a semantic tween" % state)
		await get_tree().create_timer(0.30).timeout
		assert(String(fallback.get_meta("semantic_state", "")) == "idle" and String(fallback.animation) == "idle" and not fallback.is_playing(), "Reduce Motion fallback %s must settle to a static idle frame" % state)
	OptionsMenu.settings["reduce_motion"] = false
	battle.call("_initialize_animated_battler_fallback", fallback)
	assert(String(fallback.get_meta("semantic_state", "")) == "idle" and String(fallback.animation) == "idle" and fallback.is_playing(), "The shared fallback initializer must resume normal looping idle from an already-static idle state")
	OptionsMenu.settings["reduce_motion"] = true
	battle.call("_initialize_animated_battler_fallback", fallback)
	assert(String(fallback.get_meta("semantic_state", "")) == "idle" and String(fallback.animation) == "idle" and not fallback.is_playing(), "The shared fallback initializer must return an idle fallback to static Reduce Motion state")
	battle.call("_play_actor_anim", fallback, "attack")
	var transient_generation := int(battle.call("_painterly_semantic_generation", fallback))
	await get_tree().create_timer(0.04).timeout
	battle.call("_play_actor_anim", fallback, "down")
	var down_generation := int(battle.call("_painterly_semantic_generation", fallback))
	assert(down_generation > transient_generation, "Fallback down must supersede the earlier static transient lifecycle")
	await get_tree().create_timer(0.24).timeout
	assert(String(fallback.get_meta("semantic_state", "")) == "down" and String(fallback.animation) == "down" and not fallback.is_playing(), "A stale fallback cleanup must not erase a newer persistent static down frame")
	OptionsMenu.settings["reduce_motion"] = false
	battle.call("_play_actor_anim", fallback, "attack")
	assert(String(fallback.animation) == "attack" and fallback.is_playing(), "Normal-motion fallback behavior must continue to play the requested SpriteFrames animation")
	fallback.stop()
	fallback.queue_free()
	OptionsMenu.settings["reduce_motion"] = prior_reduce

func _role_container(battle: Node, role: String) -> Control:
	return battle.get({"player": "player_sprite_container", "ally": "ally_sprite_container", "support": "tobias_sprite_container", "enemy": "enemy_sprite_container"}.get(role, "")) as Control

func _role_base_position(battle: Node, role: String) -> Vector2:
	var property_name: String = String({"player": "_player_base_pos", "ally": "_ally_base_pos", "support": "_tobias_base_pos", "enemy": "_enemy_base_pos"}.get(role, ""))
	return battle.get(String(property_name)) as Vector2

func _role_actor(battle: Node, role: String) -> CanvasItem:
	return battle.get({"player": "player_sprite", "ally": "ally_sprite", "support": "tobias_sprite", "enemy": "enemy_sprite"}.get(role, "")) as CanvasItem

func _role_anchor(battle: Node, role: String) -> Control:
	for candidate in battle.get("_battler_anchors") as Array:
		if candidate is Control and String((candidate as Control).get_meta("battle_role", "")) == role:
			return candidate as Control
	return null
