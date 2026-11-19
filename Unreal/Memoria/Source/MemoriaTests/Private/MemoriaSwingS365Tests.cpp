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
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Exercise the native subsystem with a real PIE player, its actual mapped sword
// clips and registered monsters. Only elapsed gameplay deltas and actor positions
// are controlled; no action clock, damage result or hit set is written by tests.
class FSwingS365Replay final : public IAutomationLatentCommand
{
public:
    explicit FSwingS365Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FSwingS365Replay() override
    {
        if (Combat.IsValid()) { Combat->CancelHeldInput(); Combat->ResetForRestoredRun(); }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 150.) { Test->AddError(TEXT("S365 swing PIE timeout")); Write(); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 120.);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        Player = Pawn; Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Test->TestTrue(TEXT("S365 has the registered native field player"), Combat.IsValid() && Combat->GetPlayer() == Pawn)) { Write(); return true; }
        Figure = Combat->GetPlayerFigure();
        if (!Test->TestTrue(TEXT("S365 has the actual rigged Arrel sword figure"), Figure.IsValid() && Figure->IsRigged() && Figure->HasSword())) { Write(); return true; }
        Pawn->SetActorLocation(FVector(0, -180, 0), false);
        const auto* Memory = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory();
        const auto BeforeBurned = Memory->GetSnapshot().BurnedHistory;
        RunCases();
        Test->TestTrue(TEXT("Contact tests preserve irreversible burned-memory history"), Memory->GetSnapshot().BurnedHistory == BeforeBurned);
        Write(); return true;
    }
