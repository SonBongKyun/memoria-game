## MemoryManager (Autoload)
## 기억 연소 시스템의 핵심. 기억의 보유/연소/거래를 관리.
extends Node

# --- 기억 등급 ---
# 값이 클수록 높은 등급: GRADE_5=0(최하) ~ GRADE_1=4(최상). 비교 시 >= 는 "같거나 높은 등급".
enum MemoryGrade { GRADE_5, GRADE_4, GRADE_3, GRADE_2, GRADE_1 }

# --- 기억 데이터 클래스 ---
class Memory:
	var id: String            # 고유 ID
	var title: String         # "첫 검술 수련"
	var description: String   # 상세 설명
	var grade: int            # MemoryGrade
	var burn_power: int       # 연소 시 전투력
	var is_burned: bool       # 연소 여부
	var is_residue: bool      # 잔존(희미한 흔적) 상태인지
	var is_faded: bool        # 침식으로 희미해짐 (전투 연소 불가)
	var erosion: int          # 침식도 (0~burn_power*0.7 이상이면 faded)
	var story_effect: String  # 연소 시 스토리 영향 설명
	var related_npc: String   # 관련 NPC (빈 문자열이면 없음)
	var connections: Array = []  # S62: 연결된 기억 ID 배열 (Memory Constellation)

	func _init(p_id: String, p_title: String, p_desc: String, p_grade: int, p_power: int, p_effect: String = "", p_npc: String = "") -> void:
		id = p_id
		title = p_title
		description = p_desc
		grade = p_grade
		burn_power = p_power
		is_burned = false
		is_residue = false
		is_faded = false
		erosion = 0
		story_effect = p_effect
		related_npc = p_npc

# --- 기억 저장소 (아렐의 서고) ---
var memories: Array[Memory] = []
var burned_memories: Array[Memory] = []  # 연소된 기억 기록

# --- 합성 결과 이름 ---
const SYNTHESIS_NAMES: Dictionary = {
	# grade_value → 합성 결과 제목/설명 템플릿
	MemoryGrade.GRADE_4: {"title": "Blended Sensation", "desc": "Two fading impressions fused into something richer. The detail is sharper now."},
	MemoryGrade.GRADE_3: {"title": "Woven Routine", "desc": "Daily fragments entwined — a habit you didn't know you had."},
	MemoryGrade.GRADE_2: {"title": "Bound Connection", "desc": "Relationships compressed into a single ache. Heavier, but clearer."},
	MemoryGrade.GRADE_1: {"title": "Forged Identity", "desc": "The core of who you are, distilled from what you chose to keep."},
}

# --- Burn Passives (Skill Tree) ---
var burn_passives: Dictionary = {}  # {passive_name: true}

const PASSIVE_THRESHOLDS: Dictionary = {
	5: {"id": "ember_affinity", "name": "Ember Affinity", "desc": "+10% burn damage", "icon": "flame"},
	10: {"id": "residual_warmth", "name": "Residual Warmth", "desc": "+5 HP heal after burn", "icon": "heart"},
	20: {"id": "ash_sight", "name": "Ash Sight", "desc": "See enemy HP numbers", "icon": "eye"},
	30: {"id": "void_touch", "name": "Void Touch", "desc": "+15% void damage", "icon": "skull"},
	50: {"id": "memory_cascade", "name": "Memory Cascade", "desc": "Limit break charges 20% faster", "icon": "star"},
}

# --- 시그널 ---
signal memory_burned(memory: Memory)
signal memory_added(memory: Memory)
signal memory_became_residue(memory: Memory)
signal memory_synthesized(result: Memory, consumed_a: Memory, consumed_b: Memory)
signal memory_faded(memory: Memory)
signal memories_eroded(count: int)
signal passive_unlocked(passive_name: String)

func _ready() -> void:
	_init_starting_memories()
	print("[MemoryManager] Initialized — %d memories loaded" % memories.size())

