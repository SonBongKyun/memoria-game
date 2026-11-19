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
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/App.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// This is the restore adapter's post-commit boundary, not modal cancellation.
// All hazards are established through the real public gameplay APIs. The real
// schema-1 run DTO is serialized, deserialized and accepted by RestoreSave before
// the restore-only reset is invoked. UI/cross-map ownership is covered separately.
class FS362RestoreReplay final : public IAutomationLatentCommand
{
public:
    FS362RestoreReplay(FAutomationTestBase* InTest,bool bInCombo)
        : Test(InTest),Started(FPlatformTime::Seconds()),bCombo(bInCombo) {}
    ~FS362RestoreReplay() override
    {
        if(OwnedWorld.IsValid())UGameplayStatics::SetGlobalTimeDilation(OwnedWorld.Get(),OldDilation);
        if(bFixed){FApp::SetUseFixedTimeStep(bOldFixed);FApp::SetFixedDeltaTime(OldDelta);}
    }
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;
        LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>150.)
        {Test->AddError(FString::Printf(TEXT("S362 restore regression timeout at phase%d"),Phase));return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;
        auto* Pawn=PC?Cast<AMemoriaFieldPawn>(PC->GetPawn()):nullptr;
        if(!World || !Pawn || World->GetTimeSeconds()<.3f)return false;
        auto* Game=World->GetGameInstance();
        auto* Narrative=Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Combat=World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if(!bFixed)
        {
            bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
            OwnedWorld=World;OldDilation=UGameplayStatics::GetGlobalTimeDilation(World);
            for(int32 I=0;I<20 && Narrative->GetState()==EMemoriaSliceState::VN;++I)Narrative->Confirm(1);
            return false;
        }
        if(Phase==0)
        {
            if(!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Narrative->GetState()!=EMemoriaSliceState::Exploration)return false;
            if(!Combat || !Combat->GetPlayer() || !Run || !Run->GetPlayerMemory())return false;
            OwnedWorld=World;OldDilation=UGameplayStatics::GetGlobalTimeDilation(World);
            if(!Test->TestEqual(TEXT("Public run burn unlocks the shield before the saved snapshot"),
                Run->BurnMemory(TEXT("daily_campfire_song")),EMemoriaMemoryResult::Success))return true;
            if(!Test->TestTrue(TEXT("Actual shield technique is unlocked"),Combat->IsEliaSkillUnlocked(0)))return true;
            // A real completed fight seeds an old win banner; its already-paid
            // reward belongs to the saved snapshot and must never be paid again.
            const auto OldFoes=Combat->SpawnWave(1,Pawn->GetActorLocation()+FVector(700,0,0),0.f);
            if(!Test->TestEqual(TEXT("An actual old encounter is created"),OldFoes.Num(),1))return true;
            OldFoes[0]->SetActorTickEnabled(false);
            OldFoes[0]->TakeHit(100000.f,Pawn->GetActorLocation(),0.f);
            if(!Test->TestTrue(TEXT("Old completed encounter seeds a visible reward"),Combat->GetLastReward().Age<1.f))return true;
            Run->GrantFieldItem(TEXT("firebomb"),1);
            Combat->Revive(.63f);
            const auto State=Run->GetRunSnapshot();SavedHp=State.Player.Hp;SavedGrains=State.Player.Grains;SavedRunId=State.RunId;
            SavedBombs=Run->GetItemCount(TEXT("firebomb"));SavedBurns=Run->GetPlayerMemory()->GetSnapshot().BurnedHistory;
            SavedWorldRevision=Run->GetWorldCognition()->GetSnapshot().Revision;
            if(!Test->TestTrue(TEXT("Snapshot HP is real, positive and below maximum"),SavedHp>0 && SavedHp<State.Player.MaxHp))return true;
            TStrongObjectPtr<UMemoriaRunSaveGame> Capture(Run->CaptureSave());
            if(!Test->TestNotNull(TEXT("Actual run captures its schema1 DTO"),Capture.Get()))return true;
            FString Error;
            if(!Test->TestTrue(TEXT("Captured DTO validates before serialization"),Capture->ValidateHeader(Error)))return true;
            TArray<uint8> Bytes;
            if(!Test->TestTrue(TEXT("Actual save serializer writes the DTO"),UGameplayStatics::SaveGameToMemory(Capture.Get(),Bytes)))return true;
            Saved.Reset(Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
            if(!Test->TestNotNull(TEXT("Actual save serializer reconstructs the DTO"),Saved.Get()) ||
                !Test->TestTrue(TEXT("Deserialized snapshot validates"),Saved->ValidateHeader(Error)))return true;
            if(bCombo)return CheckFreshCombo(*Combat,*Run,*Pawn);
            if(!SeedHazards(*Combat,*Run,*Pawn))return true;
            const FVector AbandonedCenter=Combat->GetBurnWave().Center;
            if(!Restore(*Combat,*Run))return true;
            Test->TestFalse(TEXT("A loaded run has no abandoned paid burn wave"),Combat->GetBurnWave().bLive);
            Test->TestFalse(TEXT("A loaded run has no old cast or hitstop"),Combat->IsCasting() || Combat->IsHitStopped());
            Test->TestEqual(TEXT("Restore releases combat-owned global slow motion immediately"),UGameplayStatics::GetGlobalTimeDilation(World),1.f);
            Test->TestFalse(TEXT("Shield from the abandoned timeline does not protect the loaded run"),Combat->IsShielded());
            for(int32 I=0;I<4;++I)Test->TestEqual(TEXT("Loaded run has no old Elia cooldown"),Combat->GetEliaCooldown(I),0.f);
            Test->TestEqual(TEXT("Loaded run has no old item cooldown"),Combat->GetItemCooldown(),0.f);
            Test->TestTrue(TEXT("Restore removes the old win banner"),Combat->GetLastReward().Age>=99.f);
            Test->TestTrue(TEXT("Restore removes old diary and impact feedback"),Combat->GetEliaNotice().IsEmpty() && Combat->GetEliaNoticeAge()>=99.f &&
                Combat->GetPopups().IsEmpty() && Combat->GetSparks().IsEmpty() && Combat->GetTrail().IsEmpty() && Combat->GetImpacts().IsEmpty());
            Test->TestTrue(TEXT("Restore clears old thrown bombs and hostile projectiles"),Combat->GetThrown().IsEmpty() && Combat->GetOrbs().IsEmpty());
            // The next foe belongs to the loaded run. It is a real registered
            // monster; only its own AI tick is disabled to isolate ghost damage.
            const auto Foes=Combat->SpawnWave(1,AbandonedCenter+FVector(80,0,0),0.f);
            if(!Test->TestEqual(TEXT("A real new-run foe is registered after restore"),Foes.Num(),1))return true;
            NewFoe=Foes[0];NewFoe->SetActorTickEnabled(false);NewFoeHp=NewFoe->GetHealth();
            // A dodge starts immediately even though the abandoned run had a
            // still-positive dodge cooldown. RequestDash changes no saved HP.
            Test->TestTrue(TEXT("A loaded run can dodge without the abandoned cooldown"),Combat->RequestDash(FVector::ForwardVector));
            for(int32 I=0;I<12;++I)Combat->Tick(.04f);
            AssertSavedState(*Combat,*Run);
            Phase=1;Frames=0;return false;
        }
        // RestoreSave intentionally reset the Narrative owner to Idle. Do not
        // wait for Exploration again; this test owns the post-restore subsystem
        // boundary and lets real native frames expose any delayed old effect.
        if(World!=OwnedWorld.Get() || !Combat || !Run)
        {Test->AddError(TEXT("Restore regression unexpectedly lost its world or subsystem"));return true;}
        if(++Frames<30)return false;
        AssertSavedState(*Combat,*Run);
        Test->AddInfo(TEXT("S362_RESTORE_NO_GHOST_EFFECTS actual_serialized_run_restored=1 paid_burn_abandoned=1 new_foe_undamaged=1 hp_exact=1"));
        return true;
    }
private:
    bool SeedHazards(UMemoriaFieldCombatSubsystem& Combat,UMemoriaRunSubsystem& Run,AMemoriaFieldPawn& Pawn)
    {
        using namespace MemoriaCombatTuning;
        if(!Test->TestTrue(TEXT("Public dash starts an old cooldown"),Combat.RequestDash(FVector::ForwardVector)))return false;
        Combat.Tick(DashTime+.001f);
        if(!Test->TestTrue(TEXT("Paid burn opens through the actual gameplay picker"),Combat.OpenBurnPicker()))return false;
        const int32 Index=Combat.GetBurnChoices().IndexOfByPredicate([](const FMemoriaBurnChoice& Choice){return Choice.Id==TEXT("rel_hand_reaching");});
        if(!Test->TestTrue(TEXT("An intact authored relational memory is available"),Index>=0))return false;
        Combat.SelectBurn(Index);bool bBurned=Combat.ConfirmBurn();
        if(Combat.IsPickingBurn() && Combat.IsBurnArmed())bBurned=Combat.ConfirmBurn();
        if(!Test->TestTrue(TEXT("The abandoned timeline actually pays for its burn"),bBurned &&
            Run.GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(TEXT("rel_hand_reaching"))))return false;
        Combat.Tick(BurnCastTime+.001f);
        if(!Test->TestTrue(TEXT("Actual paid burn released a live wave and global hitstop"),Combat.GetBurnWave().bLive && Combat.IsHitStopped() &&
            UGameplayStatics::GetGlobalTimeDilation(Pawn.GetWorld())<.9f))return false;
        if(!Test->TestTrue(TEXT("Public technique seeds a real shield and cooldown"),Combat.UseEliaSkill(0) && Combat.IsShielded() && Combat.GetEliaCooldown(0)>0.f))return false;
        if(!Test->TestTrue(TEXT("Public item throw seeds a live bomb and cooldown"),Combat.UseItem(TEXT("firebomb"),Pawn.GetActorLocation()+FVector(180,0,0)) &&
            !Combat.GetThrown().IsEmpty() && Combat.GetItemCooldown()>0.f))return false;
        return true;
    }
    bool Restore(UMemoriaFieldCombatSubsystem& Combat,UMemoriaRunSubsystem& Run)
    {
        if(!Test->TestTrue(TEXT("The real run owner accepts the validated serialized snapshot"),Run.RestoreSave(*Saved.Get())))return false;
        const int64 Before=Run.GetRunSnapshot().Player.Hp;
        Combat.ResetForRestoredRun();
        Test->TestEqual(TEXT("Restore-only reset preserves the accepted nonmax HP exactly"),Run.GetRunSnapshot().Player.Hp,Before);
        return true;
    }
    bool CheckFreshCombo(UMemoriaFieldCombatSubsystem& Combat,UMemoriaRunSubsystem& Run,AMemoriaFieldPawn& Pawn)
    {
        if(!Test->TestTrue(TEXT("Old timeline begins the first real combo step"),Combat.RequestAttack(Pawn.GetActorLocation()+FVector(300,0,0))))return true;
        Test->TestEqual(TEXT("Old initial step is zero"),Combat.GetComboStep(),0);
        Combat.Tick(10.f);
        if(!Test->TestFalse(TEXT("Old first step actually finishes"),Combat.IsAttacking()))return true;
        if(!Restore(Combat,Run))return true;
        Test->TestTrue(TEXT("Loaded timeline starts an attack"),Combat.RequestAttack(Pawn.GetActorLocation()+FVector(300,0,0)));
        Test->TestEqual(TEXT("Loaded first attack cannot inherit the abandoned combo continuation"),Combat.GetComboStep(),0);
        Test->TestEqual(TEXT("Fresh attack does not heal the loaded run"),Run.GetRunSnapshot().Player.Hp,SavedHp);
        Test->AddInfo(TEXT("S362_RESTORE_FRESH_COMBO actual_completed_old_step=1 restored_first_step=0 hp_exact=1"));
        return true;
    }
    void AssertSavedState(const UMemoriaFieldCombatSubsystem& Combat,const UMemoriaRunSubsystem& Run)
    {
        Test->TestTrue(TEXT("Loaded run keeps its exact saved identity"),Run.GetRunSnapshot().RunId==SavedRunId);
        Test->TestEqual(TEXT("Abandoned effects never damage or heal saved HP"),Run.GetRunSnapshot().Player.Hp,SavedHp);
        Test->TestEqual(TEXT("Abandoned effects never grant new-run grains"),Run.GetRunSnapshot().Player.Grains,SavedGrains);
        Test->TestEqual(TEXT("Abandoned effects never consume or reward loaded inventory"),Run.GetItemCount(TEXT("firebomb")),SavedBombs);
        Test->TestTrue(TEXT("Loaded memory burn history is exact"),Run.GetPlayerMemory()->GetSnapshot().BurnedHistory==SavedBurns);
        Test->TestEqual(TEXT("Loaded world revision stays exact"),Run.GetWorldCognition()->GetSnapshot().Revision,SavedWorldRevision);
        Test->TestTrue(TEXT("Actual newly spawned foe remains completely undamaged"),NewFoe.IsValid() && FMath::IsNearlyEqual(NewFoe->GetHealth(),NewFoeHp,.001f));
        Test->TestFalse(TEXT("No old burn wave returns on later native frames"),Combat.GetBurnWave().bLive);
    }
    FAutomationTestBase* Test;
    double Started,OldDelta=0.;
    uint64 LastFrame=MAX_uint64;
    int32 Phase=0,Frames=0;
    bool bCombo=false,bFixed=false,bOldFixed=false;
    float OldDilation=1.f,NewFoeHp=0.f;
    int64 SavedHp=0,SavedGrains=0,SavedBombs=0,SavedWorldRevision=0;
    FGuid SavedRunId;
    TArray<FString> SavedBurns;
    TWeakObjectPtr<UWorld> OwnedWorld;
    TWeakObjectPtr<AMemoriaFieldMonster> NewFoe;
    TStrongObjectPtr<UMemoriaRunSaveGame> Saved{nullptr};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362RestoreHazards,"Memoria.S362Restore.QuiescesAbandonedEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS362RestoreHazards::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362RestoreReplay(this,false)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362RestoreCombo,"Memoria.S362Restore.FreshCombo",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS362RestoreCombo::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362RestoreReplay(this,true)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
