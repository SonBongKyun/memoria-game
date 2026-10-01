#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Journal/MemoriaJournal.h"
#include "Journal/MemoriaJournalWidget.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
// S327: the journal tables as story_journal.gd holds them (46 events, 5 people, 14 world notes, 7 choices),
// with Korean beside English and the chapter names of GameManager.localized_chapter_name.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaJournalSourceTest, "Memoria.Journal.Source", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaJournalSourceTest::RunTest(const FString&)
{
    TestEqual(TEXT("Events"), MemoriaJournal::Events().Num(), 46);
    TestEqual(TEXT("People"), MemoriaJournal::People().Num(), 5);
    TestEqual(TEXT("World"), MemoriaJournal::World().Num(), 14);
    TestEqual(TEXT("Choices"), MemoriaJournal::Choices().Num(), 7);
    const auto& First = MemoriaJournal::Events()[0];
    TestTrue(TEXT("The first event, in both languages"), First.Flag == TEXT("ch1_opening_done") && First.Title == TEXT("Awakening in the Forest") && First.TitleIn(true) == TEXT("숲에서 깨어나다"));
    TestTrue(TEXT("The art table decorates its events"), First.Art.StartsWith(TEXT("res://")) );
    TestEqual(TEXT("Chapter 3 in Korean"), MemoriaJournal::ChapterName(3, true), FString(TEXT("벨트 중계소")));
    TestEqual(TEXT("Chapter 3 in English"), MemoriaJournal::ChapterName(3, false), FString(TEXT("Belt Waystation")));
    const auto* Tobias = MemoriaJournal::People().FindByPredicate([](const FMemoriaJournalEntry& E) { return E.Title == TEXT("Tobias Crane"); });
    const auto* Elia = MemoriaJournal::People().FindByPredicate([](const FMemoriaJournalEntry& E) { return E.Title == TEXT("Elia"); });
    TestTrue(TEXT("A name_ko where the source gives one"), Tobias && Tobias->TitleIn(true) == TEXT("토비아스 크레인"));
    TestTrue(TEXT("The speaker name otherwise"), Elia && Elia->TitleIn(true) == TEXT("엘리아") && Elia->RoleKo == TEXT("앵커 · 동행자"));
    return true;
}
// S332: the Quests and Losses tabs' data. SideQuest's statuses from the run's chapter and flags, with only
// the quests of ported maps listed; and the world rewrite's loss records for burned and faded memories, by
// rule or by the grade's defaults, in both languages. Every illustration of the journal's own table loads.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaJournalRecordsTest, "Memoria.Journal.Records", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaJournalRecordsTest::RunTest(const FString&)
{
    TestEqual(TEXT("Six side quests"), MemoriaJournal::Quests().Num(), 6);
    const auto* Ledger = MemoriaJournal::Quests().FindByPredicate([](const FMemoriaJournalQuest& Q) { return Q.Id == TEXT("sump_ledger"); });
    const auto* Echoes = MemoriaJournal::Quests().FindByPredicate([](const FMemoriaJournalQuest& Q) { return Q.Id == TEXT("echoes_ash"); });
    const auto* Vigil = MemoriaJournal::Quests().FindByPredicate([](const FMemoriaJournalQuest& Q) { return Q.Id == TEXT("sable_vigil"); });
    if (!TestTrue(TEXT("The source's quests"), Ledger && Echoes && Vigil)) return false;
    TestTrue(TEXT("The Sump Ledger as the source defines it"), Ledger->Map == TEXT("verdan_market") && Ledger->ChapterReq == 3 && Ledger->Steps.Num() == 3
        && Ledger->Steps[0].Flag == TEXT("sq_sump_ledger_started") && Ledger->Steps[2].Flag == TEXT("sq_sump_ledger_done") && Ledger->TitleKo == TEXT("웅덩이의 장부") && Ledger->Npc == TEXT("Nervous Trader"));
    TestTrue(TEXT("A prerequisite flag"), Vigil->PrereqFlag == TEXT("sable_joined") && Vigil->ChapterReq == 4);
    for (const auto& Art : MemoriaJournal::ArtSources())
    {
        UTexture2D* Texture = MemoriaJournal::LoadArt(Art.Source);
        if (TestNotNull(Art.Source, Texture)) TestTrue(*FString::Printf(TEXT("Decoded %s"), Art.Source), Texture->GetSizeX() > 200 && Texture->GetSizeY() > 200);
    }
    TestNull(TEXT("A picture outside the tables"), MemoriaJournal::LoadArt(TEXT("res://assets/cg/generated/archive_ch9_kairos_outcomes_v1.png")));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 2"), Run->BeginStartingMemoryRun(2) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    // Quests.
    TestTrue(TEXT("Locked before its chapter"), MemoriaJournal::QuestStatus(*Ledger, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Locked);
    TestTrue(TEXT("Echoes in the Ash is available to the source"), MemoriaJournal::QuestStatus(*Echoes, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Available);
    TestEqual(TEXT("but Rim Forest is not a map of the port, so nothing is listed"), MemoriaJournal::QuestRecords(*Run, true).Num(), 0);
    Run->SetCurrentChapter(4);
    TestTrue(TEXT("The prerequisite flag locks Sable's Vigil"), MemoriaJournal::QuestStatus(*Vigil, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Locked);
    Run->SetCurrentChapter(3);
    TestTrue(TEXT("Available from Chapter 3"), MemoriaJournal::QuestStatus(*Ledger, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Available);
    auto Rows = MemoriaJournal::QuestRecords(*Run, true);
    TestTrue(TEXT("Listed as new, with where to go"), Rows.Num() == 1 && Rows[0].Label == TEXT("[신규] 웅덩이의 장부") && Rows[0].Title == TEXT("웅덩이의 장부")
        && Rows[0].Body.StartsWith(TEXT("불안해하는 상인이")) && Rows[0].Body.EndsWith(TEXT("골목 근처의 불안한 상인과 대화하기")) && Rows[0].Art.EndsWith(TEXT("quest_sump_ledger_v1.png")));
    Rows = MemoriaJournal::QuestRecords(*Run, false);
    TestTrue(TEXT("In English"), Rows.Num() == 1 && Rows[0].Label == TEXT("[NEW] The Sump Ledger") && Rows[0].Body.EndsWith(TEXT("Talk to Nervous Trader at Verdan Market.")));
    Run->SetStoryFlag(TEXT("sq_sump_ledger_started"), true);
    TestTrue(TEXT("Active once started"), MemoriaJournal::QuestStatus(*Ledger, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Active);
    Rows = MemoriaJournal::QuestRecords(*Run, true);
    TestTrue(TEXT("Active: the current step"), Rows.Num() == 1 && Rows[0].Label == TEXT("웅덩이의 장부") && Rows[0].Body.EndsWith(TEXT("현재: 웅덩이에 숨겨진 장부 찾기")));
    Run->SetStoryFlag(TEXT("sq_sump_ledger_found"), true); Run->SetStoryFlag(TEXT("sq_sump_ledger_done"), true);
    TestTrue(TEXT("Complete on the last step"), MemoriaJournal::QuestStatus(*Ledger, Run->GetRunSnapshot()) == EMemoriaQuestStatus::Complete);
    Rows = MemoriaJournal::QuestRecords(*Run, false);
    TestTrue(TEXT("Complete: done, with the description alone"), Rows.Num() == 1 && Rows[0].Label == TEXT("[DONE] The Sump Ledger") && Rows[0].Body == Ledger->Desc);
    // Losses.
    TestEqual(TEXT("No losses while nothing is burned or faded"), MemoriaJournal::LossRecords(*Run, false).Num(), 0);
    TestTrue(TEXT("The market food burns"), Run->BurnMemory(TEXT("daily_market_food")) == EMemoriaMemoryResult::Success);
    auto Losses = MemoriaJournal::LossRecords(*Run, false);
    if (TestEqual(TEXT("One record"), Losses.Num(), 1))
    {
        const auto& L = Losses[0];
        TestEqual(TEXT("Its title"), L.Title, FString(TEXT("BURNED - Street Food in a Market")));
        TestTrue(TEXT("The rule's line, compass and hook"), L.Body.Contains(TEXT("World consequence:\nA vendor's face slips out of every market smell.")) && L.Body.Contains(TEXT("Compass reading:\nVerdan taste map erased."))
            && L.Body.EndsWith(TEXT("Story hook: world_rewrite_verdan_taste_blurred")) && L.Body.Contains(TEXT("Grade: Grade 4 / Daily Life")));
        TestTrue(TEXT("The rule's colour and picture"), L.Color.Equals(FLinearColor(.95f, .62f, .30f)) && L.Art.EndsWith(TEXT("world_rewrite_verdan_taste_v3.png")) && MemoriaJournal::LoadArt(L.Art) != nullptr);
    }
    Losses = MemoriaJournal::LossRecords(*Run, true);
    TestTrue(TEXT("In Korean"), Losses.Num() == 1 && Losses[0].Title == TEXT("연소 - 시장에서 먹던 길거리 음식") && Losses[0].Body.Contains(TEXT("시장의 모든 냄새에서 상인의 얼굴이 빠져나간다.")) && Losses[0].Body.Contains(TEXT("등급: 4등급 / 일상")));
    // A memory without a rule takes its grade's defaults.
    TestTrue(TEXT("The warm light burns"), Run->BurnMemory(TEXT("sense_warm_light")) == EMemoriaMemoryResult::Success);
    Losses = MemoriaJournal::LossRecords(*Run, false);
    const auto* Light = Losses.FindByPredicate([](const FMemoriaJournalRecord& R) { return R.Title == TEXT("BURNED - Warm Light Through a Window"); });
    if (TestNotNull(TEXT("The second record, in the archive's order"), Light))
    {
        TestTrue(TEXT("It comes before the market food"), Losses.Num() == 2 && Light == &Losses[0]);
        TestTrue(TEXT("The grade's default line and compass"), Light->Body.Contains(TEXT("A small sensation leaves the weather of the room.")) && Light->Body.Contains(TEXT("Compass reading:\nLost: Warm Light Through a Window"))
            && Light->Body.EndsWith(TEXT("Story hook: world_rewrite_sense_warm_light")));
        TestTrue(TEXT("The grade's colour and the blank book"), Light->Color.Equals(FLinearColor(.70f, .76f, .62f)) && Light->Art.EndsWith(TEXT("ui_loss_record_blank_book_v2.png")));
    }
    // Erosion fades the forest's smell by Chapter 4: a fading record, with the rule's text under the prefixes.
    TestTrue(TEXT("Chapters 3 and 4 erode"), Run->AddChapterMemories(3) == EMemoriaMemoryResult::Success && (Run->SetCurrentChapter(4), Run->AddChapterMemories(4)) == EMemoriaMemoryResult::Success);
    Losses = MemoriaJournal::LossRecords(*Run, false);
    const auto* Forest = Losses.FindByPredicate([](const FMemoriaJournalRecord& R) { return R.Title == TEXT("FADING - Forest After Rain"); });
    if (TestNotNull(TEXT("The forest's smell is fading"), Forest))
        TestTrue(TEXT("The fading prefixes"), Forest->Body.StartsWith(TEXT("FADING\n")) && Forest->Body.Contains(TEXT("World consequence:\nBefore it burns, it thins: ")) && Forest->Body.Contains(TEXT("Compass reading:\nErosion warning: "))
            && Forest->Art.EndsWith(TEXT("memory_rewrite_forest_scent_v2.png")));
    Losses = MemoriaJournal::LossRecords(*Run, true);
    TestTrue(TEXT("Fading, in Korean"), Losses.ContainsByPredicate([](const FMemoriaJournalRecord& R) { return R.Title == TEXT("바래는 중 - 비 온 뒤의 숲") && R.Body.Contains(TEXT("타기도 전에 얇아진다: ")); }));
    Game->Shutdown();
    return true;
}
namespace
{
// S327 in Verdan: after the arrival, the journal from the pause menu lists the market in Chapter 2 (its
// header and its event), Malet among the people, what Chapter 2 taught of the world, and no choices yet;
// the tabs turn with the arrows and ESC returns to the menu. Then (S332) in Chapter 3 the Quests
// tab offers the Sump Ledger and the Losses tab holds the record of what the road burned, each with its picture.
class FJournalReplay final : public IAutomationLatentCommand
{
public:
    explicit FJournalReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FJournalReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 120) { Test->AddError(FString::Printf(TEXT("Journal timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || !PC->GetPawn() || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || (Phase == 0 && Host->GetState() != EMemoriaSliceState::Exploration)) return false;
        ++Frame;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Journal") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f)); };
        auto* J = PC->GetJournalWidget();
        switch (Phase)
        {
        case 0:
            if (Frame < 20) break;
            Press(EKeys::Escape);
            if (auto* Menu = PC->GetPauseWidget())
            {
                int32 Row = -1; for (int32 I = 0; I < UMemoriaPauseWidget::ItemCount; ++I) if (Menu->ItemLabel(I) == TEXT("저널")) Row = I;
                Test->TestTrue(TEXT("The menu lists the Journal before the Codex"), Row >= 0 && Menu->ItemLabel(Row + 1) == TEXT("도감"));
                Menu->Select(Row); Press(EKeys::Enter);
            }
            ++Phase; Mark = Frame; break;
        case 1:
            if (Frame < Mark + 10) break;
            if (!J) { Test->AddError(TEXT("The Journal row did not open the journal")); return true; }
            {
                const auto Lines = J->ListLines();
                Test->TestTrue(TEXT("The summary names Chapter 2"), J->SummaryText().StartsWith(TEXT("2장 / 베르단 시장")));
                Test->TestTrue(TEXT("Chapter 2's header"), Lines.Num() >= 2 && Lines[0].Entry < 0 && Lines[0].Label == TEXT("2장 · 베르단 시장"));
                Test->TestTrue(TEXT("The market's event"), Lines.Num() >= 2 && Lines[1].Label == TEXT("베르단 시장"));
                Test->TestTrue(TEXT("Its description"), J->DetailBody().StartsWith(TEXT("그레이 벨트")));
            }
            Capture(TEXT("JournalEvents"));
            ++Phase; Mark = Frame; break;
        case 2:
            if (Frame < Mark + 6) break;
            Press(EKeys::Right);
            Test->TestEqual(TEXT("Right turns to the people"), J->GetTab(), 1);
            Test->TestTrue(TEXT("Malet is met"), J->ListLines().ContainsByPredicate([](const UMemoriaJournalWidget::FLine& L) { return L.Label == TEXT("말렛"); }));
            Test->TestTrue(TEXT("with the role first"), J->DetailBody().StartsWith(TEXT("기억 거래상")));
            ++Phase; Mark = Frame; break;
        case 3:
            if (Frame == Mark + 8) Capture(TEXT("JournalPeople"));
            if (Frame < Mark + 12) break;
            Press(EKeys::Right);
            Test->TestTrue(TEXT("The world as Chapter 2 taught it"), J->ListLines().ContainsByPredicate([](const UMemoriaJournalWidget::FLine& L) { return L.Label == TEXT("그레인, 앰풀, 그리고 빚"); }));
            Press(EKeys::Right);
            Test->TestTrue(TEXT("No choices yet"), J->ListLines().Num() == 1 && J->ListLines()[0].Entry < 0);
            // S332: no quests in Chapter 2; the Losses tab already holds what the road to Verdan burned.
            Press(EKeys::Right);
            Test->TestTrue(TEXT("The Quests tab, empty"), J->GetTab() == UMemoriaJournalWidget::QuestsTab && J->ListLines().Num() == 1 && J->ListLines()[0].Label == TEXT("아직 발견한 퀘스트가 없습니다."));
            Press(EKeys::Right);
            {
                const int32 Burned = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
                Test->TestTrue(TEXT("The Losses tab: one record per burn"), J->GetTab() == UMemoriaJournalWidget::LossesTab && Burned > 0 && J->GetLossCount() == Burned && J->ListLines().Num() == Burned);
            }
            Press(EKeys::Right);
            Test->TestEqual(TEXT("The tabs wrap to the events"), J->GetTab(), 0);
            Press(EKeys::Escape);
            Test->TestTrue(TEXT("ESC returns to the menu"), PC->GetJournalWidget() == nullptr && PC->GetPauseWidget() != nullptr);
            Press(EKeys::Escape);
            ++Phase; Mark = Frame; break;
        case 4:
        {
            if (Frame < Mark + 6) break;
            // Chapter 3: a quest to find, beside the loss to read.
            World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->SetCurrentChapter(3);
            Press(EKeys::Escape);
            if (auto* Menu = PC->GetPauseWidget())
            {
                for (int32 I = 0; I < UMemoriaPauseWidget::ItemCount; ++I) if (Menu->ItemLabel(I) == TEXT("저널")) Menu->Select(I);
                Press(EKeys::Enter);
            }
            ++Phase; Mark = Frame; break;
        }
        case 5:
            if (Frame < Mark + 10) break;
            if (!J) { Test->AddError(TEXT("The journal did not reopen")); return true; }
            {
                const int32 Burned = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory()->GetSnapshot().BurnedHistory.Num();
                Test->TestTrue(TEXT("The summary counts the losses"), J->SummaryText().Contains(FString::Printf(TEXT("연소 %d    상실 %d"), Burned, Burned)));
            }
            J->SetTab(UMemoriaJournalWidget::QuestsTab);
            Test->TestTrue(TEXT("The Sump Ledger is new"), J->ListLines().Num() == 1 && J->ListLines()[0].Label == TEXT("[신규] 웅덩이의 장부") && J->DetailTitle() == TEXT("웅덩이의 장부") && J->HasDetailArt());
            ++Phase; Mark = Frame; break;
        case 6:
            if (Frame == Mark + 8) Capture(TEXT("JournalQuests"));
            if (Frame < Mark + 14) break;
            J->SetTab(UMemoriaJournalWidget::LossesTab);
            Test->TestTrue(TEXT("The loss is recorded with its consequence and its picture"), J->ListLines().Num() >= 1 && J->ListLines()[0].Label.StartsWith(TEXT("연소 - ")) && J->DetailTitle() == J->ListLines()[0].Label
                && J->DetailBody().Contains(TEXT("세계에 남은 결과:")) && J->DetailBody().Contains(TEXT("나침반 판독:")) && J->HasDetailArt());
            ++Phase; Mark = Frame; break;
        case 7:
            if (Frame == Mark + 8) Capture(TEXT("JournalLosses"));
            if (Frame < Mark + 14) break;
            Press(EKeys::Escape); Press(EKeys::Escape);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJournalVisualTest, "MemoriaVisual.Journal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJournalVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FJournalReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
