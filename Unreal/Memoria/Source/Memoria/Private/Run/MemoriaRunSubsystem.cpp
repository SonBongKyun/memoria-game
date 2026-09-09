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
    OnRunReplaced.Clear();
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
