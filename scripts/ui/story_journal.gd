## StoryJournal, 스토리 저널 / 코덱스
## PauseMenu에서 접근. 게임 진행 중 자동으로 기록되는 이벤트/NPC/선택 목록.
## ESC로 닫기.
extends CanvasLayer

var is_open: bool = false

# ── UI 노드 ──
var backdrop: TextureRect
var overlay: ColorRect
var main_panel: PanelContainer
var tab_events: Button
var tab_npcs: Button
var tab_world: Button
var tab_choices: Button
var tab_quests_btn: Button
var tab_losses_btn: Button
var tab_leads_btn: Button
var item_list: VBoxContainer
var item_scroll: ScrollContainer
var detail_title: Label
var detail_art: TextureRect
var detail_body: RichTextLabel
var close_hint: Label
var journal_summary_label: Label

var _current_tab: String = "events"
const JOURNAL_BACKDROP_PATH: String = "res://assets/cg/generated/ui_story_journal_backdrop_v3.png"

# ── 저널 엔트리 ──
# 자동으로 story_flags 기반으로 생성
const CHAPTER_NAMES := {
	1: "Rim Forest", 2: "Verdan Market", 3: "Belt Waystation", 4: "Drift Shelter",
	5: "Crumbling Coast", 6: "The Seam", 7: "Seam Outskirts", 8: "Forgotten Forest",
	9: "Colorless Waste", 10: "BL-07 Void", 11: "Epilogue", 12: "Verdan Underlock",
	13: "Eastern Reading Wall", 14: "Confessor Hall", 15: "Mneme Cell", 16: "East Road",
	17: "Forgetting Storm", 18: "Living Funeral", 19: "Lumea Approach", 20: "Monolith",
	21: "Records Chamber", 22: "The Core", 23: "Conversion", 24: "Testimony",
}

const EVENT_ART_BY_FLAG: Dictionary = {
	"ch1_camp_done": "res://assets/cg/generated/archive_ch1_camp_humming_v2.png",
	"ch2_malet_done": "res://assets/cg/generated/archive_ch2_information_price_v1.png",
	"ch3_kairos_writing": "res://assets/cg/generated/archive_ch3_kairos_marks_v1.png",
	"ch4_reading_loss": "res://assets/cg/generated/archive_ch4_reading_loss_v1.png",
	"elia_reunited": "res://assets/cg/generated/archive_ch6_reunion_v1.png",
	"ch8_tobias_theory": "res://assets/cg/generated/archive_ch8_ring_theory_v1.png",
}

