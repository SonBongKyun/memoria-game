#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaCombatInputRouter.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsS362,"MemoriaUpgrade.S362.AccessibilitySettings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSettingsS362::RunTest(const FString&)
{
    // A GameInstanceSubsystem requires a GameInstance Outer. Neither object
    // is initialized, so bPersist stays false and player settings stay untouched.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    TStrongObjectPtr<UMemoriaSettingsSubsystem> Settings(NewObject<UMemoriaSettingsSubsystem>(Game.Get()));
    TestEqual(TEXT("Default motion preserves the source presentation"),Settings->MotionScale(),1.f);
    TestEqual(TEXT("Default shake preserves the source presentation"),Settings->ShakeScale(),1.f);
    TestEqual(TEXT("Default flash preserves the source presentation"),Settings->FlashScale(),1.f);
    TestEqual(TEXT("Default text preserves the source presentation"),Settings->ScaledTextSize(18),18);
    int32 Changes=0; auto Handle=Settings->OnChanged.AddLambda([&Changes](){++Changes;});
    Settings->SetShakePercent(-999); TestEqual(TEXT("Shake clamps to zero"),Settings->GetShakePercent(),0);
    Settings->SetShakePercent(999); TestEqual(TEXT("Shake clamps to source maximum"),Settings->GetShakePercent(),100);
    Settings->SetTextPercent(999); TestEqual(TEXT("Text bounded at 125 percent"),Settings->GetTextPercent(),125);
    TestEqual(TEXT("Pause can request the actual scaled font size"),Settings->ScaledTextSize(20),25);
    Settings->SetTextPercent(-999); TestEqual(TEXT("Small text cannot shrink below source"),Settings->GetTextPercent(),100);
    Settings->SetReduceMotion(true); TestEqual(TEXT("Reduce Motion removes decorative motion"),Settings->MotionScale(),0.f);
    TestEqual(TEXT("Reduce Motion also removes camera displacement"),Settings->ShakeScale(),0.f);
    Settings->SetReduceFlash(true); TestEqual(TEXT("Reduced flash retains a bounded readable cue"),Settings->FlashScale(),.15f);
    TestEqual(TEXT("Every change notifies consumers"),Changes,6);
    Settings->OnChanged.Remove(Handle);
    auto* Menu=NewObject<UMemoriaPauseWidget>();
    Menu->Configure(Settings.Get(),true,true,TEXT("Chapter 3 — Belt Waystation"));
    Menu->Select(1); Menu->Confirm(); TestTrue(TEXT("Options opens"),Menu->IsOptionsOpen());
    Menu->Navigate(5); Menu->Confirm(); TestFalse(TEXT("Motion row edits the actual setting"),Settings->IsReduceMotion());
    Menu->Navigate(1); Menu->Confirm(); TestFalse(TEXT("Flash row edits the actual setting"),Settings->IsReduceFlash());
    Menu->Navigate(2); Menu->Adjust(1); TestEqual(TEXT("Text row edits the actual setting"),Settings->GetTextPercent(),105);
    Menu->Navigate(1); Menu->Confirm(); TestFalse(TEXT("Last row returns to pause"),Menu->IsOptionsOpen());
    for (int32 I=0;I<4;++I)
    {
        TestEqual(TEXT("LT maps all four Elia techniques"),UMemoriaCombatInputRouter::LayeredSlot(I,true,false),I);
        TestEqual(TEXT("RT maps all four ordinary items"),UMemoriaCombatInputRouter::LayeredSlot(I,false,true),I+4);
        TestEqual(TEXT("LT wins over a simultaneous item layer"),UMemoriaCombatInputRouter::LayeredSlot(I,true,true),I);
        TestEqual(TEXT("Unmodified pad cannot accidentally spend an item"),UMemoriaCombatInputRouter::LayeredSlot(I,false,false),INDEX_NONE);
    }
    return true;
}

