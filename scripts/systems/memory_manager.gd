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

# --- S234: 기억 한국어 텍스트 ---
#
# 이 게임의 중심 결정은 "무엇을 태울 것인가"다. 그런데 한국어판에서도 기억의 제목과
# 설명만 영어로 남아 있었다. 연소 확인창, 전투 로그, 서고, 상점, 관리국 감지 로그까지
# 전부. 무엇을 잃는지 읽지 못하면 선택의 무게도 없다.
#
# Memory 클래스에 필드를 세 개 더 붙이는 대신 id로 찾는 표를 둔다.
# 40개 기억 정의를 건드리지 않고, 새 기억을 추가할 때 여기에 한 줄만 더하면 된다.
const MEMORY_TEXT_KO: Dictionary = {
	"sense_forest_smell": {
		"title": "비 온 뒤의 숲",
		"desc": "빗물을 머금은 흙냄새. 어디였는지, 언제였는지는 말할 수 없다.",
	},
	"sense_warm_light": {
		"title": "창을 넘어온 따뜻한 빛",
		"desc": "나무 바닥 위로 쏟아지던 금빛. 어느 방이었는지는 떠오르지 않는다.",
	},
	"daily_market_food": {
		"title": "시장에서 먹던 길거리 음식",
		"desc": "꼬치에 꿴 향신료 바른 고기. 파는 이의 얼굴은 사라졌고 맛만 남았다.",
		"effect": "베르단의 음식 상인을 알아보지 못하게 된다.",
	},
	"daily_campfire_song": {
		"title": "모닥불 곁의 노래",
		"desc": "누군가 노래하고 있었다. 선율은 또렷한데 목소리에 얼굴이 없다.",
		"effect": "엘리아의 허밍이 더 이상 마음을 가라앉히지 못한다.",
	},
	"rel_hand_reaching": {
		"title": "뻗어 나가던 손",
		"desc": "멈추기도 전에 손이 누군가를 향한다. 그 몸짓은 이유에 대한 기억보다 오래됐다.",
		"effect": "엘리아 곁에서 남던 잔존의 흔적을 잃는다. 그녀는 알아차린다.",
	},
	"identity_first_sword": {
		"title": "처음 검을 쥐던 날",
		"desc": "안뜰. 먼지. 내 것보다 큰 손이 나무 손잡이 위로 손가락을 감싸 쥐었다. 아직 감촉으로 남은, 가장 중요한 순간.",
		"effect": "전투 자세가 바뀐다. 기본 공격이 단순해진다. 말렛이 원하는 값.",
	},
	"core_name_origin": {
		"title": "'아렐'이라는 이름",
		"desc": "누군가 이 이름을 주었다. 누구인지는 모른다. 그런데 그 이름을 들으면 가슴 안쪽이 대답한다.",
		"effect": "엔딩 분기: 이 기억을 태우면 영점 연소의 길로 들어선다.",
	},
	"sense_dead_soil": {
		"title": "죽은 흙의 맛",
		"desc": "아무 맛도 나지 않는 먼지. 벨트는 목적을 잊은 길처럼 계속 이어진다.",
	},
	"rel_tobias_records": {
		"title": "전부 받아 적는 남자",
		"desc": "잉크에 물든 손가락. 장부를 너무 많이 본 코 위의 안경. 그는 죽어 가는 것들을 기록한다.",
		"effect": "토비아스의 설명을 떠올리지 못하게 된다. 관리국 용어가 흐려진다.",
	},
	"sense_ash_rain": {
		"title": "비가 아닌 비",
		"desc": "피부에 자국을 남기는 잿빛 방울. 누군가의 연소가 남긴 잔재가 바람에 실려 온다.",
	},
	"daily_elia_hands": {
		"title": "차가운 손바닥 위의 따뜻한 손",
		"desc": "그녀가 손을 잡자 무언가 제자리를 찾았다. 뜯겨 나가던 페이지가 다시 눌려 붙었다. 읽을 수 있게 됐다.",
		"effect": "붙들려 있다는 감각을 잃는다. 구조가 헐거워진다.",
	},
	"sense_salt_wind": {
		"title": "절벽의 소금 바람",
		"desc": "무너져 내리는 해안에서 올라오는 짠 내. 차갑고 정직하다.",
	},
	"daily_elia_walking": {
		"title": "누군가와 나란히 걷기",
		"desc": "두 사람 몫의 발소리. 내 것과, 조금 더 가벼운 것. 그 박자가 익숙하다.",
		"effect": "엘리아의 걸음 습관을 알아채지 못하게 된다.",
	},
	"rel_sable_trust": {
		"title": "돌아온 여자",
		"desc": "그녀는 보이드 홀로 걸어 들어갔다가 걸어 나왔다. 대가로 두 눈이 남았다. 그녀가 말하면 믿게 된다.",
		"effect": "세이블을 향한 본능적 신뢰를 잃는다. 그녀의 조언이 공허하게 들린다.",
	},
	"sense_seam_colors": {
		"title": "있을 수 없는 색",
		"desc": "호박색. 진홍. 아플 만큼 깊은 초록. 더 섬은 잿빛 세계에 색을 흘린다.",
	},
	"daily_garden_flowers": {
		"title": "모든 계절의 꽃",
		"desc": "봄과 가을이 같은 흙을 나눠 쓰며 함께 핀다. 여기서는 시간이 제대로 흐르지 않는다.",
		"effect": "더 섬의 정원이 흑백으로 보인다.",
	},
	"sense_static_air": {
		"title": "정전기의 맛",
		"desc": "이 사이에서 딱딱 튀는 공기. 문턱은 끝내 터지지 않는 폭풍 같은 맛이 난다.",
	},
	"rel_echo_shell": {
		"title": "껍질 속의 목소리들",
		"desc": "수십 개의 조각. 한 여자가 누군가에게 경고했다. 한 남자는 제 손이 녹는 걸 느꼈다. 한 아이는 파랑을 잊었다.",
		"effect": "에코 셸의 보호를 잃는다. BL-07의 인력이 강해진다.",
	},
	"sense_hollow_trees": {
		"title": "나무였던 것을 기억하는 나무",
		"desc": "쓰러지는 법을 잊어서 서 있다. 껍질이 낡은 생각처럼 벗겨지고, 그 아래엔 아무것도 없다.",
	},
	"rel_ghost_words": {
		"title": "유령의 마지막 문장",
		"desc": "'나는...' 그게 전부였다. 나머지는 먹혔다. 나는 이미 몇 문장을 잃었을까.",
		"effect": "끝나지 않은 생각을 알아채지 못하게 된다. 말의 구멍이 그냥 지나간다.",
	},
	"sense_no_color": {
		"title": "색이 멈춘 자리",
		"desc": "회색이 아니다. 색이라는 개념 자체의 부재다. 색을 떠올리는 일조차 여기서는 멀다.",
	},
	"identity_compass": {
		"title": "기억 나침반",
		"desc": "북쪽을 가리키지 않는다. 곧 잊게 될 것을 가리킨다. BL-07에 가까워질수록 바늘이 빨라진다.",
		"effect": "다가오는 상실을 감지하지 못하게 된다. 연소가 예고 없이 일어난다.",
	},
	"identity_void_walker": {
		"title": "BL-07 안에서 본 것",
		"desc": "공간과 공간 사이. 소리가 아닌 소리. 찢어진 틈으로 무언가가 이쪽을 마주 보았다.",
		"effect": "보이드 홀을 감지하지 못하게 된다. 오직 직감으로만 길을 찾는다.",
	},
	"daily_belt_footsteps": {
		"title": "서쪽으로 데려간 보폭",
		"desc": "엘리아는 며칠을 이 보폭에 맞춰 걸었다. 천천히 가자는 말은 한 번도 하지 않았다.",
		"effect": "엘리아가 돌아보지 않고도 따라올 수 있게 하던 공통의 박자를 잃는다.",
	},
	"rel_elia_bloodwork": {
		"title": "그녀가 숨긴 피",
		"desc": "엘리아의 장갑에 그어진 붉은 선. 공책에 베였을 뿐이라고 했다.",
		"effect": "빈 페이지를 읽는 일이 그녀에게 무엇을 치르게 하는지 처음 알았던 순간을 잃는다.",
	},
	"rel_verdan_faces": {
		"title": "베르단 인파 속의 얼굴들",
		"desc": "눈을 돌리던 상인들, 손가락질하던 아이, 길을 열어 주던 손들.",
		"effect": "베르단 사람들이 개인으로 남지 않는다. 시장은 지형이 된다.",
	},
	"rel_tobias_alliance": {
		"title": "기록 위에 놓인 세 번째 손",
		"desc": "토비아스가 추궁을 멈추고, 자기 공책을 엘리아의 것 옆에 내려놓았다.",
		"effect": "의심이 동맹으로 바뀐 정확한 순간을 잃는다.",
	},
	"identity_intervention_vow": {
		"title": "개입하겠다는 맹세",
		"desc": "추출을 지켜보고 살아남는 것만으로는 부족하다고 결정했다.",
		"effect": "관리국과 그 희생자 사이에 몸을 세우는 이유를 잃는다.",
	},
	"daily_unfinished_lullaby": {
		"title": "마지막 음이 없는 자장가",
		"desc": "한은 빠진 음 주위를 맴돌며 흥얼거렸고, 엘리아가 그 모양을 알아봤다.",
		"effect": "한과 엘리아와 셀라를 처음 묶어 준 선율을 잃는다.",
	},
	"rel_han_mneme": {
		"title": "아르케인 아래의 가수",
		"desc": "흉터가 남은 목, 석판 하나, 그리고 진동이 되어 살아남은 목소리.",
		"effect": "한이 에코 셸을 깨울 수 있게 하던 공명을 잃는다.",
	},
	"daily_eastward_route": {
		"title": "새벽에 고른 길",
		"desc": "토비아스가 안전한 수로를 표시했다. 엘리아가 지도를 접었다. 우리는 동쪽을 골랐다.",
		"effect": "지나온 길을 잃는다. 목적지만 남는다.",
	},
	"daily_three_shadows": {
		"title": "둑길 위의 세 그림자",
		"desc": "한 아침 동안, 길 위에 같은 속도로 움직이는 그림자가 셋이었다.",
		"effect": "동쪽 길을 함께 건넜다는 평범한 확신을 잃는다.",
	},
	"identity_witness_record": {
		"title": "증언을 남길 의무",
		"desc": "여기서 무슨 일이 있었는지 말할 수 있을 만큼은 누군가 살아남아야 한다. 그 의무를 받아들였다.",
		"effect": "목격된 기억이 책임이 된다는 믿음을 잃는다.",
	},
	"sense_white_noon": {
		"title": "첫 번째 하얀 정오",
		"desc": "루메아를 처음 본 순간. 그림자 하나 드리우지 않을 만큼 흰 도시가, 숨을 참듯 정오를 붙들고 있었다.",
	},
	"rel_vael_reflex": {
		"title": "주인 없는 방어 자세",
		"desc": "낯선 이의 걸음에 몸이 먼저 방어 자세를 잡는다. 반사는 살아남았고, 그것이 지키던 사람은 남지 않았다.",
		"effect": "몸이 보내던 경고를 잃는다. 바엘이 예고 없이 다가올 수 있다.",
	},
	"sense_monolith_hum": {
		"title": "청각 아래의 울림",
		"desc": "모놀리스 내부가 끝내 완성되지 않은 문장의 높이로 떨린다. 이가 그 진동을 기억한다.",
	},
	"rel_sable_ledger": {
		"title": "기름먹인 천에 싸인 열일곱 이름",
		"desc": "전령이 어둠 밖으로 들고 나온 세이블의 장부. 그녀가 끝내 세기를 멈추지 않은 열일곱 이름. 이제 그 장부가 그녀를 센다.",
		"effect": "그 셈의 무게를 잃는다. 장부는 종이가 된다.",
	},
	"identity_reverse_burn": {
		"title": "거꾸로 흐른 불",
		"desc": "기억을 바깥으로 밀어냈는데 타지 않고 건너갔다. 내가 무엇이든, 그것은 양방향으로 흐른다.",
		"effect": "주는 일과 태우는 일이 같은 근육이라는 앎을 잃는다.",
	},
	"rel_kairos_doubt": {
		"title": "보고를 멈춘 편집자",
		"desc": "페이지마다 보류된 셈. 그는 서류로 나를 지켰고, 그것을 오류라고 불렀다.",
		"effect": "체계 자신의 손도 망설일 수 있다는 증거를 잃는다.",
	},
	"identity_relay_promise": {
		"title": "펼친 책 위에 놓인 그녀의 손",
		"desc": "핵에 선 엘리아가, 모두를 한꺼번에 붙들겠다고 했다. 무엇이라 답했든, 그 질문을 받았다는 사실은 지고 간다.",
		"effect": "그녀가 내민 제안의 무게를 잃는다. 릴레이는 기계가 된다.",
	},
}

