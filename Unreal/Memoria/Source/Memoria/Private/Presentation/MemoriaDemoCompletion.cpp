#include "Presentation/MemoriaDemoCompletion.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Journal/MemoriaJournal.h"

bool MemoriaDemoCompletion::IsComplete(const FMemoriaRunSnapshot& Run)
{
    return Run.RunId.IsValid() && Run.CurrentChapter == 5 && Run.GetFlag(TEXT("ch5_classifier_started"))
        && Run.GetFlag(TEXT("canon_ch6_seam_ready"));
}
bool MemoriaDemoCompletion::CanPresent(const FMemoriaRunSnapshot& Run, const FString& Map, bool bExploring, bool bCompletionCardShowing)
{ return IsComplete(Run) && Map == TEXT("drift_shelter") && bExploring && !bCompletionCardShowing; }
FMemoriaDemoCompletionView MemoriaDemoCompletion::Build(const UMemoriaRunSubsystem& Run, bool bKo)
{
    FMemoriaDemoCompletionView View;
    View.Title = bKo ? TEXT("체험판 구간 완료") : TEXT("Demo segment complete");
    const auto State = Run.GetRunSnapshot();
    if (!IsComplete(State) || !Run.GetPlayerMemory()) return View;
    const auto Memory = Run.GetPlayerMemory()->GetSnapshot();
    for (const auto& M : Memory.Owned) View.Held += !M.bBurned && !M.bFaded ? 1 : 0;
    View.Burned = Memory.BurnedHistory.Num();
    View.Summary = bKo
        ? FString::Printf(TEXT("5장 · 분류하는 자\n\n무너진 돌벽의 동쪽 출구가 6장 「더 심」으로 이어집니다.\n기억과 기록을 돌아본 뒤 출발할 수 있습니다.\n\n온전한 기억 %d개 · 태운 기억 %d개"), View.Held, View.Burned)
        : FString::Printf(TEXT("Chapter 5 · The Classifier\n\nThe east exit of Drift Shelter now goes on to Chapter 6, The Seam.\nReview your memories and records, then set out.\n\n%d intact memories · %d burned memories"), View.Held, View.Burned);
    // Show only the classifier outcome already revealed by the completed Chapter 5 scene.
    // These existing flags are mutually exclusive narrative outputs, not new story authority.
    FString Report;
    if (State.GetFlag(TEXT("ch5_malet_report_identified_arrel")))
        Report = bKo ? TEXT("분류자 보고: 아렐 식별") : TEXT("Classifier report: Arrel identified");
    else if (State.GetFlag(TEXT("ch5_malet_report_requester_unknown")))
        Report = bKo ? TEXT("분류자 보고: 요청자 불명") : TEXT("Classifier report: requester unknown");
    if (!Report.IsEmpty()) View.Summary += TEXT("\n\n") + Report;
    View.Review = (Report.IsEmpty() ? FString() : Report + TEXT("\n\n"))
        + (bKo ? TEXT("남겨진 선택\n") : TEXT("Recorded choices\n"));
    int32 Choices = 0;
    for (const auto& Entry : MemoriaJournal::Choices())
        if ((Entry.Flag == TEXT("malet_deal_accepted") || Entry.Flag == TEXT("malet_deal_refused")) && State.GetFlag(Entry.Flag))
        {
            View.Review += TEXT("\n") + Entry.TitleIn(bKo) + TEXT("\n") + Entry.DescIn(bKo) + TEXT("\n");
            ++Choices;
        }
    if (!Choices) View.Review += bKo ? TEXT("\n기록된 선택이 없습니다.\n") : TEXT("\nNo recorded choices.\n");
    View.Review += bKo ? TEXT("\n잃어버린 기억의 기록\n") : TEXT("\nRecords of loss\n");
    const auto Losses = MemoriaJournal::LossRecords(Run, bKo);
    for (const auto& Loss : Losses) View.Review += TEXT("\n") + Loss.Title;
    if (Losses.IsEmpty()) View.Review += bKo ? TEXT("\n기록된 소실이 없습니다.") : TEXT("\nNo recorded losses.");
    // The existing loss Body contains implementation flag IDs. Show its authored title here;
    // the ordinary journal remains available from Pause for its complete presentation.
    return View;
}
