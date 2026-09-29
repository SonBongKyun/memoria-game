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
#include "Chapter/MemoriaChapterPresentation.h"
#include "EngineUtils.h"
#include "Misc/Guid.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S320: the road from Chapter 2 to Chapter 3. At the closed Chapter 2 boundary the exchange-complete screen
// offers "Travel on" (verdan_market.gd moves on to the Belt Waystation); choosing it carries the same run,
// its memories and grains, into the Belt Waystation's field.
class FChapter3TravelReplay final : public IAutomationLatentCommand
{
public:
    explicit FChapter3TravelReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 150) { Test->AddError(FString::Printf(TEXT("Chapter 3 travel timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? World->GetFirstPlayerController() : nullptr;
        if (!PC || !PC->GetPawn() || World->GetTimeSeconds() < .3) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        if (Step == 0)
        {
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) return false;
            // The closed boundary, saved as the market's save point does, and its checkpoint screen.
            Test->TestTrue(TEXT("Saves are isolated"), Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(TEXT("travel-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            FMemoriaRunSnapshot Boundary = Run->GetRunSnapshot(); Boundary.CurrentChapter = 3; Boundary.Player.Grains = 37;
            Test->TestEqual(TEXT("The run reaches the boundary"), Run->RestoreRun(Boundary, Run->GetPlayerMemory()->GetDefinitions(), Run->GetPlayerMemory()->GetSnapshot(), Run->GetWorldCognition()->GetSnapshot()), EMemoriaMemoryResult::Success);
            Run->SetStoryFlag(TEXT("ch2_complete"), true);
            Test->TestTrue(TEXT("The boundary saves"), Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->SaveClosedBoundary(FVector2D(500, 340)));
            Test->TestTrue(TEXT("The checkpoint screen opens"), Host->ContinueCheckpoint());
            const auto View = Host->GetView();
            Test->TestTrue(TEXT("It offers the road to Chapter 3"), View.Choices.ContainsByPredicate([](const FMemoriaPresentedChoice& C) { return C.OriginalIndex == 4; }));
            RunId = Run->GetRunSnapshot().RunId; Memories = Run->GetPlayerMemory()->GetSnapshot().Owned.Num();
            Host->Confirm(4);
            ++Step; return false;
        }
        if (Step == 1)
        {
            if (!World->GetMapName().EndsWith(TEXT("L_BeltWaystation"))) return false;
            AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
            if (!Map || Host->GetState() == EMemoriaSliceState::Travelling) return false;
            Test->TestEqual(TEXT("The Belt Waystation takes up the road"), Host->GetChapterMap(), FString(TEXT("belt_waystation")));
            Test->TestTrue(TEXT("The same run travels"), Run->GetRunSnapshot().RunId == RunId && Run->GetRunSnapshot().Player.Grains == 37);
            Test->TestEqual(TEXT("Its memories travel with it"), Run->GetPlayerMemory()->GetSnapshot().Owned.Num(), Memories);
            Test->TestEqual(TEXT("Chapter 3"), Run->GetRunSnapshot().CurrentChapter, int64(3));
            Test->TestFalse(TEXT("Not a development run"), Host->GetTrace().Contains(TEXT("chapter:development_run")));
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started;
    int32 Step = 0, Memories = 0;
    FGuid RunId;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3TravelTest, "MemoriaVisual.Chapter3Travel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapter3TravelTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapter3TravelReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
