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
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S320: Chapter 3, the Belt Waystation, the first content-first chapter map. Entered directly (a
// development run at Chapter 3), the field is built from belt_waystation.gd's IR; the chapter title
// shows, then the arrival chain plays in order (arrival, the blank book and its toast, the night, the
// Class Seven wall message), the story trigger plays, and the east exit closes the chapter into 4.
class FChapter3Replay final : public IAutomationLatentCommand
{
public:
    explicit FChapter3Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapter3Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Chapter 3 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Chapter3") / (FString(Name) + TEXT(".png")), true, false); };
        auto Flag = [&](const TCHAR* Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        // Dialogue rows advance one per few frames; the order of the groups is recorded as they start.
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            const FString Id = Host->GetView().Header;
            if (Frame % 4 == 0)
            {
                if (bShotDialogue == false && Frame > 40) { bShotDialogue = true; Capture(TEXT("Chapter3Dialogue")); return false; }
                Host->Confirm();
            }
            return false;
        }
        switch (Step)
        {
        case 0:
        {
            Test->TestEqual(TEXT("The map is the Belt Waystation"), Map->GetMap(), FString(TEXT("belt_waystation")));
            Test->TestEqual(TEXT("A development run stands in Chapter 3"), Run->GetRunSnapshot().CurrentChapter, int64(3));
            int32 Solid = 0; for (int32 T : Spec->Tiles) Solid += Spec->IsSolid(T) ? 1 : 0;
            Test->TestEqual(TEXT("Every wall and ruin tile blocks"), Map->GetBlockerCount(), Solid);
            Test->TestTrue(TEXT("Arrel is the rigged figure"), Map->GetArrelFigure() && Map->GetArrelFigure()->IsRigged());
            Test->TestTrue(TEXT("The chapter title shows"), Map->GetCard() && Map->GetCard()->IsShowing());
            ++Step; Mark = Frame; break;
        }
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("Chapter3Title"));
            // The whole arrival chain: arrival, blank book, night, the wall message.
            if (Flag(TEXT("ch3_class_seven_message")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("Arrival seen"), Flag(TEXT("ch3_arrived")));
                Test->TestTrue(TEXT("The blank book is found and held"), Flag(TEXT("ch3_blank_book")) && Flag(TEXT("has_blank_book")));
                Test->TestTrue(TEXT("The night passes"), Flag(TEXT("ch3_waystation_night")));
                const auto& Trace = Host->GetTrace();
                const int32 A = Trace.IndexOfByKey(TEXT("field:start:waystation_arrival")), B = Trace.IndexOfByKey(TEXT("field:start:blank_book_discovery")),
                    N = Trace.IndexOfByKey(TEXT("field:start:waystation_night")), W = Trace.IndexOfByKey(TEXT("field:start:class_seven_wall_message"));
                Test->TestTrue(TEXT("The chain plays in the source's order"), A >= 0 && A < B && B < N && N < W);
                Test->TestTrue(TEXT("The blank book toast is raised"), Trace.ContainsByPredicate([](const FString& E) { return E.StartsWith(TEXT("toast:")) && (E.Contains(TEXT("Blank Book")) || E.Contains(TEXT("백서"))); }));
                Test->TestFalse(TEXT("The chapter is still open"), Flag(TEXT("ch3_complete")));
                Capture(TEXT("Chapter3Field"));
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 3000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (Frame < Mark + 10) break;
            // The belt_atmosphere story trigger in the south-west ruins.
            Place(Spec->Triggers[0].Rect.Center());
            ++Step; Mark = Frame; break;
        case 3:
            if (Frame < Mark + 20) break;
            Test->TestTrue(TEXT("The story trigger played"), Flag(TEXT("ch3_belt_walk")) && Host->GetTrace().Contains(TEXT("field:start:belt_atmosphere")));
            // The east exit: the departure, then Chapter 4.
            Place(Spec->Exit.Rect.Center());
            ++Step; Mark = Frame; break;
        case 4:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The departure closes Chapter 3"), Flag(TEXT("ch3_complete")) && Host->GetTrace().Contains(TEXT("field:start:waystation_departure")));
                Test->TestEqual(TEXT("The run moves on to Chapter 4"), Run->GetRunSnapshot().CurrentChapter, int64(4));
                Test->TestTrue(TEXT("The completion card shows"), Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 1200) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 5:
            if (Frame == Mark + 40) { Capture(TEXT("Chapter3Complete")); }
            return Frame > Mark + 44;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0;
    bool bFixed = false, bOldFixed = false, bShotDialogue = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3Test, "MemoriaVisual.Chapter3", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapter3Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapter3Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
