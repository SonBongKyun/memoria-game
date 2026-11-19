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
#include "Interaction/MemoriaEliaCompanion.h"
#include "Materials/Material.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S366 Afterimage, with real key presses in the Verdan slice. Shift just before the thief's blow: the blow
// falls on Arrel's afterimage (no harm, no curse), the thief reels, the world slows and drains, and the ghost
// flares. Arrel's next cut is an Echo Counter (x1.6). A dash begun long before the blow is no Afterimage, and
// the ghosts fade.
class FAfterimageS366Replay final : public IAutomationLatentCommand
{
public:
    explicit FAfterimageS366Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FAfterimageS366Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Afterimage timeout in phase %d"), Phase)); return true; }
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
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S366Afterimage") / (FString(Name) + TEXT(".png")), true, false); };
        auto Key = [&](const FKey& K, EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, E, E == IE_Pressed ? 1.f : 0.f)); };
        const float Dilation = World->GetWorldSettings()->TimeDilation;
        const FVector Me = Pawn->GetActorLocation();
        const FMemoriaFoeSpec& ThiefSpec = FoeSpec(EMemoriaFoeKind::MarketThief);
        if (Phase == 0)
        {
            if (Frame < 20) return false;
            const auto* Material = LoadObject<UMaterial>(nullptr, TEXT("/Game/Memoria/Presentation/Combat/M_Afterimage.M_Afterimage"));
            Test->TestTrue(TEXT("The afterimage material exists"), Material != nullptr);
            if (Material)
            {
                Test->TestTrue(TEXT("It is see-through and unlit"), Material->BlendMode == BLEND_Translucent && Material->GetShadingModels().HasOnlyShadingModel(MSM_Unlit));
                Test->TestTrue(TEXT("It is usable on Arrel's skeletal mesh"), Material->GetUsageByFlag(MATUSAGE_SkeletalMesh));
            }
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Thief = Combat->SpawnWave(1, Me + FVector(150, 0, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
            Phase = 1; Mark = Frame; return false;
        }
        AMemoriaFieldMonster* T = Thief.Get();
        if (!T) { Test->AddError(TEXT("No thief")); return true; }
        const bool Winding = T->GetState() == EMemoriaMonsterState::Windup;
        if (Winding && !bWasWinding) { WindupFrame = Frame; ++Windups; UE_LOG(LogTemp, Display, TEXT("S366_TEST windup %d at frame %d"), Windups, Frame); }
        if (Combat->GetStrikesTaken() != LoggedStrikes) { LoggedStrikes = Combat->GetStrikesTaken(); UE_LOG(LogTemp, Display, TEXT("S366_TEST strike taken at frame %d"), Frame); }
        if (Combat->IsDashing() != bLoggedDash) { bLoggedDash = Combat->IsDashing(); UE_LOG(LogTemp, Display, TEXT("S366_TEST dashing=%d at frame %d"), bLoggedDash, Frame); }
        bWasWinding = Winding;
        const int32 StrikeFrame = WindupFrame + int32(ThiefSpec.Windup * 60.f);
        if (Phase == 1)
        {
            if (Windups == 1 && Winding && !bDodged && T->GetStateTime() >= ThiefSpec.Windup - .1f)
            {
                // A tenth of a second before the blow, by the thief's own windup clock: Shift.
                bDodged = true; DodgeFrame = Frame; TakenBefore = Combat->GetStrikesTaken(); HpBefore = Combat->GetPlayerHp();
                Key(EKeys::LeftShift, IE_Pressed);
                Test->TestTrue(TEXT("Shift dashes"), Combat->IsDashing());
                Test->TestTrue(TEXT("The dash leaves a ghost of Arrel's pose"), Combat->GetGhostCount() >= 1);
            }
            if (bDodged && Frame == DodgeFrame + 1) Key(EKeys::LeftShift, IE_Released);
            // The blow lands within the windup's last tenth of a second.
            if (bDodged && !bChecked && (Combat->GetPerfectDodges() > 0 || Combat->GetStrikesTaken() != TakenBefore || Frame > DodgeFrame + 30))
            {
                bChecked = true; CheckFrame = Frame;
                Test->TestEqual(TEXT("The blow falls on the afterimage"), Combat->GetPerfectDodges(), 1);
                Test->TestEqual(TEXT("Arrel takes no harm"), Combat->GetStrikesTaken(), TakenBefore);
                Test->TestEqual(TEXT("His HP is untouched"), Combat->GetPlayerHp(), HpBefore);
                Test->TestFalse(TEXT("No curse through an afterimage"), Combat->IsWeakened());
                Test->TestTrue(TEXT("The thief loses him and reels"), T->GetState() == EMemoriaMonsterState::Stagger);
                Test->TestTrue(TEXT("An Echo Counter is ready"), Combat->IsEchoCounterReady());
                Test->TestTrue(TEXT("The world slows"), Combat->IsAfterimageSlow() && Dilation < .5f && Dilation > .05f);
            }
            if (bChecked && Frame == CheckFrame + 8)
            {
                Test->TestTrue(TEXT("And drains of colour"), Combat->GetAfterimageWeight() > .5f);
                Test->TestTrue(TEXT("The struck ghost flares"), Combat->GetBrightestGhostAlpha() > .4f);
                Capture(TEXT("AfterimageDodge"));
            }
            // Once the slowed dash is over (no cut starts mid-dash): back into reach of the reeling thief, one tap of J.
            if (bChecked && Frame > CheckFrame + 8 && !Combat->IsDashing())
            {
                Pawn->SetActorLocation(T->GetActorLocation() - FVector(120, 0, 0));
                ThiefHpBefore = T->GetHealth(); HitsBefore = Combat->GetHitsLanded();
                Combat->RequestAttack(T->GetActorLocation());
                Key(EKeys::J, IE_Pressed); Key(EKeys::J, IE_Released);
                Phase = 2; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The thief never struck")); return true; }
            return false;
        }
        if (Phase == 2)
        {
            if (!bCountered && Combat->GetHitsLanded() > HitsBefore)
            {
                bCountered = true; CounterFrame = Frame;
                Test->TestEqual(TEXT("The cut is an Echo Counter"), Combat->GetEchoCounters(), 1);
                Test->TestTrue(TEXT("It strikes at x1.6"), FMath::IsNearlyEqual(ThiefHpBefore - T->GetHealth(), MemoriaCombatTuning::ComboDamage[0] * MemoriaCombatTuning::EchoCounterFactor, .05f));
                Test->TestFalse(TEXT("The counter is spent"), Combat->IsEchoCounterReady());
                Capture(TEXT("AfterimageCounter"));
            }
            if (bCountered && Frame == CounterFrame + 60)
            {
                Test->TestFalse(TEXT("The slow is only a breath"), Combat->IsAfterimageSlow());
                Test->TestTrue(TEXT("Time and colour return"), FMath::IsNearlyEqual(Dilation, 1.f, .001f) && Combat->GetAfterimageWeight() == 0.f);
                Phase = 3; Mark = Frame; EarlyWindups = Windups;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The counter never landed")); return true; }
            return false;
        }
        if (Phase == 3)
        {
            // The thief's next blow: Shift as its windup begins, long before the blow. No Afterimage.
            if (Windups > EarlyWindups && Winding && Frame == WindupFrame + 1 && !bEarly)
            { bEarly = true; Combat->RequestDash(FVector(0, 1, 0)); }
            if (bEarly && !Winding && EarlyEndFrame == 0) EarlyEndFrame = Frame;
            if (EarlyEndFrame && Frame == EarlyEndFrame + 10)
            {
                Test->TestEqual(TEXT("An early dash is no Afterimage"), Combat->GetPerfectDodges(), 1);
                Test->TestEqual(TEXT("The ghosts have faded"), Combat->GetGhostCount(), 0);
                Test->TestFalse(TEXT("No new counter"), Combat->IsEchoCounterReady());
                return true;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The thief never wound up again")); return true; }
            return false;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, WindupFrame = 0, Windups = 0, EarlyWindups = 0, TakenBefore = 0, HitsBefore = 0, CounterFrame = 0;
    int64 HpBefore = 0;
    float ThiefHpBefore = 0.f;
    int32 LoggedStrikes = 0, DodgeFrame = 0, CheckFrame = 0, EarlyEndFrame = 0; bool bLoggedDash = false, bChecked = false;
    bool bFixed = false, bOldFixed = false, bWasWinding = false, bDodged = false, bCountered = false, bEarly = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Thief;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAfterimageS366Test, "MemoriaVisual.S366Afterimage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAfterimageS366Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FAfterimageS366Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
