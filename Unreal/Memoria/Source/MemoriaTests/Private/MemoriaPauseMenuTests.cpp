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
// world; Options changes the shared settings; Save writes the field checkpoint and Load brings Arrel back
// to it; Quit asks first; ESC resumes.
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
        if (Step == 8 && World == TravelledFrom) return false;
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
            Test->TestTrue(TEXT("Save and Load are dark mid-chapter"), PC->GetPauseWidget() && !PC->GetPauseWidget()->IsItemEnabled(2) && !PC->GetPauseWidget()->IsItemEnabled(3));
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
            if (Host->GetState() != EMemoriaSliceState::Exploration || World == TravelledFrom) { --Step; break; }
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("The menu opens on the revisit"), PC->GetPauseWidget() != nullptr);
            Test->TestTrue(TEXT("Save and Load are lit"), PC->GetPauseWidget() && PC->GetPauseWidget()->IsItemEnabled(2) && PC->GetPauseWidget()->IsItemEnabled(3));
            break;
        case 9:
            // C. Save (the third row), then away, then Load (the fourth) brings Arrel back.
            Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Enter);
            Test->TestTrue(TEXT("The menu says it saved"), Menu && !Menu->GetNotice().IsEmpty());
            SavedAt = Pawn->GetActorLocation();
            break;
        case 10: Capture(TEXT("PauseSaved")); break;
        case 11:
            Key(EKeys::Escape);
            Pawn->SetActorLocation(SavedAt + FVector(160, 0, 0));
            break;
        case 12:
            Key(EKeys::Escape); Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Enter);
            Test->TestTrue(TEXT("Load closes the menu"), PC->GetPauseWidget() == nullptr && !World->IsPaused());
            break;
        case 13:
            Test->TestTrue(TEXT("Load brings Arrel back to the save"), FVector::Dist2D(Pawn->GetActorLocation(), SavedAt) < 20.f);
            Test->TestEqual(TEXT("Load opens the checkpoint screen, as Continue does"), Host->GetState(), EMemoriaSliceState::Deferred);
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
    UWorld* TravelledFrom = nullptr;
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
