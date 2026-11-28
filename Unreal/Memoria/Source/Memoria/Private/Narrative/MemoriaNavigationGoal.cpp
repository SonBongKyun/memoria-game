#include "Narrative/MemoriaNavigationGoal.h"
#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaSumpLedger.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Presentation/MemoriaDemoCompletion.h"
#include "Run/MemoriaRunTypes.h"

namespace MemoriaNavigationGoal
{
FString Line(const FMemoriaRunSnapshot& Run, const FString& Map, bool bVerdanRevisit)
{
    if (!Run.RunId.IsValid()) return FString();
    const bool bKo = Run.CurrentLocale == TEXT("ko");
    // Keep the established side-quest priority and complete line exactly.
    if (MemoriaSumpLedger::IsActive(Run))
        return (bKo ? TEXT("퀘스트  ") : TEXT("QUEST  ")) + MemoriaSumpLedger::Title(bKo) + TEXT("  -  ") + MemoriaSumpLedger::CurrentStepText(Run, bKo);
    const FString Prefix = bKo ? TEXT("목표  ") : TEXT("GOAL  ");
    if (Map == TEXT("verdan_market"))
    {
        if (Run.CurrentChapter < 2 || !Run.GetFlag(TEXT("ch2_arrived"))) return FString();
        // A revisit has no field exit in the current port. The Belt road is an action
        // on the closed exchange/checkpoint screen, never an invented courtyard marker.
        if (bVerdanRevisit && Run.GetFlag(TEXT("ch2_complete")))
            return Prefix + (bKo ? TEXT("베르단 시장 살펴보기") : TEXT("Explore Verdan Market"));
        if (Run.GetFlag(TEXT("ch2_complete")) || Run.GetFlag(TEXT("ch2_malet_done"))) return FString();
        // These are already chosen responses. Refusal is cleared by its end handler,
        // after which the ordinary conversation can be started again.
        if (Run.GetFlag(TEXT("malet_deal_accepted")))
            return Prefix + (bKo ? TEXT("말렛과 거래 마무리하기") : TEXT("Finish the exchange with Malet"));
        if (Run.GetFlag(TEXT("malet_deal_refused")))
            return Prefix + (bKo ? TEXT("말렛의 답변 듣기") : TEXT("Hear Malet's response"));
        return Prefix + (bKo ? TEXT("시장의 말렛과 이야기하기") : TEXT("Speak with Malet in the market"));
    }
    const auto* Spec = MemoriaChapterMaps::Find(Map);
    if (!Spec) return FString();
    // S371: after Chapter 5 the east exit is the road to the Seam.
    if (Map == TEXT("drift_shelter") && !MemoriaManuscript::RoadOnward(Map, Run).IsEmpty() && MemoriaChapterMaps::Find(MemoriaManuscript::RoadOnward(Map, Run)))
        return Prefix + (bKo ? TEXT("동쪽 출구 → 더 심") : TEXT("East exit → The Seam"));
    if (Map == MemoriaManuscript::SeamMap && !Run.GetFlag(TEXT("ch6_complete")))
    {
        if (Run.GetFlag(TEXT("ch6_sable_morning"))) return Prefix + (bKo ? TEXT("북동쪽 능선길 → 하렌을 따라") : TEXT("North-east ridge road → follow Haren"));
        if (Run.GetFlag(TEXT("ch6_sable_night"))) return Prefix + (bKo ? TEXT("아침: 개울 북쪽 집에서 세이블에게 길을 묻는다") : TEXT("Morning: ask Sable the way, north of the brook"));
        if (Run.GetFlag(TEXT("ch6_feren"))) return Prefix + (bKo ? TEXT("마을 가운데 집의 안쪽 방에서 쉰다") : TEXT("Rest in the inner room, in the middle of the village"));
    }
    if (Map == MemoriaManuscript::CoastMap && !Run.GetFlag(TEXT("ch8_complete")))
    {
        if (Run.GetFlag(TEXT("ch8_tree"))) return Prefix + (bKo ? TEXT("북쪽 출구 → 갈림길") : TEXT("North exit → the crossroads"));
        if (Run.GetFlag(TEXT("ch8_city"))) return Prefix + (bKo ? TEXT("북쪽 길로") : TEXT("Take the north path"));
        if (Run.GetFlag(TEXT("ch8_hole"))) return Prefix + (bKo ? TEXT("구멍 안으로: 같이") : TEXT("Into the hole: together"));
        if (Run.GetFlag(TEXT("ch8_shore"))) return Prefix + (bKo ? TEXT("자갈밭 가운데의 구멍으로") : TEXT("To the hole in the middle of the gravel"));
    }
    if (Map == MemoriaManuscript::EastRoadMap && !Run.GetFlag(TEXT("ch18_complete")))
    {
        if (Run.GetFlag(TEXT("ch18_yard"))) return Prefix + (bKo ? TEXT("마당 동쪽으로: 같은 수레") : TEXT("East out of the yard: the same cart"));
        if (Run.GetFlag(TEXT("ch18_post"))) return Prefix + (bKo ? TEXT("비탈 위로: 여덟째 마일 마당") : TEXT("Up the slope: the eighth-mile yard"));
        if (Run.GetFlag(TEXT("ch18_panel"))) return Prefix + (bKo ? TEXT("일곱째 마일 초소") : TEXT("The seventh-mile post"));
        if (Run.GetFlag(TEXT("ch18_rest"))) return Prefix + (bKo ? TEXT("굽이를 지나 동쪽으로") : TEXT("On east past the bend"));
        if (Run.GetFlag(TEXT("ch18_third_mile"))) return Prefix + (bKo ? TEXT("넷째 마일 굽이: 나무 밑") : TEXT("The fourth-mile bend: under the trees"));
        if (Run.GetFlag(TEXT("ch18_cart"))) return Prefix + (bKo ? TEXT("셋째 마일로") : TEXT("On to the third mile"));
        if (Run.GetFlag(TEXT("ch18_first_mile"))) return Prefix + (bKo ? TEXT("둘째 마일: 마차") : TEXT("The second mile: the cart"));
        if (Run.GetFlag(TEXT("ch18_report"))) return Prefix + (bKo ? TEXT("동쪽으로: 첫째 마일 돌") : TEXT("East: the first mile stone"));
    }
    if (Map == MemoriaManuscript::ArcaneLeaveMap && !Run.GetFlag(TEXT("ch17_complete")))
    {
        if (Run.GetFlag(TEXT("ch17_stone"))) return Prefix + (bKo ? TEXT("남문 아래로: 성문 서기") : TEXT("Under the south gate: the gate clerk"));
        if (Run.GetFlag(TEXT("ch17_cistern"))) return Prefix + (bKo ? TEXT("거리를 지나 남문 안쪽의 낮은 돌로") : TEXT("Down the streets to the low stone inside the south gate"));
        if (Run.GetFlag(TEXT("ch17_door"))) return Prefix + (bKo ? TEXT("한나를 따라 저수조 쪽으로") : TEXT("Follow Hanna toward the cistern"));
    }
    if (Map == MemoriaManuscript::ArcaneMap && !Run.GetFlag(TEXT("ch16_complete")))
    {
        if (Run.GetFlag(TEXT("ch16_cistern"))) return Prefix + (bKo ? TEXT("낮은 아치 골목 끝: 놋쇠 고리 문") : TEXT("The low-arched alley's end: the door with the brass ring"));
        if (Run.GetFlag(TEXT("ch16_alley"))) return Prefix + (bKo ? TEXT("저수조의 돌벽으로") : TEXT("To the cistern's stone wall"));
        if (Run.GetFlag(TEXT("ch16_burn"))) return Prefix + (bKo ? TEXT("북서쪽 골목으로") : TEXT("Into the alley north-west"));
        if (Run.GetFlag(TEXT("ch16_booth"))) return Prefix + (bKo ? TEXT("셋째 부스 옆 낮은 벤치") : TEXT("The low bench by the third booth"));
        if (Run.GetFlag(TEXT("ch16_squares"))) return Prefix + (bKo ? TEXT("둘째 광장: 셋째 부스") : TEXT("The second square: the third booth"));
        if (Run.GetFlag(TEXT("ch16_gate"))) return Prefix + (bKo ? TEXT("첫 광장: 빵 창구") : TEXT("The first square: the bread window"));
    }
    if (Map == MemoriaManuscript::DryWellsMap && !Run.GetFlag(TEXT("ch15_complete")))
    {
        if (Run.GetFlag(TEXT("ch15_blood"))) return Prefix + (bKo ? TEXT("북쪽 갈림 → 능선 너머") : TEXT("North to the fork → over the ridge"));
        if (Run.GetFlag(TEXT("ch15_angles"))) return Prefix + (bKo ? TEXT("엘리아 곁으로") : TEXT("To Elia's side"));
        if (Run.GetFlag(TEXT("ch15_six"))) return Prefix + (bKo ? TEXT("우물 옆 그늘: 엘리아의 각도") : TEXT("The shade by the well: Elia's angles"));
        if (Run.GetFlag(TEXT("ch15_dry"))) return Prefix + (bKo ? TEXT("토비아스의 돌: 새 글씨") : TEXT("Tobias's stone: new writing"));
        if (Run.GetFlag(TEXT("ch15_pasture"))) return Prefix + (bKo ? TEXT("움푹한 땅: 우물 둘") : TEXT("The hollow: two wells"));
        if (Run.GetFlag(TEXT("ch15_bread"))) return Prefix + (bKo ? TEXT("목초지 아래로: 물을 찾아") : TEXT("Down through the pasture: water"));
    }
    if (Map == MemoriaManuscript::RidgeMap && !Run.GetFlag(TEXT("ch14_complete")))
    {
        if (Run.GetFlag(TEXT("ch14_ten_years"))) return Prefix + (bKo ? TEXT("벽 안쪽으로: 동이 튼다") : TEXT("Inside the walls: first light"));
        if (Run.GetFlag(TEXT("ch14_slope"))) return Prefix + (bKo ? TEXT("길 가장자리: 그가 앉은 돌") : TEXT("The edge of the road: the stone he sits on"));
        if (Run.GetFlag(TEXT("ch14_night"))) return Prefix + (bKo ? TEXT("무너진 모서리: 내려오는 길을 지킨다") : TEXT("The broken corner: watch the road down"));
        if (Run.GetFlag(TEXT("ch14_house"))) return Prefix + (bKo ? TEXT("높은 벽 아래: 불 없는 밤") : TEXT("Under the high wall: a night with no fire"));
        if (Run.GetFlag(TEXT("ch14_stumps"))) return Prefix + (bKo ? TEXT("능선 아래 농가: 열린 쪽 앞의 길") : TEXT("The farmhouse under the ridge: the road at its open side"));
        if (Run.GetFlag(TEXT("ch14_tower"))) return Prefix + (bKo ? TEXT("북쪽 그루터기 사이로") : TEXT("North among the stumps"));
        if (Run.GetFlag(TEXT("ch14_fields"))) return Prefix + (bKo ? TEXT("서쪽: 무너진 돌담 너머") : TEXT("West: over the broken stone wall"));
        if (Run.GetFlag(TEXT("ch14_morning"))) return Prefix + (bKo ? TEXT("밭 가장자리 길로") : TEXT("Along the edge of the fields"));
    }
    if (Map == MemoriaManuscript::VerdanReturnMap && !Run.GetFlag(TEXT("ch13_complete")))
    {
        if (Run.GetFlag(TEXT("ch13_alley"))) return Prefix + (bKo ? TEXT("동쪽 출구 → 묵은 밭") : TEXT("East exit → the old fields"));
        if (Run.GetFlag(TEXT("ch13_vendor"))) return Prefix + (bKo ? TEXT("모퉁이 안쪽 골목으로") : TEXT("Into the alley past the corner"));
        if (Run.GetFlag(TEXT("ch13_sump"))) return Prefix + (bKo ? TEXT("동쪽 모퉁이의 기름 노점") : TEXT("The oil stall on the east corner"));
        if (Run.GetFlag(TEXT("ch13_market"))) return Prefix + (bKo ? TEXT("셈터로: 광장 남쪽의 돌계단") : TEXT("To the Sump: the stone steps south of the square"));
        if (Run.GetFlag(TEXT("ch13_farewell"))) return Prefix + (bKo ? TEXT("시장으로: 공동 저수지") : TEXT("Into the market: the public reservoir"));
    }
    if (Map == MemoriaManuscript::BasinMap && !Run.GetFlag(TEXT("ch12_complete")))
    {
        if (Run.GetFlag(TEXT("ch12_walk"))) return Prefix + (bKo ? TEXT("동쪽 출구 → 마른 시내 위의 양 우리") : TEXT("East exit → the sheep pen over the dry creek"));
        if (Run.GetFlag(TEXT("ch12_queue"))) return Prefix + (bKo ? TEXT("분지를 돌아 동쪽으로: 창구에서 육십 걸음") : TEXT("Round the basin to the east, sixty paces from the window"));
        if (Run.GetFlag(TEXT("ch12_tower"))) return Prefix + (bKo ? TEXT("서쪽 높은 땅에서 창구를 본다") : TEXT("From the higher ground on the west, look at the window"));
        if (Run.GetFlag(TEXT("ch12_road"))) return Prefix + (bKo ? TEXT("남쪽 길로: 표주를 지나") : TEXT("South road, past the post"));
    }
    if (Map == MemoriaManuscript::FarmsMap && !Run.GetFlag(TEXT("ch11_complete")))
    {
        if (Run.GetFlag(TEXT("ch11_glove"))) return Prefix + (bKo ? TEXT("북쪽 출구 → 마른 수로, 두 번째 농가") : TEXT("North exit → the dry channel, the second farmhouse"));
        if (Run.GetFlag(TEXT("ch11_fel"))) return Prefix + (bKo ? TEXT("북쪽 길로: 능선을 돌아 내려가는 곳") : TEXT("North along the road, where it comes down off the ridge"));
        if (Run.GetFlag(TEXT("ch11_night"))) return Prefix + (bKo ? TEXT("아침: 농가 문턱으로") : TEXT("Morning: to the farmhouse threshold"));
        if (Run.GetFlag(TEXT("ch11_house"))) return Prefix + (bKo ? TEXT("밤: 같은 벽을 따라") : TEXT("Night: along the same wall"));
        if (Run.GetFlag(TEXT("ch11_road"))) return Prefix + (bKo ? TEXT("둔덕 뒤, 지붕 없는 농가 안으로") : TEXT("Into the roofless farmhouse behind the mound"));
    }
    if (Map == MemoriaManuscript::ReturnMap && !Run.GetFlag(TEXT("ch10_complete")))
    {
        if (Run.GetFlag(TEXT("ch10_square"))) return Prefix + (bKo ? TEXT("서쪽 출구 → 벨트 도로 쪽") : TEXT("West exit → toward the Belt road"));
        if (Run.GetFlag(TEXT("ch10_horn"))) return Prefix + (bKo ? TEXT("광장 서쪽 끝으로") : TEXT("To the west end of the square"));
        if (Run.GetFlag(TEXT("ch10_step"))) return Prefix + (bKo ? TEXT("동쪽 길 쪽으로") : TEXT("Toward the east road"));
        if (Run.GetFlag(TEXT("ch10_bridge"))) return Prefix + (bKo ? TEXT("마을 가운데, 돌턱의 세이블에게") : TEXT("To Sable on the stone step, in the middle of the village"));
    }
    if (Map == MemoriaManuscript::ClearingMap && !Run.GetFlag(TEXT("ch9_complete")))
    {
        if (Run.GetFlag(TEXT("ch9_rock"))) return Prefix + (bKo ? TEXT("북쪽 출구 → 부서진 암반 너머 마른 수로") : TEXT("North exit → the dry channel past the broken bedrock"));
        if (Run.GetFlag(TEXT("ch9_rings"))) return Prefix + (bKo ? TEXT("길 건너편 바위의 물로: 북쪽 길") : TEXT("To the water in the rock across the path: north"));
        if (Run.GetFlag(TEXT("ch9_haren"))) return Prefix + (bKo ? TEXT("쓰러진 나무의 잘린 면을 본다") : TEXT("Look at the felled tree's cut face"));
        if (Run.GetFlag(TEXT("ch9_trees"))) return Prefix + (bKo ? TEXT("도끼 소리가 나는 공터로") : TEXT("To the clearing where the axe sounds"));
    }
    if (Map == MemoriaManuscript::ForestMap && !Run.GetFlag(TEXT("ch7_complete")))
    {
        if (Run.GetFlag(TEXT("ch7_escape"))) return Prefix + (bKo ? TEXT("남쪽 출구 → 두 돌 능선 사이") : TEXT("South exit → between the two stone ridges"));
        if (Run.GetFlag(TEXT("ch7_lost_hour"))) return Prefix + (bKo ? TEXT("숲을 빠져나간다: 남쪽으로") : TEXT("Get out of the forest: south"));
        if (Run.GetFlag(TEXT("ch7_forest_enter"))) return Prefix + (bKo ? TEXT("곧은 길을 따라 숲 안쪽으로. 소리 쪽으로 고개를 돌리지 말 것") : TEXT("Follow the straight road into the forest. Don't turn toward the sounds"));
    }
    if (Map == TEXT("drift_shelter") && MemoriaDemoCompletion::IsComplete(Run))
        return Prefix + (bKo ? TEXT("일시정지 → 체험판 구간 완료 · 기록") : TEXT("Pause → Segment complete: review"));
    const bool bExitReady = (!Spec->Exit.Requires.IsEmpty() && Run.GetFlag(Spec->Exit.Requires))
        || (!Spec->Exit.Completes.IsEmpty() && Run.GetFlag(Spec->Exit.Completes));
    if (bExitReady)
    {
        // Both authored exits lie on the east edge. Confirm that from their actual
        // rectangles before offering a direction, so a changed map never gets a false arrow.
        const bool bEast = Spec->Exit.Rect.Center().X > Spec->Width * Spec->TileSize * .75;
        if (const auto* Destination = MemoriaChapterMaps::Find(Spec->Exit.NextMap))
        {
            const FString Place = bKo ? MemoriaChapterMaps::Korean(Destination->Map)
                : Destination->Map == TEXT("drift_shelter") ? TEXT("Drift Shelter")
                : Destination->Map == TEXT("belt_waystation") ? TEXT("Belt Waystation") : Destination->TitleName;
            const FString Direction = bKo ? (bEast ? TEXT("동쪽 출구 → ") : TEXT("출구 → "))
                : (bEast ? TEXT("East exit → ") : TEXT("Exit → "));
            return Prefix + Direction + Place;
        }
        // Drift's next destination is an existing story scene, not a ported region.
        // Do not reveal its title or promise access to the unported Seam.
        if (!Spec->Exit.NextScene.IsEmpty())
        {
            if (Run.GetFlag(Spec->Exit.Completes))
                return Prefix + (bKo ? TEXT("잠시 후 다음 이야기가 이어집니다") : TEXT("The next scene will begin shortly"));
            return Prefix + (bKo ? (bEast ? TEXT("동쪽 출구 → 여정 이어가기") : TEXT("출구 → 여정 이어가기"))
                : (bEast ? TEXT("East exit → continue the journey") : TEXT("Exit → continue the journey")));
        }
    }
    if (Map == TEXT("belt_waystation"))
        return Prefix + (bKo ? TEXT("벨트 중간역 살펴보기") : TEXT("Explore the Belt Waystation"));
    if (Map == TEXT("drift_shelter"))
        return Prefix + (bKo ? TEXT("드리프트 셸터 살펴보기") : TEXT("Explore Drift Shelter"));
    return FString();
}
}
