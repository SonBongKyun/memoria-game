#include "MemoriaShopEvidence.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Framework/MemoriaSliceHost.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Framework/MemoriaCoordinates.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using namespace MemoriaPotionEvidence;
FString FreshRoot() { return TEXT("case-")+FGuid::NewGuid().ToString(EGuidFormats::Digits); }
FString EvidenceRoot() { return FPaths::ProjectSavedDir()/TEXT("Validation/Checkpoint1"); }
void Evidence(const FString& Name,const Obj& Data)
{
    IFileManager::Get().MakeDirectory(*EvidenceRoot(),true);
    FFileHelper::SaveStringToFile(Canon(Data),*(EvidenceRoot()/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
FString Read(const FString& Path) { FString S;FFileHelper::LoadFileToString(S,*Path);return S; }
void Configure(FAutomationTestBase& T,UGameInstance& G,const FString& Leaf)
{
    T.TestTrue(TEXT("Storage explicitly isolated"),G.GetSubsystem<UMemoriaCheckpointSubsystem>()->ConfigureTestStorage(Leaf));
}
void Setup(FAutomationTestBase& T,UGameInstance& G)
{
    auto* R=G.GetSubsystem<UMemoriaRunSubsystem>();auto* Shop=G.GetSubsystem<UMemoriaShopSubsystem>();
    T.TestTrue(TEXT("Source starting memory"),R->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success);
    auto S=R->GetRunSnapshot();S.Player.Grains=28;S.CurrentChapter=2;
    S.Player.Items={{TEXT("potion"),2},{TEXT("antidote"),1},{TEXT("firebomb"),1}};
    S.Player.RecentItems={TEXT("firebomb"),TEXT("antidote"),TEXT("potion")};
    R->RestoreRun(S,R->GetPlayerMemory()->GetDefinitions(),R->GetPlayerMemory()->GetSnapshot());
    R->SetStoryFlag(TEXT("ch2_malet_done"),true);
    R->BurnMemory(TEXT("daily_market_food"));R->BurnMemory(TEXT("identity_first_sword"));
    Shop->OpenMalet();Shop->SetMode(TEXT("buy"));
    T.TestTrue(TEXT("Source copper purchase"),Shop->Transact(TEXT("buy"),TEXT("sense_copper_taste"),Shop->GetView().Revision));
    T.TestTrue(TEXT("Source close triggers real disk adapter"),Shop->Close(Shop->GetView().Revision));
}
UMemoriaRunSaveGame* Snapshot(UMemoriaRunSubsystem& R)
{
    auto* S=R.CaptureSave();S->SavedAtUtc=FDateTime::UtcNow();
    const auto* C=LoadObject<UMemoriaMemoryCatalog>(nullptr,TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    S->MemoryCatalogId=C->GetPrimaryAssetId();S->FieldReturn.MapId=UMemoriaCheckpointSubsystem::BoundaryId;
    S->FieldReturn.SourceScenePath=UMemoriaCheckpointSubsystem::SourceScene;S->FieldReturn.SourcePixelPosition=FVector2D(500,340);return S;
}
void WriteRaw(UMemoriaRunSaveGame& S,const FString& Path)
{
    TArray<uint8> B;UGameplayStatics::SaveGameToMemory(&S,B);FSHAHash H;FSHA1::HashBuffer(B.GetData(),B.Num(),H.Hash);
    auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("version"),1);O->SetStringField(TEXT("sha1"),H.ToString());O->SetStringField(TEXT("payload"),FBase64::Encode(B));
    FFileHelper::SaveStringToFile(Canon(O),*Path);
}
void CheckSource(FAutomationTestBase& T,UMemoriaRunSubsystem& R)
{
    FString Text=Read(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/checkpoint/contract_expected.v1.json"));
    TSharedPtr<FJsonValue> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root);
    if(!T.TestTrue(TEXT("Fresh executed source oracle available"),Root.IsValid()))return;
    const auto E=Root->AsArray()[0]->AsObject()->GetObjectField(TEXT("data"));
    const auto S=R.GetRunSnapshot();const auto M=R.GetPlayerMemory()->GetSnapshot();const auto D=R.GetPlayerMemory()->GetDefinitions();
    T.TestEqual(TEXT("Source close chapter"),S.CurrentChapter,int64(E->GetNumberField(TEXT("chapter"))));
    T.TestEqual(TEXT("Source purchase balance"),S.Player.Grains,int64(E->GetNumberField(TEXT("grains"))));
    T.TestEqual(TEXT("Source inventory"),Canon(Inventory(S)->GetObjectField(TEXT("items"))),Canon(E->GetObjectField(TEXT("items"))));
    for(const auto& F:E->GetObjectField(TEXT("flags"))->Values)T.TestEqual(TEXT("Source flags"),S.GetFlag(FString(F.Key.ToView())),F.Value->AsBool());
    const auto Records=E->GetObjectField(TEXT("memory"))->GetArrayField(TEXT("memories"));
    T.TestEqual(TEXT("Source owned count"),M.Owned.Num(),Records.Num());
    for(int32 I=0;I<FMath::Min(M.Owned.Num(),Records.Num());++I)
    {
        const auto X=Records[I]->AsObject();const auto& V=M.Owned[I];
        T.TestEqual(TEXT("Source memory identity/order"),V.Id,X->GetStringField(TEXT("id")));
        T.TestEqual(TEXT("Source burned"),V.bBurned,X->GetBoolField(TEXT("is_burned")));
        T.TestEqual(TEXT("Source residue"),V.bResidue,X->GetBoolField(TEXT("is_residue")));
        T.TestEqual(TEXT("Source faded"),V.bFaded,X->GetBoolField(TEXT("is_faded")));
        T.TestEqual(TEXT("Source erosion"),double(V.Erosion),X->GetNumberField(TEXT("erosion")));
        const auto* Def=D.FindByPredicate([&](const auto& V2){return V2.Id==V.Id;});
        if(T.TestNotNull(TEXT("All definitions restored"),Def))
        {
            T.TestEqual(TEXT("Source grade"),int32(Def->RawGrade),int32(X->GetNumberField(TEXT("grade"))));
            T.TestEqual(TEXT("Source burn power"),double(Def->BurnPower),X->GetNumberField(TEXT("burn_power")));
            T.TestEqual(TEXT("Source title"),Def->Title,X->GetStringField(TEXT("title")));
            T.TestEqual(TEXT("Source description"),Def->Description,X->GetStringField(TEXT("description")));
        }
    }
    const auto History=E->GetObjectField(TEXT("memory"))->GetArrayField(TEXT("burned"));
    T.TestEqual(TEXT("Source burn history count"),M.BurnedHistory.Num(),History.Num());
    for(int32 I=0;I<FMath::Min(M.BurnedHistory.Num(),History.Num());++I)T.TestEqual(TEXT("Source burn history order"),M.BurnedHistory[I],History[I]->AsString());
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointRoundTrip,"Memoria.Checkpoint.DiskRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointRoundTrip::RunTest(const FString&)
{
    const FString Leaf=FreshRoot();
    TStrongObjectPtr<UGameInstance> A(NewObject<UGameInstance>());A->Init();Configure(*this,*A,Leaf);Setup(*this,*A);
    auto* R=A->GetSubsystem<UMemoriaRunSubsystem>();auto* C=A->GetSubsystem<UMemoriaCheckpointSubsystem>();
    TestTrue(TEXT("Real primary created"),IFileManager::Get().FileSize(*C->GetSlotPath())>0);CheckSource(*this,*R);
    R->GetWorldCognition()->SeedMaletRoute(true);
    TestTrue(TEXT("World cognition captured too"),C->SaveClosedBoundary(FVector2D(521,347)));
    const auto Before=Full(*R);const FString Disk=Read(C->GetSlotPath());A->Shutdown();
    TStrongObjectPtr<UGameInstance> B(NewObject<UGameInstance>());B->Init();Configure(*this,*B,Leaf);
    R=B->GetSubsystem<UMemoriaRunSubsystem>();C=B->GetSubsystem<UMemoriaCheckpointSubsystem>();
    int32 Signals=0;R->OnInventoryChanged.AddLambda([&](const auto&){++Signals;});R->OnItemToastRequested.AddLambda([&](const auto&,int32){++Signals;});
    FVector2D P;TestTrue(TEXT("Independent GameInstance restores from disk"),C->RestoreClosedBoundary(P));
    TestEqual(TEXT("Full run, memory, world and inventory"),Canon(Full(*R)),Canon(Before));
    TestEqual(TEXT("Source position restored"),P,FVector2D(521,347));TestEqual(TEXT("No duplicate rewards/toasts on load"),Signals,0);
    CheckSource(*this,*R);TestEqual(TEXT("Loading does not rewrite valid save"),Read(C->GetSlotPath()),Disk);
    auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("leaf"),Leaf);E->SetObjectField(TEXT("expected"),Before);
    Evidence(TEXT("process_pointer.json"),E);B->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointRecovery,"Memoria.Checkpoint.BackupRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointRecovery::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> G(NewObject<UGameInstance>());G->Init();Configure(*this,*G,FreshRoot());Setup(*this,*G);
    auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();auto* C=G->GetSubsystem<UMemoriaCheckpointSubsystem>();
    const auto Before=Full(*R);const FString Path=C->GetSlotPath(),First=Read(Path);
    R->SetStoryFlag(TEXT("after_first_save"),true);TestTrue(TEXT("Second disk save"),C->SaveClosedBoundary(FVector2D(700,400)));
    TestEqual(TEXT("Backup is exact previous bytes"),Read(Path+TEXT(".bak")),First);
    FFileHelper::SaveStringToFile(TEXT("{broken"),*Path);FVector2D P;
    TestTrue(TEXT("Recover actual corrupt primary"),C->RestoreClosedBoundary(P));
    TestEqual(TEXT("Recovery uses previous complete state"),Canon(Full(*R)),Canon(Before));
    TestEqual(TEXT("Repair exact source backup bytes"),Read(Path),First);
    TestTrue(TEXT("Recovery is explicit in UI"),C->GetStatusText().Contains(TEXT("recovered")));
    IFileManager::Get().Delete(*Path);
    TestTrue(TEXT("Missing primary also recovers"),C->RestoreClosedBoundary(P));
    auto E=MakeShared<FJsonObject>();E->SetObjectField(TEXT("restored"),Full(*R));E->SetStringField(TEXT("status"),C->GetStatusText());Evidence(TEXT("recovery.json"),E);
    G->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointInvalid,"Memoria.Checkpoint.RejectedSnapshots",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointInvalid::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> G(NewObject<UGameInstance>());G->Init();Configure(*this,*G,FreshRoot());Setup(*this,*G);
    auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();auto* C=G->GetSubsystem<UMemoriaCheckpointSubsystem>();
    const FString Before=Canon(Full(*R));const FString Path=C->GetSlotPath();
    for(int32 I=0;I<10;++I)
    {
        auto* S=Snapshot(*R);
        switch(I)
        {
        case 0:S->ContentRevision=TEXT("unknown");S->Run.ContentRevision=S->ContentRevision;break;
        case 1:S->FieldReturn.SourceScenePath=TEXT("res://missing_scene.tscn");break;
        case 2:S->FieldReturn.MapId=TEXT("chapter3");break;
        case 3:S->SchemaVersion=99;break;
        case 4:S->Run.CurrentChapter=2;break;
        case 5:S->PlayerMemory.Owned[0].Id=TEXT("missing_definition");break;
        case 6:S->WorldCognition.SourceJson=TEXT("{}");break;
        case 7:S->FieldReturn.SourcePixelPosition.X=MAX_dbl;break;
        case 8:S->SceneFlow.bActive=true;break;
        case 9:S->MemoryCatalogId=FPrimaryAssetId();break;
        }
        TestFalse(TEXT("Reject incompatible DTO before live mutation"),C->ValidateSnapshot(*S));
        WriteRaw(*S,Path);FVector2D Position(13,17);
        TestFalse(TEXT("Reject actual incompatible disk file"),C->RestoreClosedBoundary(Position));
        TestEqual(TEXT("Failed load retains full live state"),Canon(Full(*R)),Before);
        TestEqual(TEXT("Failed load retains output position"),Position,FVector2D(13,17));
    }
    WriteRaw(*Snapshot(*R),Path);auto Envelope=Parse(Read(Path));Envelope->SetStringField(TEXT("sha1"),TEXT("wrong"));
    FFileHelper::SaveStringToFile(Canon(Envelope),*Path);FVector2D ChecksumPosition;
    TestFalse(TEXT("Checksum mismatch rejected before deserialization"),C->RestoreClosedBoundary(ChecksumPosition));
    Envelope->SetNumberField(TEXT("version"),2);FFileHelper::SaveStringToFile(Canon(Envelope),*Path);
    TestFalse(TEXT("Unsupported envelope rejected"),C->RestoreClosedBoundary(ChecksumPosition));
    FFileHelper::SaveStringToFile(TEXT("{broken"),*Path);
    TestTrue(TEXT("Corrupt primary exists"),IFileManager::Get().FileSize(*Path)>0);
    FVector2D P;TestFalse(TEXT("No valid primary or backup"),C->RestoreClosedBoundary(P));
    TestEqual(TEXT("Corrupt load also preserves state"),Canon(Full(*R)),Before);
    IFileManager::Get().Delete(*Path);TestFalse(TEXT("Missing save rejected"),C->RestoreClosedBoundary(P));
    Evidence(TEXT("rejected.json"),Full(*R));G->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointWriteFailure,"Memoria.Checkpoint.WriteFailureRetry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointWriteFailure::RunTest(const FString&)
{
#if PLATFORM_WINDOWS
    TStrongObjectPtr<UGameInstance> G(NewObject<UGameInstance>());G->Init();Configure(*this,*G,FreshRoot());Setup(*this,*G);
    auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();auto* C=G->GetSubsystem<UMemoriaCheckpointSubsystem>();const FString Path=C->GetSlotPath(),Old=Read(Path);
    // Read access remains possible; deleting/replacing this primary is deliberately denied.
    HANDLE Lock=::CreateFileW(*Path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(!TestTrue(TEXT("Actual OS file lock acquired"),Lock!=INVALID_HANDLE_VALUE)){G->Shutdown();return false;}
    R->SetStoryFlag(TEXT("retry_marker"),true);
    TestFalse(TEXT("Locked disk replacement fails"),C->SaveClosedBoundary(FVector2D(501,341)));
    TestEqual(TEXT("Failed replacement retains old primary"),Read(Path),Old);
    TestTrue(TEXT("Failure is visible"),C->GetStatusText().Contains(TEXT("not saved")));
    ::CloseHandle(Lock);
    TestTrue(TEXT("Retry succeeds after unlock"),C->SaveClosedBoundary(FVector2D(501,341)));
    TestEqual(TEXT("Backup remains good earlier save"),Read(Path+TEXT(".bak")),Old);
    R->BeginStartingMemoryRun();FVector2D P;TestTrue(TEXT("Retry actually persisted"),C->RestoreClosedBoundary(P));
    TestTrue(TEXT("Retry snapshot is current"),R->GetRunSnapshot().GetFlag(TEXT("retry_marker")));
    Evidence(TEXT("retry.json"),Full(*R));G->Shutdown();
#endif
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointIsolation,"Memoria.Checkpoint.SyntheticIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointIsolation::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> G(NewObject<UGameInstance>());G->Init();auto* C=G->GetSubsystem<UMemoriaCheckpointSubsystem>();
    TestFalse(TEXT("No default disk IO in automated run"),C->IsStorageEnabled());
    TestTrue(TEXT("No default slot path"),C->GetSlotPath().IsEmpty());Setup(*this,*G);
    TestFalse(TEXT("Save stays disabled without test root"),C->SaveClosedBoundary(FVector2D(500,340)));
    TestFalse(TEXT("Traversal cannot target real saves"),C->ConfigureTestStorage(TEXT("../SaveGames")));
    TestFalse(TEXT("Absolute paths forbidden"),C->ConfigureTestStorage(TEXT("C:/real")));
    Configure(*this,*G,FreshRoot());
    auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();bool Nested=false;
    auto H=R->GetPlayerMemory()->OnObserved.AddLambda([&](const auto&){Nested|=C->SaveClosedBoundary(FVector2D(500,340));});
    R->BurnMemory(TEXT("sense_warm_light"));R->GetPlayerMemory()->OnObserved.Remove(H);
    TestFalse(TEXT("Cannot persist a half-dispatched memory event"),Nested);
    TestFalse(TEXT("Nested attempt did not create file"),IFileManager::Get().FileExists(*C->GetSlotPath()));
    G->Shutdown();return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointProcessRead,"MemoriaCheckpointProcess.Read",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointProcessRead::RunTest(const FString&)
{
    const auto Pointer=Parse(Read(EvidenceRoot()/TEXT("process_pointer.json")));
    if(!TestTrue(TEXT("First process wrote pointer"),Pointer.IsValid()))return false;
    TStrongObjectPtr<UGameInstance> G(NewObject<UGameInstance>());G->Init();Configure(*this,*G,Pointer->GetStringField(TEXT("leaf")));
    auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();auto* C=G->GetSubsystem<UMemoriaCheckpointSubsystem>();FVector2D P;
    TestFalse(TEXT("Second process starts without a run"),R->HasActiveRun());
    TestTrue(TEXT("Separate process reads real disk checkpoint"),C->RestoreClosedBoundary(P));
    TestEqual(TEXT("Separate process exact full state"),Canon(Full(*R)),Canon(Pointer->GetObjectField(TEXT("expected"))));
    TestEqual(TEXT("Separate process position"),P,FVector2D(521,347));CheckSource(*this,*R);
    Evidence(TEXT("process_read.json"),Full(*R));G->Shutdown();return !HasAnyErrors();
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
class FCheckpointStartup : public IAutomationLatentCommand
{
    FAutomationTestBase* Test; FString OriginalCommandLine; Obj Expected;
    double Started=FPlatformTime::Seconds(),CaptureTime=0;
    bool bMissing;
public:
    FCheckpointStartup(FAutomationTestBase* T,const FString& Original,const Obj& State,bool Missing)
        : Test(T),OriginalCommandLine(Original),Expected(State),bMissing(Missing) {}
    virtual ~FCheckpointStartup() { FCommandLine::Set(*OriginalCommandLine); }
    virtual bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>60){Test->AddError(TEXT("Continue startup timed out"));return true;}
        UWorld* World=nullptr;
        for(const auto& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::PIE)World=C.World();
        if(!World || World->GetTimeSeconds()<.5)return false;
        auto* PC=Cast<AMemoriaSliceController>(World->GetFirstPlayerController());
        if(!PC || !PC->GetNarrativeWidget())return false;
        auto* G=World->GetGameInstance();auto* R=G->GetSubsystem<UMemoriaRunSubsystem>();
        auto* H=G->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if(!CaptureTime)
        {
            Test->TestTrue(TEXT("Actual command-line route shows Continue screen"),H->GetState()==EMemoriaSliceState::Deferred);
            if(bMissing)
            {
                Test->TestFalse(TEXT("Missing save does not silently start/overwrite run"),R->HasActiveRun());
                Test->TestTrue(TEXT("Missing save presents explicit new-slice action"),PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("Start a new slice")));
                Test->TestTrue(TEXT("Missing-save error rendered"),PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("No valid checkpoint")));
            }
            else
            {
                Test->TestEqual(TEXT("Actual GameMode startup restored prior-process state"),Canon(Full(*R)),Canon(Expected));
                Test->TestTrue(TEXT("Continue did not replay rewards"),H->GetTrace()==TArray<FString>{TEXT("checkpoint:restored:before:chapter_transition_delay")});
                APawn* Pawn=PC->GetPawn();
                if(Test->TestNotNull(TEXT("Continue pawn exists"),Pawn))
                    Test->TestEqual(TEXT("Startup applies saved source position"),Memoria::Coordinates::ToSource(Pawn->GetActorLocation()),FVector2D(521,347));
                Test->TestTrue(TEXT("Continue status is visible"),PC->GetNarrativeWidget()->VisibleText().Contains(TEXT("Checkpoint loaded")));
            }
            auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("visible"),PC->GetNarrativeWidget()->VisibleText());
            if(!bMissing)E->SetObjectField(TEXT("restored"),Full(*R));
            const FString Name=bMissing?TEXT("startup_missing"):TEXT("startup_continue");
            Evidence(Name+TEXT(".json"),E);
            FScreenshotRequest::RequestScreenshot(EvidenceRoot()/(Name+TEXT(".png")),true,false);
            CaptureTime=FPlatformTime::Seconds();return false;
        }
        if(FPlatformTime::Seconds()-CaptureTime<.5)return false;
        FWorldDelegates::OnWorldCleanup.Broadcast(World,false,true);
        Test->TestTrue(TEXT("Continue presentation released with owner world"),H->GetState()==EMemoriaSliceState::Idle);
        return true;
    }
};
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCheckpointStartupTests,"MemoriaCheckpointProcess.Startup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FCheckpointStartupTests::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{Names={TEXT("Continue"),TEXT("Missing")};Commands=Names;}
bool FCheckpointStartupTests::RunTest(const FString& Mode)
{
    const bool Missing=Mode==TEXT("Missing");Obj Expected;FString Leaf;
    if(Missing)Leaf=FreshRoot();
    else
    {
        const auto Pointer=Parse(Read(EvidenceRoot()/TEXT("process_pointer.json")));
        if(!TestTrue(TEXT("Separate producer process evidence exists"),Pointer.IsValid()))return false;
        Expected=Pointer->GetObjectField(TEXT("expected"));Leaf=Pointer->GetStringField(TEXT("leaf"));
    }
    const FString Original=FCommandLine::Get();
    FCommandLine::Set(*(Original+TEXT(" -MemoriaContinue -MemoriaCheckpointTestLeaf=")+Leaf));
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_VerdanHost"))){FCommandLine::Set(*Original);return false;}
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FCheckpointStartup(this,Original,Expected,Missing)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
