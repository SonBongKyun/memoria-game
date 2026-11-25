#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Scene.h"
#include "EngineUtils.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Interaction/MemoriaTobiasCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool HairIsSilver(const UMaterialInterface* Material)
{
    if (!Material || !Material->GetName().Contains(TEXT("EliaHair"))) return false;
    const UMaterial* Source = Material->GetMaterial();
    if (!Source) return false;
    bool bSilver = false;
#if WITH_EDITORONLY_DATA
    for (const TObjectPtr<UMaterialExpression>& Expr : Source->GetExpressionCollection().Expressions)
    {
        const auto* Custom = Cast<UMaterialExpressionCustom>(Expr.Get());
        if (!Custom) continue;
        if (Custom->Code.Contains(TEXT("float3 silver=float3(0.80,0.82,0.84)"))
            && Custom->Code.Contains(TEXT("step(.10,A)*(1-step(.35,A))")))
            bSilver = true;
    }
#endif
    return bSilver;
}
bool CarriesStaff(const AActor* Actor)
{
    if (!Actor) return false;
    TArray<UStaticMeshComponent*> Meshes;
    Actor->GetComponents<UStaticMeshComponent>(Meshes);
    for (const UStaticMeshComponent* Mesh : Meshes)
    {
        const FString Path = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName() : FString();
        if (Path.Contains(TEXT("staff"), ESearchCase::IgnoreCase)) return true;
    }
    return false;
}
UStaticMeshComponent* NamedMesh(const AActor* Actor, const TCHAR* Name)
{
    if (!Actor) return nullptr;
    TArray<UStaticMeshComponent*> Meshes;
    Actor->GetComponents<UStaticMeshComponent>(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes) if (Mesh && Mesh->GetName().Equals(Name)) return Mesh;
    return nullptr;
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
ACameraActor* Aim(UWorld* World, APlayerController* PC, const FVector& Focus, const FVector& EyeOffset, float Fov, ACameraActor* Camera)
{
    if (!World || !PC) return Camera;
    const FVector Eye = Focus + EyeOffset;
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
    Camera->SetActorLocation(Eye);
    Camera->SetActorRotation((Focus - Eye).Rotation());
    if (UCameraComponent* Cam = Camera->GetCameraComponent()) Cam->SetFieldOfView(Fov);
    PC->SetViewTargetWithBlend(Camera, 0.f);
    return Camera;
}
bool ReadLooks(UMemoriaFieldCharacterComponent* Figure, bool& bHair, bool& bFace, FString& Slots)
{
    bHair = false;
    bFace = false;
    Slots.Reset();
    USkeletalMeshComponent* Body = Figure ? Figure->GetSkeletalMesh() : nullptr;
    const USkeletalMesh* Asset = Body ? Body->GetSkeletalMeshAsset() : nullptr;
    if (!Asset) return false;
    const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        const UMaterialInterface* Mat = Body->GetMaterial(Index);
        const FString Name = Materials[Index].MaterialSlotName.ToString() + Materials[Index].ImportedMaterialSlotName.ToString() + GetNameSafe(Mat);
        Slots += Name + TEXT("; ");
        if (Name.Contains(TEXT("EliaSkin"))) bFace = true;
        if (HairIsSilver(Mat)) bHair = true;
    }
    return true;
}
class FFieldLooksReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldLooksReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds())
    {
        const FString Dir = FPaths::ProjectSavedDir() / TEXT("Validation/S388Looks");
        IFileManager::Get().MakeDirectory(*Dir, true);
        EliaShot = Dir / TEXT("Elia.png");
        WalkShot = Dir / TEXT("Walking.png");
        IFileManager::Get().Delete(*EliaShot, false, true);
        IFileManager::Get().Delete(*WalkShot, false, true);
    }
    ~FFieldLooksReplay() override
    {
        if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
            if (UGameInstance* Game = World->GetGameInstance())
                if (auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>()) Run->SetStoryFlag(TEXT("party_tobias"), false);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.0)
        {
            Test->AddError(FString::Printf(TEXT("Field looks timeout at step %d"), Step));
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
        {
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) break;
            AMemoriaEliaCompanion* Elia = FindElia(World);
            if (!Elia) break;
            if (!bQuieted) { QuietBelt(Run, Spec); bQuieted = true; Mark = Frame; break; }
            if (Frame < Mark + 10) break;
            auto* Figure = Elia->GetFigure();
            Test->TestTrue(TEXT("Elia is rigged"), Figure && Figure->IsRigged());
            Test->TestEqual(TEXT("Elia carries no staff prop"), Figure ? Figure->GetPropCount() : -1, 0);
            Test->TestFalse(TEXT("No staff mesh on Elia"), CarriesStaff(Elia));
            bool bHair = false, bFace = false;
            FString Slots;
            ReadLooks(Figure, bHair, bFace, Slots);
            Test->TestTrue(FString::Printf(TEXT("Silver hair tint is applied (%s)"), *Slots), bHair);
            Test->TestTrue(FString::Printf(TEXT("Face stays on EliaSkin (%s)"), *Slots), bFace);
            ++Step;
            break;
        }
        case 1:
            Run->SetStoryFlag(TEXT("party_tobias"), true);
            From = World;
            Camera.Reset();
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step; Mark = Frame;
            break;
        case 2:
        {
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) break;
            AMemoriaEliaCompanion* Elia = FindElia(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            if (!Elia || !Tobias) break;
            QuietBelt(Run, Spec);
            auto* Figure = Tobias->GetFigure();
            auto* EliaFigure = Elia->GetFigure();
            Test->TestEqual(TEXT("Elia still carries no staff after reload"), EliaFigure ? EliaFigure->GetPropCount() : -1, 0);
            Test->TestFalse(TEXT("No staff mesh after reload"), CarriesStaff(Elia));
            Badge = NamedMesh(Tobias, TEXT("TobiasBadge"));
            auto* Body = Figure ? Figure->GetSkeletalMesh() : nullptr;
            Test->TestNotNull(TEXT("A pale badge"), Badge.Get());
            Test->TestTrue(TEXT("Badge is parented to the body"), Badge.IsValid() && Body && Badge->GetAttachParent() == Body);
            Socket = Badge.IsValid() ? Badge->GetAttachSocketName() : NAME_None;
            Test->TestTrue(FString::Printf(TEXT("Badge sits on the chest (%s)"), *Socket.ToString()),
                Socket == TEXT("spine_03") || Socket == TEXT("spine_02") || Socket == TEXT("spine_01"));
            if (Badge.IsValid())
            {
                const FVector Scale = Badge->GetRelativeScale3D();
                const float Smallest = FMath::Min3(Scale.X, Scale.Y, Scale.Z);
                const float Largest = FMath::Max3(Scale.X, Scale.Y, Scale.Z);
                Test->TestTrue(FString::Printf(TEXT("Badge is a thin plate (%s)"), *Scale.ToString()), Smallest < 0.03f && Largest < 0.12f);
                BadgeAt = Badge->GetRelativeLocation();
            }
            if (auto* Mid = Badge.IsValid() ? Cast<UMaterialInstanceDynamic>(Badge->GetMaterial(0)) : nullptr)
                Test->TestTrue(TEXT("The badge is pale"), Mid->K2_GetVectorParameterValue(TEXT("Color")).R > 0.7f);
            if (!Figure || !Badge.IsValid()) return true;
            ++Step; Mark = Frame;
            break;
        }
        case 3:
        {
            AMemoriaEliaCompanion* Elia = FindElia(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            if (!Elia || !Tobias || !Badge.IsValid()) { Test->AddError(TEXT("A companion left during the walk")); return true; }
            const int32 Walked = Frame - Mark;
            if (Walked < 220 && MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).X < 640.f)
                Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(2.5, 0, 0));
            const FVector Focus = (Pawn->GetActorLocation() + Elia->GetActorLocation() + Tobias->GetActorLocation()) / 3.0 + FVector(0, 0, 80);
            // Negative Y sees their backs. The camera sits in front (+Y) and a little ahead.
            Camera = Aim(World, PC, Focus, FVector(160.f, 520.f, 180.f), 40.f, Camera.Get());
            if (Walked == 90)
            {
                FScreenshotRequest::RequestScreenshot(WalkShot, false, false);
                bRequestedWalk = true;
                ShotAt = Frame;
            }
            if (Walked < 160) break;
            ++Step; Mark = Frame;
            break;
        }
        case 4:
        {
            if (IFileManager::Get().FileSize(*WalkShot) < 1000)
            {
                if (Frame > Mark + 150) { Test->AddError(TEXT("Walking.png was not written")); return true; }
                break;
            }
            if (!bPortrait) { bPortrait = true; Mark = Frame; }
            AMemoriaEliaCompanion* Elia = FindElia(World);
            auto* Figure = Elia ? Elia->GetFigure() : nullptr;
            if (!Elia || !Figure) { Test->AddError(TEXT("Elia left before the portrait")); return true; }
            Pawn->SetActorHiddenInGame(true);
            if (AMemoriaTobiasCompanion* Beside = FindTobias(World)) Beside->SetActorHiddenInGame(true);
            const FVector Head = Figure->FocusPosition() + FVector(0, 0, 36);
            Camera = Aim(World, PC, Head, FVector(36.f, 170.f, 22.f), 28.f, Camera.Get());
            if (Frame < Mark + 20) break;
            if (!bRequestedElia)
            {
                FScreenshotRequest::RequestScreenshot(EliaShot, false, false);
                bRequestedElia = true;
                Mark = Frame;
            }
            ++Step;
            break;
        }
        case 5:
            if (IFileManager::Get().FileSize(*EliaShot) < 1000)
            {
                if (Frame > Mark + 150) { Test->AddError(TEXT("Elia.png was not written")); return true; }
                break;
            }
            {
                AMemoriaTobiasCompanion* Tobias = FindTobias(World);
                auto* Live = NamedMesh(Tobias, TEXT("TobiasBadge"));
                Test->TestNotNull(TEXT("Badge is still there"), Live);
                if (Live)
                {
                    Test->TestEqual(TEXT("Badge stayed on the chest bone"), Live->GetAttachSocketName(), Socket);
                    Test->TestTrue(TEXT("Badge moves with him"), Live->GetRelativeLocation().Equals(BadgeAt, 0.05f));
                }
                Test->TestTrue(TEXT("Elia capture is on disk"), IFileManager::Get().FileSize(*EliaShot) > 1000);
                Test->TestTrue(TEXT("Walking capture is on disk"), IFileManager::Get().FileSize(*WalkShot) > 1000);
            }
            return true;
        default: break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started = 0, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, ShotAt = 0;
    bool bFixed = false, bOldFixed = false, bQuieted = false, bRequestedElia = false, bRequestedWalk = false, bPortrait = false;
    TWeakObjectPtr<UWorld> From;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<UStaticMeshComponent> Badge;
    FName Socket = NAME_None;
    FVector BadgeAt = FVector::ZeroVector;
    FString EliaShot, WalkShot;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldLooksS388, "Memoria.S388.FieldLooks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldLooksS388::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldLooksReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