## S234: 합성 결과도 같은 표를 따른다.
const SYNTHESIS_NAMES_KO: Dictionary = {
	MemoryGrade.GRADE_4: {"title": "뒤섞인 감각", "desc": "바래 가던 인상 둘이 녹아 붙어 더 짙어졌다. 이제 결이 또렷하다."},
	MemoryGrade.GRADE_3: {"title": "엮인 일상", "desc": "일상의 조각들이 서로 감겼다. 있는 줄도 몰랐던 습관 하나."},
	MemoryGrade.GRADE_2: {"title": "묶인 관계", "desc": "관계가 하나의 통증으로 압축됐다. 더 무겁지만, 더 분명하다."},
	MemoryGrade.GRADE_1: {"title": "벼려진 정체성", "desc": "남기기로 고른 것들에서 뽑아낸, 나라는 것의 핵."},
}

# --- 합성 결과 이름 ---
const SYNTHESIS_NAMES: Dictionary = {
	# grade_value → 합성 결과 제목/설명 템플릿
	MemoryGrade.GRADE_4: {"title": "Blended Sensation", "desc": "Two fading impressions fused into something richer. The detail is sharper now."},
	MemoryGrade.GRADE_3: {"title": "Woven Routine", "desc": "Daily fragments entwined, a habit you didn't know you had."},
	MemoryGrade.GRADE_2: {"title": "Bound Connection", "desc": "Relationships compressed into a single ache. Heavier, but clearer."},
	MemoryGrade.GRADE_1: {"title": "Forged Identity", "desc": "The core of who you are, distilled from what you chose to keep."},
}

