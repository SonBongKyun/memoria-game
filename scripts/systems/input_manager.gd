## InputManager (Autoload)
## S56: Controller support — input detection, button icons, vibration feedback.
## Tracks whether last input was keyboard or controller, provides icon hints.
extends Node

enum InputMode { KEYBOARD, CONTROLLER }

var current_mode: InputMode = InputMode.KEYBOARD

signal input_mode_changed(mode: InputMode)

# Controller button icon map (Xbox layout — most common on PC)
const GAMEPAD_ICONS: Dictionary = {
	"confirm": "A",
	"cancel": "B",
	"interact": "A",
	"menu": "Start",
	"memory_menu": "Select",
	"move_up": "D-Up",
	"move_down": "D-Down",
	"move_left": "D-Left",
	"move_right": "D-Right",
}

const KEYBOARD_ICONS: Dictionary = {
	"confirm": "Space",
	"cancel": "Esc",
	"interact": "Space/Enter",
	"menu": "Esc",
	"memory_menu": "Tab/M",
	"move_up": "W",
	"move_down": "S",
	"move_left": "A",
	"move_right": "D",
}

# Vibration presets
const VIBRATION_PRESETS: Dictionary = {
	"battle_hit": {"weak": 0.3, "strong": 0.5, "duration": 0.15},
	"battle_critical": {"weak": 0.5, "strong": 0.8, "duration": 0.25},
	"memory_burn": {"weak": 0.6, "strong": 0.9, "duration": 0.4},
	"boss_phase": {"weak": 0.4, "strong": 0.7, "duration": 0.5},
	"game_over": {"weak": 0.3, "strong": 0.6, "duration": 0.6},
	"ui_confirm": {"weak": 0.1, "strong": 0.0, "duration": 0.05},
	"heal": {"weak": 0.15, "strong": 0.0, "duration": 0.1},
}

func _ready() -> void:
	# Detect connected controllers at startup
	if Input.get_connected_joypads().size() > 0:
		print("[InputManager] Controller detected: %s" % Input.get_joy_name(0))
	print("[InputManager] Ready — current mode: KEYBOARD")

func _input(event: InputEvent) -> void:
	var new_mode = current_mode

	if event is InputEventKey or event is InputEventMouse or event is InputEventMouseButton or event is InputEventMouseMotion:
		new_mode = InputMode.KEYBOARD
	elif event is InputEventJoypadButton or event is InputEventJoypadMotion:
		if event is InputEventJoypadMotion:
			# Only switch on significant stick movement
			if abs(event.axis_value) > 0.5:
				new_mode = InputMode.CONTROLLER
		else:
			new_mode = InputMode.CONTROLLER

	if new_mode != current_mode:
		current_mode = new_mode
		input_mode_changed.emit(current_mode)
		print("[InputManager] Mode switched to: %s" % ("CONTROLLER" if current_mode == InputMode.CONTROLLER else "KEYBOARD"))

## Get button icon text for an action based on current input mode
func get_icon(action: String) -> String:
	if current_mode == InputMode.CONTROLLER:
		return GAMEPAD_ICONS.get(action, "?")
	return KEYBOARD_ICONS.get(action, "?")

## Get hint text like "[A] Confirm" or "[Space] Confirm"
func get_hint(action: String, label: String) -> String:
	return "[%s] %s" % [get_icon(action), label]

## Check if controller is connected
func is_controller_connected() -> bool:
	return Input.get_connected_joypads().size() > 0

## Check if currently using controller
func is_controller_mode() -> bool:
	return current_mode == InputMode.CONTROLLER

## Play controller vibration with named preset
func vibrate(preset_name: String) -> void:
	if current_mode != InputMode.CONTROLLER:
		return
	if not VIBRATION_PRESETS.has(preset_name):
		return
	var pads = Input.get_connected_joypads()
	if pads.is_empty():
		return
	var preset = VIBRATION_PRESETS[preset_name]
	Input.start_joy_vibration(pads[0], preset["weak"], preset["strong"], preset["duration"])

## Play custom vibration
func vibrate_custom(weak: float, strong: float, duration: float) -> void:
	if current_mode != InputMode.CONTROLLER:
		return
	var pads = Input.get_connected_joypads()
	if pads.is_empty():
		return
	Input.start_joy_vibration(pads[0], weak, strong, duration)

## Stop vibration
func stop_vibration() -> void:
	var pads = Input.get_connected_joypads()
	if pads.is_empty():
		return
	Input.stop_joy_vibration(pads[0])