## 초기 기억 세팅 (Chapter 1 시작 시)
func _init_starting_memories() -> void:
	# Grade 5 — 감각 잔편
	add_memory(Memory.new(
		"sense_forest_smell",
		"Forest After Rain",
		"The smell of wet earth after rainfall. Where or when, you can't say.",
		MemoryGrade.GRADE_5, 10
	))
	add_memory(Memory.new(
		"sense_warm_light",
		"Warm Light Through a Window",
		"Golden light falling across a wooden floor. A room you can't place.",
		MemoryGrade.GRADE_5, 10
	))

	# Grade 4 — 일상 기억
	add_memory(Memory.new(
		"daily_market_food",
		"Street Food in a Market",
		"Spiced meat on a stick. The vendor's face is gone, but the taste remains.",
		MemoryGrade.GRADE_4, 25,
		"Lose ability to recognize Verdan food vendors"
	))
	add_memory(Memory.new(
		"daily_campfire_song",
		"A Song by a Campfire",
		"Someone was singing. The melody is clear but the voice has no face.",
		MemoryGrade.GRADE_4, 25,
		"Elia's humming no longer triggers calm effect",
		"Elia"
	))

	# Grade 3 — 관계 기억
	add_memory(Memory.new(
		"rel_hand_reaching",
		"A Hand Reaching Out",
		"Your hand moves toward someone before you can stop it. The gesture is older than your memory of why.",
		MemoryGrade.GRADE_3, 50,
		"Lose the 'Residue' animation with Elia. She notices.",
		"Elia"
	))

	# Grade 2 — 정체성 기억
	add_memory(Memory.new(
		"identity_first_sword",
		"The Day You First Held a Sword",
		"A courtyard. Dust. A hand larger than yours closing your fingers around a wooden grip. The most important moment you can still feel.",
		MemoryGrade.GRADE_2, 100,
		"Combat stance changes. Base attack pattern simplified. Malet's price.",
		"Unknown"
	))

	# Grade 1 — 핵심 기억 (게임 후반부)
	add_memory(Memory.new(
		"core_name_origin",
		"The Name 'Arrel'",
		"Someone gave you this name. You don't remember who. But when you hear it, something in your chest responds.",
		MemoryGrade.GRADE_1, 999,
		"ENDING CRITICAL: Burning this memory triggers the Zero Burn ending path.",
		"Elia"
	))

## 기억 침식 — 챕터 진행 시 미연소 기억이 서서히 바래짐
## Grade 1(핵심)과 엘리아 관련 기억(동행 시)은 침식 내성
func apply_erosion(chapter: int) -> void:
	var base_amount = chapter * 1  # S53: 침식 속도 완화 (chapter*2 → chapter*1)
	var eroded_count = 0
	for m in memories:
		if m.is_burned or m.is_faded:
			continue
		# Grade 1(핵심 기억)은 침식 면역
		if m.grade >= MemoryGrade.GRADE_1:
			continue
		# 엘리아 동행 시 관련 기억은 침식 절반
		var amount = base_amount
		if m.related_npc == "Elia" and GameManager.player_data.elia_with_party:
			amount = int(amount * 0.5)
		# S53: Grade 2(정체성) 기억은 침식 75% 속도
		if m.grade == MemoryGrade.GRADE_2:
			amount = int(amount * 0.75)
		m.erosion += amount
		eroded_count += 1
		# 침식 임계: burn_power의 70% 이상이면 Faded
		if m.erosion >= int(m.burn_power * 0.7) and not m.is_faded:
			m.is_faded = true
			memory_faded.emit(m)
			NotificationToast.show_toast("Memory fading: %s" % m.title, NotificationToast.ToastType.WARNING)
			print("[MemoryManager] FADED: %s (erosion %d/%d)" % [m.title, m.erosion, m.burn_power])
	if eroded_count > 0:
		memories_eroded.emit(eroded_count)
		print("[MemoryManager] Erosion applied — %d memories affected (ch%d, +%d)" % [eroded_count, chapter, base_amount])

## 유효 연소력 계산 (침식 반영)
func get_effective_burn_power(memory: Memory) -> int:
	return maxi(1, memory.burn_power - memory.erosion)

## 침식 비율 (UI 표시용)
func get_erosion_ratio(memory: Memory) -> float:
	if memory.burn_power <= 0:
		return 0.0
	return clampf(float(memory.erosion) / float(memory.burn_power), 0.0, 1.0)

## 비전투 연소 (탐색 공명 이벤트용 — 소리 없이)
func burn_memory_silent(memory_id: String, allow_faded: bool = false) -> Memory:
	for i in range(memories.size()):
		if memories[i].id == memory_id and not memories[i].is_burned:
			var memory = memories[i]
			if memory.is_faded and not allow_faded:
				push_warning("[MemoryManager] Refused silent burn of faded memory: %s" % memory_id)
				return null
			memory.is_burned = true
			burned_memories.append(memory)
			memory_burned.emit(memory)
			check_unlock_passives()
			print("[MemoryManager] SILENT BURN: %s" % memory.title)
			return memory
	return null

