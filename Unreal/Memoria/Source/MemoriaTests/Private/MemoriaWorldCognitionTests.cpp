#include "MemoriaPlayerObservation.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "World/MemoriaWorldCognition.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using Obj=TSharedPtr<FJsonObject>;using Val=TSharedPtr<FJsonValue>;
Obj Parse(const FString& S){Obj O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O);return O;}
FString Canon(const Obj& O){return MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(O));}
FString Canon(const FString& S){return Canon(Parse(S));}
template<class T> FString Struct(const T& S){return Canon(FJsonObjectConverter::UStructToJsonObject(S));}
TArray<Val> Cases()
{
    FString S;Val O;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/malet_world_seed/contract_expected.v1.json")));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O);return O?O->AsArray():TArray<Val>();
}
void SaveEvidence(const FString& Name,const Obj& O)
{
    const auto Dir=FPaths::ProjectSavedDir()/TEXT("Validation/Phase1N");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(Canon(O)+TEXT("\n"),*(Dir/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FWorldSourceCases,"Memoria.WorldSeed.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FWorldSourceCases::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const TCHAR* N:{TEXT("fresh"),TEXT("knowledge_only"),TEXT("memory_only"),TEXT("both"),TEXT("removed"),TEXT("forgotten"),TEXT("actor_missing"),TEXT("repeat"),TEXT("already_true"),TEXT("save_restore"),TEXT("restored"),TEXT("flag_false"),TEXT("case_sensitive")}){Names.Add(N);Commands.Add(N);}}
bool FWorldSourceCases::RunTest(const FString& Id)
{
    Obj Expected;for(const auto& C:Cases())if(C->AsObject()->GetStringField(TEXT("id"))==Id)Expected=C->AsObject();
    if(!TestTrue(TEXT("Real source fixture exists"),Expected.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    Run->BeginStartingMemoryRun();Run->BurnMemory(TEXT("daily_market_food"));Run->BurnMemory(TEXT("identity_first_sword"));
    FMemoriaWorldSnapshot Before;
    TestTrue(TEXT("Source world decodes into typed DTO"),UMemoriaWorldCognition::Decode(Canon(Expected->GetObjectField(TEXT("before"))),Before));
    TestTrue(TEXT("Supplemental fixture restored through real run owner"),Run->RestoreRun(Run->GetRunSnapshot(),Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Before)==EMemoriaMemoryResult::Success);
    auto* World=Run->GetWorldCognition();const auto RunBefore=Struct(Run->GetRunSnapshot()),MemoryBefore=Struct(Run->GetPlayerMemory()->GetSnapshot()),DerivedBefore=Canon(MemoriaPlayerObservation(*Run));
    TArray<Val> Events,Steps;
    World->OnCommitted.AddLambda([&](const FMemoriaWorldEvent& E,const FMemoriaWorldSnapshot& S)
    {
        Events.Add(MakeShared<FJsonValueObject>(Parse(E.ToJson())));
        Obj Step=MakeShared<FJsonObject>();Step->SetObjectField(TEXT("event"),Parse(E.ToJson()));Step->SetObjectField(TEXT("world"),Parse(UMemoriaWorldCognition::Encode(S)));Steps.Add(MakeShared<FJsonValueObject>(Step));
        TestEqual(TEXT("Observer sees fully committed state"),Canon(World->ExportJson()),Canon(UMemoriaWorldCognition::Encode(S)));
    });
    World->SeedMaletRoute(Expected->GetBoolField(TEXT("done")));
    TestEqual(TEXT("Exact full source world after seed"),Canon(World->ExportJson()),Canon(Expected->GetObjectField(TEXT("after"))));
    Obj Actual=MakeShared<FJsonObject>(),Gold=MakeShared<FJsonObject>();Actual->SetArrayField(TEXT("events"),Events);Gold->SetArrayField(TEXT("events"),Expected->GetArrayField(TEXT("events")));
    Actual->SetArrayField(TEXT("steps"),Steps);Gold->SetArrayField(TEXT("steps"),Expected->GetArrayField(TEXT("steps")));
    TestEqual(TEXT("Exact event payload, sequence, revision and synchronous intermediate states"),Canon(Actual),Canon(Gold));
    const auto Count=Events.Num();World->SeedMaletRoute(Expected->GetBoolField(TEXT("done")));
    TestEqual(TEXT("Repeat allocates no events"),Events.Num(),Count);
    TestEqual(TEXT("Repeat exact source world"),Canon(World->ExportJson()),Canon(Expected->GetObjectField(TEXT("after_repeat"))));
    TestEqual(TEXT("Actual derived player connections/power/carry/definitions unchanged"),Canon(MemoriaPlayerObservation(*Run)),DerivedBefore);
    TestEqual(TEXT("Player full snapshot never changes during world mutation"),Struct(Run->GetPlayerMemory()->GetSnapshot()),MemoryBefore);
    TestEqual(TEXT("Run/inventory/flags independent"),Struct(Run->GetRunSnapshot()),RunBefore);
    Actual->SetStringField(TEXT("id"),Id);Actual->SetObjectField(TEXT("before"),Parse(UMemoriaWorldCognition::Encode(Before)));Actual->SetObjectField(TEXT("after"),Parse(World->ExportJson()));Actual->SetBoolField(TEXT("player_observables_unchanged"),Canon(MemoriaPlayerObservation(*Run))==DerivedBefore);Actual->SetBoolField(TEXT("player_unchanged"),Struct(Run->GetPlayerMemory()->GetSnapshot())==MemoryBefore);Actual->SetBoolField(TEXT("run_unchanged"),Struct(Run->GetRunSnapshot())==RunBefore);SaveEvidence(TEXT("source_")+Id+TEXT(".json"),Actual);
    World->OnCommitted.Clear();Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldNative,"Memoria.WorldSeed.NativeContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldNative::RunTest(const FString&)
{
    TStrongObjectPtr<UMemoriaWorldCognition> W(NewObject<UMemoriaWorldCognition>());auto S=UMemoriaWorldCognition::Defaults();S.Revision=7;S.EventSequence=3;TestTrue(TEXT("Native restore"),W->Restore(S));
    const auto Before=W->ExportJson();TestTrue(TEXT("Read default actor"),W->HasActor(TEXT("npc.malet")));TestFalse(TEXT("Missing read"),W->HasMemory(TEXT("npc.malet"),MemoriaWorldIds::RouteMemory));
    TestFalse(TEXT("Invalid mutation fails"),W->LearnFact(TEXT("NPC.MALET"),MemoriaWorldIds::RouteFact));TestEqual(TEXT("Reads/failures do not mutate"),W->ExportJson(),Before);
    TArray<int64> Revisions,Sequences;W->OnCommitted.AddLambda([&](const FMemoriaWorldEvent& E,const FMemoriaWorldSnapshot&){Revisions.Add(E.Revision);Sequences.Add(E.Sequence);TestFalse(TEXT("Reentrant state replacement rejected"),W->Restore(S));});
    W->SeedMaletRoute(true);TestTrue(TEXT("Independent persistent revision allocator"),Revisions==TArray<int64>({8,9}));TestTrue(TEXT("Independent persistent event sequence allocator"),Sequences==TArray<int64>({4,5}));
    const auto After=W->ExportJson();TestFalse(TEXT("Duplicate fact no-op"),W->LearnFact(MemoriaWorldIds::Malet,MemoriaWorldIds::RouteFact));W->SeedMaletRoute(true);TestEqual(TEXT("No-op state exact"),W->ExportJson(),After);
    auto Invalid=W->GetSnapshot();const auto DuplicateActor=Invalid.Actors[0];Invalid.Actors.Add(DuplicateActor);TestFalse(TEXT("Typed duplicate rejected before JSON collapse"),W->Restore(Invalid));Invalid=W->GetSnapshot();Invalid.WorldFlagsJson=TEXT("broken");TestFalse(TEXT("Malformed extension rejected atomically"),W->Restore(Invalid));TestEqual(TEXT("Invalid restore leaves state exact"),W->ExportJson(),After);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldIds,"Memoria.WorldSeed.IdentityContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldIds::RunTest(const FString&)
{
    TestEqual(TEXT("Source actor"),FString(MemoriaWorldIds::Malet),FString(TEXT("npc.malet")));TestEqual(TEXT("Canonical source spelling"),FString(MemoriaWorldIds::Arrel),FString(TEXT("player.arrel")));
    TestEqual(TEXT("Identity-free route fact"),FString(MemoriaWorldIds::RouteFact),FString(TEXT("fact.bl07.route_request_received")));TestEqual(TEXT("Identity-bearing source memory"),FString(MemoriaWorldIds::RouteMemory),FString(TEXT("memory.malet.bl07_request_source")));
    for(const TCHAR* Id:{TEXT("NPC.MALET"),TEXT("npc.Malet"),TEXT("player.arel"),TEXT("npc.arrel")})TestFalse(TEXT("Case-sensitive actor catalog"),MemoriaWorldIds::IsActor(Id));
    for(const TCHAR* Id:{TEXT("Fact.bl07.route_request_received"),TEXT("fact.BL07.route_request_received"),TEXT("fact.bl07.route__request"),TEXT("fact.bl07.route_")})TestFalse(TEXT("Source fact grammar"),MemoriaWorldIds::IsFact(Id));
    TestFalse(TEXT("Different owner rejected"),MemoriaWorldIds::IsOwnedMemory(MemoriaWorldIds::RouteMemory,TEXT("npc.sable")));return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSave,"Memoria.WorldSeed.SaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldSave::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();Run->BeginStartingMemoryRun();Run->BurnMemory(TEXT("daily_market_food"));Run->BurnMemory(TEXT("identity_first_sword"));Run->SetStoryFlag(TEXT("ch2_malet_done"),true);Run->GetWorldCognition()->SeedMaletRoute(true);
    const auto BeforeDerived=MemoriaPlayerObservation(*Run);
    auto Before=Run->CaptureSave();TStrongObjectPtr<UMemoriaRunSaveGame> Saved(Before);TArray<uint8> Bytes;TestTrue(TEXT("Actual SaveGameToMemory"),UGameplayStatics::SaveGameToMemory(Saved.Get(),Bytes));
    TStrongObjectPtr<UMemoriaRunSaveGame> Loaded(Cast<UMemoriaRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
    if(!TestNotNull(TEXT("Actual binary SaveGame loaded"),Loaded.Get())){Game->Shutdown();return false;}
    Run->BeginStartingMemoryRun();TestTrue(TEXT("All three sections restored through production API"),Run->RestoreSave(*Loaded));
    TestEqual(TEXT("Run roundtrip exact"),Struct(Run->GetRunSnapshot()),Struct(Before->Run));TestEqual(TEXT("Player roundtrip exact"),Struct(Run->GetPlayerMemory()->GetSnapshot()),Struct(Before->PlayerMemory));TestEqual(TEXT("World roundtrip exact"),Canon(Run->GetWorldCognition()->ExportJson()),Canon(Before->WorldCognition.SourceJson));
    TestEqual(TEXT("Derived player state recomputed exactly on binary restore"),Canon(MemoriaPlayerObservation(*Run)),Canon(BeforeDerived));
    int32 Events=0;Run->GetWorldCognition()->OnCommitted.AddLambda([&](const auto&,const auto&){++Events;});Run->GetWorldCognition()->SeedMaletRoute(true);TestEqual(TEXT("Post-restore seed no-op"),Events,0);
    Obj E=MakeShared<FJsonObject>();E->SetNumberField(TEXT("save_schema"),Loaded->SchemaVersion);E->SetNumberField(TEXT("world_schema"),Loaded->WorldCognition.SchemaVersion);E->SetNumberField(TEXT("binary_bytes"),Bytes.Num());E->SetObjectField(TEXT("before_world"),Parse(Before->WorldCognition.SourceJson));E->SetObjectField(TEXT("after_world"),Parse(Run->GetWorldCognition()->ExportJson()));E->SetObjectField(TEXT("before_run"),FJsonObjectConverter::UStructToJsonObject(Before->Run));E->SetObjectField(TEXT("after_run"),FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot()));E->SetObjectField(TEXT("before_player"),FJsonObjectConverter::UStructToJsonObject(Before->PlayerMemory));E->SetObjectField(TEXT("after_player"),FJsonObjectConverter::UStructToJsonObject(Run->GetPlayerMemory()->GetSnapshot()));E->SetObjectField(TEXT("before_player_observables"),BeforeDerived);E->SetObjectField(TEXT("after_player_observables"),MemoriaPlayerObservation(*Run));E->SetNumberField(TEXT("post_restore_events"),Events);SaveEvidence(TEXT("world_save_roundtrip.json"),E);
    Loaded->WorldCognition.SourceJson=TEXT("{\"schema_version\":99}");const auto PriorRun=Struct(Run->GetRunSnapshot()),PriorPlayer=Struct(Run->GetPlayerMemory()->GetSnapshot()),PriorWorld=Run->GetWorldCognition()->ExportJson();
    TestFalse(TEXT("Unsupported world restore fails atomically"),Run->RestoreSave(*Loaded));TestEqual(TEXT("No partial run replacement"),Struct(Run->GetRunSnapshot()),PriorRun);TestEqual(TEXT("No partial player replacement"),Struct(Run->GetPlayerMemory()->GetSnapshot()),PriorPlayer);TestEqual(TEXT("No partial world replacement"),Run->GetWorldCognition()->ExportJson(),PriorWorld);
    Loaded->WorldCognition.SourceJson.Reset();TestTrue(TEXT("Phase1A-J absent world section accepted"),Run->RestoreSave(*Loaded));TestEqual(TEXT("Absent section gets source defaults"),Canon(Run->GetWorldCognition()->ExportJson()),Canon(UMemoriaWorldCognition::Encode(UMemoriaWorldCognition::Defaults())));
    Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldLifetime,"Memoria.WorldSeed.RunReplacementIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldLifetime::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();Run->BeginStartingMemoryRun();Run->GetWorldCognition()->SeedMaletRoute(true);
    TStrongObjectPtr<UMemoriaWorldCognition> Old(Run->GetWorldCognition());const auto Before=Old->ExportJson();const auto Id=Run->GetRunSnapshot().RunId;
    Run->BeginStartingMemoryRun();TestTrue(TEXT("New run identity"),Id!=Run->GetRunSnapshot().RunId);TestTrue(TEXT("New owned cognition instance"),Old.Get()!=Run->GetWorldCognition());TestEqual(TEXT("New run only defaults"),Canon(Run->GetWorldCognition()->ExportJson()),Canon(UMemoriaWorldCognition::Encode(UMemoriaWorldCognition::Defaults())));TestEqual(TEXT("Old committed world not rolled back"),Old->ExportJson(),Before);
    Old->LearnFact(MemoriaWorldIds::Malet,TEXT("fact.test.detached"));TestFalse(TEXT("Detached old owner cannot leak"),Run->GetWorldCognition()->HasKnowledge(MemoriaWorldIds::Malet,TEXT("fact.test.detached")));Game->Shutdown();return !HasAnyErrors();
}
#endif
