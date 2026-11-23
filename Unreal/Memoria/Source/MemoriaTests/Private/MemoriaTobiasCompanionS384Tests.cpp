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
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionVertexColor.h"
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
// S384: party_tobias is off on the first Belt load, so Tobias is absent. The same run then reloads the
// map with the flag on. He spawns behind Elia, walks Arrel's trail, keeps a gap, and carries no weapon.
bool CoatKeepsThePerson(const UMaterialInterface* Material)
{
    if (!Material || !Material->GetName().Contains(TEXT("TobiasCoat")) || Material->GetName().Contains(TEXT("FieldFoe"))) return false;
    const UMaterial* Source = Material->GetMaterial();
    if (!Source) return false;
    bool bVertex = false, bInk = false;
#if WITH_EDITORONLY_DATA
    for (const TObjectPtr<UMaterialExpression>& Expr : Source->GetExpressionCollection().Expressions)
    {
        if (Cast<UMaterialExpressionVertexColor>(Expr.Get())) bVertex = true;
        if (const auto* Custom = Cast<UMaterialExpressionCustom>(Expr.Get()))
            if (Custom->Code.Contains(TEXT("Category-.75")) && Custom->Code.Contains(TEXT("0.10,0.075,0.055"))) bInk = true;
    }
#endif
    return bVertex && bInk;
}
int32 CountOf(UWorld* World)
{
    int32 Count = 0;
    if (World) for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count;
    return Count;
}
AMemoriaTobiasCompanion* FindTobias(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
AMemoriaEliaCompanion* FindElia(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) return *It;
    return nullptr;
}
UStaticMeshComponent* NamedMesh(const AActor* Actor, const TCHAR* Name)
{
    if (!Actor) return nullptr;
    TArray<UStaticMeshComponent*> Meshes;
    Actor->GetComponents<UStaticMeshComponent>(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes) if (Mesh && Mesh->GetName().Equals(Name)) return Mesh;
    return nullptr;
}
void QuietBelt(UMemoriaRunSubsystem* Run, const FMemoriaChapterMapSpec* Spec)
{
    if (!Run || !Spec) return;
    // Closing the chapter (ch3_complete) opens the road. Leave that flag unset.
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
ACameraActor* FrameParty(UWorld* World, APlayerController* PC, const FVector& Focus, ACameraActor* Camera)
{
    if (!World || !PC) return Camera;
    const FVector Eye = Focus + FVector(220.f, -560.f, 300.f);
    if (!Camera)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Camera = World->SpawnActor<ACameraActor>(Eye, (Focus - Eye).Rotation(), Params);
        if (UCameraComponent* Cam = Camera ? Camera->GetCameraComponent() : nullptr)
        {
            Cam->SetFieldOfView(42.f);
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
    PC->SetViewTargetWithBlend(Camera, 0.f);
    return Camera;
}
class FTobiasCompanionReplay final : public IAutomationLatentCommand
{
public:
    explicit FTobiasCompanionReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FTobiasCompanionReplay() override
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
            Test->AddError(FString::Printf(TEXT("Tobias companion timeout at step %d"), Step));
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
            Test->TestEqual(TEXT("Flag off: Tobias is not in the party"), CountOf(World), 0);
            Test->TestNotNull(TEXT("Elia is already with Arrel"), FindElia(World));
            Test->TestFalse(TEXT("party_tobias starts unset"), Run->GetRunSnapshot().GetFlag(TEXT("party_tobias")));
            QuietBelt(Run, Spec);
            Test->TestFalse(TEXT("The chapter stays open"), Run->GetRunSnapshot().GetFlag(Spec->Exit.Completes));
            Run->SetStoryFlag(TEXT("party_tobias"), true);
            From = World;
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step; Mark = Frame; break;
        case 1:
        {
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation")) || !FindElia(World)) break;
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            Test->TestNotNull(TEXT("Flag on: Tobias joins the reload"), Tobias);
            Test->TestEqual(TEXT("One Tobias"), CountOf(World), 1);
            if (!Tobias) return true;
            auto* Figure = Tobias->GetFigure();
            Test->TestTrue(TEXT("He is rigged"), Figure && Figure->IsRigged());
            const FString Id = Figure ? Figure->GetCharacterId() : FString();
            Test->TestEqual(TEXT("His id is Tobias"), Id, FString(TEXT("Tobias")));
            const USkeletalMesh* Asset = Figure && Figure->GetSkeletalMesh() ? Figure->GetSkeletalMesh()->GetSkeletalMeshAsset() : nullptr;
            const FString MeshPath = Asset ? Asset->GetPathName() : FString();
            Test->TestTrue(FString::Printf(TEXT("Donor mesh is the bureau agent or the scholar (%s)"), *MeshPath),
                MeshPath.Contains(TEXT("Bureauagent")) || MeshPath.Contains(TEXT("Scholar")));
            Test->TestEqual(TEXT("Badge, bag and strap"), Figure ? Figure->GetPropCount() : -1, 3);
            Test->TestFalse(TEXT("No sword"), Figure && Figure->HasSword());
            Test->TestNull(TEXT("No blade"), Figure ? Figure->GetBlade() : nullptr);
            Test->TestNull(TEXT("No lantern"), Tobias->FindComponentByClass<UPointLightComponent>());
            auto* Body = Cast<USphereComponent>(Tobias->GetRootComponent());
            Test->TestEqual(TEXT("He never blocks movement"), Body ? Body->GetCollisionEnabled() : ECollisionEnabled::QueryAndPhysics, ECollisionEnabled::QueryOnly);
            ArrelAt = Pawn->GetActorLocation();
            EliaAt = FindElia(World)->GetActorLocation();
            TobiasAt = Tobias->GetActorLocation();
            ++Step; Mark = Frame; break;
        }
        case 2:
        {
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            AMemoriaEliaCompanion* Elia = FindElia(World);
            if (!Tobias || !Elia) { Test->AddError(TEXT("A companion left during the walk")); return true; }
            const int32 Walked = Frame - Mark;
            // Stay west of the exit. Source X 640 is still short of the belt road.
            if (Walked < 260 && MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).X < 640.f)
                Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(2.5, 0, 0));
            const FVector Focus = (Pawn->GetActorLocation() + Elia->GetActorLocation() + Tobias->GetActorLocation()) / 3.0 + FVector(0, 0, 80);
            Camera = FrameParty(World, PC, Focus, Camera.Get());
            if (Walked == 200)
            {
                IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Validation/S384Tobias")), true);
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S384Tobias/Walking.png"), true, false);
            }
            if (Walked < 280) break;
            ++Step; Mark = Frame; break;
        }
        case 3:
            if (Frame < Mark + 90) break;
            {
                AMemoriaTobiasCompanion* Tobias = FindTobias(World);
                AMemoriaEliaCompanion* Elia = FindElia(World);
                auto* Figure = Tobias ? Tobias->GetFigure() : nullptr;
                if (!Tobias || !Elia || !Figure) { Test->AddError(TEXT("Companions missing after the walk")); return true; }
                const FVector ArrelNow = Pawn->GetActorLocation();
                const FVector EliaNow = Elia->GetActorLocation();
                const FVector TobiasNow = Tobias->GetActorLocation();
                Test->TestTrue(TEXT("Arrel walked"), FVector::Dist2D(ArrelNow, ArrelAt) > 150.0);
                Test->TestTrue(TEXT("Elia followed"), FVector::Dist2D(EliaNow, EliaAt) > 150.0);
                Test->TestTrue(TEXT("Tobias followed"), FVector::Dist2D(TobiasNow, TobiasAt) > 150.0);
                Test->TestTrue(TEXT("Tobias stays behind Elia"), FVector::Dist2D(TobiasNow, ArrelNow) > FVector::Dist2D(EliaNow, ArrelNow));
                Test->TestTrue(TEXT("The gap behind Elia holds"), FVector::Dist2D(TobiasNow, EliaNow) >= 60.0);
                Test->TestTrue(TEXT("He is further back than Elia"), FVector::Dist2D(TobiasNow, ArrelNow) >= FVector::Dist2D(EliaNow, ArrelNow) + 40.0);
                Test->TestTrue(TEXT("He does not crowd Arrel"), FVector::Dist2D(TobiasNow, ArrelNow) >= 140.0);
                const FVector EliaBack = FVector(EliaNow.X - ArrelNow.X, EliaNow.Y - ArrelNow.Y, 0).GetSafeNormal();
                const FVector TobiasBack = FVector(TobiasNow.X - EliaNow.X, TobiasNow.Y - EliaNow.Y, 0).GetSafeNormal();
                Test->TestTrue(TEXT("He stands on Elia's far side"), FVector::DotProduct(EliaBack, TobiasBack) > 0.5);
                Test->TestTrue(TEXT("Still west of the exit"), MemoriaChapterMaps::ToSource(ArrelNow).X < 700.f);
                Test->TestFalse(TEXT("The road did not open"), Run->GetRunSnapshot().GetFlag(Spec->Exit.Completes));
                TArray<UStaticMeshComponent*> Meshes;
                Tobias->GetComponents<UStaticMeshComponent>(Meshes);
                bool bWeapon = false;
                for (const UStaticMeshComponent* Mesh : Meshes)
                {
                    const FString Path = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName().ToLower() : FString();
                    if (Path.Contains(TEXT("sword")) || Path.Contains(TEXT("dagger")) || Path.Contains(TEXT("staff")) || Path.Contains(TEXT("lantern"))) bWeapon = true;
                }
                Test->TestFalse(TEXT("No weapon prop"), bWeapon || Figure->HasSword() || Figure->GetBlade() != nullptr);
                auto* Badge = NamedMesh(Tobias, TEXT("TobiasBadge"));
                auto* Bag = NamedMesh(Tobias, TEXT("TobiasBag"));
                Test->TestNotNull(TEXT("A pale badge"), Badge);
                Test->TestNotNull(TEXT("A bag on his back"), Bag);
                auto* Body = Figure->GetSkeletalMesh();
                const USkeletalMesh* Asset = Body ? Body->GetSkeletalMeshAsset() : nullptr;
                bool bMannequin = false, bCoat = false;
                FString Slots;
                if (Asset)
                {
                    const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
                    for (int32 Index = 0; Index < Materials.Num(); ++Index)
                    {
                        const UMaterialInterface* Mat = Body->GetMaterial(Index);
                        const FString Name = Materials[Index].MaterialSlotName.ToString() + Materials[Index].ImportedMaterialSlotName.ToString() + GetNameSafe(Mat);
                        Slots += Name + TEXT("; ");
                        if (Name.Contains(TEXT("FieldFoe"))) bMannequin = true;
                        if (const auto* Mid = Cast<UMaterialInstanceDynamic>(Mat))
                            if (Mid->Parent && Mid->Parent->GetName().Contains(TEXT("FieldFoe"))) bMannequin = true;
                        if (CoatKeepsThePerson(Mat)) bCoat = true;
                    }
                }
                Test->TestFalse(TEXT("Flat FieldFoe coat"), bMannequin);
                Test->TestTrue(FString::Printf(TEXT("Dark grey-brown coat keeps the person (%s)"), *Slots), bCoat);
                if (auto* BadgeMid = Badge ? Cast<UMaterialInstanceDynamic>(Badge->GetMaterial(0)) : nullptr)
                    Test->TestTrue(TEXT("The badge is pale"), BadgeMid->K2_GetVectorParameterValue(TEXT("Color")).R > 0.7f);
                if (auto* BagMid = Bag ? Cast<UMaterialInstanceDynamic>(Bag->GetMaterial(0)) : nullptr)
                    Test->TestTrue(TEXT("The bag is dark"), BagMid->K2_GetVectorParameterValue(TEXT("Color")).R < 0.3f);
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
    int32 Step = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<UWorld> From;
    TWeakObjectPtr<ACameraActor> Camera;
    FVector ArrelAt = FVector::ZeroVector, EliaAt = FVector::ZeroVector, TobiasAt = FVector::ZeroVector;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTobiasCompanionS384, "Memoria.S384.TobiasCompanion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTobiasCompanionS384::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FTobiasCompanionReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