## 챕터 진행에 따른 기억 추가
func add_chapter_memories(chapter: int) -> void:
	# 챕터 진행 시 기존 기억 침식 적용
	if chapter >= 3:
		apply_erosion(chapter)
	match chapter:
		3:
			# Ch3: Belt Waystation — Weight of Pages
			if not _has_memory("sense_dead_soil"):
				add_memory(Memory.new(
					"sense_dead_soil",
					"The Taste of Dead Earth",
					"Dust that tastes of nothing. The Belt stretches on, a road that forgot its purpose.",
					MemoryGrade.GRADE_5, 10
				))
			if not _has_memory("rel_tobias_records"):
				add_memory(Memory.new(
					"rel_tobias_records",
					"The Man Who Writes Everything Down",
					"Ink-stained fingers. Spectacles perched on a nose that's seen too many ledgers. He records the dying.",
					MemoryGrade.GRADE_4, 25,
					"Lose the ability to recall Tobias's explanations. Bureau terminology blurs.",
					"Tobias"
				))
		4:
			# Ch4: Drift Shelter — Drift
			if not _has_memory("sense_ash_rain"):
				add_memory(Memory.new(
					"sense_ash_rain",
					"Rain That Isn't Rain",
					"Gray drops that leave streaks on skin. Memory residue from someone else's burning, carried on the wind.",
					MemoryGrade.GRADE_5, 12
				))
			if not _has_memory("daily_elia_hands"):
				add_memory(Memory.new(
					"daily_elia_hands",
					"Warm Hands on Cold Palms",
					"She held your hands and something shifted. A page pressed back into its binding. You could read again.",
					MemoryGrade.GRADE_3, 50,
					"Lose the sensation of being anchored. The architecture loosens.",
					"Elia"
				))
		5:
			# Ch5: Crumbling Coast — The Classifier
			if not _has_memory("sense_salt_wind"):
				add_memory(Memory.new(
					"sense_salt_wind",
					"Salt Wind on the Cliffs",
					"The taste of brine carried up from where the coast is falling apart. Cold and honest.",
					MemoryGrade.GRADE_5, 12
				))
			if not _has_memory("daily_elia_walking"):
				add_memory(Memory.new(
					"daily_elia_walking",
					"Walking With Someone",
					"Two sets of footsteps. Yours and someone lighter. The rhythm is familiar.",
					MemoryGrade.GRADE_4, 30,
					"Lose awareness of Elia's movement patterns",
					"Elia"
				))
		6:
			# Ch6: The Seam — Thread That Holds
			if not _has_memory("rel_sable_trust"):
				add_memory(Memory.new(
					"rel_sable_trust",
					"The Woman Who Came Back",
					"She walked into a Void Hole and walked out. You don't know how. But when she speaks, you believe her.",
					MemoryGrade.GRADE_3, 55,
					"Lose instinctive trust toward Sable. Her advice feels hollow.",
					"Sable"
				))
			if not _has_memory("sense_seam_colors"):
				add_memory(Memory.new(
					"sense_seam_colors",
					"Colors That Shouldn't Exist",
					"Amber. Crimson. Green so deep it hurts. The Seam bleeds color into a gray world.",
					MemoryGrade.GRADE_5, 15
				))
			if not _has_memory("daily_garden_flowers"):
				add_memory(Memory.new(
					"daily_garden_flowers",
					"Flowers From Every Season",
					"They bloom together — spring and autumn sharing the same soil. Time doesn't work right here.",
					MemoryGrade.GRADE_4, 28,
					"The Seam's gardens appear monochrome"
				))
		7:
			# Ch7: Seam Outskirts — The Other Side of the Flame
			if not _has_memory("sense_static_air"):
				add_memory(Memory.new(
					"sense_static_air",
					"The Taste of Static",
					"Air that crackles against your teeth. The Threshold tastes like a storm that never breaks.",
					MemoryGrade.GRADE_5, 12
				))
			if not _has_memory("rel_echo_shell"):
				add_memory(Memory.new(
					"rel_echo_shell",
					"Voices in the Shell",
					"Dozens of fragments. A woman warned someone. A man felt his hands dissolving. A child forgot blue.",
					MemoryGrade.GRADE_3, 55,
					"Lose the Echo Shell's protection. BL-07's pull strengthens.",
					"Sable"
				))
		8:
			# Ch8: Forgotten Forest — The Forest That Forgets
			if not _has_memory("sense_hollow_trees"):
				add_memory(Memory.new(
					"sense_hollow_trees",
					"Trees That Remember Being Trees",
					"They stand because they forgot how to fall. The bark peels away like old thoughts, revealing nothing underneath.",
					MemoryGrade.GRADE_5, 14
				))
			if not _has_memory("rel_ghost_words"):
				add_memory(Memory.new(
					"rel_ghost_words",
					"A Ghost's Last Sentence",
					"'I was—' That's all they could say. The rest was eaten. You wonder how many sentences you've already lost.",
					MemoryGrade.GRADE_4, 30,
					"Lose awareness of incomplete thoughts. Gaps in speech go unnoticed.",
					""
				))
		9:
			# Ch9: Colorless Waste — Where Colors Stop
			if not _has_memory("sense_no_color"):
				add_memory(Memory.new(
					"sense_no_color",
					"The Place Where Color Stopped",
					"Gray that isn't gray — it's the absence of the concept. Even memory of color feels distant here.",
					MemoryGrade.GRADE_5, 15
				))
			if not _has_memory("identity_compass"):
				add_memory(Memory.new(
					"identity_compass",
					"The Memory Compass",
					"It doesn't point north. It points toward what you're about to forget. The needle spins faster near BL-07.",
					MemoryGrade.GRADE_2, 110,
					"Lose the ability to sense approaching memory loss. Burns happen without warning.",
					""
				))
		10:
			if not _has_memory("identity_void_walker"):
				add_memory(Memory.new(
					"identity_void_walker",
					"What You Saw Inside BL-07",
					"The space between spaces. A sound that wasn't a sound. Something looked back at you through the tear.",
					MemoryGrade.GRADE_2, 120,
					"Lose the ability to sense Void Holes. Navigate by instinct alone."
				))
		11:
			if not _has_memory("daily_belt_footsteps"):
				add_memory(Memory.new(
					"daily_belt_footsteps",
					"The Pace That Carried You West",
					"Elia matched this pace for days without asking you to slow down.",
					MemoryGrade.GRADE_4, 25,
					"Lose the shared walking rhythm that lets Elia follow without looking.",
					"Elia"
				))
			if not _has_memory("rel_elia_bloodwork"):
				add_memory(Memory.new(
					"rel_elia_bloodwork",
					"The Blood She Hid",
					"A red line on Elia's glove. She said the notebook had only cut her.",
					MemoryGrade.GRADE_3, 50,
					"Lose the moment you first understood what reading the blank pages cost her.",
					"Elia"
				))
		12:
			if not _has_memory("rel_verdan_faces"):
				add_memory(Memory.new(
					"rel_verdan_faces",
					"Faces in Verdan's Crowd",
					"The vendors who looked away, the child who pointed, the hands that opened a path.",
					MemoryGrade.GRADE_3, 50,
					"Lose the people of Verdan as individuals. The market becomes only terrain."
				))
		13:
			if not _has_memory("rel_tobias_alliance"):
				add_memory(Memory.new(
					"rel_tobias_alliance",
					"The Third Hand on the Record",
					"Tobias stopped accusing long enough to place his notebook beside Elia's.",
					MemoryGrade.GRADE_3, 50,
					"Lose the exact moment suspicion became an alliance.",
					"Tobias"
				))
		14:
			if not _has_memory("identity_intervention_vow"):
				add_memory(Memory.new(
					"identity_intervention_vow",
					"The Vow to Intervene",
					"You decided that witnessing extraction and surviving it were no longer enough.",
					MemoryGrade.GRADE_2, 100,
					"Lose the reason you step between the Authority and its victims."
				))
		15:
			if not _has_memory("daily_unfinished_lullaby"):
				add_memory(Memory.new(
					"daily_unfinished_lullaby",
					"The Lullaby Without Its Last Note",
					"Han hummed around the missing note until Elia recognized the shape of it.",
					MemoryGrade.GRADE_4, 28,
					"Lose the melody that first tied Han, Elia, and Celah together.",
					"Han"
				))
			if not _has_memory("rel_han_mneme"):
				add_memory(Memory.new(
					"rel_han_mneme",
					"The Singer Beneath Arkein",
					"A scarred throat, a slate, and a voice that survived by becoming vibration.",
					MemoryGrade.GRADE_3, 50,
					"Lose the resonance that lets Han wake the Echo Shell.",
					"Han"
				))
		16:
			if not _has_memory("daily_eastward_route"):
				add_memory(Memory.new(
					"daily_eastward_route",
					"The Road Chosen at Dawn",
					"Tobias marked the safe channels. Elia folded the map. You chose east.",
					MemoryGrade.GRADE_4, 28,
					"Lose the route behind you. Only the destination remains."
				))
		17:
			if not _has_memory("daily_three_shadows"):
				add_memory(Memory.new(
					"daily_three_shadows",
					"Three Shadows on the Causeway",
					"For one morning, the road held three shadows moving at the same pace.",
					MemoryGrade.GRADE_4, 30,
					"Lose the ordinary certainty that you crossed the east road together.",
					"Tobias"
				))
		18:
			if not _has_memory("identity_witness_record"):
				add_memory(Memory.new(
					"identity_witness_record",
					"The Duty to Leave a Witness",
					"Someone must survive long enough to say what was done here. You accepted that duty.",
					MemoryGrade.GRADE_2, 110,
					"Lose the belief that memory becomes responsibility when it is witnessed.",
					"Tobias"
				))

