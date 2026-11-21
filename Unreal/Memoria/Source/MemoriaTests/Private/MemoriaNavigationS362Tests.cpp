#include "Misc/AutomationTest.h"
#include "Narrative/MemoriaNavigationGoal.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaSumpLedger.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
void Flag(FMemoriaRunSnapshot& Run, const FString& Id)
{ FMemoriaStoryFlag Value; Value.Id = Id; Value.bValue = true; Run.StoryFlags.Add(Value); }
FMemoriaRunSnapshot At(int64 Chapter)
{ FMemoriaRunSnapshot Run; Run.RunId = FGuid::NewGuid(); Run.CurrentChapter = Chapter; return Run; }
TArray<uint8> Persisted(const UMemoriaRunSubsystem& Run)
{ TArray<uint8> Bytes; if (auto* Save = Run.CaptureSave()) UGameplayStatics::SaveGameToMemory(Save, Bytes); return Bytes; }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNavigationGoalS362, "Memoria.S362Navigation.RevealedCurrentGoals", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNavigationGoalS362::RunTest(const FString&)
{
    using MemoriaNavigationGoal::Line;
    const auto* Belt = MemoriaChapterMaps::Find(TEXT("belt_waystation"));
    const auto* Drift = MemoriaChapterMaps::Find(TEXT("drift_shelter"));
    if (!TestNotNull(TEXT("Use the actual authored Belt map"), Belt) || !TestNotNull(TEXT("Use the actual authored Drift map"), Drift)) return false;
    TestEqual(TEXT("Belt exit uses its existing source prerequisite"), Belt->Exit.Requires, FString(TEXT("ch3_class_seven_message")));
    TestEqual(TEXT("Belt destination is the existing playable Drift map"), Belt->Exit.NextMap, FString(TEXT("drift_shelter")));
    TestEqual(TEXT("Drift exit uses its existing source prerequisite"), Drift->Exit.Requires, FString(TEXT("ch4_night_watch")));
    TestTrue(TEXT("Drift has a story interlude, no invented region destination"), Drift->Exit.NextMap.IsEmpty() && !Drift->Exit.NextScene.IsEmpty());
    for (bool bKo : {false, true})
    {
        auto Run = At(3); Run.CurrentLocale = bKo ? TEXT("ko") : TEXT("en");
        TestEqual(TEXT("An incomplete arrival chain only names the current place"), Line(Run, Belt->Map), bKo ? FString(TEXT("목표  벨트 중간역 살펴보기")) : FString(TEXT("GOAL  Explore the Belt Waystation")));
        Flag(Run, Belt->Exit.Requires.ToUpper());
        TestFalse(TEXT("Case-changed source flags cannot reveal the exit"), Line(Run, Belt->Map).Contains(TEXT("→")));
        Flag(Run, Belt->Exit.Requires);
        TestEqual(TEXT("The revealed source exit names the actual destination"), Line(Run, Belt->Map), bKo ? FString(TEXT("목표  동쪽 출구 → 무너진 돌벽")) : FString(TEXT("GOAL  East exit → Drift Shelter")));
        Run = At(4); Run.CurrentLocale = bKo ? TEXT("ko") : TEXT("en");
        TestEqual(TEXT("Drift starts with current-place exploration"), Line(Run, Drift->Map), bKo ? FString(TEXT("목표  드리프트 셸터 살펴보기")) : FString(TEXT("GOAL  Explore Drift Shelter")));
        Flag(Run, Drift->Exit.Requires);
        const FString Departure = Line(Run, Drift->Map);
        TestEqual(TEXT("Drift's revealed exit continues without a future story spoiler"), Departure, bKo ? FString(TEXT("목표  동쪽 출구 → 여정 이어가기")) : FString(TEXT("GOAL  East exit → continue the journey")));
        TestFalse(TEXT("No unported destination is promised"), Departure.Contains(TEXT("Seam")) || Departure.Contains(TEXT("Classifier")) || Departure.Contains(TEXT("분류자")));
        Flag(Run, Drift->Exit.Completes);
        TestEqual(TEXT("A committed departure waits for the actual scheduled scene"), Line(Run, Drift->Map), bKo ? FString(TEXT("목표  잠시 후 다음 이야기가 이어집니다")) : FString(TEXT("GOAL  The next scene will begin shortly")));
        Run.CurrentChapter = 5; Flag(Run, TEXT("ch5_classifier_started")); Flag(Run, TEXT("canon_ch6_seam_ready"));
        // S371: the Seam is ported, so the completed segment points down its road.
        TestEqual(TEXT("The completed segment points to the Seam"), Line(Run, Drift->Map), bKo ? FString(TEXT("목표  동쪽 출구 → 더 심")) : FString(TEXT("GOAL  East exit → The Seam")));
        Run = At(2); Run.CurrentLocale = bKo ? TEXT("ko") : TEXT("en");
        TestTrue(TEXT("Malet is not suggested before actual market arrival"), Line(Run, TEXT("verdan_market")).IsEmpty());
        Flag(Run, TEXT("ch2_arrived"));
        TestEqual(TEXT("The current market goal names the actual conversation"), Line(Run, TEXT("verdan_market")), bKo ? FString(TEXT("목표  시장의 말렛과 이야기하기")) : FString(TEXT("GOAL  Speak with Malet in the market")));
        Flag(Run, TEXT("malet_deal_refused"));
        TestEqual(TEXT("Refusal waits for its existing reply"), Line(Run, TEXT("verdan_market")), bKo ? FString(TEXT("목표  말렛의 답변 듣기")) : FString(TEXT("GOAL  Hear Malet's response")));
        Run.StoryFlags.Pop();
        TestEqual(TEXT("The refusal end handler's cleared flag returns to conversation"), Line(Run, TEXT("verdan_market")), bKo ? FString(TEXT("목표  시장의 말렛과 이야기하기")) : FString(TEXT("GOAL  Speak with Malet in the market")));
        Flag(Run, TEXT("malet_deal_accepted"));
        TestEqual(TEXT("Acceptance names the current exchange, no guaranteed reward"), Line(Run, TEXT("verdan_market")), bKo ? FString(TEXT("목표  말렛과 거래 마무리하기")) : FString(TEXT("GOAL  Finish the exchange with Malet")));
        Flag(Run, TEXT("ch2_complete"));
        TestTrue(TEXT("A closed exchange is owned by its actual checkpoint screen"), Line(Run, TEXT("verdan_market")).IsEmpty());
        TestEqual(TEXT("A real revisit names exploration without inventing a field exit"), Line(Run, TEXT("verdan_market"), true), bKo ? FString(TEXT("목표  베르단 시장 살펴보기")) : FString(TEXT("GOAL  Explore Verdan Market")));
        TestTrue(TEXT("Unknown regions never inherit a chapter-number destination"), Line(Run, TEXT("forgotten_forest")).IsEmpty());
        Flag(Run, MemoriaSumpLedger::Steps()[0].Flag);
        const FString Ledger = (bKo ? TEXT("퀘스트  ") : TEXT("QUEST  ")) + MemoriaSumpLedger::Title(bKo) + TEXT("  -  ") + MemoriaSumpLedger::CurrentStepText(Run, bKo);
        TestEqual(TEXT("Active ledger keeps exact priority, prefix and authored step"), Line(Run, Drift->Map), Ledger);
        for (const FString& Text : {Departure, Ledger, Line(Run, Belt->Map)})
            TestFalse(TEXT("Player guidance never exposes raw source flags"), Text.Contains(TEXT("ch3_")) || Text.Contains(TEXT("canon_")) || Text.Contains(TEXT("sq_sump_")) || Text.Contains(TEXT("malet_deal_")));
    }
    TestTrue(TEXT("No active run has no guidance"), Line(FMemoriaRunSnapshot(), TEXT("drift_shelter")).IsEmpty());
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNavigationReadOnlyS362, "Memoria.S362Navigation.TrackerReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNavigationReadOnlyS362::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
    TestEqual(TEXT("Use the real starting run authority"), Run->BeginStartingMemoryRun(3), EMemoriaMemoryResult::Success);
    TestTrue(TEXT("An idle host never infers a field from chapter alone"), Host->GetQuestTrackerLine().IsEmpty());
    TestTrue(TEXT("Select the real current chapter map without a renderer"), Host->EnterChapterMap(TEXT("belt_waystation")));
    const auto* Belt = MemoriaChapterMaps::Find(TEXT("belt_waystation"));
    Run->SetStoryFlag(Belt->Exit.Requires, true);
    for (bool bKo : {false, true})
    {
        Run->SetLocale(bKo ? TEXT("ko") : TEXT("en"));
        const auto Before = Persisted(*Run); const FString WorldBefore = Run->GetWorldCognition()->ExportJson(); const auto TraceBefore = Host->GetTrace();
        for (int32 I = 0; I < 10; ++I)
            TestEqual(TEXT("Actual tracker delegates to the pure revealed goal"), Host->GetQuestTrackerLine(), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), Belt->Map));
        TestTrue(TEXT("Repeated HUD reads preserve all persisted run and memory state"), !Before.IsEmpty() && Before == Persisted(*Run));
        TestEqual(TEXT("Repeated HUD reads preserve world cognition"), Run->GetWorldCognition()->ExportJson(), WorldBefore);
        TestTrue(TEXT("Repeated HUD reads emit no narrative events"), Host->GetTrace() == TraceBefore);
        Run->SetStoryFlag(MemoriaSumpLedger::Steps()[0].Flag, true);
        const auto BeforeLedger = Persisted(*Run);
        const FString ExactLedger = (bKo ? TEXT("퀘스트  ") : TEXT("QUEST  ")) + MemoriaSumpLedger::Title(bKo) + TEXT("  -  ") + MemoriaSumpLedger::CurrentStepText(Run->GetRunSnapshot(), bKo);
        TestEqual(TEXT("The actual tracker preserves the old ledger line exactly"), Host->GetQuestTrackerLine(), ExactLedger);
        TestTrue(TEXT("Ledger priority also remains read only"), BeforeLedger == Persisted(*Run));
        Run->SetStoryFlag(MemoriaSumpLedger::Steps().Last().Flag, true);
        const auto AfterCompletion = Persisted(*Run);
        TestEqual(TEXT("After ledger completion the current main goal resumes"), Host->GetQuestTrackerLine(), MemoriaNavigationGoal::Line(Run->GetRunSnapshot(), Belt->Map));
        TestTrue(TEXT("Resuming a main goal after quest completion remains read only"), AfterCompletion == Persisted(*Run));
        Run->RemoveStoryFlag(MemoriaSumpLedger::Steps()[0].Flag);
        Run->RemoveStoryFlag(MemoriaSumpLedger::Steps().Last().Flag);
    }
    Game->Shutdown(); return !HasAnyErrors();
}
#endif
