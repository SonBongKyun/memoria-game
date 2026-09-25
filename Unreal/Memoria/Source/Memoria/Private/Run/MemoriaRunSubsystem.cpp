#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaRunSaveGame.h"

EMemoriaMemoryResult UMemoriaRunSubsystem::BeginStartingMemoryRun(int64 StartChapter)
{
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr,
        TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    if (!Catalog || Catalog->CatalogSchemaVersion != 1 ||
        !Catalog->ContentKind.Equals(TEXT("player_memory.starting_catalog"), ESearchCase::CaseSensitive) || Catalog->Definitions.IsEmpty())
    { return EMemoriaMemoryResult::InvalidSnapshot; }
    TArray<FString> Ids;
    for (const auto& Definition : Catalog->Definitions) { Ids.Add(Definition.Id); }
    return BeginRun(*Catalog, Ids, StartChapter);
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
    OnRunReplaced.Clear(); OnInventoryChanged.Clear(); OnItemToastRequested.Clear(); OnPotionObserved.Clear(); OnRewardItemObserved.Clear();
    PlayerMemory = nullptr; WorldCognition = nullptr;
    State = FMemoriaRunSnapshot();
    Super::Deinitialize();
}
EMemoriaMemoryResult UMemoriaRunSubsystem::BeginRun(const UMemoriaMemoryCatalog& Catalog, const TArray<FString>& InitialIds, int64 StartChapter)
{
    if (StartChapter < 1) { return EMemoriaMemoryResult::InvalidSnapshot; }
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
    Run.RunId = FGuid::NewGuid(); Run.ContentRevision = Catalog.ContentRevision; Run.CurrentChapter = StartChapter;
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
    return State.MemoryContext();
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

int64 UMemoriaRunSubsystem::GetItemCount(const FString& Id) const
{
    const auto* Item=State.Player.Items.FindByPredicate([&](const auto& I){return I.Id.Equals(Id,ESearchCase::CaseSensitive);});
    return Item ? Item->Count : 0;
}
EMemoriaRewardItemScope UMemoriaRunSubsystem::RewardItemScope(const FString& Id)
{
    if (!FMemoriaRunSnapshot::IsSourceItem(Id)) return EMemoriaRewardItemScope::InvalidSourceId;
    return Id==TEXT("potion") || Id==TEXT("antidote") || Id==TEXT("firebomb") ? EMemoriaRewardItemScope::Supported : EMemoriaRewardItemScope::DeferredByPhase;
}
TArray<FString> UMemoriaRunSubsystem::GetRecentItems() const
{
    return State.NormalizedRecentItems();
}
bool UMemoriaRunSubsystem::AddRewardPotion(const FString& Id, int64 Count)
{
    return Id.Equals(TEXT("potion"),ESearchCase::CaseSensitive) && GrantRewardItem(Id,TEXT("Potion"),Count);
}
bool UMemoriaRunSubsystem::AddRewardAntidote(const FString& Id, int64 Count)
{
    return Id.Equals(TEXT("antidote"),ESearchCase::CaseSensitive) && GrantRewardItem(Id,TEXT("Antidote"),Count);
}
bool UMemoriaRunSubsystem::AddRewardFirebomb(const FString& Id, int64 Count)
{
    return Id.Equals(TEXT("firebomb"),ESearchCase::CaseSensitive) && GrantRewardItem(Id,TEXT("Firebomb"),Count);
}
bool UMemoriaRunSubsystem::GrantRewardItem(const FString& Id, const TCHAR* DisplayName, int64 Count)
{
    if (!HasActiveRun() || RewardItemScope(Id)!=EMemoriaRewardItemScope::Supported) return false;
    const FGuid Owner=State.RunId;
    const int64 Before=GetItemCount(Id);
    // Define source signed integer addition without C++ signed-overflow UB.
    const int64 After=static_cast<int64>(static_cast<uint64>(Before)+static_cast<uint64>(Count));
    auto* Item=State.Player.Items.FindByPredicate([&](const auto& I){return I.Id.Equals(Id,ESearchCase::CaseSensitive);});
    if(Item) Item->Count=After;
    else { FMemoriaItemCount New;New.Id=Id;New.Count=After;State.Player.Items.Add(New); }
    auto Observe=[&](const TCHAR* Point){const auto Snapshot=State;if(Id==TEXT("potion"))OnPotionObserved.Broadcast(Point,Snapshot);if(State.RunId==Owner)OnRewardItemObserved.Broadcast(Id,Point,Snapshot);};
    Observe(TEXT("after_inventory_mutation"));
    if(State.RunId!=Owner)return false;
    State.RecordRecentItem(Id);
    Observe(TEXT("after_recent_items"));
    if(State.RunId!=Owner)return false;
    OnInventoryChanged.Broadcast(Id);
    if(State.RunId!=Owner)return false;
    Observe(TEXT("after_inventory_changed"));
    if(State.RunId!=Owner)return false;
    // Exact source text remains English for both source en and ko runtime localization.
    OnItemToastRequested.Broadcast(FString::Printf(TEXT("+%lld %s"),Count,DisplayName),1);
    if(State.RunId!=Owner)return false;
    Observe(Id==TEXT("potion")?TEXT("potion_complete"):(Id==TEXT("antidote")?TEXT("antidote_complete"):TEXT("firebomb_complete")));
    return State.RunId==Owner;
}
