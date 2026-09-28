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
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S313: the field foes on the mannequin stand-in. A void husk shambles on UAL2's zombie clips in the void
// material and scratches; a market thief (Quinn) runs in fast with a short blade and cuts. Close captures
// of each, then both chasing Arrel from the game camera.
class FFieldFoesReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldFoesReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldFoesReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field foes timeout in phase %d"), Phase)); return true; }
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
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldFoes") / (FString(Name) + TEXT(".png")), true, false); };
        auto Look = [&](const AActor* At, float Yaw)
        {
            if (!Camera.IsValid()) Camera = World->SpawnActor<ACameraActor>();
            const FVector Center = At->GetActorLocation();
            const FVector Eye = Center + FRotator(0, Yaw, 0).Vector() * 320.f + FVector(0, 0, 130.f);
            Camera->SetActorLocation(Eye); Camera->SetActorRotation((Center + FVector(0, 0, 85.f) - Eye).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(50.f); PC->SetViewTarget(Camera.Get());
        };
        auto UsesFoeMaterial = [](const AMemoriaFieldMonster* M)
        {
            const USkeletalMeshComponent* Mesh = M ? M->GetFigure()->GetSkeletalMesh() : nullptr;
            if (!Mesh || Mesh->GetNumMaterials() == 0) return false;
            for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I)
            {
                const auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                if (!Material || !Material->Parent || Material->Parent->GetName() != TEXT("M_FieldFoe")) return false;
            }
            return true;
        };
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* H = Husk.Get(); AMemoriaFieldMonster* T = Thief.Get();
        if (Phase == 0)
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            for (const TCHAR* Clip : MemoriaCombatClips::FoeClips())
                Test->TestNotNull(*FString::Printf(TEXT("Foe clip %s"), Clip), MemoriaCombatClips::Load(TEXT("Mannequin"), Clip));
            // The husk comes from the east and the thief from further north: the thief still strikes first.
            Husk = Combat->SpawnWave(1, Me + FVector(460, 0, 0), 0.f, EMemoriaFoeKind::VoidHusk)[0];
            Thief = Combat->SpawnWave(1, Me + FVector(0, 560, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame == Mark + 4 && H && T)
            {
                Test->TestTrue(TEXT("The husk is a void husk"), H->GetKind() == EMemoriaFoeKind::VoidHusk && H->GetMaxHealth() == FoeSpec(EMemoriaFoeKind::VoidHusk).Health);
                Test->TestTrue(TEXT("The thief is a market thief"), T->GetKind() == EMemoriaFoeKind::MarketThief && T->GetMaxHealth() == FoeSpec(EMemoriaFoeKind::MarketThief).Health);
                Test->TestTrue(TEXT("The husk wears the foe material (not the default)"), UsesFoeMaterial(H));
                Test->TestTrue(TEXT("The thief wears the foe material"), UsesFoeMaterial(T));
                const auto* HuskAnim = Cast<UMemoriaFieldAnimInstance>(H->GetFigure()->GetSkeletalMesh()->GetAnimInstance());
                Test->TestTrue(TEXT("The husk shambles on the zombie clips"), HuskAnim && HuskAnim->Idle == MemoriaCombatClips::Load(TEXT("Mannequin"), TEXT("Zombie_Idle_Loop")) &&
                    HuskAnim->Walk == MemoriaCombatClips::Load(TEXT("Mannequin"), TEXT("Zombie_Walk_Fwd_Loop")));
                Test->TestTrue(TEXT("The thief carries a blade, the husk none"), T->GetFigure()->GetBlade() != nullptr && H->GetFigure()->GetBlade() == nullptr);
                Test->TestTrue(TEXT("The thief is Quinn"), T->GetFigure()->GetSkeletalMesh()->GetSkeletalMeshAsset()->GetName().Contains(TEXT("Quinn")));
                Look(H, (Me - H->GetActorLocation()).Rotation().Yaw + 25.f);
            }
            // Close captures while each closes in: the husk's shamble, the thief's run with the blade.
            if (Frame == Mark + 22) Capture(TEXT("HuskApproach"));
            // From Arrel's side of each foe, so the camera never sits inside the market's walls.
            if (Frame == Mark + 26 && T) Look(T, (Me - T->GetActorLocation()).Rotation().Yaw + 25.f);
            if (Frame == Mark + 44) Capture(TEXT("ThiefApproach"));
            if (Frame < Mark + 48) return false;
            PC->SetViewTarget(Pawn);
            Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 30)
            {
                Test->TestTrue(TEXT("Both give chase"), H && T && H->GetState() != EMemoriaMonsterState::Idle && T->GetState() != EMemoriaMonsterState::Idle);
                Capture(TEXT("FoesChase"));
            }
            if (T && T->GetState() == EMemoriaMonsterState::Windup && !bThiefStrike)
            {
                bThiefStrike = true;
                Test->TestTrue(TEXT("The thief cuts with the blade clip"), T->GetFigure()->GetActionClip() == MemoriaCombatClips::Load(TEXT("Mannequin"), TEXT("Sword_Regular_A")));
                Look(T, 60.f); ThiefFrame = Frame;
            }
            if (bThiefStrike && Frame == ThiefFrame + 18) Capture(TEXT("ThiefStrike"));
            if (bThiefStrike && Frame == ThiefFrame + 22) PC->SetViewTarget(Pawn);
            if (H && H->GetState() == EMemoriaMonsterState::Windup && !bHuskStrike)
            {
                bHuskStrike = true;
                Test->TestTrue(TEXT("The husk scratches"), H->GetFigure()->GetActionClip() == MemoriaCombatClips::Load(TEXT("Mannequin"), TEXT("Zombie_Scratch")));
                HuskFrame = Frame;
                if (!bThiefStrike || Frame > ThiefFrame + 22) Look(H, -40.f);
            }
            if (bHuskStrike && Frame == HuskFrame + 30) { Capture(TEXT("HuskStrike")); }
            if (bHuskStrike && bThiefStrike && Frame > FMath::Max(HuskFrame, ThiefFrame) + 34)
            {
                // The thief started further off yet struck first: it closes faster than the husk.
                Test->TestTrue(TEXT("The thief outran the husk"), ThiefFrame < HuskFrame);
                Test->TestTrue(TEXT("Arrel was struck"), Combat->GetPlayerHp() < Combat->GetPlayerMaxHp());
                PC->SetViewTarget(Pawn);
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The foes never struck")); return true; }
            return false;
        }
        if (Phase == 3 && Frame == Mark + 4)
        {
            // A material that fails to compile silently renders as the engine default; require a clean compile.
            auto* Foe = Cast<UMaterialInstanceDynamic>(H ? H->GetFigure()->GetSkeletalMesh()->GetMaterial(0) : nullptr);
            const FMaterialResource* Resource = Foe ? Foe->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
            Test->TestTrue(TEXT("M_FieldFoe compiles for this platform"), Resource && Resource->GetCompileErrors().IsEmpty() && Resource->GetMaterialInterface() &&
                Resource->GetMaterialInterface()->GetName() == TEXT("M_FieldFoe"));
            if (Resource) for (const FString& Error : Resource->GetCompileErrors()) Test->AddError(Error);
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, ThiefFrame = 0, HuskFrame = 0;
    bool bFixed = false, bOldFixed = false, bThiefStrike = false, bHuskStrike = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Husk, Thief;
    TWeakObjectPtr<ACameraActor> Camera;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldFoesTest, "MemoriaVisual.FieldFoes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldFoesTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldFoesReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
