#include "MemoriaPotionEvidence.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val=TSharedPtr<FJsonValue>;
TArray<Val> ReadCases(const TCHAR* File)
{
    FString S;Val V;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/malet_firebomb")/File));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V);return V?V->AsArray():TArray<Val>();
}
Obj Find(const TArray<Val>& Cases,const FString& Id){for(const auto& C:Cases)if(C->AsObject()->GetStringField(TEXT("id"))==Id)return C->AsObject();return nullptr;}
void Setup(UMemoriaRunSubsystem& Run)
{
    Run.BeginStartingMemoryRun();Run.BurnMemory(TEXT("daily_market_food"));Run.BurnMemory(TEXT("identity_first_sword"));Run.SetStoryFlag(TEXT("ch2_malet_done"),true);Run.GetWorldCognition()->SeedMaletRoute(true);
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FFirebombSource,"Memoria.Firebomb.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FFirebombSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const auto& C:ReadCases(TEXT("contract_inputs.v1.json"))){const auto Id=C->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}}
bool FFirebombSource::RunTest(const FString& Id)
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
        if((Id==TEXT("replacement_potion") && Item==TEXT("potion")) || (Id==TEXT("replacement_antidote") && Item==TEXT("antidote")) || (Id==TEXT("replacement_firebomb") && Item==TEXT("firebomb")))
        {auto New=Run->GetRunSnapshot();New.RunId=FGuid::NewGuid();New.Player.Items.Reset();New.Player.RecentItems.Reset();Run->RestoreRun(New,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());Observe(TEXT("replacement"),MakeShared<FJsonObject>());}
    });
    Run->OnItemToastRequested.AddLambda([&](const FString& Text,int32 Type)
    {++Toasts;auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("text"),Text);P->SetNumberField(TEXT("type"),Type);Observe(TEXT("toast"),P);});
    auto Query=[&](){auto O=MakeShared<FJsonObject>();TArray<Val> A;for(const auto& R:Run->GetRecentItems())A.Add(MakeShared<FJsonValueString>(R));O->SetArrayField(TEXT("recent"),A);return O;};
    auto ExpectedQuery=MakeShared<FJsonObject>();ExpectedQuery->SetArrayField(TEXT("recent"),Gold->GetArrayField(TEXT("before_query")));
    TestEqual(TEXT("Source recent query filtering"),Canon(Query()),Canon(ExpectedQuery));
    TestEqual(TEXT("Query cannot normalize raw stored recent"),Canon(Full(*Run)),Canon(Before));
    const FGuid Owner=Run->GetRunSnapshot().RunId;
    if(Id==TEXT("sequence") || Id.StartsWith(TEXT("replacement_")))
    {Run->AddRewardPotion(TEXT("potion"),2);if(Run->GetRunSnapshot().RunId==Owner)Run->AddRewardAntidote(TEXT("antidote"),1);}
    for(int32 I=0;I<Input->GetIntegerField(TEXT("repeats"));++I)if(Run->GetRunSnapshot().RunId==Owner)Run->AddRewardFirebomb(Input->GetStringField(TEXT("item")),Input->GetIntegerField(TEXT("amount")));
    const auto After=Full(*Run);auto ExpectedSteps=Gold->GetArrayField(TEXT("steps"));
    const bool Replaced=Id.StartsWith(TEXT("replacement_"));
    if(Replaced) { const int32 Cut=ExpectedSteps.IndexOfByPredicate([](const Val& V){return V->AsObject()->GetStringField(TEXT("point"))==TEXT("replacement");});ExpectedSteps.SetNum(Cut+1); } // Source continues stale toast/grants; native original-owner contract intentionally stops here.
    auto Actual=MakeShared<FJsonObject>(),Expected=MakeShared<FJsonObject>();Actual->SetArrayField(TEXT("steps"),Steps);Expected->SetArrayField(TEXT("steps"),ExpectedSteps);
    TestEqual(TEXT("Exact synchronous source states and signal/toast payload/order"),Canon(Actual),Canon(Expected));
    TestEqual(TEXT("Exact source final inventory/recent"),Canon(After->GetObjectField(TEXT("inventory"))),Replaced?Canon(ExpectedSteps.Last()->AsObject()->GetObjectField(TEXT("state"))):Canon(Gold->GetObjectField(TEXT("after"))));
    for(const TCHAR* K:{TEXT("player"),TEXT("player_observables"),TEXT("world")})TestEqual(FString(TEXT("Potion preserves "))+K,Canon(Before->GetObjectField(K)),Canon(After->GetObjectField(K)));
    auto ExpectedRun=S;ExpectedRun.Player.Items=Run->GetRunSnapshot().Player.Items;ExpectedRun.Player.RecentItems=Run->GetRunSnapshot().Player.RecentItems;if(Replaced)ExpectedRun.RunId=Run->GetRunSnapshot().RunId;
    TestEqual(TEXT("HP Grains chapter flags and unrelated run fields unchanged"),Canon(FJsonObjectConverter::UStructToJsonObject(ExpectedRun)),Canon(After->GetObjectField(TEXT("run"))));
    Actual->SetObjectField(TEXT("query_after"),Query());Actual->SetObjectField(TEXT("before"),Before);Actual->SetObjectField(TEXT("after"),After);Actual->SetNumberField(TEXT("signals"),Signals);Actual->SetNumberField(TEXT("toasts"),Toasts);Write(TEXT("firebomb_source_")+Id+TEXT(".json"),Actual);
    Run->OnRewardItemObserved.Clear();Run->OnInventoryChanged.Clear();Run->OnItemToastRequested.Clear();
    SaveRoundTrip(*this,*Run,TEXT("firebomb_save_")+Id+TEXT(".json"));Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirebombNative,"Memoria.Firebomb.NativeScopeAndPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirebombNative::RunTest(const FString&)
{
    using namespace MemoriaPotionEvidence;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    TestFalse(TEXT("Inactive firebomb rejected"),Run->AddRewardFirebomb(TEXT("firebomb"),1));Setup(*Run);
    TestTrue(TEXT("Invalid source distinguished"),Run->RewardItemScope(TEXT("FIREBOMB"))==EMemoriaRewardItemScope::InvalidSourceId);
    TestTrue(TEXT("Other source item remains deferred"),Run->RewardItemScope(TEXT("smoke_bomb"))==EMemoriaRewardItemScope::DeferredByPhase);
    for(const TCHAR* Id:{TEXT("potion"),TEXT("antidote"),TEXT("firebomb")})TestTrue(TEXT("Exactly approved source IDs"),Run->RewardItemScope(Id)==EMemoriaRewardItemScope::Supported);
    const auto Before=Full(*Run);
    TestFalse(TEXT("Potion wrapper strict"),Run->AddRewardPotion(TEXT("firebomb"),1));TestFalse(TEXT("Antidote wrapper strict"),Run->AddRewardAntidote(TEXT("firebomb"),1));
    TestFalse(TEXT("Firebomb wrapper strict"),Run->AddRewardFirebomb(TEXT("antidote"),1));TestEqual(TEXT("Rejected wrapper no mutation"),Canon(Full(*Run)),Canon(Before));
    TArray<FString> Requests;int32 Deliveries=0;
    // Telemetry receives generated requests; the removed presentation subscriber receives none.
    Run->OnItemToastRequested.AddLambda([&](const FString& Text,int32 Type){Requests.Add(Text);TestEqual(TEXT("SUCCESS request type"),Type,1);});
    const auto Presentation=Run->OnItemToastRequested.AddLambda([&](const FString&,int32){++Deliveries;});Run->OnItemToastRequested.Remove(Presentation);
    Run->AddRewardPotion(TEXT("potion"),2);Run->AddRewardAntidote(TEXT("antidote"),1);Run->AddRewardFirebomb(TEXT("firebomb"),1);
    TestEqual(TEXT("Three requests generated without presentation"),FString::Join(Requests,TEXT("|")),FString(TEXT("+2 Potion|+1 Antidote|+1 Firebomb")));
    TestEqual(TEXT("Removed presentation zero delivery"),Deliveries,0);
    TestTrue(TEXT("Explicit second call accumulates"),Run->AddRewardFirebomb(TEXT("firebomb"),1));TestEqual(TEXT("Twice is two"),Run->GetItemCount(TEXT("firebomb")),int64(2));
    auto Evidence=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Values;for(const auto& Text:Requests)Values.Add(MakeShared<FJsonValueString>(Text));Evidence->SetArrayField(TEXT("generated_requests"),Values);Evidence->SetNumberField(TEXT("presentation_deliveries"),Deliveries);Evidence->SetObjectField(TEXT("state"),Full(*Run));Write(TEXT("firebomb_presentation.json"),Evidence);
    Run->OnItemToastRequested.Clear();SaveRoundTrip(*this,*Run,TEXT("firebomb_native_save.json"));Game->Shutdown();return !HasAnyErrors();
}
#endif
