extends Node

## S250: 로스터 28종의 전투 삽화가 무엇으로 해석되는지 전부 확인한다.
##
## 지금까지 서베이가 다섯 번 결함을 찾았고 그중 **두 번이 적 삽화**였다.
## S241은 프리셋 표의 낡은 img를, S247은 맵 스크립트의 인라인 사본을 잡았다.
## 두 번 다 "보니까" 나왔다. 그런데 전투 화면 서베이는 프리셋 6종만 봤고
## 나머지 22종은 한 번도 렌더된 적이 없다.
##
## 전투 씬을 28번 세우지 않는다. BattleScene이 실제로 쓰는 해석 경로
## (BattleManager.enemy_image → resolve_enemy_image_by_name)를 그대로 태워
## 각 적이 어떤 파일로 귀결되는지 본다. 세 가지를 신고한다.
##   - 없음: 해석 결과가 비어 절차 생성 스프라이트로 떨어지는 적
##   - 공용: 여러 적이 같은 파일을 나눠 쓰는 경우
##   - 전용: 자기 그림을 가진 적

const ROSTER: Array[Dictionary] = [
	{"name": "Ash Crawler", "scene": "res://scenes/maps/rim_forest.tscn"},
	{"name": "Forest Shade", "scene": "res://scenes/maps/rim_forest.tscn"},
	{"name": "Void Wisp", "scene": "res://scenes/maps/rim_forest.tscn"},
	{"name": "Market Thief", "scene": "res://scenes/maps/verdan_market.tscn"},
	{"name": "Alley Rat", "scene": "res://scenes/maps/verdan_market.tscn"},
	{"name": "Belt Scavenger", "scene": "res://scenes/maps/belt_waystation.tscn"},
	{"name": "Dust Crawler", "scene": "res://scenes/maps/belt_waystation.tscn"},
	{"name": "Memory Leech", "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"name": "Ash Walker", "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"name": "Rubble Rat", "scene": "res://scenes/maps/drift_shelter.tscn"},
	{"name": "Coastal Void Beast", "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"name": "Shore Wraith", "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"name": "Cliff Stalker", "scene": "res://scenes/maps/crumbling_coast.tscn"},
	{"name": "Void Sentinel", "scene": "res://scenes/maps/the_seam.tscn"},
	{"name": "Void Wraith", "scene": "res://scenes/maps/the_seam.tscn"},
	{"name": "Seam Lurker", "scene": "res://scenes/maps/the_seam.tscn"},
	{"name": "Shade Sentinel", "scene": "res://scenes/maps/the_seam.tscn"},
	{"name": "Threshold Crawler", "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"name": "Depth Crawler", "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"name": "Threshold Shade", "scene": "res://scenes/maps/seam_outskirts.tscn"},
	{"name": "Root Shade", "scene": "res://scenes/maps/forgotten_forest.tscn"},
	{"name": "Ash Phantom", "scene": "res://scenes/maps/forgotten_forest.tscn"},
	{"name": "Colorless Wraith", "scene": "res://scenes/maps/colorless_waste.tscn"},
	{"name": "Hollow Walker", "scene": "res://scenes/maps/colorless_waste.tscn"},
	{"name": "Void Fragment", "scene": "res://scenes/maps/bl07_void.tscn"},
	{"name": "Memory Eater", "scene": "res://scenes/maps/bl07_void.tscn"},
	{"name": "Null Wisp", "scene": "res://scenes/maps/bl07_void.tscn"},
	{"name": "Kairos, Authority Editor", "scene": "res://scenes/maps/bl07_void.tscn"},
]

func _ready() -> void:
	Codex.suppress_recording = true
	GameManager.current_locale = "en"
	await get_tree().process_frame

	var usage: Dictionary = {}
	var missing: Array[String] = []
	for row in ROSTER:
		var name := String(row["name"])
		var art := BattleManager.resolve_enemy_image_by_name(name)
		if art == "" or not ResourceLoader.exists(art):
			missing.append(name)
			print("ENEMY_ART %-24s <없음 — 절차 생성 스프라이트로 떨어짐>" % name)
			continue
		if not usage.has(art):
			usage[art] = []
		usage[art].append(name)

	print("")
	for art in usage:
		var users: Array = usage[art]
		var tag := "전용" if users.size() == 1 else "공용 x%d" % users.size()
		print("ENEMY_ART_FILE %-10s %-46s %s" % [tag, art.get_file(), ", ".join(users)])

	var shared := 0
	for art in usage:
		if (usage[art] as Array).size() > 1:
			shared += (usage[art] as Array).size()
	print("")
	print("ENEMY_ART_SUMMARY roster=%d missing=%d shared_users=%d distinct_files=%d" % [
		ROSTER.size(), missing.size(), shared, usage.size()])
	print("ENEMY_ART_DONE")
	get_tree().quit(0)
