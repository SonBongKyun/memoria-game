## WorldRewriteDirector (Autoload)
## Turns memory loss into visible scene-level consequences.
extends Node

signal rewrite_manifested(memory, report: Dictionary)

const MEMORY_REWRITE_RULES := {
	"daily_market_food": {
		"flag": "world_rewrite_verdan_taste_blurred",
		"title": "Verdan Taste Blurred",
		"line": "A vendor's face slips out of every market smell.",
		"title_ko": "베르단 미각 흐려짐",
		"line_ko": "시장의 모든 냄새에서 상인의 얼굴이 빠져나간다.",
		"compass": "Verdan taste map erased.",
		"color": Color(0.95, 0.62, 0.30),
		"art": "res://assets/cg/generated/illustration_expansion_v3/world_rewrite_verdan_taste_v3.png"
	},
	"daily_campfire_song": {
		"flag": "world_rewrite_elia_hum_unmoored",
		"title": "Campfire Song Unmoored",
		"line": "Elia's humming still exists, but it no longer knows where to land.",
		"title_ko": "모닥불 노래 표류",
		"line_ko": "엘리아의 흥얼거림은 남았지만, 이제 내려앉을 곳을 모른다.",
		"compass": "Anchor melody destabilized.",
		"color": Color(0.68, 0.74, 0.92),
		"art": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_campfire_song_v3.png"
	},
	"rel_hand_reaching": {
		"flag": "world_rewrite_reaching_hand_absent",
		"title": "The Reaching Hand Removed",
		"line": "When someone reaches for him, the world hesitates before drawing the hand.",
		"title_ko": "뻗던 손 삭제",
		"line_ko": "누군가 손을 뻗으면, 세계가 그 손을 그리기 전에 잠시 망설인다.",
		"compass": "Relationship contour torn.",
		"color": Color(0.80, 0.66, 0.94),
		"art": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_reaching_hand_v3.png"
	},
	"identity_first_sword": {
		"flag": "world_rewrite_first_sword_cut",
		"title": "First Sword Excised",
		"line": "The body remembers the stance. The self no longer remembers why.",
		"title_ko": "첫 검 절제",
		"line_ko": "몸은 자세를 기억한다. 자신은 그 이유를 더 기억하지 못한다.",
		"compass": "Identity architecture fractured.",
		"color": Color(0.95, 0.38, 0.26),
		"art": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_first_sword_v3.png"
	},
	"core_name_origin": {
		"flag": "world_rewrite_name_origin_void",
		"title": "Name Origin Consumed",
		"line": "The sound 'Arrel' keeps its letters and loses its owner.",
		"title_ko": "이름의 기원 소각",
		"line_ko": "'아렐'이라는 소리는 글자를 지키고 주인을 잃는다.",
		"compass": "Name-bearing thread severed.",
		"color": Color(1.00, 0.28, 0.22),
		"art": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png"
	},
	"rel_tobias_records": {
		"flag": "world_rewrite_record_ink_fades",
		"title": "Record Ink Fades",
		"line": "Tobias can still write the facts. The meaning dries before the ink does.",
		"title_ko": "기록의 잉크 바램",
		"line_ko": "토비아스는 사실을 적을 수 있다. 다만 잉크보다 의미가 먼저 마른다.",
		"compass": "Record-tree contour retained.",
		"color": Color(0.75, 0.62, 0.48),
		"art": "res://assets/cg/generated/illustration_expansion_v3/world_rewrite_tobias_ink_v3.png"
	},
	"daily_elia_hands": {
		"flag": "world_rewrite_elia_anchor_thinned",
		"title": "Anchor Warmth Thinned",
		"line": "Warm hands remain in the scene like heat after a body has left.",
		"title_ko": "앵커의 온기 희박",
		"line_ko": "따뜻한 손은 몸이 떠난 자리에 남은 열처럼 장면에 머문다.",
		"compass": "Elia anchor pressure reduced.",
		"color": Color(0.72, 0.78, 0.96),
		"art": "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_elia_anchor_v3.png"
	},
	"rel_sable_voidwalk": {
		"flag": "world_rewrite_sable_witness_dimmed",
		"title": "Witness Dimmed",
		"line": "Sable's certainty loses one scar's worth of weight.",
		"title_ko": "증인 흐려짐",
		"line_ko": "세이블의 확신에서 흉터 하나만큼의 무게가 빠진다.",
		"compass": "Void-walker witness weakened.",
		"color": Color(0.62, 0.54, 0.72),
		"art": "res://assets/cg/generated/illustration_expansion_v3/world_rewrite_sable_witness_v3.png"
	},
	"sense_forest_smell": {
		"flag": "world_rewrite_forest_scent_thinned",
		"title": "Forest Scent Thinned",
		"line": "The rain still darkens the roots, but the trail no longer has a smell to lead him home.",
		"title_ko": "숲 냄새 희박",
		"line_ko": "비는 여전히 뿌리를 적시지만, 그 길에는 그를 집으로 이끌 냄새가 없다.",
		"compass": "Rim weather contour unmoored.",
		"color": Color(0.52, 0.70, 0.88),
		"art": "res://assets/cg/generated/memory_rewrite_forest_scent_v2.png"
	},
	"rel_ghost_words": {
		"flag": "world_rewrite_ghost_words_silenced",
		"title": "Ghost Words Silenced",
		"line": "The hollow keeps its shape, but the unfinished voice has lost the way back to him.",
		"title_ko": "유령의 말 소거",
		"line_ko": "공동은 형태를 지켰지만, 끝맺지 못한 목소리가 그에게 돌아올 길을 잃었다.",
		"compass": "Forest witness contour hollowed.",
		"color": Color(0.74, 0.64, 0.86),
		"art": "res://assets/cg/generated/memory_rewrite_ghost_words_v2.png"
	},
	"identity_compass": {
		"flag": "world_rewrite_compass_unmapped",
		"title": "Compass Identity Unmapped",
		"line": "The pull remains beneath his sternum, but the direction now feels like a question asked in someone else's voice.",
		"title_ko": "나침반 정체성 미상",
		"line_ko": "가슴뼈 아래의 이끌림은 남았지만, 그 방향이 이제 남의 목소리로 던진 질문처럼 느껴진다.",
		"compass": "Colorless route no longer confirms north.",
		"color": Color(0.56, 0.78, 0.96),
		"art": "res://assets/cg/generated/illustration_expansion_v3/world_rewrite_compass_unmapped_v3.png"
	},
	"identity_void_walker": {
		"flag": "world_rewrite_void_walker_blurred",
		"title": "Void Walker Blurred",
		"line": "One footprint survives the Void's pressure, but he no longer remembers why he knew to place it there.",
		"title_ko": "보이드 보행자 흐려짐",
		"line_ko": "발자국 하나는 보이드의 압력을 견뎠지만, 왜 거기에 두어야 했는지는 남지 않았다.",
		"compass": "BL-07 passage contour weakened.",
		"color": Color(0.62, 0.48, 0.90),
		"art": "res://assets/cg/generated/illustration_expansion_v3/world_rewrite_void_walker_v3.png"
	},
}

