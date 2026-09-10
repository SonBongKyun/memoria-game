#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"

EMemoriaMemoryResult UMemoriaRunSubsystem::BeginStartingMemoryRun()
{
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr,
        TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    if (!Catalog || Catalog->CatalogSchemaVersion != 1 ||
        !Catalog->ContentKind.Equals(TEXT("player_memory.starting_catalog"), ESearchCase::CaseSensitive) || Catalog->Definitions.IsEmpty())
    { return EMemoriaMemoryResult::InvalidSnapshot; }
    TArray<FString> Ids;
    for (const auto& Definition : Catalog->Definitions) { Ids.Add(Definition.Id); }
    return BeginRun(*Catalog, Ids);
}

void UMemoriaRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PlayerMemory = NewObject<UMemoriaPlayerMemoryDomain>(this);
    WorldCognition = NewObject<UMemoriaWorldCognition>(this);
    WorldCognition->Restore(UMemoriaWorldCognition::Defaults());
}
void UMemoriaRunSubsystem::Deinitialize()
{
    OnRunReplaced.Clear(); OnInventoryChanged.Clear(); OnItemToastRequested.Clear(); OnPotionObserved.Clear();
    PlayerMemory = nullptr; WorldCognition = nullptr;
    State = FMemoriaRunSnapshot();
    Super::Deinitialize();
}
EMemoriaMemoryResult UMemoriaRunSubsystem::BeginRun(const UMemoriaMemoryCatalog& Catalog, const TArray<FString>& InitialIds)
{
    TArray<FMemoriaMemoryDefinition> Definitions;
    FMemoriaMemorySnapshot Memory;
    for (const auto& Id : InitialIds)
    {
        const auto* D = Catalog.Definitions.FindByPredicate([&](const auto& V) { return V.Id.Equals(Id, ESearchCase::CaseSensitive); });
        if (!D) { return EMemoriaMemoryResult::Missing; }
        Definitions.Add(*D);
        FMemoriaMemoryState S; S.Id = Id; Memory.Owned.Add(S);
    }
    FMemoriaRunSnapshot Run;
    Run.RunId = FGuid::NewGuid(); Run.ContentRevision = Catalog.ContentRevision;
    return RestoreRun(Run, Definitions, Memory);
}
EMemoriaMemoryResult UMemoriaRunSubsystem::RestoreRun(const FMemoriaRunSnapshot& Run, const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Memory, const FMemoriaWorldSnapshot& World)
{
    if (!PlayerMemory || !Run.IsValid() || (WorldCognition && WorldCognition->IsDispatchingEvent())) { return EMemoriaMemoryResult::InvalidSnapshot; }
    auto* CandidateWorld = NewObject<UMemoriaWorldCognition>(this);
    if (!CandidateWorld->Restore(World)) return EMemoriaMemoryResult::InvalidSnapshot;
    const auto Result = PlayerMemory->Restore(Definitions, Memory);
    if (Result == EMemoriaMemoryResult::Success) { State = Run; WorldCognition = CandidateWorld; OnRunReplaced.Broadcast(); }
    return Result;
}
FMemoriaMemoryContext UMemoriaRunSubsystem::GetMemoryContext() const
{
    FMemoriaMemoryContext C;
    C.CurrentChapter = State.CurrentChapter; C.bEliaWithParty = State.Player.bEliaWithParty;
    C.bStillHandsActive = State.GetFlag(TEXT("oath_still_sworn")) && !State.GetFlag(TEXT("oath_still_broken"));
    return C;
}
EMemoriaMemoryResult UMemoriaRunSubsystem::BurnMemory(const FString& Id, EMemoriaBurnMode Mode, bool bAllowFaded)
{
    return HasActiveRun() && PlayerMemory ? PlayerMemory->Burn(Id, Mode, bAllowFaded, GetMemoryContext()) : EMemoriaMemoryResult::InvalidSnapshot;
}
EMemoriaMemoryResult UMemoriaRunSubsystem::AcquireMemory(const FMemoriaMemoryDefinition& Definition)
{
    return HasActiveRun() && PlayerMemory ? PlayerMemory->Add(Definition, GetMemoryContext()) : EMemoriaMemoryResult::InvalidSnapshot;
}
EMemoriaMemoryResult UMemoriaRunSubsystem::ErodeMemories(int64 ChapterArgument)
{
    return HasActiveRun() && PlayerMemory ? PlayerMemory->ApplyErosion(ChapterArgument, GetMemoryContext()) : EMemoriaMemoryResult::InvalidSnapshot;
}
bool UMemoriaRunSubsystem::SetStoryFlag(const FString& Id, bool bValue)
{
    if (!HasActiveRun() || Id.IsEmpty() || !PlayerMemory || PlayerMemory->IsDispatchingEvent()) { return false; }
    auto* F = State.StoryFlags.FindByPredicate([&](const auto& V) { return V.Id.Equals(Id, ESearchCase::CaseSensitive); });
    if (F) { F->bValue = bValue; }
    else { FMemoriaStoryFlag Flag; Flag.Id = Id; Flag.bValue = bValue; State.StoryFlags.Add(Flag); }
    return true;
}

