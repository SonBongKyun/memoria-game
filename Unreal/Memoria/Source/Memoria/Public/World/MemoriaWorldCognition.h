#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MemoriaWorldCognition.generated.h"

// Native value DTOs. JSON is confined to the existing schema-1 save adapter and
// opaque source extension dictionaries; it is never a second runtime authority.
struct MEMORIA_API FMemoriaWorldKnowledge
{
    FString FactId; bool bValue = false; int64 UpdatedRevision = 0;
};
struct MEMORIA_API FMemoriaWorldMemory
{
    FString Id, OwnerActorId, Status = TEXT("active"), SourceActorId;
    TArray<FString> FactIds;
    FString ContentJson = TEXT("{}");
    int64 CreatedRevision = 0, RemovedRevision = 0, LastRemovedRevision = 0, RestoredRevision = 0;
};
struct MEMORIA_API FMemoriaActorCognition
{
    FString ActorId;
    TArray<FMemoriaWorldKnowledge> Knowledge;
    TArray<FMemoriaWorldMemory> Memories;
    FString LocationJson = TEXT("{}"), RelationshipsJson = TEXT("{}"), EmotionsJson = TEXT("{}"), QuestStateJson = TEXT("{}"), FlagsJson = TEXT("{}");
};
struct MEMORIA_API FMemoriaWorldSnapshot
{
    int32 SchemaVersion = 1;
    int64 Revision = 0, EventSequence = 0;
    TArray<FMemoriaActorCognition> Actors;
    FString WorldFlagsJson = TEXT("{}"), QuestStatesJson = TEXT("{}");
};
struct MEMORIA_API FMemoriaWorldEvent
{
    int64 Sequence = 0, Revision = 0;
    FString Type, ActorId, TargetId;
    bool bKnowledge = false;
    TArray<FString> FactIds;
    FString SourceActorId;
    FString ToJson() const;
};
DECLARE_MULTICAST_DELEGATE_TwoParams(FMemoriaWorldCommitted, const FMemoriaWorldEvent&, const FMemoriaWorldSnapshot&);

namespace MemoriaWorldIds
{
    inline constexpr const TCHAR* Malet = TEXT("npc.malet");
    inline constexpr const TCHAR* Arrel = TEXT("player.arrel");
    inline constexpr const TCHAR* RouteFact = TEXT("fact.bl07.route_request_received");
    inline constexpr const TCHAR* RouteMemory = TEXT("memory.malet.bl07_request_source");
    MEMORIA_API bool IsActor(const FString& Id);
    MEMORIA_API bool IsFact(const FString& Id);
    MEMORIA_API bool IsOwnedMemory(const FString& Id, const FString& Actor);
}

// Persistent run-owned cognition. No player-card dependency, world-actor pointer,
// inventory, timer or narrative authority. Only this phase's two write commands.
UCLASS()
class MEMORIA_API UMemoriaWorldCognition : public UObject
{
    GENERATED_BODY()
public:
    static FMemoriaWorldSnapshot Defaults();
    static FString Encode(const FMemoriaWorldSnapshot& Snapshot);
    // Strict native schema-1 restore, not the deferred permissive Godot legacy importer.
    static bool Decode(const FString& Json, FMemoriaWorldSnapshot& Out);
    bool Restore(const FMemoriaWorldSnapshot& Snapshot);
    FMemoriaWorldSnapshot GetSnapshot() const { return State; }
    FString ExportJson() const { return Encode(State); }
    bool IsDispatchingEvent() const { return bDispatching; }
    bool HasActor(const FString& Id) const;
    bool HasKnowledge(const FString& Actor, const FString& Fact) const;
    bool HasMemory(const FString& Actor, const FString& Memory) const;
    // S322 MemoryEngine.knows_fact (held as true) and WorldState.get_memory_record (active or removed).
    bool KnowsFact(const FString& Actor, const FString& Fact) const;
    const FMemoriaWorldMemory* FindMemory(const FString& Actor, const FString& Memory) const;
    bool LearnFact(const FString& Actor, const FString& Fact);
    bool AddMemory(const FString& Actor, const FString& Memory, const TArray<FString>& Facts, const FString& Source, const FString& ContentJson);
    void SeedMaletRoute(bool bDone);
    FMemoriaWorldCommitted OnCommitted;
private:
    FMemoriaWorldSnapshot State;
    bool bDispatching = false;
    FMemoriaActorCognition* FindActor(const FString& Id);
    const FMemoriaActorCognition* FindActor(const FString& Id) const;
    void Commit(FMemoriaWorldEvent Event);
};