## S226: What the player meets again after the loss, on the route they are
## already walking.  Each entry is keyed by the map the absence shows up in, so
## an early burn is answered a second time inside the same chapter instead of
## only in the archive.
const MEMORY_REVISIT_LINES := {
	"daily_campfire_song": {
		"rim_forest": {
			"en": "Elia hums at the fire. The tune arrives, but nothing in him leans toward it.",
			"ko": "엘리아가 불가에서 흥얼거린다. 가락은 닿는데, 그의 안에서 기울어지는 것이 없다.",
		},
		"verdan_market": {
			"en": "Someone whistles between the stalls. It is not the song. It could not be.",
			"ko": "가판 사이로 누군가 휘파람을 분다. 그 노래는 아니다. 그럴 수가 없다.",
		},
	},
	"sense_forest_smell": {
		"rim_forest": {
			"en": "Rain has soaked the roots again. The forest smells like nothing that leads home.",
			"ko": "비가 다시 뿌리를 적셨다. 숲에서는 집으로 이어지는 냄새가 나지 않는다.",
		},
	},
	"daily_market_food": {
		"verdan_market": {
			"en": "The spice smoke reaches him first, and stops. No vendor's face comes with it.",
			"ko": "향신료 연기가 먼저 닿고, 거기서 멈춘다. 함께 따라오던 상인의 얼굴이 없다.",
		},
	},
	"identity_first_sword": {
		"rim_forest": {
			"en": "His grip settles on its own. He no longer knows whose hand taught it that.",
			"ko": "손아귀가 저절로 자리를 잡는다. 누구의 손이 그렇게 가르쳤는지는 남아 있지 않다.",
		},
		"verdan_market": {
			"en": "A guard's drill rhythm passes by. His body answers it. His memory does not.",
			"ko": "경비병의 훈련 박자가 지나간다. 몸은 답하고, 기억은 답하지 않는다.",
		},
	},
	"rel_hand_reaching": {
		"rim_forest": {
			"en": "Elia stumbles beside him. His hand stays exactly where it was.",
			"ko": "엘리아가 옆에서 휘청인다. 그의 손은 있던 자리에 그대로 있다.",
		},
	},
}

