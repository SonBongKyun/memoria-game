#include "Run/MemoriaRunSubsystem.h"

void UMemoriaRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PlayerMemory = NewObject<UMemoriaPlayerMemoryDomain>(this);
}
void UMemoriaRunSubsystem::Deinitialize()
{
    OnRunReplaced.Clear();
    PlayerMemory = nullptr;
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
EMemoriaMemoryResult UMemoriaRunSubsystem::RestoreRun(const FMemoriaRunSnapshot& Run, const TArray<FMemoriaMemoryDefinition>& Definitions, const FMemoriaMemorySnapshot& Memory)
{
    if (!PlayerMemory || !Run.IsValid()) { return EMemoriaMemoryResult::InvalidSnapshot; }
    const auto Result = PlayerMemory->Restore(Definitions, Memory);
    if (Result == EMemoriaMemoryResult::Success) { State = Run; OnRunReplaced.Broadcast(); }
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
