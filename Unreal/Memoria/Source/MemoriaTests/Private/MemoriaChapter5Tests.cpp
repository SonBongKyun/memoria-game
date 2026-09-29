#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaClassifier.h"
#include "Run/MemoriaRunSubsystem.h"
#include "World/MemoriaWorldCognition.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Misc/Guid.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// S322: Chapter 5, The Classifier. From Drift Shelter's closed Chapter 4 (a development run whose Chapter 2
// route request reached Malet), the east exit's departure readies the classifier scene; after the card the
// entry consumes the boundary, freezes Malet's report as a Kairos fact (identified: Arrel), and plays the
// ch5_classifier VN with the identified lines. Its last step readies Chapter 6 and returns to Drift Shelter,
// where the card names the road on to Chapter 6, The Seam.
class FChapter5Replay final : public IAutomationLatentCommand
{
public:
    explicit FChapter5Replay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapter5Replay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 200) { Test->AddError(FString::Printf(TEXT("Chapter 5 timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = World->GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/Chapter5") / (FString(Name) + TEXT(".png")), true, false); };
        auto Flag = [&](const TCHAR* Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        // Dialogue and the VN advance one row every few frames.
        const auto State = Host->GetState();
        if (State == EMemoriaSliceState::Field || State == EMemoriaSliceState::VN)
        {
            if (State == EMemoriaSliceState::VN && !bShotVN && ++VNFrames > 30) { bShotVN = true; Capture(TEXT("Chapter5Classifier")); return false; }
            // Kairos's identified line, held until its portrait has faded in.
            if (State == EMemoriaSliceState::VN && bShotVN && !bShotReport && Flag(TEXT("ch5_malet_report_identified_arrel")) && Host->GetTrace().Contains(TEXT("vn:step:ch5_classifier:13")))
            {
                if (!ReportAt) ReportAt = Frame;
                if (Frame >= ReportAt + 24) { bShotReport = true; Capture(TEXT("Chapter5Report")); }
                return false;
            }
            if (Frame % 4 == 0) Host->Confirm();
            return false;
        }
        switch (Step)
        {
        case 0:
            // Chapter 4 closed up to its night watch, and Chapter 2's route request on Malet's record.
            for (const TCHAR* F : {TEXT("ch4_arrived"), TEXT("ch4_reading_loss"), TEXT("ch4_anchoring"), TEXT("ch4_night_watch")}) Run->SetStoryFlag(F, true);
            Run->GetWorldCognition()->SeedMaletRoute(true);
            // The VN's chapter-transition autosave writes only to an isolated test leaf.
            Test->TestTrue(TEXT("Saves are isolated"), World->GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(TEXT("ch5-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            Test->TestEqual(TEXT("Drift Shelter"), Map->GetMap(), FString(TEXT("drift_shelter")));
            ++Step; Mark = Frame; break;
        case 1:
            if (Frame < Mark + 30) break;
            Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Spec->Exit.Rect.Center()) + FVector(0, 0, Pawn->GetActorLocation().Z));
            ++Step; Mark = Frame; break;
        case 2:
            // The departure, the card, then the classifier VN, which ends by returning to Drift Shelter.
            if (Flag(TEXT("canon_ch6_seam_ready")) && State == EMemoriaSliceState::Exploration && Map->IsChapterComplete() && !Map->GetMap().IsEmpty() && Host->GetTrace().Contains(TEXT("chapter:travel:drift_shelter")))
            {
                const auto& Trace = Host->GetTrace();
                Test->TestTrue(TEXT("The classifier scene played"), Trace.Contains(TEXT("vn:start:ch5_classifier")));
                Test->TestTrue(TEXT("Malet's report named Arrel"), Trace.Contains(TEXT("classifier:report:identified_arrel")));
                Test->TestTrue(TEXT("Kairos keeps the identified fact"), Run->GetWorldCognition()->KnowsFact(MemoriaClassifier::Kairos, MemoriaClassifier::IdentifiedFact));
                Test->TestTrue(TEXT("The report flags select the identified lines"), Flag(TEXT("ch5_malet_report_identified_arrel")) && !Flag(TEXT("ch5_malet_report_requester_unknown")));
                Test->TestTrue(TEXT("The identified lines play"), Trace.Contains(TEXT("vn:step:ch5_classifier:12")) && Trace.Contains(TEXT("vn:step:ch5_classifier:13")));
                Test->TestFalse(TEXT("The unknown lines do not"), Trace.Contains(TEXT("vn:step:ch5_classifier:14")) || Trace.Contains(TEXT("vn:step:ch5_classifier:15")));
                Test->TestTrue(TEXT("The Chapter 4 boundary is consumed"), !Flag(TEXT("canon_ch5_classifier_ready")) && Flag(TEXT("ch5_classifier_started")) && Flag(TEXT("ch5_kairos_seen")));
                Test->TestTrue(TEXT("Chapter 5 completes"), Trace.Contains(TEXT("vn:chapter_complete:5")));
                Test->TestEqual(TEXT("The run stands in Chapter 5"), Run->GetRunSnapshot().CurrentChapter, int64(5));
                Test->TestEqual(TEXT("Back at Drift Shelter"), Host->GetChapterMap(), FString(TEXT("drift_shelter")));
                Test->TestTrue(TEXT("The card names the road on"), Map->GetCard() && Map->GetCard()->IsShowing());
                Test->TestTrue(TEXT("The next journey is named"), Trace.ContainsByPredicate([](const FString& E) { return E.StartsWith(TEXT("toast:")) && (E.Contains(TEXT("Chapter 6")) || E.Contains(TEXT("6장"))); }));
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 6000) { Test->AddError(TEXT("The classifier scene never returned to Drift Shelter")); return true; }
            break;
        case 3:
            if (Frame == Mark + 40) Capture(TEXT("Chapter5Complete"));
            // The classifier does not replay once the story has reached Chapter 6.
            if (Frame > Mark + 120)
            {
                int32 Starts = 0; for (const FString& E : Host->GetTrace()) Starts += E == TEXT("vn:start:ch5_classifier") ? 1 : 0;
                Test->TestEqual(TEXT("The classifier plays once"), Starts, 1);
                return true;
            }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, VNFrames = 0, ReportAt = 0;
    bool bFixed = false, bOldFixed = false, bShotVN = false, bShotReport = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter5Test, "MemoriaVisual.Chapter5", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapter5Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_DriftShelter"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapter5Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
