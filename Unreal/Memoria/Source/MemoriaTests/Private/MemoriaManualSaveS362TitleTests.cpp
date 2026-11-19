#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Framework/MemoriaCoordinates.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "World/MemoriaWorldCognition.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Save/MemoriaSaveSlotsWidget.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Presentation/MemoriaPauseWidget.h"
#include "Presentation/MemoriaTitleWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString ReadTitleSave(const FString& Path) { FString Text; FFileHelper::LoadFileToString(Text, *Path); return Text; }

// A manual-only run must remain reachable after Pause->Title. Focused Title keys go through Slate;
// the browser keys go through the actual PC route while the Title widget is collapsed beneath it.
class FS362TitleManualOnlyReplay final : public IAutomationLatentCommand
{
public:
    explicit FS362TitleManualOnlyReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FS362TitleManualOnlyReplay() override
    {
        if (Settings.IsValid()) Settings->SetLocale(OldLocale);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 240)
        { Test->AddError(FString::Printf(TEXT("S362 manual-only Title replay timeout at step %d"), Step)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        if (!PC || World->GetTimeSeconds() < .2f) return false;
        auto* Pawn = Cast<AMemoriaFieldPawn>(PC->GetPawn());
        auto* Game = World->GetGameInstance();
        auto* Host = Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
        auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>();
        auto* C = Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>(); OldLocale = Settings->GetLocale();
            Settings->SetLocale(TEXT("en")); Run->SetLocale(TEXT("en"));
        }
        ++Frame;
        if (Step == 0)
        {
            if (Host->GetState() == EMemoriaSliceState::Field || Host->GetState() == EMemoriaSliceState::VN)
            { if (Frame % 4 == 0) Host->Confirm(); return false; }
            if (!Pawn || Host->GetChapterMap() != TEXT("belt_waystation") || Host->GetState() != EMemoriaSliceState::Exploration ||
                !Run->GetRunSnapshot().GetFlag(TEXT("ch3_class_seven_message"))) return false;
        }
        if (Frame % 8 != 0) return false;
        auto Tap = [&](const FKey& Key)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
        };
        auto SlateTap = [](const FKey& Key)
        {
            FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
            FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
        };
        auto Capture = [&](const TCHAR* Name)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/S362/SaveSlots") / (FString(Name) + TEXT(".png")), true, false);
        };
        auto AssertManualOnly = [&]()
        {
            Test->TestTrue(TEXT("Legacy autosave Continue selector remains empty"), C->FindContinue() == EMemoriaContinueSource::None);
            Test->TestTrue(TEXT("No autosave files are manufactured by Title browsing or manual loading"), ReadTitleSave(C->GetSlotPath()).IsEmpty() &&
                ReadTitleSave(C->GetChapterSlotPath()).IsEmpty() && ReadTitleSave(C->GetMapSlotPath()).IsEmpty());
            Test->TestEqual(TEXT("Manual-only slot bytes remain unchanged"), ReadTitleSave(C->GetManualSlotPath(2)), ManualBytes);
        };
        auto* Title = PC->GetTitleWidget();
        switch (Step)
        {
        case 0:
        {
            Test->TestTrue(TEXT("Manual-only fixture gets a fresh isolated storage leaf after arrival"), C->ConfigureTestStorage(TEXT("s362title-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
            auto* Fight = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
            if (!Test->TestNotNull(TEXT("Actual field combat exists"), Fight)) return true;
            Fight->Revive(.47f);
            Run->SetCurrentChapter(4); Run->AddGrains(29); Run->SetStoryFlag(TEXT("s362_title_manual"), true);
            const auto* Drift = MemoriaChapterMaps::Find(TEXT("drift_shelter"));
            if (!Test->TestNotNull(TEXT("Ported Drift map exists"), Drift)) return true;
            SavedPosition = Drift->Spawn;
            // Explicit already-arrived test state; manual loading, not chapter dialogue, is under test.
            for (const auto& Link : Drift->Sequence)
            {
                if (!Link.Flag.IsEmpty()) Run->SetStoryFlag(Link.Flag, true);
                for (const auto& Flag : Link.Flags) Run->SetStoryFlag(Flag, true);
            }
            SavedRun = Run->GetRunSnapshot().RunId; SavedHp = Run->GetRunSnapshot().Player.Hp; SavedGrains = Run->GetRunSnapshot().Player.Grains;
            Test->TestTrue(TEXT("Manual-only snapshot has exact nonmax HP"), SavedHp > 0 && SavedHp < Run->GetRunSnapshot().Player.MaxHp);
            Test->TestTrue(TEXT("Only manual2 is seeded through isolated save API"), C->SaveManualSlot(2, TEXT("drift_shelter"), SavedPosition));
            ManualBytes = ReadTitleSave(C->GetManualSlotPath(2));
            Test->TestTrue(TEXT("Other manual rows remain empty"), C->GetManualSlotSummary(1).State == EMemoriaSaveSlotState::Empty && C->GetManualSlotSummary(3).State == EMemoriaSaveSlotState::Empty);
            AssertManualOnly();
            Run->SetCurrentChapter(3); Run->AddGrains(1); Fight->Revive(.86f);
            Tap(EKeys::Escape);
            if (!Test->TestNotNull(TEXT("Actual Pause opens before Title travel"), PC->GetPauseWidget())) return true;
            for (int32 I = 0; I < 7; ++I) Tap(EKeys::Gamepad_DPad_Down);
            Test->TestEqual(TEXT("Physical Dpad selects Return to Title"), PC->GetPauseWidget()->GetSelected(), 7);
            BeforeWorld = World; Tap(EKeys::Gamepad_FaceButton_Bottom); ++Step; break;
        }
        case 1:
            if (World == BeforeWorld.Get() || !Host->IsOnTitle() || !Title || !Title->IsIntroComplete()) break;
            Test->TestTrue(TEXT("Manual-only Title enables Continue despite no autosave"), Title->IsItemEnabled(1) && C->HasManualSave() && C->FindContinue() == EMemoriaContinueSource::None);
            Test->TestTrue(TEXT("Pause and browser owners were cleaned by Title travel"), !PC->GetPauseWidget() && !PC->GetSaveSlotsWidget() && !World->IsPaused());
            AssertManualOnly(); ++Step; break;
        case 2:
            Capture(TEXT("TitleManualOnlyContinue")); ++Step; break;
        case 3:
            SlateTap(EKeys::Gamepad_DPad_Down);
            if (!Test->TestNotNull(TEXT("Title remains focused for actual Slate navigation"), Title)) return true;
            Test->TestEqual(TEXT("Focused Title Dpad reaches Continue"), Title->GetSelected(), 1);
            SlateTap(EKeys::Gamepad_FaceButton_Bottom);
            if (!Test->TestNotNull(TEXT("Title Continue opens actual Load browser"), PC->GetSaveSlotsWidget())) return true;
            Test->TestTrue(TEXT("Authored title stays visible but cannot receive input beneath Load"), PC->GetTitleWidget() && PC->GetTitleWidget()->GetVisibility() != ESlateVisibility::Collapsed && !PC->GetTitleWidget()->GetIsEnabled());
            Test->TestTrue(TEXT("Manual-only Load shows disabled auto and enabled manual2"), !PC->GetSaveSlotsWidget()->IsRowEnabled(0) &&
                !PC->GetSaveSlotsWidget()->IsRowEnabled(1) && PC->GetSaveSlotsWidget()->IsRowEnabled(2));
            ++Step; break;
        case 4:
            Capture(TEXT("TitleManualOnlyLoadBrowser")); ++Step; break;
        case 5:
            Tap(EKeys::Gamepad_FaceButton_Right);
            Test->TestTrue(TEXT("B cancels browser and restores visible Title"), !PC->GetSaveSlotsWidget() && PC->GetTitleWidget() && PC->GetTitleWidget()->GetVisibility() != ESlateVisibility::Collapsed && Host->IsOnTitle() && !World->IsPaused());
            Test->TestEqual(TEXT("Title retains Continue selection on browser cancel"), PC->GetTitleWidget() ? PC->GetTitleWidget()->GetSelected() : INDEX_NONE, 1);
            AssertManualOnly(); ++Step; break;
        case 6:
            Capture(TEXT("TitleManualOnlyCancelled")); ++Step; break;
        case 7:
            // No direct focus or Select call: Enter must work on the Title focus restored by B.
            SlateTap(EKeys::Enter);
            if (!Test->TestNotNull(TEXT("Restored Title focus lets Enter reopen Load"), PC->GetSaveSlotsWidget())) return true;
            Tap(EKeys::Gamepad_DPad_Down); Tap(EKeys::Gamepad_DPad_Down);
            Test->TestEqual(TEXT("Physical browser Dpad explicitly chooses manual2"), PC->GetSaveSlotsWidget()->GetSelectedSlot(), 2);
            BeforeWorld = World; Tap(EKeys::Gamepad_FaceButton_Bottom); ++Step; break;
        case 8:
            if (World == BeforeWorld.Get() || Host->GetChapterMap() != TEXT("drift_shelter") || Host->GetState() != EMemoriaSliceState::Exploration || !Pawn) break;
            Test->TestTrue(TEXT("Title manual-only load restores saved run/chapter/Grains in the actual Drift world"), Run->GetRunSnapshot().RunId == SavedRun &&
                Run->GetRunSnapshot().CurrentChapter == 4 && Run->GetRunSnapshot().Player.Grains == SavedGrains && Run->GetRunSnapshot().GetFlag(TEXT("s362_title_manual")));
            Test->TestEqual(TEXT("Title load preserves exact saved nonmax HP"), Run->GetRunSnapshot().Player.Hp, SavedHp);
            Test->TestTrue(TEXT("Title load applies saved Drift coordinates"), MemoriaChapterMaps::ToSource(Pawn->GetActorLocation()).Equals(SavedPosition, .5));
            Test->TestTrue(TEXT("No Title/browser/Pause survives into loaded world"), !PC->GetTitleWidget() && !PC->GetSaveSlotsWidget() && !PC->GetPauseWidget() && !World->IsPaused());
            AssertManualOnly(); ++Step; break;
        case 9:
            Capture(TEXT("TitleManualOnlyDriftLoaded")); ++Step; break;
        default:
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    int32 Step = 0, Frame = 0;
    int64 SavedHp = 0, SavedGrains = 0;
    bool bFixed = false, bOldFixed = false;
    FGuid SavedRun;
    FVector2D SavedPosition = FVector2D::ZeroVector;
    FString ManualBytes, OldLocale;
    TWeakObjectPtr<UWorld> BeforeWorld;
    TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362TitleManualOnly, "MemoriaSave.S362.TitleManualOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FS362TitleManualOnly::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362TitleManualOnlyReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

namespace
{
TArray<uint8> TitleVerdanMemoryBytes(UMemoriaRunSubsystem& Run)
{
    auto* Save=NewObject<UMemoriaRunSaveGame>();
    Save->MemoryDefinitions=Run.GetPlayerMemory()->GetDefinitions();Save->PlayerMemory=Run.GetPlayerMemory()->GetSnapshot();
    TArray<uint8> Bytes;UGameplayStatics::SaveGameToMemory(Save,Bytes);return Bytes;
}
// Both routes use real Pause->Title, focused Slate Continue, and destination
// ?Continue travel. Only manual1 uses the browser; boundary0 takes Title's
// direct autosave route. No Title flag/widget is directly changed by this test.
class FS362TitleVerdanReplay final : public IAutomationLatentCommand
{
public:
    explicit FS362TitleVerdanReplay(FAutomationTestBase* In):Test(In),Started(FPlatformTime::Seconds()){}
    ~FS362TitleVerdanReplay() override
    {if(Settings.IsValid())Settings->SetLocale(OldLocale);if(bFixed){FApp::SetUseFixedTimeStep(bOldFixed);FApp::SetFixedDeltaTime(OldDelta);}}
    bool Update() override
    {
        if(LastFrame==GFrameCounter)return false;LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>240.){Test->AddError(FString::Printf(TEXT("Title Verdan replay timeout route%d step%d"),Route,Step));return true;}
        UWorld* World=GEditor?GEditor->PlayWorld.Get():nullptr;
        auto* PC=World && !World->bIsTearingDown?Cast<AMemoriaSliceController>(World->GetFirstPlayerController()):nullptr;
        if(!PC || World->GetTimeSeconds()<.2f)return false;
        auto* Game=World->GetGameInstance();if(!Game)return false;
        auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* C=Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        auto* Pawn=Cast<AMemoriaFieldPawn>(PC->GetPawn());
        if(!Host || !Run || !C)return false;
        if(!bFixed)
        {
            bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
            Settings=Game->GetSubsystem<UMemoriaSettingsSubsystem>();OldLocale=Settings->GetLocale();Settings->SetLocale(TEXT("en"));Run->SetLocale(TEXT("en"));
        }
        ++Frame;
        if(Step==0)
        {
            if(Host->GetState()==EMemoriaSliceState::Field || Host->GetState()==EMemoriaSliceState::VN){if(Frame%4==0)Host->Confirm();return false;}
            if(!Pawn || Host->GetState()!=EMemoriaSliceState::Exploration || PC->GetNarrativeWidget())return false;
            if(Route==0 && (Host->GetChapterMap()!=TEXT("belt_waystation") || !Run->GetRunSnapshot().GetFlag(TEXT("ch3_class_seven_message"))))return false;
            if(Route==1 && (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || !Host->IsVerdanRevisit()))return false;
        }
        if(Frame%8!=0)return false;
        auto Tap=[&](const FKey& Key){PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0.f));};
        auto SlateTap=[](const FKey& Key){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));};
        auto Capture=[&](const TCHAR* Suffix){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/S362/SaveSlots")/(FString(Route==0?TEXT("TitleVerdanManual"):TEXT("TitleVerdanAutosave"))+Suffix+TEXT(".png")),true,false);};
        const bool bManual=Route==0;
        switch(Step)
        {
        case 0:
        {
            if(!Test->TestTrue(TEXT("Verdan Title route uses a fresh isolated storage leaf"),C->ConfigureTestStorage(TEXT("s362titleverdan-")+FGuid::NewGuid().ToString(EGuidFormats::Digits))))return true;
            Run->SetCurrentChapter(3);
            for(const TCHAR* Flag:{TEXT("ch2_complete"),TEXT("ch2_arrived"),TEXT("ch2_arrival_vn_seen")})Run->SetStoryFlag(Flag,true);
            SavedFlag=bManual?TEXT("s362_title_verdan_manual"):TEXT("s362_title_verdan_auto");Run->SetStoryFlag(SavedFlag,true);Run->AddGrains(31+Route*11);
            if(!Test->TestEqual(TEXT("Saved Verdan fixture includes an actual memory burn"),Run->BurnMemory(bManual?TEXT("sense_warm_light"):TEXT("daily_campfire_song")),EMemoriaMemoryResult::Success))return true;
            auto* Fight=World->GetSubsystem<UMemoriaFieldCombatSubsystem>();if(!Test->TestNotNull(TEXT("Actual combat owner sets saved nonmax HP"),Fight))return true;
            Fight->Revive(bManual?.43f:.58f);SavedHp=Run->GetRunSnapshot().Player.Hp;SavedGrains=Run->GetRunSnapshot().Player.Grains;SavedRun=Run->GetRunSnapshot().RunId;
            SavedPosition=bManual?FVector2D(504,344):FVector2D(520,352);SavedMemory=TitleVerdanMemoryBytes(*Run);SavedBurns=Run->GetPlayerMemory()->GetSnapshot().BurnedHistory;SavedWorld=Run->GetWorldCognition()->ExportJson();
            if(!Test->TestTrue(TEXT("Saved HP is positive, nonmax and memory snapshot is nonempty"),SavedHp>0 && SavedHp<Run->GetRunSnapshot().Player.MaxHp && !SavedMemory.IsEmpty() && !SavedBurns.IsEmpty()))return true;
            const bool bSaved=bManual?C->SaveManualSlot(1,TEXT("verdan_market"),SavedPosition):C->SaveClosedBoundary(SavedPosition);
            if(!Test->TestTrue(TEXT("Verdan fixture commits through the selected actual disk adapter"),bSaved))return true;
            SavedBytes=ReadTitleSave(bManual?C->GetManualSlotPath(1):C->GetSlotPath());
            if(!Test->TestTrue(TEXT("Manual-only or boundary-autosave-only route is explicit"),!SavedBytes.IsEmpty() && C->HasManualSave()==bManual && C->FindContinue()==(bManual?EMemoriaContinueSource::None:EMemoriaContinueSource::Boundary)))return true;
            Tap(EKeys::Escape);if(!Test->TestNotNull(TEXT("Real Pause opens before return to Title"),PC->GetPauseWidget()))return true;
            // A different live run makes saved identity/memory restoration observable;
            // the actual Pause owns the screen throughout this isolated fixture change.
            if(!Test->TestEqual(TEXT("Abandoned live run differs before Title Continue"),Run->BeginStartingMemoryRun(4),EMemoriaMemoryResult::Success))return true;
            Fight->Revive(.91f);Test->TestTrue(TEXT("Live identity differs from saved run"),Run->GetRunSnapshot().RunId!=SavedRun);
            for(int32 I=0;I<7;++I)Tap(EKeys::Gamepad_DPad_Down);
            if(!Test->TestEqual(TEXT("Physical Dpad selects Return to Title"),PC->GetPauseWidget()->GetSelected(),7))return true;
            BeforeWorld=World;Tap(EKeys::Gamepad_FaceButton_Bottom);Step=1;break;
        }
        case 1:
            if(World==BeforeWorld.Get() || !Host->IsOnTitle() || !PC->GetTitleWidget() || !PC->GetTitleWidget()->IsIntroComplete())break;
            Test->TestTrue(TEXT("Actual Title enables the selected saved route"),PC->GetTitleWidget()->IsItemEnabled(1) && !World->IsPaused() && !PC->GetPauseWidget());
            Capture(TEXT("BeforeContinue"));Step=2;break;
        case 2:
            SlateTap(EKeys::Gamepad_DPad_Down);
            if(!Test->TestNotNull(TEXT("Title owns focused Slate navigation"),PC->GetTitleWidget()) || !Test->TestEqual(TEXT("Slate Dpad explicitly selects Continue"),PC->GetTitleWidget()->GetSelected(),1))return true;
            BeforeWorld=World;SlateTap(EKeys::Gamepad_FaceButton_Bottom);
            if(bManual)
            {
                if(!Test->TestNotNull(TEXT("Manual Continue opens the real load browser"),PC->GetSaveSlotsWidget()))return true;
                Test->TestTrue(TEXT("Browser exposes manual1 and no manufactured autosave"),!PC->GetSaveSlotsWidget()->IsRowEnabled(0) && PC->GetSaveSlotsWidget()->IsRowEnabled(1));Step=3;
            }
            else{Test->TestTrue(TEXT("Autosave-only Continue takes the direct travel route"),!PC->GetSaveSlotsWidget() && !C->HasManualSave());Step=4;}
            break;
        case 3:
            if(!Test->TestNotNull(TEXT("Manual browser still owns selection"),PC->GetSaveSlotsWidget()))return true;
            Tap(EKeys::Gamepad_DPad_Down);
            if(!Test->TestEqual(TEXT("Physical browser Dpad explicitly selects manual1"),PC->GetSaveSlotsWidget()->GetSelectedSlot(),1))return true;
            BeforeWorld=World;Tap(EKeys::Gamepad_FaceButton_Bottom);Step=4;break;
        case 4:
        {
            if(World==BeforeWorld.Get() || !World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || !Pawn || Host->GetState()!=EMemoriaSliceState::Deferred || Host->GetDeferredInteraction()!=TEXT("before:chapter_transition_delay"))break;
            auto* Mode=World->GetAuthGameMode();
            Test->TestTrue(TEXT("Destination was entered by the real Continue travel option"),Mode && UGameplayStatics::HasOption(Mode->OptionsString,TEXT("Continue")) && (bManual?UGameplayStatics::ParseOption(Mode->OptionsString,TEXT("ManualSlot"))==TEXT("1"):!UGameplayStatics::HasOption(Mode->OptionsString,TEXT("ManualSlot"))));
            Test->TestFalse(TEXT("Accepted Verdan restore relinquishes Title ownership"),Host->IsOnTitle());
            Test->TestTrue(TEXT("No Title/browser/Pause survives accepted checkpoint ownership"),!PC->GetTitleWidget() && !PC->GetSaveSlotsWidget() && !PC->GetPauseWidget() && !World->IsPaused());
            Test->TestTrue(TEXT("Checkpoint restores exact saved run identity, chapter, Grains and flag"),Run->GetRunSnapshot().RunId==SavedRun && Run->GetRunSnapshot().CurrentChapter==3 && Run->GetRunSnapshot().Player.Grains==SavedGrains && Run->GetRunSnapshot().GetFlag(SavedFlag));
            Test->TestEqual(TEXT("Title Verdan restore preserves exact saved nonmax HP"),Run->GetRunSnapshot().Player.Hp,SavedHp);
            Test->TestTrue(TEXT("Title Verdan restore preserves the complete serialized memory state"),TitleVerdanMemoryBytes(*Run)==SavedMemory && Run->GetPlayerMemory()->GetSnapshot().BurnedHistory==SavedBurns);
            Test->TestEqual(TEXT("Title Verdan restore preserves world cognition"),Run->GetWorldCognition()->ExportJson(),SavedWorld);
            Test->TestTrue(TEXT("Title Verdan restore applies the saved source coordinates"),Memoria::Coordinates::ToSource(Pawn->GetActorLocation()).Equals(SavedPosition,.01));
            Test->TestEqual(TEXT("Loading preserves exact selected save bytes"),ReadTitleSave(bManual?C->GetManualSlotPath(1):C->GetSlotPath()),SavedBytes);
            Test->TestTrue(TEXT("Load does not manufacture another save route"),C->HasManualSave()==bManual && C->FindContinue()==(bManual?EMemoriaContinueSource::None:EMemoriaContinueSource::Boundary));
            if(Test->HasAnyErrors())return true;StableChecks=0;Step=5;break;
        }
        case 5:
            Test->TestTrue(TEXT("Later controller ticks do not recreate Title after successful Verdan Continue"),!Host->IsOnTitle() && !PC->GetTitleWidget() && Host->GetState()==EMemoriaSliceState::Deferred && Run->GetRunSnapshot().Player.Hp==SavedHp);
            if(StableChecks==0)Capture(TEXT("Loaded"));
            if(++StableChecks<3)break;
            if(Test->HasAnyErrors() || !bManual)return true;Step=6;break;
        case 6:
        {
            auto* View=PC->GetNarrativeWidget();if(!Test->TestNotNull(TEXT("Accepted checkpoint exposes its real boundary view"),View))return true;
            for(int32 I=0;I<4 && View->SelectedOriginalIndex()!=3;++I)Tap(EKeys::Gamepad_DPad_Down);
            if(!Test->TestEqual(TEXT("Physical boundary navigation selects Return to Verdan"),View->SelectedOriginalIndex(),3))return true;
            BeforeWorld=World;Tap(EKeys::Gamepad_FaceButton_Bottom);Step=7;break;
        }
        case 7:
            if(World==BeforeWorld.Get() || !Host->IsVerdanRevisit() || Host->GetState()!=EMemoriaSliceState::Exploration || PC->GetNarrativeWidget())break;
            Test->TestTrue(TEXT("Physical checkpoint return reaches actual Verdan exploration"),World->GetMapName().EndsWith(TEXT("L_VerdanHost")) && !Host->IsOnTitle());Route=1;Step=0;break;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;double Started,OldDelta=0.;uint64 LastFrame=MAX_uint64;
    int32 Step=0,Route=0,Frame=0,StableChecks=0;bool bFixed=false,bOldFixed=false;
    FGuid SavedRun;int64 SavedHp=0,SavedGrains=0;FVector2D SavedPosition=FVector2D::ZeroVector;
    FString SavedFlag,SavedWorld,SavedBytes,OldLocale;TArray<uint8> SavedMemory;TArray<FString> SavedBurns;
    TWeakObjectPtr<UWorld> BeforeWorld;TWeakObjectPtr<UMemoriaSettingsSubsystem> Settings;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS362TitleVerdanContinue,"MemoriaSave.S362.TitleVerdanContinue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FS362TitleVerdanContinue::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Memoria/Maps/L_BeltWaystation")))return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FS362TitleVerdanReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
