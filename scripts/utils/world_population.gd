## WorldPopulation — Chapter-aware field inhabitants and visible memory threats.
## Each map receives a small social ecology: witnesses, workers, refugees, and
## optional hostile echoes.  This keeps the journey populated without turning
## every encounter into a mandatory combat interruption.
class_name WorldPopulation
extends RefCounted

const TILE_SIZE: int = 32
const NPC_SCENE := preload("res://scenes/npc/npc.tscn")

const NPC_ART := {
	"forager": "res://assets/sprites/world_population/npcs/rootbark_forager_field_v1.png",
	"debtor": "res://assets/sprites/world_population/npcs/verdan_debtor_field_v1.png",
	"courier": "res://assets/sprites/world_population/npcs/belt_courier_field_v1.png",
	"scribe": "res://assets/sprites/world_population/npcs/drift_scribe_field_v1.png",
	"lantern": "res://assets/sprites/world_population/npcs/seam_lanternkeeper_field_v1.png",
	"pilgrim": "res://assets/sprites/world_population/npcs/waste_pilgrim_field_v1.png",
}

const HUNT_ART := {
	"ash_hound": "res://assets/sprites/world_population/hostiles/ash_hound_field_v1.png",
	"belt_scavenger": "res://assets/sprites/world_population/hostiles/belt_scavenger_field_v1.png",
	"signal_wisp": "res://assets/sprites/world_population/hostiles/signal_wisp_field_v1.png",
	"rootbound_echo": "res://assets/sprites/world_population/hostiles/rootbound_echo_field_v1.png",
	"colorless_wraith": "res://assets/sprites/world_population/hostiles/colorless_wraith_field_v1.png",
	"void_fragment": "res://assets/sprites/world_population/hostiles/void_fragment_field_v1.png",
}

const SPECIAL_VOICE_ART := {
	"rim_scout": "res://assets/sprites/world_population/npcs/rim_herbalist_field_v1.png",
	"sealed_note_seller": "res://assets/sprites/world_population/npcs/verdan_runner_field_v1.png",
	"signal_keeper": "res://assets/sprites/world_population/npcs/belt_mechanic_field_v1.png",
	"rain_ledger_scribe": "res://assets/sprites/world_population/npcs/drift_child_archivist_field_v1.png",
	"quiet_healer": "res://assets/sprites/world_population/npcs/seam_medic_field_v1.png",
	"compass_pilgrim": "res://assets/sprites/world_population/npcs/waste_compass_guide_field_v1.png",
}

const SPECIAL_HUNT_ART := {
	"coast_ash_hound": "res://assets/sprites/world_population/hostiles/ash_stalker_field_v1.png",
	"belt_scavenger": "res://assets/sprites/world_population/hostiles/belt_tag_raider_field_v1.png",
	"belt_signal_wisp": "res://assets/sprites/world_population/hostiles/signal_moth_wisp_field_v1.png",
	"forest_rootbound_echo": "res://assets/sprites/world_population/hostiles/fungal_root_sentinel_field_v1.png",
	"waste_colorless_wraith": "res://assets/sprites/world_population/hostiles/colorless_husk_field_v1.png",
	"outskirts_void_fragment": "res://assets/sprites/world_population/hostiles/echo_shell_field_v1.png",
}

