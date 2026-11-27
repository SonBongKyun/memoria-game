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
#include "Interaction/MemoriaHannaCompanion.h"
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
// S395: party_hanna off means no Hanna. With the flag she follows behind Tobias. hanna_gone, set during play
// after a leaving point, walks her off and a reload does not bring her back. No weapon and no lantern.
bool HannaKeepsThePerson(const UMaterialInterface* Material)
{
    if (!Material || !Material->GetName().Contains(TEXT("HannaLook")) || Material->GetPathName().Contains(TEXT("FieldFoe"))) return false;
    if (!Material->GetPathName().Contains(TEXT("/Game/Memoria/Presentation/FieldLooks/"))) return false;
    const UMaterial* Source = Material->GetMaterial();
    if (!Source) return false;
    bool bVertex = false, bLook = false, bDark = true;
#if WITH_EDITOR
    if (const UMaterialEditorOnlyData* Editor = Source->GetEditorOnlyData())
        bDark = Editor->EmissiveColor.Expression == nullptr;
    for (const TObjectPtr<UMaterialExpression>& Expr : Source->GetExpressionCollection().Expressions)
    {
        if (Cast<UMaterialExpressionVertexColor>(Expr.Get())) bVertex = true;
        if (const auto* Custom = Cast<UMaterialExpressionCustom>(Expr.Get()))
            if (Custom->Code.Contains(TEXT("Category-.75")) && Custom->Code.Contains(TEXT("0.34,0.35,0.36")) && Custom->Code.Contains(TEXT("0.86,0.87,0.85")))
                bLook = true;
    }
#endif
    return bVertex && bLook && bDark;
}
int32 CountHanna(UWorld* World)
{
    int32 Count = 0;
    if (World) for (TActorIterator<AMemoriaHannaCompanion> It(World); It; ++It) ++Count;
    return Count;
}
int32 CountTobias(UWorld* World)
{
    int32 Count = 0;
    if (World) for (TActorIterator<AMemoriaTobiasCompanion> It(World); It; ++It) ++Count;
    return Count;
}
AMemoriaHannaCompanion* FindHanna(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AMemoriaHannaCompanion> It(World); It; ++It) return *It;
    return nullptr;
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
float InkR(UStaticMeshComponent* Mesh)
{
    auto* Mid = Mesh ? Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)) : nullptr;
    return Mid ? Mid->K2_GetVectorParameterValue(TEXT("Color")).R : -1.f;
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
void Shot(const TCHAR* Name)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Validation/S395Hanna");
    IFileManager::Get().MakeDirectory(*Dir, true);
    FScreenshotRequest::RequestScreenshot(Dir / Name, true, false);
}
class FHannaCompanionReplay final : public IAutomationLatentCommand
{
public:
    explicit FHannaCompanionReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FHannaCompanionReplay() override
    {
        AMemoriaHannaCompanion::ClearLeavingPoint();
        if (UWorld* World = GEditor ? GEditor->PlayWorld : nullptr)
            if (UGameInstance* Game = World->GetGameInstance())
                if (auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>())
                {
                    Run->SetStoryFlag(TEXT("party_hanna"), false);
                    Run->SetStoryFlag(TEXT("hanna_gone"), false);
                    Run->SetStoryFlag(TEXT("party_tobias"), false);
                }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240.0)
        {
            Test->AddError(FString::Printf(TEXT("Hanna companion timeout at step %d"), Step));
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
            Test->TestEqual(TEXT("Flag off: Hanna is not in the party"), CountHanna(World), 0);
            Test->TestNotNull(TEXT("Elia is already with Arrel"), FindElia(World));
            Test->TestFalse(TEXT("party_hanna starts unset"), Run->GetRunSnapshot().GetFlag(TEXT("party_hanna")));
            Test->TestFalse(TEXT("hanna_gone starts unset"), Run->GetRunSnapshot().GetFlag(TEXT("hanna_gone")));
            QuietBelt(Run, Spec);
            Test->TestFalse(TEXT("The chapter stays open"), Run->GetRunSnapshot().GetFlag(Spec->Exit.Completes));
            Run->SetStoryFlag(TEXT("party_tobias"), true);
            Run->SetStoryFlag(TEXT("party_hanna"), true);
            From = World;
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step; Mark = Frame; break;
        case 1:
        {
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation")) || !FindElia(World) || !FindTobias(World)) break;
            AMemoriaHannaCompanion* Hanna = FindHanna(World);
            Test->TestNotNull(TEXT("Flag on: Hanna joins the reload"), Hanna);
            Test->TestEqual(TEXT("One Hanna"), CountHanna(World), 1);
            Test->TestEqual(TEXT("Tobias is ahead of her"), CountTobias(World), 1);
            if (!Hanna) return true;
            auto* Figure = Hanna->GetFigure();
            Test->TestTrue(TEXT("She is rigged"), Figure && Figure->IsRigged());
            Test->TestEqual(TEXT("Her id is Hanna"), Figure ? Figure->GetCharacterId() : FString(), FString(TEXT("Hanna")));
            const USkeletalMesh* Asset = Figure && Figure->GetSkeletalMesh() ? Figure->GetSkeletalMesh()->GetSkeletalMeshAsset() : nullptr;
            const FString MeshPath = Asset ? Asset->GetPathName() : FString();
            Test->TestTrue(FString::Printf(TEXT("Donor mesh is villager_f or the elder (%s)"), *MeshPath),
                MeshPath.Contains(TEXT("Villagerf")) || MeshPath.Contains(TEXT("Elder")));
            Test->TestEqual(TEXT("Hat, band, patch, bag and buckle"), Figure ? Figure->GetPropCount() : -1, 5);
            Test->TestFalse(TEXT("No sword"), Figure && Figure->HasSword());
            Test->TestNull(TEXT("No blade"), Figure ? Figure->GetBlade() : nullptr);
            Test->TestNull(TEXT("No lantern"), Hanna->FindComponentByClass<UPointLightComponent>());
            auto* Body = Cast<USphereComponent>(Hanna->GetRootComponent());
            Test->TestEqual(TEXT("She never blocks movement"), Body ? Body->GetCollisionEnabled() : ECollisionEnabled::QueryAndPhysics, ECollisionEnabled::QueryOnly);
            auto* Hat = NamedMesh(Hanna, TEXT("HannaHat"));
            auto* Band = NamedMesh(Hanna, TEXT("HannaHatBand"));
            auto* Patch = NamedMesh(Hanna, TEXT("HannaHatPatch"));
            auto* Bag = NamedMesh(Hanna, TEXT("HannaBag"));
            auto* Buckle = NamedMesh(Hanna, TEXT("HannaBuckle"));
            Test->TestNotNull(TEXT("A hat"), Hat);
            Test->TestNotNull(TEXT("A hat band"), Band);
            Test->TestNotNull(TEXT("A darker patch where the tag was"), Patch);
            Test->TestNotNull(TEXT("A bag"), Bag);
            Test->TestNotNull(TEXT("A buckle"), Buckle);
            Test->TestTrue(TEXT("The patch is darker than the band"), InkR(Patch) < InkR(Band) - 0.08f);
            TArray<UStaticMeshComponent*> Meshes;
            Hanna->GetComponents<UStaticMeshComponent>(Meshes);
            bool bWeapon = false;
            for (const UStaticMeshComponent* Mesh : Meshes)
            {
                const FString Path = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName().ToLower() : FString();
                if (Path.Contains(TEXT("sword")) || Path.Contains(TEXT("blade")) || Path.Contains(TEXT("dagger")) || Path.Contains(TEXT("staff")) || Path.Contains(TEXT("lantern"))) bWeapon = true;
            }
            Test->TestFalse(TEXT("No weapon prop"), bWeapon || (Figure && (Figure->HasSword() || Figure->GetBlade() != nullptr)));
            bool bLook = false, bFoe = false;
            FString Slots;
            if (auto* Skinned = Figure ? Figure->GetSkeletalMesh() : nullptr)
            {
                const USkeletalMesh* SkinAsset = Skinned->GetSkeletalMeshAsset();
                const TArray<FSkeletalMaterial>& Materials = SkinAsset ? SkinAsset->GetMaterials() : TArray<FSkeletalMaterial>();
                for (int32 Index = 0; Index < Materials.Num(); ++Index)
                {
                    const UMaterialInterface* Mat = Skinned->GetMaterial(Index);
                    const FString Name = Materials[Index].MaterialSlotName.ToString() + GetNameSafe(Mat);
                    Slots += Name + TEXT("; ");
                    if (Name.Contains(TEXT("FieldFoe"))) bFoe = true;
                    if (const auto* Mid = Cast<UMaterialInstanceDynamic>(Mat))
                        if (Mid->Parent && Mid->Parent->GetName().Contains(TEXT("FieldFoe"))) bFoe = true;
                    if (HannaKeepsThePerson(Mat)) bLook = true;
                }
            }
            Test->TestFalse(TEXT("Flat FieldFoe coat"), bFoe);
            Test->TestTrue(FString::Printf(TEXT("Grey coat and white streaks (%s)"), *Slots), bLook);
            ArrelAt = Pawn->GetActorLocation();
            TobiasAt = FindTobias(World)->GetActorLocation();
            HannaAt = Hanna->GetActorLocation();
            ++Step; Mark = Frame; break;
        }
        case 2:
        {
            AMemoriaHannaCompanion* Hanna = FindHanna(World);
            AMemoriaTobiasCompanion* Tobias = FindTobias(World);
            AMemoriaEliaCompanion* Elia = FindElia(World);
            if (!Hanna || !Tobias || !Elia) { Test->AddError(TEXT("A companion left during the walk")); return true; }
            const int32 Walked = Frame - Mark;
            if (Walked < 260 && MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).X < 640.f)
                Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(2.5, 0, 0));
            const FVector Head = Hanna->GetActorLocation() + FVector(0, 0, 120);
            if (Walked <= 40)
                Camera = FrameAt(World, PC, Head, Head + FVector(70.f, -150.f, 28.f), 28.f, Camera.Get());
            else
            {
                const FVector Focus = (Pawn->GetActorLocation() + Tobias->GetActorLocation() + Hanna->GetActorLocation()) / 3.0 + FVector(0, 0, 80);
                Camera = FrameAt(World, PC, Focus, Focus + FVector(220.f, -560.f, 220.f), 42.f, Camera.Get());
            }
            if (Walked == 40) Shot(TEXT("Hanna.png"));
            if (Walked == 200) Shot(TEXT("Walking.png"));
            if (Walked < 280) break;
            const FVector ArrelNow = Pawn->GetActorLocation();
            const FVector TobiasNow = Tobias->GetActorLocation();
            const FVector HannaNow = Hanna->GetActorLocation();
            Test->TestTrue(TEXT("Arrel walked"), FVector::Dist2D(ArrelNow, ArrelAt) > 150.0);
            Test->TestTrue(TEXT("Tobias followed"), FVector::Dist2D(TobiasNow, TobiasAt) > 150.0);
            Test->TestTrue(TEXT("Hanna followed"), FVector::Dist2D(HannaNow, HannaAt) > 150.0);
            Test->TestTrue(TEXT("Hanna stays behind Tobias"), FVector::Dist2D(HannaNow, ArrelNow) > FVector::Dist2D(TobiasNow, ArrelNow));
            Test->TestTrue(TEXT("The gap behind Tobias holds"), FVector::Dist2D(HannaNow, TobiasNow) >= 60.0);
            Test->TestTrue(TEXT("She is further back than Tobias"), FVector::Dist2D(HannaNow, ArrelNow) >= FVector::Dist2D(TobiasNow, ArrelNow) + 40.0);
            const FVector TobiasBack = FVector(TobiasNow.X - ArrelNow.X, TobiasNow.Y - ArrelNow.Y, 0).GetSafeNormal();
            const FVector HannaBack = FVector(HannaNow.X - TobiasNow.X, HannaNow.Y - TobiasNow.Y, 0).GetSafeNormal();
            Test->TestTrue(TEXT("She stands on Tobias's far side"), FVector::DotProduct(TobiasBack, HannaBack) > 0.5);
            Test->TestTrue(TEXT("Still west of the exit"), MemoriaChapterMaps::ToSource(ArrelNow).X < 700.f);
            ++Step; Mark = Frame; break;
        }
        case 3:
        {
            AMemoriaHannaCompanion* Hanna = FindHanna(World);
            if (!Hanna) { Test->AddError(TEXT("Hanna was already gone before the flag")); return true; }
            LeaveFrom = Hanna->GetActorLocation();
            LeavePoint = LeaveFrom + FVector(0, -380, 0);
            AMemoriaHannaCompanion::SetLeavingPoint(LeavePoint);
            Run->SetStoryFlag(TEXT("hanna_gone"), true);
            Test->TestTrue(TEXT("The chapter can read the leaving point"), AMemoriaHannaCompanion::HasLeavingPoint());
            ++Step; Mark = Frame; break;
        }
        case 4:
        {
            const int32 Left = Frame - Mark;
            if (AMemoriaHannaCompanion* Hanna = FindHanna(World))
            {
                LastHanna = Hanna->GetActorLocation();
                const FVector Focus = (LeaveFrom + LastHanna) * 0.5 + FVector(0, 0, 90);
                Camera = FrameAt(World, PC, Focus, Focus + FVector(180.f, -420.f, 180.f), 40.f, Camera.Get());
            }
            if (Left == 36) Shot(TEXT("Leaving.png"));
            if (CountHanna(World) > 0 && Left < 400) break;
            Test->TestEqual(TEXT("She walked away and is gone"), CountHanna(World), 0);
            Test->TestTrue(TEXT("She moved toward the alley"), FVector::Dist2D(LastHanna, LeaveFrom) >= 80.0);
            Test->TestTrue(TEXT("The alley is closer than where she started"), FVector::Dist2D(LastHanna, LeavePoint) + 80.0 <= FVector::Dist2D(LeaveFrom, LeavePoint));
            Test->TestEqual(TEXT("Tobias stays"), CountTobias(World), 1);
            From = World;
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Memoria/Maps/L_BeltWaystation"));
            ++Step; Mark = Frame; break;
        }
        case 5:
            if (World == From.Get() || World->GetTimeSeconds() < .2) break;
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation")) || !FindElia(World)) break;
            Test->TestTrue(TEXT("hanna_gone is still set"), Run->GetRunSnapshot().GetFlag(TEXT("hanna_gone")));
            Test->TestTrue(TEXT("party_hanna is still set"), Run->GetRunSnapshot().GetFlag(TEXT("party_hanna")));
            Test->TestEqual(TEXT("A reload does not bring her back"), CountHanna(World), 0);
            Test->TestEqual(TEXT("Tobias is still with them"), CountTobias(World), 1);
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
    FVector ArrelAt = FVector::ZeroVector, TobiasAt = FVector::ZeroVector, HannaAt = FVector::ZeroVector;
    FVector LeaveFrom = FVector::ZeroVector, LeavePoint = FVector::ZeroVector, LastHanna = FVector::ZeroVector;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHannaCompanionS395, "Memoria.S395.HannaCompanion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHannaCompanionS395::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHannaCompanionReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
