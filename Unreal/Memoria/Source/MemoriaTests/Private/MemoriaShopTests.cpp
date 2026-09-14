#include "MemoriaShopEvidence.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val=TSharedPtr<FJsonValue>;
TArray<Val> ShopCases(const TCHAR* Name)
{
    FString S;Val V;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/malet_shop")/Name));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V);return V?V->AsArray():TArray<Val>();
}
Obj Case(const TArray<Val>& Cases,const FString& Id)
{for(const auto& C:Cases)if(C->AsObject()->GetStringField(TEXT("id"))==Id)return C->AsObject();return nullptr;}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FShopSource,"Memoria.Shop.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FShopSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const auto& C:ShopCases(TEXT("contract_inputs.v1.json"))){const auto Id=C->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}}
bool FShopSource::RunTest(const FString& Id)
{
    const auto Input=Case(ShopCases(TEXT("contract_inputs.v1.json")),Id),Gold=Case(ShopCases(TEXT("contract_expected.v1.json")),Id);
    if(!TestTrue(TEXT("Executed source case exists"),Input.IsValid()&&Gold.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* Shop=Game->GetSubsystem<UMemoriaShopSubsystem>();
    TestTrue(TEXT("Starting source catalog"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success);
    for(const auto& B:Input->GetArrayField(TEXT("burn")))Run->BurnMemory(B->AsString());
    auto S=Run->GetRunSnapshot();S.CurrentLocale=Input->GetStringField(TEXT("locale"));S.Player.Grains=Input->GetIntegerField(TEXT("grains"));
    auto M=Run->GetPlayerMemory()->GetSnapshot();
    if(Id==TEXT("empty"))M.Owned.Reset();
    if(Id==TEXT("core_only"))M.Owned.RemoveAll([](const auto& R){return R.Id!=TEXT("core_name_origin");});
    if(Id==TEXT("faded"))M.Owned.FindByPredicate([](const auto& R){return R.Id==TEXT("sense_forest_smell");})->bFaded=true;
    if(Id==TEXT("collateral")){M.ActiveLoan.bActive=true;M.ActiveLoan.MemoryId=TEXT("sense_forest_smell");M.ActiveLoan.Principal=5;M.ActiveLoan.Repay=6;M.ActiveLoan.DueChapter=2;}
    if(Id==TEXT("all_burned"))for(auto& R:M.Owned){R.bBurned=true;M.BurnedHistory.AddUnique(R.Id);}
    // Native snapshots store one definition per owned memory. Source removal must
    // remove both sides of this established invariant; do not weaken RestoreRun.
    auto Definitions=Run->GetPlayerMemory()->GetDefinitions();
    Definitions.RemoveAll([&](const auto& D){return !M.Owned.ContainsByPredicate([&](const auto& R){return R.Id==D.Id;});});
    if(!TestTrue(TEXT("Explicit source input restored"),Run->RestoreRun(S,Definitions,M,Run->GetWorldCognition()->GetSnapshot())==EMemoriaMemoryResult::Success))
    {Game->Shutdown();return false;}
    const auto Before=Full(*Run);TestTrue(TEXT("Open source shop"),Shop->OpenMalet());
    const auto First=MemoriaShopEvidence::View(Shop->GetView());
    if(Id==TEXT("already_open"))TestFalse(TEXT("Duplicate opening is ignored"),Shop->OpenMalet());
    const auto After=MemoriaShopEvidence::View(Shop->GetView());
    for(const auto& Pair:First->Values)
    {
        const auto A=MakeShared<FJsonObject>(),E=MakeShared<FJsonObject>();A->SetField(Pair.Key,Pair.Value);E->SetField(Pair.Key,Gold->GetObjectField(TEXT("first"))->Values[Pair.Key]);
        TestEqual(FString(TEXT("Exact executed source "))+Pair.Key,Canon(A),Canon(E));
    }
    TestEqual(TEXT("Repeated reads/open have no side effects"),Canon(First),Canon(After));
    TestEqual(TEXT("First screen preserves entire run, player, derived observations and world"),Canon(Before),Canon(Full(*Run)));
    TestEqual(TEXT("One open per owner"),Shop->GetOpenCount(),1);
    TestEqual(TEXT("Source empty label trigger (core-only remains blank)"),Shop->GetView().bEmptyAvailable,Id==TEXT("empty")||Id==TEXT("all_burned"));
    auto Evidence=MakeShared<FJsonObject>();Evidence->SetObjectField(TEXT("first"),First);Evidence->SetObjectField(TEXT("after"),After);Evidence->SetObjectField(TEXT("before_state"),Before);Evidence->SetObjectField(TEXT("after_state"),Full(*Run));
    Write(TEXT("shop_source_")+Id+TEXT(".json"),Evidence);
    SaveRoundTrip(*this,*Run,TEXT("shop_save_")+Id+TEXT(".json"));Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopLifetime,"Memoria.Shop.OwnerLifetime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShopLifetime::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* Shop=Game->GetSubsystem<UMemoriaShopSubsystem>();
    TestFalse(TEXT("Inactive run cannot open"),Shop->OpenMalet());Run->BeginStartingMemoryRun();
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(Game.Get());
    TestTrue(TEXT("World owner opens"),Shop->OpenMalet(World));const auto Before=Full(*Run);
    FWorldDelegates::OnWorldCleanup.Broadcast(World,false,true);
    TestFalse(TEXT("Cleanup discards transient shop"),Shop->IsOpen());
    TestEqual(TEXT("Cleanup calls no source close/chapter/save handler"),Canon(Full(*Run)),Canon(Before));
    TestTrue(TEXT("Independent new entry opens"),Shop->OpenMalet());
    TStrongObjectPtr<UMemoriaRunSaveGame> Save(Run->CaptureSave());TestTrue(TEXT("Save restore succeeds"),Run->RestoreSave(*Save));
    TestFalse(TEXT("Even same-ID restore cancels prior shop"),Shop->IsOpen());TestEqual(TEXT("Restore clears requests"),Shop->GetView().Requests.Num(),0);
    TestTrue(TEXT("New entry after restore"),Shop->OpenMalet());Run->BeginStartingMemoryRun();TestFalse(TEXT("New run has no prior shop"),Shop->IsOpen());
    auto E=MakeShared<FJsonObject>();E->SetBoolField(TEXT("cleanup_no_close_effects"),true);E->SetObjectField(TEXT("state"),Full(*Run));Write(TEXT("shop_lifetime.json"),E);
    World->DestroyWorld(false);Game->Shutdown();return !HasAnyErrors();
}
#endif