namespace
{
class FPadS362Replay final : public IAutomationLatentCommand
{
public:
    explicit FPadS362Replay(FAutomationTestBase* InTest):Test(InTest),Started(FPlatformTime::Seconds()){}
    ~FPadS362Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame==GFrameCounter) return false; LastFrame=GFrameCounter;
        if (FPlatformTime::Seconds()-Started>120.) { Test->AddError(TEXT("Native gamepad replay timeout")); return true; }
        UWorld* World=GEditor->PlayWorld.Get(); auto* C=World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn=C ? Cast<AMemoriaFieldPawn>(C->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds()<.3f) return false;
        auto* Host=World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed=true; bOldFixed=FApp::UseFixedTimeStep(); OldDelta=FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1./60.);
            for (int32 I=0;I<20 && Host->GetState()==EMemoriaSliceState::VN;++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState()!=EMemoriaSliceState::Exploration) return false;
        auto* B=World->GetSubsystem<UMemoriaFieldCombatSubsystem>(); auto* Router=C->GetCombatInputRouter();
        if (!B || !B->GetPlayer()) return false;
        if (!Test->TestNotNull(TEXT("The actual controller owns the Enhanced Input router"),Router)) return true;
        if (++Frame%8) return false;
        auto Key=[&](FKey K,EInputEvent E,float V=1.f){C->InputKey(FInputKeyEventArgs::CreateSimulated(K,E,V));};
        auto Press=[&](FKey K){Key(K,IE_Pressed);}; auto Release=[&](FKey K){Key(K,IE_Released,0.f);};
        auto Tap=[&](FKey K){Press(K); Release(K);};
        switch (Step++)
        {
        case 0: Test->TestEqual(TEXT("Runtime context maps eleven buttons and two aim axes"),Router->GetMappingCount(),13); Test->TestFalse(TEXT("No-pad entry preserves the original pointer/facing route"),Router->HasGamepadAim()); Test->TestFalse(TEXT("Hints default to keyboard"),Router->IsUsingGamepad()); Press(EKeys::Gamepad_LeftShoulder); break;
        case 1: Test->TestTrue(TEXT("Physical LB raises guard through Enhanced Input"),B->IsBlocking()); Test->TestTrue(TEXT("Real gamepad press selects pad hints"),Router->IsUsingGamepad()); Tap(EKeys::Escape); Release(EKeys::Gamepad_LeftShoulder); break;
        case 2: Test->TestTrue(TEXT("Pause is the active modal"),World->IsPaused()); Test->TestFalse(TEXT("Release while paused clears guard"),B->IsBlocking()); Test->TestFalse(TEXT("A pad release cannot steal the keyboard pause gesture"),Router->IsUsingGamepad()); Tap(EKeys::Escape); break;
        case 3: Press(EKeys::Gamepad_LeftShoulder); break;
        case 4: Test->TestTrue(TEXT("The next physical LB gesture rearms normally"),B->IsBlocking()); Release(EKeys::Gamepad_LeftShoulder); break;
        case 5: Press(EKeys::Gamepad_RightShoulder); break;
        case 6: Test->TestTrue(TEXT("Physical RB really holds attack"),Router->IsHoldingAttack()); Test->TestTrue(TEXT("Physical RB accumulated charge"),B->GetCharge()>0.f); Tap(EKeys::Escape); Release(EKeys::Gamepad_RightShoulder); break;
        case 7: Test->TestEqual(TEXT("Paused RB release leaves no charge"),B->GetCharge(),0.f); Tap(EKeys::Escape); break;
        case 8: Key(EKeys::Gamepad_RightX,IE_Axis,.9f); Key(EKeys::Gamepad_RightY,IE_Axis,0.f); break;
        case 9:
        {
            const FVector Aim=Router->AimPoint()-Pawn->GetActorLocation();
            Test->TestTrue(TEXT("Actual right-stick X aims world +X"),Aim.X>990.f && FMath::Abs(Aim.Y)<1.f);
            Test->TestTrue(TEXT("Physical stick motion selects pad hints independently"),Router->IsUsingGamepad());
            FVector2D Origin,Target; C->ProjectWorldLocationToScreen(Pawn->GetActorLocation(),Origin); C->ProjectWorldLocationToScreen(Router->AimPoint(),Target);
            Test->TestTrue(TEXT("Actual +X aim projects screen right"),Target.X>Origin.X);
            Key(EKeys::Gamepad_RightX,IE_Axis,0.f); Key(EKeys::Gamepad_RightY,IE_Axis,.9f); break;
        }
        case 10:
        {
            const FVector Aim=Router->AimPoint()-Pawn->GetActorLocation();
            Test->TestTrue(TEXT("Actual right-stick Y aims world +Y"),Aim.Y>990.f && FMath::Abs(Aim.X)<1.f);
            FVector2D Origin,Target; C->ProjectWorldLocationToScreen(Pawn->GetActorLocation(),Origin); C->ProjectWorldLocationToScreen(Router->AimPoint(),Target);
            Test->TestTrue(TEXT("Actual +Y aim projects screen up"),Target.Y<Origin.Y);
            Key(EKeys::Gamepad_RightY,IE_Axis,0.f); Press(EKeys::Gamepad_FaceButton_Top); break;
        }
        case 11: Test->TestTrue(TEXT("Physical Y opens the burn picker"),B->IsPickingBurn()); Test->TestTrue(TEXT("Picker uses the real pad preference"),Router->IsUsingGamepad()); FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/S362Controls/PadPickerHints.png"),true,false); Release(EKeys::Gamepad_FaceButton_Top); OldSelection=B->GetBurnSelection(); Tap(EKeys::Gamepad_DPad_Down); break;
        case 12: if (B->GetBurnChoices().Num()>1) Test->TestTrue(TEXT("Physical D-pad navigates the picker"),B->GetBurnSelection()!=OldSelection); Press(EKeys::Gamepad_FaceButton_Right); break;
        case 13: Test->TestFalse(TEXT("B cancels the picker"),B->IsPickingBurn()); Test->TestFalse(TEXT("Picker B did not also dodge"),B->IsDashing()); Release(EKeys::Gamepad_FaceButton_Right); break;
        case 14: Press(EKeys::Gamepad_FaceButton_Right); break;
        case 15: Test->TestTrue(TEXT("A new physical B gesture dodges in Exploration"),B->IsDashing()); Release(EKeys::Gamepad_FaceButton_Right); break;
        case 16: break; // let the committed .32s dodge finish before testing keyboard guard
        case 17: Press(EKeys::K); break;
        case 18: Test->TestTrue(TEXT("Existing keyboard guard remains live"),B->IsBlocking()); Test->TestFalse(TEXT("Physical keyboard press restores keyboard hints"),Router->IsUsingGamepad()); Release(EKeys::K); break;
        case 19: Key(EKeys::Gamepad_RightX,IE_Axis,.9f); Key(EKeys::Gamepad_RightY,IE_Axis,0.f); break;
        case 20: Press(EKeys::Gamepad_RightShoulder); break;
        case 21:
            Test->TestTrue(TEXT("The actual attacking figure faces the +X pad aim"),B->GetPlayerFigure() && FMath::Abs(FMath::FindDeltaAngleDegrees(B->GetPlayerFigure()->GetYaw(),0.f))<1.f);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/S362Controls/PadAimX.png"),true,false);
            Release(EKeys::Gamepad_RightShoulder); break;
        case 22: case 23: case 24: case 25: break;
        case 26: Key(EKeys::Gamepad_RightX,IE_Axis,0.f); Key(EKeys::Gamepad_RightY,IE_Axis,.9f); break;
        case 27: Press(EKeys::Gamepad_RightShoulder); break;
        case 28:
            Test->TestTrue(TEXT("The actual attacking figure faces the +Y pad aim"),B->GetPlayerFigure() && FMath::Abs(FMath::FindDeltaAngleDegrees(B->GetPlayerFigure()->GetYaw(),90.f))<1.f);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/S362Controls/PadAimY.png"),true,false);
            Release(EKeys::Gamepad_RightShoulder); Key(EKeys::Gamepad_RightY,IE_Axis,0.f); break;
        case 29: case 30: case 31: case 32: break;
        case 33: MovementOrigin=Pawn->GetActorLocation(); Key(EKeys::Gamepad_LeftX,IE_Axis,.8f); break;
        case 34:
            Test->TestTrue(TEXT("The original physical left-stick route visibly moves the pawn right"),Pawn->GetActorLocation().X>MovementOrigin.X+1.f);
            Test->TestTrue(TEXT("Original left-stick motion also selects pad hints"),Router->IsUsingGamepad());
            Key(EKeys::Gamepad_LeftX,IE_Axis,0.f); break;
        case 35:
            Press(EKeys::MiddleMouseButton); Release(EKeys::MiddleMouseButton);
            Test->TestFalse(TEXT("A physical mouse press restores keyboard/mouse hints"),Router->IsUsingGamepad());
            Press(EKeys::K); break;
        case 36:
            Test->TestTrue(TEXT("Actual guard keeps the keyboard command ribbon visible for capture"),B->IsBlocking());
            Test->TestFalse(TEXT("Neutral pad axes do not steal the keyboard/mouse preference"),Router->IsUsingGamepad());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/S362Controls/KeyboardHints.png"),true,false); break;
        case 37: Release(EKeys::K); return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test; double Started,OldDelta=0.; uint64 LastFrame=MAX_uint64;
    int32 Step=0,Frame=0,OldSelection=0; bool bFixed=false,bOldFixed=false;
    FVector MovementOrigin=FVector::ZeroVector;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPadS362,"MemoriaUpgrade.S362.NativeGamepad",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPadS362::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FPadS362Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
