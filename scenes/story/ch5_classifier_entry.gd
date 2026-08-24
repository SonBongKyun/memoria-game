## Canon Season 1 Chapter 5 entry.
##
## This content controller consumes the saveable Chapter 4 boundary, freezes
## Malet's report as one persistent Kairos fact, then hands presentation to the
## existing VN pipeline. It is deliberately specific to The Classifier.
extends Node

const VN_SCENE_ID: String = "ch5_classifier"
const VN_HOST_PATH: String = "res://scenes/main/vn_host.tscn"
const MALET_ACTOR_ID: String = "npc.malet"
const KAIROS_ACTOR_ID: String = "npc.kairos"
const MALET_ROUTE_FACT_ID: String = "fact.bl07.route_request_received"
const MALET_ROUTE_MEMORY_ID: String = "memory.malet.bl07_request_source"
const REPORT_IDENTIFIED_FACT_ID: String = "fact.kairos.malet_report_identified_arrel"
const REPORT_UNKNOWN_FACT_ID: String = "fact.kairos.malet_report_requester_unknown"
const REPORT_IDENTIFIED: String = "identified_arrel"
const REPORT_UNKNOWN: String = "requester_unknown"

@export var launch_vn_on_ready: bool = true
@export var autosave_on_ready: bool = true


func _ready() -> void:
	var outcome := prepare_classifier_entry()
	if outcome == "":
		push_warning("[Ch5Classifier] Entry ignored outside the canonical Chapter 5 boundary")
		return
	if not launch_vn_on_ready:
		return
	SceneFlow.pending_scene_id = VN_SCENE_ID
	SceneFlow.pending_start_index = 0
	if autosave_on_ready:
		SaveManager.autosave_on_chapter_transition()
	call_deferred("_launch_vn_when_transition_idle")


func _launch_vn_when_transition_idle() -> void:
	# Drift Shelter reaches this entry through SceneTransition. Wait for that
	# owner to release the shared transition layer before requesting VNHost.
	if SceneTransition.is_transition_in_progress():
		await SceneTransition.transition_finished
	if not is_inside_tree():
		return
	SceneTransition.change_scene_styled(VN_HOST_PATH)


## Consumes the Chapter 4 boundary once. A save made on this small entry scene
## can resume because ch5_classifier_started remains true until the VN reaches
## the canonical Chapter 6 boundary.
func prepare_classifier_entry() -> String:
	if GameManager.get_flag("canon_ch6_seam_ready"):
		return ""
	var boundary_ready := GameManager.get_flag("canon_ch5_classifier_ready")
	var resuming_entry := GameManager.get_flag("ch5_classifier_started") \
		and GameManager.current_chapter == 5
	if not boundary_ready and not resuming_entry:
		return ""

	if boundary_ready:
		GameManager.set_flag("canon_ch5_classifier_ready", false)
	GameManager.set_flag("ch5_classifier_started")
	GameManager.set_flag("ch5_kairos_seen")
	GameManager.current_chapter = 5

	var outcome := resolve_malet_report_outcome()
	_project_report_flags(outcome)
	return outcome


## Decide the historical report exactly once. Later changes to Malet's current
## memory do not rewrite either Kairos fact, so Chapter 21 can consume the
## information that actually crossed the gap in Chapter 5.
static func resolve_malet_report_outcome() -> String:
	var identified := MemoryEngine.knows_fact(KAIROS_ACTOR_ID, REPORT_IDENTIFIED_FACT_ID)
	var unknown := MemoryEngine.knows_fact(KAIROS_ACTOR_ID, REPORT_UNKNOWN_FACT_ID)
	if identified or unknown:
		if identified and unknown:
			push_warning("[Ch5Classifier] Conflicting historical report facts; preserving identified outcome")
		return REPORT_IDENTIFIED if identified else REPORT_UNKNOWN

	var request_recorded := MemoryEngine.knows_fact(MALET_ACTOR_ID, MALET_ROUTE_FACT_ID)
	var source_memory_active := MemoryEngine.check_memory(
		MALET_ACTOR_ID, MALET_ROUTE_MEMORY_ID)
	var source_record := WorldState.get_memory_record(
		MALET_ACTOR_ID, MALET_ROUTE_MEMORY_ID)
	var source_is_arrel := String(source_record.get("source_actor_id", "")) == "player.arrel"
	var outcome_fact := REPORT_IDENTIFIED_FACT_ID \
		if request_recorded and source_memory_active and source_is_arrel \
		else REPORT_UNKNOWN_FACT_ID
	if not MemoryEngine.learn_fact(KAIROS_ACTOR_ID, outcome_fact):
		push_error("[Ch5Classifier] Could not commit historical Malet report outcome")
		return ""
	return REPORT_IDENTIFIED if outcome_fact == REPORT_IDENTIFIED_FACT_ID else REPORT_UNKNOWN


static func get_persistent_report_outcome() -> String:
	if MemoryEngine.knows_fact(KAIROS_ACTOR_ID, REPORT_IDENTIFIED_FACT_ID):
		return REPORT_IDENTIFIED
	if MemoryEngine.knows_fact(KAIROS_ACTOR_ID, REPORT_UNKNOWN_FACT_ID):
		return REPORT_UNKNOWN
	return ""


static func _project_report_flags(outcome: String) -> void:
	# These flags only select authored VN lines. The durable source of truth is
	# the pair of Kairos WorldState facts above.
	GameManager.set_flag("ch5_malet_report_identified_arrel", outcome == REPORT_IDENTIFIED)
	GameManager.set_flag("ch5_malet_report_requester_unknown", outcome == REPORT_UNKNOWN)
