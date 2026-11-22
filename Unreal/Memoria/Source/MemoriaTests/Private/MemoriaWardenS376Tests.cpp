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
#include "Combat/MemoriaFieldMonster.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Localization/MemoriaKorean.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Scene.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaCombatTuning;

void HideElia(UWorld* World)
{
    for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
}
bool HasClub(const AMemoriaFieldMonster* Warden)
{
    if (!Warden) return false;
    TArray<UStaticMeshComponent*> Meshes;
    Warden->GetComponents<UStaticMeshComponent>(Meshes);
    for (const UStaticMeshComponent* Mesh : Meshes)
        if (Mesh && Mesh->IsVisible() && FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, .46f, .01f)) return true;
    return false;
}
const UStaticMeshComponent* ClubOf(const AMemoriaFieldMonster* Warden)
{
    if (!Warden) return nullptr;
    TArray<UStaticMeshComponent*> Meshes;
    Warden->GetComponents<UStaticMeshComponent>(Meshes);
    for (const UStaticMeshComponent* Mesh : Meshes)
        if (Mesh && Mesh->IsVisible() && FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, .46f, .01f)) return Mesh;
    return nullptr;
}
bool WearsDarkCoat(const AMemoriaFieldMonster* Warden)
{
    USkeletalMeshComponent* Body = Warden && Warden->GetFigure() ? Warden->GetFigure()->GetSkeletalMesh() : nullptr;
    const USkeletalMesh* Asset = Body ? Body->GetSkeletalMeshAsset() : nullptr;
    if (!Asset) return false;
    bool bShieldGone = false, bCoat = false, bFace = false;
    const TArray<FSkeletalMaterial>& Slots = Asset->GetMaterials();
    for (int32 I = 0; I < Slots.Num(); ++I)
    {
        const FString Name = Slots[I].MaterialSlotName.ToString() + Slots[I].ImportedMaterialSlotName.ToString() + GetNameSafe(Slots[I].MaterialInterface);
        if (Name.Contains(TEXT("Gear"))) bShieldGone = !Body->IsMaterialSectionShown(I, 0);
        else if (auto* Mid = Cast<UMaterialInstanceDynamic>(Body->GetMaterial(I)))
        {
            const FLinearColor Tint = Mid->K2_GetVectorParameterValue(TEXT("SurfaceTint"));
            if (Name.Contains(TEXT("Body")))
            {
                const FLinearColor Color = Mid->K2_GetVectorParameterValue(TEXT("Color"));
                const UMaterialInterface* Parent = Mid->Parent;
                // SurfaceTint barely darkens this coat. The body slot has to wear the flat dark cloth.
                bCoat = Parent && Parent->GetName().Contains(TEXT("FieldFoe")) && Color.R < .08f && Color.G < .08f && Color.B < .08f;
            }
            if (Name.Contains(TEXT("Face"))) bFace = Tint.R > .8f;
        }
    }
    return bShieldGone && bCoat && bFace;
}
void PlaceForTouch(AMemoriaFieldMonster* Warden, const FVector& PlayerLocation)
{
    Warden->SetActorLocation(PlayerLocation + FVector(80.f, 0.f, 0.f), false);
    if (UMemoriaFieldCharacterComponent* Figure = Warden->GetFigure())
        Figure->SetAim((PlayerLocation - Warden->GetActorLocation()).Rotation().Yaw);
}
void HideNearbyBodies(UWorld* World, const AActor* Keep, const AActor* Player)
{
    if (!Keep) return;
    const FVector At = Keep->GetActorLocation();
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor == Keep || Actor == Player) continue;
        if (FVector::Dist2D(Actor->GetActorLocation(), At) > 500.f) continue;
        if (Actor->FindComponentByClass<USkeletalMeshComponent>()) Actor->SetActorHiddenInGame(true);
    }
}
// Close enough that the coat and the short club fill the frame, from the side so the reach reads as a push.
ACameraActor* FrameWarden(UWorld* World, APlayerController* PC, const AMemoriaFieldMonster* Warden, ACameraActor* Camera)
{
    if (!World || !PC || !Warden) return Camera;
    const float Yaw = Warden->GetFigure() ? Warden->GetFigure()->GetYaw() : 0.f;
    const FVector Focus = Warden->GetActorLocation() + FVector(0, 0, Warden->Spec().Height * .52f);
    const FVector Side = FRotator(0.f, Yaw + 50.f, 0.f).Vector();
    const FVector Eye = Focus - Side * 640.f + FVector(0, 0, 170.f);
    if (!Camera)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Camera = World->SpawnActor<ACameraActor>(Eye, (Focus - Eye).Rotation(), Params);
        if (UCameraComponent* Cam = Camera ? Camera->GetCameraComponent() : nullptr)
        {
            Cam->SetFieldOfView(34.f);
            // A dark scene would open the exposure until the coat reads as white. Hold it still.
            Cam->PostProcessBlendWeight = 1.f;
            FPostProcessSettings& PP = Cam->PostProcessSettings;
            PP.bOverride_AutoExposureMethod = true;
            PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
            PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
            PP.AutoExposureApplyPhysicalCameraExposure = false;
            PP.bOverride_AutoExposureBias = true;
            PP.AutoExposureBias = 0.15f;
        }
        if (Camera)
        {
            auto* Lamp = NewObject<UPointLightComponent>(Camera, TEXT("WardenFill"));
            Lamp->SetupAttachment(Camera->GetRootComponent());
            Lamp->SetRelativeLocation(FVector(40.f, -180.f, 70.f));
            Lamp->SetMobility(EComponentMobility::Movable);
            Lamp->SetIntensityUnits(ELightUnits::Candelas);
            Lamp->SetIntensity(280.f);
            Lamp->SetAttenuationRadius(2400.f);
            Lamp->SetCastShadows(false);
            Lamp->SetLightColor(FLinearColor(.9f, .86f, .76f));
            Lamp->RegisterComponent();
        }
    }
    if (!Camera) return nullptr;
    Camera->SetActorLocation(Eye);
    Camera->SetActorRotation((Focus - Eye).Rotation());
    PC->SetViewTargetWithBlend(Camera, 0.f);
    return Camera;
}

