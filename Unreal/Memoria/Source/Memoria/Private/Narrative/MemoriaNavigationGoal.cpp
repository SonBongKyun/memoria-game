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