func _get_memory(memory_id: String) -> Memory:
	for m in memories:
		if m.id == memory_id:
			return m
	return null

func _has_memory(memory_id: String) -> bool:
	for m in memories:
		if m.id == memory_id:
			return true
	return false

## 기억 추가
func add_memory(memory: Memory) -> void:
	memories.append(memory)
	memory_added.emit(memory)
	GameManager.add_stat("memories_collected")  # S55
	if is_inside_tree():
		AudioManager.play_sfx("memory_add")
	_refresh_connections()  # S62: 새 기억 추가 시 연결 자동 재계산

## S62: Memory Constellation — 기억 간 연결 자동 생성
## 규칙: (1) 같은 related_npc 끼리 연결, (2) 인접 등급의 같은 주제 키워드 연결
func _refresh_connections() -> void:
	# 초기화
	for m in memories:
		m.connections.clear()
	# related_npc 기반 연결 (같은 NPC 관련 기억들은 서로 연결)
	var npc_groups: Dictionary = {}
	for m in memories:
		if m.related_npc == "":
			continue
		if not npc_groups.has(m.related_npc):
			npc_groups[m.related_npc] = []
		npc_groups[m.related_npc].append(m)
	for npc in npc_groups.keys():
		var group: Array = npc_groups[npc]
		for i in range(group.size()):
			for j in range(i + 1, group.size()):
				_link(group[i], group[j])
	# id prefix(카테고리) 기반 연결 (sense_*, daily_*, rel_*, identity_*, core_*)
	var prefix_groups: Dictionary = {}
	for m in memories:
		var parts = m.id.split("_")
		if parts.size() < 2:
			continue
		var prefix = parts[0]
		if not prefix_groups.has(prefix):
			prefix_groups[prefix] = []
		prefix_groups[prefix].append(m)
	for prefix in prefix_groups.keys():
		var group: Array = prefix_groups[prefix]
		# 같은 prefix는 인접 항목만 연결 (모든 쌍이면 너무 지저분)
		for i in range(group.size() - 1):
			_link(group[i], group[i + 1])

