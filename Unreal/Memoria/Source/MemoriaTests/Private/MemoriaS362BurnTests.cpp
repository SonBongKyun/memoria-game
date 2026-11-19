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
#include "Domain/MemoriaChapterMemories.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Misc/App.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Compile the regression before the production API exists: its absence must be a red assertion,
// rather than a build failure that prevents the existing runtime paths from being exercised.
template <typename T> auto CancelHeld(T* Combat, int) -> decltype(Combat->CancelHeldInput(), bool())
{ Combat->CancelHeldInput(); return true; }
template <typename T> bool CancelHeld(T*, long) { return false; }

struct FBurnCase
{
    const TCHAR* Name;
    int32 PriorBurns;
    const TCHAR* Target;
    bool bPrimeChain;
};
const FBurnCase Cases[] = {
    {TEXT("Baseline"), 0, TEXT("sense_forest_smell"), false},
    {TEXT("Fifth burn unlocks Ember Affinity"), 4, TEXT("identity_first_sword"), false},
    {TEXT("Existing Ember Affinity and second chain burn"), 5, TEXT("identity_first_sword"), true},
    {TEXT("Thirtieth burn unlocks Void Touch"), 29, TEXT("identity_first_sword"), false},
    {TEXT("Existing Void Touch and second chain burn"), 30, TEXT("identity_first_sword"), true},
    {TEXT("Low grade resets an existing chain"), 5, TEXT("sense_forest_smell"), true},
};

