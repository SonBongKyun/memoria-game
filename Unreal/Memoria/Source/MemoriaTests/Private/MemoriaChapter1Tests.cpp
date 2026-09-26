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
#endif
