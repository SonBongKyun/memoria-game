#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Framework/MemoriaPlayerController.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCoordinates.h"
#include "Camera/CameraComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Blueprint/UserWidget.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedPlayerInput.h"
#include "InputKeyEventArgs.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FMemoriaInputProbe final : public IAutomationLatentCommand
{
public:
    explicit FMemoriaInputProbe(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    virtual ~FMemoriaInputProbe() override { if (bStarted) { FApp::SetUseFixedTimeStep(bWasFixed); FApp::SetFixedDeltaTime(PreviousDelta); } }
    virtual bool Update() override
    {
        // Automation may poll repeatedly within one engine frame. Each trace
        // step must correspond to an actual input/world tick.
        if (LastEngineFrame == GFrameCounter) { return false; }
        LastEngineFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 60) { Test->AddError(TEXT("PIE input probe timed out")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaPlayerController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!PC || !Pawn || !PC->PlayerInput || !Pawn->HasActorBegunPlay() || World->GetTimeSeconds() < 0.5) { return false; }
        auto* Enhanced = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
        if (!Enhanced || Enhanced->GetEnhancedActionMappingsView().Num() == 0) { return false; }
        auto Key = [&](FKey K, EInputEvent Event, float Value = 1.0f)
        { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Value)); };
        if (!bStarted)
        {
            bStarted = true; bWasFixed = FApp::UseFixedTimeStep(); PreviousDelta = FApp::GetFixedDeltaTime();
            FApp::SetFixedDeltaTime(1.0 / 60.0); FApp::SetUseFixedTimeStep(true);
            Source = FVector2D(137.25, -83.5); Origin = Memoria::Coordinates::FromSource(Source);
            Pawn->SetActorLocation(Origin, false);
            Test->TestTrue(TEXT("Nonzero source restore: (137.25,-83.5) -> (137.25,83.5,0)"), Pawn->GetActorLocation().Equals(FVector(137.25, 83.5, 0), 0.001));
            Test->TestTrue(TEXT("Source inverse"), Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(Source, 0.001));
            auto* Camera = Pawn->GetFieldCamera(); auto* Component = Pawn->GetFieldSprite();
            Test->TestTrue(TEXT("Orthographic projection"), Camera->ProjectionMode == ECameraProjectionMode::Orthographic);
            Test->TestTrue(TEXT("Camera points -Z"), Camera->GetForwardVector().Equals(-FVector::UpVector, 0.0001));
            Test->TestTrue(TEXT("Camera above movement origin"), Camera->GetComponentLocation().Z > Pawn->GetActorLocation().Z);
            Test->TestTrue(TEXT("Sprite plane normal parallel to camera axis"), FMath::Abs(FVector::DotProduct(Component->GetRightVector(), Camera->GetForwardVector())) > 0.9999);
            Test->TestTrue(TEXT("Sprite top points world +Y"), Component->GetUpVector().Equals(FVector::RightVector, 0.0001));
            auto* Sprite = Component->GetSprite();
            if (!Test->TestNotNull(TEXT("Saved Paper2D sprite loaded"), Sprite)) { return true; }
            Test->TestTrue(TEXT("Bottom center pivot (16,48)"), Sprite->GetPivotPosition().Equals(FVector2D(16, 48), 0.001));
            Test->TestEqual(TEXT("One source pixel per world unit"), Sprite->GetPixelsPerUnrealUnit(), 1.0f);
            const FBox Bounds = Sprite->GetRenderBounds().TransformBy(Component->GetComponentTransform()).GetBox();
            FVector2D ScreenOrigin, ScreenX, ScreenY;
            PC->ProjectWorldLocationToScreen(Origin, ScreenOrigin);
            PC->ProjectWorldLocationToScreen(Origin + FVector(32, 0, 0), ScreenX);
            PC->ProjectWorldLocationToScreen(Origin + FVector(0, 32, 0), ScreenY);
            Test->TestTrue(TEXT("World +X projects screen right"), ScreenX.X > ScreenOrigin.X && FMath::IsNearlyEqual(ScreenX.Y, ScreenOrigin.Y, 0.01));
            Test->TestTrue(TEXT("World +Y projects screen up"), ScreenY.Y < ScreenOrigin.Y && FMath::IsNearlyEqual(ScreenY.X, ScreenOrigin.X, 0.01));
            FVector RayOrigin, RayDirection;
            Test->TestTrue(TEXT("Screen deprojection available"), PC->DeprojectScreenPositionToWorld(ScreenX.X, ScreenX.Y, RayOrigin, RayDirection));
            const FVector Recovered = FMath::LinePlaneIntersection(RayOrigin, RayOrigin + RayDirection * 2000.0, FVector::ZeroVector, FVector::UpVector);
            Test->AddInfo(FString::Printf(TEXT("DEPROJECTION_TRACE expected=%s recovered=%s ray_origin=%s ray_direction=%s"), *(Origin + FVector(32, 0, 0)).ToString(), *Recovered.ToString(), *RayOrigin.ToString(), *RayDirection.ToString()));
            // Installed UE5.8 SceneView.cpp truncates screen coordinates to an
            // integer pixel before deprojection. Preserve that documented pixel
            // behavior; world/source restoration above still requires 0.001 units.
            FVector2D Reprojected; PC->ProjectWorldLocationToScreen(Recovered, Reprojected);
            const FVector2D Pixel(FMath::TruncToDouble(ScreenX.X), FMath::TruncToDouble(ScreenX.Y));
            Test->TestTrue(TEXT("Deprojected ray returns to the exact selected screen pixel"), Reprojected.Equals(Pixel, 0.01));
            const FVector Error = Recovered - (Origin + FVector(32, 0, 0));
            Test->TestTrue(TEXT("Deprojection error bounded by one pixel in world units"),
                FMath::Abs(Error.X) <= 32.0 / (ScreenX.X - ScreenOrigin.X) + 0.001 &&
                FMath::Abs(Error.Y) <= 32.0 / (ScreenOrigin.Y - ScreenY.Y) + 0.001 && FMath::Abs(Error.Z) < 0.001);
            Test->AddInfo(FString::Printf(TEXT("CAMERA_AXIS forward=%s right=%s up=%s sprite_up=%s bounds=%s screen_origin=%s screen_x=%s screen_y=%s"),
                *Camera->GetForwardVector().ToString(), *Camera->GetRightVector().ToString(), *Camera->GetUpVector().ToString(), *Component->GetUpVector().ToString(),
                *Bounds.ToString(), *ScreenOrigin.ToString(), *ScreenX.ToString(), *ScreenY.ToString()));
            Test->TestTrue(TEXT("Rendered sprite rises from foot origin"), Bounds.Min.Y >= Origin.Y - 0.01 && Bounds.Max.Y > Origin.Y + 40.0);
            Test->TestTrue(TEXT("Sprite foot and movement origin coincide"), Component->GetComponentLocation().Equals(Origin, 0.001));
        }
        switch (Frame)
        {
        case 4:
            Test->TestTrue(TEXT("Restored coordinate persists before input"), Pawn->GetActorLocation().Equals(Origin, 0.001));
            break;
        case 5: Key(EKeys::D, IE_Pressed); break;
        case 25:
            Test->AddInfo(FString::Printf(TEXT("KEYBOARD_TRACE before=%s after=%s pressed=%d ignored=%d"), *Origin.ToString(), *Pawn->GetActorLocation().ToString(), PC->IsInputKeyDown(EKeys::D), PC->IsMoveInputIgnored()));
            Key(EKeys::D, IE_Released, 0);
            Test->TestTrue(TEXT("Keyboard moves +X through Enhanced Input"), Pawn->GetActorLocation().X > Origin.X + 1.0);
            Test->TestTrue(TEXT("Keyboard movement holds Z=0"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z, 0.001));
            break;
        case 40: BeforeStick = Pawn->GetActorLocation(); Key(EKeys::Gamepad_LeftY, IE_Axis, 0.8f); break;
        case 60:
            Key(EKeys::Gamepad_LeftY, IE_Axis, 0);
            Test->TestTrue(TEXT("Analog gamepad moves +Y"), Pawn->GetActorLocation().Y > BeforeStick.Y + 1.0);
            Test->TestTrue(TEXT("Analog movement holds Z=0"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z, 0.001));
            break;
        case 80: Key(EKeys::Escape, IE_Pressed); break;
        case 85:
            Test->TestTrue(TEXT("Escape opens real modal"), PC->IsModalOpen());
            if (PC->GetModal()) { Test->TestTrue(TEXT("Modal takes user focus"), PC->GetModal()->HasUserFocus(PC)); }
            ModalPosition = Pawn->GetActorLocation(); Key(EKeys::D, IE_Pressed); Key(EKeys::Escape, IE_Repeat); break;
        case 95:
            Test->TestEqual(TEXT("Opening press/hold only one transition"), PC->GetModalTransitionCount(), 1);
            Test->TestEqual(TEXT("Opening press is not also Back"), PC->GetBackDispatchCount(), 0);
            Test->TestTrue(TEXT("Modal blocks movement and residual velocity"), Pawn->GetActorLocation().Equals(ModalPosition, 0.001));
            Key(EKeys::D, IE_Released, 0); Key(EKeys::Escape, IE_Released, 0); break;
        case 100: Key(EKeys::Enter, IE_Pressed); break;
        case 103: Key(EKeys::Enter, IE_Repeat); break;
        case 106:
            Test->TestEqual(TEXT("Held confirm dispatches once"), PC->GetConfirmCount(), 1);
            Key(EKeys::Enter, IE_Released, 0);
            if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FoundationModal.png"), true, false); }
            break;
        case 115: Key(EKeys::Escape, IE_Pressed); break;
        case 120: Key(EKeys::Escape, IE_Repeat); break;
        case 130:
            Test->TestFalse(TEXT("Back closes modal"), PC->IsModalOpen());
            Test->TestEqual(TEXT("Physical Back press/hold causes one dispatch"), PC->GetBackDispatchCount(), 1);
            Test->TestEqual(TEXT("Held Back does not reopen menu"), PC->GetModalTransitionCount(), 2);
            Test->TestFalse(TEXT("Move input restored"), PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Focus returned to game viewport"), FSlateApplication::Get().GetUserFocusedWidget(0) == World->GetGameViewport()->GetGameViewportWidget());
            Key(EKeys::Escape, IE_Released, 0); break;
        case 140: Key(EKeys::Gamepad_Special_Right, IE_Pressed); break;
        case 145: Key(EKeys::Gamepad_Special_Right, IE_Released, 0); break;
        case 150:
            Test->TestTrue(TEXT("Gamepad menu opens modal"), PC->IsModalOpen());
            Key(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed); break;
        case 155: Key(EKeys::Gamepad_FaceButton_Bottom, IE_Released, 0); break;
        case 160:
            Test->TestEqual(TEXT("Gamepad confirm"), PC->GetConfirmCount(), 2);
            Key(EKeys::Gamepad_FaceButton_Right, IE_Pressed); break;
        case 165: Key(EKeys::Gamepad_FaceButton_Right, IE_Repeat); break;
        case 175:
            Test->TestFalse(TEXT("Gamepad Back closes"), PC->IsModalOpen());
            Test->TestEqual(TEXT("Gamepad held Back single dispatch"), PC->GetBackDispatchCount(), 2);
            Key(EKeys::Gamepad_FaceButton_Right, IE_Released, 0); break;
        case 185: BeforeResume = Pawn->GetActorLocation(); Key(EKeys::W, IE_Pressed); break;
        case 205:
            Key(EKeys::W, IE_Released, 0);
            Test->TestTrue(TEXT("Movement works after modal focus return"), Pawn->GetActorLocation().Y > BeforeResume.Y + 1.0);
            Test->TestTrue(TEXT("All movement remains on plane"), FMath::IsNearlyZero(Pawn->GetActorLocation().Z, 0.001)); break;
        case 225:
            if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
            { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FoundationSprite.png"), true, false); }
            break;
        case 240:
            Test->AddInfo(FString::Printf(TEXT("FOUNDATION_RUNTIME_PROBE_FINISHED frames=%d back=%d transitions=%d source=(137.25,-83.5)"), Frame, PC->GetBackDispatchCount(), PC->GetModalTransitionCount()));
            return true;
        }
        if (Frame > 40 && Frame < 60) { Key(EKeys::Gamepad_LeftY, IE_Axis, 0.8f); }
        ++Frame;
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started;
    double PreviousDelta = 0;
    bool bWasFixed = false;
    bool bStarted = false;
    int32 Frame = 0;
    uint64 LastEngineFrame = MAX_uint64;
    FVector2D Source;
    FVector Origin, BeforeStick, ModalPosition, BeforeResume;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaFoundationInputTest, "Memoria.Foundation.MapInputAndModal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaFoundationInputTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : {TEXT("IA_Move"), TEXT("IA_Confirm"), TEXT("IA_Back"), TEXT("IA_Menu")})
    {
        const FString Path = FString::Printf(TEXT("/Game/Tests/Foundation/%s.%s"), Name, Name);
        if (!TestNotNull(*Path, LoadObject<UInputAction>(nullptr, *Path))) { return false; }
    }
    for (const TCHAR* Name : {TEXT("IMC_Foundation"), TEXT("IMC_Modal")})
    {
        const FString Path = FString::Printf(TEXT("/Game/Tests/Foundation/%s.%s"), Name, Name);
        if (!TestNotNull(*Path, LoadObject<UInputMappingContext>(nullptr, *Path))) { return false; }
    }
    if (!AutomationOpenMap(TEXT("/Game/Tests/Foundation/L_FoundationTest"))) { return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FMemoriaInputProbe(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