const DEFAULT_LINES := {
	MemoryManager.MemoryGrade.GRADE_5: "A small sensation leaves the weather of the room.",
	MemoryManager.MemoryGrade.GRADE_4: "A daily habit disappears, and the world quietly edits around it.",
	MemoryManager.MemoryGrade.GRADE_3: "A relationship contour collapses into afterimage.",
	MemoryManager.MemoryGrade.GRADE_2: "A piece of identity breaks alignment with the body.",
	MemoryManager.MemoryGrade.GRADE_1: "The core thread burns white. Reality pauses before continuing.",
}

## S226: 연소 미리보기와 승리 화면이 이 문장을 정면에 세우므로, 한국어 플레이에서도
## 대가가 영어로 남지 않아야 한다.
const DEFAULT_LINES_KO := {
	MemoryManager.MemoryGrade.GRADE_5: "작은 감각 하나가 방의 공기에서 빠져나간다.",
	MemoryManager.MemoryGrade.GRADE_4: "일상의 습관이 사라지고, 세계가 조용히 그 주위를 고쳐 쓴다.",
	MemoryManager.MemoryGrade.GRADE_3: "관계의 윤곽이 잔상으로 무너진다.",
	MemoryManager.MemoryGrade.GRADE_2: "정체성의 한 조각이 몸과 어긋난다.",
	MemoryManager.MemoryGrade.GRADE_1: "핵심 가닥이 하얗게 탄다. 현실이 잠시 멈췄다가 이어진다.",
}

const AFTERGLOW_SHADER_PATH := "res://assets/shaders/desaturation.gdshader"
const AFTERGLOW_GROUP := "memory_absence_afterglow"

var _last_scene_path := ""
var _scene_residue_cooldown := 0.0
var _afterglow_layer: CanvasLayer = null
var _afterglow_tween: Tween = null

func _ready() -> void:
	if MemoryManager and MemoryManager.has_signal("memory_burned"):
		MemoryManager.memory_burned.connect(_on_memory_burned)
	if MemoryManager and MemoryManager.has_signal("memory_faded"):
		MemoryManager.memory_faded.connect(_on_memory_faded)
	set_process(true)
	print("[WorldRewriteDirector] Ready")

func _process(delta: float) -> void:
	_scene_residue_cooldown = maxf(_scene_residue_cooldown - delta, 0.0)
	var scene := get_tree().current_scene
	if scene == null:
		return
	var path := scene.scene_file_path
	if path != _last_scene_path:
		_last_scene_path = path
		if _scene_residue_cooldown <= 0.0:
			_scene_residue_cooldown = 1.2
			call_deferred("_manifest_scene_residue")

func _on_memory_burned(memory) -> void:
	var report := _build_report(memory, false)
	_apply_story_flags(memory, report)
	call_deferred("_manifest_rewrite", memory, report)
	rewrite_manifested.emit(memory, report)

