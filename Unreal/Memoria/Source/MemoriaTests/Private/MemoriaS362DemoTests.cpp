#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaClassifier.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Presentation/MemoriaDemoCompletion.h"
#include "Presentation/MemoriaDemoCompletionWidget.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
TArray<uint8> Persisted(const UMemoriaRunSubsystem& Run)
{ TArray<uint8> Bytes; if (auto* Save = Run.CaptureSave()) UGameplayStatics::SaveGameToMemory(Save, Bytes); return Bytes; }
void CompleteFlags(FMemoriaRunSnapshot& Run)
{
    Run.RunId = FGuid::NewGuid(); Run.CurrentChapter = 5;
    for (const TCHAR* Id : {TEXT("ch5_classifier_started"), TEXT("canon_ch6_seam_ready")})
    { FMemoriaStoryFlag Flag; Flag.Id = Id; Flag.bValue = true; Run.StoryFlags.Add(Flag); }
}
class FDemoReplay final : public IAutomationLatentCommand
{
public:
    FDemoReplay(FAutomationTestBase* InTest, bool bInIdentified) : Test(InTest), bIdentified(bInIdentified), Started(FPlatformTime::Seconds()) {}
    ~FDemoReplay() override
    {
        if (bSettingsCaptured && SavedSettings.IsValid()) SavedSettings->SetLocale(OldSettingsLocale);
        if (GEditor && GEditor->PlayWorld)
            if (auto* PC = GEditor->PlayWorld->GetFirstPlayerController())
            {
                if (bTitleDownHeld) PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Down, IE_Released, 0.f));
                if (bTitleConfirmHeld) PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Released, 0.f));
            }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240.) { Test->AddError(FString::Printf(TEXT("S362 demo timeout, step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 60.); }
        ++Frame; auto* Game = World->GetGameInstance();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Checkpoint = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        if (Step == 0)
        {
            if (!Map || !Map->GetSpec()) return false;
            for (const TCHAR* Flag : {TEXT("ch4_arrived"), TEXT("ch4_reading_loss"), TEXT("ch4_anchoring"), TEXT("ch4_night_watch")}) Run->SetStoryFlag(Flag, true);
            Run->GetWorldCognition()->SeedMaletRoute(bIdentified);
            Test->TestTrue(TEXT("Demo route uses isolated disk storage"), Checkpoint->ConfigureTestStorage(TEXT("s362-demo-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            SavedSettings = Game->GetSubsystem<UMemoriaSettingsSubsystem>(); OldSettingsLocale = SavedSettings->GetLocale(); bSettingsCaptured = true;
            SavedSettings->SetLocale(bIdentified ? TEXT("ko") : TEXT("en")); Run->SetLocale(bIdentified ? TEXT("ko") : TEXT("en"));
            Step = 1; Mark = Frame;
        }
        if (Step < 3 && (Host->GetState() == EMemoriaSliceState::Field || Host->GetState() == EMemoriaSliceState::VN))
        { if (Frame % 4 == 0) Host->Confirm(); return false; }
        if (Step == 1 && Frame > Mark + 30 && Map && Map->GetSpec())
        { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Map->GetSpec()->Exit.Rect.Center()) + FVector(0, 0, Pawn->GetActorLocation().Z)); Step = 2; }
        if (Step == 2 && Map && Map->IsChapterComplete() && MemoriaDemoCompletion::CanPresent(Run->GetRunSnapshot(), Host->GetChapterMap(), Host->GetState() == EMemoriaSliceState::Exploration, Map->GetCard() && Map->GetCard()->IsShowing()))
        {
            Test->TestTrue(TEXT("Actual classifier branch is retained"), Run->GetRunSnapshot().GetFlag(bIdentified ? TEXT("ch5_malet_report_identified_arrel") : TEXT("ch5_malet_report_requester_unknown")));
            Test->TestTrue(TEXT("Actual route played Chapter 5"), Host->GetTrace().Contains(TEXT("vn:start:ch5_classifier")));
            Test->TestTrue(TEXT("Controller enables completion after the card closes"), PC->CanOpenDemoCompletion());
            Test->TestNull(TEXT("Chapter completion does not auto-open a blocking modal"), PC->GetDemoCompletionWidget());
            Before = Persisted(*Run); SavedRunId = Run->GetRunSnapshot().RunId; SavedHistory = Run->GetPlayerMemory()->GetSnapshot().BurnedHistory; SavedWorld = Run->GetWorldCognition()->ExportJson();
            if (!OpenFromPause(PC)) return true;
            Widget = PC->GetDemoCompletionWidget();
            Test->TestTrue(TEXT("The actual controller completion modal pauses the field"), UGameplayStatics::IsGamePaused(World));
            Test->TestNull(TEXT("Completion replaces the Pause menu"), PC->GetPauseWidget());
            Test->TestTrue(TEXT("Controller passes the requested locale to the actual widget"), Widget->VisibleText().Contains(bIdentified ? TEXT("체험판 구간 완료") : TEXT("Demo segment complete")));
            Step = 3; Mark = Frame;
        }
        if (Step == 3 && Widget.IsValid())
        {
            if (Frame == Mark + 20)
            {
                const auto Size = Widget->GetCachedGeometry().GetLocalSize();
                Test->TestEqual(TEXT("Four controller-owned completion actions are rendered"), Widget->GetActionBounds().Num(), 4);
                for (const auto& Bounds : Widget->GetActionBounds()) Test->TestTrue(TEXT("Completion actions remain within the native viewport"), Bounds.Min.X >= 0 && Bounds.Min.Y >= 0 && Bounds.Max.X <= Size.X && Bounds.Max.Y <= Size.Y);
                Capture(bIdentified ? TEXT("CompleteKo") : TEXT("CompleteEn"));
            }
            if (Frame == Mark + 24)
            {
                // A screenshot is fulfilled on a later rendered frame. Keep the summary
                // visible until its pixels have been captured before changing to records.
                Test->TestTrue(TEXT("Completion exposes only the already revealed classifier result"),
                    Widget->VisibleText().Contains(bIdentified ? TEXT("아렐 식별") : TEXT("requester unknown")));
                Press(PC, EKeys::Enter); // actual controller routes selected row 0 to Review
                Test->TestTrue(TEXT("Controller confirmation opens records"), Widget->IsReviewing());
            }
            if (Frame == Mark + 30) Capture(bIdentified ? TEXT("ReviewKo") : TEXT("ReviewEn"));
            if (Frame == Mark + 40)
            {
                Press(PC, EKeys::Escape);
                Test->TestFalse(TEXT("Controller Back from review returns to the summary"), Widget->IsReviewing());
                Test->TestTrue(TEXT("Review Back leaves the actual completion modal open"), PC->GetDemoCompletionWidget() == Widget.Get() && UGameplayStatics::IsGamePaused(World));
                Test->TestTrue(TEXT("Read-only controller review preserves all persisted state"), !Before.IsEmpty() && Before == Persisted(*Run));
                // An invalid position is the isolated failure fixture; the Save action itself is always
                // dispatched by the real controller rather than directly invoking the widget method.
                Widget->SetSourcePosition(FVector2D(-1, -1)); Press(PC, EKeys::Down); Press(PC, EKeys::Enter);
                Test->TestFalse(TEXT("Real Save action never claims invalid-position success"), Widget->WasLastSaveSuccessful());
                Test->TestEqual(TEXT("Failure exposes the actual disk adapter status"), Widget->GetDiskStatus(), Checkpoint->GetStatusText());
                Widget->SetSourcePosition(Map->PlayerSource()); Press(PC, EKeys::Enter);
                Test->TestTrue(TEXT("Controller retries saving at the real current position"), Widget->WasLastSaveSuccessful());
                Test->TestTrue(TEXT("Save leaves persisted gameplay state unchanged"), Before == Persisted(*Run));
                Test->TestTrue(TEXT("Continue has an actual Drift save"), Checkpoint->FindContinue() == EMemoriaContinueSource::Map && Checkpoint->PeekChapterMap() == TEXT("drift_shelter"));
                Press(PC, EKeys::Down); Press(PC, EKeys::Enter); // selected row 2, Return to Drift
                Test->TestNull(TEXT("Actual field-return action closes the controller modal"), PC->GetDemoCompletionWidget());
                Test->TestFalse(TEXT("Actual field-return action unpauses gameplay"), UGameplayStatics::IsGamePaused(World));
                Test->TestEqual(TEXT("Return remains in the current field"), Host->GetChapterMap(), FString(TEXT("drift_shelter")));
                Test->TestTrue(TEXT("Return preserves the run"), Before == Persisted(*Run));
                if (!OpenFromPause(PC)) return true;
                Widget = PC->GetDemoCompletionWidget();
                for (int32 I = 0; I < 3; ++I) Press(PC, EKeys::Down);
                Press(PC, EKeys::Enter); // real row 3 delegates to root controller's title travel
                Step = 4; Mark = Frame;
            }
        }
        if (Step == 4 && Host->IsOnTitle() && PC->GetTitleWidget() && PC->GetTitleWidget()->IsIntroComplete())
        {
            Test->TestTrue(TEXT("Title still offers the saved map"), Checkpoint->FindContinue() == EMemoriaContinueSource::Map);
            Test->TestNull(TEXT("Title travel closes the completion modal"), PC->GetDemoCompletionWidget());
            // The title owns Slate focus, unlike controller-painted Pause/completion menus.
            // Send real focused key events through the same path as TitleManualOnly.
            SlateTap(EKeys::Down);
            Test->TestEqual(TEXT("Focused title navigation selects Continue"), PC->GetTitleWidget()->GetSelected(), 1);
            Step = 5; Mark = Frame;
        }
        if (Step == 5 && Frame >= Mark + 2 && PC->GetTitleWidget())
        {
            SlateTap(EKeys::Enter);
            Step = 7;
        }
        if (Step == 7 && Map && Host->GetState() == EMemoriaSliceState::Exploration && Map->GetMap() == TEXT("drift_shelter"))
        {
            if (!ReturnAt) ReturnAt = Frame;
            // Fixed world frames do not advance the UMG card's Slate tick at the same rate.
            // Keep the world-delay regression window, then observe the actual card completion.
            if (Frame < ReturnAt + 420) return false;
            const auto* Fight = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            const bool bCardShowing = Map->GetCard() && Map->GetCard()->IsShowing();
            if (!bRestoredGateLogged || !bCardShowing)
            {
                Test->AddInfo(FString::Printf(TEXT("S362_DEMO_RESTORED_GATE route=%s card=%d live=%d defeated=%d picker=%d state=%d chapterMap=%s derived=%d canOpen=%d worldFrames=%d"),
                    bIdentified ? TEXT("identified") : TEXT("unknown"), int32(bCardShowing), Fight ? Fight->LiveMonsterCount() : 0,
                    int32(Fight && Fight->IsDefeated()), int32(Fight && Fight->IsPickingBurn()), int32(Host->GetState()), *Host->GetChapterMap(),
                    int32(MemoriaDemoCompletion::IsComplete(Run->GetRunSnapshot())), int32(PC->CanOpenDemoCompletion()), Frame - ReturnAt));
                bRestoredGateLogged = true;
            }
            if (bCardShowing) return false;
            Test->TestNotNull(TEXT("Continue creates the existing completion card"), Map->GetCard());
            Test->TestTrue(TEXT("Restored completion entry waits until the real card closes"), !Map->GetCard() || !Map->GetCard()->IsShowing());
            Test->TestEqual(TEXT("Restored completion is not blocked by live enemies"), Fight ? Fight->LiveMonsterCount() : 0, 0);
            Test->TestEqual(TEXT("Continue preserves the completed run identity"), Run->GetRunSnapshot().RunId, SavedRunId);
            Test->TestTrue(TEXT("Continue preserves irreversible burns"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory == SavedHistory);
            Test->TestEqual(TEXT("Continue preserves classifier world facts"), Run->GetWorldCognition()->ExportJson(), SavedWorld);
            Test->TestTrue(TEXT("Completed segment remains derived after Continue"), MemoriaDemoCompletion::IsComplete(Run->GetRunSnapshot()));
            Test->TestFalse(TEXT("Continue never replays the classifier"), Host->GetTrace().Contains(TEXT("vn:start:ch5_classifier")));
            Test->TestTrue(TEXT("Completion can be explicitly reopened after the restored card"), PC->CanOpenDemoCompletion());
            Test->TestNull(TEXT("Continue also preserves explicit, non-blocking completion entry"), PC->GetDemoCompletionWidget());
            Test->TestFalse(TEXT("Continue returns to an unpaused field"), UGameplayStatics::IsGamePaused(World));
            if (!OpenFromPause(PC)) return true;
            Test->TestTrue(TEXT("Restored completion opens through the real Pause action"), PC->GetDemoCompletionWidget() && UGameplayStatics::IsGamePaused(World));
            Press(PC, EKeys::Escape);
            Test->TestNull(TEXT("Restored completion Back closes the actual controller modal"), PC->GetDemoCompletionWidget());
            Test->TestFalse(TEXT("Restored completion Back unpauses the actual field"), UGameplayStatics::IsGamePaused(World));
            return true;
        }
        return false;
    }
private:
    void SlateTap(const FKey& Key)
    {
        FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
        FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
    }
    void Press(AMemoriaSliceController* PC, const FKey& Key)
    {
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
    }
    bool OpenFromPause(AMemoriaSliceController* PC)
    {
        Press(PC, EKeys::Escape); // InputKey -> OpenPause, no direct test widget creation
        auto* Pause = PC->GetPauseWidget();
        if (!Test->TestNotNull(TEXT("Physical Escape opens the actual Pause menu"), Pause)) return false;
        for (int32 I = 0; I < 12 && Pause->GetSelected() != 9; ++I) Press(PC, EKeys::Down);
        if (!Test->TestEqual(TEXT("Physical Pause navigation reaches the appended completion row"), Pause->GetSelected(), 9)) return false;
        Press(PC, EKeys::Enter);
        return Test->TestNotNull(TEXT("Pause confirm opens the actual controller-owned completion widget"), PC->GetDemoCompletionWidget());
    }
    void Capture(const TCHAR* Name)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S362Demo") / (FString(Name) + TEXT(".png")), true, false); }
    FAutomationTestBase* Test; bool bIdentified, bFixed = false, bOldFixed = false, bSettingsCaptured = false, bTitleConfirmHeld = false, bTitleDownHeld = false, bRestoredGateLogged = false;
    double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64; int32 Step = 0, Frame = 0, Mark = 0, ReturnAt = 0;
    TWeakObjectPtr<UMemoriaDemoCompletionWidget> Widget; TWeakObjectPtr<UMemoriaSettingsSubsystem> SavedSettings;
    TArray<uint8> Before; FGuid SavedRunId; TArray<FString> SavedHistory; FString SavedWorld, OldSettingsLocale;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362DemoContract, "Memoria.S362Demo.DerivedCompletionContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362DemoContract::RunTest(const FString&)
{
    FMemoriaRunSnapshot Run; CompleteFlags(Run);
    TestTrue(TEXT("Existing finished Chapter 5 boundary completes the segment"), MemoriaDemoCompletion::CanPresent(Run, TEXT("drift_shelter"), true, false));
    TestFalse(TEXT("Completion waits for the existing card"), MemoriaDemoCompletion::CanPresent(Run, TEXT("drift_shelter"), true, true));
    TestFalse(TEXT("Completion never interrupts dialogue"), MemoriaDemoCompletion::CanPresent(Run, TEXT("drift_shelter"), false, false));
    TestFalse(TEXT("Completion is not offered on another map"), MemoriaDemoCompletion::CanPresent(Run, TEXT("belt_waystation"), true, false));
    Run.CurrentChapter = 6; TestFalse(TEXT("Future chapters are not relabeled as this demo ending"), MemoriaDemoCompletion::IsComplete(Run));
    Run.CurrentChapter = 5; Run.StoryFlags.Pop(); TestFalse(TEXT("Chapter 4 readiness alone is not completion"), MemoriaDemoCompletion::IsComplete(Run));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362DemoReadOnly, "Memoria.S362Demo.ReadOnlyReviewAndDisabledStorage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362DemoReadOnly::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); TestEqual(TEXT("Use the real starting catalog"), Run->BeginStartingMemoryRun(5), EMemoriaMemoryResult::Success);
    for (const TCHAR* Flag : {TEXT("ch5_classifier_started"), TEXT("canon_ch6_seam_ready")}) Run->SetStoryFlag(Flag, true);
    const auto Before = Persisted(*Run); auto* Checkpoint = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    for (bool bKo : {false, true})
    {
        auto* Widget = CreateWidget<UMemoriaDemoCompletionWidget>(Game.Get()); Widget->Configure(Run, Checkpoint, FVector2D(320, 512), bKo);
        TestTrue(TEXT("The summary uses the requested locale"), Widget->VisibleText().Contains(bKo ? TEXT("체험판 구간 완료") : TEXT("Demo segment complete")));
        Widget->Activate(0); Widget->Scroll(999); Widget->Back();
        TestTrue(TEXT("Summary/review are read only"), !Before.IsEmpty() && Before == Persisted(*Run));
        TestFalse(TEXT("Disabled automation storage reports a failed save"), Widget->SaveCurrentProgress());
        TestFalse(TEXT("A failed save remains explicitly unsuccessful"), Widget->WasLastSaveSuccessful());
        TestEqual(TEXT("Save failure has the real adapter status"), Widget->GetDiskStatus(), Checkpoint->GetStatusText());
        TestTrue(TEXT("Save failure preserves the run"), Before == Persisted(*Run));
    }
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362DemoIdentified, "MemoriaVisual.S362DemoIdentified", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362DemoIdentified::RunTest(const FString&)
{ if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FDemoReplay(this, true))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362DemoUnknown, "MemoriaVisual.S362DemoUnknown", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362DemoUnknown::RunTest(const FString&)
{ if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false; FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FDemoReplay(this, false))); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true; }
#endif