func _link(a: Memory, b: Memory) -> void:
	if a.id == b.id:
		return
	if not a.connections.has(b.id):
		a.connections.append(b.id)
	if not b.connections.has(a.id):
		b.connections.append(a.id)

## S62: id로 기억 조회
func find_memory(memory_id: String) -> Memory:
	for m in memories:
		if m.id == memory_id:
			return m
	return null

## S62: 특정 기억의 연결된 기억 중 태워진 것의 수 (균열 표현용)
func burned_neighbor_count(memory_id: String) -> int:
	var m = find_memory(memory_id)
	if m == null:
		return 0
	var count = 0
	for cid in m.connections:
		var neighbor = find_memory(cid)
		if neighbor != null and neighbor.is_burned:
			count += 1
	return count

## 기억 연소
func burn_memory(memory_id: String, allow_faded: bool = false) -> Memory:
	for i in range(memories.size()):
		if memories[i].id == memory_id and not memories[i].is_burned:
			var memory = memories[i]
			if memory.is_faded and not allow_faded:
				push_warning("[MemoryManager] Refused burn of faded memory: %s" % memory_id)
				return null
			memory.is_burned = true

			# 엘리아 동행 시 잔존 가능성
			if GameManager.player_data.elia_with_party and memory.grade >= MemoryGrade.GRADE_3:
				memory.is_residue = true
				memory_became_residue.emit(memory)
				print("[MemoryManager] BURNED (Residue): %s" % memory.title)
			else:
				print("[MemoryManager] BURNED (Gone): %s" % memory.title)

			burned_memories.append(memory)
			memory_burned.emit(memory)
			check_unlock_passives()
			return memory

	return null