const POPULATIONS := {
	"rim_forest": {
		"voices": [
			{"id": "rootbark_forager", "tile": [4, 11], "art": NPC_ART.forager, "name": "Root-Bark Forager", "name_ko": "뿌리껍질 채집가", "line": "Chew slowly. The numbness gives your hands something harmless to remember.", "line_ko": "천천히 씹어. 감각이 무뎌지면 손이 기억할 만한 해가 없는 일을 하나 얻거든."},
			{"id": "rim_scout", "tile": [20, 12], "art": NPC_ART.courier, "name": "Rim Trail Scout", "name_ko": "림 길잡이", "line": "The Rim keeps no ledgers. That is why the forgotten still find their way here.", "line_ko": "림에는 장부가 없어. 그래서 잊힌 사람도 아직 이곳까지는 찾아와."},
			{"id": "ash_rain_mother", "tile": [7, 13], "art": NPC_ART.scribe, "name": "Ash-Rain Mother", "name_ko": "재비의 어머니", "line": "When the ash starts speaking in a child's voice, cover your ears. It only wants to borrow tomorrow.", "line_ko": "재가 아이 목소리로 말하기 시작하면 귀를 막아. 그건 내일을 빌리고 싶어 할 뿐이야."},
		],
		"hunts": [
			{"id": "rim_ash_hound", "tile": [18, 10], "art": HUNT_ART.ash_hound, "name": "Ash Hound", "name_ko": "재 사냥개", "hp": 42, "atk": 9, "abilities": ["poison"], "requires": "ch1_complete", "weakness": "fire"},
		],
	},
	"verdan_market": {
		"voices": [
			{"id": "loan_debtor", "tile": [5, 5], "art": NPC_ART.debtor, "name": "Memory-Debt Laborer", "name_ko": "기억 채무 노동자", "line": "My next summer was worth eleven Grains. I counted the years before I signed.", "line_ko": "내 다음 여름은 열한 그레인 값이었어. 서명하기 전에 몇 번이나 남은 해를 세었지."},
			{"id": "mine_shift_elder", "tile": [14, 4], "art": NPC_ART.forager, "name": "Mine Shift Elder", "name_ko": "광산 교대 노인", "line": "The ore remembers every hand that bled on it. The Authority calls that a defect.", "line_ko": "광석은 피를 흘린 손을 전부 기억해. 관리국은 그걸 불량이라고 부르지."},
			{"id": "spice_lane_runner", "tile": [23, 12], "art": NPC_ART.courier, "name": "Spice-Lane Runner", "name_ko": "향신료 골목 심부름꾼", "line": "Keep your badge turned in. In Verdan, even hunger has a clerk watching it.", "line_ko": "배지는 안쪽으로 돌려. 베르단에서는 굶주림에도 장부 담당자가 붙어 있어."},
			{"id": "sealed_note_seller", "tile": [27, 16], "art": NPC_ART.debtor, "name": "Sealed-Note Seller", "name_ko": "봉인 증서 상인", "line": "A loan note is only paper until it decides which face you can no longer picture.", "line_ko": "대출 증서는 종이일 뿐이야. 더는 떠올릴 수 없는 얼굴을 고르기 전까진."},
		],
		"hunts": [],
	},
	"belt_waystation": {
		"voices": [
			{"id": "record_courier", "tile": [5, 7], "art": NPC_ART.courier, "name": "Record Courier", "name_ko": "기록 전령", "line": "Do not say names near the tower. It listens for the shape a name makes in your mouth.", "line_ko": "탑 근처에서는 이름을 말하지 마. 탑은 입 안에서 이름이 만들어내는 모양까지 들어."},
			{"id": "signal_keeper", "tile": [18, 8], "art": NPC_ART.scribe, "name": "Signal-Tower Keeper", "name_ko": "신호탑 관리인", "line": "The lamps blink for weather, work orders, and funerals. The rhythm is getting faster.", "line_ko": "등불은 날씨와 작업 지시와 장례를 알려. 요즘은 그 리듬이 너무 빨라졌어."},
			{"id": "unbadged_pilgrim", "tile": [21, 14], "art": NPC_ART.pilgrim, "name": "Unbadged Pilgrim", "name_ko": "무배지 순례자", "line": "A badge is a door that remembers you. I have walked through enough doors already.", "line_ko": "배지는 너를 기억하는 문이야. 난 이미 너무 많은 문을 지나왔어."},
		],
		"hunts": [
			{"id": "belt_scavenger", "tile": [20, 5], "art": HUNT_ART.belt_scavenger, "name": "Belt Scavenger", "name_ko": "벨트 약탈자", "hp": 58, "atk": 12, "abilities": ["weaken"], "weakness": "fire"},
			{"id": "belt_signal_wisp", "tile": [6, 13], "art": HUNT_ART.signal_wisp, "name": "Signal Wisp", "name_ko": "신호 도깨비불", "hp": 48, "atk": 13, "is_void": true, "abilities": ["drain"], "weakness": "void"},
		],
	},
	"drift_shelter": {
		"voices": [
			{"id": "rain_ledger_scribe", "tile": [5, 11], "art": NPC_ART.scribe, "name": "Rain-Ledger Scribe", "name_ko": "빗물 장부 서기", "line": "I write the route twice: once as words, once as marks. On bad days, only one stays readable.", "line_ko": "길을 두 번 적어. 한 번은 말로, 한 번은 표식으로. 나쁜 날엔 하나만 읽히거든."},
			{"id": "basin_keeper", "tile": [20, 8], "art": NPC_ART.lantern, "name": "Rain Basin Keeper", "name_ko": "빗물통 지기", "line": "The shelter shares water before stories. Stories have started changing their price.", "line_ko": "피난처는 이야기보다 먼저 물을 나눠. 요즘은 이야기도 값이 달라졌지만."},
			{"id": "waystone_reader", "tile": [18, 14], "art": NPC_ART.pilgrim, "name": "Waystone Reader", "name_ko": "이정석 해독가", "line": "If the letters scatter, touch the stone and ask where you slept. The road still knows that much.", "line_ko": "글자가 흩어지면 돌을 만지고 어디서 잤는지 물어. 길은 아직 그 정도는 기억해."},
		],
		"hunts": [
			{"id": "drift_signal_wisp", "tile": [8, 13], "art": HUNT_ART.signal_wisp, "name": "Signal Wisp", "name_ko": "신호 도깨비불", "hp": 54, "atk": 14, "is_void": true, "abilities": ["drain", "weaken"], "requires": "ch4_complete", "weakness": "void"},
		],
	},
	"crumbling_coast": {
		"voices": [
			{"id": "ash_netter", "tile": [6, 6], "art": NPC_ART.forager, "name": "Ash Netter", "name_ko": "재 그물꾼", "line": "The tide brings back pieces of people. We give them a name before the waves take them again.", "line_ko": "밀물은 사람의 조각을 되돌려줘. 우리는 파도가 다시 데려가기 전에 이름을 붙여."},
			{"id": "wreck_cartographer", "tile": [18, 8], "art": NPC_ART.scribe, "name": "Wreck Cartographer", "name_ko": "난파 지도제작자", "line": "The coast moves every season. The graves move farther.", "line_ko": "해안선은 계절마다 움직여. 무덤은 그보다 더 멀리 움직이고."},
			{"id": "salt_walker", "tile": [20, 13], "art": NPC_ART.pilgrim, "name": "Salt Walker", "name_ko": "소금길 나그네", "line": "Ash tastes different here. It remembers water.", "line_ko": "여기 재는 맛이 달라. 물을 기억하고 있거든."},
		],
		"hunts": [
			{"id": "coast_ash_hound", "tile": [10, 10], "art": HUNT_ART.ash_hound, "name": "Ash Hound", "name_ko": "재 사냥개", "hp": 70, "atk": 16, "abilities": ["poison"], "weakness": "fire"},
			{"id": "coast_belt_scavenger", "tile": [16, 5], "art": HUNT_ART.belt_scavenger, "name": "Coast Scavenger", "name_ko": "해안 약탈자", "hp": 66, "atk": 15, "abilities": ["weaken"], "weakness": "fire"},
		],
	},
	"the_seam": {
		"voices": [
			{"id": "lantern_keeper", "tile": [4, 10], "art": NPC_ART.lantern, "name": "Seam Lantern Keeper", "name_ko": "심의 등불지기", "line": "Light the lantern before you cross. Not for the dark—for the part of you that likes it.", "line_ko": "건너기 전에 등불부터 밝혀. 어둠 때문이 아니라, 어둠을 좋아하는 네 안의 부분 때문에."},
			{"id": "quiet_healer", "tile": [18, 7], "art": NPC_ART.scribe, "name": "Quiet Healer", "name_ko": "고요한 치료사", "line": "Here, we ask what hurts before we ask what you lost.", "line_ko": "여기서는 무엇을 잃었는지보다 무엇이 아픈지부터 물어."},
			{"id": "gully_child", "tile": [20, 12], "art": NPC_ART.courier, "name": "Gully Child", "name_ko": "골짜기 아이", "line": "The color begins over there. I thought it was a story until it touched my sleeve.", "line_ko": "색은 저쪽에서 시작돼. 난 소문인 줄 알았는데 내 소매를 만졌어."},
		],
		"hunts": [
			{"id": "seam_signal_wisp", "tile": [9, 14], "art": HUNT_ART.signal_wisp, "name": "Seam Wisp", "name_ko": "심의 도깨비불", "hp": 78, "atk": 17, "is_void": true, "abilities": ["drain", "shield"], "requires": "ch6_complete", "weakness": "void"},
		],
	},
	"seam_outskirts": {
		"voices": [
			{"id": "threshold_warden", "tile": [5, 7], "art": NPC_ART.lantern, "name": "Threshold Warden", "name_ko": "경계의 파수꾼", "line": "Do not answer the voice that knows your first fear. It is only wearing it.", "line_ko": "네 첫 공포를 아는 목소리에게 대답하지 마. 그건 그 공포를 입고 있을 뿐이야."},
			{"id": "gate_penitent", "tile": [19, 8], "art": NPC_ART.pilgrim, "name": "Gate Penitent", "name_ko": "문 앞의 참회자", "line": "I came to leave a memory behind. The Threshold asked for the one I was hiding from.", "line_ko": "기억 하나를 두고 가려고 왔어. 경계는 내가 숨기던 것을 달라고 했지."},
			{"id": "last_courier", "tile": [16, 13], "art": NPC_ART.courier, "name": "Last Courier", "name_ko": "마지막 전령", "line": "There are messages on the other side with no sender left to deny them.", "line_ko": "저편에는 보내는 이가 없어져서 부정할 수도 없는 편지들이 있어."},
		],
		"hunts": [
			{"id": "outskirts_void_fragment", "tile": [10, 11], "art": HUNT_ART.void_fragment, "name": "Void Fragment", "name_ko": "보이드 파편", "hp": 96, "atk": 20, "is_void": true, "abilities": ["burn_attack", "charge"], "requires": "ch7_complete", "weakness": "void"},
			{"id": "outskirts_signal_wisp", "tile": [21, 5], "art": HUNT_ART.signal_wisp, "name": "Threshold Wisp", "name_ko": "경계 도깨비불", "hp": 86, "atk": 19, "is_void": true, "abilities": ["drain", "stun"], "requires": "ch7_complete", "weakness": "fire"},
		],
	},
	"forgotten_forest": {
		"voices": [
			{"id": "name_whisperer", "tile": [5, 8], "art": NPC_ART.pilgrim, "name": "Name Whisperer", "name_ko": "이름 속삭이는 이", "line": "Say your name only once. The trees repeat whatever you give them.", "line_ko": "이름은 한 번만 말해. 나무는 네가 준 것을 계속 따라 하거든."},
			{"id": "lost_patrol_echo", "tile": [18, 7], "art": NPC_ART.forager, "name": "Lost Patrol Echo", "name_ko": "길 잃은 순찰의 메아리", "line": "North was safe yesterday. Yesterday was someone else's word.", "line_ko": "어제는 북쪽이 안전했어. 하지만 어제는 이제 다른 사람의 말이야."},
			{"id": "root_listener", "tile": [20, 13], "art": NPC_ART.scribe, "name": "Root Listener", "name_ko": "뿌리의 청자", "line": "The roots do not hate us. They are trying to finish a story they were fed.", "line_ko": "뿌리는 우리를 미워하지 않아. 누군가 먹인 이야기를 끝내려는 것뿐이야."},
		],
		"hunts": [
			{"id": "forest_rootbound_echo", "tile": [11, 10], "art": HUNT_ART.rootbound_echo, "name": "Rootbound Echo", "name_ko": "뿌리에 묶인 메아리", "hp": 104, "atk": 23, "is_void": true, "abilities": ["drain", "poison"], "weakness": "fire"},
			{"id": "forest_signal_wisp", "tile": [7, 14], "art": HUNT_ART.signal_wisp, "name": "Lost Signal Wisp", "name_ko": "길 잃은 신호 도깨비불", "hp": 90, "atk": 20, "is_void": true, "abilities": ["drain", "weaken"], "weakness": "void"},
		],
	},
	"colorless_waste": {
		"voices": [
			{"id": "compass_pilgrim", "tile": [5, 7], "art": NPC_ART.pilgrim, "name": "Compass Pilgrim", "name_ko": "나침반 순례자", "line": "It does not point north. It points toward the memory you are most afraid to keep.", "line_ko": "이건 북쪽을 가리키지 않아. 네가 가장 간직하기 두려운 기억을 가리켜."},
			{"id": "grey_pathkeeper", "tile": [19, 8], "art": NPC_ART.lantern, "name": "Grey Pathkeeper", "name_ko": "회색 길지기", "line": "Color is not gone. It is holding its breath under the dust.", "line_ko": "색이 사라진 게 아니야. 먼지 밑에서 숨을 참고 있을 뿐이지."},
			{"id": "chalk_witness", "tile": [21, 13], "art": NPC_ART.scribe, "name": "Chalk Witness", "name_ko": "분필 증인", "line": "I mark every return. The wind erases them, so I can prove I was here again.", "line_ko": "돌아올 때마다 표시해. 바람이 지우니까, 내가 다시 여기 있었다는 걸 증명할 수 있어."},
		],
		"hunts": [
			{"id": "waste_colorless_wraith", "tile": [10, 10], "art": HUNT_ART.colorless_wraith, "name": "Colorless Wraith", "name_ko": "무색 망령", "hp": 120, "atk": 26, "is_void": true, "abilities": ["drain", "stun"], "weakness": "physical"},
			{"id": "waste_void_fragment", "tile": [16, 5], "art": HUNT_ART.void_fragment, "name": "Hollow Fragment", "name_ko": "속 빈 파편", "hp": 105, "atk": 23, "is_void": true, "abilities": ["charge", "reflect"], "weakness": "void"},
		],
	},
	"bl07_void": {
		"voices": [
			{"id": "shell_custodian", "tile": [5, 8], "art": NPC_ART.scribe, "name": "Shell Custodian", "name_ko": "에코 셸 관리자", "line": "These shells are not empty. They are waiting for someone to stop calling them tools.", "line_ko": "이 셸들은 비어 있지 않아. 누군가 도구라고 부르기를 그만둘 때를 기다리고 있을 뿐이야."},
			{"id": "unfiled_witness", "tile": [15, 6], "art": NPC_ART.lantern, "name": "Unfiled Witness", "name_ko": "미등록 증인", "line": "Nothing here can be filed. That is why it is still true.", "line_ko": "여기의 것은 아무것도 등록할 수 없어. 그래서 아직 진실인 거야."},
			{"id": "seed_bearer", "tile": [17, 14], "art": NPC_ART.forager, "name": "Seed Bearer", "name_ko": "씨앗을 든 이", "line": "A record tree does not need permission to grow. It only needs a place that remembers rain.", "line_ko": "기록목은 자라기 위해 허가가 필요 없어. 비를 기억하는 땅 하나면 돼."},
		],
		"hunts": [
			{"id": "void_fragment", "tile": [10, 11], "art": HUNT_ART.void_fragment, "name": "Void Fragment", "name_ko": "보이드 파편", "hp": 120, "atk": 28, "is_void": true, "abilities": ["burn_attack", "charge"], "weakness": "void"},
			{"id": "void_colorless_wraith", "tile": [4, 14], "art": HUNT_ART.colorless_wraith, "name": "Memory Eater", "name_ko": "기억 포식자", "hp": 135, "atk": 30, "is_void": true, "abilities": ["drain", "multi_hit", "weaken"], "weakness": "fire"},
		],
	},
	"rim_root_hollow": {
		"voices": [
			{"id": "seed_listener", "tile": [6, 8], "art": NPC_ART.forager, "name": "Seed Listener", "name_ko": "씨앗의 청자", "line": "The roots keep the words no ledger can seize.", "line_ko": "뿌리는 어떤 장부도 빼앗지 못하는 말을 간직해."},
			{"id": "paper_shrine_keeper", "tile": [18, 8], "art": NPC_ART.lantern, "name": "Paper Shrine Keeper", "name_ko": "종이 사당지기", "line": "This paper has no price. That is why they call it dangerous.", "line_ko": "이 종이에는 값이 없어. 그래서 그들은 위험하다고 부르지."},
		],
		"hunts": [
			{"id": "hollow_rootbound_echo", "tile": [12, 7], "art": HUNT_ART.rootbound_echo, "name": "Rootbound Echo", "name_ko": "뿌리에 묶인 메아리", "hp": 62, "atk": 13, "is_void": true, "abilities": ["drain", "poison"], "requires": "ch1_complete", "weakness": "fire"},
			{"id": "hollow_signal_wisp", "tile": [6, 12], "art": HUNT_ART.signal_wisp, "name": "Archive Wisp", "name_ko": "기록 도깨비불", "hp": 55, "atk": 12, "is_void": true, "abilities": ["drain"], "requires": "ch1_complete", "weakness": "void"},
		],
	},
	"verdan_ledger_cellar": {
		"voices": [
			{"id": "default_clerk", "tile": [7, 10], "art": NPC_ART.debtor, "name": "Default Clerk", "name_ko": "연체 기록원", "line": "The ledger knows the due date before the debtor does.", "line_ko": "장부는 채무자보다 먼저 납부일을 알아."},
			{"id": "foreclosed_miner", "tile": [17, 10], "art": NPC_ART.forager, "name": "Foreclosed Miner", "name_ko": "압류당한 광부", "line": "They took the day my son learned to walk. Called it a clean settlement.", "line_ko": "아들이 걸음마를 배운 날을 가져갔어. 깔끔한 정산이라 부르더군."},
			{"id": "ledger_ghost", "tile": [12, 6], "art": NPC_ART.scribe, "name": "Ledger Ghost", "name_ko": "장부의 유령", "line": "Every erased signature leaves a hand behind.", "line_ko": "지워진 서명마다 손 하나는 남아."},
		],
		"hunts": [
			{"id": "cellar_collection_warden", "tile": [12, 10], "art": HUNT_ART.belt_scavenger, "name": "Collection Warden", "name_ko": "추심 파수꾼", "hp": 68, "atk": 15, "abilities": ["weaken", "shield"], "requires": "ch2_malet_done", "weakness": "fire"},
		],
	},
}

