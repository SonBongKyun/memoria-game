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
#include "Chapter/MemoriaChapterMap.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S342: the foes of the chapter maps' pools. Each carries its source name, HP and attack; the wisp keeps its
// distance and throws an orb that can be stepped out of; the crawler marks a lane and runs it; the leech's and
// the wisp's blows drain; the ash walker's scorches and weakens, and the antidote ends the scorch.
class FFoeVarietyReplay final : public IAutomationLatentCommand
{
public:
    explicit FFoeVarietyReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFoeVarietyReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240) { Test->AddError(FString::Printf(TEXT("Foe variety timeout in phase %d"), Phase)); return true; }
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
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FoeVariety") / (FString(Name) + TEXT(".png")), true, false); };
        auto Spawn = [&](EMemoriaFoeKind Kind, const FVector& Offset) { return Combat->SpawnWave(1, Pawn->GetActorLocation() + Offset, 0.f, Kind)[0]; };
        auto Clear = [&] { for (const auto& M : Combat->GetMonsters()) if (M.IsValid()) M->Destroy(); };
        using namespace MemoriaCombatTuning;
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* F = Foe.Get();
        const float Away = F ? FVector::Dist2D(F->GetActorLocation(), Me) : 0.f;
        switch (Phase)
        {
        case 0:
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            Home = Me;
            // The pools' entries, as the maps' IR carries them, find their own foes with the source's numbers.
            int32 Found = 0;
            for (const TCHAR* Map : {TEXT("belt_waystation"), TEXT("drift_shelter")})
                for (const auto& E : MemoriaChapterMaps::Find(Map)->Encounters)
                {
                    const EMemoriaFoeKind Kind = FoeKindByName(E.Name, E.bVoid);
                    const FMemoriaFoeSpec& S = FoeSpec(Kind);
                    Test->TestTrue(*FString::Printf(TEXT("%s is a foe of its own"), *E.Name), E.Name == S.Name && Kind != EMemoriaFoeKind::VoidHusk && Kind != EMemoriaFoeKind::MarketThief);
                    Test->TestTrue(*FString::Printf(TEXT("%s keeps the source's HP, attack and kind"), *E.Name), S.SourceHp == E.Hp && S.SourceAtk == E.Atk && S.bVoid == E.bVoid);
                    Test->TestTrue(*FString::Printf(TEXT("%s is scaled like the thief"), *E.Name), S.Health <= E.Hp && S.Health >= E.Hp * .85f && S.Damage <= E.Atk * .75f && S.Damage >= E.Atk * .5f);
                    ++Found;
                }
            Test->TestEqual(TEXT("Six pool entries"), Found, 6);
            Test->TestTrue(TEXT("An unknown name falls back by its kind"), FoeKindByName(TEXT("Nobody"), true) == EMemoriaFoeKind::VoidHusk && FoeKindByName(TEXT("Nobody"), false) == EMemoriaFoeKind::MarketThief);
            Test->TestTrue(TEXT("How each fights"), FoeSpec(EMemoriaFoeKind::VoidWisp).Behaviour == EMemoriaFoeBehaviour::Caster && FoeSpec(EMemoriaFoeKind::DustCrawler).Behaviour == EMemoriaFoeBehaviour::Charger
                && FoeSpec(EMemoriaFoeKind::RubbleRat).Behaviour == EMemoriaFoeBehaviour::Charger && FoeSpec(EMemoriaFoeKind::MemoryLeech).Behaviour == EMemoriaFoeBehaviour::Brawler);
            Test->TestTrue(TEXT("What their blows carry"), FoeSpec(EMemoriaFoeKind::VoidWisp).Ability == EMemoriaFoeAbility::Drain && FoeSpec(EMemoriaFoeKind::MemoryLeech).Ability == EMemoriaFoeAbility::Drain
                && FoeSpec(EMemoriaFoeKind::AshWalker).Ability == EMemoriaFoeAbility::Burn && FoeSpec(EMemoriaFoeKind::AshWalker).Second == EMemoriaFoeAbility::Weaken
                && FoeSpec(EMemoriaFoeKind::BeltScavenger).Ability == EMemoriaFoeAbility::Weaken && FoeSpec(EMemoriaFoeKind::DustCrawler).Ability == EMemoriaFoeAbility::Poison);
            Test->TestTrue(TEXT("The packs"), FoePackSize(EMemoriaFoeKind::DustCrawler) == 3 && FoePackSize(EMemoriaFoeKind::VoidWisp) == 2 && FoePackSize(EMemoriaFoeKind::VoidHusk) == 3 && FoePackSize(EMemoriaFoeKind::MarketThief) == 2);
            // One of each, in a row, to be read.
            const EMemoriaFoeKind Kinds[6] = {EMemoriaFoeKind::BeltScavenger, EMemoriaFoeKind::VoidWisp, EMemoriaFoeKind::DustCrawler, EMemoriaFoeKind::MemoryLeech, EMemoriaFoeKind::RubbleRat, EMemoriaFoeKind::AshWalker};
            for (int32 I = 0; I < 6; ++I) Spawn(Kinds[I], FVector(-375.f + 150.f * I, 150.f, 0))->Stun(30.f);
            Phase = 1; Mark = Frame; break;
        }
        case 1:
            if (Frame == Mark + 200)
            {
                for (const auto& M : Combat->GetMonsters())
                    if (M.IsValid()) Test->TestTrue(*FString::Printf(TEXT("%s stands at its own height"), M->Spec().Name), FMath::IsNearlyEqual(M->GetFigure()->GetWorldHeight(), M->Spec().Height));
                Capture(TEXT("FoeLineup"));
            }
            // (Held until the tutorial's first hint has left the top of the screen.)
            if (Frame < Mark + 206) break;
            Clear();
            // The wisp: a caster.
            Foe = Spawn(EMemoriaFoeKind::VoidWisp, FVector(700, 0, 0));
            Hp = Combat->GetPlayerHp(); Orbs = Combat->GetOrbsLaunched(); Closest = 1e9f;
            Phase = 2; Mark = Frame; break;
        case 2:
            if (!F) break;
            Closest = FMath::Min(Closest, Away);
            if (Combat->GetOrbsLaunched() > Orbs && !bOrbShot && Combat->GetOrbs().Num() > 0 && Combat->GetOrbs()[0].Age > .25f) { bOrbShot = true; Capture(TEXT("FoeWispOrb")); }
            if (Combat->GetPlayerHp() < Hp)
            {
                Test->TestTrue(TEXT("The wisp cast from its range and never closed"), Combat->GetOrbsLaunched() == Orbs + 1 && Closest >= CasterRetreat && Closest <= CasterRange + 5.f);
                Test->TestEqual(TEXT("Its orb did the wisp's harm"), Combat->GetPlayerHp(), Hp - int64(F->Spec().Damage));
                Test->TestEqual(TEXT("No orb is left"), Combat->GetOrbs().Num(), 0);
                Hp = Combat->GetPlayerHp(); Orbs = Combat->GetOrbsLaunched();
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The wisp's orb never reached Arrel")); return true; }
            break;
        case 3:
            // The next orb is stepped out of: Arrel moves aside once it is in the air, and the wisp is held.
            if (!F) break;
            if (Combat->GetOrbsLaunched() > Orbs)
            {
                F->Stun(30.f);
                Pawn->SetActorLocation(Me + FVector(0, 380, 0));
                Hp = Combat->GetPlayerHp();
                Phase = 4; Mark = Frame;
            }
            if (Frame > Mark + 600) { Test->AddError(TEXT("The wisp never cast again")); return true; }
            break;
        case 4:
            if (Frame < Mark + int32(OrbLife * 60.f) + 10 || !F) break;
            Test->TestTrue(TEXT("An orb can be stepped out of"), Combat->GetPlayerHp() == Hp && Combat->GetOrbs().Num() == 0);
            // A wounded wisp's blow gives it back half the harm it does.
            F->TakeHit(20.f, Me, 0.f);
            Health = F->GetHealth(); Hp = Combat->GetPlayerHp();
            Test->TestTrue(TEXT("Its blow lands from range"), Combat->StrikePlayer(F, F->Spec().Damage, true));
            Test->TestTrue(TEXT("And drains: half the harm comes back to it"), FMath::IsNearlyEqual(F->GetHealth(), Health + F->Spec().Damage * DrainShare) && Combat->GetPlayerHp() == Hp - int64(F->Spec().Damage));
            Clear(); Pawn->SetActorLocation(Home);
            Phase = 5; Mark = Frame; break;
        case 5:
            if (Frame < Mark + 40) break;
            // The crawler: a charger.
            Foe = Spawn(EMemoriaFoeKind::DustCrawler, FVector(-640, 0, 0));
            Taken = Combat->GetStrikesTaken();
            Phase = 6; Mark = Frame; break;
        case 6:
            if (!F) break;
            if (F->GetState() == EMemoriaMonsterState::Windup && !bLaneShot)
            {
                bLaneShot = true; WindupAt = Frame;
                Test->TestTrue(TEXT("It commits to its lane within reach of a rush"), Away <= RushFrom + 5.f && Away > RushHit);
                Test->TestTrue(TEXT("The lane points at Arrel"), FVector::DotProduct(F->GetRushDirection(), (Me - F->GetActorLocation()).GetSafeNormal2D()) > .99f);
            }
            if (bLaneShot && Frame == WindupAt + 20) Capture(TEXT("FoeCrawlerLane"));
            if (F->GetState() == EMemoriaMonsterState::Rush && !bRushShot && Away < 200.f) { bRushShot = true; Capture(TEXT("FoeCrawlerRush")); }
            if (bLaneShot && F->GetState() == EMemoriaMonsterState::Recover)
            {
                Test->TestEqual(TEXT("The rush struck Arrel once"), Combat->GetStrikesTaken(), Taken + 1);
                Test->TestTrue(TEXT("And poisoned him"), Combat->IsPoisoned());
                Test->TestTrue(TEXT("It ran on past him"), FVector::DotProduct(F->GetActorLocation() - Me, F->GetRushDirection()) > 0.f);
                Clear();
                Phase = 7; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The crawler never finished its rush")); return true; }
            break;
        case 7:
            if (Frame < Mark + 20) break;
            // The ash walker's blow scorches and weakens; the antidote ends the scorch and the poison.
            Foe = Spawn(EMemoriaFoeKind::AshWalker, FVector(110, 0, 0));
            Phase = 8; Mark = Frame; break;
        case 8:
            if (Frame < Mark + 3 || !F) break;
            if (Frame == Mark + 3) F->Stun(60.f);
            if (Frame < Mark + 8) break;
            Test->TestTrue(TEXT("The walker's blow lands"), Combat->StrikePlayer(F, F->Spec().Damage));
            Test->TestTrue(TEXT("It scorches and weakens"), Combat->IsScorched() && Combat->GetScorchTicksLeft() == ScorchTicks && Combat->IsWeakened());
            Hp = Combat->GetPlayerHp();
            while (Run->ConsumeItem(TEXT("antidote"))) {}
            Run->GrantFieldItem(TEXT("antidote"), 1);
            Phase = 9; Mark = Frame; break;
        case 9:
            if (Frame == Mark + 20) Capture(TEXT("FoeWalkerScorch"));
            if (Combat->GetScorchTicksLeft() == ScorchTicks - 1 && !bTicked)
            {
                bTicked = true;
                Test->TestTrue(TEXT("The scorch bites"), Combat->GetPlayerHp() < Hp);
                Test->TestTrue(TEXT("The antidote is used"), Combat->UseQuickItem(1, Me));
                Test->TestTrue(TEXT("It ends the scorch and the poison"), !Combat->IsScorched() && !Combat->IsPoisoned());
                Clear();
                return true;
            }
            if (Frame > Mark + 300) { Test->AddError(TEXT("The scorch never ticked")); return true; }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, Orbs = 0, Taken = 0, WindupAt = 0;
    int64 Hp = 0;
    float Closest = 0.f, Health = 0.f;
    FVector Home = FVector::ZeroVector;
    bool bFixed = false, bOldFixed = false, bOrbShot = false, bLaneShot = false, bRushShot = false, bTicked = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Foe;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoeVarietyTest, "MemoriaVisual.FoeVariety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFoeVarietyTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFoeVarietyReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