## 특정 등급의 사용 가능한 기억 목록
func get_available_memories(min_grade: int = MemoryGrade.GRADE_5, allow_faded: bool = false) -> Array[Memory]:
	var available: Array[Memory] = []
	for memory in memories:
		if not memory.is_burned and (allow_faded or not memory.is_faded) and memory.grade >= min_grade:
			available.append(memory)
	return available

## 잔존 기억 목록 (연소됨 + is_residue)
func get_residue_memories() -> Array[Memory]:
	var residues: Array[Memory] = []
	for memory in memories:
		if memory.is_burned and memory.is_residue:
			residues.append(memory)
	return residues

## 특정 잔존 기억 가져오기
func get_residue_memory(memory_id: String) -> Memory:
	for memory in memories:
		if memory.id == memory_id and memory.is_burned and memory.is_residue:
			return memory
	return null

## 전체 연소된 기억 수
func get_burn_count() -> int:
	return burned_memories.size()

## 연소율 (엔딩 분기 판정용)
func get_burn_ratio() -> float:
	if memories.size() == 0:
		return 0.0
	return float(burned_memories.size()) / float(memories.size())

## 특정 기억이 연소되었는지 확인
func is_memory_burned(memory_id: String) -> bool:
	for memory in burned_memories:
		if memory.id == memory_id:
			return true
	return false

## S146: 기억이 온전한 상태인지 (보유 + 미연소 + 미침식)
## 대화 선택지 게이팅 및 The Weave 엔딩 판정에 사용.
func is_intact(memory_id: String) -> bool:
	var m = find_memory(memory_id)
	return m != null and not m.is_burned and not m.is_faded

## S146: The Weave 엔딩 앵커 기억 정의
## PRIMARY(이름)는 반드시 온전해야 하고, SECONDARY 중 3개 이상이 온전하며,
## 전체 연소 수가 적을 때(보존 플레이) 제3의 길이 열린다.
const WEAVE_PRIMARY: String = "core_name_origin"
const WEAVE_SECONDARY: Array = [
	"identity_first_sword",  # 정체성
	"rel_hand_reaching",     # 엘리아를 향한 손짓
	"daily_elia_hands",      # 앵커링 감각
	"rel_sable_trust",       # 세이블에 대한 신뢰
]
const WEAVE_MAX_BURNS: int = 4

## 온전한 SECONDARY 앵커 수
func intact_anchor_count() -> int:
	var count := 0
	for aid in WEAVE_SECONDARY:
		if is_intact(aid):
			count += 1
	return count

## The Weave(제3의 길) 해금 조건 충족 여부
func weave_unlocked() -> bool:
	if not is_intact(WEAVE_PRIMARY):
		return false
	if get_burn_count() >= WEAVE_MAX_BURNS:
		return false
	return intact_anchor_count() >= 3

## 기억 합성 — 동일 등급 기억 2개 → 상위 등급 1개
## 원본은 소실(연소와 다른 방식의 상실). Grade 1(=4)은 최고 등급이므로 합성 불가.
func synthesize(memory_a_id: String, memory_b_id: String) -> Memory:
	var mem_a: Memory = null
	var mem_b: Memory = null
	for m in memories:
		if m.id == memory_a_id and not m.is_burned:
			mem_a = m
		elif m.id == memory_b_id and not m.is_burned:
			mem_b = m

	if mem_a == null or mem_b == null:
		return null
	if mem_a.grade != mem_b.grade:
		return null
	if mem_a.grade >= MemoryGrade.GRADE_1:  # 이미 최고 등급
		return null

	var new_grade = mem_a.grade + 1
	var new_power = int((mem_a.burn_power + mem_b.burn_power) * 0.7) + 10
	var template = SYNTHESIS_NAMES.get(new_grade, {"title": "Synthesized Memory", "desc": "Two memories became one."})

	var synth_id = "synth_%s_%s" % [mem_a.id.left(8), mem_b.id.left(8)]
	var synth = Memory.new(
		synth_id,
		template["title"],
		template["desc"] + "\n(From: %s + %s)" % [mem_a.title, mem_b.title],
		new_grade,
		new_power,
		"Synthesized — cannot be undone"
	)

	# 원본 제거
	memories.erase(mem_a)
	memories.erase(mem_b)

	# 새 기억 추가
	add_memory(synth)
	memory_synthesized.emit(synth, mem_a, mem_b)
	NotificationToast.show_toast("Synthesized: %s" % synth.title, NotificationToast.ToastType.SUCCESS)
	print("[MemoryManager] SYNTHESIZED: %s + %s → %s (Grade %d)" % [mem_a.title, mem_b.title, synth.title, new_grade])
	return synth