# --- Burn Passives (Skill Tree) ---
var burn_passives: Dictionary = {}  # {passive_name: true}

const PASSIVE_THRESHOLDS: Dictionary = {
	5: {"id": "ember_affinity", "name": "Ember Affinity", "name_ko": "잔불 친화", "desc": "+10% burn damage", "desc_ko": "연소 피해 +10%", "icon": "flame"},
	10: {"id": "residual_warmth", "name": "Residual Warmth", "name_ko": "남은 온기", "desc": "+5 HP heal after burn", "desc_ko": "연소 후 HP 5 회복", "icon": "heart"},
	20: {"id": "ash_sight", "name": "Ash Sight", "name_ko": "잿빛 시야", "desc": "See enemy HP numbers", "desc_ko": "적 HP 수치 표시", "icon": "eye"},
	30: {"id": "void_touch", "name": "Void Touch", "name_ko": "공허 접촉", "desc": "+15% void damage", "desc_ko": "보이드 피해 +15%", "icon": "skull"},
	50: {"id": "memory_cascade", "name": "Memory Cascade", "name_ko": "메모리 캐스케이드", "desc": "Limit break charges 20% faster", "desc_ko": "리밋 충전 20% 가속", "icon": "star"},
}

# --- S231: 기억 파수 (Anchor Vigil) ---
#
# 연소 패시브의 첫 문턱은 5회다. The Weave(제3의 길)는 총 연소 4회 미만을 요구한다.
# 두 숫자가 겹치지 않아서, 보존을 택한 플레이어는 게임 전체에서 패시브를 하나도
# 얻지 못했다. 연소는 동시에 주 딜 수단이고 적은 챕터마다 강해지므로,
# "지키는 길"은 주제적으로만 길이고 기계적으로는 그냥 열등한 빌드였다.
#
# 파수는 연소 카운터의 거울이다. 태운 횟수가 아니라 "온전하게 지켜 낸 앵커 × 챕터"로
# 쌓이고, 태우지 않는 플레이(WITNESS, BREAK, 파티 지시, 방어)를 강화한다.
# 이제 두 철학은 두 개의 빌드다.
var anchor_vigil: int = 0
var anchor_passives: Dictionary = {}
var _vigil_chapters_counted: Dictionary = {}  # 챕터당 한 번만 적립/침식

const VIGIL_PER_ANCHOR: int = 2
const VIGIL_PRIMARY_BONUS: int = 3

const ANCHOR_THRESHOLDS: Dictionary = {
	8: {
		"id": "quiet_focus", "name": "Quiet Focus", "name_ko": "고요한 집중",
		"desc": "WITNESS needs one less echo", "desc_ko": "기억 읽기 요구 1 감소", "icon": "eye",
	},
	20: {
		"id": "steady_hand", "name": "Steady Hand", "name_ko": "흔들리지 않는 손",
		"desc": "+30% BREAK pressure from unburned attacks", "desc_ko": "비연소 공격의 브레이크 압력 +30%", "icon": "flame",
	},
	36: {
		"id": "unbroken_edge", "name": "Unbroken Edge", "name_ko": "무뎌지지 않는 날",
		"desc": "Your blade bites the void far better", "desc_ko": "보이드수에 대한 일반 공격 감쇠 완화", "icon": "skull",
	},
	56: {
		"id": "shared_burden", "name": "Shared Burden", "name_ko": "나눠 진 무게",
		"desc": "Elia's techniques recover one turn sooner", "desc_ko": "엘리아 기술 쿨다운 1턴 감소", "icon": "heart",
	},
	80: {
		"id": "deep_anchor", "name": "Deep Anchor", "name_ko": "깊은 닻",
		"desc": "Each intact anchor blunts incoming damage", "desc_ko": "온전한 앵커 하나당 받는 피해 감소", "icon": "star",
	},
}

## deep_anchor: 온전한 앵커 하나당 받는 피해 3% 감소 (이름 포함 최대 15%).
const DEEP_ANCHOR_STEP: float = 0.03

# --- S231: 기억 고정 (Erosion Guard) ---
#
# 침식은 챕터마다 무조건 깎였고 대항 수단이 엘리아 동행 하나뿐이었다.
# 태우면 사라지고 아껴도 삭으니, 보존 플레이어는 그냥 가만히 지는 쪽이었다.
# 이제 그레인을 써서 챕터마다 몇 개를 지킬 수 있다. 전부는 못 지킨다.
# 매 챕터 "이번엔 무엇을 지킬 것인가"라는 분류 결정이 생긴다.
const EROSION_GUARD_SLOTS: int = 2
## 등급이 높을수록 붙잡아 두는 값이 비싸다. (Grade 5 -> Grade 1 순)
const EROSION_GUARD_COST: Array = [8, 14, 22, 34, 50]
var erosion_guarded: Dictionary = {}   # memory_id -> true, 다음 침식 1회 면제
var _guard_slots_used: int = 0

# --- 시그널 ---
signal memory_burned(memory: Memory)
signal memory_added(memory: Memory)
signal memory_became_residue(memory: Memory)
signal memory_synthesized(result: Memory, consumed_a: Memory, consumed_b: Memory)
signal memory_faded(memory: Memory)
signal memories_eroded(count: int)
signal passive_unlocked(passive_name: String)
signal anchor_passive_unlocked(passive_name: String)
signal anchor_vigil_changed(vigil: int, gained: int)
signal memory_guarded(memory: Memory)
signal guard_slots_changed(remaining: int)
signal loan_changed(loan: Dictionary)
signal memory_extracted(memory: Memory)
signal memory_cascaded(source: Memory, affected: Array, amount: int)

