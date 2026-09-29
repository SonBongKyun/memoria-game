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
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaGameOverWidget.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Misc/App.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S318: Arrel falls in the field and game_over.gd's screen takes the choice, with real key presses:
// Load Save without a save only answers with the cancel sound; Stagger On returns him at 30% HP with the
// husks gone; a second fall's Return to Title reaches the title.
class FGameOverReplay final : public IAutomationLatentCommand
{
public:
    explicit FGameOverReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FGameOverReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Game over timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (Step == 9)
        {
            // Return to Title travels to the title map.
            if (Host->IsOnTitle()) { Test->TestTrue(TEXT("Return to Title reaches the title"), true); return true; }
            return false;
        }
        auto* Pawn = Cast<AMemoriaFieldPawn>(PC->GetPawn());
        if (!Pawn || !World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        if (Step == 0 && Host->GetState() != EMemoriaSliceState::Exploration) return false;
        ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer()) return false;
        auto Key = [&](const FKey& K) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.f)); };
        auto Fall = [&]
        {
            // A killing blow from an adjacent husk.
            AMemoriaFieldMonster* Husk = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(60, 0, 0), 0.f)[0];
            return Husk && Combat->StrikePlayer(Husk, 9999.f) && Combat->IsDefeated();
        };
        UMemoriaGameOverWidget* Screen = PC->GetGameOverWidget();
        switch (Step)
        {
        case 0:
            Test->TestTrue(TEXT("No save in the isolated storage"), World->GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(TEXT("gameover-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            Test->TestTrue(TEXT("Arrel falls"), Fall());
            Test->TestTrue(TEXT("The fall plays before the screen"), PC->GetGameOverWidget() == nullptr);
            ++Step; Mark = Frame; break;
        case 1:
            if (!Screen) { if (Frame > Mark + 400) { Test->AddError(TEXT("No game over screen")); return true; } break; }
            Test->TestTrue(TEXT("The screen follows the fall"), Frame >= Mark + 140);
            Test->TestFalse(TEXT("Nothing to load"), Screen->CanLoad());
            ++Step; Mark = Frame; break;
        case 2:
            if (Frame == Mark + 40) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/GameOver/GameOver.png"), true, false);
            if (Frame < Mark + 46) break;
            // Load Save without a save: the screen stays.
            Key(EKeys::Down); Key(EKeys::Enter);
            Test->TestTrue(TEXT("Load without a save keeps the screen"), PC->GetGameOverWidget() != nullptr && Combat->IsDefeated());
            // Stagger On.
            Key(EKeys::Up); Key(EKeys::Enter);
            Test->TestTrue(TEXT("Stagger On closes the screen"), PC->GetGameOverWidget() == nullptr && !Combat->IsDefeated());
            Test->TestEqual(TEXT("Arrel rises at 30% HP"), Combat->GetPlayerHp(), int64(Combat->GetPlayerMaxHp() * .3));
            Test->TestEqual(TEXT("The husks are gone"), Combat->LiveMonsterCount(), 0);
            ++Step; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 20) break;
            Test->TestTrue(TEXT("Arrel can move again"), Combat->CanMove());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/GameOver/StaggerOn.png"), true, false);
            ++Step; Mark = Frame; break;
        case 4:
            if (Frame < Mark + 4) break;
            Test->TestTrue(TEXT("Arrel falls again"), Fall());
            ++Step; Mark = Frame; break;
        case 5:
            if (!Screen) break;
            // Return to Title, the third choice.
            Key(EKeys::Down); Key(EKeys::Down); Key(EKeys::Enter);
            Step = 9; break;
        default: return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameOverTest, "MemoriaVisual.GameOver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameOverTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FGameOverReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