## 합성 가능한 쌍 존재 여부 (같은 등급 미연소 기억 2개 이상, Grade 1 제외)
func has_synthesizable_pair() -> bool:
	var grade_counts: Dictionary = {}
	for m in memories:
		if not m.is_burned and m.grade < MemoryGrade.GRADE_1:
			grade_counts[m.grade] = grade_counts.get(m.grade, 0) + 1
	for count in grade_counts.values():
		if count >= 2:
			return true
	return false

## 전체 연소 횟수 합계 (모든 기억의 burn 상태)
func get_total_burn_count() -> int:
	return burned_memories.size()

## 연소 패시브 해금 체크 (연소 후 호출)
func check_unlock_passives() -> void:
	var total = get_total_burn_count()
	for threshold in PASSIVE_THRESHOLDS:
		var passive = PASSIVE_THRESHOLDS[threshold]
		if total >= threshold and not burn_passives.has(passive["id"]):
			burn_passives[passive["id"]] = true
			passive_unlocked.emit(passive["name"])
			NotificationToast.show_toast("Passive Unlocked: %s" % passive["name"], NotificationToast.ToastType.SUCCESS)
			print("[MemoryManager] PASSIVE UNLOCKED: %s (burns: %d)" % [passive["name"], total])
			# S58: Power milestone — dramatic screen effect
			_play_power_milestone(passive["name"], total)

## 활성 패시브 목록 반환
func get_active_passives() -> Array:
	var result: Array = []
	for pid in burn_passives:
		result.append(pid)
	return result

## 특정 패시브 보유 여부
func has_passive(passive_id: String) -> bool:
	return burn_passives.has(passive_id)