// S376: the adjuster's club. A touch makes the hand forget. It does not cut.
// Another touch lengthens the forgetting up to the cap. Attack and the burn picker stop; dodge does not.
class FWardenNumbReplay final : public IAutomationLatentCommand
{
public:
    explicit FWardenNumbReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FWardenNumbReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.0) { Test->AddError(FString::Printf(TEXT("Warden numb timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || !Run || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        AMemoriaFieldMonster* W = Warden.Get();
        if (Phase == 0)
        {
            HideElia(World);
            const bool bOpened = Combat->OpenBurnPicker();
            if (bOpened) Combat->CloseBurnPicker();
            Test->TestTrue(TEXT("The burn picker opens before the touch"), bOpened && !Combat->IsPickingBurn());
            const TArray<AMemoriaFieldMonster*> Spawned = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(80.f, 0.f, 0.f), 0.f, EMemoriaFoeKind::Warden);
            if (!Test->TestTrue(TEXT("One warden stands in reach"), Spawned.Num() == 1 && Spawned[0])) return true;
            Warden = Spawned[0]; W = Warden.Get();
            W->SetActorLocation(Pawn->GetActorLocation() + FVector(80.f, 0.f, 0.f), false);
            W->Stun(60.f);
            const UStaticMeshComponent* Blade = W->GetFigure() ? W->GetFigure()->GetBlade() : nullptr;
            Test->TestTrue(TEXT("The coated man carries a short club, not the dagger"), HasClub(W) && (!Blade || !Blade->IsVisible()));
            Test->TestTrue(TEXT("A dark coat, and the round shield stays with the thief"), WearsDarkCoat(W));
            if (const UStaticMeshComponent* Club = ClubOf(W))
            {
                const FVector Held = Club->GetComponentLocation() - W->GetActorLocation();
                Test->TestTrue(TEXT("At rest the club is low in front, not raised"), Held.Z > W->Spec().Height * .25f && Held.Z < W->Spec().Height * .6f && FMath::Abs(Club->GetComponentRotation().Pitch) < 8.f);
            }
            if (UMemoriaFieldCharacterComponent* Figure = W->GetFigure()) Figure->SetAim(0.f);
            const int32 Strikes = Combat->GetStrikesTaken();
            Test->TestFalse(TEXT("A club held the wrong way does not touch"), Combat->StrikePlayer(W, W->Spec().Damage));
            Test->TestTrue(TEXT("A miss leaves the hand usable"), !Combat->IsNumbed() && Combat->GetStrikesTaken() == Strikes);
            PlaceForTouch(W, Pawn->GetActorLocation());
            const int64 Hp = Combat->GetPlayerHp();
            const int32 Impacts = Combat->GetImpactsMade();
            Test->TestTrue(TEXT("The reach touches"), Combat->StrikePlayer(W, W->Spec().Damage));
            Test->TestTrue(TEXT("The touch draws no blood"), Combat->GetHurtAge() > 1.f && Combat->GetImpactsMade() == Impacts && Combat->GetPlayerHp() == Hp && !Combat->IsPoisoned() && !Combat->IsScorched());
            Test->TestEqual(TEXT("One touch starts the forgetting"), Combat->GetNumbLeft(), NumbTouch, .001f);
            Test->TestEqual(TEXT("The touch is counted, and the cut is not"), Combat->GetStrikesTaken(), Strikes + 1);
            Test->TestTrue(TEXT("The hand can still walk"), Combat->CanMove() && Combat->GetComboStep() < 0);
            Combat->BeginCharge();
            Test->TestFalse(TEXT("The basic attack does nothing while the hand has forgotten"), Combat->RequestAttack(W->GetActorLocation()) || Combat->GetCharge() > 0.f);
            Test->TestFalse(TEXT("The burn picker cannot open"), Combat->OpenBurnPicker());
            Test->TestTrue(TEXT("The first touch says what happened"), Combat->GetNumbNoticeAge() < .05f
                && Combat->GetNumbNotice() == MemoriaKorean::For(Combat, TEXT("The hand forgets what it was doing.")));
            if (Combat->IsEliaSkillUnlocked(0))
                Test->TestTrue(TEXT("Elia's skill still answers"), Combat->UseEliaSkill(0));
            else Test->AddInfo(TEXT("Elia's skills are still locked on this slice; the dodge covers that the hand is not the only act."));
            Test->TestTrue(TEXT("Dodge is still allowed"), Combat->RequestDash(FVector(-1.f, 0.f, 0.f)));
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame < Mark + 60 || !W) return false;
            const float Before = Combat->GetNumbLeft();
            Test->TestTrue(TEXT("One second of forgetting has passed"), FMath::IsNearlyEqual(Before, NumbTouch - 1.f, .08f));
            PlaceForTouch(W, Pawn->GetActorLocation());
            const float Notice = Combat->GetNumbNoticeAge();
            Test->TestTrue(TEXT("The second touch lands"), Combat->StrikePlayer(W, W->Spec().Damage));
            const float After = Combat->GetNumbLeft();
            Test->TestEqual(TEXT("Another touch refreshes up to the cap"), After, FMath::Min(NumbCap, Before + NumbTouch), .001f);
            Test->TestTrue(TEXT("The forgetting does not stack past the cap"), After > Before && After <= NumbCap + .001f);
            Test->TestTrue(TEXT("A later touch does not tell the hand again"), Combat->GetNumbNoticeAge() > .4f && FMath::IsNearlyEqual(Combat->GetNumbNoticeAge(), Notice, .02f));
            Phase = 2; Mark = Frame; return false;
        }
        if (Frame < Mark + FMath::CeilToInt(NumbCap * 60.f) + 10) return false;
        Test->TestFalse(TEXT("The hand remembers when the time is gone"), Combat->IsNumbed());
        Test->TestTrue(TEXT("The basic attack works again"), W && Combat->RequestAttack(W->GetActorLocation()));
        return true;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Warden;
};

// The same club, in the field: the warden reaches, the hand forgets, and the picture is taken while it lasts.
class FWardenNumbVisualReplay final : public IAutomationLatentCommand
{
public:
    explicit FWardenNumbVisualReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FWardenNumbVisualReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.0) { Test->AddError(TEXT("Warden visual timeout")); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        if (Phase == 0)
        {
            if (!Warden.IsValid())
            {
                HideElia(World);
                const TArray<AMemoriaFieldMonster*> Spawned = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(80.f, 0.f, 0.f), 0.f, EMemoriaFoeKind::Warden);
                if (!Test->TestTrue(TEXT("A warden enters the field"), Spawned.Num() == 1 && Spawned[0])) return true;
                Warden = Spawned[0];
                Warden->SetActorLocation(Pawn->GetActorLocation() + FVector(80.f, 0.f, 0.f), false);
                if (UMemoriaFieldCharacterComponent* Figure = Warden->GetFigure())
                    Figure->SetAim((Pawn->GetActorLocation() - Warden->GetActorLocation()).Rotation().Yaw);
                HideNearbyBodies(World, Warden.Get(), Pawn);
                Pawn->SetActorHiddenInGame(true);
                Camera = FrameWarden(World, PC, Warden.Get(), Camera.Get());
                Test->TestTrue(TEXT("The field warden wears the dark coat without the shield"), WearsDarkCoat(Warden.Get()));
                Mark = Frame; return false;
            }
            Camera = FrameWarden(World, PC, Warden.Get(), Camera.Get());
            if (Frame < Mark + 3) return false;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S376Warden/WardenRest.png"), true, false);
            Phase = 1; Mark = Frame; return false;
        }
        AMemoriaFieldMonster* W = Warden.Get();
        if (!bSawTouch && W && W->GetState() == EMemoriaMonsterState::Windup)
        {
            const float Push = W->GetStateTime() / FMath::Max(W->Spec().Windup, SMALL_NUMBER);
            if (Push >= .55f && Push <= .92f)
            {
                bSawTouch = true;
                const UStaticMeshComponent* Club = ClubOf(W);
                Test->TestTrue(TEXT("The club is still in his hand at the touch"), Club != nullptr);
                if (Club)
                {
                    const FVector Held = Club->GetComponentLocation() - W->GetActorLocation();
                    const FVector Forward = FRotator(0.f, W->GetFigure() ? W->GetFigure()->GetYaw() : 0.f, 0.f).Vector();
                    Test->TestTrue(TEXT("The touch is a straight push, kept at the waist"), FVector::DotProduct(Held, Forward) > 50.f
                        && Held.Z > W->Spec().Height * .25f && Held.Z < W->Spec().Height * .6f && FMath::Abs(Club->GetComponentRotation().Pitch) < 8.f);
                }
                Camera = FrameWarden(World, PC, W, Camera.Get());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S376Warden/WardenTouch.png"), true, false);
            }
        }
        if (!bSawReach && W && W->GetState() == EMemoriaMonsterState::Windup)
        {
            const FMemoriaAttackCue Cue = W->GetAttackCue();
            if (Cue.bActive && Cue.bReach)
            {
                bSawReach = true;
                Test->TestEqual(TEXT("The warning is the short corridor"), Cue.Length, W->Spec().Reach + 40.f, .1f);
                Test->TestEqual(TEXT("The warning is as narrow as the club"), Cue.Radius, WardenReachWidth, .1f);
            }
        }
        if (!Combat->IsNumbed())
        {
            if (Frame > Mark + 600) { Test->AddError(TEXT("The warden never numbed Arrel")); return true; }
            return false;
        }
        if (!bCaptured)
        {
            Test->TestTrue(TEXT("The warden's own reach did it"), bSawReach && bSawTouch && W && W->GetStrikeCount() >= 1);
            Test->TestTrue(TEXT("While numb he can walk and cannot cut"), Combat->CanMove() && !Combat->RequestAttack(Pawn->GetActorLocation() + FVector(80.f, 0.f, 0.f)));
            if (W) Camera = FrameWarden(World, PC, W, Camera.Get());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S376Warden/WardenNumb.png"), true, false);
            Test->TestTrue(TEXT("Dodge still answers after the picture"), Combat->RequestDash(FVector(0.f, -1.f, 0.f)));
            bCaptured = true; Mark = Frame;
        }
        return Frame > Mark + 20;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false, bSawReach = false, bSawTouch = false, bCaptured = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Warden;
    TWeakObjectPtr<ACameraActor> Camera;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenNumbTest, "Memoria.S376.Warden", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardenNumbTest::RunTest(const FString&)
{
    const FMemoriaFoeSpec& Spec = FoeSpec(EMemoriaFoeKind::Warden);
    TestEqual(TEXT("Korean name"), FString(Spec.NameKo), FString(TEXT("감시자")));
    TestEqual(TEXT("The table says the hand forgot"), MemoriaKorean::Text(TEXT("Numbed")), FString(TEXT("손이 잊었다")));
    TestEqual(TEXT("The table says what the hand forgot"), MemoriaKorean::Text(TEXT("The hand forgets what it was doing.")), FString(TEXT("손이 하던 일을 잊었다.")));
    TestEqual(TEXT("The table names the man"), MemoriaKorean::Text(TEXT("Warden")), FString(TEXT("감시자")));
    TestEqual(TEXT("No cut"), Spec.Damage, 0.f, .001f);
    TestEqual(TEXT("A slow man"), Spec.Speed, MemoriaCombatTuning::WardenSpeed, .001f);
    TestEqual(TEXT("A short reach"), Spec.Reach, MemoriaCombatTuning::WardenReach, .001f);
    TestEqual(TEXT("A long windup"), Spec.Windup, MemoriaCombatTuning::WardenWindup, .001f);
    TestTrue(TEXT("The blow is a reach that numbs"), Spec.bReach && Spec.Ability == EMemoriaFoeAbility::Numb && Spec.Second == EMemoriaFoeAbility::None && !Spec.Strike);
    TestEqual(TEXT("An existing coated body"), FString(Spec.Model), FString(TEXT("BeltScavenger")));
    TestTrue(TEXT("Named Warden, and not placed by a chapter"), FoeKindByName(TEXT("Warden"), false) == EMemoriaFoeKind::Warden);
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FWardenNumbReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenNumbVisualTest, "MemoriaVisual.S376Warden", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWardenNumbVisualTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FWardenNumbVisualReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
