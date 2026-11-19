#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaSaveSlotsWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Misc/FileHelper.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Misc/App.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S317: the ESC pause menu after pause_menu.gd, with real key presses in the Verdan field. It stops the
// world; Options changes the shared settings; Save writes an explicitly selected manual slot and Load
// brings Arrel back to it; Quit asks first; ESC backs out of the browser before it resumes the field.
class FPauseMenuReplay final : public IAutomationLatentCommand
{
public:
    explicit FPauseMenuReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FPauseMenuReplay() override
    {
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
        if (Settings.IsValid() && MasterBefore >= 0) Settings->SetMasterVolume(MasterBefore);
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Pause menu timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        // Frames, not world time: the menu pauses the world.
        if (Host->GetState() != EMemoriaSliceState::Exploration && Step == 0) return false;
        if (Step == 8 && World == TravelledFrom.Get()) return false;
        ++Frame;
        auto* Checkpoint = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
        auto Key = [&](const FKey& K) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.f)); };
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/PauseMenu") / (FString(Name) + TEXT(".png")), true, false); };
        UMemoriaPauseWidget* Menu = PC->GetPauseWidget();
        if (Step == 7) TravelledFrom = World;
        if (Frame % 6 != 0) return false; // one step every six frames, so each capture shows its state
        switch (Step++)
        {
        // A. Mid-Chapter 2: the ported checkpoint cannot be written here, and nothing is saved yet.
        case 0:
            Test->TestTrue(TEXT("Saves are isolated"), Checkpoint->ConfigureTestStorage(TEXT("pause-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("ESC opens the pause menu"), PC->GetPauseWidget() != nullptr);
            Test->TestTrue(TEXT("The world stops"), World->IsPaused());
            Test->TestTrue(TEXT("Save is dark mid-chapter but Load can browse empty slots"), PC->GetPauseWidget() && !PC->GetPauseWidget()->IsItemEnabled(2) && PC->GetPauseWidget()->IsItemEnabled(3));
            if (!PC->GetPauseWidget()) return true;
            PC->GetPauseWidget()->Select(3); Key(EKeys::Enter);
            if (auto* Browser = PC->GetSaveSlotsWidget())
            {
                Test->TestTrue(TEXT("Empty Load browser exposes autosave and three manual rows"), Browser->GetMode() == EMemoriaSaveBrowserMode::Load && Browser->GetRowCount() == 5);
                for (int32 Row = 0; Row < 4; ++Row) Test->TestFalse(TEXT("An empty slot cannot restore"), Browser->IsRowEnabled(Row));
            }
            else { Test->AddError(TEXT("Empty Load browser did not open")); return true; }
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("Esc leaves the empty browser, keeps Pause"), !PC->GetSaveSlotsWidget() && PC->GetPauseWidget() && World->IsPaused());
            if (!PC->GetPauseWidget()) return true;
            PC->GetPauseWidget()->Select(0);
            break;
        case 1: Capture(TEXT("PauseMenu")); break;
        case 2:
            // Options: the master volume moves in tens with Right.
            MasterBefore = Settings->GetMasterVolume();
            Key(EKeys::Down); Key(EKeys::Enter);
            Test->TestTrue(TEXT("Options opens"), Menu && Menu->IsOptionsOpen());
            Key(EKeys::Right);
            Test->TestEqual(TEXT("Right raises the master volume"), Settings->GetMasterVolume(), FMath::Min(100, MasterBefore + 10));
            break;
        case 3: Capture(TEXT("PauseOptions")); break;
        case 4:
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("ESC leaves Options, the menu stays"), Menu && !Menu->IsOptionsOpen() && World->IsPaused());
            // Quit (the last row: from Options, up past Resume) asks first, and the default No keeps playing.
            Key(EKeys::Up); Key(EKeys::Up); Key(EKeys::Enter);
            Test->TestTrue(TEXT("Quit asks first"), Menu && Menu->IsAskingQuit());
            break;
        case 5: Capture(TEXT("PauseQuit")); break;
        case 6:
            Key(EKeys::Enter);
            Test->TestTrue(TEXT("No stays in the menu"), Menu && !Menu->IsAskingQuit());
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("ESC resumes"), PC->GetPauseWidget() == nullptr && !World->IsPaused());
            break;
        // B. The closed Chapter 2 boundary, saved as the market's save point does, then its checkpoint screen and
        // the revisit into Verdan: the state in which the game saves.
        case 7:
        {
            auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
            FMemoriaRunSnapshot Boundary = Run->GetRunSnapshot(); Boundary.CurrentChapter = 3;
            Test->TestEqual(TEXT("The run reaches the boundary"), Run->RestoreRun(Boundary, Run->GetPlayerMemory()->GetDefinitions(), Run->GetPlayerMemory()->GetSnapshot(), Run->GetWorldCognition()->GetSnapshot()), EMemoriaMemoryResult::Success);
            Test->TestTrue(TEXT("Chapter 2 is closed"), Run->SetStoryFlag(TEXT("ch2_complete"), true));
            Test->TestTrue(TEXT("The boundary saves"), Checkpoint->SaveClosedBoundary(FVector2D(500, 340)));
            Test->TestTrue(TEXT("Its checkpoint screen opens"), Host->ContinueCheckpoint());
            Test->TestTrue(TEXT("The revisit travels"), Host->RequestCheckpointRevisit());
            break;
        }
        case 8:
            if (Host->GetState() != EMemoriaSliceState::Exploration || World == TravelledFrom.Get()) { --Step; break; }
            if (auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>()) Combat->Revive(.63f);
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("The menu opens on the revisit"), PC->GetPauseWidget() != nullptr);
            Test->TestTrue(TEXT("Save and Load are lit"), PC->GetPauseWidget() && PC->GetPauseWidget()->IsItemEnabled(2) && PC->GetPauseWidget()->IsItemEnabled(3));
            break;
        case 9:
        {
            // C. Save (third row) opens the real browser; explicitly choose empty manual1.
            // The old boundary autosave remains a separate Continue source, not a side effect of Save.
            FFileHelper::LoadFileToString(BoundaryAutosave, *Checkpoint->GetSlotPath());
            Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Enter);
            if (!Test->TestNotNull(TEXT("Save opens the manual browser"), PC->GetSaveSlotsWidget())) return true;
            Test->TestEqual(TEXT("Default selected save is manual1"), PC->GetSaveSlotsWidget()->GetSelectedSlot(), 1);
            Key(EKeys::Enter);
            Test->TestTrue(TEXT("The menu says manual1 saved"), Menu && !Menu->GetNotice().IsEmpty() && Checkpoint->GetManualSlotSummary(1).CanLoad());
            const auto Info = Checkpoint->GetManualSlotSummary(1);
            Test->TestTrue(TEXT("Manual metadata names the actual region and chapter"), Info.Map == TEXT("verdan_market") && Info.Chapter == 3 && Info.SavedAtUtc.GetTicks() > 0);
            SavedAt = Pawn->GetActorLocation();
            SavedRun = Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().RunId;
            SavedHp = Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().Player.Hp;
            SavedGrains = Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().Player.Grains;
            Test->TestTrue(TEXT("The saved HP fixture is nonmax"), SavedHp > 0 && SavedHp < Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().Player.MaxHp);
            break;
        }
        case 10: Capture(TEXT("PauseSaved")); break;
        case 11:
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("First Esc closes only the browser"), !PC->GetSaveSlotsWidget() && PC->GetPauseWidget() && World->IsPaused());
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("Second Esc resumes the field"), !PC->GetPauseWidget() && !World->IsPaused());
            Pawn->SetActorLocation(SavedAt + FVector(160, 0, 0));
            Game->GetSubsystem<UMemoriaRunSubsystem>()->AddGrains(1);
            if (auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>()) Combat->Revive(.91f);
            break;
        case 12:
            Key(EKeys::Escape); Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Enter);
            if (!Test->TestNotNull(TEXT("Load opens the real browser"), PC->GetSaveSlotsWidget())) return true;
            Key(EKeys::Down);
            Test->TestEqual(TEXT("Load explicitly selects manual1 rather than autosave0"), PC->GetSaveSlotsWidget()->GetSelectedSlot(), 1);
            Key(EKeys::Enter);
            Test->TestTrue(TEXT("Selected manual load closes both menus"), !PC->GetSaveSlotsWidget() && PC->GetPauseWidget() == nullptr && !World->IsPaused());
            break;
        case 13:
            Test->TestTrue(TEXT("Load brings Arrel back to the save"), FVector::Dist2D(Pawn->GetActorLocation(), SavedAt) < 20.f);
            Test->TestEqual(TEXT("Load opens the checkpoint screen, as Continue does"), Host->GetState(), EMemoriaSliceState::Deferred);
            Test->TestTrue(TEXT("Manual load restores the run and Grains"), Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().RunId == SavedRun && Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().Player.Grains == SavedGrains);
            Test->TestEqual(TEXT("Manual load preserves exact saved nonmax HP"), Game->GetSubsystem<UMemoriaRunSubsystem>()->GetRunSnapshot().Player.Hp, SavedHp);
            { FString Current; FFileHelper::LoadFileToString(Current, *Checkpoint->GetSlotPath()); Test->TestEqual(TEXT("Pause manual Save/Load leaves boundary autosave bytes unchanged"), Current, BoundaryAutosave); }
            return true;
        default: return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, MasterBefore = -1;
    bool bFixed = false, bOldFixed = false;
    FVector SavedAt = FVector::ZeroVector;
    FGuid SavedRun;
    int64 SavedHp = 0, SavedGrains = 0;
    FString BoundaryAutosave;
    // Weak: the old world is freed after travel, and a raw pointer can match the new world at a reused address.
    TWeakObjectPtr<UWorld> TravelledFrom;
    TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseMenuTest, "MemoriaVisual.PauseMenu", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPauseMenuTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FPauseMenuReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
