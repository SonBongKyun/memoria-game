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
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S311: Arrel fights a void husk in the Verdan field with the retargeted combo, takes a telegraphed
// strike, dodges through the next one, and the captures show the rigged motions up close.
class FFieldCombatReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldCombatReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldCombatReplay() override
    {
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field combat timeout in phase %d"), Phase)); return true; }
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
        auto* Arrel = Combat->GetPlayerFigure();
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldCombat") / (FString(Name) + TEXT(".png")), true, false); };
        const FVector Me = Pawn->GetActorLocation();
        auto CloseCamera = [&](const FVector& Look, float Yaw)
        {
            // A review camera three metres off, level with the chest, like PlayFeel's close captures.
            if (!Camera.IsValid()) Camera = World->SpawnActor<ACameraActor>();
            const FVector Eye = Look + FRotator(0, Yaw, 0).Vector() * 330.f + FVector(0, 0, 150.f);
            Camera->SetActorLocation(Eye); Camera->SetActorRotation((Look + FVector(0, 0, 90.f) - Eye).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(50.f); PC->SetViewTarget(Camera.Get());
        };
        if (Phase == 0)
        {
            Test->TestTrue(TEXT("Arrel is the rigged figure"), Arrel && Arrel->IsRigged() && Arrel->GetCharacterId() == TEXT("Arrel"));
            for (int32 Step = 0; Step < 3; ++Step)
                Test->TestNotNull(*FString::Printf(TEXT("Retargeted combo step %d"), Step + 1), MemoriaCombatClips::Load(TEXT("Arrel"), MemoriaCombatClips::Attack(Step)));
            Test->TestNotNull(TEXT("Retargeted dash"), MemoriaCombatClips::Load(TEXT("Arrel"), MemoriaCombatClips::Dash()));
            Test->TestNotNull(TEXT("Retargeted death"), MemoriaCombatClips::Load(TEXT("Arrel"), MemoriaCombatClips::Death()));
            for (const TCHAR* Clip : MemoriaCombatClips::SwordClips())
                Test->TestNotNull(*FString::Printf(TEXT("Sword clip %s"), Clip), MemoriaCombatClips::Load(TEXT("Arrel"), Clip));
            Test->TestTrue(TEXT("The sword starts sheathed"), Arrel && Arrel->HasSword() && !Arrel->IsSwordDrawn());
            StartHp = Combat->GetPlayerHp();
            // The review hides the following companion so the close captures show Arrel alone.
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Husk = Combat->SpawnWave(1, Me + FVector(260, 0, 0), 0.f)[0];
            Test->TestNotNull(TEXT("A husk rises"), Husk.Get());
            Phase = 1; Mark = Frame; return false;
        }
        AMemoriaFieldMonster* Target = Husk.Get();
        if (Phase == 1)
        {
            if (Frame == Mark + 20)
            {
                Test->TestTrue(TEXT("The husk wears Codex's model"), Target && Target->GetFigure()->IsRigged() && Target->GetFigure()->IsFoeModel() && Target->GetFigure()->GetCharacterId() == TEXT("Husk"));
                Capture(TEXT("CombatApproach"));
            }
            if (Frame < Mark + 24 || !Target) return false;
            // A real key press on J swings toward the husk (the cursor has no position in automation).
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Pressed, 1.f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Released, 0.f));
            Combat->RequestAttack(Target->GetActorLocation());
            Test->TestTrue(TEXT("The combo starts"), Combat->IsAttacking());
            Test->TestFalse(TEXT("Arrel is rooted while swinging"), Combat->CanMove());
            Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2)
        {
            // Three moments of the first swing from Arrel's front quarter: wind-up, strike, follow-through.
            if (Arrel && Arrel->IsActing() && SwingShots < 3 && Arrel->GetActionTime() > Arrel->GetActionLength() * (.2f + .25f * SwingShots))
            {
                if (SwingShots == 0) CloseCamera(Me, Arrel->GetYaw() + 35.f);
                Capture(*FString::Printf(TEXT("CombatSwing%d"), SwingShots + 1)); ++SwingShots;
            }
            if (Arrel && Arrel->IsActing() && !bSwingShot && Arrel->GetActionTime() > Arrel->GetActionLength() * .35f)
            {
                bSwingShot = true;
                // S312: Arrel's combo is the sword set's cuts when it is there.
                auto Cut = [&](int32 Step) { return MemoriaCombatClips::Load(TEXT("Arrel"), MemoriaCombatClips::ForAction(TEXT("Arrel"), MemoriaCombatClips::Attack(Step))); };
                Test->TestTrue(TEXT("Arrel plays his retargeted swing"), Arrel->GetActionClip() == Cut(0) || Arrel->GetActionClip() == Cut(1) || Arrel->GetActionClip() == Cut(2));
                Test->TestTrue(TEXT("The swing is a sword cut"), FString(Arrel->GetActionClip() ? Arrel->GetActionClip()->GetName() : FString()).Contains(TEXT("Sword_Regular")));
                Test->TestTrue(TEXT("Arrel fights with the sword drawn"), Arrel->HasSword() && Arrel->IsSwordDrawn() && Arrel->GetBlade() && Arrel->GetBlade()->IsVisible());
                SwingFrame = Frame;
            }
            // Keep pressing through the combo until the husk falls.
            if (Target && !Target->IsDead() && Frame % 6 == 0) Combat->RequestAttack(Target->GetActorLocation());
            if (Target && Target->IsDead() && !bKillShot) { bKillShot = true; KillFrame = Frame; }
            if (bKillShot && Frame == KillFrame + 40)
            {
                Test->TestEqual(TEXT("One husk slain"), Combat->GetKills(), 1);
                Test->TestTrue(TEXT("Several blows landed"), Combat->GetHitsLanded() >= 4);
                Test->TestTrue(TEXT("The combo reached its third step"), bReachedThird);
                Capture(TEXT("CombatKill"));
            }
            if (Combat->GetComboStep() == 2) bReachedThird = true;
            if (SwingShots >= 3 && Frame == SwingFrame + 40) PC->SetViewTarget(Pawn);
            if (bKillShot && Frame > KillFrame + 44)
            {
                PC->SetViewTarget(Pawn);
                Husk = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(-120, 0, 0), 0.f)[0];
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The husk never fell")); return true; }
            return false;
        }
        if (Phase == 3)
        {
            // Stand still and let the husk wind up: the telegraph shows, then the blow lands.
            if (Target && Target->GetState() == EMemoriaMonsterState::Windup && !bTelegraphShot)
            {
                bTelegraphShot = true; CloseCamera(Target->GetActorLocation(), 70.f);
                TelegraphFrame = Frame;
            }
            if (bTelegraphShot && Frame == TelegraphFrame + 20) { Capture(TEXT("CombatTelegraph")); PC->SetViewTarget(Pawn); }
            if (Target && Target->GetStrikeCount() >= 1 && !bStruck)
            {
                bStruck = true;
                Test->TestTrue(TEXT("The strike wounds Arrel"), Combat->GetPlayerHp() < StartHp);
                Phase = 4; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The husk never struck")); return true; }
            return false;
        }
        if (Phase == 4)
        {
            // Dash out of the next windup: invulnerable while dashing, and the strike misses.
            if (Target && Target->GetState() == EMemoriaMonsterState::Windup && !bDashed)
            {
                HpBeforeDash = Combat->GetStrikesTaken(); const FVector From = Pawn->GetActorLocation();
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1.f));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0.f));
                if (!Combat->IsDashing()) Combat->RequestDash(From - Target->GetActorLocation());
                Test->TestTrue(TEXT("Shift dashes"), Combat->IsDashing() && Combat->IsInvulnerable());
                bDashed = true; DashFrame = Frame; DashFrom = From;
            }
            if (bDashed && Frame == DashFrame + 8) Capture(TEXT("CombatDash"));
            if (bDashed && Frame == DashFrame + 60)
            {
                Test->TestTrue(TEXT("The dash carried Arrel away"), FVector::Dist2D(Pawn->GetActorLocation(), DashFrom) > MemoriaCombatTuning::DashDistance * .6f);
                // Counted in blows taken: the husk's poison (S314) may still tick through the dodge.
                Test->TestEqual(TEXT("The dodged strike did no harm"), int64(Combat->GetStrikesTaken()), HpBeforeDash);
                Capture(TEXT("CombatField"));
                Phase = 5; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("No windup to dodge")); return true; }
            return false;
        }
        return Phase == 5 && Frame > Mark + 6;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, SwingFrame = 0, SwingShots = 0, KillFrame = 0, TelegraphFrame = 0, DashFrame = 0;
    int64 StartHp = 0, HpBeforeDash = 0;
    bool bFixed = false, bOldFixed = false, bSwingShot = false, bKillShot = false, bReachedThird = false, bTelegraphShot = false, bStruck = false, bDashed = false;
    FVector DashFrom = FVector::ZeroVector;
    TWeakObjectPtr<AMemoriaFieldMonster> Husk;
    TWeakObjectPtr<ACameraActor> Camera;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldCombatTest, "MemoriaVisual.FieldCombat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldCombatTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldCombatReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
