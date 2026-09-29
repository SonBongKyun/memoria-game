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
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Combat/MemoriaExplorationHudWidget.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S314: the source's battle rules in the field. The thief's blow weakens Arrel (his cut does 70%), the husk's
// poisons him (it ticks, never fells him), each fallen foe pays its source grains, and the won fight heals 20%
// and rolls the 30% drop; the statuses end with the fight.
class FFieldRewardsReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldRewardsReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldRewardsReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field rewards timeout in phase %d"), Phase)); return true; }
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
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldRewards") / (FString(Name) + TEXT(".png")), true, false); };
        auto Items = [&](const FString& Id) { const auto* I = Run->GetRunSnapshot().Player.Items.FindByPredicate([&](const FMemoriaItemCount& C) { return C.Id == Id; }); return I ? I->Count : 0; };
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* T = Thief.Get(); AMemoriaFieldMonster* H = Husk.Get();
        if (Phase == 0)
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Combat->SeedDrops(7);
            GrainsBefore = Run->GetRunSnapshot().Player.Grains;
            Thief = Combat->SpawnWave(1, Me + FVector(90, 0, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
            Husk = Combat->SpawnWave(1, Me + FVector(-90, 30, 0), 0.f, EMemoriaFoeKind::VoidHusk)[0];
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame < Mark + 3 || !T || !H) return false;
            // Their blows, landed directly: the thief weakens, the husk (the rat's stand-in) poisons.
            Test->TestTrue(TEXT("The thief's blow lands"), Combat->StrikePlayer(T, FoeSpec(EMemoriaFoeKind::MarketThief).Damage));
            Test->TestTrue(TEXT("It weakens Arrel"), Combat->IsWeakened() && !Combat->IsPoisoned());
            Test->TestTrue(TEXT("The husk's blow lands"), Combat->StrikePlayer(H, FoeSpec(EMemoriaFoeKind::VoidHusk).Damage));
            Test->TestTrue(TEXT("It poisons Arrel"), Combat->IsPoisoned() && Combat->GetPoisonTicksLeft() == MemoriaCombatTuning::PoisonTicks);
            Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 14) Capture(TEXT("RewardsAfflicted"));
            // A weakened cut: the combo's first blow does 70%.
            if (Frame > Mark + 20 && T && !T->IsDead() && Frame % 5 == 0 && !bCutChecked) Combat->RequestAttack(T->GetActorLocation());
            if (!bCutChecked && T && T->GetHealth() < T->GetMaxHealth())
            {
                bCutChecked = true;
                Test->TestTrue(TEXT("Still weakened at the cut"), Combat->IsWeakened());
                Test->TestEqual(TEXT("The weakened cut does 70%"), T->GetMaxHealth() - T->GetHealth(), MemoriaCombatTuning::ComboDamage[0] * MemoriaCombatTuning::WeakenFactor, .01f);
            }
            if (Frame == Mark + int32(MemoriaCombatTuning::PoisonInterval * 60.f) + 30)
                Test->TestTrue(TEXT("The poison ticks"), Combat->GetPoisonTicksLeft() < MemoriaCombatTuning::PoisonTicks && Combat->GetPlayerHp() >= 1);
            if (!bCutChecked || Frame < Mark + int32(MemoriaCombatTuning::PoisonInterval * 60.f) + 32)
            {
                if (Frame > Mark + 600) { Test->AddError(TEXT("The weakened cut never landed")); return true; }
                return false;
            }
            // End the fight: both fall, and the source Win pays out.
            HpBefore = Combat->GetPlayerHp(); Potions = Items(TEXT("potion"));
            if (T && !T->IsDead()) T->TakeHit(999.f, Me);
            if (H && !H->IsDead()) H->TakeHit(999.f, Me);
            Phase = 3; Mark = Frame; return false;
        }
        if (Phase == 3)
        {
            if (Frame == Mark + 1)
            {
                const auto& Won = Combat->GetLastReward();
                const int64 Expected = (3 + 45 / 20) + (8 + 60 / 20);
                Test->TestEqual(TEXT("Each foe pays its source grains"), Run->GetRunSnapshot().Player.Grains - GrainsBefore, Expected);
                Test->TestEqual(TEXT("The reward records them"), Won.Grains, Expected);
                Test->TestEqual(TEXT("Two foes in the fight"), Won.Kills, 2);
                const int64 Max = Combat->GetPlayerMaxHp();
                Test->TestEqual(TEXT("The won fight heals 20%"), Won.Heal, FMath::Min<int64>(int64(Max * .2), Max - HpBefore));
                Test->TestEqual(TEXT("HP includes it"), Combat->GetPlayerHp(), HpBefore + Won.Heal);
                Test->TestFalse(TEXT("The statuses end with the fight"), Combat->IsWeakened() || Combat->IsPoisoned());
                if (!Won.ItemId.IsEmpty())
                {
                    Test->TestTrue(TEXT("The dropped item is in the pack"), Items(Won.ItemId) >= 1 && !Won.ItemName.IsEmpty());
                    if (Won.ItemId == TEXT("potion")) Test->TestEqual(TEXT("One more potion"), Items(TEXT("potion")), Potions + 1);
                }
                Test->AddInfo(FString::Printf(TEXT("FIELD_REWARD grains=%lld heal=%lld item=%s"), Won.Grains, Won.Heal, *Won.ItemId));
            }
            if (Frame == Mark + 20)
            {
                // S316: the exploration panel after exploration_hud.gd shows the same numbers.
                const UMemoriaExplorationHudWidget* Hud = nullptr;
                for (TActorIterator<AMemoriaVerdanPresentation> It(World); It; ++It) Hud = It->GetExplorationHud();
                Test->TestTrue(TEXT("The exploration panel is up"), Hud && Hud->IsShowing() && Hud->GetLines().Num() == 5);
                if (Hud && Hud->GetLines().Num() == 5)
                {
                    Test->TestEqual(TEXT("It shows the grains"), Hud->GetLines()[3], FString::Printf(TEXT("Grains  %lld"), Run->GetRunSnapshot().Player.Grains));
                    Test->TestTrue(TEXT("It shows the HP"), Hud->GetLines()[0].Contains(FString::Printf(TEXT("%lld / %lld"), Combat->GetPlayerHp(), Combat->GetPlayerMaxHp())));
                }
            }
            if (Frame == Mark + 24) Capture(TEXT("RewardsVictory"));
            return Frame > Mark + 28;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    int64 GrainsBefore = 0, HpBefore = 0, Potions = 0;
    bool bFixed = false, bOldFixed = false, bCutChecked = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Thief, Husk;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldRewardsTest, "MemoriaVisual.FieldRewards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldRewardsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldRewardsReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
