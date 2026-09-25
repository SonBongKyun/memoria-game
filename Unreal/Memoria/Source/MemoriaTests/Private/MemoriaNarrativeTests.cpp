#include "Misc/AutomationTest.h"
#include "Import/MemoriaNarrativeImport.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Narrative/MemoriaNarrativeRuntime.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using Obj=TSharedPtr<FJsonObject>; using Val=TSharedPtr<FJsonValue>;
FString Base() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/")); }
FString Ir(bool V) { return Base()/TEXT("ir/narrative/")/(V?TEXT("ch2_market_arrival.vn.v1.json"):TEXT("verdan_arrival.field.v1.json")); }
Val ReadJson(const FString& Path)
{ FString S; Val V; if (FFileHelper::LoadFileToString(S,*Path)) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V); return V; }
FString Canon(const Val& V) { return MemoriaCatalogImport::Canonical(V); }
void Eq(FAutomationTestBase& T,const FString& Label,const FString& A,const FString& E)
{ T.TestTrue(*(Label+TEXT(" exact")),A.Equals(E,ESearchCase::CaseSensitive)); if (!A.Equals(E,ESearchCase::CaseSensitive)) T.AddError(Label+TEXT(" actual=")+A+TEXT(" expected=")+E); }
TArray<Val> Strings(const TArray<FString>& S) { TArray<Val> R; for (const auto& V:S) R.Add(MakeShared<FJsonValueString>(V)); return R; }
Obj ContinuationJson(const FMemoriaVNContinuation& C)
{
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("current_id"),C.Current.SequenceId); R->SetNumberField(TEXT("current_index"),C.Current.OriginalIndex);
    R->SetStringField(TEXT("pending_scene_id"),C.Pending.SequenceId); R->SetNumberField(TEXT("pending_start_index"),C.Pending.OriginalIndex); R->SetBoolField(TEXT("is_active"),C.bActive); R->SetNumberField(TEXT("ledger_burn_snapshot"),double(C.LedgerBurnSnapshot));
    TArray<Val> Q; for (const auto& C0:C.ResumeQueue) { auto V=MakeShared<FJsonObject>(); V->SetStringField(TEXT("scene_id"),C0.SequenceId); V->SetNumberField(TEXT("index"),C0.OriginalIndex); Q.Add(MakeShared<FJsonValueObject>(V)); } R->SetArrayField(TEXT("resume_queue"),Q); return R;
}
Obj Snapshot(FMemoriaNarrativeContext& C,bool Active,int32 Index,const FMemoriaVNContinuation* VN=nullptr)
{
    auto R=MakeShared<FJsonObject>(); R->SetArrayField(TEXT("events"),Strings(C.Events)); auto Flags=MakeShared<FJsonObject>(); for (const auto& F:C.Run.StoryFlags) Flags->SetBoolField(F.Id,F.bValue); R->SetObjectField(TEXT("flags"),Flags);
    R->SetNumberField(TEXT("grains"),double(C.Run.Player.Grains)); R->SetNumberField(TEXT("hp"),double(C.Run.Player.Hp)); R->SetStringField(TEXT("map"),C.RequestedMap);
    R->SetArrayField(TEXT("burned"),Strings(C.Memory.GetSnapshot().BurnedHistory)); R->SetBoolField(TEXT("active"),Active); R->SetNumberField(TEXT("index"),Index);
    if (VN) R->SetObjectField(TEXT("continuation"),ContinuationJson(*VN)); return R;
}
bool Replay(FAutomationTestBase& T,const FString& Id)
{
    auto Inputs=ReadJson(Base()/TEXT("ir/narrative/contract_inputs.v1.json")); auto Expected=ReadJson(Base()/TEXT("ir/narrative/contract_expected.v1.json"));
    if (!Inputs || !Expected) { T.AddError(TEXT("Missing attested oracle files")); return false; }
    Obj Input,Expect;
    for (auto C:Inputs->AsArray()) if (C->AsObject()->GetStringField(TEXT("id"))==Id) Input=C->AsObject();
    for (auto C:Expected->AsArray()) if (C->AsObject()->GetStringField(TEXT("id"))==Id) Expect=C->AsObject();
    if (!Input||!Expect) { T.AddError(TEXT("Case missing")); return false; }
    auto* Catalog=LoadObject<UMemoriaMemoryCatalog>(nullptr,MemoriaCatalogImport::ObjectPath); if (!T.TestNotNull(TEXT("Starting memory catalog"),Catalog)) return false;
    TStrongObjectPtr<UMemoriaPlayerMemoryDomain> Memory(NewObject<UMemoriaPlayerMemoryDomain>()); FMemoriaMemorySnapshot Initial;
    for (const auto& D:Catalog->Definitions) { FMemoriaMemoryState S; S.Id=D.Id; Initial.Owned.Add(S); }
    T.TestTrue(TEXT("Real memory domain initializes"),Memory->Restore(Catalog->Definitions,Initial)==EMemoriaMemoryResult::Success);
    FMemoriaRunSnapshot Run; Run.CurrentLocale=Input->GetStringField(TEXT("locale")); Run.Player.Hp=50;
    FMemoriaNarrativeContext Context(Run,*Memory);
    for (auto V:Input->GetArrayField(TEXT("burn_before"))) Context.Burn(V->AsString()); Context.Events.Reset();
    bool VN=Input->GetStringField(TEXT("dialect"))==TEXT("vn"), Synthetic=Input->GetBoolField(TEXT("synthetic"));
    TStrongObjectPtr<UMemoriaFieldAsset> Field(Synthetic && !VN?NewObject<UMemoriaFieldAsset>():LoadObject<UMemoriaFieldAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(false)));
    TStrongObjectPtr<UMemoriaVNAsset> Sequence(Synthetic && VN?NewObject<UMemoriaVNAsset>():LoadObject<UMemoriaVNAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(true)));
    if (!T.TestNotNull(TEXT("Field asset"),Field.Get()) || !T.TestNotNull(TEXT("VN asset"),Sequence.Get())) return false;
    if (Synthetic)
    {
        FString Error; auto P=Base()/TEXT("fixtures/narrative/")/(Id+TEXT(".json"));
        if (!T.TestTrue(TEXT("Isolated synthetic fixture to transient typed object"),VN?MemoriaNarrativeImport::ReadIr(P,*Sequence,Error,false):MemoriaNarrativeImport::ReadIr(P,*Field,Error,false))) { T.AddError(Error); return false; }
    }
    FMemoriaFieldInterpreter F(Field->Definition,Context); FMemoriaVNInterpreter V(Sequence->Definition,Context);
    TArray<Val> States;
    auto Snap=[&]() { auto C=V.ExportContinuation(); States.Add(MakeShared<FJsonValueObject>(Snapshot(Context,VN?C.bActive:F.IsActive(),VN?C.Current.OriginalIndex:F.OriginalIndex(),VN?&C:nullptr))); };
    if (VN) V.Play(); else F.Start(); Snap();
    for (auto Cmd:Input->GetArrayField(TEXT("commands")))
    {
        auto C=Cmd->AsObject();
        if (C->HasField(TEXT("advance"))) { if (VN) V.Advance(); else F.Advance(); }
        if (C->HasField(TEXT("select"))) { if (VN) V.SelectOriginalChoice(int32(C->GetNumberField(TEXT("select")))); else F.SelectFilteredChoice(int32(C->GetNumberField(TEXT("select")))); }
        if (C->HasField(TEXT("resume")))
        {
            TStrongObjectPtr<UMemoriaRunSaveGame> Save(NewObject<UMemoriaRunSaveGame>()); Save->SceneFlow=V.ExportContinuation();
            Save->SceneFlow.Pending.SequenceId=Sequence->Definition.Id; Save->SceneFlow.Pending.OriginalIndex=1;
            Save->SceneFlow.ResumeQueue.Reset(); for (int32 I:{3,4}) { FMemoriaVNCursor Cursor; Cursor.SequenceId=Sequence->Definition.Id; Cursor.OriginalIndex=I; Save->SceneFlow.ResumeQueue.Add(Cursor); }
            TArray<uint8> Bytes; T.TestTrue(TEXT("Existing SaveGame schema serializes continuation"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
            TStrongObjectPtr<UMemoriaRunSaveGame> Reload(Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
            if (!T.TestNotNull(TEXT("DTO reload"),Reload.Get())) return false;
            T.TestEqual(TEXT("Schema unchanged"),Reload->SchemaVersion,1); T.TestTrue(TEXT("Active takes precedence over pending"),V.PrepareResume(Reload->SceneFlow)); Snap();
            T.TestTrue(TEXT("Consume restored pending"),V.ConsumePendingOrQueue());
        }
        if (C->HasField(TEXT("consume"))) T.TestTrue(TEXT("FIFO queue resumes"),V.ConsumePendingOrQueue());
        Snap();
    }
    const auto& ExpectedStates=Expect->GetArrayField(TEXT("states")); T.TestEqual(TEXT("Observable snapshot count"),States.Num(),ExpectedStates.Num());
    for (int32 I=0;I<FMath::Min(States.Num(),ExpectedStates.Num());++I) Eq(T,Id+TEXT(" snapshot ")+FString::FromInt(I),Canon(States[I]),Canon(ExpectedStates[I]));
    T.AddInfo(Id+TEXT(" source-authentic snapshots=")+FString::FromInt(States.Num())); return !T.HasAnyErrors();
}
template<class A> bool Parity(FAutomationTestBase& T,bool V)
{
    auto* Actual=LoadObject<A>(nullptr,*MemoriaNarrativeImport::ObjectPath(V)); if (!T.TestNotNull(TEXT("Saved production asset reload"),Actual)) return false;
    TStrongObjectPtr<A> Expected(NewObject<A>()); FString Error; if (!T.TestTrue(TEXT("Strict IR + direct source attestation"),MemoriaNarrativeImport::ReadIr(Ir(V),*Expected,Error))) { T.AddError(Error); return false; }
    for (TFieldIterator<FProperty> P(A::StaticClass(),EFieldIterationFlags::None);P;++P) T.TestTrue(*P->GetName(),P->Identical_InContainer(Actual,Expected.Get()));
    Eq(T,TEXT("Typed semantic fingerprint"),MemoriaNarrativeImport::Fingerprint(*Actual),Expected->ImportMetadata.SemanticSha256); return !T.HasAnyErrors();
}
}
#define NARRATIVE_TEST(Class,Name) IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class,Name,EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
NARRATIVE_TEST(FNarrativeVNParity,"Memoria.Narrative.VNImportParity")
bool FNarrativeVNParity::RunTest(const FString&) { return Parity<UMemoriaVNAsset>(*this,true); }
NARRATIVE_TEST(FNarrativeFieldParity,"Memoria.Narrative.FieldImportParity")
bool FNarrativeFieldParity::RunTest(const FString&) { return Parity<UMemoriaFieldAsset>(*this,false); }
NARRATIVE_TEST(FNarrativeReimport,"Memoria.Narrative.DeterministicReimport")
bool FNarrativeReimport::RunTest(const FString&)
{
    for (bool V:{false,true})
    {
        Obj R; FString Error; TestTrue(TEXT("Reload/check-only"),MemoriaNarrativeImport::Import(Ir(V),V,true,R,Error)); if (!R) { AddError(Error); return false; }
        Eq(*this,TEXT("No-op"),R->GetStringField(TEXT("result")),TEXT("UNCHANGED")); TestFalse(TEXT("No unnecessary save"),R->GetBoolField(TEXT("saved")));
    }
    return !HasAnyErrors();
}
NARRATIVE_TEST(FNarrativeFieldExecution,"Memoria.Narrative.FieldExecutionContract")
bool FNarrativeFieldExecution::RunTest(const FString&)
{ Replay(*this,TEXT("source_field_en")); Replay(*this,TEXT("source_field_ko")); Replay(*this,TEXT("synthetic_field_order_cost_jump")); return !HasAnyErrors(); }
NARRATIVE_TEST(FNarrativeVNExecution,"Memoria.Narrative.VNExecutionContract")
bool FNarrativeVNExecution::RunTest(const FString&)
{ for (int32 I=0;I<3;++I) Replay(*this,TEXT("source_vn_choice_")+FString::FromInt(I)); Replay(*this,TEXT("synthetic_vn_order_cost_jump")); return !HasAnyErrors(); }
NARRATIVE_TEST(FNarrativeChoiceIndices,"Memoria.Narrative.OriginalVisibleChoiceIndices")
bool FNarrativeChoiceIndices::RunTest(const FString&)
{ Replay(*this,TEXT("source_vn_failed_cost")); Replay(*this,TEXT("source_vn_filtered_original")); Replay(*this,TEXT("synthetic_field_order_cost_jump")); return !HasAnyErrors(); }
NARRATIVE_TEST(FNarrativeContinuation,"Memoria.Narrative.ContinuationDTO")
bool FNarrativeContinuation::RunTest(const FString&) { return Replay(*this,TEXT("source_vn_resume")); }
NARRATIVE_TEST(FNarrativeStrictValidation,"Memoria.Narrative.StrictValidationAndSemanticChange")
bool FNarrativeStrictValidation::RunTest(const FString&)
{
    TArray<FString> Files; IFileManager::Get().FindFiles(Files,*(Base()/TEXT("fixtures/narrative/reject_*.json")),true,false);
    TestEqual(TEXT("All negative mutations discovered"),Files.Num(),24);
    for (const auto& F:Files)
    {
        TStrongObjectPtr<UMemoriaVNAsset> A(NewObject<UMemoriaVNAsset>()); FString Error;
        TestFalse(*F,MemoriaNarrativeImport::ReadIr(Base()/TEXT("fixtures/narrative/")/F,*A,Error,true)); TestFalse(TEXT("Rejection diagnostic"),Error.IsEmpty());
    }
    const auto TempDir=FPaths::ProjectSavedDir()/TEXT("Validation/NarrativeTemporary"); IFileManager::Get().MakeDirectory(*TempDir,true);
    const auto P=TempDir/TEXT("modified_ir.json"); TestEqual(TEXT("Temporary IR copy outside production"),IFileManager::Get().Copy(*P,*(Base()/TEXT("fixtures/narrative/modified_semantic.json"))),COPY_OK);
    TStrongObjectPtr<UMemoriaVNAsset> Original(NewObject<UMemoriaVNAsset>()), Changed(NewObject<UMemoriaVNAsset>()); FString Error;
    TestTrue(TEXT("Original validates"),MemoriaNarrativeImport::ReadIr(Ir(true),*Original,Error)); TestTrue(TEXT("Temporary well-typed mutation"),MemoriaNarrativeImport::ReadIr(P,*Changed,Error,false));
    TestTrue(TEXT("Semantic modification detected from typed values"),MemoriaNarrativeImport::Fingerprint(*Original)!=MemoriaNarrativeImport::Fingerprint(*Changed));
    TestFalse(TEXT("Modified source payload cannot pass production attestation"),MemoriaNarrativeImport::ReadIr(P,*Changed,Error,true));
    TestTrue(TEXT("Mutation remains transient, no package"),Changed->GetOutermost()==GetTransientPackage());
    TStrongObjectPtr<UMemoriaFieldAsset> Wrong(NewObject<UMemoriaFieldAsset>()); TestFalse(TEXT("VN cannot enter field contract"),MemoriaNarrativeImport::ReadIr(Ir(true),*Wrong,Error));
    AddInfo(TEXT("No temporary Unreal package created or saved")); return !HasAnyErrors();
}
namespace
{
FMemoriaMemoryDefinition ProbeMemory(const TCHAR* Id,const TCHAR* Npc)
{ FMemoriaMemoryDefinition D; D.Id=Id; D.Title=Id; D.RelatedNpc=Npc; D.RawGrade=EMemoriaMemoryGrade::Grade3; D.BurnPower=30; return D; }
TStrongObjectPtr<UMemoriaPlayerMemoryDomain> ProbeDomain(FAutomationTestBase& T,const TArray<FMemoriaMemoryDefinition>& Definitions)
{
    TStrongObjectPtr<UMemoriaPlayerMemoryDomain> Memory(NewObject<UMemoriaPlayerMemoryDomain>()); FMemoriaMemorySnapshot S;
    for (const auto& D:Definitions) { FMemoriaMemoryState State; State.Id=D.Id; S.Owned.Add(State); }
    T.TestTrue(TEXT("Probe memory domain initializes"),Memory->Restore(Definitions,S)==EMemoriaMemoryResult::Success); return Memory;
}
}
NARRATIVE_TEST(FNarrativeBurnRunContext,"Memoria.Narrative.BurnUsesRunContext")
bool FNarrativeBurnRunContext::RunTest(const FString&)
{
    auto Memory=ProbeDomain(*this,{ProbeMemory(TEXT("sense_rain"),TEXT("")),ProbeMemory(TEXT("daily_bread"),TEXT(""))});
    FMemoriaRunSnapshot Run; Run.CurrentChapter=3;
    int64 Capacity=-1;
    const auto Handle=Memory->OnObserved.AddLambda([&Capacity](const FMemoriaMemoryEvent& E){ if (E.Kind==EMemoriaMemoryEventKind::CarryChanged) Capacity=E.CarryCapacity; });
    FMemoriaNarrativeContext Context(Run,*Memory);
    TestTrue(TEXT("Narrative burn succeeds"),Context.Burn(TEXT("sense_rain")));
    Memory->OnObserved.Remove(Handle);
    TestEqual(TEXT("Carry capacity follows the run chapter"),Capacity,Memory->GetCarryCapacity(3));
    TestTrue(TEXT("Not New Game's chapter-1 capacity"),Capacity!=Memory->GetCarryCapacity(1));
    return !HasAnyErrors();
}
NARRATIVE_TEST(FNarrativeStillHands,"Memoria.Narrative.StillHandsOath")
bool FNarrativeStillHands::RunTest(const FString&)
{
    auto Memory=ProbeDomain(*this,{ProbeMemory(TEXT("rel_elia_hand"),TEXT("Elia")),ProbeMemory(TEXT("rel_elia_song"),TEXT("Elia")),ProbeMemory(TEXT("sense_rain"),TEXT(""))});
    FMemoriaRunSnapshot Run; FMemoriaStoryFlag Sworn; Sworn.Id=TEXT("oath_still_sworn"); Sworn.bValue=true; Run.StoryFlags.Add(Sworn);
    FMemoriaNarrativeContext Context(Run,*Memory);
    // Source scene_flow step burn_memory never calls JourneyOath.on_player_burn.
    FMemoriaVNDefinition Sequence; Sequence.Id=TEXT("oath_probe");
    FMemoriaVNStep Step; Step.Effects.bHasBurnMemory=true; Step.Effects.BurnMemory=TEXT("rel_elia_hand"); Sequence.Steps.Add(Step);
    FMemoriaVNInterpreter VN(Sequence,Context); VN.Play();
    TestFalse(TEXT("Authored step burned the Elia memory"),Memory->IsIntact(TEXT("rel_elia_hand")));
    TestFalse(TEXT("Authored step burn keeps Still Hands"),Run.GetFlag(TEXT("oath_still_broken")));
    // Source dialogue_manager choice burn_memory / cost_memory call on_player_burn.
    FMemoriaFieldDefinition Field; Field.Id=TEXT("oath_probe");
    FMemoriaFieldChoice Rain; Rain.OriginalIndex=0; Rain.Effects.bHasBurnMemory=true; Rain.Effects.BurnMemory=TEXT("sense_rain");
    FMemoriaFieldChoice Song; Song.OriginalIndex=1; Song.Effects.bHasCostMemory=true; Song.Effects.CostMemory=TEXT("rel_elia_song");
    FMemoriaFieldRow Row; Row.bChoicesPresent=true; Row.Choices={Rain,Song}; Field.Rows={Row,Row};
    FMemoriaFieldInterpreter Interpreter(Field,Context); Interpreter.Start();
    Interpreter.SelectFilteredChoice(0);
    TestFalse(TEXT("Choosing to burn a memory not tied to Elia keeps the oath"),Run.GetFlag(TEXT("oath_still_broken")));
    Interpreter.SelectFilteredChoice(1);
    TestFalse(TEXT("Chosen cost burned the Elia memory"),Memory->IsIntact(TEXT("rel_elia_song")));
    TestTrue(TEXT("Choosing to burn an Elia memory breaks Still Hands"),Run.GetFlag(TEXT("oath_still_broken")));
    TestTrue(TEXT("Oath break is traced for the host"),Context.Events.Contains(TEXT("oath:broken:still")));
    TestFalse(TEXT("A broken oath no longer shields Elia memories"),Run.MemoryContext().bStillHandsActive);
    return !HasAnyErrors();
}
NARRATIVE_TEST(FNarrativeAddItem,"Memoria.Narrative.AddItemSourceRules")
bool FNarrativeAddItem::RunTest(const FString&)
{
    auto Memory=ProbeDomain(*this,{ProbeMemory(TEXT("sense_rain"),TEXT(""))});
    FMemoriaRunSnapshot Run;
    Run.Player.RecentItems={TEXT("potion"),TEXT("antidote"),TEXT("not_an_item"),TEXT("firebomb"),TEXT("smoke_bomb"),TEXT("witness_ink")};
    FMemoriaNarrativeContext Context(Run,*Memory);
    auto Count=[&](const TCHAR* Id){ const auto* I=Run.Player.Items.FindByPredicate([&](const auto& V){return V.Id.Equals(Id,ESearchCase::CaseSensitive);}); return I?I->Count:int64(0); };
    for (const TCHAR* Unknown : {TEXT("mystery_relic"),TEXT("Root_Balm")})
    {
        FMemoriaNarrativeEffects E; E.bHasAddItem=true; E.AddItem=Unknown; Context.Rewards(E,false);
        TestEqual(TEXT("Identity outside source ITEMS is ignored"),Count(Unknown),int64(0));
        TestTrue(TEXT("Ignored identity is traced"),Context.Events.Contains(FString(TEXT("item:ignored:"))+Unknown));
    }
    FMemoriaNarrativeEffects Balm; Balm.bHasAddItem=true; Balm.AddItem=TEXT("root_balm"); Balm.bHasAddItemCount=true; Balm.AddItemCount=2;
    Context.Rewards(Balm,true); Context.Rewards(Balm,false);
    TestEqual(TEXT("Known item accumulates"),Count(TEXT("root_balm")),int64(4));
    const TArray<FString> Expected={TEXT("root_balm"),TEXT("potion"),TEXT("antidote"),TEXT("firebomb"),TEXT("smoke_bomb")};
    TestEqual(TEXT("Recent items: front, unique, known, five"),FString::Join(Run.Player.RecentItems,TEXT(",")),FString::Join(Expected,TEXT(",")));
    TestTrue(TEXT("Inventory change is traced for the host"),Context.Events.Contains(TEXT("inventory_changed:root_balm")));
    return !HasAnyErrors();
}
#undef NARRATIVE_TEST
#endif