# 이벤트 엔트리 (flag → 표시 정보)
const EVENT_ENTRIES := [
	{"flag": "ch1_opening_done", "chapter": 1, "title": "Awakening in the Forest", "title_ko": "숲에서 깨어나다", "desc": "Arrel came to in the Rim Forest, blade drawn, a dead Void Beast dissolving at his feet. No memory of the fight.", "desc_ko": "아렐은 림 숲에서 정신을 차렸다. 칼은 뽑혀 있었고, 발치에서는 죽은 보이드 비스트가 녹아내리고 있었다. 싸운 기억은 없었다.", "art": "res://assets/portraits/character_shots/arrel_battle_v3.png"},
	{"flag": "ch1_elia_appeared", "chapter": 1, "title": "Elia Appears", "title_ko": "엘리아, 나타나다", "desc": "A woman called Elia found him, or rather, had been following him. She spoke as if she knew him. He couldn't remember.", "desc_ko": "엘리아라는 여자가 그를 찾아냈다. 아니, 줄곧 뒤를 따라오고 있었다. 그를 아는 사람처럼 말했지만 그는 기억할 수 없었다.", "art": "res://assets/portraits/character_shots/elia_anchor_v3.png"},
	{"flag": "ch1_ash_rain_seen", "chapter": 1, "title": "Ash Rain", "title_ko": "잿비", "desc": "Gray flakes began to fall. Not snow, ash. The residue of dissolved memories, raining from the empty sky.", "desc_ko": "회색 조각이 떨어지기 시작했다. 눈이 아니라 재였다. 녹아 없어진 기억의 잔여물이 텅 빈 하늘에서 비처럼 내렸다."},
	{"flag": "hidden_ch1_stump", "chapter": 1, "title": "[Hidden] The Old Stump", "title_ko": "[숨은] 오래된 그루터기", "desc": "A tree stump with rings too numerous to count. Something about it felt important, older than the Collapse.", "desc_ko": "셀 수 없이 많은 나이테를 가진 그루터기. 무언가 중요하다는 느낌이 들었다. 붕괴보다도 오래된 것이었다."},
	{"flag": "ch1_camp_done", "chapter": 1, "title": "Camp at the Forest Edge", "title_ko": "숲 가장자리의 야영", "desc": "Night. Elia hummed a melody. Arrel's hand trembled. In the morning, they headed south toward Verdan.", "desc_ko": "밤. 엘리아가 멜로디를 흥얼거렸다. 아렐의 손은 떨렸다. 아침이 되자 둘은 남쪽 베르단으로 향했다."},
	{"flag": "ch2_arrived", "chapter": 2, "title": "Verdan Market", "title_ko": "베르단 시장", "desc": "The Gray Belt's largest settlement. Smoke, noise, and the smell of things being traded that shouldn't be.", "desc_ko": "그레이 벨트에서 가장 큰 정착지. 연기와 소음, 그리고 거래되어서는 안 될 것들이 거래되는 냄새."},
	{"flag": "malet_deal_accepted", "chapter": 2, "title": "Malet's Price, Paid", "title_ko": "말렛의 값, 치르다", "art": "res://assets/cg/generated/archive_verdan_memory_price_v1.png", "desc": "Malet extracted the memory of holding a sword for the first time. A courtyard, dust, a hand closing fingers around a grip. Gone.", "desc_ko": "말렛은 처음 칼을 쥐던 기억을 뽑아냈다. 안뜰, 먼지, 손잡이를 감싸 쥐던 손가락. 사라졌다."},
	{"flag": "malet_deal_refused", "chapter": 2, "title": "Malet's Price, Refused", "title_ko": "말렛의 값, 거절하다", "desc": "Arrel refused to sell his sword memory. The amber-eyed dealer's dismissal was absolute.", "desc_ko": "아렐은 칼의 기억을 팔기를 거절했다. 호박색 눈의 거래상은 조금도 아쉬워하지 않았다."},
	{"flag": "ch2_malet_done", "chapter": 2, "title": "Information Acquired", "title_ko": "정보를 얻다", "desc": "Three things from Malet: a route through the Coast, a name (Sable), and a warning, Editor Kairos. Four days.", "desc_ko": "말렛에게서 셋을 얻었다. 해안을 지나는 경로, 이름 하나(세이블), 그리고 경고 하나. 편집관 카이로스. 나흘."},
	{"flag": "ch3_arrived", "chapter": 3, "title": "The Belt Waystation", "title_ko": "벨트 중계소", "desc": "A dead road through dead earth. Bureau Relay Station 14 stands alone, and someone new sits inside.", "desc_ko": "죽은 땅을 가로지르는 죽은 길. 관리국 제14 중계소가 홀로 서 있고, 그 안에 낯선 사람이 앉아 있다."},
	{"flag": "ch3_tobias_met", "chapter": 3, "title": "Tobias Crane", "title_ko": "토비아스 크레인", "desc": "A Bureau Recorder with ink-stained fingers. Twenty years of memory transactions. He sees things others don't.", "desc_ko": "잉크로 물든 손가락을 가진 관리국 기록관. 이십 년 동안 기억 거래를 적어 왔다. 남들이 못 보는 것을 본다.", "art": "res://assets/portraits/character_shots/tobias_ledger_v3.png"},
	{"flag": "has_blank_book", "chapter": 3, "title": "The Blank Book", "title_ko": "백지의 책", "art": "res://assets/cg/generated/archive_belt_blank_book_v1.png", "desc": "Record-tree fiber. It absorbs the shape of memories, the contour remains even after the memory burns.", "desc_ko": "기록목 섬유로 만들어졌다. 기억의 형태를 빨아들여, 기억이 타 버린 뒤에도 윤곽이 남는다."},
	{"flag": "ch3_kairos_writing", "chapter": 3, "title": "Wall Writing", "title_ko": "벽의 글씨", "desc": "'Subject demonstrates Class Seven combustion efficiency.' Scratched into concrete. Recent. Someone is watching.", "desc_ko": "'대상은 7등급 연소 효율을 보임.' 콘크리트에 긁어 새겨져 있었다. 최근의 것이다. 누군가 지켜보고 있다."},
	{"flag": "ch4_arrived", "chapter": 4, "title": "Drift", "title_ko": "드리프트", "desc": "A collapsed overpass. Memory rain falling. The letters in the Blank Book swim and blur.", "desc_ko": "무너진 고가도로. 기억의 비가 내린다. 백지의 책에 적힌 글자들이 헤엄치며 흐려진다."},
	{"flag": "ch4_reading_loss", "chapter": 4, "title": "Reading Deterioration", "title_ko": "읽기 저하", "desc": "Words move when they shouldn't. A known side effect, Tobias says. 'Recovery not guaranteed.'", "desc_ko": "가만히 있어야 할 글자가 움직인다. 알려진 부작용이라고 토비아스는 말했다. '회복은 보장되지 않습니다.'"},
	{"flag": "ch4_anchoring", "chapter": 4, "title": "Anchoring Session", "title_ko": "앵커링", "art": "res://assets/cg/generated/archive_drift_anchoring_v1.png", "desc": "Warm hands. The smell of bread. A page pressed back into its binding. Elia tethers the architecture.", "desc_ko": "따뜻한 손. 빵 냄새. 제본 안으로 다시 눌러 넣은 한 장. 엘리아가 구조를 붙들어 맨다."},
	{"flag": "ch5_arrived", "chapter": 5, "title": "The Crumbling Coast", "title_ko": "무너지는 해안", "desc": "Where land forgets how to be solid. Salt air and dissolved memory leaching into the sea.", "desc_ko": "땅이 단단해지는 법을 잊은 곳. 소금기 밴 공기와, 녹아 바다로 스며드는 기억."},
	{"flag": "ch5_kairos_seen", "chapter": 5, "title": "Kairos Observed", "title_ko": "카이로스를 목격하다", "desc": "A figure on the ridge. Still as stone. Not chasing, classifying. Elia said that was worse.", "desc_ko": "능선 위의 형체. 돌처럼 미동도 없었다. 쫓는 게 아니라 분류하고 있었다. 엘리아는 그게 더 나쁘다고 했다.", "art": "res://assets/portraits/character_shots/kairos_edit_v3.png"},
	{"flag": "elia_separates", "chapter": 5, "title": "Elia Took the Coastal Path", "title_ko": "엘리아, 해안길로 가다", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png", "desc": "To split Kairos's attention, Elia went south alone. Without her, burned memories would leave no residue. Just absence.", "desc_ko": "카이로스의 주의를 나누려고 엘리아는 홀로 남쪽으로 갔다. 그녀가 없으면 태운 기억은 잔존을 남기지 않는다. 그저 부재만."},
	{"flag": "elia_stays", "chapter": 5, "title": "Together Through the Coast", "title_ko": "해안을 함께 지나다", "desc": "They stayed together. Two signatures, easier to track, but harder to break.", "desc_ko": "둘은 함께 남았다. 신호가 둘이면 추적하기는 쉬워지지만, 꺾기는 어려워진다."},
	{"flag": "ch6_arrived", "chapter": 6, "title": "The Seam", "title_ko": "더 심", "desc": "Color between gray cliffs. Amber, crimson, green. A settlement in the cracks of what was and what will be.", "desc_ko": "회색 절벽 사이의 색. 호박빛, 진홍, 초록. 있었던 것과 있을 것 사이의 틈에 들어선 정착지."},
	{"flag": "elia_reunited", "chapter": 6, "title": "Reunion", "title_ko": "재회", "desc": "Elia stood at the Seam's entrance. The coastal path worked, Kairos went south. The anchor tightened.", "desc_ko": "엘리아가 심 입구에 서 있었다. 해안길은 통했고 카이로스는 남쪽으로 갔다. 앵커가 다시 조여졌다."},
	{"flag": "hidden_ch6_garden", "chapter": 6, "title": "[Hidden] The Impossible Garden", "title_ko": "[숨은] 있을 수 없는 정원", "desc": "White petals veined with gold. Warm to the touch. A fragment of someone handing a flower, small hands, a child's laugh.", "desc_ko": "금빛 잎맥이 지나는 흰 꽃잎. 만지면 따뜻하다. 누군가 꽃을 건네던 조각, 작은 손, 아이의 웃음소리."},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "Sable's Briefing", "title_ko": "세이블의 보고", "desc": "BL-07 forming south. If it opens, the Seam dies. A Shade Sentinel guards the entrance. Investigation required.", "desc_ko": "남쪽에 BL-07이 형성되고 있다. 열리면 심은 죽는다. 입구는 그림자 파수꾼이 지킨다. 조사가 필요하다.", "art": "res://assets/cg/generated/archive_seam_lantern_watch_v1.png"},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "The Warden's Watch", "title_ko": "파수꾼의 밤", "desc": "Sable never promised safety. She stood beside Arrel anyway, keeping watch while the Seam slept.", "desc_ko": "세이블은 안전을 약속한 적이 없다. 그래도 아렐 곁에 서서, 심이 잠든 동안 망을 봤다.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_sable_watch_v1.png"},
	{"flag": "ch6_bl07_entered", "chapter": 6, "title": "The Shade Sentinel", "title_ko": "그림자 파수꾼", "desc": "Dark. Wrong. The Void's immune response. Between them and BL-07, it coalesced.", "desc_ko": "어둡고, 어긋나 있었다. 보이드의 면역 반응. 그들과 BL-07 사이에서 그것이 뭉쳐 올랐다."},
	{"flag": "ch7_arrived", "chapter": 7, "title": "The Threshold", "title_ko": "경계", "desc": "Beyond The Seam's walls, the cliffs opened into a jagged plateau. The air tasted of static.", "desc_ko": "심의 성벽 너머, 절벽이 갈라져 들쭉날쭉한 대지가 열렸다. 공기에서 정전기 맛이 났다."},
	{"flag": "ch7_sable_truth", "chapter": 7, "title": "Sable's Truth", "title_ko": "세이블의 진실", "art": "res://assets/cg/generated/archive_ch7_seven_lanterns_v1.png", "desc": "BL-07 isn't a hole. It's a mouth. It calls memories. It's hungry. Seven people went in with Sable. She came out alone.", "desc_ko": "BL-07은 구멍이 아니다. 입이다. 기억을 부른다. 굶주려 있다. 세이블과 함께 일곱이 들어갔고, 그녀만 나왔다."},
	{"flag": "has_echo_shell", "chapter": 7, "title": "The Echo Shell", "title_ko": "메아리 껍데기", "desc": "A spiraling shell covered in luminescent veins. It holds the last words of everyone BL-07 consumed.", "desc_ko": "발광하는 잎맥으로 덮인 나선형 껍데기. BL-07이 삼킨 모든 이의 마지막 말이 담겨 있다."},
	{"flag": "ch7_trial_complete", "chapter": 7, "title": "Sable's Trial", "title_ko": "세이블의 시련", "desc": "A controlled burn on the Threshold. The memory passed like a wave of heat. Arrel held. Most people don't.", "desc_ko": "경계에서의 통제된 연소. 기억이 열기의 물결처럼 지나갔다. 아렐은 버텼다. 대부분은 버티지 못한다."},
	{"flag": "ch8_arrived", "chapter": 8, "title": "The Forest That Forgets", "title_ko": "잊는 숲", "desc": "Trees that weren't growing, standing because they forgot how to fall. Body-temperature bark. Memory-parasitic ecosystem.", "desc_ko": "자라지 않으면서도 쓰러지는 법을 잊어 서 있는 나무들. 체온과 같은 나무껍질. 기억에 기생하는 생태계."},
	{"flag": "ch8_ghost", "chapter": 8, "title": "The Remnants", "title_ko": "잔재들", "desc": "Shapes between the trees. Mouths open in soundless words. What's left when BL-07 takes everything except the shape.", "desc_ko": "나무 사이의 형체들. 소리 없는 말에 입을 벌린 채. BL-07이 형태만 남기고 전부 가져갔을 때 남는 것."},
	{"flag": "ch8_tobias_theory", "chapter": 8, "title": "Ring Theory", "title_ko": "나이테 이론", "desc": "The forest grows in concentric rings. Each ring is a feeding event, a generation of people erased. Eighteen rings total.", "desc_ko": "숲은 동심원으로 자란다. 원 하나가 포식 한 번이고, 지워진 한 세대다. 모두 열여덟 개."},
	{"flag": "ch9_arrived", "chapter": 9, "title": "Where Colors Stop", "title_ko": "색이 멎는 곳", "desc": "Color ended like a sentence cut short. Not gray, the concept of color simply withdrew from the world.", "desc_ko": "색이 잘린 문장처럼 끝났다. 회색이 아니라, 색이라는 개념이 세계에서 물러나 버린 것이다."},
	{"flag": "ch9_compass", "chapter": 9, "title": "The Memory Compass", "title_ko": "기억 나침반", "desc": "BL-07 recognizes Class Seven combustion. Arrel's body became a compass, pulled toward the Void Hole's core.", "desc_ko": "BL-07은 7등급 연소를 알아본다. 아렐의 몸이 나침반이 되어 보이드 홀의 중심으로 끌려간다."},
	{"flag": "ch9_kairos", "chapter": 9, "title": "Kairos Confrontation", "title_ko": "카이로스와 마주하다", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png", "desc": "The Editor appeared in the Waste. Not to stop them, to observe the convergence. Two outcomes. Both terrible.", "desc_ko": "편집관이 황무지에 나타났다. 막으려는 게 아니라 수렴을 관찰하러. 결과는 둘. 둘 다 끔찍하다."},
	{"flag": "ch9_kairos_truth", "chapter": 9, "title": "Kairos's Calculation", "title_ko": "카이로스의 계산", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png", "desc": "Outcome A: BL-07 collapses, subject loses everything. Outcome B: BL-07 doubles. Sable said they'd find a third.", "desc_ko": "결과 A. BL-07이 붕괴하고 대상은 전부를 잃는다. 결과 B. BL-07이 두 배가 된다. 세이블은 세 번째를 찾겠다고 했다."},
	{"flag": "ch10_complete", "chapter": 10, "title": "The Seal", "title_ko": "봉인", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "BL-07's core. A decision: burn everything to close it, or keep your name and find another way.", "desc_ko": "BL-07의 중심. 결정해야 한다. 전부 태워 닫을 것인가, 이름을 지키고 다른 길을 찾을 것인가."},
	{"flag": "zero_burn_path", "chapter": 10, "title": "Zero Burn, Name Consumed", "title_ko": "무연소, 이름을 삼키다", "desc": "He burned 'Arrel.' The name that meant something. The Void Hole collapsed. He didn't know who he was anymore.", "desc_ko": "그는 '아렐'을 태웠다. 무언가를 뜻하던 그 이름을. 보이드 홀은 무너졌다. 그는 자기가 누구인지 더는 알지 못했다."},
	{"flag": "seal_refused", "chapter": 10, "title": "Preservation, Name Kept", "title_ko": "보존, 이름을 지키다", "desc": "He refused to burn his name. BL-07 remains unsolved. But he remembers who he is.", "desc_ko": "그는 이름 태우기를 거절했다. BL-07은 풀리지 않은 채 남았다. 대신 그는 자기가 누구인지 기억한다."},
	{"flag": "seal_weave", "chapter": 10, "title": "The Weave, Everything Kept", "title_ko": "엮음, 전부를 지키다", "desc": "He offered every memory he had preserved, all at once. BL-07 sealed without erasing him. Part of him now holds the door shut.", "desc_ko": "그는 지켜 온 기억 전부를 한꺼번에 내밀었다. BL-07은 그를 지우지 않고 봉인되었다. 이제 그의 일부가 그 문을 붙들고 있다."},
	{"flag": "epilogue_complete", "chapter": 11, "title": "Epilogue", "title_ko": "에필로그", "desc": "The Seam. Aftermath. What remains after everything is either burned or saved.", "desc_ko": "더 심. 그 뒤의 일. 태워지거나 지켜지고 난 다음에 남는 것."},
	{"flag": "ch13_trusted_tobias", "chapter": 13, "title": "The Record Tree Between Them", "title_ko": "둘 사이의 기록목", "desc": "Arrel trusted Tobias with the shape of a memory neither man could safely carry alone.", "desc_ko": "아렐은 어느 쪽도 혼자 안전하게 질 수 없는 기억의 형태를 토비아스에게 맡겼다.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_tobias_record_tree_v1.png"},
	{"flag": "storm_survived", "chapter": 17, "title": "A Thread Through the Storm", "title_ko": "폭풍을 지나는 실 한 가닥", "desc": "When the forgetting storm took the road, Arrel and Elia kept one another present by touch and voice.", "desc_ko": "망각의 폭풍이 길을 삼켰을 때, 아렐과 엘리아는 손과 목소리로 서로를 현재에 붙들어 두었다.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_elia_storm_thread_v1.png"},
	{"flag": "nera_first_emotion", "chapter": 21, "title": "The Mirror Refused Its Order", "title_ko": "거울이 명령을 거부하다", "desc": "Elia recognized the first human hesitation beneath Nera's immaculate Authority mask.", "desc_ko": "엘리아는 네라의 완벽한 관리국 가면 아래에서 첫 인간적인 망설임을 알아보았다.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_elia_nera_mirror_v1.png"},
	{"flag": "kairos_final", "chapter": 21, "title": "The Missing Piece", "title_ko": "빠진 조각", "desc": "Arrel and Kairos faced the same broken record and chose opposite meanings for its empty space.", "desc_ko": "아렐과 카이로스는 같은 부서진 기록을 마주하고, 그 빈자리에 정반대의 의미를 골랐다.", "art": "res://assets/cg/generated/illustration_expansion_v2/bond_arrel_kairos_missing_piece_v1.png"},
]

# NPC 엔트리
const NPC_ENTRIES := [
	{"flag": "ch1_elia_appeared", "name": "Elia", "role": "Anchor / Companion", "role_ko": "앵커 · 동행자", "art": "res://assets/portraits/character_shots/elia_anchor_v3.png",
	 "desc": "Silver-haired woman who follows Arrel. Her presence slows memory decay, an 'anchoring' effect. Knows more than she says.", "desc_ko": "아렐을 따라다니는 은발의 여자. 그녀가 곁에 있으면 기억의 붕괴가 느려진다. 이를 '앵커링'이라 한다. 말하는 것보다 훨씬 많이 알고 있다."},
	{"flag": "ch2_arrived", "name": "Malet", "role": "Memory Dealer", "role_ko": "기억 거래상",
	 "desc": "Small, thin, dressed in gray. Amber eyes from high-grade Memory Ampoules. Trades information for memories in the Sump beneath Verdan.", "desc_ko": "작고 마른 몸에 회색 옷차림. 고급 기억 앰풀 탓에 눈이 호박색이다. 베르단 아래 슘에서 기억을 받고 정보를 판다."},
	{"flag": "ch3_tobias_met", "name": "Tobias Crane", "name_ko": "토비아스 크레인", "role": "Bureau Recorder / Class C", "role_ko": "관리국 기록관 · C등급", "art": "res://assets/portraits/character_shots/tobias_ledger_v3.png",
	 "desc": "Ink-stained fingers, spectacles. Twenty years recording memory transactions. Walked the Belt alone to update records. Knows the Bureau inside out.", "desc_ko": "잉크로 물든 손가락과 안경. 이십 년간 기억 거래를 기록해 왔다. 장부를 갱신하러 홀로 벨트를 걸었다. 관리국의 안팎을 훤히 안다."},
	{"flag": "ch6_arrived", "name": "Sable", "role": "Seam Leader / Void Walker", "role_ko": "심의 지도자 · 보이드 워커", "art": "res://assets/portraits/character_shots/sable_warden_v3.png",
	 "desc": "An older Void Walker with clouded gray eyes, silver hair, and a dark-plum hood. Walked into a Void Hole and walked out. Leads the Seam through practical routines, mutual witness, and hard choices.", "desc_ko": "흐린 잿빛 눈과 은발, 짙은 자두색 두건을 쓴 나이 든 보이드 워커. 보이드 홀로 걸어 들어갔다 걸어 나왔다. 실용적인 규율과 상호 증언, 그리고 모진 결정으로 심을 이끈다."},
	{"flag": "ch5_kairos_seen", "name": "Kairos", "role": "Editor / Pursuer", "role_ko": "편집관 · 추적자", "art": "res://assets/portraits/character_shots/kairos_edit_v3.png",
	 "desc": "An Authority Editor. Quiet. Methodical. Classifies rather than chases. His presence means the Authority knows about Arrel's burning.", "desc_ko": "관리국 편집관. 조용하고 치밀하다. 쫓기보다 분류한다. 그가 나타났다는 것은 관리국이 아렐의 연소를 알고 있다는 뜻이다."},
]

# World entries are deliberately chapter-gated. They explain the pressure around the party
# without revealing later-book answers before the player has reached the relevant place.
const WORLD_ENTRIES := [
	{"flag": "ch1_ash_rain_seen", "chapter": 1, "title": "Ash Rain, What the Sky Keeps", "title_ko": "잿비, 하늘이 간직하는 것", "art": "res://assets/cg/generated/story_ch1_ash_rain_touch.png",
	 "desc": "Ash Rain is not weather. When extracted memories are broken, their emotional residue disperses into the air and eventually falls as gray flakes. It can carry a pressure, a scent, or the edge of a feeling, never a complete life. The Rim has learned to close its windows before it starts.", "desc_ko": "잿비는 날씨가 아니다. 뽑아낸 기억이 부서지면 그 감정의 잔여물이 공기 중으로 흩어졌다가 회색 조각이 되어 떨어진다. 압박감이나 냄새, 감정의 가장자리 정도는 실어 오지만 온전한 삶은 결코 담기지 않는다. 림 사람들은 잿비가 시작되기 전에 창을 닫는 법을 익혔다."},
	{"flag": "ch2_arrived", "chapter": 2, "title": "Grains, Ampoules, and Debt", "title_ko": "그레인, 앰풀, 그리고 빚", "art": "res://assets/environment/map_canvases/map_verdan_market_canvas_v1.png",
	 "desc": "A memory can be weighed, graded, sealed in an ampoule, and traded as Grains. It can buy food, passage, or a lie someone needs to hear. But a purchased memory is only a visitor in another mind: it may be felt, never truly burned as fuel. That boundary is why the market can profit from memory without giving everyone Arrel's power.", "desc_ko": "기억은 무게를 재고 등급을 매겨 앰풀에 봉인한 뒤 그레인으로 거래할 수 있다. 음식도, 통행도, 누군가 들어야 할 거짓말도 살 수 있다. 그러나 사들인 기억은 다른 사람의 머릿속에서는 방문객일 뿐이다. 느낄 수는 있어도 연료로 태울 수는 없다. 시장이 기억으로 이윤을 남기면서도 모두가 아렐의 힘을 갖지는 못하는 이유가 그 경계에 있다."},
	{"flag": "ch2_malet_done", "chapter": 2, "title": "Information Is a Delayed Weapon", "title_ko": "정보는 시차를 둔 무기다", "art": "res://assets/cg/generated/archive_ch2_information_price_v1.png",
	 "desc": "Malet profits from more than secrets. A route disclosed today can outrun an Authority courier by one night; a name withheld can keep a settlement alive until morning. In the Gray Belt, information has value because every system arrives late, and someone always pays for the time between knowing and acting.", "desc_ko": "말렛이 파는 것은 비밀만이 아니다. 오늘 알려 준 경로 하나가 관리국 전령보다 하룻밤 앞설 수 있고, 감춰 둔 이름 하나가 정착지를 아침까지 살려 둘 수 있다. 그레이 벨트에서 정보에 값이 붙는 것은 모든 체계가 늦게 도착하기 때문이고, 아는 것과 움직이는 것 사이의 시간을 누군가는 반드시 지불하기 때문이다."},
	{"flag": "ch3_arrived", "chapter": 3, "title": "The Belt Watches in Delays", "title_ko": "벨트는 시차를 두고 지켜본다", "art": "res://assets/environment/map_canvases/map_belt_signal_yard_canvas_v1.png",
	 "desc": "The Authority's signal towers catch the flare left by memory combustion. In the Belt, Arrel's unusually bright signature is a beacon; in the Rim, sparse relays turn it into a rumor. The system is frightening, not perfect: scribes must still write, couriers must still travel, and frightened citizens decide whether to report what they saw.", "desc_ko": "관리국의 신호탑은 기억 연소가 남기는 섬광을 잡아낸다. 벨트에서 아렐의 유난히 밝은 신호는 봉화나 다름없고, 중계가 성긴 림에서는 소문이 된다. 이 체계는 무섭지만 완전하지는 않다. 필경사는 여전히 손으로 적어야 하고, 전령은 여전히 걸어야 하며, 겁먹은 주민은 본 것을 신고할지 스스로 정한다."},
	{"flag": "has_blank_book", "chapter": 3, "title": "Record-Tree Paper", "title_ko": "기록목 종이", "art": "res://assets/cg/generated/story_ch3_waystation_blank_book.png",
	 "desc": "The Blank Book is made from the fiber of a record-tree, a First-Age plant that does not answer the Authority's record ink. Its pages cannot be scanned by the Monolith and must be copied by hand. That freedom has a cost: destroy the physical page and there is no archive beneath it, no recovery, and no official proof it ever existed.", "desc_ko": "백지의 책은 기록목 섬유로 만들어졌다. 관리국의 기록 잉크에 반응하지 않는 제1시대 식물이다. 그 책장은 모노리스가 판독할 수 없어 손으로 옮겨 적어야 한다. 그 자유에는 값이 따른다. 실물 책장을 없애면 그 아래에 아무 기록도 없고, 복구도, 존재했다는 공식 증거도 없다."},
	{"flag": "ch4_anchoring", "chapter": 4, "title": "Anchoring Is a Practice, Not a Cure", "title_ko": "앵커링은 치료가 아니라 습관이다", "art": "res://assets/environment/map_canvases/map_drift_shelter_canvas_v2.png",
	 "desc": "An anchor does not restore what was burned. Elia uses ordinary, repeatable sensory facts, the warmth of bread, a hand around a cup, a familiar song, to give damaged memory somewhere to settle. It keeps a person from coming apart in the moment. It cannot promise that the page will still be there tomorrow.", "desc_ko": "앵커는 태워 없앤 것을 되돌리지 못한다. 엘리아는 평범하고 되풀이할 수 있는 감각적 사실을 쓴다. 빵의 온기, 잔을 감싼 손, 익숙한 노래. 손상된 기억이 내려앉을 자리를 만들어 주는 것이다. 그 순간 사람이 흩어지지 않게 붙들 수는 있어도, 내일도 그 책장이 남아 있으리라고 약속하지는 못한다."},
	{"flag": "ch5_arrived", "chapter": 5, "title": "The Coast Returns Pressure, Not People", "title_ko": "해안은 사람이 아니라 압력을 돌려준다", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png",
	 "desc": "Dissolved memories wash back as salt-light, familiar gestures, and dreams that belong to no single survivor. Coastal families leave two route markers at every dangerous fork: one for the traveler and one for the witness expected to remember which way they went. A returned feeling may deserve care without being mistaken for a restored life.", "desc_ko": "녹아 흩어진 기억은 소금빛과 낯익은 몸짓, 어느 생존자의 것도 아닌 꿈이 되어 밀려온다. 해안 사람들은 위험한 갈림길마다 표지를 둘 남긴다. 하나는 길 떠나는 사람 몫이고, 하나는 그가 어느 쪽으로 갔는지 기억해 줄 증인 몫이다. 돌아온 감정은 돌보아 마땅하지만, 되살아난 삶으로 착각해서는 안 된다."},
	{"flag": "ch6_arrived", "chapter": 6, "title": "The Seam's Refuge Pact", "title_ko": "심의 피난 규약", "art": "res://assets/environment/map_canvases/map_the_seam_canvas_v1.png",
	 "desc": "The Seam survives because people share what the Authority turns into leverage: names, meals, routes, and witnesses. No one is asked to prove a loss with an official record. Each resident is expected to remember one practical thing for someone else, a safe path, a medicine recipe, a face that must not vanish.", "desc_ko": "심이 살아남는 이유는 관리국이 지렛대로 삼는 것들을 사람들이 서로 나누기 때문이다. 이름, 끼니, 경로, 그리고 증인. 누구에게도 공식 기록으로 상실을 증명하라고 요구하지 않는다. 대신 주민 각자가 남을 위해 실용적인 것 하나를 기억해야 한다. 안전한 길, 약 짓는 법, 사라져서는 안 될 얼굴 하나."},
	{"flag": "ch6_briefing_done", "chapter": 6, "title": "A Refuge Is Made of Work", "title_ko": "피난처는 노동으로 이루어진다", "art": "res://assets/game_image/reference/seam_residents_reference_sheet_v1.png",
	 "desc": "The Seam has no permanent monument to its dead. It has lantern rotations, witness ledgers, a garden roster, and scouts who bring route knots home. Sable assigns every newcomer one task that preserves another person's tomorrow. Here, maintenance is not background labor; it is how an undocumented community refuses to disappear.", "desc_ko": "심에는 죽은 이를 위한 영구 기념물이 없다. 대신 랜턴 당번과 증언 장부, 정원 배정표, 그리고 경로의 매듭을 들고 돌아오는 정찰대가 있다. 세이블은 새로 온 사람 모두에게 다른 누군가의 내일을 지키는 일 하나를 맡긴다. 이곳에서 유지 보수는 뒷일이 아니라, 기록되지 않은 공동체가 사라지기를 거부하는 방식이다."},
	{"flag": "ch7_sable_truth", "chapter": 7, "title": "What a Void Hole Returns", "title_ko": "보이드 홀이 돌려주는 것", "art": "res://assets/cg/character_shots/echo_shell_shot_v2.png",
	 "desc": "BL-07 does not simply kill. It strips a life into pressure, gesture, and unfinished voice. Echo Shells are not the people they imitate, but neither are they empty monsters. Treating them as disposable makes the Void's work easier; listening to them risks letting their hunger answer back.", "desc_ko": "BL-07은 그저 죽이지 않는다. 하나의 삶을 압박감과 몸짓, 끝맺지 못한 목소리로 발라낸다. 메아리 껍데기는 그들이 흉내 내는 사람이 아니지만, 텅 빈 괴물도 아니다. 함부로 버려도 되는 것으로 취급하면 보이드의 일이 쉬워지고, 귀를 기울이면 그 굶주림이 되받아 답할 위험을 진다."},
	{"flag": "ch8_tobias_theory", "chapter": 8, "title": "The Listening Wood", "title_ko": "듣는 숲", "art": "res://assets/cg/generated/archive_ch8_ring_theory_v1.png",
	 "desc": "The forest grows around repeated erasures. Its roots remember heat and fear as if they were water, while its paths borrow familiar shapes to lead travelers deeper. The rings Tobias counted are not years. They are feeding events, places where too many people were reduced to echoes at once.", "desc_ko": "이 숲은 되풀이된 지움을 둘러싸고 자란다. 뿌리는 열기와 공포를 물처럼 기억하고, 길은 낯익은 형태를 빌려 나그네를 더 깊이 끌어들인다. 토비아스가 센 나이테는 햇수가 아니다. 한 번에 너무 많은 사람이 메아리로 줄어든 자리, 곧 포식의 흔적이다."},
	{"flag": "ch9_compass", "chapter": 9, "title": "Witnesses in the Colorless Waste", "title_ko": "무색 황무지의 증인들", "art": "res://assets/environment/map_canvases/map_colorless_waste_canvas_v2.png",
	 "desc": "Here a compass points toward the densest surviving memory rather than north. Caravans trade testimony in pairs: one person speaks, another confirms they heard it. It is a fragile economy built against revision. In a place color itself can forget, an unwitnessed story is already halfway gone.", "desc_ko": "이곳에서 나침반은 북쪽이 아니라 가장 짙게 살아남은 기억을 가리킨다. 캐러밴은 증언을 둘씩 짝지어 거래한다. 한 사람이 말하고, 다른 사람이 들었음을 확인해 준다. 개정에 맞서 세운 위태로운 경제다. 색조차 잊을 수 있는 곳에서, 증인 없는 이야기는 이미 절반쯤 사라진 것이다."},
	{"flag": "ch9_kairos_truth", "chapter": 9, "title": "Prediction Is Not Responsibility", "title_ko": "예측은 책임이 아니다", "art": "res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png",
	 "desc": "Editors are trained to make suffering legible as probability: acceptable loss, unstable subject, necessary revision. Kairos can describe both futures with precision, but a correct forecast does not choose who bears its cost. The Authority's deepest habit is not ignorance. It is treating responsibility as a category someone else will fill.", "desc_ko": "편집관은 고통을 확률로 읽히게 만들도록 훈련받는다. 허용 가능한 손실, 불안정한 대상, 필요한 개정. 카이로스는 두 미래를 정확히 서술할 수 있지만, 맞는 예측이 그 값을 누가 치를지 골라 주지는 않는다. 관리국의 가장 깊은 습관은 무지가 아니다. 책임을 다른 누군가가 채워 넣을 항목으로 취급하는 것이다."},
	{"flag": "ch10_complete", "chapter": 10, "title": "The Seal Is a Choice of Burdens", "title_ko": "봉인은 짐을 고르는 일이다", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png",
	 "desc": "Closing a wound in the world is not the same as making it whole. Every answer at BL-07 asks who carries the cost afterward: Arrel's name, the people waiting beyond the door, or a future problem left alive. The world changes most when someone refuses to call that cost inevitable.", "desc_ko": "세계의 상처를 닫는 것과 온전하게 만드는 것은 같지 않다. BL-07에서 나오는 모든 답은 그 값을 누가 지고 갈지를 묻는다. 아렐의 이름인지, 문 너머에서 기다리는 사람들인지, 아니면 살려 둔 미래의 문제인지. 세계가 가장 크게 바뀌는 것은 누군가 그 값을 불가피하다고 부르기를 거부할 때다."},
]

func _ready() -> void:
	layer = 57  # OptionsMenu(56) 위
	_build_ui()
	_hide_ui()
	print("[StoryJournal] Ready, Codex")

func _unhandled_input(event: InputEvent) -> void:
	if not is_open:
		return
	if event.is_action_pressed("cancel"):
		close_journal()
		get_viewport().set_input_as_handled()

func open_journal() -> void:
	if is_open:
		return
	is_open = true
	AudioManager.play_sfx("ui_open")
	_current_tab = "events"
	_refresh_list()
	_show_ui()

func close_journal() -> void:
	if not is_open:
		return
	is_open = false
	AudioManager.play_sfx("ui_close")
	_hide_ui()

## ===================== UI 구축 =====================

func _build_ui() -> void:
	var root = Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	backdrop = TextureRect.new()
	backdrop.set_anchors_preset(Control.PRESET_FULL_RECT)
	backdrop.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	backdrop.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	backdrop.mouse_filter = Control.MOUSE_FILTER_IGNORE
	backdrop.modulate = Color(0.86, 0.78, 0.70, 0.94)
	if ResourceLoader.exists(JOURNAL_BACKDROP_PATH):
		backdrop.texture = load(JOURNAL_BACKDROP_PATH)
	root.add_child(backdrop)

	overlay = ColorRect.new()
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.010, 0.010, 0.015, 0.66)
	root.add_child(overlay)

	main_panel = PanelContainer.new()
	main_panel.anchor_left = 0.06
	main_panel.anchor_right = 0.94
	main_panel.anchor_top = 0.04
	main_panel.anchor_bottom = 0.96
	main_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.050, 0.042, 0.038, 0.90),
		Color(0.62, 0.45, 0.24, 0.72),
		2, 6, 16
	))
	root.add_child(main_panel)

	var main_vbox = VBoxContainer.new()
	main_vbox.add_theme_constant_override("separation", 10)
	main_panel.add_child(main_vbox)

	# ── 헤더 ──
	var header = Label.new()
	header.text = _loc("JOURNAL, Field Notes of a Memory Carrier", "일지 · 기억 운반자의 현장 기록")
	header.add_theme_font_size_override("font_size", 18)
	header.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
	main_vbox.add_child(header)

	journal_summary_label = Label.new()
	journal_summary_label.add_theme_font_size_override("font_size", 13)
	journal_summary_label.add_theme_color_override("font_color", Color(0.64, 0.58, 0.48))
	journal_summary_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	main_vbox.add_child(journal_summary_label)

	# ── 탭 ──
	var tab_row = HBoxContainer.new()
	tab_row.add_theme_constant_override("separation", 8)
	main_vbox.add_child(tab_row)

	tab_events = _create_tab(_loc("Events", "사건"), "events")
	tab_row.add_child(tab_events)
	tab_npcs = _create_tab(_loc("People", "인물"), "npcs")
	tab_row.add_child(tab_npcs)
	tab_world = _create_tab(_loc("World", "세계"), "world")
	tab_row.add_child(tab_world)
	tab_choices = _create_tab(_loc("Choices", "선택"), "choices")
	tab_row.add_child(tab_choices)
	tab_quests_btn = _create_tab(_loc("Quests", "의뢰"), "quests")
	tab_row.add_child(tab_quests_btn)
	tab_losses_btn = _create_tab(_loc("Losses", "상실"), "losses")
	tab_row.add_child(tab_losses_btn)
	# S217: 미해결 단서. 저널은 "끝난 일"만 기록해서, 지역에 무엇이 남았는지
	# 알려 주지 않았다. 남은 것을 세어 주는 탭이 하나 필요하다.
	tab_leads_btn = _create_tab("미해결" if GameManager.current_locale == "ko" else "Leads", "leads")
	tab_row.add_child(tab_leads_btn)

	# 구분선
	var sep = HSeparator.new()
	sep.add_theme_color_override("separator", UITheme.BORDER_DIM)
	main_vbox.add_child(sep)

	# ── 본문 ──
	var content = HBoxContainer.new()
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.add_theme_constant_override("separation", 12)
	main_vbox.add_child(content)

	# 좌측: 목록
	item_scroll = ScrollContainer.new()
	item_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	content.add_child(item_scroll)

	item_list = VBoxContainer.new()
	item_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	item_list.add_theme_constant_override("separation", 4)
	item_scroll.add_child(item_list)

	# 우측: 상세
	var detail_panel = PanelContainer.new()
	detail_panel.custom_minimum_size = Vector2(380, 0)
	detail_panel.add_theme_stylebox_override("panel", UITheme.make_panel_style(
		Color(0.06, 0.05, 0.04, 0.8), UITheme.BORDER_DIM, 1, 4, 16
	))
	content.add_child(detail_panel)

	var detail_vbox = VBoxContainer.new()
	detail_vbox.add_theme_constant_override("separation", 10)
	detail_panel.add_child(detail_vbox)

	detail_title = Label.new()
	detail_title.text = _loc("Select an entry...", "항목을 고르세요...")
	detail_title.add_theme_font_size_override("font_size", 16)
	detail_title.add_theme_color_override("font_color", Color(0.8, 0.72, 0.58))
	detail_vbox.add_child(detail_title)

	var sep2 = HSeparator.new()
	sep2.add_theme_color_override("separator", UITheme.BORDER_DIM)
	detail_vbox.add_child(sep2)

	detail_art = TextureRect.new()
	detail_art.custom_minimum_size = Vector2(360, 142)
	detail_art.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	detail_art.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	detail_art.mouse_filter = Control.MOUSE_FILTER_IGNORE
	detail_art.visible = false
	detail_vbox.add_child(detail_art)

	detail_body = RichTextLabel.new()
	detail_body.bbcode_enabled = false
	detail_body.fit_content = true
	detail_body.scroll_active = true
	detail_body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	detail_body.add_theme_font_size_override("normal_font_size", 14)
	detail_body.add_theme_color_override("default_color", Color(0.7, 0.65, 0.6))
	detail_vbox.add_child(detail_body)

	# ── 하단 ──
	close_hint = Label.new()
	close_hint.text = _loc("[ESC] Close Journal", "[ESC] 일지 닫기")
	close_hint.add_theme_font_size_override("font_size", 13)
	close_hint.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	close_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	main_vbox.add_child(close_hint)

