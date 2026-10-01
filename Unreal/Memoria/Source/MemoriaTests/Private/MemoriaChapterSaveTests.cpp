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
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Chapter/MemoriaChapterPresentation.h"
#include "Chapter/MemoriaChapterCardWidget.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Presentation/MemoriaArchiveWidget.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
int64 TotalErosion(const UMemoriaRunSubsystem& Run)
{
    int64 Total = 0; for (const auto& S : Run.GetPlayerMemory()->GetSnapshot().Owned) Total += S.Erosion; return Total;
}
bool Holds(const UMemoriaRunSubsystem& Run, const TCHAR* Id)
{
    return Run.GetPlayerMemory()->GetSnapshot().Owned.ContainsByPredicate([&](const auto& S) { return S.Id == Id; });
}
}
// S330: memory_manager.gd add_chapter_memories. The table comes from the source; the grant runs the chapter's
// bookkeeping once and adds only the memories not held yet.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapterMemoriesTest, "Memoria.Chapter.Memories", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapterMemoriesTest::RunTest(const FString&)
{
    // The source table: Chapters 3 to 5 bring two memories each, with their Korean text.
    struct FRow { int64 Chapter; const TCHAR* Id; const TCHAR* Title; EMemoriaMemoryGrade Grade; int64 Power; const TCHAR* Npc; bool bEffect; };
    const FRow Rows[] = {
        {3, TEXT("sense_dead_soil"), TEXT("The Taste of Dead Earth"), EMemoriaMemoryGrade::Grade5, 10, TEXT(""), false},
        {3, TEXT("rel_tobias_records"), TEXT("The Man Who Writes Everything Down"), EMemoriaMemoryGrade::Grade4, 25, TEXT("Tobias"), true},
        {4, TEXT("sense_ash_rain"), TEXT("Rain That Isn't Rain"), EMemoriaMemoryGrade::Grade5, 12, TEXT(""), false},
        {4, TEXT("daily_elia_hands"), TEXT("Warm Hands on Cold Palms"), EMemoriaMemoryGrade::Grade3, 50, TEXT("Elia"), true},
        {5, TEXT("sense_salt_wind"), TEXT("Salt Wind on the Cliffs"), EMemoriaMemoryGrade::Grade5, 12, TEXT(""), false},
        {5, TEXT("daily_elia_walking"), TEXT("Walking With Someone"), EMemoriaMemoryGrade::Grade4, 30, TEXT("Elia"), true}};
    for (const int64 Chapter : {3, 4, 5}) TestEqual(TEXT("Two memories per chapter"), MemoriaChapterMemories::For(Chapter).Num(), 2);
    TestEqual(TEXT("Chapters 1 and 2 bring none"), MemoriaChapterMemories::For(1).Num() + MemoriaChapterMemories::For(2).Num(), 0);
    for (const auto& Row : Rows)
    {
        const auto* D = MemoriaChapterMemories::For(Row.Chapter).FindByPredicate([&](const auto& V) { return V.Id == Row.Id; });
        if (!TestNotNull(*FString::Printf(TEXT("Chapter %lld brings %s"), Row.Chapter, Row.Id), D)) continue;
        TestEqual(TEXT("Title"), D->Title, FString(Row.Title));
        TestTrue(TEXT("Grade and power"), D->RawGrade == Row.Grade && D->BurnPower == Row.Power);
        TestEqual(TEXT("Related NPC"), D->RelatedNpc, FString(Row.Npc));
        TestEqual(TEXT("Story effect"), !D->StoryEffect.IsEmpty(), Row.bEffect);
        FMemoriaMemoryLocalizedText Ko;
        TestTrue(TEXT("Korean text"), MemoriaChapterMemories::Korean(Row.Id, Ko) && !Ko.Title.IsEmpty() && !Ko.Description.IsEmpty() && Ko.bHasStoryEffect == Row.bEffect && Ko.Title != D->Title);
    }
    FMemoriaMemoryLocalizedText None;
    TestFalse(TEXT("Other memories have no chapter text"), MemoriaChapterMemories::Korean(TEXT("core_name_origin"), None));

    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 3"), Run->BeginStartingMemoryRun(3) == EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    const int32 Start = Run->GetPlayerMemory()->GetSnapshot().Owned.Num();
    int32 Added = 0; Run->GetPlayerMemory()->OnObserved.AddLambda([&](const FMemoriaMemoryEvent& E) { if (E.Kind == EMemoriaMemoryEventKind::Added) ++Added; });
    TestEqual(TEXT("No erosion before the chapter is counted"), TotalErosion(*Run), int64(0));
    TestTrue(TEXT("The Belt's memories are granted"), Run->AddChapterMemories(3) == EMemoriaMemoryResult::Success);
    TestEqual(TEXT("Two memories join the archive"), Run->GetPlayerMemory()->GetSnapshot().Owned.Num(), Start + 2);
    TestEqual(TEXT("Each raises memory_added"), Added, 2);
    TestTrue(TEXT("They are the Belt's"), Holds(*Run, TEXT("sense_dead_soil")) && Holds(*Run, TEXT("rel_tobias_records")));
    TestTrue(TEXT("The chapter is counted once"), Run->GetPlayerMemory()->GetSnapshot().VigilChapters.Contains(3));
    const int64 Eroded = TotalErosion(*Run);
    TestTrue(TEXT("Chapter 3 erodes what Arrel already held"), Eroded > 0);
    for (const auto& S : Run->GetPlayerMemory()->GetSnapshot().Owned)
        if (S.Id == TEXT("sense_dead_soil") || S.Id == TEXT("rel_tobias_records")) TestEqual(TEXT("The new memories arrive whole"), S.Erosion, int64(0));
    // Entering the map again: no second grant and no second erosion.
    TestTrue(TEXT("A second call succeeds"), Run->AddChapterMemories(3) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Nothing is granted or eroded twice"), Run->GetPlayerMemory()->GetSnapshot().Owned.Num() == Start + 2 && Added == 2 && TotalErosion(*Run) == Eroded);
    // A burned chapter memory is still held (_has_memory), so it does not come back.
    TestTrue(TEXT("The dead earth burns"), Run->BurnMemory(TEXT("sense_dead_soil")) == EMemoriaMemoryResult::Success);
    Run->AddChapterMemories(3);
    TestEqual(TEXT("A burned memory is not granted again"), Run->GetPlayerMemory()->GetSnapshot().Owned.Num(), Start + 2);
    // Chapter 4: the drift's memories, and the chapter's erosion on everything held, the Belt's included.
    Run->SetCurrentChapter(4);
    TestTrue(TEXT("The drift's memories are granted"), Run->AddChapterMemories(4) == EMemoriaMemoryResult::Success);
    TestTrue(TEXT("Rain and warm hands"), Holds(*Run, TEXT("sense_ash_rain")) && Holds(*Run, TEXT("daily_elia_hands")) && Run->GetPlayerMemory()->GetSnapshot().Owned.Num() == Start + 4);
    TestTrue(TEXT("Chapter 4 erodes further"), TotalErosion(*Run) > Eroded);
    // The archive shows them in the run's language.
    Run->SetLocale(TEXT("ko"));
    FMemoriaMemoryLocalizedText Ko; MemoriaChapterMemories::Korean(TEXT("daily_elia_hands"), Ko);
    const auto ViewKo = MemoriaArchive::Build(*Run);
    const auto* RowKo = ViewKo.Rows.FindByPredicate([](const auto& R) { return R.Id == TEXT("daily_elia_hands"); });
    TestTrue(TEXT("Korean archive row"), RowKo && RowKo->Title == Ko.Title && RowKo->Description == Ko.Description && RowKo->StoryEffect == Ko.StoryEffect);
    Run->SetLocale(TEXT("en"));
    const auto ViewEn = MemoriaArchive::Build(*Run);
    const auto* RowEn = ViewEn.Rows.FindByPredicate([](const auto& R) { return R.Id == TEXT("daily_elia_hands"); });
    TestTrue(TEXT("English archive row"), RowEn && RowEn->Title == TEXT("Warm Hands on Cold Palms"));
    Game->Shutdown();
    return true;
}

// S330: the map slot. A save in a ported chapter map round-trips through disk into a fresh game instance; saves
// outside a map's bounds, chapter or scene are refused; Continue takes the newest valid slot.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapterMapSaveTest, "Memoria.Checkpoint.ChapterMap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapterMapSaveTest::RunTest(const FString&)
{
    const FString Leaf = TEXT("map-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TStrongObjectPtr<UGameInstance> A(NewObject<UGameInstance>()); A->Init();
    auto* Run = A->GetSubsystem<UMemoriaRunSubsystem>(); auto* Checkpoint = A->GetSubsystem<UMemoriaCheckpointSubsystem>();
    if (!TestTrue(TEXT("A run at Chapter 3"), Run->BeginStartingMemoryRun(3) == EMemoriaMemoryResult::Success)) { A->Shutdown(); return false; }
    Run->SetStoryFlag(TEXT("ch2_complete"), true); Run->SetStoryFlag(TEXT("ch3_arrived"), true); Run->AddGrains(41);
    Run->AddChapterMemories(3);
    TestFalse(TEXT("Disabled storage cannot save"), Checkpoint->CanSaveChapterMap(TEXT("belt_waystation"), FVector2D(400, 300)));
    if (!TestTrue(TEXT("Isolated storage"), Checkpoint->ConfigureTestStorage(Leaf))) { A->Shutdown(); return false; }
    TestTrue(TEXT("Empty storage offers no Continue"), Checkpoint->FindContinue() == EMemoriaContinueSource::None && Checkpoint->PeekChapterMap().IsEmpty());
    // What a map save accepts.
    TestTrue(TEXT("The Belt Waystation saves"), Checkpoint->CanSaveChapterMap(TEXT("belt_waystation"), FVector2D(400, 300)));
    TestFalse(TEXT("An unported map does not"), Checkpoint->CanSaveChapterMap(TEXT("the_seam"), FVector2D(400, 300)));
    TestFalse(TEXT("Verdan is not a chapter map"), Checkpoint->CanSaveChapterMap(TEXT("verdan_market"), FVector2D(400, 300)));
    TestFalse(TEXT("A place outside the map does not"), Checkpoint->CanSaveChapterMap(TEXT("belt_waystation"), FVector2D(-1, 300)) || Checkpoint->CanSaveChapterMap(TEXT("belt_waystation"), FVector2D(400, 9000)));
    TestFalse(TEXT("Drift Shelter is ahead of a Chapter 3 run"), Checkpoint->CanSaveChapterMap(TEXT("drift_shelter"), FVector2D(320, 512)));
    TestFalse(TEXT("A refused save writes nothing"), Checkpoint->SaveChapterMap(TEXT("drift_shelter"), FVector2D(320, 512)) || IFileManager::Get().FileExists(*Checkpoint->GetMapSlotPath()));
    // The round trip.
    Run->GetWorldCognition()->SeedMaletRoute(true);
    TestTrue(TEXT("The save is written"), Checkpoint->SaveChapterMap(TEXT("belt_waystation"), FVector2D(412, 297)));
    TestTrue(TEXT("Continue offers the map"), Checkpoint->FindContinue() == EMemoriaContinueSource::Map && Checkpoint->PeekChapterMap() == TEXT("belt_waystation"));
    const auto SavedRun = Run->GetRunSnapshot(); const auto SavedMemory = Run->GetPlayerMemory()->GetSnapshot();
    const FString SavedWorld = Run->GetWorldCognition()->ExportJson();
    A->Shutdown();
    TStrongObjectPtr<UGameInstance> B(NewObject<UGameInstance>()); B->Init();
    Run = B->GetSubsystem<UMemoriaRunSubsystem>(); Checkpoint = B->GetSubsystem<UMemoriaCheckpointSubsystem>();
    TestTrue(TEXT("Isolated storage again"), Checkpoint->ConfigureTestStorage(Leaf));
    FString Map; FVector2D Place;
    TestTrue(TEXT("A fresh game instance restores it"), Checkpoint->RestoreChapterMap(Map, Place));
    TestTrue(TEXT("The map and Arrel's place"), Map == TEXT("belt_waystation") && Place.Equals(FVector2D(412, 297)));
    const auto Restored = Run->GetRunSnapshot(); const auto RestoredMemory = Run->GetPlayerMemory()->GetSnapshot();
    TestTrue(TEXT("The run"), Restored.RunId == SavedRun.RunId && Restored.CurrentChapter == 3 && Restored.Player.Grains == SavedRun.Player.Grains && Restored.GetFlag(TEXT("ch3_arrived")));
    TestTrue(TEXT("The chapter's memories and its erosion"), RestoredMemory.Owned.Num() == SavedMemory.Owned.Num() && Holds(*Run, TEXT("rel_tobias_records")) && TotalErosion(*Run) > 0 && RestoredMemory.VigilChapters.Contains(3));
    TestTrue(TEXT("Their definitions"), Run->GetPlayerMemory()->GetDefinitions().ContainsByPredicate([](const auto& D) { return D.Id == TEXT("sense_dead_soil") && D.BurnPower == 10; }));
    TestEqual(TEXT("World cognition"), Run->GetWorldCognition()->ExportJson(), SavedWorld);
    // Continue takes the newest valid slot: the Verdan boundary written later wins, then a later map save.
    FPlatformProcess::Sleep(.02f);
    TestTrue(TEXT("The Verdan boundary saves"), Checkpoint->SaveClosedBoundary(FVector2D(500, 340)));
    TestTrue(TEXT("The newer boundary is offered"), Checkpoint->FindContinue() == EMemoriaContinueSource::Boundary);
    FPlatformProcess::Sleep(.02f);
    Run->SetCurrentChapter(4); Run->SetStoryFlag(TEXT("ch3_complete"), true);
    TestTrue(TEXT("Drift Shelter saves once the run is in Chapter 4"), Checkpoint->SaveChapterMap(TEXT("drift_shelter"), FVector2D(320, 512)));
    TestTrue(TEXT("The newer map save is offered"), Checkpoint->FindContinue() == EMemoriaContinueSource::Map && Checkpoint->PeekChapterMap() == TEXT("drift_shelter"));
    // A damaged slot is never offered, and a refused restore leaves the run alone.
    const FString Slot = Checkpoint->GetMapSlotPath(); FString Good; FFileHelper::LoadFileToString(Good, *Slot);
    FFileHelper::SaveStringToFile(TEXT("{\"version\":1}"), *Slot);
    TestTrue(TEXT("A damaged map slot is not offered"), Checkpoint->FindContinue() == EMemoriaContinueSource::Boundary && Checkpoint->PeekChapterMap().IsEmpty());
    const FGuid Live = Run->GetRunSnapshot().RunId; Run->AddGrains(7); const int64 Grains = Run->GetRunSnapshot().Player.Grains;
    TestFalse(TEXT("A damaged slot does not restore"), Checkpoint->RestoreChapterMap(Map, Place));
    TestTrue(TEXT("The live run is unchanged"), Run->GetRunSnapshot().RunId == Live && Run->GetRunSnapshot().Player.Grains == Grains);
    // A boundary save is not a map save, whatever file it sits in.
    FString Boundary; FFileHelper::LoadFileToString(Boundary, *Checkpoint->GetSlotPath());
    FFileHelper::SaveStringToFile(Boundary, *Slot);
    TestTrue(TEXT("A boundary save in the map slot is refused"), Checkpoint->PeekChapterMap().IsEmpty());
    FFileHelper::SaveStringToFile(Good, *Slot);
    TestTrue(TEXT("The good slot is offered again"), Checkpoint->PeekChapterMap() == TEXT("drift_shelter"));
    B->Shutdown();
    return true;
}

namespace
{
// S330: Chapter 3 played through its arrival, then saved and continued. The arrival grants the Belt's memories
// and raises their toasts when its chain of dialogues ends; the departure autosaves in the map; the pause menu's Load brings
// the run back into the closed Belt Waystation at the saved place, where the gated chests are open and stepping
// into the exit again takes the road on to Drift Shelter, whose arrival grants Chapter 4's memories.
class FChapterContinueReplay final : public IAutomationLatentCommand
{
public:
    explicit FChapterContinueReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FChapterContinueReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 260) { Test->AddError(FString::Printf(TEXT("Chapter continue timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .2) return false;
        if (!bFixed) { bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime(); FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0); }
        ++Frame;
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        auto* Checkpoint = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        AMemoriaChapterPresentation* Map = nullptr; for (TActorIterator<AMemoriaChapterPresentation> It(World); It; ++It) Map = *It;
        const auto* Spec = Map ? Map->GetSpec() : nullptr;
        if (!Spec) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/ChapterContinue") / (FString(Name) + TEXT(".png")), true, false); };
        auto Flag = [&](const TCHAR* Id) { return Run->GetRunSnapshot().GetFlag(Id); };
        auto Place = [&](const FVector2D& Source) { Pawn->SetActorLocation(MemoriaChapterMaps::ToWorld(Source) + FVector(0, 0, Pawn->GetActorLocation().Z)); };
        auto Traced = [&](const TCHAR* Prefix) { return Host->GetTrace().ContainsByPredicate([&](const FString& E) { return E.StartsWith(Prefix); }); };
        if (Host->GetState() == EMemoriaSliceState::Field) { if (Frame % 4 == 0) Host->Confirm(); return false; }
        switch (Step)
        {
        case 0:
            Test->TestTrue(TEXT("Saves are isolated"), Checkpoint->ConfigureTestStorage(TEXT("continue-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            Run->SetLocale(TEXT("ko"));
            Owned = Run->GetPlayerMemory()->GetSnapshot().Owned.Num();
            Test->TestFalse(TEXT("The Belt's memories are not held on the road in"), Holds(*Run, TEXT("sense_dead_soil")));
            ++Step; Mark = Frame; break;
        case 1:
            // The arrival grants the chapter's memories; the toasts follow its dialogue.
            if (Flag(TEXT("ch3_arrived")) && !bGrantChecked)
            {
                bGrantChecked = true;
                Test->TestTrue(TEXT("The arrival grants the Belt's memories"), Holds(*Run, TEXT("sense_dead_soil")) && Holds(*Run, TEXT("rel_tobias_records")) && Run->GetPlayerMemory()->GetSnapshot().Owned.Num() == Owned + 2);
                Test->TestTrue(TEXT("The grant is recorded"), Host->GetTrace().Contains(TEXT("chapter:memories:3:2")));
            }
            if (Flag(TEXT("ch3_class_seven_message")) && Host->GetState() == EMemoriaSliceState::Exploration && Frame > Mark + 60)
            {
                Test->TestTrue(TEXT("The memory toasts are raised in Korean"), Host->GetTrace().Contains(TEXT("toast:기억 획득: 죽은 흙의 맛")) && Host->GetTrace().Contains(TEXT("toast:기억 획득: 전부 받아 적는 남자")));
                Test->TestTrue(TEXT("Chapter 3's erosion is counted once"), Run->GetPlayerMemory()->GetSnapshot().VigilChapters.Contains(3));
                Test->TestFalse(TEXT("Nothing is autosaved yet"), Traced(TEXT("autosave:map_saved")));
                // The pause menu saves here, in the map slot.
                Place(FVector2D(400, 300));
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 3200) { Test->AddError(TEXT("The arrival chain did not finish")); return true; }
            break;
        case 2:
            if (Frame == Mark + 20) Capture(TEXT("ContinueToasts"));
            if (Frame < Mark + 30) break;
            Test->TestTrue(TEXT("The pause menu opens"), PC->OpenPause() && PC->GetPauseWidget());
            if (auto* Menu = PC->GetPauseWidget())
            {
                Test->TestTrue(TEXT("Save is lit in a chapter map"), Menu->IsItemEnabled(2));
                Menu->Select(2); Menu->Confirm();
                Test->TestTrue(TEXT("It saves the map slot"), Checkpoint->FindContinue() == EMemoriaContinueSource::Map && Checkpoint->PeekChapterMap() == TEXT("belt_waystation"));
                Test->TestTrue(TEXT("Load is lit after the save"), Menu->IsItemEnabled(3));
            }
            PC->ClosePause();
            // On to the east exit: the departure closes the chapter and autosaves.
            Place(Spec->Exit.Rect.Center());
            ++Step; Mark = Frame; break;
        case 3:
            if (Map->IsChapterComplete())
            {
                Test->TestTrue(TEXT("The departure autosaves in the Belt Waystation"), Host->GetTrace().Contains(TEXT("autosave:map_saved:belt_waystation")) && Host->GetTrace().Contains(TEXT("toast:자동 저장 완료")));
                Test->TestEqual(TEXT("The run is in Chapter 4"), Run->GetRunSnapshot().CurrentChapter, int64(4));
                RunId = Run->GetRunSnapshot().RunId; Grains = Run->GetRunSnapshot().Player.Grains;
                // Away from the save: the live run changes, then the pause menu's Load brings the save back.
                Run->AddGrains(500);
                Test->TestTrue(TEXT("The pause menu opens"), PC->OpenPause() && PC->GetPauseWidget());
                if (auto* Menu = PC->GetPauseWidget()) { Menu->Select(3); Menu->Confirm(); }
                Before = World;
                ++Step; Mark = Frame;
            }
            if (Frame > Mark + 1200) { Test->AddError(TEXT("The exit never closed the chapter")); return true; }
            break;
        case 4:
            // The load travels into the Belt Waystation again and restores the run there.
            if (World == Before.Get() || Map->GetMap() != TEXT("belt_waystation") || Host->GetState() != EMemoriaSliceState::Exploration)
            {
                if (Frame > Mark + 900) { Test->AddError(TEXT("The load never reached the Belt Waystation")); return true; }
                break;
            }
            Test->TestTrue(TEXT("The save is resumed"), Host->GetTrace().Contains(TEXT("autosave:map_resumed:belt_waystation")));
            Test->TestTrue(TEXT("The saved run, not the changed one"), Run->GetRunSnapshot().RunId == RunId && Run->GetRunSnapshot().Player.Grains == Grains);
            Test->TestTrue(TEXT("Chapter 3 is closed in it"), Flag(TEXT("ch3_complete")) && Run->GetRunSnapshot().CurrentChapter == 4);
            Test->TestTrue(TEXT("Arrel stands where the save was made"), Spec->Exit.Rect.Contains(MemoriaChapterMaps::ToSource(Pawn->GetActorLocation())));
            Test->TestTrue(TEXT("The memories came back with it"), Holds(*Run, TEXT("rel_tobias_records")));
            Test->TestFalse(TEXT("Standing in the exit does not travel"), Map->IsChapterComplete());
            Test->TestTrue(TEXT("The road onward is announced"), Traced(TEXT("toast:3장 완료.")));
            ++Step; Mark = Frame; break;
        case 5:
            if (Frame == Mark + 30) Capture(TEXT("ContinueBelt"));
            if (Frame == Mark + 50) PC->ToggleArchive();
            if (Frame == Mark + 60)
                if (auto* Archive = PC->GetArchiveWidget())
                {
                    // The Belt's memories are the last rows.
                    Archive->Select(Archive->GetView().Rows.Num() - 1);
                    Test->TestTrue(TEXT("The archive shows the Belt's memory in Korean"), Archive->GetSelectedId() == TEXT("rel_tobias_records") && Archive->VisibleText().Contains(TEXT("전부 받아 적는 남자")));
                }
            if (Frame == Mark + 80)
            {
                Test->TestTrue(TEXT("The archive opens"), PC->GetArchiveWidget() != nullptr);
                Capture(TEXT("ContinueArchive"));
            }
            if (Frame == Mark + 100)
            {
                PC->CloseArchive();
                Test->TestFalse(TEXT("Still in the Belt after standing in the exit"), Host->GetState() == EMemoriaSliceState::Travelling);
                // The closed chapter's chest is open to a walk-over (objects_gate: ch3_complete).
                ChestGrains = Run->GetRunSnapshot().Player.Grains;
                Place(Spec->Chests[0].Origin + FVector2D(Spec->TileSize * .5, Spec->TileSize * .5));
            }
            if (Frame == Mark + 115)
            {
                Test->TestTrue(TEXT("The chest pays out"), Flag(*Spec->Chests[0].Flag) && Run->GetRunSnapshot().Player.Grains == ChestGrains + Spec->Chests[0].Grains);
                // Back into the exit: body_entered, the road on.
                Place(Spec->Exit.Rect.Center());
                Before = World; ++Step; Mark = Frame;
            }
            break;
        case 6:
            if (World == Before.Get() || Map->GetMap() != TEXT("drift_shelter"))
            {
                if (Frame > Mark + 900) { Test->AddError(TEXT("The exit did not take the road on")); return true; }
                break;
            }
            Test->TestTrue(TEXT("The road reaches Drift Shelter with the same run"), Run->GetRunSnapshot().RunId == RunId && Host->GetChapterMap() == TEXT("drift_shelter"));
            Test->TestTrue(TEXT("The chapter title shows"), Map->GetCard() && Map->GetCard()->IsShowing());
            ++Step; Mark = Frame; break;
        case 7:
            if (Flag(TEXT("ch4_arrived")))
            {
                Test->TestTrue(TEXT("The arrival grants the drift's memories"), Holds(*Run, TEXT("sense_ash_rain")) && Holds(*Run, TEXT("daily_elia_hands")) && Host->GetTrace().Contains(TEXT("chapter:memories:4:2")));
                Test->TestTrue(TEXT("Chapter 4 is counted"), Run->GetPlayerMemory()->GetSnapshot().VigilChapters.Contains(4));
                return true;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("Drift Shelter's arrival never started")); return true; }
            break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0, Mark = 0, Owned = 0;
    int64 Grains = 0, ChestGrains = 0;
    FGuid RunId;
    TWeakObjectPtr<UWorld> Before;
    bool bFixed = false, bOldFixed = false, bGrantChecked = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapterContinueTest, "MemoriaVisual.ChapterContinue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChapterContinueTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FChapterContinueReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
