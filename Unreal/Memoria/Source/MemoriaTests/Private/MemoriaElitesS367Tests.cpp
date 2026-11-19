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
#include "Interaction/MemoriaEliaCompanion.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S367 void-touched elites, with real key presses in the Verdan slice. The schedule tightens with burns. An
// elite thief rises named, taller and stronger, wearing a violet shell and a breathing light. A plain cut
// grazes it (a third, no stagger); a held J heavy cut breaks the shell (full harm, the foe reels), and the
// fallen elite pays three times the grains and always leaves an item.
class FElitesS367Replay final : public IAutomationLatentCommand
{
public:
    explicit FElitesS367Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FElitesS367Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        using namespace MemoriaCombatTuning;
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Elite timeout in phase %d"), Phase)); return true; }
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
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        if (!Combat || !Run || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S367Elite") / (FString(Name) + TEXT(".png")), true, false); };
        auto Key = [&](const FKey& K, EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, E, E == IE_Pressed ? 1.f : 0.f)); };
        const FVector Me = Pawn->GetActorLocation();
        const FMemoriaFoeSpec& ThiefSpec = FoeSpec(EMemoriaFoeKind::MarketThief);
        if (Phase == 0)
        {
            if (Frame < 20) return false;
            // The schedule: never in the first two fights; every 5th unburned, every 3rd at 4 burns, every 2nd at 6+.
            Test->TestFalse(TEXT("No elite in the first fights"), EliteWave(1, 10) || EliteWave(2, 10));
            Test->TestTrue(TEXT("Unburned: the 5th fight"), EliteWave(5, 0) && !EliteWave(4, 0) && !EliteWave(3, 0));
            Test->TestTrue(TEXT("Four burns: every 3rd"), EliteWave(3, 4) && EliteWave(6, 4) && !EliteWave(4, 4));
            Test->TestTrue(TEXT("Many burns: every 2nd"), EliteWave(4, 6) && EliteWave(6, 12) && !EliteWave(5, 12));
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Elite = Combat->SpawnWave(1, Me + FVector(160, 0, 0), 0.f, EMemoriaFoeKind::MarketThief, 1)[0];
            AMemoriaFieldMonster* E = Elite.Get();
            Test->TestTrue(TEXT("The wave's first foe is an elite"), E && E->IsElite() && E->IsShelled());
            Test->TestTrue(TEXT("It is stronger"), E && FMath::IsNearlyEqual(E->GetMaxHealth(), ThiefSpec.Health * EliteHealth, .01f));
            Test->TestTrue(TEXT("And taller"), E && FMath::IsNearlyEqual(E->GetFigure()->GetWorldHeight(), ThiefSpec.Height * EliteScale, .5f));
            const USkeletalMeshComponent* Body = E ? E->GetFigure()->GetSkeletalMesh() : nullptr;
            Test->TestTrue(TEXT("It wears the violet shell"), Body && Body->GetOverlayMaterial() != nullptr);
            bool bLight = false;
            if (E) { TArray<UPointLightComponent*> Lights; E->GetComponents(Lights); bLight = Lights.Num() > 0; }
            Test->TestTrue(TEXT("And a breathing light"), bLight);
            Phase = 1; Mark = Frame; return false;
        }
        AMemoriaFieldMonster* E = Elite.Get();
        if (!E) { Test->AddError(TEXT("No elite")); return true; }
        if (Phase == 1)
        {
            if (Frame == Mark + 24) Capture(TEXT("EliteShelled"));
            // A plain cut, a tap of J, once the elite is in reach.
            if (Frame > Mark + 24 && !bPlainSent && FVector::Dist2D(Me, E->GetActorLocation()) < AttackRange - 20.f && !Combat->IsAttacking())
            {
                bPlainSent = true; HpBefore = E->GetHealth(); HitsBefore = Combat->GetHitsLanded();
                Combat->RequestAttack(E->GetActorLocation()); Key(EKeys::J, IE_Pressed); Key(EKeys::J, IE_Released);
            }
            if (Frame > Mark + 24 && !bPlainSent && Frame % 30 == 0) Pawn->SetActorLocation(E->GetActorLocation() - FVector(120, 0, 0));
            if (bPlainSent && Combat->GetHitsLanded() > HitsBefore)
            {
                Test->TestTrue(TEXT("A plain cut only grazes the shell"), FMath::IsNearlyEqual(HpBefore - E->GetHealth(), ComboDamage[0] * EliteShellFactor, .05f));
                Test->TestTrue(TEXT("And cannot stop it"), E->GetState() != EMemoriaMonsterState::Stagger);
                Test->TestTrue(TEXT("The shell holds"), E->IsShelled() && Combat->GetShellsBroken() == 0);
                Phase = 2; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The plain cut never landed")); return true; }
            return false;
        }
        if (Phase == 2)
        {
            // Hold J for a heavy cut once the plain one is over.
            if (!bHeavySent && !Combat->IsAttacking() && Frame > Mark + 10)
            {
                bHeavySent = true; HpBefore = E->GetHealth(); HitsBefore = Combat->GetHitsLanded();
                HeavyExpected = HeavyDamage * (Combat->IsWeakened() ? WeakenFactor : 1.f);   // the thief's blow may have weakened Arrel
                Pawn->SetActorLocation(E->GetActorLocation() - FVector(120, 0, 0));
                // Held still for the charge (a fixture aid: a blow mid-charge would cancel the heavy cut).
                E->Stun(2.f);
                Combat->RequestAttack(E->GetActorLocation()); Key(EKeys::J, IE_Pressed);
            }
            // Pressing J swings an ordinary cut first (it grazes the shell); measure from the heavy cut's start.
            if (bHeavySent && Combat->IsHeavy() && !bHeld)
            { bHeld = true; HpBefore = E->GetHealth(); HeavyExpected = HeavyDamage * (Combat->IsWeakened() ? WeakenFactor : 1.f); }
            if (bHeld && !bBroken && !E->IsShelled())
            {
                bBroken = true; BrokenFrame = Frame;
                Key(EKeys::J, IE_Released);
                Test->TestTrue(TEXT("A heavy cut breaks the shell"), Combat->GetShellsBroken() == 1);
                Test->TestTrue(FString::Printf(TEXT("At full harm (%.2f, expected %.2f)"), HpBefore - E->GetHealth(), HeavyExpected), FMath::IsNearlyEqual(HpBefore - E->GetHealth(), HeavyExpected, .05f));
                Test->TestTrue(TEXT("And the elite reels"), E->GetState() == EMemoriaMonsterState::Stagger);
                Test->TestTrue(TEXT("The shell's glow is gone"), E->GetFigure()->GetSkeletalMesh() && E->GetFigure()->GetSkeletalMesh()->GetOverlayMaterial() == nullptr);
            }
            if (bBroken && Frame == BrokenFrame + 3) Capture(TEXT("EliteShellBroken"));
            if (bBroken && Frame == BrokenFrame + 30)
            {
                GrainsBefore = Run->GetRunSnapshot().Player.Grains;
                E->TakeHit(999.f, Me);
                const int64 Expected = ((ThiefSpec.bVoid ? 8 : 3) + int64(ThiefSpec.Health) / 20) * EliteGrainsFactor;
                Test->TestEqual(TEXT("A fallen elite pays three times the grains"), Run->GetRunSnapshot().Player.Grains - GrainsBefore, Expected);
                Test->TestTrue(TEXT("And always leaves an item"), !Combat->GetLastReward().ItemId.IsEmpty());
                return true;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The shell never broke")); return true; }
            return false;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, HitsBefore = 0, BrokenFrame = 0;
    int64 GrainsBefore = 0;
    float HpBefore = 0.f, HeavyExpected = 0.f;
    bool bFixed = false, bOldFixed = false, bPlainSent = false, bHeavySent = false, bHeld = false, bBroken = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Elite;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElitesS367Test, "MemoriaVisual.S367Elite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FElitesS367Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FElitesS367Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