// Targets and the chain primer use authored/imported definitions. Inert test-only seeds let this
// isolated PIE run reach the thirtieth-burn threshold without inventing story content or touching saves.
class FS362BurnReplay final : public IAutomationLatentCommand
{
public:
    FS362BurnReplay(FAutomationTestBase* InTest, bool bInCancellation)
        : Test(InTest), Started(FPlatformTime::Seconds()), bCancellation(bInCancellation) {}
    ~FS362BurnReplay() override
    { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180.)
        { Test->AddError(FString::Printf(TEXT("S362 burn timeout, case %d phase %d"), CaseIndex, Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 60.);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || !Run || !Run->GetPlayerMemory()) return false;
        if (bCancellation) return CheckCancellation(Combat, Pawn);
        auto* Memory = Run->GetPlayerMemory();
        if (!bCatalogReady)
        {
            OriginalWorld = Run->GetWorldCognition()->GetSnapshot();
            Definitions = Memory->GetDefinitions();
            for (int64 Chapter = 1; Chapter <= 30; ++Chapter)
                for (const auto& Definition : MemoriaChapterMemories::For(Chapter))
                    if (!Definitions.ContainsByPredicate([&](const auto& D) { return D.Id == Definition.Id; })) Definitions.Add(Definition);
            // The connected slice has thirteen authored memories. Extra already-burned, inert fixtures
            // exercise late-game passive thresholds only; they have no description, NPC or story effect.
            for (int32 I = 0; Definitions.Num() < 32; ++I)
            {
                FMemoriaMemoryDefinition Seed; Seed.Id = FString::Printf(TEXT("s362_test_prior_burn_%02d"), I);
                Seed.Title = TEXT("S362 isolated prior burn"); Seed.BurnPower = 1;
                Seed.SourcePath = TEXT("test://S362/passive-threshold"); Definitions.Add(Seed);
            }
            bCatalogReady = true;
        }
        if (CaseIndex >= UE_ARRAY_COUNT(Cases)) return true;
        const FBurnCase& Case = Cases[CaseIndex];
        auto Label = [&](const TCHAR* What) { return FString::Printf(TEXT("%s: %s"), Case.Name, What); };
        if (Phase == 0)
        {
            Combat->CloseBurnPicker(); Combat->Revive(1.f);
            FMemoriaMemorySnapshot Snapshot; int32 Seeded = 0;
            for (const auto& D : Definitions)
            {
                FMemoriaMemoryState S; S.Id = D.Id;
                if (Seeded < Case.PriorBurns && D.Id != Case.Target && D.Id != TEXT("rel_hand_reaching"))
                { S.bBurned = true; Snapshot.BurnedHistory.Add(D.Id); ++Seeded; }
                Snapshot.Owned.Add(S);
            }
            // Restore only the isolated domains: RestoreRun intentionally resets the narrative host to
            // Idle, which would leave a gameplay replay waiting forever for Exploration after phase 0.
            if (!Test->TestEqual(*Label(TEXT("seed requested prior history")), Seeded, Case.PriorBurns) ||
                !Test->TestEqual(*Label(TEXT("restore isolated memory snapshot")), Memory->Restore(Definitions, Snapshot), EMemoriaMemoryResult::Success)) return true;
            Test->TestTrue(*Label(TEXT("restore isolated world snapshot")), Run->GetWorldCognition()->Restore(OriginalWorld));
            Memory->EvaluatePassives();
            Test->AddInfo(FString::Printf(TEXT("S362_PHASE case=%s seeded=%d host=%d"), Case.Name, Seeded, int32(Host->GetState())));
            // Keep the wave open: a distant stationary foe prevents victory from resetting the chain.
            auto Sentinel = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(3000, 0, 0), 0.f);
            if (Sentinel.IsEmpty()) { Test->AddError(TEXT("S362 could not spawn sentinel")); return true; }
            Sentinel[0]->SetActorTickEnabled(false);
            if (Case.bPrimeChain)
            {
                if (!Burn(Combat, TEXT("rel_hand_reaching"), nullptr)) return true;
                Phase = 1;
            }
            else Phase = 2;
            return false;
        }
        if (Phase == 1)
        {
            if (Combat->IsCasting() || Combat->GetBurnWave().bLive) return false;
            Test->TestEqual(*Label(TEXT("primer establishes one relationship chain burn")), Combat->GetBurnChain(), 1);
            Phase = 2;
        }
        if (Phase == 2)
        {
            const auto* Definition = Definitions.FindByPredicate([&](const auto& D) { return D.Id == Case.Target; });
            if (!Definition) { Test->AddError(TEXT("S362 target is not authored")); return true; }
            const int32 Grade = int32(Definition->RawGrade);
            const int32 PriorChain = Combat->GetBurnChain();
            const int32 NextChain = Grade >= 2 ? PriorChain + 1 : 0;
            const int32 NextBurnCount = Memory->GetSnapshot().BurnedHistory.Num() + 1;
            Expected = MemoriaCombatTuning::BurnBaseDamage[Grade] + float(Memory->GetEffectiveBurnPower(Case.Target));
            if (Memory->HasPassive(TEXT("ember_affinity")) || NextBurnCount >= 5) Expected *= MemoriaCombatTuning::EmberAffinity;
            if (NextChain >= 2) Expected *= 1.f + (NextChain - 1) * MemoriaCombatTuning::BurnChainBonus;
            if (Grade >= 3 && (Memory->HasPassive(TEXT("void_touch")) || NextBurnCount >= 30)) Expected *= MemoriaCombatTuning::VoidTouch;
            auto Targets = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(120, 0, 0), 0.f);
            if (Targets.IsEmpty()) { Test->AddError(TEXT("S362 could not spawn burn target")); return true; }
            Target = Targets[0]; Target->SetActorTickEnabled(false);
            const int32 BeforeHistory = Memory->GetSnapshot().BurnedHistory.Num();
            if (!Burn(Combat, Case.Target, &Preview)) return true;
            Test->TestEqual(*Label(TEXT("one irreversible memory is consumed")), Memory->GetSnapshot().BurnedHistory.Num(), BeforeHistory + 1);
            Test->TestEqual(*Label(TEXT("chain follows the committed grade")), Combat->GetBurnChain(), NextChain);
            const int32 BeforeBurns = Combat->GetBurns();
            CancelHeld(Combat, 0);
            Test->TestTrue(*Label(TEXT("held-input cancellation preserves the committed cast")), Combat->IsCasting());
            Test->TestEqual(*Label(TEXT("held-input cancellation never repeats or undoes a burn")), Combat->GetBurns(), BeforeBurns);
            Test->TestTrue(*Label(TEXT("burn stays irreversible")), Memory->GetSnapshot().BurnedHistory.Contains(Case.Target));
            Phase = 3; return false;
        }
        if (Phase == 3)
        {
            for (const auto& Popup : Combat->GetPopups())
            {
                if (Popup.bPlayer || Popup.Amount <= 0.f || !Popup.Label.IsEmpty()) continue;
                // The cast is the only attack in this fixture. Ignore an earlier primer's aged popup.
                if (Popup.Age > .1f) continue;
                Test->TestEqual(*Label(TEXT("committed damage retains the existing balance formula")), Popup.Amount, Expected, .01f);
                Test->TestEqual(*Label(TEXT("picker preview equals the actual released hit")), Preview, Popup.Amount, .01f);
                Test->AddInfo(FString::Printf(TEXT("S362_BURN case=%s preview=%.3f actual=%.3f expected=%.3f"), Case.Name, Preview, Popup.Amount, Expected));
                ++CaseIndex; Phase = 4; return false;
            }
            return false;
        }
        if (Phase == 4)
        {
            if (Combat->IsCasting() || Combat->GetBurnWave().bLive) return false;
            Phase = 0; return false;
        }
        return false;
    }
