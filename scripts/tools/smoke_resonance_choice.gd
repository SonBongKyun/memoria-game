extends Node

func _ready() -> void:
	GameManager.change_state(GameManager.GameState.EXPLORATION)
	GameManager.story_flags.erase("smoke_resonance_choice")

	var map := Node2D.new()
	map.name = "ResonanceChoiceSmokeMap"
	add_child(map)
	var echo := Area2D.new()
	echo.name = "SmokeEcho"
	map.add_child(echo)

	var memory := MemoryManager.Memory.new(
		"smoke_resonance_choice",
		"Smoke-Test Memory",
		"A memory used only to validate the choice interface.",
		1,
		1,
		""
	)
	MemoryResonance._show_resonance_choice(echo, null, "smoke_resonance_choice", memory, "grains", 10, "+10 Grains")
	await get_tree().process_frame

	var layer := get_tree().root.get_node_or_null("MemoryResonanceChoice") as CanvasLayer
	assert(layer != null, "Entering a resonance must open the three-way choice layer")
	assert(GameManager.current_state == GameManager.GameState.DIALOGUE, "Resonance choice must freeze exploration input")
	assert(layer.get_node_or_null("ChoiceRoot") != null, "Choice layer must include the interactive UI root")

	MemoryResonance._resolve_resonance_choice("leave", layer, echo, null, "smoke_resonance_choice", memory, "grains", 10, "+10 Grains")
	await get_tree().process_frame

	assert(GameManager.current_state == GameManager.GameState.EXPLORATION, "Leaving an echo must restore exploration")
	assert(not GameManager.get_flag("smoke_resonance_choice"), "Leaving an echo must not consume its flag")
	assert(not bool(echo.get_meta("choice_open", true)), "Leaving an echo must make it available again")
	assert(echo.monitoring, "Leaving an echo must re-enable its trigger")

	echo.queue_free()
	map.queue_free()
	print("RESONANCE_CHOICE_SMOKE_PASS options=3 leave_persists=true")
	get_tree().quit(0)
