## Main Scene — 임시 테스트 씬
## Godot 실행 시 첫 화면. 시스템 확인용.
extends Node2D

func _ready() -> void:
	print("=== MEMORIA: The Price of Oblivion ===")
	print("Version: 0.1.0")
	print("")
	print("Controls:")
	print("  SPACE — Test dialogue")
	print("  M — List memories")
	print("  B — Test burn (first available)")
	print("  ESC — Quit")
	print("======================================")

func _unhandled_key_input(event: InputEvent) -> void:
	if not event.is_pressed() or event.is_echo():
		return

	var key_event = event as InputEventKey
	if key_event == null:
		return

	# SPACE — 대화 테스트
	if key_event.physical_keycode == KEY_SPACE:
		print("[INPUT] SPACE pressed — testing dialogue")
		_test_dialogue()

	# M — 기억 목록
	if key_event.physical_keycode == KEY_M:
		print("[INPUT] M pressed — listing memories")
		_list_memories()

	# B — 기억 연소 테스트
	if key_event.physical_keycode == KEY_B:
		print("[INPUT] B pressed — testing burn")
		_test_burn()

	# ESC — 종료
	if key_event.physical_keycode == KEY_ESCAPE:
		get_tree().quit()

func _test_dialogue() -> void:
	var test_dialogue = [
		{"speaker": "Arrel", "text": "The beast was dead.", "portrait": "arrel_neutral"},
		{"speaker": "", "text": "His skull throbbed. Not the clean pain of a headache but a deeper pressure, as if someone had scooped out a piece of his brain and left the cavity unsealed."},
		{"speaker": "Elia", "text": "How bad?", "portrait": "elia_concern"},
		{"speaker": "Arrel", "text": "One burn. Grade three, maybe. Nothing I'll miss.", "portrait": "arrel_neutral"},
		{"speaker": "", "text": "That was probably true. That was probably a lie. The problem with burning memories was that you could never be sure which it was."},
	]

	print("\n--- DIALOGUE START ---")
	for line in test_dialogue:
		if line.speaker != "":
			print("  [%s]: %s" % [line.speaker, line.text])
		else:
			print("  (Narration): %s" % line.text)
	print("--- DIALOGUE END ---\n")

func _list_memories() -> void:
	print("\n--- ARREL'S MEMORY ARCHIVE ---")
	var grade_names = ["Grade 5 (Sensory)", "Grade 4 (Daily)", "Grade 3 (Relational)", "Grade 2 (Identity)", "Grade 1 (Core)"]
	for memory in MemoryManager.memories:
		var status = "BURNED" if memory.is_burned else ("RESIDUE" if memory.is_residue else "INTACT")
		print("  [%s] %s — %s | Power: %d" % [status, memory.title, grade_names[memory.grade], memory.burn_power])
	print("Burn ratio: %.1f%%" % (MemoryManager.get_burn_ratio() * 100))
	print("------------------------------\n")

func _test_burn() -> void:
	var available = MemoryManager.get_available_memories()
	if available.size() > 0:
		var memory = available[0]
		print("\n[BURN] Attempting to burn: %s" % memory.title)
		MemoryManager.burn_memory(memory.id)
		print("[BURN] Burn ratio now: %.1f%%" % (MemoryManager.get_burn_ratio() * 100))
	else:
		print("[BURN] No memories left to burn.")