private:
    bool Burn(UMemoriaFieldCombatSubsystem* Combat, const TCHAR* Id, float* OutPreview)
    {
        auto* Run = Combat->GetWorld()->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Memory = Run->GetPlayerMemory(); const auto Before = Memory->GetSnapshot();
        int32 Observations = 0;
        const FDelegateHandle Observe = Memory->OnObserved.AddLambda([&](const FMemoriaMemoryEvent&) { ++Observations; });
        const bool bOpened = Combat->OpenBurnPicker();
        Memory->OnObserved.Remove(Observe);
        Test->TestEqual(TEXT("Preview publishes no authoritative memory events"), Observations, 0);
        const auto After = Memory->GetSnapshot();
        Test->TestTrue(TEXT("Preview does not consume or unlock any real memory state"), Before.BurnedHistory == After.BurnedHistory && Before.BurnPassives.Num() == After.BurnPassives.Num());
        for (int32 I = 0; I < Before.Owned.Num() && I < After.Owned.Num(); ++I)
            Test->TestTrue(TEXT("Preview leaves erosion, fading and residue unchanged"), Before.Owned[I].Erosion == After.Owned[I].Erosion && Before.Owned[I].bFaded == After.Owned[I].bFaded && Before.Owned[I].bResidue == After.Owned[I].bResidue);
        if (!Test->TestTrue(TEXT("S362 opens an authored burn picker"), bOpened)) return false;
        const int32 Index = Combat->GetBurnChoices().IndexOfByPredicate([&](const FMemoriaBurnChoice& C) { return C.Id == Id; });
        if (!Test->TestTrue(TEXT("S362 target is available"), Index >= 0)) { Combat->CloseBurnPicker(); return false; }
        const auto& Choice = Combat->GetBurnChoices()[Index];
        const auto Archive = MemoriaArchive::Build(*Run);
        const auto* Authored = Archive.Rows.FindByPredicate([&](const FMemoriaArchiveRow& Row) { return Row.Id == Id; });
        Test->TestTrue(TEXT("Picker retains the authored localized memory meaning"), Authored && Choice.Description == Authored->Description);
        Test->TestTrue(TEXT("Picker retains only the authored known consequence"), Authored && Choice.KnownConsequence == Authored->StoryEffect);
        if (OutPreview) *OutPreview = Choice.Damage;
        Combat->SelectBurn(Index);
        bool bBurned = Combat->ConfirmBurn();
        if (Combat->IsPickingBurn() && Combat->IsBurnArmed()) bBurned = Combat->ConfirmBurn();
        return Test->TestTrue(TEXT("S362 performs one committed burn"), bBurned);
    }
    bool CheckCancellation(UMemoriaFieldCombatSubsystem* Combat, AMemoriaFieldPawn* Pawn)
    {
        auto* Figure = Combat->GetPlayerFigure();
        Test->TestTrue(TEXT("S362 raises a held guard"), Combat->BeginBlock());
        Test->TestTrue(TEXT("S362 guard holds an action pose"), Figure && Figure->IsActing());
        Test->TestTrue(TEXT("CancelHeldInput API exists"), CancelHeld(Combat, 0));
        Test->TestFalse(TEXT("Cancelling held input lowers the guard"), Combat->IsBlocking());
        Test->TestTrue(TEXT("Cancelling the guard releases movement"), Combat->CanMove());
        Test->TestFalse(TEXT("Cancelling the guard releases its held animation pose"), Figure && Figure->IsActing());
        Combat->EndBlock(); // Keep the remaining assertions meaningful on the pre-fix tree.
        Combat->BeginCharge(); Combat->Tick(.1f);
        Test->TestTrue(TEXT("S362 establishes a partial held charge"), Combat->GetCharge() > 0.f);
        CancelHeld(Combat, 0);
        Test->TestEqual(TEXT("Cancelling held input drops a partial charge"), Combat->GetCharge(), 0.f);
        Combat->EndCharge();
        Test->TestTrue(TEXT("S362 commits an ordinary swing"), Combat->RequestAttack(Pawn->GetActorLocation() + FVector(100, 0, 0)));
        const int32 Step = Combat->GetComboStep(); const auto* Clip = Figure ? Figure->GetActionClip() : nullptr;
        Combat->BeginCharge(); CancelHeld(Combat, 0);
        Test->TestTrue(TEXT("Held-input cancellation preserves an already committed swing"), Combat->IsAttacking());
        Test->TestEqual(TEXT("Held-input cancellation preserves its combo step"), Combat->GetComboStep(), Step);
        Test->TestTrue(TEXT("Held-input cancellation preserves its action clip"), Figure && Figure->GetActionClip() == Clip);
        auto Foes = Combat->SpawnWave(1, Pawn->GetActorLocation() + FVector(3000, 0, 0), 0.f);
        if (Foes.IsEmpty()) { Test->AddError(TEXT("S362 could not spawn reward fixture")); return true; }
        Foes[0]->TakeHit(9999.f, Pawn->GetActorLocation());
        const auto Reward = Combat->GetLastReward(); const int32 Kills = Combat->GetKills();
        const int64 Hp = Combat->GetPlayerHp();
        CancelHeld(Combat, 0); CancelHeld(Combat, 0);
        Test->TestEqual(TEXT("Repeated cancellation preserves rewards"), Combat->GetLastReward().Grains, Reward.Grains);
        Test->TestEqual(TEXT("Repeated cancellation preserves reward drops"), Combat->GetLastReward().ItemId, Reward.ItemId);
        Test->TestEqual(TEXT("Repeated cancellation preserves kill history"), Combat->GetKills(), Kills);
        Test->TestEqual(TEXT("Repeated cancellation preserves current health"), Combat->GetPlayerHp(), Hp);
        return true;
    }
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64;
    bool bCancellation = false, bFixed = false, bOldFixed = false, bCatalogReady = false;
    int32 CaseIndex = 0, Phase = 0; float Preview = 0.f, Expected = 0.f;
    FMemoriaWorldSnapshot OriginalWorld;
    TArray<FMemoriaMemoryDefinition> Definitions; TWeakObjectPtr<AMemoriaFieldMonster> Target;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362BurnPreviewTest, "Memoria.S362Burn.PreviewMatchesCommittedDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362BurnPreviewTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362BurnReplay(this, false)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362HeldCancellationTest, "Memoria.S362Burn.CancelHeldInputPreservesCommittedActions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362HeldCancellationTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362BurnReplay(this, true)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
