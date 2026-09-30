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
#include "World/MemoriaWorldCognition.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Misc/App.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S328: random encounters in the field (random_encounter.gd), the path play already takes. On a Verdan
// revisit, walking fills the source distance model: the warning comes at 72% of the threshold, then the
// encounter raises the source pool's foes around Arrel, counts as a battle started (TotalBattles, saved),
// and holds while they live, so walking through the fight starts no second one.
class FFieldEncounterReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldEncounterReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldEncounterReplay() override
    {
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
        UMemoriaFieldCombatSubsystem::SetFieldEncountersForTests(false);
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field encounter timeout in phase %d"), Phase)); return true; }
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
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            UMemoriaFieldCombatSubsystem::SetFieldEncountersForTests(true);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer()) return false;
        ++Frame;
        auto Count = [&](const TCHAR* Event) { int32 N = 0; for (const FString& E : Host->GetTrace()) N += E == Event ? 1 : 0; return N; };
        switch (Phase)
        {
        case 0:
        {
            // The closed Chapter 2 boundary and its revisit, the route that enables encounters.
            auto* Checkpoint = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
            Test->TestTrue(TEXT("Saves are isolated"), Checkpoint->ConfigureTestStorage(TEXT("enc-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            FMemoriaRunSnapshot Boundary = Run->GetRunSnapshot(); Boundary.CurrentChapter = 3;
            Test->TestEqual(TEXT("The run reaches the boundary"), Run->RestoreRun(Boundary, Run->GetPlayerMemory()->GetDefinitions(), Run->GetPlayerMemory()->GetSnapshot(), Run->GetWorldCognition()->GetSnapshot()), EMemoriaMemoryResult::Success);
            Run->SetStoryFlag(TEXT("ch2_complete"), true);
            Test->TestTrue(TEXT("The boundary saves"), Checkpoint->SaveClosedBoundary(FVector2D(500, 340)));
            Test->TestTrue(TEXT("Its checkpoint screen opens"), Host->ContinueCheckpoint());
            Test->TestTrue(TEXT("The revisit travels"), Host->RequestCheckpointRevisit());
            From = World; ++Phase; break;
        }
        case 1:
            if (World == From.Get() || !Host->IsVerdanRevisit() || Frame < 10) break;
            Test->TestTrue(TEXT("Field encounters are the encounter path"), UMemoriaFieldCombatSubsystem::UseFieldEncounters());
            BattlesBefore = Run->GetRunSnapshot().TotalBattles;
            Origin = Pawn->GetActorLocation();
            ++Phase; Mark = Frame; break;
        case 2:
        {
            // Pace back and forth across the square, a tile every few frames, until the foes come.
            const float Swing = FMath::Fmod(float(Frame - Mark) * 12.f, 1200.f);
            Pawn->SetActorLocation(Origin + FVector(Swing < 600.f ? Swing : 1200.f - Swing, 0, 0));
            if (Count(TEXT("encounter:field_started")) == 1)
            {
                const auto& Trace = Host->GetTrace();
                Test->TestTrue(TEXT("The warning comes first"), Trace.IndexOfByKey(TEXT("encounter:warning")) >= 0 &&
                    Trace.IndexOfByKey(TEXT("encounter:warning")) < Trace.IndexOfByKey(TEXT("encounter:field_started")));
                const int32 Foes = Combat->LiveMonsterCount();
                Test->TestTrue(TEXT("The source pool's foes rise: three husks or two thieves"), Foes == 2 || Foes == 3);
                Test->TestEqual(TEXT("It counts as a battle started"), Run->GetRunSnapshot().TotalBattles, BattlesBefore + 1);
                ++Phase; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("No encounter came")); return true; }
            break;
        }
        case 3:
        {
            // Walking on through the fight starts nothing new while the foes live.
            if (Frame == Mark + 20) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldEncounters/EncounterStart.png"), true, false);
            const float Swing = FMath::Fmod(float(Frame - Mark) * 12.f, 1200.f);
            Pawn->SetActorLocation(Origin + FVector(Swing < 600.f ? Swing : 1200.f - Swing, 0, 0));
            if (Frame < Mark + 900 && Combat->LiveMonsterCount() > 0) break;
            Test->TestEqual(TEXT("One encounter while the foes lived"), Count(TEXT("encounter:field_started")), 1);
            return true;
        }
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0;
    int64 BattlesBefore = 0;
    FVector Origin = FVector::ZeroVector;
    TWeakObjectPtr<UWorld> From;
    bool bFixed = false, bOldFixed = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldEncounterTest, "MemoriaVisual.FieldEncounters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldEncounterTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldEncounterReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
