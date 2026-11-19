#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCoordinates.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaSaveSlotsWidget.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString ReadSave(const FString& Path) { FString Text; FFileHelper::LoadFileToString(Text, *Path); return Text; }

// Uses real controller InputKey dispatch for every pause/browser action. Fixture-only direct disk writes
// seed destination slots and simulate an external corrupt writer; they stay under a unique test leaf.
class FS362SaveControllerReplay final : public IAutomationLatentCommand
{
public:
    explicit FS362SaveControllerReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FS362SaveControllerReplay() override
    {
        if (Settings.IsValid()) { Settings->SetTextPercent(OldTextPercent); Settings->SetLocale(OldLocale); }
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 320)
        {
            Test->AddError(FString::Printf(TEXT("S362 save controller replay timeout at step %d"), Step));
            return true;
        }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2f) return false;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        auto* Fight = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
            OldTextPercent = Settings->GetTextPercent(); OldLocale = Settings->GetLocale();
            Settings->SetTextPercent(100); Settings->SetLocale(TEXT("en")); Run->SetLocale(TEXT("en"));
            Test->TestTrue(TEXT("Controller replay configures only isolated disk storage"), C->ConfigureTestStorage(TEXT("s362flow-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        }
        ++Frame;
        // The actual Belt arrival is allowed to finish before opening a save modal. No run restore/reset
        // is used as setup: that would reset the Narrative owner and hide input integration defects.
        if (Step == 0)
        {
            if (Host->GetState() == EMemoriaSliceState::Field || Host->GetState() == EMemoriaSliceState::VN)
            { if (Frame % 4 == 0) Host->Confirm(); return false; }
            if (Host->GetChapterMap() != TEXT("belt_waystation") || Host->GetState() != EMemoriaSliceState::Exploration ||
                !Run->GetRunSnapshot().GetFlag(TEXT("ch3_class_seven_message"))) return false;
            if (!Fight || Fight->LiveMonsterCount() != 0) return false;
        }
        // One physical gesture every eight rendered frames, including while the world is paused.
        if (Frame % 8 != 0) return false;
        auto Tap = [&](const FKey& Key)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
        };
        auto Down = [&](int32 Count) { for (int32 I = 0; I < Count; ++I) Tap(EKeys::Gamepad_DPad_Down); };
        auto Capture = [&](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S362/SaveSlots") / (FString(Name) + TEXT(".png")), true, false);
        };
        auto AssertAutosave = [&]()
        {
            Test->TestEqual(TEXT("Browser never overwrites old boundary autosave"), ReadSave(C->GetSlotPath()), AutoBoundary);
            Test->TestEqual(TEXT("Browser never overwrites old chapter autosave"), ReadSave(C->GetChapterSlotPath()), AutoChapter);
            Test->TestEqual(TEXT("Browser never overwrites old map autosave"), ReadSave(C->GetMapSlotPath()), AutoMap);
        };
        auto* Browser = PC->GetSaveSlotsWidget();
        switch (Step)
        {
        case 0:
            Run->SetStoryFlag(TEXT("ch2_complete"), true); Run->SetStoryFlag(TEXT("ch2_arrived"), true); Run->SetStoryFlag(TEXT("ch2_arrival_vn_seen"), true);
            Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(BeltPosition) + FVector(0, 0, Pawn->GetActorLocation().Z));
            // Use the existing combat API to seed a nonmax HP fixture without replacing the run or
            // resetting its Narrative owner. Manual loads must preserve this exact saved value.
            Fight->Revive(.63f); LatestHp = Run->GetRunSnapshot().Player.Hp;
            Test->TestTrue(TEXT("Fixture starts with real nonmax saved HP"), LatestHp > 0 && LatestHp < Run->GetRunSnapshot().Player.MaxHp);
            Test->TestTrue(TEXT("Existing autosave fixture uses the old map adapter"), C->SaveChapterMap(TEXT("belt_waystation"), BeltPosition));
            AutoBoundary = ReadSave(C->GetSlotPath()); AutoChapter = ReadSave(C->GetChapterSlotPath()); AutoMap = ReadSave(C->GetMapSlotPath());
            RunId = Run->GetRunSnapshot().RunId;
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Keyboard opens actual paused field menu"), PC->GetPauseWidget() && World->IsPaused());
            Down(2); Tap(EKeys::Gamepad_FaceButton_Bottom);
            if (!Test->TestNotNull(TEXT("Physical Dpad/A opens Save browser"), PC->GetSaveSlotsWidget())) return true;
            Test->TestTrue(TEXT("Save browser has three manual slots, retains Pause owner"), PC->GetSaveSlotsWidget()->GetMode() == EMemoriaSaveBrowserMode::Save &&
                PC->GetSaveSlotsWidget()->GetRowCount() == 4 && PC->GetPauseWidget() && World->IsPaused());
            ++Step; break;
        case 1:
            Capture(TEXT("Save100Empty")); ++Step; break;
        case 2:
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("Physical A saves selected empty manual slot"), C->GetManualSlotSummary(1).CanLoad() && Browser && !Browser->IsAskingOverwrite());
            FirstManual = ReadSave(C->GetManualSlotPath(1));
            Run->AddGrains(17); Run->SetStoryFlag(TEXT("s362_ui_replaced"), true);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("A on occupied slot asks with Keep selected"), Browser && Browser->IsAskingOverwrite() && !Browser->WillReplaceOnConfirm());
            ++Step; break;
        case 3:
            Capture(TEXT("OverwriteDefaultsKeep")); ++Step; break;
        case 4:
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("Default A keeps bytes unchanged"), Browser && !Browser->IsAskingOverwrite() && ReadSave(C->GetManualSlotPath(1)) == FirstManual);
            Tap(EKeys::Enter);
            Test->TestTrue(TEXT("Keyboard Enter asks the same replacement question"), Browser && Browser->IsAskingOverwrite());
            Tap(EKeys::Gamepad_FaceButton_Right);
            Test->TestTrue(TEXT("Controller B cancels only overwrite question"), Browser && !Browser->IsAskingOverwrite() && PC->GetPauseWidget() && World->IsPaused());
            Test->TestEqual(TEXT("B cancellation never writes"), ReadSave(C->GetManualSlotPath(1)), FirstManual);
            ++Step; break;
        case 5:
            Tap(EKeys::Enter); Tap(EKeys::Gamepad_DPad_Left);
            Test->TestTrue(TEXT("Dpad explicitly selects Replace"), Browser && Browser->WillReplaceOnConfirm());
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("Explicit replacement writes new state and retains old backup"), Browser && !Browser->IsAskingOverwrite() &&
                ReadSave(C->GetManualSlotPath(1)) != FirstManual && ReadSave(C->GetManualSlotPath(1) + TEXT(".bak")) == FirstManual);
            LatestManual = ReadSave(C->GetManualSlotPath(1)); LatestGrains = Run->GetRunSnapshot().Player.Grains;
            AssertAutosave(); ++Step; break;
        case 6:
            Capture(TEXT("Save100Committed")); ++Step; break;
        case 7:
            Tap(EKeys::Gamepad_FaceButton_Right);
            Test->TestTrue(TEXT("B closes browser while field remains paused"), !PC->GetSaveSlotsWidget() && PC->GetPauseWidget() && World->IsPaused());
            Settings->SetTextPercent(125);
            Tap(EKeys::Enter);
            Test->TestTrue(TEXT("Keyboard reopens same Save row at125percent"), PC->GetSaveSlotsWidget() && PC->GetSaveSlotsWidget()->GetSelectedSlot() == 1);
            ++Step; break;
        case 8:
            Capture(TEXT("Save125Committed")); ++Step; break;
        case 9:
            Tap(EKeys::Escape);
            Test->TestTrue(TEXT("Esc closes browser, keeps Pause"), !PC->GetSaveSlotsWidget() && PC->GetPauseWidget() && World->IsPaused());
            Tap(EKeys::Gamepad_FaceButton_Right);
            Test->TestTrue(TEXT("Second B resumes field and clears modal ownership"), !PC->GetPauseWidget() && !World->IsPaused() && Fight && Fight->CanMove());
            // Seed two destination snapshots only inside this isolated leaf. Arrival flags are taken
            // directly from the ported Drift spec to keep the remainder about save ownership/routing.
            Test->TestTrue(TEXT("Manual2 boundary fixture"), C->SaveManualSlot(2, TEXT("verdan_market"), VerdanPosition));
            BoundaryManual = ReadSave(C->GetManualSlotPath(2));
            Run->SetCurrentChapter(4); Run->AddGrains(23); Run->SetStoryFlag(TEXT("s362_drift_fixture"), true);
            Fight->Revive(.41f); DriftHp = Run->GetRunSnapshot().Player.Hp;
            Test->TestTrue(TEXT("Drift fixture uses distinct nonmax HP"), DriftHp > 0 && DriftHp < Run->GetRunSnapshot().Player.MaxHp && DriftHp != LatestHp);
            if (const auto* Drift = MemoriaChapterMaps::Find(TEXT("drift_shelter")))
            {
                DriftPosition = Drift->Spawn;
                for (const auto& Link : Drift->Sequence)
                {
                    if (!Link.Flag.IsEmpty()) Run->SetStoryFlag(Link.Flag, true);
                    for (const auto& Flag : Link.Flags) Run->SetStoryFlag(Flag, true);
                }
            }
            Test->TestTrue(TEXT("Manual3 Drift fixture"), C->SaveManualSlot(3, TEXT("drift_shelter"), DriftPosition));
            DriftGrains = Run->GetRunSnapshot().Player.Grains;
            Run->SetCurrentChapter(3);
            Tap(EKeys::Escape); Down(3); Tap(EKeys::Gamepad_FaceButton_Bottom);
            if (!Test->TestNotNull(TEXT("Dpad/A opens actual Load browser"), PC->GetSaveSlotsWidget())) return true;
            Test->TestTrue(TEXT("Existing newest autosave remains distinct row0"), PC->GetSaveSlotsWidget()->GetMode() == EMemoriaSaveBrowserMode::Load &&
                PC->GetSaveSlotsWidget()->GetSelectedSlot() == 0 && C->FindContinue() == EMemoriaContinueSource::Map);
            Down(2);
            Test->TestTrue(TEXT("Dpad selects boundary manual2"), PC->GetSaveSlotsWidget()->GetSelectedSlot() == 2);
            ++Step; break;
        case 10:
            Capture(TEXT("Load125Regions")); ++Step; break;
        case 11:
        {
            // Corrupt after the browser cached Ready. The Host must revalidate before closing/travel.
            const auto Id = Run->GetRunSnapshot().RunId; const auto Grains = Run->GetRunSnapshot().Player.Grains;
            const int64 HpBefore = Run->GetRunSnapshot().Player.Hp;
            Test->TestTrue(TEXT("External corrupt fixture stays within test storage"), FFileHelper::SaveStringToFile(TEXT("{broken"), *C->GetManualSlotPath(2)));
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Browser = PC->GetSaveSlotsWidget();
            Test->TestTrue(TEXT("Stale valid UI cannot travel or replace a run after disk corruption"), Browser && PC->GetPauseWidget() && World->IsPaused() &&
                Run->GetRunSnapshot().RunId == Id && Run->GetRunSnapshot().Player.Grains == Grains && Host->GetChapterMap() == TEXT("belt_waystation"));
            Test->TestEqual(TEXT("Corrupt preflight cannot heal or damage live HP"), Run->GetRunSnapshot().Player.Hp, HpBefore);
            Test->TestTrue(TEXT("Corrupt row becomes disabled and explains failure"), Browser && !Browser->IsRowEnabled(2) && !Browser->GetNotice().IsEmpty());
            ++Step; break;
        }
        case 12:
            Capture(TEXT("LoadCorruptRefused")); ++Step; break;
        case 13:
            Test->TestTrue(TEXT("Fixture repairs exact original manual2 bytes"), FFileHelper::SaveStringToFile(BoundaryManual, *C->GetManualSlotPath(2)));
            if (Browser) Browser->Refresh();
            Tap(EKeys::Gamepad_DPad_Down);
            Test->TestTrue(TEXT("Physical navigation selects Drift manual3"), Browser && Browser->GetSelectedSlot() == 3 && Browser->IsRowEnabled(3));
            BeforeWorld = World; Tap(EKeys::Gamepad_FaceButton_Bottom); ++Step; break;
        case 14:
            if (World == BeforeWorld.Get() || Host->GetChapterMap() != TEXT("drift_shelter") || Host->GetState() != EMemoriaSliceState::Exploration) break;
            Test->TestTrue(TEXT("Manual3 loads across actual worlds into Drift"), Run->GetRunSnapshot().RunId == RunId && Run->GetRunSnapshot().CurrentChapter == 4 &&
                Run->GetRunSnapshot().Player.Grains == DriftGrains && Run->GetRunSnapshot().GetFlag(TEXT("s362_drift_fixture")));
            Test->TestEqual(TEXT("Cross-map Drift restore preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, DriftHp);
            Test->TestTrue(TEXT("Drift manual source position applied"), MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).Equals(DriftPosition, .5));
            Test->TestTrue(TEXT("Loaded world owns no old pause/browser widgets"), !PC->GetPauseWidget() && !PC->GetSaveSlotsWidget() && !World->IsPaused());
            AssertAutosave(); ++Step; break;
        case 15:
            Capture(TEXT("DriftManualLoaded")); ++Step; break;
        case 16:
            Tap(EKeys::Escape); Down(3); Tap(EKeys::Enter); Down(2);
            Browser = PC->GetSaveSlotsWidget();
            if (!Test->TestTrue(TEXT("Drift's actual Load chooses Verdan manual2"), Browser && Browser->GetSelectedSlot() == 2)) return true;
            BeforeWorld = World; Tap(EKeys::Gamepad_FaceButton_Bottom); ++Step; break;
        case 17:
            if (World == BeforeWorld.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Deferred) break;
            Test->TestTrue(TEXT("Manual boundary is owned by Verdan checkpoint screen"), Run->GetRunSnapshot().RunId == RunId && Run->GetRunSnapshot().CurrentChapter == 3 &&
                Run->GetRunSnapshot().Player.Grains == LatestGrains && !Run->GetRunSnapshot().GetFlag(TEXT("s362_drift_fixture")) && PC->GetNarrativeWidget());
            Test->TestEqual(TEXT("Cross-map Verdan restore preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, LatestHp);
            Test->TestTrue(TEXT("Verdan manual source position applied"), Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(VerdanPosition, .5));
            AssertAutosave(); ++Step; break;
        case 18:
            Capture(TEXT("VerdanManualCheckpoint")); ++Step; break;
        case 19:
            BeforeWorld = World;
            Test->TestTrue(TEXT("Existing checkpoint revisit action remains available after manual restore"), Host->RequestCheckpointRevisit());
            ++Step; break;
        case 20:
            if (World == BeforeWorld.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) break;
            Tap(EKeys::Escape); Down(3); Tap(EKeys::Enter); Down(2);
            Browser = PC->GetSaveSlotsWidget();
            if (!Test->TestTrue(TEXT("Verdan's actual Load selects same-world manual2"), Browser && Browser->GetSelectedSlot() == 2 && Browser->IsRowEnabled(2))) return true;
            {
                // A valid summary does not authorize a restore during domain event dispatch. This
                // deterministic reentrancy fixture reaches Root's same-world restore-failure path
                // after its valid preflight, without relying on a filesystem thread race.
                const int64 HpBefore = Run->GetRunSnapshot().Player.Hp;
                Test->TestTrue(TEXT("Same-world failure fixture begins with nonmax HP"), HpBefore > 0 && HpBefore < Run->GetRunSnapshot().Player.MaxHp);
                bool bAttempted = false; int32 Replaced = 0;
                auto* Memory = Run->GetPlayerMemory();
                const auto ReplacedHandle = Run->OnRunReplaced.AddLambda([&]() { ++Replaced; });
                const auto Observer = Memory->OnObserved.AddLambda([&](const auto&)
                {
                    if (bAttempted) return;
                    bAttempted = true; Tap(EKeys::Gamepad_FaceButton_Bottom);
                });
                const auto Burned = Run->BurnMemory(TEXT("sense_warm_light"));
                Memory->OnObserved.Remove(Observer); Run->OnRunReplaced.Remove(ReplacedHandle);
                Test->TestTrue(TEXT("Isolated dispatch fixture reached actual physical A load intent"), bAttempted && Burned == EMemoriaMemoryResult::Success);
                Test->TestEqual(TEXT("Failed same-world restore never replaces the run"), Replaced, 0);
                Test->TestEqual(TEXT("Failed same-world restore preserves exact live nonmax HP"), Run->GetRunSnapshot().Player.Hp, HpBefore);
                Browser = PC->GetSaveSlotsWidget();
                Test->TestTrue(TEXT("Failed same-world restore retains browser/Pause and notice"), Browser && PC->GetPauseWidget() && World->IsPaused() && !Browser->GetNotice().IsEmpty());
                Test->TestTrue(TEXT("Valid manual remains available after dispatch failure"), C->GetManualSlotSummary(2).CanLoad() && Host->GetState() == EMemoriaSliceState::Exploration);
            }
            ++Step; break;
        case 21:
            Capture(TEXT("SameWorldRestoreRefusedHpRetained")); ++Step; break;
        case 22:
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("Normal same-world restore succeeds after dispatch ends"), Host->GetState() == EMemoriaSliceState::Deferred &&
                !PC->GetSaveSlotsWidget() && !PC->GetPauseWidget() && !World->IsPaused());
            Test->TestEqual(TEXT("Successful same-world restore preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, LatestHp);
            Test->TestTrue(TEXT("Successful restore clears transient combat without a heal"), Fight && !Fight->IsDefeated() && !Fight->IsBlocking() && Fight->GetCharge() == 0.f && Fight->CanMove());
            AssertAutosave(); ++Step; break;
        case 23:
            Capture(TEXT("SameWorldManualLoadedHpRetained")); ++Step; break;
        case 24:
            BeforeWorld = World;
            Test->TestTrue(TEXT("Second existing checkpoint revisit remains available"), Host->RequestCheckpointRevisit());
            ++Step; break;
        case 25:
            if (World == BeforeWorld.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) break;
            // Explicit old-adapter fixture write: row0 now names this same-world boundary, so refusal
            // is tested during Restore rather than pretending queued cross-map travel can fail here.
            Test->TestTrue(TEXT("Old boundary adapter seeds row0 in isolated test storage"), C->SaveClosedBoundary(VerdanPosition));
            AutoBoundary = ReadSave(C->GetSlotPath()); AutoSavedHp = Run->GetRunSnapshot().Player.Hp;
            Test->TestTrue(TEXT("Newest autosave fixture is the current Verdan boundary"), C->FindContinue() == EMemoriaContinueSource::Boundary && AutoSavedHp > 0 && AutoSavedHp < Run->GetRunSnapshot().Player.MaxHp);
            Tap(EKeys::Escape); Down(3); Tap(EKeys::Enter);
            Browser = PC->GetSaveSlotsWidget();
            if (!Test->TestTrue(TEXT("Actual Load starts on valid autosave row0"), Browser && Browser->GetSelectedSlot() == 0 && Browser->IsRowEnabled(0))) return true;
            {
                const int64 HpBefore = Run->GetRunSnapshot().Player.Hp;
                bool bAttempted = false; int32 Replaced = 0;
                auto* Memory = Run->GetPlayerMemory();
                const auto ReplacedHandle = Run->OnRunReplaced.AddLambda([&]() { ++Replaced; });
                const auto Observer = Memory->OnObserved.AddLambda([&](const auto&)
                {
                    if (bAttempted) return;
                    bAttempted = true; Tap(EKeys::Gamepad_FaceButton_Bottom);
                });
                const auto Burned = Run->BurnMemory(TEXT("sense_warm_light"));
                Memory->OnObserved.Remove(Observer); Run->OnRunReplaced.Remove(ReplacedHandle);
                Test->TestTrue(TEXT("Physical autosave load intent reached its dispatch refusal"), bAttempted && Burned == EMemoriaMemoryResult::Success);
                Test->TestEqual(TEXT("Failed autosave restore never replaces the run"), Replaced, 0);
                Test->TestEqual(TEXT("Failed autosave restore preserves exact live nonmax HP"), Run->GetRunSnapshot().Player.Hp, HpBefore);
                Browser = PC->GetSaveSlotsWidget();
                Test->TestTrue(TEXT("Failed autosave load retains browser/Pause and notice"), Browser && PC->GetPauseWidget() && World->IsPaused() && !Browser->GetNotice().IsEmpty());
            }
            AssertAutosave(); ++Step; break;
        case 26:
            Capture(TEXT("AutoRestoreRefusedHpRetained")); ++Step; break;
        case 27:
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Test->TestTrue(TEXT("Normal autosave row0 load succeeds after dispatch ends"), Host->GetState() == EMemoriaSliceState::Deferred &&
                !PC->GetSaveSlotsWidget() && !PC->GetPauseWidget() && !World->IsPaused());
            Test->TestEqual(TEXT("Successful autosave load preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, AutoSavedHp);
            AssertAutosave(); ++Step; break;
        case 28:
            Capture(TEXT("AutoLoadedHpRetained")); ++Step; break;
        case 29:
            BeforeWorld = World;
            Test->TestTrue(TEXT("Autosave checkpoint revisit remains available"), Host->RequestCheckpointRevisit());
            ++Step; break;
        case 30:
            if (World == BeforeWorld.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Host->GetState() != EMemoriaSliceState::Exploration) break;
            Tap(EKeys::Escape); Down(3); Tap(EKeys::Enter); Down(1);
            Browser = PC->GetSaveSlotsWidget();
            if (!Test->TestTrue(TEXT("Verdan's actual Load chooses Belt manual1"), Browser && Browser->GetSelectedSlot() == 1)) return true;
            BeforeWorld = World; Tap(EKeys::Gamepad_FaceButton_Bottom); ++Step; break;
        case 31:
            if (World == BeforeWorld.Get() || Host->GetChapterMap() != TEXT("belt_waystation") || Host->GetState() != EMemoriaSliceState::Exploration) break;
            Test->TestTrue(TEXT("Manual1 returns to Belt with confirmed newer state"), Run->GetRunSnapshot().RunId == RunId && Run->GetRunSnapshot().CurrentChapter == 3 &&
                Run->GetRunSnapshot().Player.Grains == LatestGrains && Run->GetRunSnapshot().GetFlag(TEXT("s362_ui_replaced")) && !Run->GetRunSnapshot().GetFlag(TEXT("s362_drift_fixture")));
            Test->TestEqual(TEXT("Cross-map Belt restore preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, LatestHp);
            Test->TestTrue(TEXT("Belt manual source position applied"), MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).Equals(BeltPosition, .5));
            Test->TestEqual(TEXT("Loading never rewrites selected manual1"), ReadSave(C->GetManualSlotPath(1)), LatestManual);
            Test->TestEqual(TEXT("Backup remains the pre-confirmation manual1"), ReadSave(C->GetManualSlotPath(1) + TEXT(".bak")), FirstManual);
            AssertAutosave(); ++Step; break;
        case 32:
            Capture(TEXT("BeltManualLoaded")); ++Step; break;
        default:
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, OldTextPercent = 100;
    int64 LatestGrains = 0, DriftGrains = 0, LatestHp = 0, DriftHp = 0, AutoSavedHp = 0;
    bool bFixed = false, bOldFixed = false;
    FGuid RunId;
    FVector2D BeltPosition = FVector2D(400, 300), VerdanPosition = FVector2D(500, 340), DriftPosition = FVector2D(320, 512);
    FString FirstManual, LatestManual, BoundaryManual, AutoBoundary, AutoChapter, AutoMap, OldLocale;
    TWeakObjectPtr<UWorld> BeforeWorld;
    TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362SaveControllerFlow, "MemoriaSave.S362.ControllerFlow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362SaveControllerFlow::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362SaveControllerReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
