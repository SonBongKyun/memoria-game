#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S315: the feel of a blow and the two new moves, with real key presses.
// A cut lands: the world stops for a beat, the foe flashes, sparks fly, the camera shakes and the blade
// leaves a trail. Holding J spins a heavy cut that reaches a foe behind Arrel. K raised just before the
// thief's blow parries it (no harm, the thief reels); raised early, it only blocks (30%, no curse).
class FFieldFeelReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldFeelReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldFeelReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Field feel timeout in phase %d"), Phase)); return true; }
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
        // Frames, not world time: the hit stop nearly freezes world time on purpose.
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) return false;
        ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldFeel") / (FString(Name) + TEXT(".png")), true, false); };
        auto Key = [&](const FKey& K, EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, E, E == IE_Pressed ? 1.f : 0.f)); };
        const float Dilation = World->GetWorldSettings()->TimeDilation;
        const FVector Me = Pawn->GetActorLocation();
        if (Phase == 0)
        {
            if (Frame < 20) return false;
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Target = Combat->SpawnWave(1, Me + FVector(140, 0, 0), 0.f)[0];
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            AMemoriaFieldMonster* H = Target.Get();
            if (Frame == Mark + 3 && H)
            {
                // Aimed at the husk first (automation has no cursor), then a real tap of J: pressed and
                // released at once, so no heavy cut.
                Combat->RequestAttack(H->GetActorLocation());
                Key(EKeys::J, IE_Pressed); Key(EKeys::J, IE_Released);
            }
            if (!bLanded && H && Frame > Mark + 3 && (Frame - Mark) % 8 == 0) Combat->RequestAttack(H->GetActorLocation());
            if (Combat->GetTrail().Num() >= 4 && !bTrailShot) { bTrailShot = true; Capture(TEXT("FeelTrail")); }
            if (!bLanded && Combat->GetHitsLanded() > 0)
            {
                bLanded = true; LandFrame = Frame;
                Test->TestTrue(TEXT("The blow stops the world for a beat"), Combat->IsHitStopped() && Dilation < .2f);
                Test->TestTrue(TEXT("The foe flashes"), H && H->GetHitFlash() > .5f);
                Test->TestTrue(TEXT("Sparks fly"), Combat->GetSparks().Num() > 0);
                Test->TestTrue(TEXT("The camera shakes"), !Combat->GetShakeOffset().IsNearlyZero());
                Test->TestTrue(TEXT("The blade leaves a trail"), Combat->GetTrail().Num() > 1);
                Capture(TEXT("FeelImpact"));
            }
            if (bLanded && Frame == LandFrame + 15)
            {
                Test->TestFalse(TEXT("The stop is only a beat"), Combat->IsHitStopped());
                Test->TestTrue(TEXT("Time runs again"), FMath::IsNearlyEqual(Dilation, 1.f, .001f));
                if (H) H->TakeHit(999.f, Me);
                // A foe on each side for the heavy cut: ahead and behind.
                // Far enough that they reach Arrel only after the heavy cut has spun.
                const auto Ahead = Combat->SpawnWave(1, Me + FVector(190, 0, 0), 0.f);
                const auto Behind = Combat->SpawnWave(1, Me + FVector(-190, 0, 0), 0.f);
                Front = Ahead[0]; Back = Behind[0];
                Phase = 2; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The cut never landed")); return true; }
            return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 30) { Combat->RequestAttack(Me + FVector(100, 0, 0)); Key(EKeys::J, IE_Pressed); }
            if (Frame == Mark + 30 + int32(MemoriaCombatTuning::ChargeTime * 60.f * .6f)) Capture(TEXT("FeelCharge"));
            if (Combat->IsHeavy() && !bHeavy) { bHeavy = true; HeavyFrame = Frame; }
            if (bHeavy && Combat->GetTrail().Num() >= 5 && !bHeavyShot) { bHeavyShot = true; Capture(TEXT("FeelHeavy")); }
            if (bHeavy && !Combat->IsHeavy() && !bHeavyDone)
            {
                bHeavyDone = true; Key(EKeys::J, IE_Released);
                AMemoriaFieldMonster* F = Front.Get(); AMemoriaFieldMonster* B = Back.Get();
                Test->TestTrue(TEXT("Holding J spins the heavy cut"), bHeavy);
                Test->TestTrue(TEXT("It reaches the foe behind Arrel"), B && B->GetHealth() <= B->GetMaxHealth() - MemoriaCombatTuning::HeavyDamage + .01f);
                Test->TestTrue(TEXT("And the one ahead"), F && F->GetHealth() < F->GetMaxHealth());
                for (AMemoriaFieldMonster* M : {F, B}) if (M && !M->IsDead()) M->TakeHit(999.f, Me);
                Thief = Combat->SpawnWave(1, Me + FVector(150, 0, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("No heavy cut")); return true; }
            return false;
        }
        if (Phase == 3)
        {
            AMemoriaFieldMonster* T = Thief.Get();
            if (!T) { Test->AddError(TEXT("No thief")); return true; }
            const bool Winding = T->GetState() == EMemoriaMonsterState::Windup;
            if (Winding && !bWasWinding) { WindupFrame = Frame; ++Windups; }
            bWasWinding = Winding;
            const int32 StrikeFrame = WindupFrame + int32(FoeSpec(EMemoriaFoeKind::MarketThief).Windup * 60.f);
            if (Windups == 1 && Winding && Frame == StrikeFrame - 8 && !bParryTried)
            {
                // Just before the blow: a parry.
                bParryTried = true; TakenBefore = Combat->GetStrikesTaken();
                Key(EKeys::K, IE_Pressed);
                Test->TestTrue(TEXT("K raises the guard"), Combat->IsBlocking() && !Combat->CanMove());
            }
            if (bParryTried && !bParryChecked && Frame == StrikeFrame + 6)
            {
                bParryChecked = true;
                Test->TestEqual(TEXT("The blow is parried"), Combat->GetParries(), 1);
                Test->TestEqual(TEXT("A parry takes no harm"), Combat->GetStrikesTaken(), TakenBefore);
                Test->TestTrue(TEXT("The thief reels"), T->GetState() == EMemoriaMonsterState::Stagger);
                Test->TestFalse(TEXT("No curse through a parry"), Combat->IsWeakened());
                Capture(TEXT("FeelParry"));
            }
            if (bParryChecked && Frame == StrikeFrame + 12) Key(EKeys::K, IE_Released);
            if (Windups == 2 && Winding && Frame == WindupFrame + 1 && !bBlockTried)
            {
                // Raised as the windup begins, long before the blow: only a block.
                bBlockTried = true; Key(EKeys::K, IE_Pressed); HpBefore = Combat->GetPlayerHp();
            }
            if (bBlockTried && Frame == StrikeFrame + 6)
            {
                Test->TestEqual(TEXT("An early guard blocks"), Combat->GetBlocks(), 1);
                Test->TestEqual(TEXT("Still one parry"), Combat->GetParries(), 1);
                if (!Combat->IsPoisoned())
                    Test->TestTrue(TEXT("The block softens the blow to 30%"), HpBefore - Combat->GetPlayerHp() <= FMath::RoundToInt(FoeSpec(EMemoriaFoeKind::MarketThief).Damage * MemoriaCombatTuning::BlockFactor));
                Test->TestFalse(TEXT("A blocked blow brings no curse"), Combat->IsWeakened());
                Capture(TEXT("FeelBlock"));
                Key(EKeys::K, IE_Released);
                Test->TestFalse(TEXT("Releasing K lowers the guard"), Combat->IsBlocking());
                return true;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The thief never struck twice")); return true; }
            return false;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, LandFrame = 0, HeavyFrame = 0, WindupFrame = 0, Windups = 0, TakenBefore = 0;
    int64 HpBefore = 0;
    bool bFixed = false, bOldFixed = false, bLanded = false, bTrailShot = false, bHeavy = false, bHeavyShot = false, bHeavyDone = false;
    bool bWasWinding = false, bParryTried = false, bParryChecked = false, bBlockTried = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Target, Front, Back, Thief;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldFeelTest, "MemoriaVisual.FieldFeel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldFeelTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldFeelReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