func _on_memory_faded(memory) -> void:
	var report := _build_report(memory, true)
	call_deferred("_manifest_rewrite", memory, report)
	rewrite_manifested.emit(memory, report)

func _build_report(memory, faded: bool) -> Dictionary:
	var id := String(memory.id) if memory != null else ""
	var grade := int(memory.grade) if memory != null else MemoryManager.MemoryGrade.GRADE_5
	var rule: Dictionary = MEMORY_REWRITE_RULES.get(id, {})
	var is_ko := GameManager.current_locale == "ko"
	var title := String(rule.get("title", "Uncatalogued Absence"))
	var line := String(rule.get("line", DEFAULT_LINES.get(grade, "A contour vanished.")))
	if is_ko:
		title = String(rule.get("title_ko", "기록되지 않은 공백"))
		line = String(rule.get("line_ko", DEFAULT_LINES_KO.get(grade, "윤곽 하나가 사라졌다.")))
	var compass := String(rule.get("compass", _default_compass_line(memory, faded)))
	var color: Color = rule.get("color", _grade_color(grade))
	if faded:
		title = ("바래는 중: " if is_ko else "Fading: ") + title
		line = ("타기도 전에 얇아진다: " if is_ko else "Before it burns, it thins: ") + line
		compass = ("Erosion warning: ") + compass
	return {
		"id": id,
		"flag": String(rule.get("flag", "world_rewrite_" + id)),
		"title": title,
		"line": line,
		"compass": compass,
		"color": color,
		"grade": grade,
		"faded": faded,
		"memory_title": MemoryManager.localized_memory_title(memory) if memory != null else "Unknown Memory",
		"art": String(rule.get("art", _fallback_art_for_grade(grade))),
	}

func get_loss_records() -> Array[Dictionary]:
	var records: Array[Dictionary] = []
	if not MemoryManager:
		return records
	for memory in MemoryManager.memories:
		if memory.is_burned:
			records.append(_build_loss_record(memory, false))
		elif memory.is_faded:
			records.append(_build_loss_record(memory, true))
	return records

func get_rewrite_report(memory_id: String) -> Dictionary:
	if not MemoryManager:
		return {}
	for memory in MemoryManager.memories:
		if memory.id == memory_id:
			return _build_report(memory, memory.is_faded and not memory.is_burned)
	return {}

func _build_loss_record(memory, faded: bool) -> Dictionary:
	var report := _build_report(memory, faded)
	var status := "FADING" if faded else "BURNED"
	var grade_name := _grade_name(int(report.grade))
	var body := "%s\n\nMemory: %s\nGrade: %s\n\nWorld consequence:\n%s\n\nCompass reading:\n%s\n\nStory hook: %s" % [
		status,
		String(report.memory_title),
		grade_name,
		String(report.line),
		String(report.compass),
		String(report.flag),
	]
	return {
		"title": "%s - %s" % [status, String(report.memory_title)],
		"body": body,
		"color": report.color,
		"grade": int(report.grade),
		"faded": faded,
		"art": String(report.art),
	}

func _default_compass_line(memory, faded: bool) -> String:
	if memory == null:
		return "Unmapped contour missing."
	var prefix := "Eroding" if faded else "Lost"
	return "%s: %s" % [prefix, String(memory.title)]

func _apply_story_flags(memory, report: Dictionary) -> void:
	if memory == null:
		return
	GameManager.set_flag(String(report.flag), true)
	GameManager.set_flag("world_forgot_" + String(memory.id), true)
	if int(memory.grade) >= MemoryManager.MemoryGrade.GRADE_2:
		GameManager.set_flag("identity_rewrite_active", true)

func _manifest_rewrite(memory, report: Dictionary) -> void:
	var scene := get_tree().current_scene
	if scene == null:
		return
	PerceptionFilter.apply(scene)
	_show_rewrite_art(report)
	_spawn_echo_cluster(scene, report, _find_manifest_origin(scene), memory)
	play_absence_afterglow(int(report.get("grade", MemoryManager.MemoryGrade.GRADE_5)))
	if NotificationToast:
		var prefix := "세계 재기록" if GameManager.current_locale == "ko" else "World rewrite"
		NotificationToast.show_toast("%s: %s" % [prefix, String(report.title)], NotificationToast.ToastType.WARNING)

