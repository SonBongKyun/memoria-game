## A visible field threat that telegraphs contact before it becomes a battle.
##
## Distance drives a shared pressure value on the player.  The way the player
## crosses the contact line is consumed by FieldFlow and carried into battle.
class_name FieldThreat
extends Area2D

const DETECTION_RADIUS: float = 132.0
const CONTACT_RADIUS: float = 24.0
const PHASE_BYPASS_GRAINS: int = 3

var map_id: String = ""
var hunt_data: Dictionary = {}
var defeated_flag: String = ""
var source_id: String = ""
var _player: Node2D = null
var _engaged: bool = false
var _time: float = 0.0
var _last_pressure: float = 0.0
var _sprite: Sprite2D = null
var _ring: Line2D = null
var _aura: Line2D = null
var _sight: Line2D = null
var _core: Polygon2D = null

# S236: 위협은 제자리에서 맥동만 했다. 사냥한다고 말하면서 한 발도 움직이지 않으면
# 지형지물과 구분되지 않는다. 자기 자리 주변을 천천히 돌고, 플레이어를 알아채면
# 그쪽으로 기운다. 접촉 판정 자체는 건드리지 않는다.
const PATROL_RADIUS: float = 26.0
const PATROL_SPEED: float = 0.55
const LEAN_DISTANCE: float = 14.0
var _anchor: Vector2 = Vector2.ZERO
var _patrol_phase: float = 0.0
var _drift: Vector2 = Vector2.ZERO
## 이번 프레임이 목표한 자리. 궤도 위상과 무관하게 "어디로 기울려 했는가"를 읽을 수 있다.
var _patrol_target: Vector2 = Vector2.ZERO


func configure(new_map_id: String, data: Dictionary, flag: String) -> void:
	map_id = new_map_id
	hunt_data = data.duplicate(true)
	defeated_flag = flag
	source_id = "visible:%s:%s" % [map_id, String(data.get("id", name))]


func _ready() -> void:
	add_to_group("field_threat")
	monitoring = true
	monitorable = true
	_sprite = get_node_or_null("ThreatSprite") as Sprite2D
	_ring = get_node_or_null("ThreatRing") as Line2D
	_aura = get_node_or_null("ThreatAura") as Line2D
	_sight = get_node_or_null("ThreatSight") as Line2D
	_core = get_node_or_null("ThreatCore") as Polygon2D
	body_entered.connect(_on_body_entered)
	# 같은 자리에서 태어난 위협들이 한 몸처럼 움직이지 않도록 위상을 흩는다.
	_anchor = position
	_patrol_phase = randf() * TAU
	set_process(true)


func _process(delta: float) -> void:
	_time += delta
	if _engaged or GameManager.current_state != GameManager.GameState.EXPLORATION:
		_clear_player_pressure()
		return
	if _player == null or not is_instance_valid(_player):
		_player = get_tree().get_first_node_in_group("player") as Node2D
	if _player == null:
		return

	var distance := global_position.distance_to(_player.global_position)
	var pressure := clampf(1.0 - (distance - CONTACT_RADIUS) / (DETECTION_RADIUS - CONTACT_RADIUS), 0.0, 1.0)
	_last_pressure = pressure
	_update_patrol(delta, pressure)
	if _player.has_method("set_field_threat_source"):
		_player.call("set_field_threat_source", source_id, pressure)
	_update_visuals(delta, distance, pressure)


## 느린 8자 궤도 + 플레이어 쪽으로의 기울기.
## 압박이 오를수록 궤도는 좁아지고 기울기는 커진다. 노려보는 것처럼 읽힌다.
func _update_patrol(delta: float, pressure: float) -> void:
	_patrol_phase += delta * PATROL_SPEED * (1.0 + pressure * 0.9)
	var orbit_radius := PATROL_RADIUS * (1.0 - pressure * 0.55)
	var orbit := Vector2(
		sin(_patrol_phase) * orbit_radius,
		sin(_patrol_phase * 2.0) * orbit_radius * 0.42
	)
	var lean := Vector2.ZERO
	if _player != null and is_instance_valid(_player):
		var toward := _player.global_position - (_anchor + global_position - position)
		if toward.length() > 0.001:
			lean = toward.normalized() * LEAN_DISTANCE * pressure
	# 목표로 곧장 튀지 않고 미끄러지듯 따라간다. 위협은 서두르지 않는다.
	_patrol_target = orbit + lean
	_drift = _drift.lerp(_patrol_target, 1.0 - exp(-2.4 * delta))
	position = _anchor + _drift

