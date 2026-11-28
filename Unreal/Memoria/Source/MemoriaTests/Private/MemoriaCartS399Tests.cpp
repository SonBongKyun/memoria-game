#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Scene.h"
#include "EngineUtils.h"
#include "ReferenceSkeleton.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Interaction/MemoriaCart.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
void QuietBelt(UMemoriaRunSubsystem* Run, const FMemoriaChapterMapSpec* Spec)
{
    if (!Run || !Spec) return;
    const FString Closed = Spec->Exit.Completes;
    auto Set = [&](const FString& Flag)
    {
        if (!Flag.IsEmpty() && !Flag.Equals(Closed)) Run->SetStoryFlag(Flag, true);
    };
    for (const auto& Step : Spec->Sequence)
    {
        Set(Step.Flag);
        for (const FString& Flag : Step.Flags) Set(Flag);
    }
    for (const auto& Chest : Spec->Chests) Set(Chest.Flag);
    for (const auto& Clue : Spec->Clues) Set(Clue.Flag);
    for (const auto& Trigger : Spec->Triggers) Set(Trigger.Flag);
    for (int32 Index = 0; Index < Spec->Battles.Num(); ++Index)
        Set(FString::Printf(TEXT("battle_%s_%d"), *Spec->Map, Index + 1));
}
AMemoriaEliaCompanion* FindElia(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
AMemoriaTobiasCompanion* FindTobias(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
UMemoriaFieldCharacterComponent* FindArrel(const APawn* Pawn)
{
    if (!Pawn || !Pawn->GetRootComponent()) return nullptr;
    TArray<USceneComponent*> Children;
    Pawn->GetRootComponent()->GetChildrenComponents(true, Children);
    for (USceneComponent* Child : Children)
        if (auto* Figure = Cast<UMemoriaFieldCharacterComponent>(Child))
            if (Figure->GetCharacterId().Equals(TEXT("Arrel"), ESearchCase::IgnoreCase))
                return Figure;
    return nullptr;
}
USceneComponent* Named(const AActor* Actor, const TCHAR* Name)
{
    if (!Actor) return nullptr;
    TArray<USceneComponent*> Parts;
    Actor->GetComponents<USceneComponent>(Parts);
    for (USceneComponent* Part : Parts) if (Part && Part->GetName().Equals(Name)) return Part;
    return nullptr;
}
bool SavedLook(const UStaticMeshComponent* Mesh, const TCHAR* Asset)
{
    const UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(0) : nullptr;
    if (!Material || !Material->GetName().Contains(Asset)) return false;
    if (!Material->GetPathName().Contains(TEXT("/Game/Memoria/Presentation/FieldLooks/"))) return false;
    const UMaterial* Source = Material->GetMaterial();
    if (!Source) return false;
#if WITH_EDITOR
    const UMaterialEditorOnlyData* Editor = Source->GetEditorOnlyData();
    if (!Editor || Editor->EmissiveColor.Expression != nullptr) return false;
    return Cast<UMaterialExpressionConstant3Vector>(Editor->BaseColor.Expression) != nullptr;
#else
    return true;
#endif
}
bool ForbiddenProp(const UStaticMeshComponent* Mesh)
{
    if (!Mesh) return false;
    const FString Name = Mesh->GetName() + (Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : FString());
    return Name.Contains(TEXT("Sword")) || Name.Contains(TEXT("Blade")) || Name.Contains(TEXT("Lantern"))
        || Name.Contains(TEXT("Staff")) || Name.Contains(TEXT("Weapon"));
}
bool LegsFolded(const UMemoriaFieldCharacterComponent* Figure)
{
    const USkeletalMeshComponent* Skel = Figure ? Figure->GetSkeletalMesh() : nullptr;
    const USkeletalMesh* Mesh = Skel ? Skel->GetSkeletalMeshAsset() : nullptr;
    if (!Mesh) return false;
    const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
    const TArray<FTransform>& Bones = Skel->GetComponentSpaceTransforms();
    auto At = [&](const TCHAR* Name) -> FVector
    {
        const int32 Index = Ref.FindBoneIndex(FName(Name));
        return Bones.IsValidIndex(Index) ? Bones[Index].GetLocation() : FVector(1.e9f, 0.f, 0.f);
    };
    const FVector Thigh = At(TEXT("thigh_l"));
    const FVector Calf = At(TEXT("calf_l"));
    if (Thigh.X > 1.e8f || Calf.X > 1.e8f) return false;
    return (Calf.X - Thigh.X) > 10.f && (Thigh.Z - Calf.Z) < 25.f;
}
ACameraActor* FrameAt(UWorld* World, APlayerController* PC, const FVector& Focus, const FVector& Eye, float Fov, ACameraActor* Camera)
{
    if (!World || !PC) return Camera;
    if (!Camera)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Camera = World->SpawnActor<ACameraActor>(Eye, (Focus - Eye).Rotation(), Params);
        if (UCameraComponent* Cam = Camera ? Camera->GetCameraComponent() : nullptr)
        {
            Cam->PostProcessBlendWeight = 1.f;
            FPostProcessSettings& PP = Cam->PostProcessSettings;
            PP.bOverride_AutoExposureMethod = true;
            PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
            PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
            PP.AutoExposureApplyPhysicalCameraExposure = false;
            PP.bOverride_AutoExposureBias = true;
            PP.AutoExposureBias = 0.35f;
        }
    }
    if (!Camera) return nullptr;
    if (UCameraComponent* Cam = Camera->GetCameraComponent()) Cam->SetFieldOfView(Fov);
    Camera->SetActorLocation(Eye);
    Camera->SetActorRotation((Focus - Eye).Rotation());
    PC->SetViewTargetWithBlend(Camera, 0.f);
    return Camera;
}
uint32 BitmapHash(const TArray<FColor>& Bitmap)
{
    uint32 Hash = 2166136261u;
    for (const FColor& Pixel : Bitmap)
    {
        Hash ^= uint32(Pixel.R) | (uint32(Pixel.G) << 8) | (uint32(Pixel.B) << 16);
        Hash *= 16777619u;
    }
    return Hash;
}
bool CaptureCurrentView(UWorld* World, TArray<FColor>& Bitmap, FIntPoint& Size, FString& Error)
{
    Size = FIntPoint(1280, 720);
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    AActor* View = PC ? PC->GetViewTarget() : nullptr;
    UCameraComponent* Cam = View ? View->FindComponentByClass<UCameraComponent>() : nullptr;
    if (!View || !Cam)
    {
        Error = TEXT("No view camera");
        return false;
    }
    UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
    Target->ClearColor = FLinearColor::Black;
    Target->InitCustomFormat(Size.X, Size.Y, PF_B8G8R8A8, false);
    Target->UpdateResourceImmediate(true);
    USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(View);
    Capture->TextureTarget = Target;
    Capture->CaptureSource = SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = true;
    Capture->bExcludeFromSceneTextureExtents = true;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    Capture->FOVAngle = Cam->FieldOfView;
    Capture->PostProcessBlendWeight = 1.f;
    FPostProcessSettings& PP = Capture->PostProcessSettings;
    PP.bOverride_AutoExposureMethod = true;
    PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    PP.AutoExposureApplyPhysicalCameraExposure = false;
    PP.bOverride_AutoExposureBias = true;
    PP.AutoExposureBias = 0.35f;
    Capture->RegisterComponentWithWorld(World);
    Capture->SetWorldLocationAndRotation(Cam->GetComponentLocation(), Cam->GetComponentRotation());
    Capture->CaptureScene();
    FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
    const bool bRead = Resource && Resource->ReadPixels(Bitmap);
    Capture->UnregisterComponent();
    Capture->DestroyComponent();
    Target->ReleaseResource();
    if (!bRead || Bitmap.Num() != Size.X * Size.Y)
    {
        Error = FString::Printf(TEXT("Scene capture read %d pixels"), Bitmap.Num());
        return false;
    }
    return true;
}
void Shot(UWorld* World, const TCHAR* Name, FAutomationTestBase* Test)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Validation/S399Cart");
    IFileManager::Get().MakeDirectory(*Dir, true);
    const FString Path = Dir / Name;
    TArray<FColor> Bitmap;
    FIntPoint Size;
    FString Error;
    if (!CaptureCurrentView(World, Bitmap, Size, Error))
    {
        Test->AddError(FString::Printf(TEXT("%s: %s"), Name, *Error));
        return;
    }
    int64 Luma = 0;
    int32 Samples = 0;
    const int32 Stride = FMath::Max(1, Bitmap.Num() / 4096);
    for (int32 Index = 0; Index < Bitmap.Num(); Index += Stride)
    {
        const FColor& Pixel = Bitmap[Index];
        Luma += int32(Pixel.R) + int32(Pixel.G) + int32(Pixel.B);
        ++Samples;
    }
    const int64 Average = Samples > 0 ? Luma / Samples : 0;
    if (Average < 18)
    {
        Test->AddError(FString::Printf(TEXT("%s is nearly black (%dx%d, luma %lld)"), Name, Size.X, Size.Y, Average));
        return;
    }
    static TArray<uint32> Seen;
    if (FCString::Strcmp(Name, TEXT("Standing.png")) == 0) Seen.Reset();
    const uint32 Hash = BitmapHash(Bitmap);
    if (Seen.Contains(Hash))
        Test->AddError(FString::Printf(TEXT("%s repeats an earlier capture (hash %08x)"), Name, Hash));
    else
        Seen.Add(Hash);
    const FImageView View(Bitmap.GetData(), Size.X, Size.Y);
    if (!FImageUtils::SaveImageByExtension(*Path, View))
        Test->AddError(FString::Printf(TEXT("Could not save %s"), *Path));
    else
        Test->AddInfo(FString::Printf(TEXT("Captured %s %dx%d luma %lld hash %08x"), Name, Size.X, Size.Y, Average, Hash));
}
class FCartReplay final : public IAutomationLatentCommand
{
public:
    explicit FCartReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FCartReplay() override
    {
        if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
        {
            for (TActorIterator<AMemoriaCart> It(World); It; ++It)
                if (It->IsRiding()) It->GetDown();
            if (UGameInstance* Game = World->GetGameInstance())
                if (auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>())
                    Run->SetStoryFlag(TEXT("party_tobias"), false);
        }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.0)
        {
            Test->AddError(FString::Printf(TEXT("Cart timeout at step %d"), Step));
            return true;
        }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
        }
        ++Frame;
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr;
        for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const FMemoriaChapterMapSpec* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Run || !Spec) return false;
        switch (Step)
        {
        case 0:
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) break;
            Test->TestNotNull(TEXT("Elia is already with Arrel"), FindElia(World));
            QuietBelt(Run, Spec);
            Test->TestFalse(TEXT("The chapter stays open"), Run->GetRunSnapshot().GetFlag(Spec->Exit.Completes));
            Run->SetStoryFlag(TEXT("party_tobias"), true);
            From = World;
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step; break;
        case 1:
        {
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation")) || !FindElia(World) || !FindTobias(World)) break;
            const FVector Origin = Pawn->GetActorLocation() + FVector(160.f, 220.f, 0.f);
            Mid = Origin + FVector(200.f, 0.f, 0.f);
            End = Origin + FVector(420.f, 0.f, 0.f);
            Cart = AMemoriaCart::SpawnOnRoute(World, {Origin, Mid, End});
            Test->TestNotNull(TEXT("Cart spawns on the route"), Cart.Get());
            if (!Cart.IsValid()) return true;
            Test->TestFalse(TEXT("It does not drive before Board"), Cart->IsDriving());
            Test->TestFalse(TEXT("It has not arrived before Board"), Cart->HasArrived());
            auto Mesh = [&](const TCHAR* Name) { return Cast<UStaticMeshComponent>(Named(Cart.Get(), Name)); };
            Test->TestNotNull(TEXT("Uncovered bed"), Mesh(TEXT("CartBed")));
            Test->TestNotNull(TEXT("Low left side"), Mesh(TEXT("CartSideL")));
            Test->TestNotNull(TEXT("Low right side"), Mesh(TEXT("CartSideR")));
            Test->TestNotNull(TEXT("Left wheel"), Mesh(TEXT("CartWheelL")));
            Test->TestNotNull(TEXT("Right wheel"), Mesh(TEXT("CartWheelR")));
            Test->TestNotNull(TEXT("One horse"), Mesh(TEXT("CartHorseBody")));
            Test->TestNotNull(TEXT("Hooded driver"), Mesh(TEXT("DriverHood")));
            if (UStaticMeshComponent* Side = Mesh(TEXT("CartSideL")))
                Test->TestTrue(TEXT("Sides stay low"), Side->GetRelativeScale3D().Z > 0.15f && Side->GetRelativeScale3D().Z < 0.30f);
            if (UStaticMeshComponent* Wheel = Mesh(TEXT("CartWheelL")))
                Test->TestTrue(TEXT("Wheels are cylinders"), Wheel->GetStaticMesh() && Wheel->GetStaticMesh()->GetName().Contains(TEXT("Cylinder")));
            Test->TestTrue(TEXT("Bed uses the saved wood"), SavedLook(Mesh(TEXT("CartBed")), TEXT("M_CartWood")));
            Test->TestTrue(TEXT("Horse uses the saved coat"), SavedLook(Mesh(TEXT("CartHorseBody")), TEXT("M_CartHorse")));
            Test->TestTrue(TEXT("Hood uses the saved cloth"), SavedLook(Mesh(TEXT("DriverHood")), TEXT("M_CartHood")));
            TArray<UPointLightComponent*> Lights;
            Cart->GetComponents<UPointLightComponent>(Lights);
            Test->TestEqual(TEXT("No lantern"), Lights.Num(), 0);
            TArray<UStaticMeshComponent*> Meshes;
            Cart->GetComponents<UStaticMeshComponent>(Meshes);
            bool bWeapon = false;
            for (UStaticMeshComponent* Part : Meshes) if (ForbiddenProp(Part)) bWeapon = true;
            Test->TestFalse(TEXT("No weapon on the cart or the driver"), bWeapon);
            const FVector Focus = Cart->GetActorLocation() + FVector(-20.f, 0.f, 110.f);
            Camera = FrameAt(World, PC, Focus, Focus + FVector(420.f, -980.f, 360.f), 42.f, Camera.Get());
            ++Step; Mark = Frame; break;
        }
        case 2:
        {
            if (Frame < Mark + 2 || !Cart.IsValid()) break;
            Shot(World, TEXT("Standing.png"), Test);
            Test->TestTrue(TEXT("Board seats the three"), Cart->Board());
            Test->TestTrue(TEXT("They are riding"), Cart->IsRiding());
            Test->TestTrue(TEXT("The cart is driving"), Cart->IsDriving());
            Test->TestTrue(TEXT("Move input is held"), PC->IsMoveInputIgnored());
            UMemoriaFieldCharacterComponent* Arrel = FindArrel(Pawn);
            AMemoriaEliaCompanion* Elia = FindElia(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            Test->TestTrue(TEXT("Arrel sits"), Arrel && Arrel->IsSeated());
            Test->TestTrue(TEXT("Elia sits"), Elia && Elia->GetFigure() && Elia->GetFigure()->IsSeated());
            Test->TestTrue(TEXT("Tobias sits"), Tobias && Tobias->GetFigure() && Tobias->GetFigure()->IsSeated());
            auto NearSeat = [&](const AActor* Actor, const TCHAR* Seat)
            {
                const USceneComponent* Point = Named(Cart.Get(), Seat);
                return Actor && Point && FVector::Dist(Actor->GetActorLocation(), Point->GetComponentLocation()) < 25.f;
            };
            Test->TestTrue(TEXT("Arrel is on his seat"), NearSeat(Pawn, TEXT("SeatArrel")));
            Test->TestTrue(TEXT("Elia is on her seat"), NearSeat(Elia, TEXT("SeatElia")));
            Test->TestTrue(TEXT("Tobias is on his seat"), NearSeat(Tobias, TEXT("SeatTobias")));
            if (Arrel && Arrel->GetSkeletalMesh())
                Test->TestTrue(TEXT("Arrel's hips drop onto the bed"), Arrel->GetSkeletalMesh()->GetRelativeLocation().Z < -8.f - Arrel->GetWorldHeight() * 0.4f);
            ++Step; Mark = Frame; break;
        }
        case 3:
        {
            if (!Cart.IsValid()) return true;
            if (!bSawMid && Cart->IsDriving() && FVector::Dist2D(Cart->GetActorLocation(), Mid) < 40.f)
            {
                bSawMid = true;
                Test->TestTrue(TEXT("An intermediate point is not a stop"), Cart->IsDriving());
                Test->TestFalse(TEXT("The middle is not the arrival"), Cart->HasArrived());
            }
            if (Frame == Mark + 24)
            {
                const FVector Focus = Cart->GetActorLocation() + Cart->GetActorTransform().TransformVector(FVector(-80.f, 0.f, 80.f));
                Camera = FrameAt(World, PC, Focus, Focus + FVector(260.f, -620.f, 240.f), 40.f, Camera.Get());
            }
            if (Frame == Mark + 25)
            {
                Shot(World, TEXT("Riding.png"), Test);
                UMemoriaFieldCharacterComponent* Arrel = FindArrel(Pawn);
                AMemoriaEliaCompanion* Elia = FindElia(World);
                AMemoriaTobiasCompanion* Tobias = FindTobias(World);
                Test->TestTrue(TEXT("Arrel's legs are folded"), LegsFolded(Arrel));
                Test->TestTrue(TEXT("Elia's legs are folded"), Elia && LegsFolded(Elia->GetFigure()));
                Test->TestTrue(TEXT("Tobias's legs are folded"), Tobias && LegsFolded(Tobias->GetFigure()));
            }
            if (Cart.IsValid() && !Cart->HasArrived() && Frame < Mark + 450) break;
            Test->TestTrue(TEXT("The cart passed the middle while driving"), bSawMid);
            Test->TestTrue(TEXT("The cart arrived"), Cart->HasArrived());
            Test->TestFalse(TEXT("The cart stopped"), Cart->IsDriving());
            Test->TestEqual(TEXT("Route index is past the last point"), Cart->GetRouteIndex(), 3);
            Test->TestTrue(TEXT("The cart is on the last point"), FVector::Dist(Cart->GetActorLocation(), End) < 20.f);
            Test->TestTrue(TEXT("Input stays held until they get down"), PC->IsMoveInputIgnored());
            Cart->TurnDriverTowardBed();
            ++Step; Mark = Frame; break;
        }
        case 4:
        {
            if (!Cart.IsValid()) return true;
            if (Frame == Mark + 3)
            {
                const USceneComponent* Hood = Named(Cart.Get(), TEXT("DriverHood"));
                const FVector Focus = Hood ? Hood->GetComponentLocation() : Cart->GetActorLocation() + FVector(0, 0, 140);
                Camera = FrameAt(World, PC, Focus, Focus + FVector(40.f, -280.f, 70.f), 32.f, Camera.Get());
            }
            if (Frame < Mark + 4) break;
            Shot(World, TEXT("DriverTurned.png"), Test);
            Test->TestTrue(TEXT("The driver has turned toward the bed"), Cart->IsDriverTurnedTowardBed());
            if (const USceneComponent* Hood = Named(Cart.Get(), TEXT("DriverHood")))
            {
                const float Delta = FMath::FindDeltaAngleDegrees(Hood->GetComponentRotation().Yaw - Cart->GetActorRotation().Yaw, 180.f);
                Test->TestTrue(TEXT("Hood and shoulders face the bed"), FMath::Abs(Delta) < 15.f);
            }
            Cart->TurnDriverAhead();
            ++Step; Mark = Frame; break;
        }
        case 5:
            if (Frame < Mark + 2 || !Cart.IsValid()) break;
            Test->TestFalse(TEXT("The driver has turned back"), Cart->IsDriverTurnedTowardBed());
            Test->TestTrue(TEXT("His yaw is toward the horse"), FMath::Abs(FMath::FindDeltaAngleDegrees(Cart->GetDriverYaw(), 0.f)) < 15.f);
            Cart->GetDown();
            ++Step; Mark = Frame; break;
        case 6:
        {
            if (Frame < Mark + 2 || !Cart.IsValid()) break;
            Test->TestFalse(TEXT("They are down"), Cart->IsRiding());
            Test->TestFalse(TEXT("Move input is back"), PC->IsMoveInputIgnored());
            UMemoriaFieldCharacterComponent* Arrel = FindArrel(Pawn);
            AMemoriaEliaCompanion* Elia = FindElia(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            Test->TestTrue(TEXT("Arrel is standing"), Arrel && !Arrel->IsSeated());
            Test->TestTrue(TEXT("Elia is standing"), Elia && Elia->GetFigure() && !Elia->GetFigure()->IsSeated());
            Test->TestTrue(TEXT("Tobias is standing"), Tobias && Tobias->GetFigure() && !Tobias->GetFigure()->IsSeated());
            const FVector Ground = Cart->GetActorLocation();
            const FVector Right = Cart->GetActorRightVector();
            auto OnRoad = [&](const AActor* Actor)
            {
                if (!Actor) return false;
                const FVector At = Actor->GetActorLocation();
                const float Side = FVector::DotProduct(FVector(At.X - Ground.X, At.Y - Ground.Y, 0.f), Right);
                return FMath::Abs(At.Z - Ground.Z) < 40.f && FVector::Dist2D(At, Ground) > 100.f && Side > 80.f;
            };
            Test->TestTrue(TEXT("Arrel is on open ground beside the cart"), OnRoad(Pawn));
            Test->TestTrue(TEXT("Elia is on open ground beside the cart"), OnRoad(Elia));
            Test->TestTrue(TEXT("Tobias is on open ground beside the cart"), OnRoad(Tobias));
            return true;
        }
        default: break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started = 0, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false, bSawMid = false;
    TWeakObjectPtr<UWorld> From;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<AMemoriaCart> Cart;
    FVector Mid = FVector::ZeroVector, End = FVector::ZeroVector;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCartS399, "Memoria.S399.Cart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCartS399::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FCartReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
