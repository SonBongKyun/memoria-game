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
	var story_effect: String  # 연소 시 스토리 영향 설명
	var related_npc: String   # 관련 NPC (빈 문자열이면 없음)

	func _init(p_id: String, p_title: String, p_desc: String, p_grade: int, p_power: int, p_effect: String = "", p_npc: String = "") -> void:
		id = p_id
		title = p_title
		description = p_desc
		grade = p_grade
		burn_power = p_power
		is_burned = false
		is_residue = false
		story_effect = p_effect
		related_npc = p_npc

# --- 기억 저장소 (아렐의 서고) ---
var memories: Array[Memory] = []
var burned_memories: Array[Memory] = []  # 연소된 기억 기록

# --- 시그널 ---
signal memory_burned(memory: Memory)
signal memory_added(memory: Memory)
signal memory_became_residue(memory: Memory)

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

## 기억 추가
func add_memory(memory: Memory) -> void:
	memories.append(memory)
	memory_added.emit(memory)

## 기억 연소
func burn_memory(memory_id: String) -> Memory:
	for i in range(memories.size()):
		if memories[i].id == memory_id and not memories[i].is_burned:
			var memory = memories[i]
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
			return memory

	return null

## 특정 등급의 사용 가능한 기억 목록
func get_available_memories(min_grade: int = MemoryGrade.GRADE_5) -> Array[Memory]:
	var available: Array[Memory] = []
	for memory in memories:
		if not memory.is_burned and memory.grade >= min_grade:
			available.append(memory)
	return available

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

## 세이브용 데이터 내보내기
func export_data() -> Dictionary:
	var data = {
		"memories": [],
		"burned": []
	}
	for m in memories:
		data.memories.append({
			"id": m.id, "title": m.title, "description": m.description,
			"grade": m.grade, "burn_power": m.burn_power,
			"is_burned": m.is_burned, "is_residue": m.is_residue,
			"story_effect": m.story_effect, "related_npc": m.related_npc
		})
	for m in burned_memories:
		data.burned.append(m.id)
	return data

## 세이브 데이터 불러오기
func import_data(data: Dictionary) -> void:
	if not data.has("memories"):
		return

	memories.clear()
	burned_memories.clear()

	var burned_ids = data.get("burned", [])
	for m_data in data.memories:
		var m = Memory.new(
			m_data.id, m_data.title, m_data.description,
			m_data.grade, m_data.burn_power,
			m_data.get("story_effect", ""), m_data.get("related_npc", "")
		)
		m.is_burned = m_data.get("is_burned", false)
		m.is_residue = m_data.get("is_residue", false)
		memories.append(m)
		if m.is_burned:
			burned_memories.append(m)

	print("[MemoryManager] Imported — %d memories, %d burned" % [memories.size(), burned_memories.size()])
