## FieldFlow turns exploration movement into battle preparation.
##
## Walking and sprinting build Flow.  The player can spend it on a Phase Step,
## or carry the way they approached a threat into the opening turn:
## - AMBUSH: cross the contact line during a Phase Step / at high Flow.
## - GUARDED: hold composure while encounter pressure closes in.
## - WITNESS: answer the warning with Memory Pulse.
class_name FieldFlow
extends Node

signal flow_changed(value: float, maximum: float)
signal pressure_changed(value: float)
signal phase_step_started(direction: Vector2)
signal phase_step_ended()

const FLOW_MAX: float = 100.0
const STARTING_FLOW: float = 24.0
const PHASE_STEP_COST: float = 42.0
const GUARDED_ENTRY_COST: float = 28.0
const WITNESS_ENTRY_COST: float = 24.0
const PHASE_STEP_DURATION: float = 0.22
const PHASE_STEP_COOLDOWN: float = 0.54
const PHASE_STEP_SPEED_MULTIPLIER: float = 2.45

var flow: float = STARTING_FLOW
var dash_direction: Vector2 = Vector2.DOWN
var _dash_timer: float = 0.0
var _dash_cooldown: float = 0.0
var _witness_window: float = 0.0
var _threat_sources: Dictionary = {}
var _last_emitted_flow: float = -100.0
var _last_emitted_pressure: float = -100.0


func _ready() -> void:
	_emit_if_changed(true)


func update_motion(
		delta: float,
		moved_distance: float,
		moving: bool,
		sprinting: bool,
		actual_speed: float
	) -> void:
	if _dash_timer > 0.0:
		_dash_timer = maxf(0.0, _dash_timer - delta)
		if _dash_timer <= 0.0:
			phase_step_ended.emit()
	if _dash_cooldown > 0.0:
		_dash_cooldown = maxf(0.0, _dash_cooldown - delta)
	if _witness_window > 0.0:
		_witness_window = maxf(0.0, _witness_window - delta)

	if GameManager.current_state == GameManager.GameState.EXPLORATION and not is_dashing():
		var pressure := get_pressure()
		if moving:
			var distance_gain := moved_distance * (0.11 if sprinting else 0.058)
			var danger_gain := pressure * delta * 8.0
			var speed_gain := clampf(actual_speed / 220.0, 0.0, 1.0) * delta * 2.0
			_set_flow(flow + distance_gain + danger_gain + speed_gain)
		else:
			var decay := (3.4 if pressure < 0.35 else 1.2) * delta
			_set_flow(flow - decay)
	_emit_if_changed()


func try_phase_step(direction: Vector2) -> bool:
	if GameManager.current_state != GameManager.GameState.EXPLORATION:
		return false
	if _dash_timer > 0.0 or _dash_cooldown > 0.0:
		return false
	if flow < PHASE_STEP_COST or direction.length_squared() < 0.01:
		return false
	dash_direction = direction.normalized()
	_set_flow(flow - PHASE_STEP_COST)
	_dash_timer = PHASE_STEP_DURATION
	_dash_cooldown = PHASE_STEP_COOLDOWN
	phase_step_started.emit(dash_direction)
	_emit_if_changed(true)
	return true


func open_witness_window() -> void:
	_witness_window = 1.05
	_emit_if_changed(true)


func is_dashing() -> bool:
	return _dash_timer > 0.0


func get_speed_multiplier() -> float:
	return PHASE_STEP_SPEED_MULTIPLIER if is_dashing() else 1.0


func get_dash_ratio() -> float:
	return clampf(_dash_timer / PHASE_STEP_DURATION, 0.0, 1.0)


func set_threat_source(source_id: String, pressure: float) -> void:
	if source_id == "":
		return
	var clamped := clampf(pressure, 0.0, 1.0)
	if clamped <= 0.001:
		_threat_sources.erase(source_id)
	else:
		_threat_sources[source_id] = clamped
	_emit_if_changed()


func clear_threat_source(source_id: String) -> void:
	_threat_sources.erase(source_id)
	_emit_if_changed(true)


func get_pressure() -> float:
	var strongest := 0.0
	for value in _threat_sources.values():
		strongest = maxf(strongest, float(value))
	return strongest


func get_forecast_mode() -> String:
	if is_dashing():
		return "ambush"
	if _witness_window > 0.0:
		return "witness"
	var pressure := get_pressure()
	if pressure >= 0.18 and flow >= 75.0 and _has_visible_threat():
		return "ambush_ready"
	if pressure >= 0.45 and flow >= GUARDED_ENTRY_COST:
		return "guarded"
	if flow >= 75.0:
		return "ambush_ready"
	return "neutral"


func consume_battle_entry(source: String = "ambient") -> Dictionary:
	var pressure := get_pressure()
	var mode := "neutral"
	if is_dashing():
		mode = "ambush"
	elif _witness_window > 0.0 and flow >= WITNESS_ENTRY_COST:
		mode = "witness"
	elif source == "visible" and flow >= 75.0:
		mode = "ambush"
	elif pressure >= 0.45 and flow >= GUARDED_ENTRY_COST:
		mode = "guarded"

	var power := int(round(flow))
	match mode:
		"guarded":
			_set_flow(flow - GUARDED_ENTRY_COST)
		"witness":
			_set_flow(flow - WITNESS_ENTRY_COST)
		"ambush":
			# A live Phase Step has already paid its cost.  A high-flow silent
			# approach spends part of the bank here.
			if not is_dashing():
				_set_flow(flow - 32.0)
	_witness_window = 0.0
	_threat_sources.clear()
	_emit_if_changed(true)
	return {
		"mode": mode,
		"power": power,
		"pressure": pressure,
		"source": source,
	}


func get_status() -> Dictionary:
	return {
		"flow": flow,
		"maximum": FLOW_MAX,
		"pressure": get_pressure(),
		"mode": get_forecast_mode(),
		"dashing": is_dashing(),
		"dash_ready": flow >= PHASE_STEP_COST and _dash_cooldown <= 0.0,
		"dash_cost": PHASE_STEP_COST,
		"dash_cooldown": _dash_cooldown,
		"witness_window": _witness_window,
	}


func _set_flow(value: float) -> void:
	flow = clampf(value, 0.0, FLOW_MAX)

func _has_visible_threat() -> bool:
	for source in _threat_sources:
		if String(source).begins_with("visible:"):
			return true
	return false


func _emit_if_changed(force: bool = false) -> void:
	var pressure := get_pressure()
	if force or absf(flow - _last_emitted_flow) >= 0.35:
		_last_emitted_flow = flow
		flow_changed.emit(flow, FLOW_MAX)
	if force or absf(pressure - _last_emitted_pressure) >= 0.015:
		_last_emitted_pressure = pressure
		pressure_changed.emit(pressure)
