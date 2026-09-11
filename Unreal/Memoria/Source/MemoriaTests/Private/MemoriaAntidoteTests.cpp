#include "MemoriaPotionEvidence.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val=TSharedPtr<FJsonValue>;
TArray<Val> ReadCases(const TCHAR* File)
{
    FString S;Val V;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/malet_antidote")/File));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V);return V?V->AsArray():TArray<Val>();
}
Obj Find(const TArray<Val>& Cases,const FString& Id){for(const auto& C:Cases)if(C->AsObject()->GetStringField(TEXT("id"))==Id)return C->AsObject();return nullptr;}
void Setup(UMemoriaRunSubsystem& Run)
{
    Run.BeginStartingMemoryRun();Run.BurnMemory(TEXT("daily_market_food"));Run.BurnMemory(TEXT("identity_first_sword"));Run.SetStoryFlag(TEXT("ch2_malet_done"),true);Run.GetWorldCognition()->SeedMaletRoute(true);
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAntidoteSource,"Memoria.Antidote.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FAntidoteSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const auto& C:ReadCases(TEXT("contract_inputs.v1.json"))){const auto Id=C->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}}
bool FAntidoteSource::RunTest(const FString& Id)
{
    using namespace MemoriaPotionEvidence;
    const auto Input=Find(ReadCases(TEXT("contract_inputs.v1.json")),Id),Gold=Find(ReadCases(TEXT("contract_expected.v1.json")),Id);
    if(!TestTrue(TEXT("Executed source case"),Input.IsValid() && Gold.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();Setup(*Run);
    auto S=Run->GetRunSnapshot();S.Player.Items.Reset();S.Player.RecentItems.Reset();
    for(const auto& P:Input->GetObjectField(TEXT("items"))->Values){FMemoriaItemCount I;I.Id=P.Key;I.Count=P.Value->AsNumber();S.Player.Items.Add(I);}
    for(const auto& V:Input->GetArrayField(TEXT("recent")))S.Player.RecentItems.Add(V->AsString());
    S.CurrentLocale=Id==TEXT("ko")?TEXT("ko"):TEXT("en");Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());
    if(Id==TEXT("existing")) {auto W=Run->GetWorldCognition()->GetSnapshot();W.Revision=19;W.EventSequence=23;TestTrue(TEXT("Explicit supplemental nondefault world fixture"),Run->GetWorldCognition()->Restore(W));}
    const auto Before=Full(*Run);TArray<Val> Steps;int32 Signals=0,Toasts=0;
    auto Observe=[&](const FString& Point,const Obj& Payload)
    {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("point"),Point);O->SetObjectField(TEXT("payload"),Payload);O->SetObjectField(TEXT("state"),Inventory(Run->GetRunSnapshot()));Steps.Add(MakeShared<FJsonValueObject>(O));};
    Run->OnRewardItemObserved.AddLambda([&](const FString&,const FString& Point,const FMemoriaRunSnapshot& Snap)
    {
        TestEqual(TEXT("Read-only observer sees exact committed run"),Canon(FJsonObjectConverter::UStructToJsonObject(Snap)),Canon(FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot())));
        if(Point==TEXT("after_inventory_mutation") || Point==TEXT("after_recent_items"))Observe(Point,MakeShared<FJsonObject>());
    });
    Run->OnInventoryChanged.AddLambda([&](const FString& Item)
    {
        ++Signals;auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("item_id"),Item);Observe(TEXT("inventory_changed"),P);
        if((Id==TEXT("replacement_potion") && Item==TEXT("potion")) || (Id==TEXT("replacement_antidote") && Item==TEXT("antidote")))
        {auto New=Run->GetRunSnapshot();New.RunId=FGuid::NewGuid();New.Player.Items.Reset();New.Player.RecentItems.Reset();Run->RestoreRun(New,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());Observe(TEXT("replacement"),MakeShared<FJsonObject>());}
    });
    Run->OnItemToastRequested.AddLambda([&](const FString& Text,int32 Type)
    {++Toasts;auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("text"),Text);P->SetNumberField(TEXT("type"),Type);Observe(TEXT("toast"),P);});
    auto Query=[&](){auto O=MakeShared<FJsonObject>();TArray<Val> A;for(const auto& R:Run->GetRecentItems())A.Add(MakeShared<FJsonValueString>(R));O->SetArrayField(TEXT("recent"),A);return O;};
    auto ExpectedQuery=MakeShared<FJsonObject>();ExpectedQuery->SetArrayField(TEXT("recent"),Gold->GetArrayField(TEXT("before_query")));
    TestEqual(TEXT("Source recent query filtering"),Canon(Query()),Canon(ExpectedQuery));
    TestEqual(TEXT("Query cannot normalize raw stored recent"),Canon(Full(*Run)),Canon(Before));
    const FGuid Owner=Run->GetRunSnapshot().RunId;
    if(Id==TEXT("sequence") || Id.StartsWith(TEXT("replacement_")))Run->AddRewardPotion(TEXT("potion"),2);
    for(int32 I=0;I<Input->GetIntegerField(TEXT("repeats"));++I)if(Run->GetRunSnapshot().RunId==Owner)Run->AddRewardAntidote(Input->GetStringField(TEXT("item")),Input->GetIntegerField(TEXT("amount")));
    const auto After=Full(*Run);auto ExpectedSteps=Gold->GetArrayField(TEXT("steps"));
    const bool Replaced=Id.StartsWith(TEXT("replacement_"));
    if(Replaced) { const int32 Cut=ExpectedSteps.IndexOfByPredicate([](const Val& V){return V->AsObject()->GetStringField(TEXT("point"))==TEXT("replacement");});ExpectedSteps.SetNum(Cut+1); } // Source continues stale toast/grants; native original-owner contract intentionally stops here.
    auto Actual=MakeShared<FJsonObject>(),Expected=MakeShared<FJsonObject>();Actual->SetArrayField(TEXT("steps"),Steps);Expected->SetArrayField(TEXT("steps"),ExpectedSteps);
    TestEqual(TEXT("Exact synchronous source states and signal/toast payload/order"),Canon(Actual),Canon(Expected));
    TestEqual(TEXT("Exact source final inventory/recent"),Canon(After->GetObjectField(TEXT("inventory"))),Replaced?Canon(ExpectedSteps.Last()->AsObject()->GetObjectField(TEXT("state"))):Canon(Gold->GetObjectField(TEXT("after"))));
    for(const TCHAR* K:{TEXT("player"),TEXT("player_observables"),TEXT("world")})TestEqual(FString(TEXT("Potion preserves "))+K,Canon(Before->GetObjectField(K)),Canon(After->GetObjectField(K)));
    auto ExpectedRun=S;ExpectedRun.Player.Items=Run->GetRunSnapshot().Player.Items;ExpectedRun.Player.RecentItems=Run->GetRunSnapshot().Player.RecentItems;if(Replaced)ExpectedRun.RunId=Run->GetRunSnapshot().RunId;
    TestEqual(TEXT("HP Grains chapter flags and unrelated run fields unchanged"),Canon(FJsonObjectConverter::UStructToJsonObject(ExpectedRun)),Canon(After->GetObjectField(TEXT("run"))));
    Actual->SetObjectField(TEXT("query_after"),Query());Actual->SetObjectField(TEXT("before"),Before);Actual->SetObjectField(TEXT("after"),After);Actual->SetNumberField(TEXT("signals"),Signals);Actual->SetNumberField(TEXT("toasts"),Toasts);Write(TEXT("antidote_source_")+Id+TEXT(".json"),Actual);
    Run->OnRewardItemObserved.Clear();Run->OnInventoryChanged.Clear();Run->OnItemToastRequested.Clear();
    SaveRoundTrip(*this,*Run,TEXT("antidote_save_")+Id+TEXT(".json"));Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntidoteNative,"Memoria.Antidote.NativeScopeAndPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAntidoteNative::RunTest(const FString&)
{
    using namespace MemoriaPotionEvidence;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    TestFalse(TEXT("Inactive authority rejects antidote"),Run->AddRewardAntidote(TEXT("antidote"),1));Setup(*Run);
    TestTrue(TEXT("Unknown source ID distinguished"),Run->RewardItemScope(TEXT("bad"))==EMemoriaRewardItemScope::InvalidSourceId);
    TestTrue(TEXT("Valid source ID deferred by phase"),Run->RewardItemScope(TEXT("firebomb"))==EMemoriaRewardItemScope::DeferredByPhase);
    const auto Before=Full(*Run);TestFalse(TEXT("No firebomb entry"),Run->AddRewardAntidote(TEXT("firebomb"),1));TestEqual(TEXT("No side effects outside scope"),Canon(Full(*Run)),Canon(Before));
    TestTrue(TEXT("No presentation subscribers potion"),Run->AddRewardPotion(TEXT("potion"),2));
    TestTrue(TEXT("No presentation subscribers antidote"),Run->AddRewardAntidote(TEXT("antidote"),1));
    int32 Toasts=0;const auto Handle=Run->OnItemToastRequested.AddLambda([&](const FString&,int32){++Toasts;});Run->OnItemToastRequested.Remove(Handle);
    TestTrue(TEXT("Removed presentation does not prevent accumulation"),Run->AddRewardAntidote(TEXT("antidote"),1));TestEqual(TEXT("Absent subscriber has no deliveries"),Toasts,0);TestEqual(TEXT("Explicit repeat accumulates"),Run->GetItemCount(TEXT("antidote")),int64(2));
    // Preserve an independently nondefault world; grant cannot reset its revision.
    const auto World=Run->GetWorldCognition()->ExportJson();const auto Player=Canon(MemoriaPlayerObservation(*Run));
    auto S=Run->GetRunSnapshot();S.Player.Items[1].Count=MAX_int64;Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());
    TestTrue(TEXT("Defined signed-wrap grant"),Run->AddRewardAntidote(TEXT("antidote"),1));TestEqual(TEXT("Native signed overflow preserved without UB"),Run->GetItemCount(TEXT("antidote")),MIN_int64);
    TestEqual(TEXT("Overflow does not touch world"),Run->GetWorldCognition()->ExportJson(),World);TestEqual(TEXT("Overflow does not touch player"),Canon(MemoriaPlayerObservation(*Run)),Player);
    // Binary evidence uses ordinary counts to avoid JSON floating-point loss of int64 extrema.
    Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());Run->AddRewardAntidote(TEXT("antidote"),-MAX_int64);
    SaveRoundTrip(*this,*Run,TEXT("antidote_native_save.json"));Game->Shutdown();return !HasAnyErrors();
}
#endif