## S58: Power milestone dramatic effect — screen flash + particle burst + text
func _play_power_milestone(passive_name: String, burn_count: int) -> void:
	if not is_inside_tree():
		return
	var tree = get_tree()
	if not tree or not tree.root:
		return
	# Create a CanvasLayer overlay for the milestone effect
	var overlay = CanvasLayer.new()
	overlay.layer = 120
	tree.root.add_child(overlay)

	# White flash screen
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 0.95, 0.7, 0.0)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(flash)

	# "Power Awakened" title label
	var title_lbl = Label.new()
	title_lbl.text = "POWER AWAKENED"
	title_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	title_lbl.set_anchors_preset(Control.PRESET_CENTER)
	title_lbl.position = Vector2(-200, -50)
	title_lbl.size = Vector2(400, 40)
	title_lbl.add_theme_font_size_override("font_size", 32)
	title_lbl.add_theme_color_override("font_color", Color(1.0, 0.85, 0.3))
	title_lbl.add_theme_color_override("font_outline_color", Color(0.2, 0.1, 0.0))
	title_lbl.add_theme_constant_override("outline_size", 4)
	title_lbl.modulate.a = 0.0
	title_lbl.scale = Vector2(0.5, 0.5)
	title_lbl.pivot_offset = Vector2(200, 20)
	title_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(title_lbl)

	# Passive name subtitle
	var sub_lbl = Label.new()
	sub_lbl.text = passive_name
	sub_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	sub_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	sub_lbl.set_anchors_preset(Control.PRESET_CENTER)
	sub_lbl.position = Vector2(-200, 0)
	sub_lbl.size = Vector2(400, 30)
	sub_lbl.add_theme_font_size_override("font_size", 18)
	sub_lbl.add_theme_color_override("font_color", Color(0.9, 0.75, 0.4, 0.0))
	sub_lbl.add_theme_color_override("font_outline_color", Color(0.1, 0.08, 0.0))
	sub_lbl.add_theme_constant_override("outline_size", 2)
	sub_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(sub_lbl)

	# Burn count indicator
	var count_lbl = Label.new()
	count_lbl.text = "%d memories burned" % burn_count
	count_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	count_lbl.set_anchors_preset(Control.PRESET_CENTER)
	count_lbl.position = Vector2(-200, 30)
	count_lbl.size = Vector2(400, 20)
	count_lbl.add_theme_font_size_override("font_size", 12)
	count_lbl.add_theme_color_override("font_color", Color(0.7, 0.6, 0.4, 0.0))
	count_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(count_lbl)

	# Particle burst — radial gold particles
	var particles = GPUParticles2D.new()
	particles.position = Vector2(640, 360)  # Screen center
	particles.emitting = false
	particles.one_shot = true
	particles.amount = 40
	particles.lifetime = 1.2
	particles.explosiveness = 0.9
	var pmat = ParticleProcessMaterial.new()
	pmat.direction = Vector3(0, -1, 0)
	pmat.spread = 180.0
	pmat.initial_velocity_min = 80.0
	pmat.initial_velocity_max = 200.0
	pmat.gravity = Vector3(0, 40, 0)
	pmat.scale_min = 2.0
	pmat.scale_max = 5.0
	pmat.color = Color(1.0, 0.85, 0.3, 0.9)
	var color_ramp = Gradient.new()
	color_ramp.add_point(0.0, Color(1.0, 0.9, 0.4, 1.0))
	color_ramp.add_point(0.7, Color(1.0, 0.6, 0.2, 0.6))
	color_ramp.add_point(1.0, Color(0.8, 0.4, 0.1, 0.0))
	pmat.color_ramp = GradientTexture1D.new()
	pmat.color_ramp.gradient = color_ramp
	particles.process_material = pmat
	overlay.add_child(particles)

	# Animate sequence
	# 1. Flash in
	var tw = overlay.create_tween()
	tw.tween_property(flash, "color:a", 0.6, 0.15)
	tw.tween_property(flash, "color:a", 0.0, 0.4).set_ease(Tween.EASE_OUT)
	# 2. Title scale-in
	var tw2 = overlay.create_tween()
	tw2.tween_property(title_lbl, "modulate:a", 1.0, 0.2)
	tw2.parallel().tween_property(title_lbl, "scale", Vector2(1.0, 1.0), 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	# 3. Subtitle fade in (delayed)
	var tw3 = overlay.create_tween()
	tw3.tween_interval(0.3)
	tw3.tween_property(sub_lbl, "theme_override_colors/font_color", Color(0.9, 0.75, 0.4, 1.0), 0.3)
	tw3.parallel().tween_property(count_lbl, "theme_override_colors/font_color", Color(0.7, 0.6, 0.4, 1.0), 0.3)
	# 4. Fire particles
	particles.emitting = true
	# Play SFX
	AudioManager.play_sfx("heal")
	# 5. Hold, then fade out everything
	var tw4 = overlay.create_tween()
	tw4.tween_interval(2.0)
	tw4.tween_property(overlay, "modulate:a", 0.0, 0.5).set_ease(Tween.EASE_IN)
	tw4.tween_callback(overlay.queue_free)
	print("[MemoryManager] S58: Power milestone displayed — %s (%d burns)" % [passive_name, burn_count])

## 세이브용 데이터 내보내기
func export_data() -> Dictionary:
	var data = {
		"memories": [],
		"burned": [],
		"burn_passives": burn_passives.duplicate(),
	}
	for m in memories:
		data.memories.append({
			"id": m.id, "title": m.title, "description": m.description,
			"grade": m.grade, "burn_power": m.burn_power,
			"is_burned": m.is_burned, "is_residue": m.is_residue,
			"is_faded": m.is_faded, "erosion": m.erosion,
			"story_effect": m.story_effect, "related_npc": m.related_npc
		})
	for m in burned_memories:
		data.burned.append(m.id)
	return data

## 세이브 데이터 불러오기
func import_data(data: Dictionary) -> void:
	if not data.has("memories") or not (data.memories is Array):
		return

	memories.clear()
	burned_memories.clear()
	burn_passives = data.get("burn_passives", {})

	var burned_ids = data.get("burned", [])
	for m_data in data.memories:
		if not (m_data is Dictionary) or not m_data.has("id"):
			continue
		var memory_id := String(m_data.get("id", ""))
		if memory_id == "":
			continue
		var m = Memory.new(
			memory_id,
			String(m_data.get("title", memory_id)),
			String(m_data.get("description", "")),
			int(m_data.get("grade", MemoryGrade.GRADE_5)),
			int(m_data.get("burn_power", 0)),
			m_data.get("story_effect", ""), m_data.get("related_npc", "")
		)
		m.is_burned = bool(m_data.get("is_burned", burned_ids.has(memory_id)))
		m.is_residue = m_data.get("is_residue", false)
		m.is_faded = m_data.get("is_faded", false)
		m.erosion = m_data.get("erosion", 0)
		memories.append(m)
		if m.is_burned:
			burned_memories.append(m)
	if memories.is_empty():
		_init_starting_memories()
	_refresh_connections()

	print("[MemoryManager] Imported — %d memories, %d burned" % [memories.size(), burned_memories.size()])
