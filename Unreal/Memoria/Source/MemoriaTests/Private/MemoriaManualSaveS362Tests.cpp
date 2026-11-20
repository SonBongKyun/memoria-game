#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaSaveSlotsWidget.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString FreshLeaf() { return TEXT("s362manual-") + FGuid::NewGuid().ToString(EGuidFormats::Digits); }
FString Read(const FString& Path) { FString Out; FFileHelper::LoadFileToString(Out, *Path); return Out; }
bool Setup(FAutomationTestBase& Test, UGameInstance& Game, const FString& Leaf)
{
    auto* Run = Game.GetSubsystem<UMemoriaRunSubsystem>();
    auto* Checkpoint = Game.GetSubsystem<UMemoriaCheckpointSubsystem>();
    if (!Test.TestTrue(TEXT("Synthetic storage stays disabled by default"), !Checkpoint->IsStorageEnabled()) ||
        !Test.TestTrue(TEXT("Fresh isolated storage leaf"), Checkpoint->ConfigureTestStorage(Leaf)) ||
        !Test.TestEqual(TEXT("Current chapter run"), Run->BeginStartingMemoryRun(3), EMemoriaMemoryResult::Success)) return false;
    Run->SetStoryFlag(TEXT("ch2_complete"), true); Run->SetStoryFlag(TEXT("ch3_arrived"), true);
    return true;
}
TArray<uint8> StateBytes(UMemoriaRunSubsystem& Run)
{
    TArray<uint8> Bytes; auto* Save = Run.CaptureSave();
    if (Save) UGameplayStatics::SaveGameToMemory(Save, Bytes);
    return Bytes;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManualDiskS362, "MemoriaSave.S362.ManualDiskRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManualDiskS362::RunTest(const FString&)
{
    const FString Leaf = FreshLeaf();
    TStrongObjectPtr<UGameInstance> A(NewObject<UGameInstance>()); A->Init();
    if (!Setup(*this, *A, Leaf)) { A->Shutdown(); return false; }
    auto* Run = A->GetSubsystem<UMemoriaRunSubsystem>(); auto* C = A->GetSubsystem<UMemoriaCheckpointSubsystem>();
    TestTrue(TEXT("Legacy autosave still writes"), C->SaveClosedBoundary(FVector2D(500, 340)));
    const FString AutoBytes = Read(C->GetSlotPath());
    TestTrue(TEXT("No manual slot joins Continue"), C->FindContinue() == EMemoriaContinueSource::Boundary && !C->HasManualSave());
    const auto BeforeBrowse = StateBytes(*Run);
    for (int32 Slot = 1; Slot <= 3; ++Slot)
    {
        const auto Info = C->GetManualSlotSummary(Slot);
        TestTrue(TEXT("Each manual slot begins empty"), Info.Slot == Slot && Info.State == EMemoriaSaveSlotState::Empty && !Info.CanLoad());
        TestTrue(TEXT("Empty slots have no payload-derived metadata"), Info.BurnedMemoryCount == 0 && Info.RecordTitle.IsEmpty() && Info.RecordTitleKo.IsEmpty());
    }
    TestTrue(TEXT("Browsing does not replace or mutate the live run"), BeforeBrowse == StateBytes(*Run));
    TestTrue(TEXT("Manual boundary uses its own file"), C->SaveManualSlot(1, TEXT("verdan_market"), FVector2D(504, 343)));
    Run->AddGrains(41); Run->SetStoryFlag(TEXT("s362_manual_belt"), true);
    TestTrue(TEXT("Manual Belt save"), C->SaveManualSlot(2, TEXT("belt_waystation"), FVector2D(412, 297)));
    const auto BeltRun = Run->GetRunSnapshot(); const auto BeltMemory = Run->GetPlayerMemory()->GetSnapshot();
    const FString BeltWorld = Run->GetWorldCognition()->ExportJson();
    Run->SetCurrentChapter(4);
    TestTrue(TEXT("Manual Drift save"), C->SaveManualSlot(3, TEXT("drift_shelter"), FVector2D(320, 512)));
    const auto Boundary = C->GetManualSlotSummary(1), Belt = C->GetManualSlotSummary(2), Drift = C->GetManualSlotSummary(3);
    TestTrue(TEXT("Summaries show validated chapter and canonical region"), Boundary.CanLoad() && Boundary.Map == TEXT("verdan_market") && Boundary.Chapter == 3 &&
        Belt.CanLoad() && Belt.Map == TEXT("belt_waystation") && Belt.Chapter == 3 && Drift.CanLoad() && Drift.Map == TEXT("drift_shelter") && Drift.Chapter == 4);
    TestTrue(TEXT("UTC summary is from the saved payload"), Belt.SavedAtUtc.GetTicks() > 0 && Belt.SavedAtUtc <= FDateTime::UtcNow());
    TestTrue(TEXT("Newest autosave excludes newer manual slots"), C->FindContinue() == EMemoriaContinueSource::Boundary);
    TestEqual(TEXT("Manual writes preserve legacy autosave bytes"), Read(C->GetSlotPath()), AutoBytes);
    TestTrue(TEXT("Manual slots have distinct paths"), C->GetManualSlotPath(1) != C->GetManualSlotPath(2) && C->GetManualSlotPath(2) != C->GetManualSlotPath(3));
    A->Shutdown();
    TStrongObjectPtr<UGameInstance> B(NewObject<UGameInstance>()); B->Init();
    Run = B->GetSubsystem<UMemoriaRunSubsystem>(); C = B->GetSubsystem<UMemoriaCheckpointSubsystem>();
    TestTrue(TEXT("Independent instance opens isolated leaf"), C->ConfigureTestStorage(Leaf));
    FString Map; FVector2D Position;
    TestTrue(TEXT("Independent instance restores manual slot"), C->RestoreManualSlot(2, Map, Position));
    TestTrue(TEXT("Correct map and saved position"), Map == TEXT("belt_waystation") && Position == FVector2D(412, 297));
    TestTrue(TEXT("Run identity, chapter and inventory restored"), Run->GetRunSnapshot().RunId == BeltRun.RunId && Run->GetRunSnapshot().CurrentChapter == BeltRun.CurrentChapter &&
        Run->GetRunSnapshot().Player.Grains == BeltRun.Player.Grains && Run->GetRunSnapshot().GetFlag(TEXT("s362_manual_belt")));
    TestTrue(TEXT("Memory and world domains restored"), Run->GetPlayerMemory()->GetSnapshot().BurnedHistory == BeltMemory.BurnedHistory &&
        Run->GetPlayerMemory()->GetSnapshot().Owned.Num() == BeltMemory.Owned.Num() && Run->GetWorldCognition()->ExportJson() == BeltWorld);
    TestTrue(TEXT("Boundary restore explicitly names the Verdan owner"), C->RestoreManualSlot(1, Map, Position) && Map.IsEmpty() && Position == FVector2D(504, 343));
    TestEqual(TEXT("Loads preserve legacy autosave"), Read(C->GetSlotPath()), AutoBytes);
    B->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManualSafetyS362, "MemoriaSave.S362.ManualSafetyRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManualSafetyS362::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    if (!Setup(*this, *Game, FreshLeaf())) { Game->Shutdown(); return false; }
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    const FVector2D First(410, 300), Second(440, 310);
    for (int32 Slot : {-1, 0, 4})
        TestFalse(TEXT("Manual slots are bounded to 1..3"), C->CanSaveManualSlot(Slot, TEXT("belt_waystation"), First) || C->SaveManualSlot(Slot, TEXT("belt_waystation"), First) || !C->GetManualSlotPath(Slot).IsEmpty());
    TestFalse(TEXT("Invalid map and out-of-map position rejected"), C->SaveManualSlot(1, TEXT("the_seam"), First) || C->SaveManualSlot(1, TEXT("belt_waystation"), FVector2D(-1, 3)));
    TestTrue(TEXT("Initial manual save"), C->SaveManualSlot(1, TEXT("belt_waystation"), First));
    const FString Path = C->GetManualSlotPath(1), Original = Read(Path);
    Run->SetStoryFlag(TEXT("s362_replacement"), true);
    TestFalse(TEXT("Existing slot requires explicit overwrite"), C->SaveManualSlot(1, TEXT("belt_waystation"), Second));
    TestEqual(TEXT("Refused overwrite retains primary bytes"), Read(Path), Original);
    TestTrue(TEXT("Confirmed overwrite commits"), C->SaveManualSlot(1, TEXT("belt_waystation"), Second, true));
    TestEqual(TEXT("Backup is exact previous valid save"), Read(Path + TEXT(".bak")), Original);
    TestTrue(TEXT("Fixture corrupts only isolated primary"), FFileHelper::SaveStringToFile(TEXT("{broken"), *Path));
    TestTrue(TEXT("Backup is explicit in summary"), C->GetManualSlotSummary(1).State == EMemoriaSaveSlotState::Backup);
    FString Map; FVector2D Position;
    TestTrue(TEXT("Actual corrupt primary recovers from backup"), C->RestoreManualSlot(1, Map, Position));
    TestTrue(TEXT("Recovery restores previous state"), Position == First && !Run->GetRunSnapshot().GetFlag(TEXT("s362_replacement")));
    TestEqual(TEXT("Recovery repairs exact primary bytes"), Read(Path), Original);
    TestTrue(TEXT("Recovery status is visible"), C->GetStatusText().Contains(TEXT("recovered")));
    // Copying a valid payload to another manual path cannot impersonate that slot.
    TestTrue(TEXT("Wrong-slot fixture copied"), FFileHelper::SaveStringToFile(Original, *C->GetManualSlotPath(2)));
    TestTrue(TEXT("Wrong-slot payload is disabled"), C->GetManualSlotSummary(2).State == EMemoriaSaveSlotState::Corrupt);
    TestTrue(TEXT("Both isolated copies can be corrupted"), FFileHelper::SaveStringToFile(TEXT("bad"), *Path) && FFileHelper::SaveStringToFile(TEXT("bad"), *(Path + TEXT(".bak"))));
    const auto Before = StateBytes(*Run); Map = TEXT("unchanged"); Position = FVector2D(13, 17);
    TestTrue(TEXT("Corrupt slot reports concise disabled state"), C->GetManualSlotSummary(1).State == EMemoriaSaveSlotState::Corrupt && !C->GetManualSlotSummary(1).CanLoad());
    TestFalse(TEXT("Corrupt restore refused"), C->RestoreManualSlot(1, Map, Position));
    TestTrue(TEXT("Failed restore preserves live state and outputs"), StateBytes(*Run) == Before && Map == TEXT("unchanged") && Position == FVector2D(13, 17));
    TestFalse(TEXT("Corrupt file also requires overwrite confirmation"), C->SaveManualSlot(1, TEXT("belt_waystation"), First));
    bool bNestedSaved = false;
    const auto Handle = Run->GetPlayerMemory()->OnObserved.AddLambda([&](const auto&) { bNestedSaved |= C->SaveManualSlot(3, TEXT("belt_waystation"), First); });
    Run->BurnMemory(TEXT("sense_warm_light")); Run->GetPlayerMemory()->OnObserved.Remove(Handle);
    TestFalse(TEXT("Half-dispatched events cannot persist a manual snapshot"), bNestedSaved);
    TestFalse(TEXT("Rejected dispatch writes no file"), IFileManager::Get().FileExists(*C->GetManualSlotPath(3)));
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManualMetadataS362, "MemoriaSave.S362.MetadataSummary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManualMetadataS362::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    if (!Setup(*this, *Game, FreshLeaf())) { Game->Shutdown(); return false; }
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    // Explicit isolated fixtures: arbitrary future flags and pre-VN classifier projection are not revealed records.
    for (const TCHAR* Flag : {TEXT("zero_burn_path"), TEXT("seal_refused"), TEXT("seal_weave"), TEXT("elia_separates"), TEXT("elia_stays"),
        TEXT("ch5_classifier_started"), TEXT("ch5_malet_report_requester_unknown")}) Run->SetStoryFlag(Flag, true);
    TestTrue(TEXT("Unrevealed-record fixture saves in isolated slot1"), C->SaveManualSlot(1, TEXT("belt_waystation"), FVector2D(410, 300)));
    const auto Unrevealed = C->GetManualSlotSummary(1);
    TestTrue(TEXT("Future choices and pre-reveal classifier titles stay hidden"), Unrevealed.CanLoad() && Unrevealed.BurnedMemoryCount == 0 && Unrevealed.RecordTitle.IsEmpty() && Unrevealed.RecordTitleKo.IsEmpty());
    Run->SetStoryFlag(TEXT("malet_deal_refused"), true);
    TestEqual(TEXT("One actual burn before the recorded choice save"), Run->BurnMemory(TEXT("sense_warm_light")), EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Recorded choice fixture saves in isolated slot2"), C->SaveManualSlot(2, TEXT("belt_waystation"), FVector2D(420, 310)));
    const auto Choice = C->GetManualSlotSummary(2);
    TestTrue(TEXT("Saved burn count and authored choice titles in both locales"), Choice.CanLoad() && Choice.BurnedMemoryCount == 1 &&
        Choice.RecordTitle == TEXT("Refused Malet's Deal") && Choice.RecordTitleKo == TEXT("말레트의 거래를 거절하다"));
    TestTrue(TEXT("Legacy autosave uses the same read-only metadata"), C->SaveClosedBoundary(FVector2D(500, 340)));
    const auto Auto = C->GetAutosaveSummary();
    TestTrue(TEXT("Autosave metadata comes from its validated snapshot"), Auto.CanLoad() && Auto.BurnedMemoryCount == 1 && Auto.RecordTitle == Choice.RecordTitle && Auto.RecordTitleKo == Choice.RecordTitleKo);
    Run->SetCurrentChapter(5); Run->SetStoryFlag(TEXT("canon_ch6_seam_ready"), true);
    TestTrue(TEXT("Completed classifier fixture saves in isolated slot3"), C->SaveManualSlot(3, TEXT("drift_shelter"), FVector2D(320, 512)));
    const auto Unknown = C->GetManualSlotSummary(3);
    TestTrue(TEXT("Already revealed classifier report takes priority over the earlier choice"), Unknown.CanLoad() && Unknown.BurnedMemoryCount == 1 &&
        Unknown.RecordTitle == TEXT("Classifier report: requester unknown") && Unknown.RecordTitleKo == TEXT("분류하는 자 보고: 요청자 불명"));
    // Change the live run after saving. Browsing must keep the snapshot's count/report, not project live state.
    Run->SetStoryFlag(TEXT("malet_deal_refused"), false); Run->SetStoryFlag(TEXT("malet_deal_accepted"), true);
    Run->SetStoryFlag(TEXT("ch5_malet_report_requester_unknown"), false); Run->SetStoryFlag(TEXT("ch5_malet_report_identified_arrel"), true);
    FString AnotherMemory;
    for (const auto& Memory : Run->GetPlayerMemory()->GetSnapshot().Owned)
        if (!Memory.bBurned && !Memory.bFaded) { AnotherMemory = Memory.Id; break; }
    TestTrue(TEXT("A second actual burn changes only the live fixture"), !AnotherMemory.IsEmpty() && Run->BurnMemory(AnotherMemory) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Saved choice metadata ignores newer live flags and burn count"), C->GetManualSlotSummary(2).BurnedMemoryCount == 1 && C->GetManualSlotSummary(2).RecordTitle == Choice.RecordTitle);
    TestTrue(TEXT("Saved report ignores the live classifier outcome"), C->GetManualSlotSummary(3).RecordTitle == Unknown.RecordTitle);
    TestTrue(TEXT("Explicit replacement records the other revealed classifier outcome"), C->SaveManualSlot(3, TEXT("drift_shelter"), FVector2D(324, 516), true));
    const auto Identified = C->GetManualSlotSummary(3);
    TestTrue(TEXT("Identified report and saved two-burn count localize exactly"), Identified.CanLoad() && Identified.BurnedMemoryCount == 2 &&
        Identified.RecordTitle == TEXT("Classifier report: Arrel identified") && Identified.RecordTitleKo == TEXT("분류하는 자 보고: 아렐 식별"));
    const FString Path = C->GetManualSlotPath(3);
    TestTrue(TEXT("Only isolated primary is corrupted for validated-backup preview"), FFileHelper::SaveStringToFile(TEXT("broken"), *Path));
    const FString PrimaryBefore = Read(Path), BackupBefore = Read(Path + TEXT(".bak")), AutoBefore = Read(C->GetSlotPath());
    const auto Before = StateBytes(*Run); const FString WorldBefore = Run->GetWorldCognition()->ExportJson();
    int32 Replacements = 0, MemoryEvents = 0;
    const auto ReplacedHandle = Run->OnRunReplaced.AddLambda([&] { ++Replacements; });
    const auto MemoryHandle = Run->GetPlayerMemory()->OnObserved.AddLambda([&](const auto&) { ++MemoryEvents; });
    const auto Backup = C->GetManualSlotSummary(3); C->GetManualSlotSummary(1); C->GetManualSlotSummary(2); C->GetAutosaveSummary();
    Run->OnRunReplaced.Remove(ReplacedHandle); Run->GetPlayerMemory()->OnObserved.Remove(MemoryHandle);
    TestTrue(TEXT("Backup preview uses its own validated earlier count and report"), Backup.State == EMemoriaSaveSlotState::Backup && Backup.BurnedMemoryCount == 1 &&
        Backup.RecordTitle == Unknown.RecordTitle && Backup.RecordTitleKo == Unknown.RecordTitleKo);
    TestTrue(TEXT("Metadata browsing applies no save, emits no live events and preserves both domains"), Replacements == 0 && MemoryEvents == 0 && StateBytes(*Run) == Before && Run->GetWorldCognition()->ExportJson() == WorldBefore);
    TestTrue(TEXT("Metadata browsing does not repair, rewrite or overwrite any save file"), Read(Path) == PrimaryBefore && Read(Path + TEXT(".bak")) == BackupBefore && Read(C->GetSlotPath()) == AutoBefore);
    TestTrue(TEXT("Both isolated copies are corrupted for disabled metadata preview"), FFileHelper::SaveStringToFile(TEXT("broken"), *(Path + TEXT(".bak"))));
    const auto Corrupt = C->GetManualSlotSummary(3);
    TestTrue(TEXT("Unvalidated data never supplies count or record metadata"), Corrupt.State == EMemoriaSaveSlotState::Corrupt && !Corrupt.CanLoad() &&
        Corrupt.BurnedMemoryCount == 0 && Corrupt.RecordTitle.IsEmpty() && Corrupt.RecordTitleKo.IsEmpty());
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManualBrowserS362, "MemoriaSave.S362.BrowserContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManualBrowserS362::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    if (!Setup(*this, *Game, FreshLeaf())) { Game->Shutdown(); return false; }
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    auto* Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
    TStrongObjectPtr<UMemoriaSaveSlotsWidget> Save(NewObject<UMemoriaSaveSlotsWidget>());
    Save->Configure(C, Settings, EMemoriaSaveBrowserMode::Save, TEXT("belt_waystation"), FVector2D(410, 300), true, 1.25f);
    int32 Saved = 0; Save->OnSaved.BindLambda([&](int32 Slot) { TestEqual(TEXT("Correct slot committed"), Slot, 1); ++Saved; });
    TestTrue(TEXT("Three manual slots and Back"), Save->GetRowCount() == 4 && Save->GetSelectedSlot() == 1 && Save->IsRowEnabled(0));
    Save->Confirm(); const FString Original = Read(C->GetManualSlotPath(1));
    TestTrue(TEXT("Empty slot saves once without a replacement prompt"), Saved == 1 && !Original.IsEmpty() && !Save->IsAskingOverwrite());
    Run->SetStoryFlag(TEXT("s362_ui_replaced"), true);
    Save->Confirm();
    TestTrue(TEXT("Occupied slot asks and defaults to keep"), Save->IsAskingOverwrite() && !Save->WillReplaceOnConfirm());
    Save->Confirm();
    TestTrue(TEXT("Default confirm cancels replacement"), Saved == 1 && Read(C->GetManualSlotPath(1)) == Original && !Save->IsAskingOverwrite());
    Save->Confirm(); TestTrue(TEXT("Back cancels only the prompt"), Save->Back() && !Save->IsAskingOverwrite());
    TestEqual(TEXT("Back never rewrites save"), Read(C->GetManualSlotPath(1)), Original);
    Save->Confirm(); Save->Adjust(-1);
    TestTrue(TEXT("Direction explicitly selects replacement"), Save->WillReplaceOnConfirm()); Save->Confirm();
    TestTrue(TEXT("Explicit replacement saves and backs up"), Saved == 2 && Read(C->GetManualSlotPath(1) + TEXT(".bak")) == Original && Read(C->GetManualSlotPath(1)) != Original);
    Save->Navigate(-1); TestTrue(TEXT("Navigation wraps to Back"), Save->GetSelectedRow() == 3 && Save->GetSelectedSlot() == INDEX_NONE);
    TestFalse(TEXT("Back at browser is returned to host"), Save->Back());
    TStrongObjectPtr<UMemoriaSaveSlotsWidget> Load(NewObject<UMemoriaSaveSlotsWidget>());
    Load->Configure(C, Settings, EMemoriaSaveBrowserMode::Load, FString(), FVector2D::ZeroVector, false);
    int32 SelectedSlot = INDEX_NONE; Load->OnLoadRequested.BindLambda([&](int32 Slot) { SelectedSlot = Slot; });
    TestTrue(TEXT("Load exposes existing autosave and three manual slots"), Load->GetRowCount() == 5 && Load->GetSelectedSlot() == 0 && !Load->IsRowEnabled(0));
    Load->Confirm(); TestEqual(TEXT("Empty autosave cannot request restore"), SelectedSlot, INDEX_NONE);
    Load->Navigate(1); Load->Confirm(); TestEqual(TEXT("Manual load is explicitly selected"), SelectedSlot, 1);
    SelectedSlot = INDEX_NONE;
    TestTrue(TEXT("Isolated corrupt slot fixture"), FFileHelper::SaveStringToFile(TEXT("broken"), *C->GetManualSlotPath(2)));
    Load->Refresh(); Load->Select(2); Load->Confirm();
    TestTrue(TEXT("Corrupt slot stays inspectable but cannot restore"), Load->GetSelectedSlot() == 2 && !Load->IsRowEnabled(2) && SelectedSlot == INDEX_NONE && !Load->GetNotice().IsEmpty());
    TestEqual(TEXT("English map is region rather than chapter title"), UMemoriaSaveSlotsWidget::RegionLabel(TEXT("belt_waystation"), false), FString(TEXT("Belt Waystation")));
    TestEqual(TEXT("Korean region"), UMemoriaSaveSlotsWidget::RegionLabel(TEXT("drift_shelter"), true), FString(TEXT("무너진 돌벽")));
    Save->Configure(C, Settings, EMemoriaSaveBrowserMode::Save, TEXT("belt_waystation"), FVector2D(410, 300), false);
    TestFalse(TEXT("Host field-safety refusal disables every save slot"), Save->IsRowEnabled(0) || Save->IsRowEnabled(1) || Save->IsRowEnabled(2));
    Game->Shutdown(); return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManualWindowsLongPathS362, "MemoriaSave.S362.WindowsLongPathCommit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FManualWindowsLongPathS362::RunTest(const FString&)
{
#if PLATFORM_WINDOWS
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    ON_SCOPE_EXIT { Game->Shutdown(); };
    if(!Setup(*this,*Game,FreshLeaf()))return false;
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* C=Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
    FString Boundary=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Validation/CheckpointTests"));
    FString Initial=C->StorageRoot;FPaths::NormalizeDirectoryName(Boundary);FPaths::NormalizeDirectoryName(Initial);
    if(!TestTrue(TEXT("Long-path fixture starts inside its fresh configured validation leaf"),FPaths::CollapseRelativeDirectories(Boundary) && FPaths::CollapseRelativeDirectories(Initial) && Initial.StartsWith(Boundary+TEXT("/"),ESearchCase::IgnoreCase)))return false;
    // Only two fixed, bounded nested components are added; the public80-char
    // leaf guard remains unchanged. This never selects the normal profile root.
    FString LongRoot=Initial/TEXT("windows-long-rename-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")/TEXT("windows-long-rename-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    LongRoot=FPaths::ConvertRelativePathToFull(LongRoot);FPaths::NormalizeDirectoryName(LongRoot);
    if(!TestTrue(TEXT("Extended disk fixture stays below both validation root and its unique leaf"),FPaths::CollapseRelativeDirectories(LongRoot) && !FPaths::IsRelative(LongRoot) && LongRoot.StartsWith(Boundary+TEXT("/"),ESearchCase::IgnoreCase) && LongRoot.StartsWith(Initial+TEXT("/"),ESearchCase::IgnoreCase)))return false;
    C->StorageRoot=LongRoot;
    const FString Path=C->GetManualSlotPath(1),TempExample=Path+TEXT(".")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".tmp");
    if(!TestTrue(TEXT("Real primary and actual-format temporary paths both exceed legacy260characters"),Path.Len()>260 && TempExample.Len()>260))return false;
    AddInfo(FString::Printf(TEXT("S362_WINDOWS_LONG_SAVE_PATH primary_chars=%d temporary_chars=%d root=%s"),Path.Len(),TempExample.Len(),*LongRoot));
    const FVector2D FirstPosition(410,300),SecondPosition(440,310);
    Run->AddGrains(37);Run->SetStoryFlag(TEXT("s362_windows_long_first"),true);
    const auto FirstRun=Run->GetRunSnapshot();const auto FirstMemory=Run->GetPlayerMemory()->GetSnapshot();const FString FirstWorld=Run->GetWorldCognition()->ExportJson();
    if(!TestTrue(TEXT("Initial manual save commits through the real long-path disk adapter"),C->SaveManualSlot(1,TEXT("belt_waystation"),FirstPosition)))return false;
    const FString FirstBytes=Read(Path);const auto FirstInfo=C->GetManualSlotSummary(1);
    TestTrue(TEXT("Committed long primary is nonempty and fully validates"),!FirstBytes.IsEmpty() && FirstInfo.State==EMemoriaSaveSlotState::Ready && FirstInfo.Map==TEXT("belt_waystation") && FirstInfo.Chapter==3);
    Run->AddGrains(13);Run->SetStoryFlag(TEXT("s362_windows_long_second"),true);
    TestEqual(TEXT("Long-path replacement fixture includes a real memory burn"),Run->BurnMemory(TEXT("sense_warm_light")),EMemoriaMemoryResult::Success);
    const auto SecondRun=Run->GetRunSnapshot();const auto SecondMemory=Run->GetPlayerMemory()->GetSnapshot();const FString SecondWorld=Run->GetWorldCognition()->ExportJson();
    const auto BeforeRefusal=StateBytes(*Run);
    TestFalse(TEXT("Occupied long path still requires explicit overwrite confirmation"),C->SaveManualSlot(1,TEXT("belt_waystation"),SecondPosition));
    TestTrue(TEXT("Refused long-path overwrite retains exact primary bytes and live state"),Read(Path)==FirstBytes && StateBytes(*Run)==BeforeRefusal && !IFileManager::Get().FileExists(*(Path+TEXT(".bak"))));
    if(!TestTrue(TEXT("Confirmed overwrite atomically commits at the same long path"),C->SaveManualSlot(1,TEXT("belt_waystation"),SecondPosition,true)))return false;
    const FString SecondBytes=Read(Path);
    TestTrue(TEXT("Long-path backup is the exact earlier valid save"),Read(Path+TEXT(".bak"))==FirstBytes && !SecondBytes.IsEmpty() && SecondBytes!=FirstBytes);
    TestTrue(TEXT("Overwritten long primary remains validated"),C->GetManualSlotSummary(1).State==EMemoriaSaveSlotState::Ready && C->GetManualSlotSummary(1).BurnedMemoryCount==1);
    TestEqual(TEXT("Different live run is created before long restore"),Run->BeginStartingMemoryRun(5),EMemoriaMemoryResult::Success);
    FString Map=TEXT("unchanged");FVector2D Position(13,17);
    if(!TestTrue(TEXT("Long primary restores through the public manual-slot adapter"),C->RestoreManualSlot(1,Map,Position)))return false;
    TestTrue(TEXT("Long primary restores saved identity, region, position, chapter and grains"),Map==TEXT("belt_waystation") && Position==SecondPosition && Run->GetRunSnapshot().RunId==SecondRun.RunId && Run->GetRunSnapshot().CurrentChapter==SecondRun.CurrentChapter && Run->GetRunSnapshot().Player.Grains==SecondRun.Player.Grains && Run->GetRunSnapshot().GetFlag(TEXT("s362_windows_long_first")) && Run->GetRunSnapshot().GetFlag(TEXT("s362_windows_long_second")));
    TestTrue(TEXT("Long primary restores actual memory and world domains"),Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==SecondMemory.BurnedHistory && Run->GetPlayerMemory()->GetSnapshot().Owned.Num()==SecondMemory.Owned.Num() && Run->GetWorldCognition()->ExportJson()==SecondWorld);
    TestTrue(TEXT("Only the isolated long primary is corrupted for recovery"),FFileHelper::SaveStringToFile(TEXT("{broken"),*Path));
    TestTrue(TEXT("Long-path backup is decoded as a valid recovery source"),C->GetManualSlotSummary(1).State==EMemoriaSaveSlotState::Backup);
    Run->AddGrains(99);Run->SetStoryFlag(TEXT("s362_windows_long_first"),false);
    if(!TestTrue(TEXT("Long backup restores and repairs its primary through the real commit path"),C->RestoreManualSlot(1,Map,Position)))return false;
    TestTrue(TEXT("Long recovery restores earlier semantic state"),Map==TEXT("belt_waystation") && Position==FirstPosition && Run->GetRunSnapshot().RunId==FirstRun.RunId && Run->GetRunSnapshot().CurrentChapter==FirstRun.CurrentChapter && Run->GetRunSnapshot().Player.Grains==FirstRun.Player.Grains && Run->GetRunSnapshot().GetFlag(TEXT("s362_windows_long_first")) && !Run->GetRunSnapshot().GetFlag(TEXT("s362_windows_long_second")) && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==FirstMemory.BurnedHistory && Run->GetPlayerMemory()->GetSnapshot().Owned.Num()==FirstMemory.Owned.Num() && Run->GetWorldCognition()->ExportJson()==FirstWorld);
    TestTrue(TEXT("Long recovery retains exact files and valid repaired summary"),Read(Path)==FirstBytes && Read(Path+TEXT(".bak"))==FirstBytes && C->GetManualSlotSummary(1).State==EMemoriaSaveSlotState::Ready && C->GetStatusText().Contains(TEXT("recovered")));
    // Keep this unique validation leaf and its primary/backup as real I/O evidence.
    return !HasAnyErrors();
#else
    AddInfo(TEXT("Windows extended-length atomic rename regression is not applicable on this platform."));return true;
#endif
}
#endif