## S226: The seconds right after a loss.  The world keeps running and the player
## keeps control, but the colour does not come back immediately.
func play_absence_afterglow(grade: int) -> void:
	_clear_absence_afterglow()
	var shader := load(AFTERGLOW_SHADER_PATH) if ResourceLoader.exists(AFTERGLOW_SHADER_PATH) else null
	if shader == null:
		return
	var layer := CanvasLayer.new()
	layer.name = "MemoryAbsenceAfterglow"
	layer.add_to_group(AFTERGLOW_GROUP)
	layer.layer = 7
	var rect := ColorRect.new()
	rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rect.color = Color(1, 1, 1, 1)
	var material := ShaderMaterial.new()
	material.shader = shader
	material.set_shader_parameter("desaturation", 0.0)
	material.set_shader_parameter("tint_color", Color(0.13, 0.11, 0.17, 1.0))
	material.set_shader_parameter("tint_strength", 0.0)
	rect.material = material
	layer.add_child(rect)
	add_child(layer)
	_afterglow_layer = layer

	var peak := clampf(0.34 + float(grade) * 0.07, 0.34, 0.62)
	var hold := 4.2 + float(grade) * 0.9  # 4.2s for a sensation, ~8s for a core memory
	_afterglow_tween = create_tween().bind_node(layer)
	_afterglow_tween.tween_method(func(value: float): material.set_shader_parameter("desaturation", value), 0.0, peak, 0.45)
	_afterglow_tween.parallel().tween_method(func(value: float): material.set_shader_parameter("tint_strength", value), 0.0, peak * 0.22, 0.45)
	_afterglow_tween.tween_interval(hold)
	_afterglow_tween.tween_method(func(value: float): material.set_shader_parameter("desaturation", value), peak, 0.0, 1.6)
	_afterglow_tween.parallel().tween_method(func(value: float): material.set_shader_parameter("tint_strength", value), peak * 0.22, 0.0, 1.6)
	_afterglow_tween.tween_callback(func():
		if is_instance_valid(layer):
			layer.queue_free()
		if _afterglow_layer == layer:
			_afterglow_layer = null
			_afterglow_tween = null
	)

func _clear_absence_afterglow() -> void:
	if _afterglow_tween != null and _afterglow_tween.is_valid():
		_afterglow_tween.kill()
	_afterglow_tween = null
	if _afterglow_layer != null and is_instance_valid(_afterglow_layer):
		var parent := _afterglow_layer.get_parent()
		if parent != null:
			parent.remove_child(_afterglow_layer)
		_afterglow_layer.queue_free()
	_afterglow_layer = null

func get_active_afterglow_rect() -> ColorRect:
	if _afterglow_layer == null or not is_instance_valid(_afterglow_layer):
		return null
	return _afterglow_layer.get_child(0) as ColorRect if _afterglow_layer.get_child_count() > 0 else null

func get_active_afterglow_count() -> int:
	return get_tree().get_nodes_in_group(AFTERGLOW_GROUP).size()

func _manifest_scene_residue() -> void:
	if not GameManager or GameManager.current_state != GameManager.GameState.EXPLORATION:
		return
	if not MemoryManager or MemoryManager.burned_memories.is_empty():
		return
	var scene := get_tree().current_scene
	if scene == null:
		return
	var map_id := get_map_id_for_scene(scene.scene_file_path)
	var memory = _pick_revisit_memory(map_id)
	var authored := ""
	if memory != null:
		authored = get_revisit_line(String(memory.id), map_id)
	if memory == null:
		memory = MemoryManager.burned_memories.back()
	var report := _build_report(memory, false)
	if authored != "":
		GameManager.set_flag(revisit_flag(String(memory.id), map_id), true)
		report["line"] = authored
		report["title"] = "기억이 빠져나간 자리" if GameManager.current_locale == "ko" else "Where the Memory Was"
		if NotificationToast:
			NotificationToast.show_toast(authored, NotificationToast.ToastType.WARNING)
	else:
		report["line"] = ("이 장소는 %s 없이 다시 맞춰졌다." if GameManager.current_locale == "ko" else "This place has adjusted around %s.") % String(memory.title)
		report["title"] = "잔여 공백" if GameManager.current_locale == "ko" else "Residual Absence"
	_spawn_echo_cluster(scene, report, _find_manifest_origin(scene) + Vector2(0, -18), memory, 1)

