#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FS362HeldInputReplay final : public IAutomationLatentCommand
{
public:
    explicit FS362HeldInputReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FS362HeldInputReplay() override
    {
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 120) { Test->AddError(TEXT("S362 held-input replay timeout")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3f) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer()) return false;
        // Count rendered frames even while the game world is paused.
        if (++Frame % 8 != 0) return false;
        auto Press = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)); };
        auto Release = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f)); };
        auto Tap = [&](const FKey& Key) { Press(Key); Release(Key); };
        switch (Step++)
        {
        case 0:
            Press(EKeys::K);
            Test->TestTrue(TEXT("Real guard key raises the guard"), Combat->IsBlocking());
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Pause opens while guard was held"), PC->GetPauseWidget() != nullptr && World->IsPaused());
            break;
        case 1:
            Release(EKeys::K);
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Resume closes the pause menu"), PC->GetPauseWidget() == nullptr && !World->IsPaused());
            break;
        case 2:
            Test->TestFalse(TEXT("Guard released in menu cannot remain latched"), Combat->IsBlocking());
            Test->TestTrue(TEXT("Player can move after menu guard release"), Combat->CanMove());
            Combat->EndBlock(); // Cleanup also lets the pre-fix red replay test charge independently.
            Combat->BeginCharge();
            break;
        case 3:
            Test->TestTrue(TEXT("Charge really accumulated before pause"), Combat->GetCharge() > 0.f);
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Pause opens during charge"), World->IsPaused());
            break;
        case 4:
            Release(EKeys::J);
            Tap(EKeys::Escape);
            break;
        case 5:
            Test->TestEqual(TEXT("Charge released in menu is zero after resume"), Combat->GetCharge(), 0.f);
            Combat->EndCharge();
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S362/HeldInputRecovered.png"), true, false);
            break;
        default:
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362HeldInputTest, "MemoriaUpgrade.S362.HeldInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362HeldInputTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362HeldInputReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