func _create_tab(text: String, tab_name: String) -> Button:
	var btn = Button.new()
	btn.text = text
	btn.custom_minimum_size = Vector2(120, 28)
	var style = UITheme.make_button_style()
	btn.add_theme_stylebox_override("normal", style)
	btn.add_theme_stylebox_override("hover", UITheme.make_hover_style(style))
	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
	btn.add_theme_color_override("font_hover_color", UITheme.TEXT_ACCENT)
	btn.pressed.connect(func():
		_current_tab = tab_name
		_refresh_list()
		AudioManager.play_sfx("ui_select")
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	return btn

## ===================== 목록 갱신 =====================

func _refresh_list() -> void:
	_update_tab_styles()

	for child in item_list.get_children():
		child.queue_free()

	detail_title.text = _loc("Select an entry...", "항목을 고르세요...")
	detail_body.text = ""
	if detail_art:
		detail_art.texture = null
		detail_art.visible = false
	_update_journal_summary()

	match _current_tab:
		"events":
			_populate_events()
		"npcs":
			_populate_npcs()
		"world":
			_populate_world()
		"choices":
			_populate_choices()
		"quests":
			_populate_quests()
		"losses":
			_populate_losses()
		"leads":
			_populate_leads()

func _update_journal_summary() -> void:
	if not journal_summary_label:
		return
	var burn_count := MemoryManager.get_burn_count() if MemoryManager else 0
	var held_count := MemoryManager.memories.size() - burn_count if MemoryManager else 0
	var loss_count := 0
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		loss_count = WorldRewriteDirector.get_loss_records().size()
	var unlocked_events := 0
	var illustrated_events := 0
	for entry in EVENT_ENTRIES:
		if not GameManager.get_flag(String(entry.get("flag", ""))):
			continue
		unlocked_events += 1
		var art_path := String(EVENT_ART_BY_FLAG.get(String(entry.get("flag", "")), entry.get("art", "")))
		if art_path != "" and ResourceLoader.exists(art_path):
			illustrated_events += 1
	var ch_name: String = GameManager.localized_chapter_name(GameManager.current_chapter,
		String(CHAPTER_NAMES.get(GameManager.current_chapter, "Unknown")))
	journal_summary_label.text = _loc("Ch.%d / %s    Held: %d    Burned: %d    Losses: %d    Illustrated: %d/%d",
		"%d장 / %s    보유 %d    연소 %d    상실 %d    삽화 %d/%d") % [
		GameManager.current_chapter, ch_name, held_count, burn_count, loss_count, illustrated_events, unlocked_events
	]

## S242: 한국어 로케일인데 저널만 통째로 영어로 남아 있었다.
func _loc(en: String, ko: String) -> String:
	return ko if GameManager.current_locale == "ko" else en

func _update_tab_styles() -> void:
	for tab_data in [{"btn": tab_events, "name": "events"}, {"btn": tab_npcs, "name": "npcs"}, {"btn": tab_world, "name": "world"}, {"btn": tab_choices, "name": "choices"}, {"btn": tab_quests_btn, "name": "quests"}, {"btn": tab_losses_btn, "name": "losses"}]:
		var btn: Button = tab_data.btn
		var active: bool = (_current_tab == tab_data.name)
		if active:
			btn.add_theme_color_override("font_color", UITheme.TEXT_ACCENT)
			var style = btn.get_theme_stylebox("normal").duplicate()
			style.border_color = UITheme.TEXT_ACCENT
			style.set_border_width_all(1)
			btn.add_theme_stylebox_override("normal", style)
		else:
			btn.add_theme_color_override("font_color", UITheme.TEXT_DIM)
			btn.add_theme_stylebox_override("normal", UITheme.make_button_style())

func _populate_events() -> void:
	var last_chapter := 0
	for entry in EVENT_ENTRIES:
		if not GameManager.get_flag(entry.flag):
			continue
		# 챕터 헤더
		if entry.chapter != last_chapter:
			last_chapter = entry.chapter
			var header = Label.new()
			header.text = _loc("Chapter %d: %s", "%d장 · %s") % [entry.chapter,
				GameManager.localized_chapter_name(entry.chapter, String(CHAPTER_NAMES.get(entry.chapter, "")))]
			header.add_theme_font_size_override("font_size", 13)
			header.add_theme_color_override("font_color", UITheme.TEXT_NARRATION)
			item_list.add_child(header)
		var is_hidden = String(entry.title).begins_with("[Hidden]")
		var color = Color(0.6, 0.5, 0.7) if is_hidden else Color(0.75, 0.7, 0.65)
		var art_path := String(EVENT_ART_BY_FLAG.get(String(entry.get("flag", "")), entry.get("art", "")))
		_add_list_button(_entry_title(entry), color, _entry_title(entry), _entry_desc(entry), art_path)

## S244: 항목 표는 영어 원문 옆에 `_ko`를 함께 들고 있다(VN의 text/text_ko,
## 장소 씬의 title_en/title_ko와 같은 관례). 한국어일 때만 그쪽을 쓰고, 없으면
## 영어로 되돌아간다. 그래서 번역이 빠진 항목이 있어도 화면이 비지 않는다.
func _field(entry: Dictionary, key: String) -> String:
	if GameManager.current_locale == "ko":
		var ko := String(entry.get(key + "_ko", ""))
		if ko != "":
			return ko
	return String(entry.get(key, ""))

func _npc_name(npc: Dictionary) -> String:
	if GameManager.current_locale == "ko":
		var ko := String(npc.get("name_ko", ""))
		if ko != "":
			return ko
	return GameManager.localized_speaker(String(npc.get("name", "")))

func _entry_title(entry: Dictionary) -> String:
	return _field(entry, "title")

func _entry_desc(entry: Dictionary) -> String:
	return _field(entry, "desc")

func _populate_npcs() -> void:
	for npc in NPC_ENTRIES:
		if not GameManager.get_flag(npc.flag):
			continue
		var speaker_color = UITheme.get_speaker_color(npc.name)
		var full_desc = "%s\n\n%s" % [_field(npc, "role"), _entry_desc(npc)]
		var npc_name := _npc_name(npc)
		_add_list_button(npc_name, speaker_color, npc_name, full_desc, String(npc.get("art", "")))

func _populate_world() -> void:
	var last_chapter := 0
	for entry in WORLD_ENTRIES:
		if not GameManager.get_flag(entry.flag):
			continue
		if entry.chapter != last_chapter:
			last_chapter = entry.chapter
			var header = Label.new()
			header.text = _loc("Learned in Chapter %d: %s", "%d장에서 알게 된 것 · %s") % [entry.chapter,
				GameManager.localized_chapter_name(entry.chapter, String(CHAPTER_NAMES.get(entry.chapter, "")))]
			header.add_theme_font_size_override("font_size", 13)
			header.add_theme_color_override("font_color", Color(0.55, 0.60, 0.78))
			item_list.add_child(header)
		_add_list_button(_entry_title(entry), Color(0.50, 0.57, 0.78), _entry_title(entry), _entry_desc(entry), String(entry.get("art", "")))

	if item_list.get_child_count() == 0:
		var empty = Label.new()
		empty.text = "The world is still becoming legible. Travel, listen, and keep a record."
		empty.autowrap_mode = TextServer.AUTOWRAP_WORD
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _populate_choices() -> void:
	# 선택 기록, 주요 분기점
	var choice_entries := [
		{"flag": "malet_deal_accepted", "title": "Accepted Malet's Deal", "title_ko": "말렛의 거래를 받아들이다", "art": "res://assets/cg/generated/archive_verdan_memory_price_v1.png", "desc": "Traded the memory of first holding a sword for information. The courtyard, the dust, the hand, gone.", "desc_ko": "처음 칼을 쥐던 기억을 정보와 맞바꿨다. 안뜰, 먼지, 그 손. 사라졌다."},
		{"flag": "malet_deal_refused", "title": "Refused Malet's Deal", "title_ko": "말렛의 거래를 거절하다", "desc": "Kept the sword memory. Left the Sump without Malet's help. (But returned later.)", "desc_ko": "칼의 기억을 지켰다. 말렛의 도움 없이 슘을 떠났다. (나중에 다시 돌아왔지만.)"},
		{"flag": "elia_separates", "title": "Sent Elia South", "title_ko": "엘리아를 남쪽으로 보내다", "art": "res://assets/cg/generated/archive_ch5_coastal_parting_v1.png", "desc": "Split up at the Crumbling Coast to confuse Kairos. Burned memories during separation left no residue.", "desc_ko": "카이로스를 흩뜨리려 무너지는 해안에서 갈라섰다. 떨어져 있는 동안 태운 기억은 잔존을 남기지 않았다."},
		{"flag": "elia_stays", "title": "Kept Elia Close", "title_ko": "엘리아를 곁에 두다", "desc": "Traveled the Coast together. The anchor stayed. Memories burned still left traces.", "desc_ko": "해안을 함께 지났다. 앵커는 남아 있었다. 태운 기억도 흔적을 남겼다."},
		{"flag": "zero_burn_path", "title": "Burned Your Name", "title_ko": "이름을 태우다", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "Zero Burn. The ultimate sacrifice. BL-07 closed, but the person called 'Arrel' ceased to exist.", "desc_ko": "무연소. 마지막까지 내어 준 것. BL-07은 닫혔지만 '아렐'이라 불리던 사람은 더 이상 존재하지 않는다."},
		{"flag": "seal_refused", "title": "Kept Your Name", "title_ko": "이름을 지키다", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "Refused the Seal. BL-07 remains, but so does the person who remembers.", "desc_ko": "봉인을 거절했다. BL-07은 남았지만, 기억하는 사람도 남았다."},
		{"flag": "seal_weave", "title": "Wove the Seal", "title_ko": "봉인을 엮다", "art": "res://assets/cg/generated/archive_ch10_burden_choice_v1.png", "desc": "The Weave. Sealed BL-07 by offering every preserved memory at once, name and self kept intact. The price was never being able to set them down.", "desc_ko": "엮음. 지켜 온 기억 전부를 한꺼번에 내밀어 BL-07을 봉인했다. 이름도 자신도 온전했다. 값은, 그것들을 결코 내려놓을 수 없게 된 것이었다."},
	]
	for entry in choice_entries:
		if not GameManager.get_flag(entry.flag):
			continue
		_add_list_button(_entry_title(entry), Color(0.7, 0.6, 0.45), _entry_title(entry), _entry_desc(entry), String(entry.get("art", "")))

	if item_list.get_child_count() == 0:
		var empty = Label.new()
		empty.text = "No major choices recorded yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _populate_quests() -> void:
	var quests = SideQuest.get_all_quests()
	var has_any = false
	for q in quests:
		var status = q["status"]
		if status == "locked":
			continue
		has_any = true
		var color: Color
		var prefix: String
		var desc_text: String
		match status:
			"complete":
				color = Color(0.4, 0.7, 0.4)
				prefix = "[DONE] "
				desc_text = SideQuest.loc(q, "desc")
			"active":
				color = Color(0.85, 0.7, 0.4)
				prefix = ""
				desc_text = q["desc"] + "\n\nCurrent: " + q["step_desc"]
			"available":
				color = Color(0.55, 0.5, 0.45)
				prefix = "[NEW] "
				desc_text = q["desc"] + "\n\nTalk to %s at %s." % [q["npc"], q["map"].replace("_", " ").capitalize()]
		# S217: 로케일 반영. 데이터에 한국어가 생겼으니 저널도 그것을 쓴다.
		var quest_title := SideQuest.loc(q, "title")
		_add_list_button(prefix + quest_title, color, quest_title, desc_text, String(q.get("art", "")))

	if not has_any:
		var empty = Label.new()
		empty.text = "No quests discovered yet."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

## ===================== S217: 미해결 단서 =====================
## 지역마다 아직 손대지 않은 발견물, 유물, 공명 지점이 몇 개인지 세어 준다.
## 위치를 콕 집어 주지는 않는다. "여기 아직 남았다"까지만 알려주는 것이
## 탐색을 대신해 주지 않으면서도 길을 잃지 않게 해 준다.
func _populate_leads() -> void:
	var is_ko := GameManager.current_locale == "ko"
	var map_ids: Array = []
	for map_id: String in WorldPopulation.POPULATIONS:
		map_ids.append(map_id)
	map_ids.sort()

	var total_open := 0
	for map_id: String in map_ids:
		var open_lines: Array[String] = []
		var population: Dictionary = WorldPopulation.POPULATIONS.get(map_id, {})

		var caches_left := 0
		for cache: Dictionary in population.get("caches", []):
			if not GameManager.get_flag("world_cache_%s_%s" % [map_id, String(cache.get("id", ""))]):
				caches_left += 1
		if caches_left > 0:
			open_lines.append(("· 발견물 %d" if is_ko else "· %d cache(s)") % caches_left)

		var curios_left := 0
		for curio: Dictionary in _curios_for(map_id):
			if not GameManager.get_flag("world_curio_%s_%s" % [map_id, String(curio.get("id", ""))]):
				curios_left += 1
		if curios_left > 0:
			open_lines.append(("· 지역 유물 %d" if is_ko else "· %d relic(s)") % curios_left)

		var resonance_left := 0
		for point: Dictionary in MemoryResonance.RESONANCE_POINTS.get(map_id, []):
			if not GameManager.get_flag(String(point.get("flag", ""))):
				resonance_left += 1
		if resonance_left > 0:
			open_lines.append(("· 기억 공명 %d" if is_ko else "· %d resonance") % resonance_left)

		if open_lines.is_empty():
			continue
		total_open += open_lines.size()
		var region := String(ExplorationHUD.MAP_NAMES_KO.get(map_id, map_id)) if is_ko else String(ExplorationHUD.MAP_NAMES.get(map_id, map_id))
		_add_list_button(
			region,
			Color(0.62, 0.74, 0.88),
			region,
			("아직 손대지 않은 것들:

%s" if is_ko else "Still untouched here:

%s") % "
".join(open_lines)
		)

	if total_open == 0:
		var empty := Label.new()
		empty.text = "남은 단서가 없습니다." if is_ko else "No open leads."
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

## 해당 맵의 지역 유물 목록. CURIOS_BY_MAP은 맵당 하나를 담는다.
func _curios_for(map_id: String) -> Array:
	var entry: Variant = WorldPopulation.CURIOS_BY_MAP.get(map_id, null)
	if entry == null:
		return []
	if entry is Array:
		return entry
	return [entry]

func _populate_losses() -> void:
	var records: Array[Dictionary] = []
	if WorldRewriteDirector and WorldRewriteDirector.has_method("get_loss_records"):
		records = WorldRewriteDirector.get_loss_records()

	for record in records:
		var color: Color = record.get("color", Color(0.75, 0.58, 0.42))
		_add_list_button(String(record.get("title", "Uncatalogued Loss")), color, String(record.get("title", "Uncatalogued Loss")), String(record.get("body", "")), String(record.get("art", "")))

	if records.is_empty():
		var empty = Label.new()
		empty.text = _loc("No irreversible losses recorded yet.", "아직 되돌릴 수 없는 상실이 기록되지 않았습니다.")
		empty.add_theme_font_size_override("font_size", 13)
		empty.add_theme_color_override("font_color", UITheme.TEXT_DIM)
		item_list.add_child(empty)

func _add_list_button(text: String, color: Color, title: String, desc: String, art_path: String = "") -> void:
	var btn = Button.new()
	btn.text = text
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.custom_minimum_size = Vector2(0, 32)

	var style = StyleBoxFlat.new()
	style.bg_color = Color(color.r * 0.2, color.g * 0.2, color.b * 0.2, 0.4)
	style.border_width_left = 3
	style.border_color = Color(color.r * 0.4, color.g * 0.4, color.b * 0.4, 0.3)
	style.set_content_margin_all(6)
	style.set_corner_radius_all(2)
	btn.add_theme_stylebox_override("normal", style)

	var hover = style.duplicate()
	hover.bg_color = Color(color.r * 0.3, color.g * 0.3, color.b * 0.3, 0.6)
	hover.border_color = color
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("focus", hover)

	btn.add_theme_font_size_override("font_size", 13)
	btn.add_theme_color_override("font_color", Color(0.72, 0.68, 0.62))
	btn.add_theme_color_override("font_hover_color", color.lightened(0.3))

	btn.pressed.connect(func():
		detail_title.text = title
		detail_body.text = desc
		if detail_art:
			if art_path != "" and ResourceLoader.exists(art_path):
				detail_art.texture = load(art_path)
				detail_art.visible = true
			else:
				detail_art.texture = null
				detail_art.visible = false
	)
	btn.mouse_entered.connect(func(): AudioManager.play_sfx("ui_hover"))
	item_list.add_child(btn)

func _show_ui() -> void:
	if backdrop:
		backdrop.visible = true
	overlay.visible = true
	main_panel.visible = true

func _hide_ui() -> void:
	if backdrop:
		backdrop.visible = false
	overlay.visible = false
	main_panel.visible = false