private:
    void Reset() { Combat->ResetForRestoredRun(); Combat->Revive(1.f); }
    AMemoriaFieldMonster* Spawn(const FVector& Offset)
    {
        const auto Foes = Combat->SpawnWave(1, Player->GetActorLocation() + Offset, 0.f);
        if (!Test->TestEqual(TEXT("Actual HP target spawned"), Foes.Num(), 1)) return nullptr;
        Foes[0]->SetActorTickEnabled(false); return Foes[0];
    }
    bool Start(int32 Step = 0, const FVector& Direction = FVector::ForwardVector)
    {
        if (!Test->TestTrue(TEXT("Actual RequestAttack starts or chains the cut"), Combat->RequestAttack(Player->GetActorLocation() + Direction * 1000.f))) return false;
        return CheckClip(Step);
    }
    bool CheckClip(int32 Step)
    {
        const auto* Expected = MemoriaCombatClips::Load(Figure->GetCharacterId(), MemoriaCombatClips::ForAction(Figure->GetCharacterId(), MemoriaCombatClips::Attack(FMath::Min(Step, 2))));
        const float Rate = Step == 3 ? MemoriaCombatTuning::HeavyRate : MemoriaCombatTuning::SwordComboRate[Step];
        const auto P = Combat->GetComboInputProbe();
        return Test->TestTrue(TEXT("Cut uses the real imported mapped clip and unchanged playback rate"), Expected && Figure->GetActionClip() == Expected && P.Step == Step && FMath::IsNearlyEqual(P.StepLength, float(Expected->GetPlayLength()) / Rate, .0001f));
    }
    void Record(const TCHAR* Name, float HpBefore, const AMemoriaFieldMonster* Foe, int32 HitsBefore, int32 ImpactsBefore)
    {
        auto Row = MakeShared<FJsonObject>(); const auto P = Combat->GetComboInputProbe();
        Row->SetStringField(TEXT("case"), Name); Row->SetNumberField(TEXT("actual_hp_loss"), HpBefore - Foe->GetHealth());
        Row->SetNumberField(TEXT("actual_hits"), Combat->GetHitsLanded() - HitsBefore); Row->SetNumberField(TEXT("actual_impacts"), Combat->GetImpactsMade() - ImpactsBefore);
        Row->SetNumberField(TEXT("step"), P.Step); Row->SetNumberField(TEXT("step_time"), P.StepTime); Row->SetNumberField(TEXT("step_length"), P.StepLength);
        Row->SetStringField(TEXT("committed_aim"), P.SwingAim.ToString()); Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    void RunCases()
    {
        // The skipped-window regression: both endpoints lie outside the active
        // window, yet this real target receives one ordinary sword hit.
        Reset(); auto* Front = Spawn(FVector(130, 0, 0)); auto* Rear = Spawn(FVector(-130, 0, 0));
        if (!Front || !Rear || !Start()) return;
        float Hp = Front->GetHealth(), RearHp = Rear->GetHealth(); int32 Hits = Combat->GetHitsLanded(), Impacts = Combat->GetImpactsMade();
        float Length = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(Length * .29f);
        Test->TestEqual(TEXT("Before active window there is no HP loss"), Front->GetHealth(), Hp, .01f);
        Combat->Tick(Length * .27f);
        Test->TestEqual(TEXT("Frame .29 to .56 hits front for unchanged12"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Crossing does not hit a rear target beyond close-range exception"), Rear->GetHealth(), RearHp, .01f);
        Test->TestEqual(TEXT("Crossing creates exactly one native hit"), Combat->GetHitsLanded() - Hits, 1);
        Test->TestEqual(TEXT("Crossing creates exactly one native impact"), Combat->GetImpactsMade() - Impacts, 1);
        Combat->Tick(Length * .1f); Combat->Tick(Length * .1f);
        Test->TestEqual(TEXT("Later frames cannot duplicate the same cut's damage"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Later frames cannot duplicate the same cut's popup"), Combat->GetPopups().Num(), 1);
        Record(TEXT("SkippedWindowFrontRearDedup"), Hp, Front, Hits, Impacts);

        Reset(); Front = Spawn(FVector(130, 0, 0)); if (!Front || !Start()) return;
        Hp = Front->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade(); Length = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(Length * 1.01f);
        Test->TestEqual(TEXT("A whole-step frame still hits once before ending"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestFalse(TEXT("Whole-step crossing finishes the action normally"), Combat->IsAttacking());
        Test->TestEqual(TEXT("Whole-step crossing makes one native hit"), Combat->GetHitsLanded() - Hits, 1);
        Record(TEXT("WholeStepCrossing"), Hp, Front, Hits, Impacts);

        Reset(); Front = Spawn(FVector(130, 0, 0)); if (!Front || !Start()) return;
        Hp = Front->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade(); Length = Combat->GetComboInputProbe().StepLength;
        for (int32 I = 0; I < 101; ++I) Combat->Tick(Length * .01f);
        Test->TestEqual(TEXT("Ordinary small frames produce the same12 HP loss"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Repeated frames within the window keep one hit"), Combat->GetHitsLanded() - Hits, 1);
        Record(TEXT("SmallFrameEquivalent"), Hp, Front, Hits, Impacts);

        // A frame wholly after the window must not acquire a newly arrived foe.
        // Place it out of range during the crossing, then enter during recovery.
        Reset(); Front = Spawn(FVector(500, 0, 0)); if (!Front || !Start()) return;
        Hp = Front->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade(); Length = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(Length * .56f);
        Test->TestEqual(TEXT("Crossing preserves the existing range exclusion"), Front->GetHealth(), Hp, .01f);
        Front->SetActorLocation(Player->GetActorLocation() + FVector(130, 0, 0), false); Combat->Tick(Length * .1f);
        Test->TestEqual(TEXT("A target entering after the active window is not hit"), Front->GetHealth(), Hp, .01f);
        Test->TestEqual(TEXT("Recovery-only frame creates no damage popup"), Combat->GetPopups().Num(), 0);
        Record(TEXT("RangeAndRecoveryExclusion"), Hp, Front, Hits, Impacts);

        // Dedup belongs to each target, not the whole window: a second foe may
        // enter a later active frame without the first foe taking a second hit.
        Reset(); Front = Spawn(FVector(130, 0, 0)); auto* Late = Spawn(FVector(500, 0, 0)); if (!Front || !Late || !Start()) return;
        Hp = Front->GetHealth(); const float LateHp = Late->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade(); Length = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(Length * .32f); Late->SetActorLocation(Player->GetActorLocation() + FVector(120, 30, 0), false); Combat->Tick(Length * .08f);
        Test->TestEqual(TEXT("First target remains deduplicated within the window"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Another target entering inside the window gets its own hit"), LateHp - Late->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Two different targets produce two native hits"), Combat->GetHitsLanded() - Hits, 2);
        Record(TEXT("LaterTargetWithinWindow"), Hp, Front, Hits, Impacts);

        // Queue a reverse cut between the hit-window endpoints. The crossing
        // resolves the old committed front direction before the next cut turns.
        Reset(); Front = Spawn(FVector(130, 0, 0)); Rear = Spawn(FVector(-130, 0, 0)); if (!Front || !Rear || !Start()) return;
        Hp = Front->GetHealth(); RearHp = Rear->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade(); Length = Combat->GetComboInputProbe().StepLength;
        Combat->Tick(Length * .29f); Combat->RequestAttack(Player->GetActorLocation() + FVector(-1000, 0, 0)); Combat->Tick(Length * .72f);
        Test->TestEqual(TEXT("Crossing before a queued transition hits committed front12"), Hp - Front->GetHealth(), 12.f, .01f);
        Test->TestEqual(TEXT("Queued reverse cannot turn old damage backward"), Rear->GetHealth(), RearHp, .01f);
        if (!CheckClip(1)) return;
        Test->TestTrue(TEXT("Second cut commits the queued reverse aim"), FVector::DotProduct(Combat->GetComboInputProbe().SwingAim, -FVector::ForwardVector) > .999f);
        Combat->Tick(Combat->GetComboInputProbe().StepLength * 1.01f);
        Test->TestEqual(TEXT("Second whole-step crossing hits rear for unchanged14"), RearHp - Rear->GetHealth(), 14.f, .01f);
        Test->TestEqual(TEXT("Reverse second cut does not hit front twice"), Hp - Front->GetHealth(), 12.f, .01f);
        if (!Start(2)) return;
        Combat->Tick(Combat->GetComboInputProbe().StepLength * 1.01f);
        Test->TestEqual(TEXT("Third cut's crossing preserves finisher22"), Hp - Front->GetHealth(), 34.f, .01f);
        Record(TEXT("QueuedReverseAndFinisher"), Hp, Front, Hits, Impacts);

        // Enter the unchanged hold-charge path while the foe is out of range,
        // then cross the heavy window with a rear target (the spin is radial).
        Reset(); Rear = Spawn(FVector(-500, 0, 0)); if (!Rear) return;
        Hp = Rear->GetHealth(); Hits = Combat->GetHitsLanded(); Impacts = Combat->GetImpactsMade();
        Combat->BeginCharge(); Combat->Tick(MemoriaCombatTuning::ChargeTime + .001f);
        if (!CheckClip(3)) return;
        const auto Heavy = Combat->GetComboInputProbe();
        Test->TestTrue(TEXT("Real charged clip is still before heavy contact window"), Heavy.StepTime < Heavy.StepLength * MemoriaCombatTuning::HitWindowStart);
        Rear->SetActorLocation(Player->GetActorLocation() + FVector(-130, 0, 0), false);
        Combat->Tick(Heavy.StepLength * 1.01f);
        Test->TestEqual(TEXT("Charged whole-window crossing preserves radial heavy30"), Hp - Rear->GetHealth(), 30.f, .01f);
        Test->TestEqual(TEXT("Charged crossing creates one native hit"), Combat->GetHitsLanded() - Hits, 1);
        Test->TestFalse(TEXT("Heavy cut completes without inventing a queued cut"), Combat->IsAttacking());
        Record(TEXT("ChargedHeavyCrossing"), Hp, Rear, Hits, Impacts);
    }
    void Write()
    {
        Report->SetStringField(TEXT("status"), Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"));
        Report->SetStringField(TEXT("scope"), TEXT("Native PIE player, actual mapped sword clips and registered monster HP/counters; manually advanced gameplay deltas with frozen target AI. Tests temporal active-window sampling, not moving-target rewind, a geometric weapon sweep, rendered hitch contact or hardware latency."));
        Report->SetArrayField(TEXT("cases"), Rows);
        FString Json; const auto Writer = TJsonWriterFactory<>::Create(&Json); FJsonSerializer::Serialize(Report, Writer);
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("Validation/S365Swing") / FApp::GetInstanceId().ToString(EGuidFormats::Digits);
        IFileManager::Get().MakeDirectory(*Folder, true);
        Test->TestTrue(TEXT("Native S365 swing evidence saved"), FFileHelper::SaveStringToFile(Json, *(Folder / TEXT("swing.json"))));
    }
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.; uint64 LastFrame = MAX_uint64;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<AMemoriaFieldPawn> Player;
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Rows;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwingS365, "MemoriaUpgrade.S365Swing.FrameWindowContact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSwingS365::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FSwingS365Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
