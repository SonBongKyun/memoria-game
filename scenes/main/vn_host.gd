## VNHost — VN 씬 전용 빈 컨테이너
## S60: SceneFlow가 VN을 구동할 때 '배경'이 될 빈 씬. VN UI가 CanvasLayer로 덮음.
## 탐색 맵이 아닌 순수 VN 구간에서 사용.
extends Node2D

func _ready() -> void:
	# VN이 끝났을 때 자동 복귀할 곳이 없으면 타이틀로 돌아감
	SceneFlow.scene_ended.connect(_on_scene_ended)
	# 대기 중인 씬이 있으면 재생 (예: 새 게임 시작)
	if SceneFlow.pending_scene_id != "":
		var sid = SceneFlow.pending_scene_id
		var idx = SceneFlow.pending_start_index
		SceneFlow.pending_scene_id = ""
		SceneFlow.pending_start_index = 0
		SceneFlow.play(sid, idx)
	# 아니면 resume 큐에서 자동 재개 (탐색/전투에서 복귀)
	elif SceneFlow.resume_queue.size() > 0:
		SceneFlow.resume_if_queued()

func _on_scene_ended(_id: String) -> void:
	# 복귀 큐가 있으면 SceneFlow가 스스로 다음 씬 재생
	if SceneFlow.resume_if_queued():
		return
	# 없으면 대기 (플레이어가 타이틀로 돌아가려면 ESC 일시정지 → 타이틀)
	print("[VNHost] VN ended with no resume queued")