func _ready() -> void:
	_init_starting_memories()
	print("[MemoryManager] Initialized, %d memories loaded" % memories.size())

## 초기 기억 세팅 (Chapter 1 시작 시)
func _init_starting_memories() -> void:
	# Grade 5, 감각 잔편
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

	# Grade 4, 일상 기억
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

	# Grade 3, 관계 기억
	add_memory(Memory.new(
		"rel_hand_reaching",
		"A Hand Reaching Out",
		"Your hand moves toward someone before you can stop it. The gesture is older than your memory of why.",
		MemoryGrade.GRADE_3, 50,
		"Lose the 'Residue' animation with Elia. She notices.",
		"Elia"
	))

	# Grade 2, 정체성 기억
	add_memory(Memory.new(
		"identity_first_sword",
		"The Day You First Held a Sword",
		"A courtyard. Dust. A hand larger than yours closing your fingers around a wooden grip. The most important moment you can still feel.",
		MemoryGrade.GRADE_2, 100,
		"Combat stance changes. Base attack pattern simplified. Malet's price.",
		"Unknown"
	))

	# Grade 1, 핵심 기억 (게임 후반부)
	add_memory(Memory.new(
		"core_name_origin",
		"The Name 'Arrel'",
		"Someone gave you this name. You don't remember who. But when you hear it, something in your chest responds.",
		MemoryGrade.GRADE_1, 999,
		"ENDING CRITICAL: Burning this memory triggers the Zero Burn ending path.",
		"Elia"
	))

## 기억 침식, 챕터 진행 시 미연소 기억이 서서히 바래짐
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
		# S231: 이번 챕터에 고정해 둔 기억은 한 번 넘어간다. 보호는 여기서 소모된다.
		if erosion_guarded.has(m.id):
			erosion_guarded.erase(m.id)
			print("[MemoryManager] Guard held: %s" % m.title)
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
			NotificationToast.show_toast(
				("기억이 흐려진다: %s" if GameManager.current_locale == "ko" else "Memory fading: %s") % localized_memory_title(m),
				NotificationToast.ToastType.WARNING
			)
			print("[MemoryManager] FADED: %s (erosion %d/%d)" % [m.title, m.erosion, m.burn_power])
	if eroded_count > 0:
		memories_eroded.emit(eroded_count)
		print("[MemoryManager] Erosion applied, %d memories affected (ch%d, +%d)" % [eroded_count, chapter, base_amount])

## 유효 연소력 계산 (침식 반영)
func get_effective_burn_power(memory: Memory) -> int:
	return maxi(1, memory.burn_power - memory.erosion)

## 침식 비율 (UI 표시용)
func get_erosion_ratio(memory: Memory) -> float:
	if memory.burn_power <= 0:
		return 0.0
	return clampf(float(memory.erosion) / float(memory.burn_power), 0.0, 1.0)

## 비전투 연소 (탐색 공명 이벤트용, 소리 없이)
func burn_memory_silent(memory_id: String, allow_faded: bool = false) -> Memory:
	for i in range(memories.size()):
		if memories[i].id == memory_id and not memories[i].is_burned:
			var memory = memories[i]
			if memory.is_faded and not allow_faded:
				push_warning("[MemoryManager] Refused silent burn of faded memory: %s" % memory_id)
				return null
			if is_collateral(memory_id):
				push_warning("[MemoryManager] Refused silent burn of pledged collateral: %s" % memory_id)
				return null
			memory.is_burned = true
			burned_memories.append(memory)
			_apply_burn_cascade(memory)
			memory_burned.emit(memory)
			check_unlock_passives()
			print("[MemoryManager] SILENT BURN: %s" % memory.title)
			return memory
	return null