## S226: Map key used by the revisit table (`res://scenes/maps/rim_forest.tscn` -> `rim_forest`).
func get_map_id_for_scene(scene_path: String) -> String:
	if scene_path == "":
		return ""
	return scene_path.get_file().get_basename()

func revisit_flag(memory_id: String, map_id: String) -> String:
	return "world_revisit_%s_%s" % [memory_id, map_id]

## S226: The authored second exposure for a burned memory on this map, if any.
func get_revisit_line(memory_id: String, map_id: String) -> String:
	if map_id == "" or not MEMORY_REVISIT_LINES.has(memory_id):
		return ""
	var by_map: Dictionary = MEMORY_REVISIT_LINES[memory_id]
	if not by_map.has(map_id):
		return ""
	var entry: Dictionary = by_map[map_id]
	return String(entry.get("ko", entry.get("en", ""))) if GameManager.current_locale == "ko" else String(entry.get("en", ""))

## The first burned memory whose absence this map can still show for the first time.
func _pick_revisit_memory(map_id: String):
	if map_id == "":
		return null
	for memory in MemoryManager.burned_memories:
		var mid := String(memory.id)
		if get_revisit_line(mid, map_id) == "":
			continue
		if GameManager.get_flag(revisit_flag(mid, map_id)):
			continue
		return memory
	return null

func _find_manifest_origin(scene: Node) -> Vector2:
	var players := get_tree().get_nodes_in_group("player")
	for player in players:
		if player is Node2D and (scene == player or scene.is_ancestor_of(player)):
			return (player as Node2D).global_position
	if scene is Node2D:
		return Vector2(640, 360)
	return Vector2(640, 360)

func _spawn_echo_cluster(scene: Node, report: Dictionary, origin: Vector2, memory, count: int = 3) -> void:
	var color: Color = report.get("color", Color(0.86, 0.68, 0.44))
	var line := String(report.get("line", "A contour vanished."))
	var grade := int(report.get("grade", MemoryManager.MemoryGrade.GRADE_5))
	var radius := 34.0 + float(grade) * 10.0
	for i in range(count):
		var angle := (TAU / maxf(float(count), 1.0)) * float(i) + randf_range(-0.42, 0.42)
		var offset := Vector2(cos(angle), sin(angle)) * randf_range(18.0, radius)
		var echo := _make_echo_node(line, color, grade, i == 0)
		if echo is Node2D:
			(echo as Node2D).global_position = origin + offset
		scene.add_child(echo)
		_animate_echo(echo, color, i)

func _make_echo_node(line: String, color: Color, grade: int, show_text: bool) -> Node2D:
	var root := Node2D.new()
	root.name = "MemoryRewriteEcho"
	root.z_index = 96

	var shard := Polygon2D.new()
	var size := 13.0 + float(grade) * 3.0
	shard.polygon = PackedVector2Array([
		Vector2(0, -size),
		Vector2(size * 0.48, 0),
		Vector2(0, size),
		Vector2(-size * 0.48, 0),
	])
	shard.color = Color(color.r, color.g, color.b, 0.48)
	root.add_child(shard)

	var core := ColorRect.new()
	core.position = Vector2(-1, -size)
	core.size = Vector2(2, size * 2.0)
	core.color = Color(1.0, 0.92, 0.72, 0.68)
	root.add_child(core)

	if show_text:
		var label := Label.new()
		label.text = line
		label.position = Vector2(18, -20)
		label.custom_minimum_size = Vector2(260, 42)
		label.autowrap_mode = TextServer.AUTOWRAP_WORD
		label.add_theme_font_size_override("font_size", 13)
		label.add_theme_color_override("font_color", color.lightened(0.28))
		label.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75))
		label.add_theme_constant_override("shadow_offset_x", 1)
		label.add_theme_constant_override("shadow_offset_y", 1)
		root.add_child(label)

	return root

