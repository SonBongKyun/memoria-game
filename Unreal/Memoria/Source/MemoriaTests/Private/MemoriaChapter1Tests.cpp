#include "Misc/AutomationTest.h"
#include "Import/MemoriaNarrativeImport.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Narrative/MemoriaNarrativeRuntime.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Current Chapter 1 route, in scene_flow.gd goto_scene order, ending at the Chapter 2 arrival.
const TCHAR* Chapter1Route[] = {TEXT("ch1_cold_open"),TEXT("ch1_prologue"),TEXT("ch1_forest_walk"),TEXT("ch1_void_beast"),TEXT("ch1_after_forest")};
FString Chapter1Ir(const FString& Id)
{ return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/ir/narrative/")/(Id+TEXT(".vn.v1.json"))); }
const UMemoriaVNAsset* Chapter1Asset(const FString& Id)
{ return LoadObject<UMemoriaVNAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(true,Id)); }
TSharedPtr<FJsonValue> Chapter1Fixture(const FString& Name)
{
    FString Text; TSharedPtr<FJsonValue> V;
    if (FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/chapter1/")/Name))) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),V);
    return V;
}
TSharedPtr<FJsonObject> Chapter1Case(const FString& File,const FString& Id)
{
    const auto Root=Chapter1Fixture(File); if (!Root) return nullptr;
    for (const auto& V:Root->AsArray()) if (V->AsObject()->GetStringField(TEXT("id"))==Id) return V->AsObject();
    return nullptr;
}
// Oracle vocabulary only: inventory_changed is an Unreal-side signal record, not a SceneFlow event.
bool Chapter1Comparable(const FString& E) { return !E.StartsWith(TEXT("inventory_changed:")); }
// Drive the real host with the source picks; a view with a choice title is a choice step.
int32 DriveHost(UMemoriaNarrativeSubsystem* Host,const TArray<TSharedPtr<FJsonValue>>& Picks,TFunctionRef<void(const FMemoriaNarrativeView&)> Each)
{
    int32 Used=0;
    for (int32 Guard=0;Host->GetState()==EMemoriaSliceState::VN && Guard<4000;++Guard)
    {
        const auto View=Host->GetView(); Each(View);
        if (!View.ChoiceTitle.IsEmpty()) { if (!Picks.IsValidIndex(Used)) break; Host->Confirm(int32(Picks[Used++]->AsNumber())); }
        else Host->Confirm(INDEX_NONE);
    }
    return Used;
}
// The host trace as oracle events: interpreter events are recorded under the vn: dialect.
TArray<FString> HostEvents(const UMemoriaNarrativeSubsystem* Host)
{
    TArray<FString> Out;
    for (const auto& E:Host->GetTrace())
        if (E.StartsWith(TEXT("vn:")) && !E.StartsWith(TEXT("vn:start:")) && Chapter1Comparable(E.RightChop(3))) Out.Add(E.RightChop(3));
    return Out;
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FChapter1ImportParity,"Memoria.Chapter1.ImportParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FChapter1ImportParity::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{ for (const TCHAR* Id:Chapter1Route) { Names.Add(Id); Commands.Add(Id); } }
bool FChapter1ImportParity::RunTest(const FString& Id)
{
    // The saved production asset equals a fresh strict read of the source-attested IR.
    const auto* Actual=Chapter1Asset(Id); if (!TestNotNull(TEXT("Saved Chapter 1 asset"),Actual)) return false;
    TStrongObjectPtr<UMemoriaVNAsset> Expected(NewObject<UMemoriaVNAsset>()); FString Error;
    if (!TestTrue(TEXT("Strict IR + direct source attestation"),MemoriaNarrativeImport::ReadIr(Chapter1Ir(Id),*Expected,Error))) { AddError(Error); return false; }
    for (TFieldIterator<FProperty> P(UMemoriaVNAsset::StaticClass(),EFieldIterationFlags::None);P;++P) TestTrue(*P->GetName(),P->Identical_InContainer(Actual,Expected.Get()));
    TestEqual(TEXT("Typed semantic fingerprint"),MemoriaNarrativeImport::Fingerprint(*Actual),Expected->ImportMetadata.SemanticSha256);
    TestEqual(TEXT("Chapter 1 metadata"),Actual->Definition.Metadata.Chapter,1);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter1RouteChain,"Memoria.Chapter1.RouteChain",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FChapter1RouteChain::RunTest(const FString&)
{
    // Every scene hands off to the next with goto_scene; the last reaches the Chapter 2 arrival.
    int32 Distorted=0,SetChapter=0,CompleteChapter=0,Autosave=0,Impacts=0,ChoiceFraming=0,ChoiceEffects=0;
    for (int32 I=0;I<UE_ARRAY_COUNT(Chapter1Route);++I)
    {
        const auto* A=Chapter1Asset(Chapter1Route[I]); if (!TestNotNull(Chapter1Route[I],A)) return false;
        const FString Next=I+1<UE_ARRAY_COUNT(Chapter1Route)?FString(Chapter1Route[I+1]):FString(TEXT("ch2_market_arrival"));
        bool bHandsOff=false;
        for (const auto& S:A->Definition.Steps)
        {
            if (S.Action.bHasAction && S.Action.Action==TEXT("goto_scene") && S.Action.Id==Next) bHandsOff=true;
            if (S.Action.bHasAction && S.Action.Action==TEXT("goto_scene"))
                TestTrue(TEXT("goto_scene target is in the imported cohort"),Chapter1Asset(S.Action.Id)!=nullptr);
            Distorted+=S.Text.bHasDistortIfBurned; SetChapter+=S.Effects.bHasSetChapter; CompleteChapter+=S.Effects.bHasCompleteChapter;
            Autosave+=S.Effects.bHasAutosaveChapterTransition; Impacts+=S.Presentation.bHasImpact; ChoiceFraming+=S.Text.bHasChoiceTitle;
            for (const auto& C:S.Choices) ChoiceEffects+=C.Text.bHasEffect;
        }
        TestTrue(*FString::Printf(TEXT("%s hands off to %s"),Chapter1Route[I],*Next),bHandsOff);
    }
    // Source counts from data/vn_scenes/ch1_*.json (see the S302 survey).
    TestEqual(TEXT("Distortion steps"),Distorted,4);
    TestEqual(TEXT("set_chapter"),SetChapter,1);TestEqual(TEXT("complete_chapter"),CompleteChapter,1);TestEqual(TEXT("autosave"),Autosave,1);
    TestEqual(TEXT("Impact cues"),Impacts,5);TestEqual(TEXT("Choice titles"),ChoiceFraming,3);TestEqual(TEXT("Choice effect texts"),ChoiceEffects,8);
    TestNotNull(TEXT("Chapter 2 arrival still imported"),Chapter1Asset(TEXT("ch2_market_arrival")));
    return !HasAnyErrors();
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FChapter1OracleRoute,"Memoria.Chapter1.OracleRoute",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FChapter1OracleRoute::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    if (const auto Root=Chapter1Fixture(TEXT("contract_inputs.v1.json")))
        for (const auto& V:Root->AsArray()) { const FString Id=V->AsObject()->GetStringField(TEXT("id")); Names.Add(Id); Commands.Add(Id); }
}
bool FChapter1OracleRoute::RunTest(const FString& Id)
{
    // Play the imported Chapter 1 route with the source picks and compare to the executed SceneFlow.
    const auto Input=Chapter1Case(TEXT("contract_inputs.v1.json"),Id),Gold=Chapter1Case(TEXT("contract_expected.v1.json"),Id);
    if (!TestTrue(TEXT("Oracle case"),Input.IsValid()&&Gold.IsValid())) return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init(); auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    if (!TestTrue(TEXT("Starting run"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success)) { Game->Shutdown(); return false; }
    // The oracle's GameManager: chapter 1, 0 Grains, 50/100 HP, Elia in the party, no flags.
    FMemoriaRunSnapshot State=Run->GetRunSnapshot(); State.CurrentChapter=1; State.CurrentLocale=Input->GetStringField(TEXT("locale"));
    State.Player.Grains=0; State.Player.Hp=50; State.Player.MaxHp=100; State.Player.bEliaWithParty=true; State.StoryFlags.Reset(); State.Player.Items.Reset();
    FMemoriaNarrativeContext Context(State,*Run->GetPlayerMemory());
    const auto* Start=Chapter1Asset(TEXT("ch1_cold_open")); if (!TestNotNull(TEXT("Cold open"),Start)) { Game->Shutdown(); return false; }
    FMemoriaVNInterpreter VN(Start->Definition,Context,[](const FString& Scene)->const FMemoriaVNDefinition*{ const auto* A=Chapter1Asset(Scene); return A?&A->Definition:nullptr; },true);
    TArray<TSharedPtr<FJsonObject>> Points;
    auto Snapshot=[&](const TCHAR* Kind)
    {
        // Scene is SceneFlow.current_id, which stays empty until play().
        auto O=MakeShared<FJsonObject>(); const auto C=VN.ExportContinuation(); const auto M=Run->GetPlayerMemory()->GetSnapshot();
        O->SetStringField(TEXT("kind"),Kind); O->SetStringField(TEXT("scene"),C.Current.SequenceId); O->SetNumberField(TEXT("index"),C.Current.OriginalIndex);
        O->SetNumberField(TEXT("grains"),State.Player.Grains); O->SetNumberField(TEXT("hp"),State.Player.Hp); O->SetNumberField(TEXT("chapter"),State.CurrentChapter);
        TArray<FString> Flags; for (const auto& F:State.StoryFlags) if (F.bValue) Flags.Add(F.Id); Flags.Sort(); O->SetStringField(TEXT("flags"),FString::Join(Flags,TEXT(",")));
        O->SetStringField(TEXT("burned"),FString::Join(M.BurnedHistory,TEXT(","))); O->SetNumberField(TEXT("anchor_vigil"),M.AnchorVigil);
        TArray<FString> Passives; for (const auto& P:M.AnchorPassives) if (P.bValue) Passives.Add(P.Id); Passives.Sort(); O->SetStringField(TEXT("anchor_passives"),FString::Join(Passives,TEXT(",")));
        O->SetNumberField(TEXT("guard_slots_used"),M.GuardSlotsUsed); O->SetStringField(TEXT("map"),Context.RequestedMap); Points.Add(O);
    };
    Snapshot(TEXT("start")); VN.Play(0);
    const auto& Picks=Input->GetArrayField(TEXT("picks")); int32 Used=0;
    for (int32 Guard=0;VN.ExportContinuation().bActive&&Guard<4000;++Guard)
    {
        const auto& Steps=VN.GetDefinition().Steps; const int32 I=VN.ExportContinuation().Current.OriginalIndex;
        if (Steps.IsValidIndex(I)&&Steps[I].bChoicesPresent)
        {
            Snapshot(TEXT("choice")); if (!TestTrue(TEXT("Pick available"),Picks.IsValidIndex(Used))) break;
            VN.SelectOriginalChoice(int32(Picks[Used++]->AsNumber()));
        }
        else VN.Advance();
    }
    TestEqual(TEXT("Every source pick used"),Used,Picks.Num());
    TestEqual(TEXT("Route reaches Verdan"),Context.RequestedMap,FString(TEXT("res://scenes/maps/verdan_market.tscn")));
    Snapshot(TEXT("final"));
    // Event trace: visits, steps with displayed (distorted) text, choices, flags, burns, items, chapter effects.
    TArray<FString> Actual; for (const auto& E:Context.Events) if (Chapter1Comparable(E)) Actual.Add(E);
    TArray<FString> Expected; for (const auto& V:Gold->GetArrayField(TEXT("events"))) if (Chapter1Comparable(V->AsString())) Expected.Add(V->AsString());
    TestEqual(TEXT("Event count"),Actual.Num(),Expected.Num());
    for (int32 I=0;I<FMath::Min(Actual.Num(),Expected.Num());++I)
        if (!Actual[I].Equals(Expected[I],ESearchCase::CaseSensitive)) { AddError(FString::Printf(TEXT("First event mismatch at %d: actual=%s expected=%s"),I,*Actual[I],*Expected[I])); break; }
    const auto& States=Gold->GetArrayField(TEXT("states"));
    if (TestEqual(TEXT("Snapshot count"),Points.Num(),States.Num()))
        for (int32 I=0;I<States.Num();++I)
        {
            const auto E=States[I]->AsObject(); const auto A=Points[I]; const FString At=FString::Printf(TEXT(" at %d"),I);
            TArray<FString> Flags; for (const auto& F:E->GetObjectField(TEXT("flags"))->Values) if (F.Value->AsBool()) Flags.Add(FString(F.Key.ToView())); Flags.Sort();
            TArray<FString> Burned; for (const auto& B:E->GetArrayField(TEXT("burned"))) Burned.Add(B->AsString());
            TArray<FString> Passives; for (const auto& P:E->GetArrayField(TEXT("anchor_passives"))) Passives.Add(P->AsString());
            TestEqual(TEXT("Scene")+At,A->GetStringField(TEXT("scene")),E->GetStringField(TEXT("scene")));
            TestEqual(TEXT("Index")+At,int32(A->GetNumberField(TEXT("index"))),int32(E->GetNumberField(TEXT("index"))));
            TestEqual(TEXT("Flags")+At,A->GetStringField(TEXT("flags")),FString::Join(Flags,TEXT(",")));
            TestEqual(TEXT("Grains")+At,A->GetNumberField(TEXT("grains")),E->GetNumberField(TEXT("grains")));
            TestEqual(TEXT("HP")+At,A->GetNumberField(TEXT("hp")),E->GetNumberField(TEXT("hp")));
            TestEqual(TEXT("Chapter")+At,A->GetNumberField(TEXT("chapter")),E->GetNumberField(TEXT("chapter")));
            TestEqual(TEXT("Burned")+At,A->GetStringField(TEXT("burned")),FString::Join(Burned,TEXT(",")));
            TestEqual(TEXT("Anchor vigil")+At,A->GetNumberField(TEXT("anchor_vigil")),E->GetNumberField(TEXT("anchor_vigil")));
            TestEqual(TEXT("Anchor passives")+At,A->GetStringField(TEXT("anchor_passives")),FString::Join(Passives,TEXT(",")));
            TestEqual(TEXT("Guard slots")+At,A->GetNumberField(TEXT("guard_slots_used")),E->GetNumberField(TEXT("guard_slots_used")));
            TestEqual(TEXT("Map")+At,A->GetStringField(TEXT("map")),E->GetStringField(TEXT("map")));
        }
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FChapter1NewGameHost,"Memoria.Chapter1.NewGameHost",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FChapter1NewGameHost::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{
    if (const auto Root=Chapter1Fixture(TEXT("contract_inputs.v1.json")))
        for (const auto& V:Root->AsArray()) { const FString Id=V->AsObject()->GetStringField(TEXT("id")); Names.Add(Id); Commands.Add(Id); }
}
bool FChapter1NewGameHost::RunTest(const FString& Id)
{
    // New Game through the production host plays the same route as the executed SceneFlow.
    const auto Input=Chapter1Case(TEXT("contract_inputs.v1.json"),Id),Gold=Chapter1Case(TEXT("contract_expected.v1.json"),Id);
    if (!TestTrue(TEXT("Oracle case"),Input.IsValid()&&Gold.IsValid())) return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>(); auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    auto* Audio=Game->GetSubsystem<UMemoriaAudioSubsystem>(); const int32 PulsesBefore=Audio->GetCueCount(TEXT("void_pulse"));
    if (!TestTrue(TEXT("New Game starts"),Host->StartNewGame(Input->GetStringField(TEXT("locale"))))) { Game->Shutdown(); return false; }
    // main.gd _on_new_game_pressed player_data.
    const auto Start=Run->GetRunSnapshot();
    TestEqual(TEXT("Chapter 1"),Start.CurrentChapter,int64(1)); TestEqual(TEXT("Full HP"),Start.Player.Hp,int64(100)); TestEqual(TEXT("Max HP"),Start.Player.MaxHp,int64(100));
    TestEqual(TEXT("No Grains"),Start.Player.Grains,int64(0)); TestTrue(TEXT("Elia with party"),Start.Player.bEliaWithParty);
    TestEqual(TEXT("Witness ink"),Run->GetItemCount(TEXT("witness_ink")),int64(1)); TestEqual(TEXT("Only the ink"),Start.Player.Items.Num(),1);
    TestEqual(TEXT("Quick slots"),FString::Join(Start.Player.QuickSlots,TEXT(",")),FString(TEXT("witness_ink,potion,antidote")));
    TestTrue(TEXT("Route host"),Host->IsNewGameRoute());
    const auto First=Host->GetView();
    TestEqual(TEXT("Cold open location"),First.LocationTitle,FString(Input->GetStringField(TEXT("locale"))==TEXT("ko")?TEXT("1장  /  재가 가라앉기 전"):TEXT("CHAPTER 1  /  BEFORE THE ASH SETTLED")));
    TestTrue(TEXT("VN header"),First.Header.Contains(TEXT("VN")));
    TestEqual(TEXT("First cue"),First.CueKey,FString(TEXT("ch1_cold_open:0"))); TestEqual(TEXT("Pull back"),First.CgMotion,FString(TEXT("pull_back")));
    TestEqual(TEXT("Cold open plays void_pulse"),Audio->GetCueCount(TEXT("void_pulse")),PulsesBefore+1);
    TestEqual(TEXT("Cold open music"),Host->GetSceneMusic(),FName(TEXT("dialogue_tense")));
    int32 Views=0,Choices=0,SystemLogs=0,Distorted=0; TSet<FString> Cues; FMemoriaNarrativeView Final;
    const int32 Used=DriveHost(Host,Input->GetArrayField(TEXT("picks")),[&](const FMemoriaNarrativeView& V)
    {
        ++Views; Cues.Add(V.CueKey); Final=V;
        TestNotNull(*(TEXT("Backdrop resolves at ")+V.CueKey),MemoriaNarrativeArtwork::Load(V.BackdropSource));
        if (!V.PortraitSource.IsEmpty()) TestNotNull(*(TEXT("Portrait resolves at ")+V.CueKey),MemoriaNarrativeArtwork::Load(V.PortraitSource));
        if (!V.ChoiceTitle.IsEmpty()) { ++Choices; TestFalse(TEXT("Choice hint"),V.ChoiceHint.IsEmpty()); }
        if (V.bSystemLog) { ++SystemLogs; TestTrue(TEXT("System log body"),V.Body.Contains(Input->GetStringField(TEXT("locale"))==TEXT("ko")?TEXT("[연소 감지"):TEXT("[COMBUSTION DETECTED")));  }
        if (V.bDistorted) ++Distorted;
    });
    TestEqual(TEXT("Every source pick used"),Used,Input->GetArrayField(TEXT("picks")).Num());
    TestEqual(TEXT("Route travels to Verdan"),Host->GetState(),EMemoriaSliceState::Travelling);
    TestTrue(TEXT("Choices shown"),Choices==Used); TestEqual(TEXT("Prologue system log shown once"),SystemLogs,1);
    const auto Actual=HostEvents(Host); TArray<FString> Expected;
    for (const auto& V:Gold->GetArrayField(TEXT("events"))) if (Chapter1Comparable(V->AsString())) Expected.Add(V->AsString());
    TestEqual(TEXT("Host event count"),Actual.Num(),Expected.Num());
    for (int32 I=0;I<FMath::Min(Actual.Num(),Expected.Num());++I)
        if (!Actual[I].Equals(Expected[I],ESearchCase::CaseSensitive)) { AddError(FString::Printf(TEXT("First host event mismatch at %d: actual=%s expected=%s"),I,*Actual[I],*Expected[I])); break; }
    // Chapter completion: the ledger follows _show_chapter_ledger's lines.
    // The last VN view (the arrival choice) still carries the ledger raised at completion.
    const auto End=Run->GetRunSnapshot();
    TestEqual(TEXT("Chapter 2 at arrival"),End.CurrentChapter,int64(2));
    TestEqual(TEXT("One ledger"),Final.LedgerSerial,1);
    if (!TestEqual(TEXT("Ledger lines"),Final.LedgerLines.Num(),4)) { Game->Shutdown(); return false; }
    const bool Ko=Input->GetStringField(TEXT("locale"))==TEXT("ko");
    TestEqual(TEXT("Ledger title"),Final.LedgerTitle,FString(Ko?TEXT("장부, 제1장"):TEXT("THE LEDGER, CHAPTER 1")));
    // Source order: set_chapter 2 resets the ledger snapshot on the same step, so it lists nothing.
    TestEqual(TEXT("Ledger burn line (source order)"),Final.LedgerLines[0],FString(Ko?TEXT("이번 장에서 태운 기억: 없음"):TEXT("Burned this chapter: nothing")));
    const bool SongBurned=Run->GetPlayerMemory()->GetSnapshot().BurnedHistory.Contains(TEXT("daily_campfire_song"));
    TestEqual(TEXT("Song burn shows distorted lines"),Distorted>0,SongBurned);
    TestTrue(TEXT("Trace records the ledger"),Host->GetTrace().ContainsByPredicate([](const FString& E){return E.StartsWith(TEXT("ledger:shown:1:"));}));
    // Automation storage is disabled unless a test configures it: the autosave is recorded as skipped.
    TestTrue(TEXT("Autosave attempted"),Host->GetTrace().Contains(TEXT("autosave:skipped")));
    TestEqual(TEXT("Arrival music"),Host->GetSceneMusic(),FName(TEXT("ch2_verdan")));
    Game->Shutdown(); return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter1AutosaveResume,"Memoria.Chapter1.AutosaveResume",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FChapter1AutosaveResume::RunTest(const FString&)
{
    // SaveManager.autosave_on_chapter_transition: the completed chapter resumes at the Chapter 2 arrival.
    const auto Input=Chapter1Case(TEXT("contract_inputs.v1.json"),TEXT("burn_song_strike_en"));
    if (!TestTrue(TEXT("Oracle case"),Input.IsValid())) return false;
    const FString Leaf=TEXT("ch1-autosave-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TArray<FString> BurnedFirst; FString ChapterSlot;
    {
        TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
        auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>(); auto* Checkpoint=Game->GetSubsystem<UMemoriaCheckpointSubsystem>();
        if (!TestTrue(TEXT("Isolated storage"),Checkpoint->ConfigureTestStorage(Leaf)) || !TestTrue(TEXT("New Game"),Host->StartNewGame())) { Game->Shutdown(); return false; }
        TArray<TSharedPtr<FJsonValue>> Picks=Input->GetArrayField(TEXT("picks")); Picks.Pop(); // stop before the arrival choice
        DriveHost(Host,Picks,[](const FMemoriaNarrativeView&){});
        TestEqual(TEXT("One autosave"),Host->GetAutosaveCount(),1);
        TestTrue(TEXT("Autosave written"),Host->GetTrace().Contains(TEXT("autosave:saved")));
        ChapterSlot=Checkpoint->GetChapterSlotPath(); TestTrue(TEXT("Chapter slot on disk"),IFileManager::Get().FileExists(*ChapterSlot));
        TestTrue(TEXT("Boundary slot untouched"),!IFileManager::Get().FileExists(*Checkpoint->GetSlotPath()));
        BurnedFirst=Game->GetSubsystem<UMemoriaRunSubsystem>()->GetPlayerMemory()->GetSnapshot().BurnedHistory;
        Game->Shutdown();
    }
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>(); auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    TestTrue(TEXT("Same isolated storage"),Game->GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(Leaf));
    if (!TestTrue(TEXT("Autosave resumes"),Host->ResumeChapterAutosave())) { Game->Shutdown(); return false; }
    TestEqual(TEXT("Resumes at the arrival"),Host->GetContinuation().Current.SequenceId,FString(TEXT("ch2_market_arrival")));
    TestEqual(TEXT("From its first step"),Host->GetContinuation().Current.OriginalIndex,0);
    TestEqual(TEXT("Chapter 2 restored"),Run->GetRunSnapshot().CurrentChapter,int64(2));
    TestEqual(TEXT("Burns restored"),FString::Join(Run->GetPlayerMemory()->GetSnapshot().BurnedHistory,TEXT(",")),FString::Join(BurnedFirst,TEXT(",")));
    TestTrue(TEXT("Chapter flag restored"),Run->GetRunSnapshot().GetFlag(TEXT("ch1_complete")));
    TestTrue(TEXT("Route presentation"),Host->IsNewGameRoute()); TestEqual(TEXT("No ledger replay"),Host->GetView().LedgerSerial,0);
    const auto Picks=Input->GetArrayField(TEXT("picks"));
    DriveHost(Host,TArray<TSharedPtr<FJsonValue>>{Picks.Last()},[](const FMemoriaNarrativeView&){});
    TestEqual(TEXT("Resumed route reaches Verdan"),Host->GetState(),EMemoriaSliceState::Travelling);
    TestTrue(TEXT("Arrival seen"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_arrival_vn_seen")));
    Game->Shutdown();
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(ChapterSlot),false,true);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter1Presentation,"MemoriaVisual.Chapter1Presentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FChapter1Presentation::RunTest(const FString&)
{
    // vn_scene.gd one-shot cues on the real widget, driven by the deterministic cue clock.
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>()); Game->Init();
    auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>();
    if (!TestTrue(TEXT("New Game"),Host->StartNewGame())) { Game->Shutdown(); return false; }
    auto* Widget=CreateWidget<UMemoriaDevelopmentNarrativeWidget>(Game.Get()); Widget->TakeWidget();
    Widget->Display(Host->GetView()); Widget->AdvancePresentation(0.f);
    auto Probe=Widget->GetPresentationProbe();
    TestNotNull(TEXT("Cold open CG"),Widget->DisplayedBackdrop());
    TestEqual(TEXT("Pull back motion"),Probe.Motion,FString(TEXT("pull_back"))); TestTrue(TEXT("Pull back starts wide"),FMath::IsNearlyEqual(Probe.BackdropScale,1.09f,.001f));
    Widget->AdvancePresentation(8.5f); TestTrue(TEXT("Pull back settles"),FMath::IsNearlyEqual(Widget->GetPresentationProbe().BackdropScale,1.f,.001f));
    Host->Confirm(INDEX_NONE); const auto Impact=Host->GetView(); Widget->Display(Impact);
    TestEqual(TEXT("Void impact step"),Impact.Impact,FString(TEXT("void")));
    TestTrue(TEXT("Crossfade from the previous CG"),Widget->GetPresentationProbe().BackdropOpacity<.01f);
    Widget->AdvancePresentation(.2f); Probe=Widget->GetPresentationProbe();
    TestTrue(TEXT("Crossfade progresses"),Probe.BackdropOpacity>.5f);
    TestTrue(TEXT("Flash waits for the fade"),Probe.FlashAlpha==0.f);
    Widget->AdvancePresentation(.1f); Probe=Widget->GetPresentationProbe();
    TestTrue(TEXT("Void flash visible"),Probe.FlashAlpha>.05f); TestTrue(TEXT("Nudge moves the CG"),!Probe.Nudge.IsNearlyZero());
    Widget->AdvancePresentation(1.f); Probe=Widget->GetPresentationProbe();
    TestEqual(TEXT("Flash decays"),Probe.FlashAlpha,0.f); TestTrue(TEXT("Nudge returns"),Probe.Nudge.IsNearlyZero());
    Host->Confirm(INDEX_NONE); const auto Choice=Host->GetView(); Widget->Display(Choice);
    const FString Visible=Widget->VisibleText();
    TestTrue(TEXT("Choice title"),Visible.Contains(TEXT("WHAT MOVES BEFORE MEMORY?")));
    TestTrue(TEXT("Choice hint"),Visible.Contains(TEXT("small advantage")));
    TestTrue(TEXT("Choice effect preview"),Choice.Choices.Num()>0 && !Choice.Choices[0].Effect.IsEmpty() && Visible.Contains(Choice.Choices[0].Effect));
    // Pick the song burn, then walk to the prologue system log and the campfire distortion.
    const auto Input=Chapter1Case(TEXT("contract_inputs.v1.json"),TEXT("burn_song_strike_en"));
    bool SawSystem=false,SawDistorted=false,SawLedger=false; FString LedgerText;
    DriveHost(Host,Input->GetArrayField(TEXT("picks")),[&](const FMemoriaNarrativeView& V)
    {
        Widget->Display(V); Widget->AdvancePresentation(1.f); const auto P=Widget->GetPresentationProbe();
        SawSystem|=P.bSystemStyle; SawDistorted|=P.bDistortedStyle;
        if (P.LedgerAlpha>.99f && !SawLedger) { SawLedger=true; LedgerText=P.LedgerText; }
    });
    TestTrue(TEXT("System log styled"),SawSystem); TestTrue(TEXT("Distorted line styled"),SawDistorted);
    TestTrue(TEXT("Ledger overlay shown"),SawLedger); TestTrue(TEXT("Ledger text"),LedgerText.Contains(TEXT("THE LEDGER, CHAPTER 1")) && LedgerText.Contains(TEXT("Anchors:")));
    Widget->AdvancePresentation(6.f); TestEqual(TEXT("Ledger fades out"),Widget->GetPresentationProbe().LedgerAlpha,0.f);
    Game->Shutdown(); return !HasAnyErrors();
}
#endif