## S187: Seven chapter-earned atlas pockets.  These are deliberately small,
## returnable maps, but their population is large enough to make the journey
## feel inhabited and to turn each route into a reward after its chapter ends.
const ATLAS_POPULATIONS := {
	"belt_signal_yard": {
		"voices": [
			{"id": "yard_code_runner", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/belt_code_runner_field_v1.png", "name": "Belt Code Runner", "name_ko": "벨트 암호 주자", "line": "The tower hears every official signal. This wire only remembers the ones we cut loose.", "line_ko": "탑은 모든 공식 신호를 듣습니다. 이 전선은 우리가 끊어낸 신호만 기억해요."},
			{"id": "yard_ash_listener", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/ash_weather_listener_field_v1.png", "name": "Ash Weather Listener", "name_ko": "재비 기상 청취자", "line": "When the ash falls sideways, the Authority is searching by sound.", "line_ko": "재가 옆으로 내리면 관리국이 소리로 찾고 있다는 뜻이에요."},
			{"id": "yard_root_tender", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/rim_root_tender_field_v1.png", "name": "Rail Root Tender", "name_ko": "철로 뿌리 돌보미", "line": "Roots grow through every abandoned rail. They do not care whose border it was.", "line_ko": "뿌리는 버려진 철로마다 자라요. 누구의 경계였는지는 관심 없죠."},
			{"id": "yard_debt_witness", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/verdan_debt_witness_field_v1.png", "name": "Unfiled Debt Witness", "name_ko": "미등재 채무 증인", "line": "I carried a receipt out of Verdan. It has three names the ledger says never existed.", "line_ko": "베르단에서 영수증 하나를 들고 나왔어요. 장부에는 없었다는 이름이 셋이나 있어요."},
		],
		"hunts": [
			{"id": "yard_signal_scavenger", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/signal_scavenger_field_v1.png", "name": "Signal Scavenger", "name_ko": "신호 약탈자", "hp": 76, "atk": 17, "abilities": ["weaken", "charge"], "weakness": "fire"},
			{"id": "yard_rail_sentinel", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/rail_sentinel_field_v1.png", "name": "Rail Sentinel", "name_ko": "철로 감시병", "hp": 84, "atk": 18, "is_void": true, "abilities": ["shield", "stun"], "weakness": "void"},
		],
		"caches": [{"id": "jammer_cache", "tile": [12, 7], "item": "signal_jammer", "count": 1}],
	},
	"drift_waymarker_shrine": {
		"voices": [
			{"id": "shrine_route_translator", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/drift_route_translator_field_v1.png", "name": "Route Translator", "name_ko": "경로 번역가", "line": "The marker changes language at dawn. I write the directions before the letters leave.", "line_ko": "이 표지는 새벽마다 언어가 바뀌어요. 글자가 떠나기 전에 길을 적어 둡니다."},
			{"id": "shrine_lantern_child", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/coast_lantern_child_field_v1.png", "name": "Waymarker Child", "name_ko": "이정표 아이", "line": "I remember the road by the sound of rain in each crack.", "line_ko": "균열마다 빗소리가 달라요. 난 그 소리로 길을 기억해요."},
			{"id": "shrine_baker", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/seam_baker_field_v1.png", "name": "Rain Bread Baker", "name_ko": "비빵 제빵사", "line": "Bread lasts longer when you knead in a story. It tastes worse, but people stay.", "line_ko": "이야기를 반죽하면 빵이 오래 가요. 맛은 나빠져도 사람은 남죠."},
			{"id": "shrine_bridge_keeper", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/seam_bridge_keeper_field_v1.png", "name": "Collapsed Bridge Keeper", "name_ko": "무너진 다리 지기", "line": "A waymarker is a promise from someone who got lost before you did.", "line_ko": "이정표는 당신보다 먼저 길을 잃은 누군가의 약속이에요."},
		],
		"hunts": [
			{"id": "shrine_rain_oracle", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/rain_oracle_field_v1.png", "name": "Rain Oracle", "name_ko": "비의 신탁", "hp": 90, "atk": 20, "is_void": true, "abilities": ["drain", "weaken"], "weakness": "void"},
			{"id": "shrine_lantern_leech", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/lantern_leech_field_v1.png", "name": "Lantern Leech", "name_ko": "등불 거머리", "hp": 72, "atk": 18, "abilities": ["poison", "shield"], "weakness": "fire"},
		],
		"caches": [{"id": "root_balm_cache", "tile": [12, 7], "item": "root_balm", "count": 2}],
	},
	"coast_cinder_harbor": {
		"voices": [
			{"id": "harbor_net_mender", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/harbor_net_mender_field_v1.png", "name": "Cinder Net Mender", "name_ko": "신더 그물 수선공", "line": "We mend nets for the tide and names for the living. The second job is harder.", "line_ko": "우린 조수의 그물과 산 자의 이름을 함께 꿰매요. 두 번째 일이 더 어렵죠."},
			{"id": "harbor_baker", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/seam_baker_field_v1.png", "name": "Harbor Bread Keeper", "name_ko": "항구 빵지기", "line": "The ovens are warm because the drowned hate a cold shore.", "line_ko": "익사자들이 차가운 해안을 싫어해서 화덕을 꺼뜨리지 않아요."},
			{"id": "harbor_lantern_child", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/coast_lantern_child_field_v1.png", "name": "Tide Lantern Child", "name_ko": "조수 등불 아이", "line": "Every lantern has a name. I carry the ones nobody comes back for.", "line_ko": "등불마다 이름이 있어요. 아무도 돌아오지 않는 이름은 내가 들고 다녀요."},
			{"id": "harbor_debt_witness", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/verdan_debt_witness_field_v1.png", "name": "Wreck Debt Witness", "name_ko": "난파 채무 증인", "line": "The sea is kinder than a ledger. It returns at least part of what it takes.", "line_ko": "바다는 장부보다 친절해요. 가져간 것의 일부는 돌려주니까요."},
		],
		"hunts": [
			{"id": "harbor_drowned_echo", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/coast_drowned_echo_field_v1.png", "name": "Drowned Echo", "name_ko": "익사한 메아리", "hp": 102, "atk": 22, "is_void": true, "abilities": ["drain", "poison"], "weakness": "fire"},
			{"id": "harbor_ash_bone_hound", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/ash_bone_hound_field_v1.png", "name": "Ash Bone Hound", "name_ko": "재뼈 사냥개", "hp": 94, "atk": 21, "abilities": ["burn_attack", "multi_hit"], "weakness": "fire"},
		],
		"caches": [{"id": "lantern_salve_cache", "tile": [12, 7], "item": "lantern_salve", "count": 1}],
	},
	"seam_lantern_ward": {
		"voices": [
			{"id": "ward_bridge_keeper", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/seam_bridge_keeper_field_v1.png", "name": "Lantern Bridge Keeper", "name_ko": "등불 다리 지기", "line": "We let every traveler cross once. After that, they choose which side remembers them.", "line_ko": "모든 여행자에게 한 번은 건널 기회를 줍니다. 그 다음엔 어느 쪽이 자신을 기억할지 고르죠."},
			{"id": "ward_baker", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/seam_baker_field_v1.png", "name": "Lantern Ward Baker", "name_ko": "랜턴 구역 제빵사", "line": "The first color we kept was saffron. It makes the children believe morning can return.", "line_ko": "우리가 지킨 첫 색은 샤프란이었어요. 아이들이 아침도 돌아올 수 있다고 믿게 하죠."},
			{"id": "ward_net_mender", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/harbor_net_mender_field_v1.png", "name": "Thread Net Mender", "name_ko": "실그물 수선공", "line": "A torn memory can be tied, but never pulled tight. Leave it room to breathe.", "line_ko": "찢어진 기억은 묶을 수 있어도 너무 세게 당기면 안 돼요. 숨 쉴 틈을 남겨야죠."},
			{"id": "ward_route_translator", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/drift_route_translator_field_v1.png", "name": "Color Route Translator", "name_ko": "색의 경로 번역가", "line": "These colors are not decorations. They are directions our eyes can carry.", "line_ko": "이 색들은 장식이 아니에요. 눈이 들고 갈 수 있는 방향이죠."},
		],
		"hunts": [
			{"id": "ward_lantern_leech", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/lantern_leech_field_v1.png", "name": "Ward Lantern Leech", "name_ko": "구역 등불 거머리", "hp": 98, "atk": 22, "abilities": ["drain", "weaken"], "weakness": "fire"},
			{"id": "ward_rain_oracle", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/rain_oracle_field_v1.png", "name": "Drowned Rain Oracle", "name_ko": "익사한 비의 신탁", "hp": 112, "atk": 24, "is_void": true, "abilities": ["drain", "shield"], "weakness": "void"},
		],
		"caches": [{"id": "name_thread_cache", "tile": [12, 7], "item": "name_thread", "count": 1}],
	},
	"forest_name_hollow": {
		"voices": [
			{"id": "hollow_name_keeper", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/forest_name_keeper_field_v1.png", "name": "Name Hollow Keeper", "name_ko": "이름 골짜기 지기", "line": "Write your name on the bark, then read it aloud before the tree learns another voice.", "line_ko": "나무가 다른 목소리를 배우기 전에 껍질에 이름을 쓰고 소리 내어 읽으세요."},
			{"id": "hollow_root_tender", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/rim_root_tender_field_v1.png", "name": "Record Root Tender", "name_ko": "기록 뿌리 돌보미", "line": "The trees copy us because they are lonely, not because they are cruel.", "line_ko": "나무가 우리를 베끼는 건 잔인해서가 아니라 외로워서예요."},
			{"id": "hollow_weather_listener", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/ash_weather_listener_field_v1.png", "name": "Moss Weather Listener", "name_ko": "이끼 기상 청취자", "line": "When the moss hums, do not answer. It is practicing your voice.", "line_ko": "이끼가 흥얼거리면 대답하지 마세요. 당신 목소리를 연습하는 중이에요."},
			{"id": "hollow_route_translator", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/drift_route_translator_field_v1.png", "name": "Deer Path Translator", "name_ko": "사슴길 번역가", "line": "The deer paths are honest. The straight roads are the ones that lie.", "line_ko": "사슴길은 정직해요. 거짓말하는 건 반듯한 길이죠."},
		],
		"hunts": [
			{"id": "hollow_mimic_shade", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/forest_mimic_shade_field_v1.png", "name": "Mimic Shade", "name_ko": "모방 그림자", "hp": 128, "atk": 27, "is_void": true, "abilities": ["drain", "reflect"], "weakness": "fire"},
			{"id": "hollow_root_memory_swarm", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/root_memory_swarm_field_v1.png", "name": "Root Memory Swarm", "name_ko": "뿌리 기억 군집", "hp": 116, "atk": 25, "is_void": true, "abilities": ["poison", "multi_hit"], "weakness": "fire"},
		],
		"caches": [{"id": "compass_shard_cache", "tile": [12, 7], "item": "compass_shard", "count": 1}],
	},
	"waste_grey_caravan": {
		"voices": [
			{"id": "caravan_quartermaster", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/waste_caravan_quartermaster_field_v1.png", "name": "Grey Caravan Quartermaster", "name_ko": "회색 캐러밴 병참관", "line": "We trade in weight, water, and witness marks. Color is too expensive to carry.", "line_ko": "우린 무게와 물과 증인 표식으로 거래해요. 색은 들고 다니기엔 너무 비싸죠."},
			{"id": "caravan_debt_witness", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/verdan_debt_witness_field_v1.png", "name": "Caravan Debt Witness", "name_ko": "캐러밴 채무 증인", "line": "No one signs here. We remember the exchange together, or it did not happen.", "line_ko": "여기선 누구도 서명하지 않아요. 함께 교환을 기억하지 못하면 거래도 없었던 겁니다."},
			{"id": "caravan_bridge_keeper", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/seam_bridge_keeper_field_v1.png", "name": "Dust Bridge Keeper", "name_ko": "먼지 다리 지기", "line": "The caravan circles because a straight route gives the waste time to notice you.", "line_ko": "캐러밴이 빙 도는 건 곧은 길이 황무지에게 당신을 알아볼 시간을 주기 때문이에요."},
			{"id": "caravan_lantern_child", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/coast_lantern_child_field_v1.png", "name": "Chalk Lantern Child", "name_ko": "분필 등불 아이", "line": "My lantern is grey, but it still makes a shadow. That means I am here.", "line_ko": "내 등불은 회색이지만 그림자는 만들어요. 그럼 내가 여기에 있다는 뜻이죠."},
		],
		"hunts": [
			{"id": "caravan_glass_crawler", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/waste_glass_crawler_field_v1.png", "name": "Glass Crawler", "name_ko": "유리 기어다님", "hp": 142, "atk": 30, "abilities": ["reflect", "charge"], "weakness": "physical"},
			{"id": "caravan_raider", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/caravan_raider_field_v1.png", "name": "Grey Caravan Raider", "name_ko": "회색 캐러밴 약탈자", "hp": 138, "atk": 29, "is_void": true, "abilities": ["weaken", "stun"], "weakness": "void"},
		],
		"caches": [{"id": "root_balm_cache", "tile": [12, 7], "item": "root_balm", "count": 2}],
	},
	"bl07_seed_vault": {
		"voices": [
			{"id": "vault_seed_custodian", "tile": [5, 8], "art": "res://assets/sprites/world_population/npcs/bl07_seed_custodian_field_v1.png", "name": "Seed Vault Custodian", "name_ko": "씨앗 금고 관리자", "line": "These seeds do not grow trees. They grow the right to remember a tree.", "line_ko": "이 씨앗은 나무를 키우지 않아요. 나무를 기억할 권리를 키웁니다."},
			{"id": "vault_root_tender", "tile": [19, 8], "art": "res://assets/sprites/world_population/npcs/rim_root_tender_field_v1.png", "name": "Void Root Tender", "name_ko": "보이드 뿌리 돌보미", "line": "Even here, a root searches for rain it has never seen.", "line_ko": "여기서도 뿌리는 한 번도 본 적 없는 비를 찾아요."},
			{"id": "vault_code_runner", "tile": [7, 12], "art": "res://assets/sprites/world_population/npcs/belt_code_runner_field_v1.png", "name": "Unsent Code Runner", "name_ko": "미발송 암호 주자", "line": "The last message was never transmitted. That is why it still belongs to us.", "line_ko": "마지막 메시지는 전송되지 않았어요. 그래서 아직 우리 것이죠."},
			{"id": "vault_name_keeper", "tile": [17, 12], "art": "res://assets/sprites/world_population/npcs/forest_name_keeper_field_v1.png", "name": "Seed Name Keeper", "name_ko": "씨앗 이름 지기", "line": "A name can survive without a body. A seed can survive without a world.", "line_ko": "이름은 몸 없이도 남을 수 있어요. 씨앗은 세계 없이도 남을 수 있고요."},
		],
		"hunts": [
			{"id": "vault_archive_warden", "tile": [10, 5], "art": "res://assets/sprites/world_population/hostiles/bl07_archive_warden_field_v1.png", "name": "Archive Warden", "name_ko": "기록 보관자", "hp": 162, "atk": 34, "is_void": true, "abilities": ["burn_attack", "shield", "charge"], "weakness": "void"},
			{"id": "vault_seed_guardian", "tile": [14, 5], "art": "res://assets/sprites/world_population/hostiles/seed_guardian_field_v1.png", "name": "Seed Guardian", "name_ko": "씨앗 수호자", "hp": 176, "atk": 32, "is_void": true, "abilities": ["reflect", "poison", "multi_hit"], "weakness": "fire"},
		],
		"caches": [{"id": "seed_capsule_cache", "tile": [12, 7], "item": "seed_capsule", "count": 1}],
	},
}

static func populate(map: Node2D, map_id: String) -> void:
	if map == null or _population_for(map_id).is_empty():
		return
	if map.has_node("WorldPopulation"):
		return
	var root := Node2D.new()
	root.name = "WorldPopulation"
	root.z_index = 1
	map.add_child(root)
	var data: Dictionary = _population_for(map_id)
	var voice_count := 0
	var hunt_count := 0
	var cache_count := 0
	for cache_data in data.get("caches", []):
		if _spawn_cache(root, map_id, cache_data):
			cache_count += 1
	for voice_data in data.get("voices", []):
		if _spawn_voice(root, voice_data):
			voice_count += 1
	for hunt_data in data.get("hunts", []):
		if _spawn_hunt(root, map_id, hunt_data):
			hunt_count += 1
	print("[WorldPopulation] %s populated: %d voices, %d visible threats, %d caches" % [map_id, voice_count, hunt_count, cache_count])

static func _population_for(map_id: String) -> Dictionary:
	if POPULATIONS.has(map_id):
		return POPULATIONS[map_id]
	return ATLAS_POPULATIONS.get(map_id, {})

static func _spawn_cache(root: Node2D, map_id: String, data: Dictionary) -> bool:
	var required_flag := String(data.get("requires", ""))
	if required_flag != "" and not GameManager.get_flag(required_flag):
		return false
	var cache := WorldCache.new()
	cache.name = "WorldCache_%s" % String(data.get("id", "find"))
	cache.map_id = map_id
	cache.cache_id = String(data.get("id", "find"))
	cache.item_id = String(data.get("item", ""))
	cache.quantity = int(data.get("count", 1))
	cache.position = _tile_position(data.get("tile", [1, 1]))
	root.add_child(cache)
	return true

static func _spawn_voice(root: Node2D, data: Dictionary) -> bool:
	var required_flag := String(data.get("requires", ""))
	if required_flag != "" and not GameManager.get_flag(required_flag):
		return false
	var npc := NPC_SCENE.instantiate() as StaticBody2D
	if npc == null:
		return false
	npc.name = "WorldVoice_%s" % String(data.get("id", "unknown"))
	npc.set("npc_name", String(data.get("name", "Witness")))
	npc.set("display_name_ko", String(data.get("name_ko", "목격자")))
	npc.set("repeat_line", String(data.get("line", "...")))
	npc.set("repeat_line_ko", String(data.get("line_ko", "...")))
	npc.set("field_art_path", _art_for_voice(data))
	npc.position = _tile_position(data.get("tile", [1, 1]))
	root.add_child(npc)
	return true

static func _spawn_hunt(root: Node2D, map_id: String, data: Dictionary) -> bool:
	var required_flag := String(data.get("requires", ""))
	if required_flag != "" and not GameManager.get_flag(required_flag):
		return false
	var hunt_id := String(data.get("id", "unknown"))
	var defeated_flag := "world_hunt_%s_%s" % [map_id, hunt_id]
	if GameManager.get_flag(defeated_flag):
		return false
	var area := Area2D.new()
	area.name = "WorldThreat_%s" % hunt_id
	area.position = _tile_position(data.get("tile", [1, 1]))
	area.collision_layer = 0
	area.collision_mask = 2
	area.z_index = 2
	var collision := CollisionShape2D.new()
	var shape := CircleShape2D.new()
	shape.radius = 18.0
	collision.shape = shape
	collision.position = Vector2(0, -5)
	area.add_child(collision)
	var shadow := Polygon2D.new()
	shadow.polygon = PackedVector2Array([Vector2(-17, 2), Vector2(-9, -1), Vector2(9, -1), Vector2(17, 2), Vector2(9, 5), Vector2(-9, 5)])
	shadow.color = Color(0.0, 0.0, 0.0, 0.34)
	shadow.z_index = -1
	area.add_child(shadow)
	var sprite := Sprite2D.new()
	sprite.texture = load(_art_for_hunt(data)) as Texture2D
	sprite.position = Vector2(0, -29)
	sprite.scale = Vector2(0.38, 0.38)
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.z_index = 1
	area.add_child(sprite)
	var ring := Line2D.new()
	ring.points = PackedVector2Array([Vector2(-13, 2), Vector2(-7, 5), Vector2(7, 5), Vector2(13, 2)])
	ring.width = 1.0
	ring.default_color = _threat_color(hunt_id)
	ring.z_index = 0
	area.add_child(ring)
	area.body_entered.connect(func(body: Node2D) -> void:
		if body == null or body.name != "Player":
			return
		if GameManager.current_state != GameManager.GameState.EXPLORATION or GameManager.get_flag(defeated_flag):
			return
		GameManager.set_flag(defeated_flag)
		var enemy := BattleManager.Enemy.new(
			String(data.get("name", "Memory Echo")),
			int(data.get("hp", 70)),
			int(data.get("atk", 14)),
			bool(data.get("is_void", false))
		)
		enemy.abilities = Array(data.get("abilities", [])).duplicate()
		enemy.weakness = String(data.get("weakness", ""))
		enemy.resistance = String(data.get("resistance", ""))
		BattleManager.start_battle(enemy, "res://scenes/maps/%s.tscn" % map_id)
		SceneTransition.change_scene_battle("res://scenes/battle/battle_scene.tscn")
		area.call_deferred("queue_free")
	)
	root.add_child(area)
	return true

static func _tile_position(tile: Variant) -> Vector2:
	if tile is Array and tile.size() >= 2:
		return Vector2(float(tile[0]) * TILE_SIZE + TILE_SIZE * 0.5, float(tile[1]) * TILE_SIZE + TILE_SIZE * 0.5)
	return Vector2(TILE_SIZE * 1.5, TILE_SIZE * 1.5)

static func _art_for_voice(data: Dictionary) -> String:
	var id := String(data.get("id", ""))
	return String(SPECIAL_VOICE_ART.get(id, data.get("art", "")))

static func _art_for_hunt(data: Dictionary) -> String:
	var id := String(data.get("id", ""))
	return String(SPECIAL_HUNT_ART.get(id, data.get("art", "")))

static func _threat_color(hunt_id: String) -> Color:
	if "root" in hunt_id:
		return Color(0.42, 0.86, 0.48, 0.62)
	if "ash" in hunt_id or "scavenger" in hunt_id:
		return Color(0.96, 0.42, 0.20, 0.60)
	if "wraith" in hunt_id:
		return Color(0.82, 0.82, 0.90, 0.60)
	return Color(0.66, 0.42, 0.96, 0.66)
