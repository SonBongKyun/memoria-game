#include "MemoriaPotionEvidence.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val=TSharedPtr<FJsonValue>;
TArray<Val> ReadCases(const TCHAR* File)
{
    FString S;Val V;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/malet_potion")/File));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V);return V?V->AsArray():TArray<Val>();
}
Obj Find(const TArray<Val>& Cases,const FString& Id){for(const auto& C:Cases)if(C->AsObject()->GetStringField(TEXT("id"))==Id)return C->AsObject();return nullptr;}
void Setup(UMemoriaRunSubsystem& Run)
{
    Run.BeginStartingMemoryRun();Run.BurnMemory(TEXT("daily_market_food"));Run.BurnMemory(TEXT("identity_first_sword"));Run.SetStoryFlag(TEXT("ch2_malet_done"),true);Run.GetWorldCognition()->SeedMaletRoute(true);
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FPotionSource,"Memoria.Potion.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FPotionSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const auto& C:ReadCases(TEXT("contract_inputs.v1.json"))){const auto Id=C->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}}
bool FPotionSource::RunTest(const FString& Id)
{
    using namespace MemoriaPotionEvidence;
    const auto Input=Find(ReadCases(TEXT("contract_inputs.v1.json")),Id),Gold=Find(ReadCases(TEXT("contract_expected.v1.json")),Id);
    if(!TestTrue(TEXT("Executed source case"),Input.IsValid() && Gold.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();Setup(*Run);
    auto S=Run->GetRunSnapshot();S.Player.Items.Reset();S.Player.RecentItems.Reset();
    for(const auto& P:Input->GetObjectField(TEXT("items"))->Values){FMemoriaItemCount I;I.Id=P.Key;I.Count=P.Value->AsNumber();S.Player.Items.Add(I);}
    for(const auto& V:Input->GetArrayField(TEXT("recent")))S.Player.RecentItems.Add(V->AsString());
    S.CurrentLocale=Id==TEXT("ko")?TEXT("ko"):TEXT("en");Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());
    const auto Before=Full(*Run);TArray<Val> Steps;int32 Signals=0,Toasts=0;
    auto Observe=[&](const FString& Point,const Obj& Payload)
    {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("point"),Point);O->SetObjectField(TEXT("payload"),Payload);O->SetObjectField(TEXT("state"),Inventory(Run->GetRunSnapshot()));Steps.Add(MakeShared<FJsonValueObject>(O));};
    Run->OnPotionObserved.AddLambda([&](const FString& Point,const FMemoriaRunSnapshot& Snap)
    {
        TestEqual(TEXT("Read-only observer sees exact committed run"),Canon(FJsonObjectConverter::UStructToJsonObject(Snap)),Canon(FJsonObjectConverter::UStructToJsonObject(Run->GetRunSnapshot())));
        if(Point==TEXT("after_inventory_mutation") || Point==TEXT("after_recent_items"))Observe(Point,MakeShared<FJsonObject>());
    });
    Run->OnInventoryChanged.AddLambda([&](const FString& Item)
    {
        ++Signals;auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("item_id"),Item);Observe(TEXT("inventory_changed"),P);
        if(Id==TEXT("replacement_signal"))
        {auto New=Run->GetRunSnapshot();New.RunId=FGuid::NewGuid();New.Player.Items.Reset();New.Player.RecentItems.Reset();Run->RestoreRun(New,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot(),Run->GetWorldCognition()->GetSnapshot());Observe(TEXT("replacement"),MakeShared<FJsonObject>());}
    });
    Run->OnItemToastRequested.AddLambda([&](const FString& Text,int32 Type)
    {++Toasts;auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("text"),Text);P->SetNumberField(TEXT("type"),Type);Observe(TEXT("toast"),P);});
    for(int32 I=0;I<Input->GetIntegerField(TEXT("repeats"));++I)Run->AddRewardPotion(Input->GetStringField(TEXT("item")),Input->GetIntegerField(TEXT("amount")));
    const auto After=Full(*Run);auto ExpectedSteps=Gold->GetArrayField(TEXT("steps"));
    if(Id==TEXT("replacement_signal"))ExpectedSteps.Pop(); // Explicit accepted owner-isolation difference: suppress old toast.
    auto Actual=MakeShared<FJsonObject>(),Expected=MakeShared<FJsonObject>();Actual->SetArrayField(TEXT("steps"),Steps);Expected->SetArrayField(TEXT("steps"),ExpectedSteps);
    TestEqual(TEXT("Exact synchronous source states and signal/toast payload/order"),Canon(Actual),Canon(Expected));
    TestEqual(TEXT("Exact source final inventory/recent"),Canon(After->GetObjectField(TEXT("inventory"))),Canon(Gold->GetObjectField(TEXT("after"))));
    for(const TCHAR* K:{TEXT("player"),TEXT("player_observables"),TEXT("world")})TestEqual(FString(TEXT("Potion preserves "))+K,Canon(Before->GetObjectField(K)),Canon(After->GetObjectField(K)));
    auto ExpectedRun=S;ExpectedRun.Player.Items=Run->GetRunSnapshot().Player.Items;ExpectedRun.Player.RecentItems=Run->GetRunSnapshot().Player.RecentItems;if(Id==TEXT("replacement_signal"))ExpectedRun.RunId=Run->GetRunSnapshot().RunId;
    TestEqual(TEXT("HP Grains chapter flags and unrelated run fields unchanged"),Canon(FJsonObjectConverter::UStructToJsonObject(ExpectedRun)),Canon(After->GetObjectField(TEXT("run"))));
    Actual->SetObjectField(TEXT("before"),Before);Actual->SetObjectField(TEXT("after"),After);Actual->SetNumberField(TEXT("signals"),Signals);Actual->SetNumberField(TEXT("toasts"),Toasts);Write(TEXT("potion_source_")+Id+TEXT(".json"),Actual);
    Run->OnPotionObserved.Clear();Run->OnInventoryChanged.Clear();Run->OnItemToastRequested.Clear();
    SaveRoundTrip(*this,*Run,TEXT("potion_save_")+Id+TEXT(".json"));Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPotionNative,"Memoria.Potion.NativeInventoryContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPotionNative::RunTest(const FString&)
{
    using namespace MemoriaPotionEvidence;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    TestFalse(TEXT("No active authority"),Run->AddRewardPotion(TEXT("potion"),2));Setup(*Run);
    const auto Before=Full(*Run);TestFalse(TEXT("Antidote command outside authorized API"),Run->AddRewardPotion(TEXT("antidote"),1));TestFalse(TEXT("Firebomb command outside authorized API"),Run->AddRewardPotion(TEXT("firebomb"),1));TestEqual(TEXT("Unsupported grants change no state"),Canon(Full(*Run)),Canon(Before));
    // No presentation endpoint/subscribers at all; committed authority survives it.
    TestTrue(TEXT("Synchronous grant with absent presentation"),Run->AddRewardPotion(TEXT("potion"),2));TestEqual(TEXT("Real count with no UI"),Run->GetItemCount(TEXT("potion")),int64(2));
    TestEqual(TEXT("Case-sensitive count"),Run->GetItemCount(TEXT("POTION")),int64(0));SaveRoundTrip(*this,*Run,TEXT("potion_native_save.json"));Game->Shutdown();return !HasAnyErrors();
}
#endif
