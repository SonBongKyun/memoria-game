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
namespace
{
// S327 in Verdan: after the arrival, the journal from the pause menu lists the market in Chapter 2 (its
// header and its event), Malet among the people, what Chapter 2 taught of the world, and no choices yet;
// the tabs turn with the arrows and ESC returns to the menu.
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
            Press(EKeys::Escape);
            Test->TestTrue(TEXT("ESC returns to the menu"), PC->GetJournalWidget() == nullptr && PC->GetPauseWidget() != nullptr);
            Press(EKeys::Escape);
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
