#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// New Game from the title flow, played through Chapter 1 in PIE to the Verdan field.
class FChapter1Journey final : public IAutomationLatentCommand
{
public:
    explicit FChapter1Journey(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapter1Journey() override
    { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 420) { Test->AddError(FString::Printf(TEXT("Chapter 1 journey timeout in phase %d at %s"), Phase, *LastCue)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || World->GetTimeSeconds() < .3) return false;
        auto* GI = World->GetGameInstance();
        auto* Host = GI->GetSubsystem<UMemoriaNarrativeSubsystem>(); auto* Run = GI->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Audio = GI->GetSubsystem<UMemoriaAudioSubsystem>(); auto* Checkpoint = GI->GetSubsystem<UMemoriaCheckpointSubsystem>();
        if (Phase == 0)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/60.0);
            Test->TestTrue(TEXT("Isolated autosave storage"), Checkpoint->ConfigureTestStorage(Leaf));
            FirstWorld = World; Phase = 1;
            // The title's New Game re-enters the VN host with the NewGame option.
            UGameplayStatics::OpenLevel(World, TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"), true, TEXT("NewGame"));
            return false;
        }
        if (World->GetTimeSeconds() == LastWorldTime) return false; // shader compiles pump frames without ticking PIE
        LastWorldTime = World->GetTimeSeconds();
        if (Phase == 1)
        {
            if (World == FirstWorld.Get() || !Host->IsNewGameRoute() || Host->GetState() != EMemoriaSliceState::VN || !PC->GetNarrativeWidget()) return false;
            const auto Start = Run->GetRunSnapshot();
            Test->TestEqual(TEXT("New Game chapter"), Start.CurrentChapter, int64(1));
            Test->TestEqual(TEXT("New Game ink"), Run->GetItemCount(TEXT("witness_ink")), int64(1));
            Test->TestTrue(TEXT("Starts at the cold open"), Host->GetView().CueKey == TEXT("ch1_cold_open:0"));
            Pulses = Audio->GetCueCount(TEXT("void_pulse")); Phase = 2; Hold = 0;
            return false;
        }
        auto Capture = [&](const FString& Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/Chapter1Journey")/(Name+TEXT(".png")), true, false);
            Captured.Add(Name);
        };
        if (Phase == 2)
        {
            auto* Widget = PC->GetNarrativeWidget();
            if (Host->GetState() == EMemoriaSliceState::Travelling || World->GetMapName().EndsWith(TEXT("L_VerdanHost"))) { Phase = 3; Hold = 0; return false; }
            if (Host->GetState() != EMemoriaSliceState::VN || !Widget) return false;
            const auto View = Host->GetView();
            if (View.CueKey != LastCue)
            {
                LastCue = View.CueKey; Hold = 0; ++Steps;
            }
            ++Hold;
            if (Hold == 2)
            {
                // Music follows each scene's declared bgm (SceneFlow.play).
                const FName Expected = View.CueKey.StartsWith(TEXT("ch1_cold_open")) ? FName(TEXT("dialogue_tense")) : View.CueKey.StartsWith(TEXT("ch2_")) ? FName(TEXT("ch2_verdan")) : FName(TEXT("ch1_forest"));
                Test->TestEqual(*(TEXT("Scene music at ")+View.CueKey), Audio->GetMusic(), Expected);
                Test->TestNotNull(*(TEXT("Rendered CG at ")+View.CueKey), Widget->DisplayedBackdrop());
                // A line over a full-scene story CG keeps the stage clear (_should_hide_portraits_for_cg_line).
                const bool StoryCg = View.bStepHasCg && (View.BackdropSource.Contains(TEXT("/generated/story_")) || View.BackdropSource.Contains(TEXT("/generated/dialogue_")));
                if (!View.PortraitSource.IsEmpty() && !View.Speaker.IsEmpty() && View.Choices.IsEmpty() && !StoryCg) Test->TestNotNull(*(TEXT("Rendered portrait at ")+View.CueKey), Widget->DisplayedPortrait());
            }
            // Captures of the presentation cues at their visible moment.
            FString Shot; int32 At = 0;
            if (View.CueKey == TEXT("ch1_cold_open:0")) { Shot = TEXT("Ch1_01_ColdOpen"); At = 60; }
            else if (View.CueKey == TEXT("ch1_cold_open:1")) { Shot = TEXT("Ch1_02_VoidImpact"); At = 19; }
            else if (View.CueKey == TEXT("ch1_cold_open:2")) { Shot = TEXT("Ch1_03_Choice"); At = 20; }
            else if (View.CueKey == TEXT("ch1_prologue:4")) { Shot = TEXT("Ch1_04_SystemLog"); At = 30; }
            else if (View.bDistorted && !Captured.Contains(TEXT("Ch1_05_Distorted"))) { Shot = TEXT("Ch1_05_Distorted"); At = 40; }
            else if (!View.Speaker.IsEmpty() && !View.PortraitSource.IsEmpty() && !View.bStepHasCg && View.CueKey.StartsWith(TEXT("ch1_prologue")) && !Captured.Contains(TEXT("Ch1_09_Speaker"))) { Shot = TEXT("Ch1_09_Speaker"); At = 40; }
            else if (!View.ChoiceTitle.IsEmpty() && View.CueKey.StartsWith(TEXT("ch1_void_beast")) && !Captured.Contains(TEXT("Ch1_06_VoidBeast"))) { Shot = TEXT("Ch1_06_VoidBeast"); At = 30; }
            else if (View.LedgerSerial > 0 && !Captured.Contains(TEXT("Ch1_07_Ledger"))) { Shot = TEXT("Ch1_07_Ledger"); At = 45; }
            if (!Shot.IsEmpty() && !Captured.Contains(Shot))
            {
                if (Hold < At) return false;
                const auto Probe = Widget->GetPresentationProbe();
                if (Shot == TEXT("Ch1_02_VoidImpact")) { Test->TestTrue(TEXT("Void flash on screen"), Probe.FlashAlpha > .02f); Test->TestTrue(TEXT("Push-in motion"), Probe.Motion == TEXT("push_in")); }
                if (Shot == TEXT("Ch1_04_SystemLog")) Test->TestTrue(TEXT("System log styled"), Probe.bSystemStyle);
                if (Shot == TEXT("Ch1_05_Distorted")) Test->TestTrue(TEXT("Distorted line styled"), Probe.bDistortedStyle);
                if (Shot == TEXT("Ch1_09_Speaker")) { Test->TestNotNull(TEXT("Speaker portrait on stage"), Widget->DisplayedPortrait()); Test->TestFalse(TEXT("Speaker side lit"), Probe.ActiveSide.IsEmpty()); Test->TestTrue(TEXT("Memory frame shown"), Probe.bStoryFrame); }
                if (Shot == TEXT("Ch1_07_Ledger")) { Test->TestTrue(TEXT("Ledger fully shown"), Probe.LedgerAlpha > .95f); Test->TestTrue(TEXT("Ledger text"), Probe.LedgerText.Contains(TEXT("THE LEDGER, CHAPTER 1"))); }
                Capture(Shot); return false;
            }
            if (Hold < 12) return false;
            if (!View.ChoiceTitle.IsEmpty())
            {
                if (!Picks.IsValidIndex(Pick)) { Test->AddError(TEXT("Ran out of source picks")); return true; }
                Host->Confirm(Picks[Pick++]);
            }
            else Host->Confirm(INDEX_NONE);
            return false;
        }
        // Phase 3: the arrival VN requested Verdan; the field host takes over.
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || (Host->GetState() != EMemoriaSliceState::Exploration && Host->GetState() != EMemoriaSliceState::Field)) return false;
        if (++Hold < 60) return false;
        const auto End = Run->GetRunSnapshot();
        Test->TestEqual(TEXT("Chapter 2 in Verdan"), End.CurrentChapter, int64(2));
        Test->TestTrue(TEXT("Chapter 1 complete"), End.GetFlag(TEXT("ch1_complete")));
        Test->TestTrue(TEXT("Arrival seen"), End.GetFlag(TEXT("ch2_arrival_vn_seen")));
        Test->TestTrue(TEXT("Song burned on this route"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(TEXT("daily_campfire_song")));
        Test->TestEqual(TEXT("Every source pick used"), Pick, Picks.Num());
        Test->TestTrue(TEXT("Whole route shown"), Steps > 80);
        Test->TestTrue(TEXT("Cold open pulse played"), Pulses >= 1 && Audio->GetCueCount(TEXT("void_pulse")) >= Pulses);
        Test->TestTrue(TEXT("Chapter autosave on disk"), IFileManager::Get().FileExists(*Checkpoint->GetChapterSlotPath()));
        Test->TestEqual(TEXT("Verdan music"), Audio->GetMusic(), FName(TEXT("ch2_verdan")));
        Test->TestTrue(TEXT("Travel recorded"), Host->GetTrace().Contains(TEXT("travel:verdan")));
        for (const TCHAR* Name : {TEXT("Ch1_01_ColdOpen"),TEXT("Ch1_02_VoidImpact"),TEXT("Ch1_03_Choice"),TEXT("Ch1_04_SystemLog"),TEXT("Ch1_05_Distorted"),TEXT("Ch1_06_VoidBeast"),TEXT("Ch1_07_Ledger"),TEXT("Ch1_09_Speaker")})
            Test->TestTrue(*(FString(TEXT("Captured "))+Name), Captured.Contains(Name));
        Capture(TEXT("Ch1_08_Verdan"));
        IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Checkpoint->GetChapterSlotPath()), false, true);
        return true;
    }
private:
    FAutomationTestBase* Test;
    double Started;
    uint64 LastFrame = 0;
    double LastWorldTime = -1;
    bool bFixed = false, bOldFixed = false;
    double OldDelta = 0;
    int32 Phase = 0, Hold = 0, Pick = 0, Steps = 0, Pulses = 0;
    TWeakObjectPtr<UWorld> FirstWorld;
    FString LastCue;
    TSet<FString> Captured;
    // Oracle case burn_song_strike: the song burn, the humming gate and the strike line.
    const TArray<int32> Picks = {0,0,0,0,3,0,0,0};
    const FString Leaf = TEXT("ch1-journey-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter1JourneyTest, "MemoriaVisual.Chapter1Journey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapter1JourneyTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapter1Journey(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