func _animate_echo(echo: Node, color: Color, index: int) -> void:
	if not is_instance_valid(echo):
		return
	echo.modulate = Color(1, 1, 1, 0.0)
	echo.scale = Vector2(0.72, 0.72)
	var delay := float(index) * 0.08
	var tween := echo.create_tween()
	tween.tween_interval(delay)
	tween.set_parallel(true)
	tween.tween_property(echo, "modulate:a", 1.0, 0.16)
	tween.tween_property(echo, "scale", Vector2(1.12, 1.12), 0.28).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.set_parallel(false)
	tween.tween_property(echo, "position:y", echo.position.y - 24.0, 2.1).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tween.parallel().tween_property(echo, "modulate:a", 0.0, 2.1).set_delay(0.35)
	tween.finished.connect(func():
		if is_instance_valid(echo):
			echo.queue_free()
	)

func _show_rewrite_art(report: Dictionary) -> void:
	var art_path := String(report.get("art", ""))
	if art_path == "" or not ResourceLoader.exists(art_path):
		return
	var layer := CanvasLayer.new()
	layer.layer = 8
	layer.name = "WorldRewriteArtFlash"
	add_child(layer)

	var root := Control.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.modulate.a = 0.0
	layer.add_child(root)

	var plate := TextureRect.new()
	plate.anchor_left = 0.66
	plate.anchor_right = 0.985
	plate.anchor_top = 0.08
	plate.anchor_bottom = 0.52
	plate.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	plate.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	plate.texture = load(art_path)
	plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	plate.modulate = Color(1.0, 0.92, 0.78, 0.58)
	root.add_child(plate)

	var wash := ColorRect.new()
	wash.anchor_left = plate.anchor_left
	wash.anchor_right = plate.anchor_right
	wash.anchor_top = plate.anchor_top
	wash.anchor_bottom = plate.anchor_bottom
	var tint: Color = report.get("color", Color(0.86, 0.68, 0.44))
	wash.color = Color(tint.r, tint.g, tint.b, 0.08)
	wash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(wash)

	var tween := create_tween()
	tween.tween_property(root, "modulate:a", 0.62, 0.16)
	tween.tween_interval(0.46)
	tween.tween_property(root, "modulate:a", 0.0, 0.46).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tween.finished.connect(func():
		if is_instance_valid(layer):
			layer.queue_free()
	)

func _grade_color(grade: int) -> Color:
	match grade:
		MemoryManager.MemoryGrade.GRADE_1:
			return Color(1.0, 0.36, 0.22)
		MemoryManager.MemoryGrade.GRADE_2:
			return Color(0.9, 0.46, 0.52)
		MemoryManager.MemoryGrade.GRADE_3:
			return Color(0.72, 0.58, 0.9)
		MemoryManager.MemoryGrade.GRADE_4:
			return Color(0.88, 0.70, 0.42)
	return Color(0.70, 0.76, 0.62)

func _fallback_art_for_grade(grade: int) -> String:
	if grade >= MemoryManager.MemoryGrade.GRADE_2:
		return "res://assets/cg/generated/illustration_expansion_v2/world_rewrite_name_origin_v3.png"
	return "res://assets/cg/generated/ui_loss_record_blank_book_v2.png"

func _grade_name(grade: int) -> String:
	match grade:
		MemoryManager.MemoryGrade.GRADE_1:
			return "Grade 1 / Core"
		MemoryManager.MemoryGrade.GRADE_2:
			return "Grade 2 / Identity"
		MemoryManager.MemoryGrade.GRADE_3:
			return "Grade 3 / Relationship"
		MemoryManager.MemoryGrade.GRADE_4:
			return "Grade 4 / Daily Life"
	return "Grade 5 / Sensation"
