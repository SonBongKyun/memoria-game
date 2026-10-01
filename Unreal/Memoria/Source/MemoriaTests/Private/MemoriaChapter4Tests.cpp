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
// S321: Chapter 4, Drift Shelter, the second content-first chapter map. Entered directly (a development
// run at Chapter 4), the chapter title shows and the arrival chain plays in drift_shelter.gd's order
// (arrival, reading deterioration, the anchoring session, the night watch). The map has no story
// triggers, and its chests and clues wait behind _can_resume_ch4_exploration: closed during the chapter,
// and still closed after the east exit, because the departure readies Chapter 5's classifier scene
// (canon_ch5_classifier_ready) and raises the boundary notice; after the card the classifier scene begins (S322).
class FChapter4Replay final : public IAutomationLatentCommand
{
public:
    explicit FChapter4Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapter4Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Chapter 4 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Chapter4") / (FString(Name) + TEXT(".png")), true, false); };
        auto Flag = [&](const TCHAR* Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        const FVector2D Campfire = Spec->Clues.Num() > 0 ? Spec->Clues[0].Origin + FVector2D(Spec->TileSize * .5, Spec->TileSize * .5) : FVector2D::ZeroVector;
        if (Host->GetState() == EMemoriaSliceState::Field)
        {
            if (Frame % 4 == 0)
            {
                if (!bShotDialogue && Frame > 40) { bShotDialogue = true; Capture(TEXT("Chapter4Dialogue")); return false; }
                Host->Confirm();
            }
            return false;
        }
        switch (Step)
        {
        case 0:
        {
            Test->TestEqual(TEXT("The map is Drift Shelter"), Map->GetMap(), FString(TEXT("drift_shelter")));
            Test->TestEqual(TEXT("A development run stands in Chapter 4"), Run->GetRunSnapshot().CurrentChapter, int64(4));
            int32 Solid = 0; for (int32 T : Spec->Tiles) Solid += Spec->IsSolid(T) ? 1 : 0;
            Test->TestEqual(TEXT("Every wall, rubble and concrete tile blocks"), Map->GetBlockerCount(), Solid);
            Test->TestTrue(TEXT("No story triggers, as drift_shelter.gd"), Spec->Triggers.Num() == 0);
            Test->TestTrue(TEXT("The post-chapter sections wait on the later canon flags"), Spec->ResumeBlocked.Contains(TEXT("canon_ch5_classifier_ready")));
            Test->TestTrue(TEXT("Arrel is the rigged figure"), Map->GetArrelFigure() && Map->GetArrelFigure()->IsRigged());
            Test->TestTrue(TEXT("The chapter title shows"), Map->GetCard() && Map->GetCard()->IsShowing());
            ++Step; Mark = Frame; break;
        }
        case 1:
            if (Frame == Mark + 40) Capture(TEXT("Chapter4Title"));
            if (Flag(TEXT("ch4_night_watch")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("Arrival, reading loss and anchoring seen"), Flag(TEXT("ch4_arrived")) && Flag(TEXT("ch4_reading_loss")) && Flag(TEXT("ch4_anchoring")));
                const auto& Trace = Host->GetTrace();
                const int32 A = Trace.IndexOfByKey(TEXT("field:start:drift_arrival")), R = Trace.IndexOfByKey(TEXT("field:start:reading_deterioration")),
                    S = Trace.IndexOfByKey(TEXT("field:start:anchoring_session")), N = Trace.IndexOfByKey(TEXT("field:start:night_watch"));
                Test->TestTrue(TEXT("The chain plays in the source's order"), A >= 0 && A < R && R < S && S < N);
                Test->TestFalse(TEXT("The chapter is still open"), Flag(TEXT("ch4_complete")));
                Capture(TEXT("Chapter4Field"));
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 4000) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (Frame < Mark + 10) break;
            // The campfire clue is closed while the chapter is open.
            Place(Campfire);
            ++Step; Mark = Frame; break;
        case 3:
            // S335: the campfire and the eight rubble heaps are Codex's models; a close look at each on the way to the exit.
            if (Frame == Mark + 1) Test->TestEqual(TEXT("Drift Shelter's props are models"), Map->GetModelPropCount(), 9);
            if (Frame == Mark + 15) Capture(TEXT("Chapter4Campfire"));
            if (Frame == Mark + 20) { Test->TestFalse(TEXT("Clues wait for the chapter's end"), Flag(TEXT("clue_drift_campfire"))); Place(FVector2D(600, 350)); }
            if (Frame == Mark + 35) Capture(TEXT("Chapter4Rubble"));
            if (Frame < Mark + 40) break;
            Place(Spec->Exit.Rect.Center());
            ++Step; Mark = Frame; break;
        case 4:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The departure closes Chapter 4"), Flag(TEXT("ch4_complete")) && Host->GetTrace().Contains(TEXT("field:start:drift_departure")));
                Test->TestTrue(TEXT("Chapter 5's classifier scene is readied"), Flag(TEXT("canon_ch5_classifier_ready")));
                Test->TestEqual(TEXT("The run keeps the source's chapter number"), Run->GetRunSnapshot().CurrentChapter, int64(4));
                Test->TestTrue(TEXT("The boundary notice is raised"), Host->GetTrace().ContainsByPredicate([](const FString& E) { return E.StartsWith(TEXT("toast:")) && (E.Contains(TEXT("measuring")) || E.Contains(TEXT("측정"))); }));
                Test->TestTrue(TEXT("The completion card shows"), Map->GetCard()->IsShowing());
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 1200) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 5:
            if (Frame == Mark + 40) Capture(TEXT("Chapter4Complete"));
            if (Frame < Mark + 60) break;
            // Chapter 5 is not ported: no road on, and the readied classifier keeps the clues closed.
            Place(Campfire);
            ++Step; Mark = Frame; break;
        case 6:
            if (Frame < Mark + 60) break;
            Test->TestFalse(TEXT("The classifier's readiness keeps the clues closed"), Flag(TEXT("clue_drift_campfire")));
            ++Step; Mark = Frame; break;
        case 7:
            // S322: after the card, the road leads into Chapter 5's classifier scene (MemoriaVisual.Chapter5 plays it).
            if (Host->GetTrace().Contains(TEXT("vn:start:ch5_classifier")))
            {
                Test->TestEqual(TEXT("The Classifier opens Chapter 5"), Run->GetRunSnapshot().CurrentChapter, int64(5));
                Test->TestFalse(TEXT("The Chapter 4 boundary is consumed"), Flag(TEXT("canon_ch5_classifier_ready")));
                return true;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The classifier scene never began")); return true; }
            break;
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter4Test, "MemoriaVisual.Chapter4", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapter4Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapter4Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