## 챕터 진행에 따른 기억 추가
func add_chapter_memories(chapter: int) -> void:
	# S231: 이 함수는 맵 진입에서도 불린다. 기억 추가는 원래 멱등했지만
	# 침식은 아니어서, 같은 챕터 맵을 다시 밟으면 또 깎였다. 챕터당 한 번으로 고정한다.
	if not _vigil_chapters_counted.has(chapter):
		_vigil_chapters_counted[chapter] = true
		# 챕터 진행에 따른 기존 기억 침식 적용
		if chapter >= 3:
			apply_erosion(chapter)
		# 그 챕터를 앵커를 지킨 채로 넘겼다면 파수가 쌓인다.
		accrue_anchor_vigil()
		# 새 챕터, 새 고정 슬롯.
		_guard_slots_used = 0
		guard_slots_changed.emit(guard_slots_remaining())
		# S232: 상환 기한이 지난 담보는 여기서 회수된다.
		check_loan_maturity(chapter)
	match chapter:
		3:
			# Ch3: Belt Waystation, Weight of Pages
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
			# Ch4: Drift Shelter, Drift
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
			# Ch5: Crumbling Coast, The Classifier
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
			# Ch6: The Seam, Thread That Holds
			if not _has_memory("rel_sable_trust"):
				add_memory(Memory.new(
					"rel_sable_trust",
					"The Woman Who Came Back",
					"She walked into a Void Hole and walked out. Her eyes stayed behind as the toll. When she speaks, you believe her.",
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
					"They bloom together, spring and autumn sharing the same soil. Time doesn't work right here.",
					MemoryGrade.GRADE_4, 28,
					"The Seam's gardens appear monochrome"
				))
		7:
			# Ch7: Seam Outskirts, The Other Side of the Flame
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
			# Ch8: Forgotten Forest, The Forest That Forgets
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
					"'I was...' That's all they could say. The rest was eaten. You wonder how many sentences you've already lost.",
					MemoryGrade.GRADE_4, 30,
					"Lose awareness of incomplete thoughts. Gaps in speech go unnoticed.",
					""
				))
		9:
			# Ch9: Colorless Waste, Where Colors Stop
			if not _has_memory("sense_no_color"):
				add_memory(Memory.new(
					"sense_no_color",
					"The Place Where Color Stopped",
					"Gray that isn't gray, it's the absence of the concept. Even memory of color feels distant here.",
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
		19:
			# S147 Ch19: The Approach, Lumea 외곽
			if not _has_memory("sense_white_noon"):
				add_memory(Memory.new(
					"sense_white_noon",
					"The First White Noon",
					"Your first sight of Lumea: a city so white it casts no shadow, holding noon in place like a held breath.",
					MemoryGrade.GRADE_5, 14
				))
			if not _has_memory("rel_vael_reflex"):
				add_memory(Memory.new(
					"rel_vael_reflex",
					"A Guard Stance With No Owner",
					"Your body drops into a defensive stance at a stranger's walk. The reflex survived. The person it protected did not.",
					MemoryGrade.GRADE_3, 55,
					"Lose the body's warning. Vael can approach unannounced.",
					"Vael"
				))
		20:
			# S147 Ch20: Into the Monolith, 기억의 바다
			if not _has_memory("sense_monolith_hum"):
				add_memory(Memory.new(
					"sense_monolith_hum",
					"The Hum Below Hearing",
					"The monolith's interior vibrates at the pitch of a sentence never finished. Your teeth remember it.",
					MemoryGrade.GRADE_5, 15
				))
			if not _has_memory("rel_sable_ledger"):
				add_memory(Memory.new(
					"rel_sable_ledger",
					"Seventeen Names in Oilcloth",
					"Sable's ledger, carried out of the dark by a runner. Seventeen names she never stopped counting. Now it counts her.",
					MemoryGrade.GRADE_3, 55,
					"Lose the weight of her count. The ledger becomes paper.",
					"Sable"
				))
			if not _has_memory("identity_reverse_burn"):
				add_memory(Memory.new(
					"identity_reverse_burn",
					"Fire Flowing Backward",
					"You pressed a memory outward and it arrived without burning. Whatever you are, it runs in both directions.",
					MemoryGrade.GRADE_2, 115,
					"Lose the knowledge that giving is the same muscle as burning.",
					""
				))
		21:
			# S147 Ch21: The Editor's Turn
			if not _has_memory("rel_kairos_doubt"):
				add_memory(Memory.new(
					"rel_kairos_doubt",
					"The Editor Who Stopped Reporting",
					"Page after page of withheld arithmetic. He protected you with paperwork, and called it an error.",
					MemoryGrade.GRADE_3, 55,
					"Lose the proof that the system's own hands can hesitate.",
					"Kairos"
				))
		22:
			# S147 Ch22: The Core
			if not _has_memory("identity_relay_promise"):
				add_memory(Memory.new(
					"identity_relay_promise",
					"Her Hand on the Open Book",
					"Elia at the core, offering to hold everyone at once. Whatever you answered, you will carry having been asked.",
					MemoryGrade.GRADE_2, 115,
					"Lose the weight of her offer. The relay becomes machinery.",
					"Elia"
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

## S62: Memory Constellation, 기억 간 연결 자동 생성
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
			# S232: 담보로 잡힌 기억은 아렐 것이 아니다. 갚기 전에는 태울 수 없다.
			if is_collateral(memory_id):
				push_warning("[MemoryManager] Refused burn of pledged collateral: %s" % memory_id)
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
			# S233: 이어진 기억이 함께 상한다. 어느 것을 태우는지가 중요해지는 지점.
			_apply_burn_cascade(memory)
			memory_burned.emit(memory)
			check_unlock_passives()
			return memory

	return null

## 특정 등급의 사용 가능한 기억 목록
## 연소/판매 후보 목록.
## S232: 담보로 잡힌 기억은 아렐이 처분할 수 있는 것이 아니므로 여기 나오지 않는다.
## (연소 목록, 판매 목록, 자동 전투가 모두 이 함수를 통해 후보를 얻는다.)
func get_available_memories(min_grade: int = MemoryGrade.GRADE_5, allow_faded: bool = false) -> Array[Memory]:
	var available: Array[Memory] = []
	for memory in memories:
		if memory.is_burned or is_collateral(memory.id):
			continue
		if (allow_faded or not memory.is_faded) and memory.grade >= min_grade:
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

## 기억 합성, 동일 등급 기억 2개 → 상위 등급 1개
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
		"Synthesized, cannot be undone"
	)

	# 원본 제거
	memories.erase(mem_a)
	memories.erase(mem_b)

	# 새 기억 추가
	add_memory(synth)
	memory_synthesized.emit(synth, mem_a, mem_b)
	NotificationToast.show_toast(
		("합성: %s" if GameManager.current_locale == "ko" else "Synthesized: %s") % localized_memory_title(synth),
		NotificationToast.ToastType.SUCCESS
	)
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
	# S232: 빼앗긴 기억은 세지 않는다. 힘은 스스로 태운 것에서만 온다.
	var total = get_voluntary_burn_count()
	for threshold in PASSIVE_THRESHOLDS:
		var passive = PASSIVE_THRESHOLDS[threshold]
		if total >= threshold and not burn_passives.has(passive["id"]):
			burn_passives[passive["id"]] = true
			passive_unlocked.emit(passive["name"])
			NotificationToast.show_toast("Passive Unlocked: %s" % passive["name"], NotificationToast.ToastType.SUCCESS)
			print("[MemoryManager] PASSIVE UNLOCKED: %s (burns: %d)" % [passive["name"], total])
			# S58: Power milestone, dramatic screen effect
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

## ===================== S231: 기억 파수 (Anchor Vigil) =====================

## 한 챕터를 넘길 때, 온전하게 지켜 낸 앵커만큼 파수가 쌓인다.
## 태운 횟수를 세는 연소 트리와 정확히 대칭이다.
func accrue_anchor_vigil() -> int:
	var gained := intact_anchor_count() * VIGIL_PER_ANCHOR
	if is_intact(WEAVE_PRIMARY):
		gained += VIGIL_PRIMARY_BONUS
	if gained <= 0:
		return 0
	anchor_vigil += gained
	anchor_vigil_changed.emit(anchor_vigil, gained)
	check_unlock_anchor_passives()
	print("[MemoryManager] Anchor vigil +%d (total %d, intact anchors %d)" % [gained, anchor_vigil, intact_anchor_count()])
	return gained

func check_unlock_anchor_passives() -> void:
	for threshold in ANCHOR_THRESHOLDS:
		var passive: Dictionary = ANCHOR_THRESHOLDS[threshold]
		if anchor_vigil >= int(threshold) and not anchor_passives.has(passive["id"]):
			anchor_passives[passive["id"]] = true
			var shown := localized_passive_name(passive)
			anchor_passive_unlocked.emit(shown)
			NotificationToast.show_toast(
				("파수 해금: %s" % shown) if GameManager.current_locale == "ko" else ("Vigil Unlocked: %s" % shown),
				NotificationToast.ToastType.SUCCESS
			)
			print("[MemoryManager] ANCHOR PASSIVE UNLOCKED: %s (vigil: %d)" % [passive["id"], anchor_vigil])

func has_anchor_passive(passive_id: String) -> bool:
	return anchor_passives.has(passive_id)

func get_active_anchor_passives() -> Array:
	return anchor_passives.keys()

## deep_anchor의 실제 값. 온전한 앵커(이름 포함)가 많을수록 크다.
func anchor_damage_reduction() -> float:
	if not has_anchor_passive("deep_anchor"):
		return 0.0
	var kept := intact_anchor_count()
	if is_intact(WEAVE_PRIMARY):
		kept += 1
	return clampf(float(kept) * DEEP_ANCHOR_STEP, 0.0, 0.5)

## 다음 문턱까지 얼마나 남았는지. 서고 UI가 진행도를 보여 줄 때 쓴다.
func next_anchor_threshold() -> int:
	var best := 0
	for threshold in ANCHOR_THRESHOLDS:
		var value := int(threshold)
		if anchor_vigil < value and (best == 0 or value < best):
			best = value
	return best

## ===================== S233: 기억 연쇄 (Constellation Cascade) =====================
#
# 기억 그래프(connections)는 S62부터 있었지만 실제 용도는 연쇄 연소 +20% 하나뿐이었다.
# 그래서 "무엇을 태울까"는 사실상 "가장 싼 걸 태운다"였다.
#
# 이제 태우면 이어진 기억이 함께 침식된다. 엘리아 관련 기억은 서로 묶여 있으므로
# 그 묶음을 건드리면 묶음 전체가 상한다. 외딴 기억을 태우면 값이 싸다.
# 서고가 소비하는 목록이 아니라 플레이하는 판이 된다.
const CASCADE_EROSION: Array = [2, 4, 7, 11, 0]  # Grade 5 -> Grade 1
## 빼앗기는 것은 스스로 태우는 것보다 거칠다.
const CASCADE_EXTRACTION_MULT: float = 1.5

## 이 기억을 태우면 함께 상하는 이웃들.
## 핵심 기억은 침식 면역이고, 이번 챕터에 고정해 둔 기억은 보호를 소모하며 버틴다.
func cascade_targets(memory: Memory) -> Array[Memory]:
	var targets: Array[Memory] = []
	if memory == null:
		return targets
	for neighbor_id in memory.connections:
		var neighbor := find_memory(String(neighbor_id))
		if neighbor == null or neighbor.is_burned or neighbor.is_faded:
			continue
		if neighbor.grade >= MemoryGrade.GRADE_1:
			continue
		targets.append(neighbor)
	return targets

func cascade_amount(memory: Memory, forced: bool = false) -> int:
	if memory == null:
		return 0
	var index := clampi(memory.grade, 0, CASCADE_EROSION.size() - 1)
	var amount := int(CASCADE_EROSION[index])
	if amount <= 0:
		return 0
	if forced:
		amount = int(round(amount * CASCADE_EXTRACTION_MULT))
	return amount

## 연소 확인창이 커밋 전에 보여 줄 예보. 숨은 대가를 만들지 않는다.
func cascade_preview(memory: Memory, forced: bool = false) -> Dictionary:
	var amount := cascade_amount(memory, forced)
	var targets := cascade_targets(memory)
	var names: Array[String] = []
	var guarded: Array[String] = []
	for target in targets:
		if erosion_guarded.has(target.id):
			guarded.append(localized_memory_title(target))
		else:
			names.append(localized_memory_title(target))
	return {"amount": amount, "titles": names, "guarded": guarded, "count": names.size()}

func _apply_burn_cascade(memory: Memory, forced: bool = false) -> int:
	var amount := cascade_amount(memory, forced)
	if amount <= 0:
		return 0
	var affected: Array[Memory] = []
	for target in cascade_targets(memory):
		# 고정해 둔 기억은 연쇄도 한 번 막아 낸다. 보호는 여기서도 소모된다.
		if erosion_guarded.has(target.id):
			erosion_guarded.erase(target.id)
			continue
		target.erosion += amount
		affected.append(target)
		if target.erosion >= int(target.burn_power * 0.7) and not target.is_faded:
			target.is_faded = true
			memory_faded.emit(target)
	if affected.is_empty():
		return 0
	memory_cascaded.emit(memory, affected, amount)
	print("[MemoryManager] CASCADE from %s: %d linked memories +%d erosion" % [memory.title, affected.size(), amount])
	return affected.size()

## ===================== S232: 기억 대출 (GDD 2.4) =====================
#
# 지금까지 연소 결정에는 "태운다 / 안 태운다" 둘뿐이었다.
# 대출은 시간 축을 넣는다. 지금은 태우지 않고 그레인을 받되, 갚지 못하면
# 관리국이 담보를 가져간다. 그때 아렐은 기억을 잃고도 아무 힘을 얻지 못한다.
# 자기가 태우는 것과 빼앗기는 것의 차이가 이 시스템의 요점이다.
const LOAN_PRINCIPAL: Array = [14, 26, 44, 70, 0]  # Grade 5 -> Grade 1 (핵심 기억은 담보 불가)
const LOAN_INTEREST: float = 0.4
const LOAN_TERM_CHAPTERS: int = 2

var active_loan: Dictionary = {}        # {memory_id, principal, repay, due_chapter}
var extracted_memories: Array[String] = []  # 강제 추출로 잃은 기억 (연소 패시브 적립 제외)

func loan_offer(memory: Memory) -> Dictionary:
	if memory == null:
		return {}
	var index := clampi(memory.grade, 0, LOAN_PRINCIPAL.size() - 1)
	var principal := int(LOAN_PRINCIPAL[index])
	if principal <= 0:
		return {}
	return {
		"memory_id": memory.id,
		"principal": principal,
		"repay": int(round(principal * (1.0 + LOAN_INTEREST))),
		"due_chapter": GameManager.current_chapter + LOAN_TERM_CHAPTERS,
	}

func has_active_loan() -> bool:
	return not active_loan.is_empty()

func is_collateral(memory_id: String) -> bool:
	return has_active_loan() and String(active_loan.get("memory_id", "")) == memory_id

## 담보로 잡을 수 있는지와 그 이유.
func loan_availability(memory: Memory) -> Dictionary:
	var is_ko := GameManager.current_locale == "ko"
	if memory == null:
		return {"ok": false, "reason": "없음" if is_ko else "No memory"}
	if has_active_loan():
		return {"ok": false, "reason": "이미 대출 중" if is_ko else "A loan is already open"}
	if memory.is_burned:
		return {"ok": false, "reason": "이미 태워짐" if is_ko else "Already burned"}
	if memory.grade >= MemoryGrade.GRADE_1:
		return {"ok": false, "reason": "핵심 기억은 담보로 잡히지 않음" if is_ko else "Core memories cannot be pledged"}
	var offer := loan_offer(memory)
	if offer.is_empty():
		return {"ok": false, "reason": "담보 불가" if is_ko else "Cannot be pledged"}
	return {"ok": true, "reason": "", "offer": offer}

func take_loan(memory_id: String) -> bool:
	var memory := find_memory(memory_id)
	var availability := loan_availability(memory)
	if not bool(availability.get("ok", false)):
		return false
	var offer: Dictionary = availability["offer"]
	active_loan = offer.duplicate()
	GameManager.player_data["grains"] = int(GameManager.player_data.get("grains", 0)) + int(offer["principal"])
	loan_changed.emit(active_loan)
	SystemLog.show_log(
		("담보 등록: %s. %d장까지 %d 그레인 상환." % [localized_memory_title(memory), int(offer["due_chapter"]), int(offer["repay"])]) if GameManager.current_locale == "ko"
		else ("Collateral registered: %s. Repay %d Grains by Chapter %d." % [localized_memory_title(memory), int(offer["repay"]), int(offer["due_chapter"])])
	)
	print("[MemoryManager] LOAN: %s for %d (repay %d by ch%d)" % [memory.title, int(offer["principal"]), int(offer["repay"]), int(offer["due_chapter"])])
	return true

func can_repay_loan() -> bool:
	if not has_active_loan():
		return false
	return int(GameManager.player_data.get("grains", 0)) >= int(active_loan.get("repay", 0))

func repay_loan() -> bool:
	if not can_repay_loan():
		return false
	var repay := int(active_loan.get("repay", 0))
	var memory := find_memory(String(active_loan.get("memory_id", "")))
	GameManager.player_data["grains"] = int(GameManager.player_data.get("grains", 0)) - repay
	active_loan = {}
	loan_changed.emit(active_loan)
	var shown := localized_memory_title(memory) if memory != null else "?"
	NotificationToast.show_toast(
		("담보 회수: %s" % shown) if GameManager.current_locale == "ko" else ("Collateral released: %s" % shown),
		NotificationToast.ToastType.SUCCESS
	)
	print("[MemoryManager] LOAN REPAID: %s (-%d grains)" % [shown, repay])
	return true

## 만기 검사. 챕터가 넘어갈 때 한 번만 불린다.
func check_loan_maturity(chapter: int) -> bool:
	if not has_active_loan():
		return false
	if chapter <= int(active_loan.get("due_chapter", 0)):
		return false
	return extract_collateral()

## 강제 추출. 태우는 것과 다르다.
## 기억은 사라지지만 연소 위력도, 연소 패시브 적립도 없다.
func extract_collateral() -> bool:
	if not has_active_loan():
		return false
	var memory := find_memory(String(active_loan.get("memory_id", "")))
	active_loan = {}
	loan_changed.emit(active_loan)
	if memory == null or memory.is_burned:
		return false
	memory.is_burned = true
	memory.is_residue = false
	burned_memories.append(memory)
	extracted_memories.append(memory.id)
	# S233: 뜯어 가는 쪽이 더 거칠다.
	_apply_burn_cascade(memory, true)
	memory_extracted.emit(memory)
	# 세계는 여전히 이 부재를 기록한다. 잃은 방식만 다를 뿐이다.
	memory_burned.emit(memory)
	SystemLog.show_log(
		("강제 추출: %s. 상환 기한이 지났다." % localized_memory_title(memory)) if GameManager.current_locale == "ko"
		else ("Forced extraction: %s. The term expired." % localized_memory_title(memory))
	)
	print("[MemoryManager] EXTRACTED (no power gained): %s" % memory.title)
	return true

## 연소 패시브는 "스스로 태운" 횟수만 센다.
## 빼앗긴 기억은 아무 힘도 남기지 않는다.
func get_voluntary_burn_count() -> int:
	return maxi(0, burned_memories.size() - extracted_memories.size())

## ===================== S231: 기억 고정 =====================

func guard_slots_remaining() -> int:
	return maxi(0, EROSION_GUARD_SLOTS - _guard_slots_used)

func erosion_guard_cost(memory: Memory) -> int:
	if memory == null:
		return 0
	var index := clampi(memory.grade, 0, EROSION_GUARD_COST.size() - 1)
	return int(EROSION_GUARD_COST[index])

func is_guarded(memory_id: String) -> bool:
	return erosion_guarded.has(memory_id)

## 고정할 수 있는지와 그 이유. UI가 버튼 문구를 만들 때 그대로 쓴다.
func guard_availability(memory: Memory) -> Dictionary:
	var is_ko := GameManager.current_locale == "ko"
	if memory == null:
		return {"ok": false, "reason": "없음" if is_ko else "No memory"}
	if memory.is_burned:
		return {"ok": false, "reason": "이미 태워짐" if is_ko else "Already burned"}
	if memory.is_faded:
		return {"ok": false, "reason": "이미 희미해짐" if is_ko else "Already faded"}
	if memory.grade >= MemoryGrade.GRADE_1:
		return {"ok": false, "reason": "핵심 기억은 침식되지 않음" if is_ko else "Core memories never erode"}
	if erosion_guarded.has(memory.id):
		return {"ok": false, "reason": "이번 챕터 고정됨" if is_ko else "Guarded this chapter"}
	if guard_slots_remaining() <= 0:
		return {"ok": false, "reason": "고정 슬롯 없음" if is_ko else "No guard slots left"}
	var cost := erosion_guard_cost(memory)
	if int(GameManager.player_data.get("grains", 0)) < cost:
		return {"ok": false, "reason": ("그레인 부족 (%d)" % cost) if is_ko else ("Needs %d Grains" % cost)}
	return {"ok": true, "reason": "", "cost": cost}

## 침식을 되돌리고 다음 한 번을 막는다. 전부는 못 지킨다는 것이 요점이다.
func guard_memory(memory_id: String) -> bool:
	var memory := find_memory(memory_id)
	var availability := guard_availability(memory)
	if not bool(availability.get("ok", false)):
		return false
	var cost := int(availability.get("cost", 0))
	GameManager.player_data["grains"] = int(GameManager.player_data.get("grains", 0)) - cost
	memory.erosion = 0
	erosion_guarded[memory.id] = true
	_guard_slots_used += 1
	memory_guarded.emit(memory)
	guard_slots_changed.emit(guard_slots_remaining())
	NotificationToast.show_toast(
		("고정: %s" % localized_memory_title(memory)) if GameManager.current_locale == "ko" else ("Anchored: %s" % localized_memory_title(memory)),
		NotificationToast.ToastType.SUCCESS
	)
	print("[MemoryManager] GUARDED: %s (-%d grains, %d slots left)" % [memory.title, cost, guard_slots_remaining()])
	return true

## ===================== S234: 기억 표시 텍스트 =====================
##
## 기억을 화면에 적는 곳은 모두 이 세 함수를 지난다.
## (연소 확인창, 연소 목록, 전투 로그, 서고, 상점, 관리국 로그, 도감, 저널)

static func localized_memory_title(memory) -> String:
	if memory == null:
		return ""
	if GameManager.current_locale == "ko":
		var entry: Dictionary = MEMORY_TEXT_KO.get(memory.id, {})
		var shown := String(entry.get("title", ""))
		if shown != "":
			return shown
	return String(memory.title)

static func localized_memory_description(memory) -> String:
	if memory == null:
		return ""
	if GameManager.current_locale == "ko":
		var entry: Dictionary = MEMORY_TEXT_KO.get(memory.id, {})
		var shown := String(entry.get("desc", ""))
		if shown != "":
			return shown
	return String(memory.description)

static func localized_memory_effect(memory) -> String:
	if memory == null:
		return ""
	if GameManager.current_locale == "ko":
		var entry: Dictionary = MEMORY_TEXT_KO.get(memory.id, {})
		var shown := String(entry.get("effect", ""))
		if shown != "":
			return shown
	return String(memory.story_effect)

## 한국어 표가 비어 있는 기억을 찾아낸다. 스모크가 회귀를 잡는 데 쓴다.
func untranslated_memory_ids() -> Array[String]:
	var missing: Array[String] = []
	for memory in memories:
		var entry: Dictionary = MEMORY_TEXT_KO.get(memory.id, {})
		if String(entry.get("title", "")) == "" or String(entry.get("desc", "")) == "":
			missing.append(memory.id)
			continue
		if memory.story_effect != "" and String(entry.get("effect", "")) == "":
			missing.append(memory.id)
	return missing

static func localized_passive_name(passive: Dictionary) -> String:
	if GameManager.current_locale == "ko":
		return String(passive.get("name_ko", passive.get("name", "")))
	return String(passive.get("name", ""))

static func localized_passive_desc(passive: Dictionary) -> String:
	if GameManager.current_locale == "ko":
		return String(passive.get("desc_ko", passive.get("desc", "")))
	return String(passive.get("desc", ""))

## S58: Power milestone dramatic effect, screen flash + particle burst + text
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

	# S232: 예전에는 CanvasLayer의 "modulate"를 트윈하려 했다. CanvasLayer에는 그 속성이
	# 없어서 tween_property가 null을 돌려주고, 페이드아웃과 queue_free가 통째로 사라졌다.
	# 결과적으로 5/10/20/30/50번째 연소마다 "POWER AWAKENED" 오버레이가 화면에 영구히
	# 남았다. 이제 페이드 대상이 되는 Control 하나를 두고 그 아래에 전부 매단다.
	var content = Control.new()
	content.set_anchors_preset(Control.PRESET_FULL_RECT)
	content.mouse_filter = Control.MOUSE_FILTER_IGNORE
	overlay.add_child(content)

	# White flash screen
	var flash = ColorRect.new()
	flash.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash.color = Color(1.0, 0.95, 0.7, 0.0)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_child(flash)

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
	content.add_child(title_lbl)

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
	content.add_child(sub_lbl)

	# Burn count indicator
	var count_lbl = Label.new()
	count_lbl.text = "%d memories burned" % burn_count
	count_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	count_lbl.set_anchors_preset(Control.PRESET_CENTER)
	count_lbl.position = Vector2(-200, 30)
	count_lbl.size = Vector2(400, 20)
	count_lbl.add_theme_font_size_override("font_size", 13)
	count_lbl.add_theme_color_override("font_color", Color(0.7, 0.6, 0.4, 0.0))
	count_lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.add_child(count_lbl)

	# Particle burst, radial gold particles
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
	content.add_child(particles)

	# Animate sequence
	# 1. Flash in
	var tw = content.create_tween()
	tw.tween_property(flash, "color:a", 0.6, 0.15)
	tw.tween_property(flash, "color:a", 0.0, 0.4).set_ease(Tween.EASE_OUT)
	# 2. Title scale-in
	var tw2 = content.create_tween()
	tw2.tween_property(title_lbl, "modulate:a", 1.0, 0.2)
	tw2.parallel().tween_property(title_lbl, "scale", Vector2(1.0, 1.0), 0.35).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)
	# 3. Subtitle fade in (delayed)
	var tw3 = content.create_tween()
	tw3.tween_interval(0.3)
	tw3.tween_property(sub_lbl, "theme_override_colors/font_color", Color(0.9, 0.75, 0.4, 1.0), 0.3)
	tw3.parallel().tween_property(count_lbl, "theme_override_colors/font_color", Color(0.7, 0.6, 0.4, 1.0), 0.3)
	# 4. Fire particles
	particles.emitting = true
	# Play SFX
	AudioManager.play_sfx("heal")
	# 5. Hold, then fade out everything
	var tw4 = content.create_tween()
	tw4.tween_interval(2.0)
	tw4.tween_property(content, "modulate:a", 0.0, 0.5).set_ease(Tween.EASE_IN)
	tw4.tween_callback(overlay.queue_free)
	print("[MemoryManager] S58: Power milestone displayed, %s (%d burns)" % [passive_name, burn_count])

## 세이브용 데이터 내보내기
func export_data() -> Dictionary:
	var data = {
		"memories": [],
		"burned": [],
		"burn_passives": burn_passives.duplicate(),
		"anchor_vigil": anchor_vigil,
		"anchor_passives": anchor_passives.duplicate(),
		"vigil_chapters": _vigil_chapters_counted.keys(),
		"erosion_guarded": erosion_guarded.keys(),
		"guard_slots_used": _guard_slots_used,
		"active_loan": active_loan.duplicate(),
		"extracted": extracted_memories.duplicate(),
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
	anchor_vigil = int(data.get("anchor_vigil", 0))
	anchor_passives = data.get("anchor_passives", {})
	_vigil_chapters_counted.clear()
	for chapter in data.get("vigil_chapters", []):
		_vigil_chapters_counted[int(chapter)] = true
	erosion_guarded.clear()
	for guarded_id in data.get("erosion_guarded", []):
		erosion_guarded[String(guarded_id)] = true
	_guard_slots_used = int(data.get("guard_slots_used", 0))
	active_loan = data.get("active_loan", {})
	extracted_memories.clear()
	for extracted_id in data.get("extracted", []):
		extracted_memories.append(String(extracted_id))

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

	print("[MemoryManager] Imported, %d memories, %d burned" % [memories.size(), burned_memories.size()])
