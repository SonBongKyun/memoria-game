#include "MemoriaShopEvidence.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
using Val=TSharedPtr<FJsonValue>;
TArray<Val> TransactionCases(const TCHAR* Name)
{
    FString S;Val V;FFileHelper::LoadFileToString(S,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/shop_transactions")/Name));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),V);return V?V->AsArray():TArray<Val>();
}
Obj TransactionCase(const TArray<Val>& Cases,const FString& Id)
{for(const auto& C:Cases)if(C->AsObject()->GetStringField(TEXT("id"))==Id)return C->AsObject();return nullptr;}
Obj TransactionState(UMemoriaRunSubsystem& Run,UMemoriaShopSubsystem& Shop)
{
    auto O=MakeShared<FJsonObject>();auto S=Run.GetRunSnapshot();auto M=Run.GetPlayerMemory()->GetSnapshot();auto V=Shop.GetView();
    O->SetNumberField(TEXT("grains"),S.Player.Grains);O->SetNumberField(TEXT("chapter"),S.CurrentChapter);O->SetBoolField(TEXT("open"),V.bOpen);
    auto Flags=MakeShared<FJsonObject>();for(const auto& F:S.StoryFlags)Flags->SetBoolField(F.Id,F.bValue);O->SetObjectField(TEXT("flags"),Flags);
    TArray<Val> Rows,History,Sold,Toasts;
    for(const auto& R:M.Owned)
    {
        auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("id"),R.Id);X->SetBoolField(TEXT("burned"),R.bBurned);X->SetBoolField(TEXT("residue"),R.bResidue);
        X->SetBoolField(TEXT("faded"),R.bFaded);X->SetNumberField(TEXT("erosion"),R.Erosion);
        TArray<Val> Connections;for(const auto& C:R.Connections)Connections.Add(MakeShared<FJsonValueString>(C));X->SetArrayField(TEXT("connections"),Connections);
        Rows.Add(MakeShared<FJsonValueObject>(X));
    }
    for(const auto& H:M.BurnedHistory)History.Add(MakeShared<FJsonValueString>(H));
    for(const auto& Offer:V.Stock)if(Offer.bSold)Sold.Add(MakeShared<FJsonValueString>(Offer.Id));
    for(const auto& T:Shop.GetToasts()){auto X=MakeShared<FJsonObject>();X->SetStringField(TEXT("text"),T.Text);X->SetNumberField(TEXT("type"),T.Type);Toasts.Add(MakeShared<FJsonValueObject>(X));}
    O->SetArrayField(TEXT("memories"),Rows);O->SetArrayField(TEXT("history"),History);O->SetArrayField(TEXT("sold"),Sold);O->SetArrayField(TEXT("toasts"),Toasts);
    return O;
}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FShopTransactionsSource,"Memoria.ShopTransactions.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FShopTransactionsSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{for(const auto& C:TransactionCases(TEXT("contract_inputs.v1.json"))){const auto Id=C->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}}
bool FShopTransactionsSource::RunTest(const FString& Id)
{
    const auto Input=TransactionCase(TransactionCases(TEXT("contract_inputs.v1.json")),Id),Gold=TransactionCase(TransactionCases(TEXT("contract_expected.v1.json")),Id);
    if(!TestTrue(TEXT("Executed source case exists"),Input.IsValid()&&Gold.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* Shop=Game->GetSubsystem<UMemoriaShopSubsystem>();Run->BeginStartingMemoryRun();
    auto S=Run->GetRunSnapshot();S.CurrentLocale=Input->GetStringField(TEXT("locale"));S.Player.Grains=Input->GetIntegerField(TEXT("grains"));
    TestTrue(TEXT("Setup restore"),Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot())==EMemoriaMemoryResult::Success);
    if(Input->GetBoolField(TEXT("oath")))Run->SetStoryFlag(TEXT("oath_ash_sworn"),true);
    const auto WorldBefore=Run->GetWorldCognition()->ExportJson();Shop->OpenMalet();int32 Index=0;TArray<Val> States;
    for(const auto& A:Input->GetArrayField(TEXT("actions")))
    {
        const auto Action=A->AsArray();const auto Mode=Action[0]->AsString(),Memory=Action[1]->AsString();
        if(Mode==TEXT("close"))TestTrue(TEXT("Explicit close"),Shop->Close(Shop->GetView().Revision));
        else
        {
            Shop->SetMode(Mode);
            TestEqual(TEXT("Source transaction success or insufficient balance"),Shop->Transact(Mode,Memory,Shop->GetView().Revision),Id!=TEXT("buy_poor"));
        }
        auto Actual=TransactionState(*Run,*Shop);auto Expected=Gold->GetArrayField(TEXT("states"))[Index++]->AsObject();
        // Source audio/profile/stats signal sinks are separate from state ownership.
        auto Projected=MakeShared<FJsonObject>();for(const auto& P:Actual->Values)Projected->SetField(P.Key,Expected->Values[P.Key]);
        TestEqual(TEXT("Exact executed source state incl connection cascade and notifications"),Canon(Actual),Canon(Projected));
        States.Add(MakeShared<FJsonValueObject>(Actual));
    }
    TestEqual(TEXT("Shop cannot mutate NPC world cognition"),Run->GetWorldCognition()->ExportJson(),WorldBefore);
    if(Id==TEXT("close"))
    {
        const auto V=Shop->GetView();
        TestTrue(TEXT("Source outbound autosave observed"),V.Requests.Contains(TEXT("request:autosave:chapter_transition")));
        TestTrue(TEXT("Delay is explicitly deferred"),V.Requests.Last()==TEXT("deferred:chapter_transition_delay:1.5"));
        TestFalse(TEXT("Duplicate close ignored"),Shop->Close(V.Revision));
        TestFalse(TEXT("Cannot reopen completed exchange in same run"),Shop->OpenMalet());
    }
    auto E=MakeShared<FJsonObject>();E->SetArrayField(TEXT("states"),States);E->SetObjectField(TEXT("full"),Full(*Run));Write(TEXT("shop_transaction_")+Id+TEXT(".json"),E);
    SaveRoundTrip(*this,*Run,TEXT("shop_transaction_save_")+Id+TEXT(".json"));Game->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopTransactionGuards,"Memoria.ShopTransactions.Guards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShopTransactionGuards::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();auto* Shop=Game->GetSubsystem<UMemoriaShopSubsystem>();Run->BeginStartingMemoryRun();Shop->OpenMalet();
    auto V=Shop->GetView();const auto Before=Canon(Full(*Run));
    TestFalse(TEXT("Core excluded"),Shop->Transact(TEXT("sell"),TEXT("core_name_origin"),V.Revision));
    TestFalse(TEXT("Wrong case ID excluded"),Shop->Transact(TEXT("sell"),TEXT("Sense_forest_smell"),V.Revision));
    Shop->SetMode(TEXT("buy"));TestFalse(TEXT("Old tab revision excluded"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),V.Revision));
    TestEqual(TEXT("Rejected commands preserve complete state"),Canon(Full(*Run)),Before);
    Shop->SetMode(TEXT("sell"));V=Shop->GetView();
    bool NestedAccepted=false;
    auto Handle=Run->GetPlayerMemory()->OnObserved.AddLambda([&](const auto&){NestedAccepted|=Shop->Close(Shop->GetView().Revision);NestedAccepted|=Shop->Transact(TEXT("sell"),TEXT("sense_warm_light"),Shop->GetView().Revision);});
    TestTrue(TEXT("Sell succeeds"),Shop->Transact(TEXT("sell"),TEXT("daily_campfire_song"),V.Revision));
    Run->GetPlayerMemory()->OnObserved.Remove(Handle);TestFalse(TEXT("Nested mutation blocked during source signals"),NestedAccepted);
    TestFalse(TEXT("Repeated physical confirmation has stale revision"),Shop->Transact(TEXT("sell"),TEXT("daily_campfire_song"),V.Revision));
    Shop->SetMode(TEXT("buy"));V=Shop->GetView();
    int64 SeenBalance=-1;bool AcquiredAtGrainsSignal=true;
    auto GH=Shop->OnGrainsChanged.AddLambda([&](int64 G){SeenBalance=G;AcquiredAtGrainsSignal=Run->GetPlayerMemory()->GetDefinitions().ContainsByPredicate([](const auto& D){return D.Id==TEXT("sense_copper_taste");});});
    TestTrue(TEXT("Purchase succeeds"),Shop->Transact(TEXT("buy"),TEXT("sense_copper_taste"),V.Revision));
    Shop->OnGrainsChanged.Remove(GH);
    TestEqual(TEXT("Source subtracts before grains signal"),SeenBalance,int64(7));TestFalse(TEXT("Source acquisition follows grains signal"),AcquiredAtGrainsSignal);
    TestFalse(TEXT("Fresh revision still cannot duplicate sold stock"),Shop->Transact(TEXT("buy"),TEXT("sense_copper_taste"),Shop->GetView().Revision));
    // Replacement from an outbound observer must not acquire into the new run.
    Run->BeginStartingMemoryRun();auto S=Run->GetRunSnapshot();S.Player.Grains=30;Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot());Shop->OpenMalet();Shop->SetMode(TEXT("buy"));
    GH=Shop->OnGrainsChanged.AddLambda([&](int64){Run->BeginStartingMemoryRun();});
    TestFalse(TEXT("Run replacement aborts continuation"),Shop->Transact(TEXT("buy"),TEXT("daily_malet_deal"),Shop->GetView().Revision));Shop->OnGrainsChanged.Remove(GH);
    TestEqual(TEXT("Replacement retains starting definitions"),Run->GetPlayerMemory()->GetDefinitions().Num(),7);
    TestEqual(TEXT("Replacement has no old debit"),Run->GetRunSnapshot().Player.Grains,int64(0));TestFalse(TEXT("Replacement closes owner"),Shop->IsOpen());
    // Same-ID save restore cancels stale view just as a new run does.
    Shop->OpenMalet();V=Shop->GetView();auto* Save=Run->CaptureSave();Run->RestoreSave(*Save);
    TestFalse(TEXT("Old view cannot transact after restore"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),V.Revision));
    Shop->OpenMalet();Run->BurnMemory(TEXT("sense_forest_smell"));
    TestFalse(TEXT("Externally burned memory cannot sell"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),Shop->GetView().Revision));
    Run->BeginStartingMemoryRun();S=Run->GetRunSnapshot();S.Player.Grains=MAX_int64;
    Run->RestoreRun(S,Run->GetPlayerMemory()->GetDefinitions(),Run->GetPlayerMemory()->GetSnapshot());Shop->OpenMalet();
    TestFalse(TEXT("Sale cannot overflow balance"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),Shop->GetView().Revision));
    TestTrue(TEXT("Overflow cannot burn the memory"),Run->GetPlayerMemory()->IsIntact(TEXT("sense_forest_smell")));
    Run->BeginStartingMemoryRun();auto M=Run->GetPlayerMemory()->GetSnapshot();M.Owned[0].bFaded=true;
    M.ActiveLoan.bActive=true;M.ActiveLoan.MemoryId=TEXT("sense_warm_light");M.ActiveLoan.Principal=5;M.ActiveLoan.Repay=6;M.ActiveLoan.DueChapter=2;
    TestTrue(TEXT("Faded and collateral setup"),Run->RestoreRun(Run->GetRunSnapshot(),Run->GetPlayerMemory()->GetDefinitions(),M)==EMemoriaMemoryResult::Success);
    Shop->OpenMalet();
    TestFalse(TEXT("Faded excluded from sell command"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),Shop->GetView().Revision));
    TestFalse(TEXT("Collateral excluded from sell command"),Shop->Transact(TEXT("sell"),TEXT("sense_warm_light"),Shop->GetView().Revision));
    Run->BeginStartingMemoryRun();auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(Game.Get());Shop->OpenMalet(World);V=Shop->GetView();
    FWorldDelegates::OnWorldCleanup.Broadcast(World,false,true);
    TestFalse(TEXT("Dead owner cannot transact"),Shop->Transact(TEXT("sell"),TEXT("sense_forest_smell"),V.Revision));
    TestFalse(TEXT("Dead owner cannot complete chapter"),Shop->Close(V.Revision));
    TestFalse(TEXT("Cleanup does not set close flag"),Run->GetRunSnapshot().GetFlag(TEXT("ch2_complete")));
    World->DestroyWorld(false);
    auto E=MakeShared<FJsonObject>();E->SetObjectField(TEXT("full"),Full(*Run));Write(TEXT("shop_transaction_guards.json"),E);
    Game->Shutdown();return !HasAnyErrors();
}
#endif

