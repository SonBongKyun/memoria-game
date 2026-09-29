#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Tutorial/MemoriaHintWidget.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S323: tutorial_hints.gd in the Verdan field. The first foes raise the first-battle hint (the action
// controls); a dash lets it go and still dashes; the first burn raises the burn's hint in Korean; it leaves
// on its own after four seconds; and neither returns when its moment comes again.
class FTutorialHintsReplay final : public IAutomationLatentCommand
{
public:
    explicit FTutorialHintsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FTutorialHintsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Tutorial hints timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Tutorial = World->GetGameInstance()->GetSubsystem<UMemoriaTutorialSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        ++Frame;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/TutorialHints") / (FString(Name) + TEXT(".png")), true, false); };
        auto Press = [&](const FKey& Key) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f)); };
        auto Count = [&](const TCHAR* Event) { int32 N = 0; for (const FString& E : Host->GetTrace()) N += E == Event ? 1 : 0; return N; };
        switch (Phase)
        {
        case 0:
            Test->TestTrue(TEXT("No hint has been shown in a fresh test profile"), Tutorial->GetShown().IsEmpty() && !Tutorial->IsShowing());
            Test->TestFalse(TEXT("A hint the action field does not carry is refused"), Tutorial->ShowHint(TEXT("first_directive")));
            // Beyond their aggro, so they keep still and no status interrupts the burn hint.
            Combat->SpawnWave(2, Pawn->GetActorLocation(), 1000.f);
            ++Phase; Mark = Frame; break;
        case 1:
            if (Frame < Mark + 30) break;
            Test->TestEqual(TEXT("The first foes raise the first-battle hint"), Tutorial->GetCurrent(), FString(TEXT("first_battle")));
            Test->TestTrue(TEXT("It is painted"), PC->GetHintWidget() && PC->GetHintWidget()->GetLines().Num() >= 2);
            Capture(TEXT("HintBattle"));
            ++Phase; Mark = Frame; break;
        case 2:
            if (Frame < Mark + 6) break;
            // A dash lets the hint go, and still dashes.
            Press(EKeys::LeftShift);
            Test->TestTrue(TEXT("The key is not swallowed"), Combat->IsDashing());
            Test->TestTrue(TEXT("The hint leaves"), Tutorial->IsLeaving());
            ++Phase; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 30) break;
            Test->TestFalse(TEXT("The hint is gone"), Tutorial->IsShowing());
            // The first burn: the hand reaching, one confirm.
            Press(EKeys::R);
            if (const int32 Hand = Combat->GetBurnChoices().IndexOfByPredicate([](const FMemoriaBurnChoice& C) { return C.Id == TEXT("rel_hand_reaching"); }); Hand >= 0) Combat->SelectBurn(Hand);
            Press(EKeys::Enter);
            ++Phase; Mark = Frame; break;
        case 4:
            if (Frame < Mark + 40) break;
            Test->TestEqual(TEXT("The first burn raises its hint"), Tutorial->GetCurrent(), FString(TEXT("first_burn")));
            Test->TestTrue(TEXT("In Korean, in the source's words"), PC->GetHintWidget() && FString::Join(PC->GetHintWidget()->GetLines(), TEXT(" ")) == UMemoriaTutorialSubsystem::Text(TEXT("first_burn"), true));
            Capture(TEXT("HintBurn"));
            ++Phase; Mark = Frame; break;
        case 5:
            // Four seconds, then it leaves on its own.
            if (Frame < Mark + int32((UMemoriaTutorialSubsystem::HoldSeconds + UMemoriaTutorialSubsystem::OutSeconds) * 60.f) + 5) break;
            Test->TestFalse(TEXT("The hint leaves after four seconds"), Tutorial->IsShowing());
            Combat->SpawnWave(1, Pawn->GetActorLocation(), 1000.f);
            ++Phase; Mark = Frame; break;
        case 6:
            if (Frame < Mark + 20) break;
            Test->TestFalse(TEXT("A shown hint never returns"), Tutorial->IsShowing());
            Test->TestEqual(TEXT("The first-battle hint showed once"), Count(TEXT("hint:first_battle")), 1);
            Test->TestEqual(TEXT("The burn hint showed once"), Count(TEXT("hint:first_burn")), 1);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialHintsTest, "MemoriaVisual.TutorialHints", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialHintsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FTutorialHintsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