bool UMemoriaRunSubsystem::RemoveStoryFlag(const FString& Id)
{
    if (!HasActiveRun() || Id.IsEmpty() || !PlayerMemory || PlayerMemory->IsDispatchingEvent()) return false;
    State.StoryFlags.RemoveAll([&](const auto& V) { return V.Id.Equals(Id, ESearchCase::CaseSensitive); });
    return true;
}

UMemoriaRunSaveGame* UMemoriaRunSubsystem::CaptureSave() const
{
    if(!HasActiveRun() || !PlayerMemory || !WorldCognition)return nullptr;
    auto* Save=NewObject<UMemoriaRunSaveGame>();Save->Run=State;Save->ContentRevision=State.ContentRevision;
    Save->MemoryDefinitions=PlayerMemory->GetDefinitions();Save->PlayerMemory=PlayerMemory->GetSnapshot();
    Save->WorldCognition.SourceJson=WorldCognition->ExportJson();return Save;
}
bool UMemoriaRunSubsystem::RestoreSave(const UMemoriaRunSaveGame& Save)
{
    FString Error;FMemoriaWorldSnapshot World=UMemoriaWorldCognition::Defaults();
    if(!Save.ValidateHeader(Error) || Save.WorldCognition.SchemaVersion!=1 ||
        (!Save.WorldCognition.SourceJson.IsEmpty() && !UMemoriaWorldCognition::Decode(Save.WorldCognition.SourceJson,World)))return false;
    return RestoreRun(Save.Run,Save.MemoryDefinitions,Save.PlayerMemory,World)==EMemoriaMemoryResult::Success;
}

namespace
{
bool KnownRecentItem(const FString& Id)
{
    // Source ITEMS membership only, used to preserve recent-item normalization.
    // No use/equipment/catalog import or grants for these other identities.
    for (const TCHAR* Known : {TEXT("potion"),TEXT("hi_potion"),TEXT("antidote"),TEXT("firebomb"),TEXT("smoke_bomb"),TEXT("witness_ink"),TEXT("root_balm"),TEXT("signal_jammer"),TEXT("lantern_salve"),TEXT("name_thread"),TEXT("compass_shard"),TEXT("seed_capsule"),TEXT("anchor_lantern"),TEXT("ledger_chalk"),TEXT("cinder_vial"),TEXT("witness_knot")})
        if (Id.Equals(Known, ESearchCase::CaseSensitive)) return true;
    return false;
}
}
int64 UMemoriaRunSubsystem::GetItemCount(const FString& Id) const
{
    const auto* Item=State.Player.Items.FindByPredicate([&](const auto& I){return I.Id.Equals(Id,ESearchCase::CaseSensitive);});
    return Item ? Item->Count : 0;
}
bool UMemoriaRunSubsystem::AddRewardPotion(const FString& Id, int64 Count)
{
    if (!HasActiveRun() || !Id.Equals(TEXT("potion"),ESearchCase::CaseSensitive)) return false;
    const FGuid Owner=State.RunId;
    const int64 Before=GetItemCount(Id);
    // Define source signed integer addition without C++ signed-overflow UB.
    const int64 After=static_cast<int64>(static_cast<uint64>(Before)+static_cast<uint64>(Count));
    auto* Item=State.Player.Items.FindByPredicate([&](const auto& I){return I.Id.Equals(Id,ESearchCase::CaseSensitive);});
    if(Item) Item->Count=After;
    else { FMemoriaItemCount New;New.Id=Id;New.Count=After;State.Player.Items.Add(New); }
    auto Observe=[&](const TCHAR* Point){const auto Snapshot=State;OnPotionObserved.Broadcast(Point,Snapshot);};
    Observe(TEXT("after_inventory_mutation"));
    if(State.RunId!=Owner)return false;
    TArray<FString> Recent;
    for(const auto& Value:State.Player.RecentItems)
    {
        if(KnownRecentItem(Value) && !Recent.ContainsByPredicate([&](const auto& R){return R.Equals(Value,ESearchCase::CaseSensitive);}))Recent.Add(Value);
        if(Recent.Num()>=5)break;
    }
    Recent.RemoveAll([&](const auto& R){return R.Equals(Id,ESearchCase::CaseSensitive);});
    Recent.Insert(Id,0);if(Recent.Num()>5)Recent.SetNum(5);
    State.Player.RecentItems=MoveTemp(Recent);
    Observe(TEXT("after_recent_items"));
    if(State.RunId!=Owner)return false;
    OnInventoryChanged.Broadcast(Id);
    if(State.RunId!=Owner)return false;
    Observe(TEXT("after_inventory_changed"));
    if(State.RunId!=Owner)return false;
    // Exact source text remains English for both source en and ko runtime localization.
    OnItemToastRequested.Broadcast(FString::Printf(TEXT("+%lld Potion"),Count),1);
    if(State.RunId!=Owner)return false;
    Observe(TEXT("potion_complete"));
    return State.RunId==Owner;
}