func _update_visuals(delta: float, distance: float, pressure: float) -> void:
	var pulse := 0.5 + sin(_time * (2.8 + pressure * 3.4)) * 0.5
	if _sprite:
		_sprite.position.y = -29.0 - sin(_time * 2.25) * (1.2 + pressure * 1.8)
		var lean_target := 0.0
		if _player and distance < DETECTION_RADIUS:
			lean_target = clampf((_player.global_position.x - global_position.x) / DETECTION_RADIUS, -1.0, 1.0) * 0.055
		_sprite.rotation = lerp_angle(_sprite.rotation, lean_target, 1.0 - exp(-5.0 * delta))
		_sprite.modulate = Color(1.0 + pressure * 0.15, 1.0 - pressure * 0.18, 1.0 - pressure * 0.10, 1.0)
	if _ring:
		_ring.modulate.a = 0.28 + pressure * 0.68
		_ring.width = 1.0 + pressure * 1.35
		_ring.scale = Vector2.ONE * (1.0 + pulse * (0.035 + pressure * 0.07))
	if _aura:
		_aura.modulate.a = 0.10 + pressure * 0.52 + pulse * 0.10
		_aura.width = 0.8 + pressure * 1.1
		_aura.scale = Vector2.ONE * (0.90 + pulse * 0.08 + pressure * 0.18)
	if _core:
		_core.modulate.a = 0.18 + pressure * 0.68 + pulse * 0.12
		_core.scale = Vector2.ONE * (0.82 + pulse * 0.24 + pressure * 0.18)
	if _sight and _player:
		var target := to_local(_player.global_position)
		var visible_amount := clampf((pressure - 0.18) / 0.62, 0.0, 1.0)
		_sight.points = PackedVector2Array([Vector2(0, -15), target])
		_sight.modulate.a = visible_amount * (0.28 + pulse * 0.24)
		_sight.width = 0.7 + pressure * 1.4


func _on_body_entered(body: Node2D) -> void:
	if _engaged or body == null or not body.is_in_group("player"):
		return
	if GameManager.current_state != GameManager.GameState.EXPLORATION:
		return
	if defeated_flag != "" and GameManager.get_flag(defeated_flag):
		return
	_player = body
	var contact_pressure := maxf(
		_last_pressure,
		clampf(1.0 - (global_position.distance_to(body.global_position) - CONTACT_RADIUS) / (DETECTION_RADIUS - CONTACT_RADIUS), 0.0, 1.0)
	)
	if contact_pressure >= 0.7 and body.has_method("is_field_dashing") and bool(body.call("is_field_dashing")):
		_resolve_phase_bypass()
		return
	_engaged = true
	if body.has_method("prepare_field_entry_for_battle"):
		body.call("prepare_field_entry_for_battle", "visible")
	_clear_player_pressure()
	if defeated_flag != "":
		GameManager.set_flag(defeated_flag)

	var enemy := BattleManager.Enemy.new(
		String(hunt_data.get("name", "Memory Echo")),
		int(hunt_data.get("hp", 70)),
		int(hunt_data.get("atk", 14)),
		bool(hunt_data.get("is_void", false))
	)
	enemy.abilities = Array(hunt_data.get("abilities", [])).duplicate()
	enemy.weakness = String(hunt_data.get("weakness", ""))
	enemy.resistance = String(hunt_data.get("resistance", ""))
	var battle_art := String(hunt_data.get("battle_art", hunt_data.get("_resolved_field_art", hunt_data.get("art", ""))))
	BattleManager.start_battle(enemy, "res://scenes/maps/%s.tscn" % map_id, "", battle_art)
	SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
	call_deferred("queue_free")


func _resolve_phase_bypass() -> void:
	_engaged = true
	monitoring = false
	_clear_player_pressure()
	if defeated_flag != "":
		GameManager.set_flag(defeated_flag)
	GameManager.player_data["grains"] = int(GameManager.player_data.get("grains", 0)) + PHASE_BYPASS_GRAINS
	# S263: 여정 맹세 '증인의 맹세' — 목격한 위협을 건너뛰면 깨진다.
	JourneyOath.on_threat_bypassed()
	# S226: Part of the step returns, so the escape leaves the player able to act.
	var flow_back := 0
	if _player and is_instance_valid(_player) and _player.has_method("reward_field_bypass"):
		_player.call("reward_field_bypass")
		flow_back = int(round(FieldFlow.PHASE_STEP_COST * 0.5))
	AudioManager.play_sfx("flee")
	var threat_name := String(hunt_data.get("name", "Memory Echo"))
	var message := "PHASE SKIM: %s bypassed. +%d Grains, Flow +%d" % [threat_name, PHASE_BYPASS_GRAINS, flow_back]
	if GameManager.current_locale == "ko":
		message = "위상 스침: %s 회피. +%d 그레인, 흐름 +%d" % [threat_name, PHASE_BYPASS_GRAINS, flow_back]
	NotificationToast.show_toast(message, NotificationToast.ToastType.SUCCESS)
	call_deferred("queue_free")


func _clear_player_pressure() -> void:
	if _player and is_instance_valid(_player) and _player.has_method("clear_field_threat_source"):
		_player.call("clear_field_threat_source", source_id)


func _exit_tree() -> void:
	_clear_player_pressure()
