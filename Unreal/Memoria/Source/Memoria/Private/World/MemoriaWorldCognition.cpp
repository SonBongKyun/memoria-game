#include "World/MemoriaWorldCognition.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace
{
using Obj = TSharedPtr<FJsonObject>;
using Val = TSharedPtr<FJsonValue>;
bool Eq(const FString& A,const FString& B) { return A.Equals(B,ESearchCase::CaseSensitive); }
Obj Parse(const FString& Text) { Obj O; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O); return O; }
FString Write(const Obj& O) { FString S; FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&S));return S; }
bool Slug(const FString& S)
{
    if(S.IsEmpty() || S[0]<'a' || S[0]>'z' || S.EndsWith(TEXT("_")) || S.Contains(TEXT("__")))return false;
    for(TCHAR C:S) if(!((C>='a' && C<='z') || (C>='0' && C<='9') || C=='_'))return false;
    return true;
}
bool Triple(const FString& S,const FString& Prefix)
{ TArray<FString> P;S.ParseIntoArray(P,TEXT("."),false);return P.Num()==3 && Eq(P[0],Prefix) && Slug(P[1]) && Slug(P[2]); }
bool Integer(const Obj& O,const TCHAR* K,int64& Out)
{ double D; if(!O->TryGetNumberField(K,D) || !FMath::IsFinite(D) || D<0 || D>9007199254740991.0 || FMath::FloorToDouble(D)!=D)return false;Out=int64(D);return true; }
bool Bag(const Obj& O,const TCHAR* K,FString& Out)
{ const Obj* B=nullptr;if(!O->TryGetObjectField(K,B))return false;Out=Write(*B);return true; }
bool Unique(TSet<FString>& Seen,const FString& Id) { if(Seen.Contains(Id))return false;Seen.Add(Id);return true; }
TArray<Val> Array(const TArray<FString>& S) { TArray<Val> A;for(const auto& V:S)A.Add(MakeShared<FJsonValueString>(V));return A; }
}
bool MemoriaWorldIds::IsActor(const FString& Id)
{ return Eq(Id,Arrel) || Eq(Id,Malet) || Eq(Id,TEXT("npc.sable")) || Eq(Id,TEXT("npc.kairos")); }
bool MemoriaWorldIds::IsFact(const FString& Id) { return Triple(Id,TEXT("fact")); }
bool MemoriaWorldIds::IsOwnedMemory(const FString& Id,const FString& Actor)
{
    if(!IsActor(Actor) || !Triple(Id,TEXT("memory")))return false;
    TArray<FString> M,A;Id.ParseIntoArray(M,TEXT("."));Actor.ParseIntoArray(A,TEXT("."));return Eq(M[1],A[1]);
}
FMemoriaWorldSnapshot UMemoriaWorldCognition::Defaults()
{
    FMemoriaWorldSnapshot S;
    for(const TCHAR* Id:{TEXT("npc.kairos"),MemoriaWorldIds::Malet,TEXT("npc.sable"),MemoriaWorldIds::Arrel})
    { FMemoriaActorCognition A;A.ActorId=Id;if(Eq(Id,MemoriaWorldIds::Malet)) { FMemoriaWorldKnowledge K;K.FactId=TEXT("fact.veil.exists");K.bValue=true;A.Knowledge.Add(K); } S.Actors.Add(A); }
    return S;
}
FString UMemoriaWorldCognition::Encode(const FMemoriaWorldSnapshot& S)
{
    Obj O=MakeShared<FJsonObject>(),Actors=MakeShared<FJsonObject>();
    O->SetNumberField(TEXT("schema_version"),S.SchemaVersion);O->SetNumberField(TEXT("revision"),S.Revision);O->SetNumberField(TEXT("event_sequence"),S.EventSequence);
    O->SetObjectField(TEXT("world_flags"),Parse(S.WorldFlagsJson));O->SetObjectField(TEXT("quest_states"),Parse(S.QuestStatesJson));
    for(const auto& A:S.Actors)
    {
        Obj Actor=MakeShared<FJsonObject>(),Knowledge=MakeShared<FJsonObject>(),Memories=MakeShared<FJsonObject>();
        Actor->SetObjectField(TEXT("location"),Parse(A.LocationJson));Actor->SetObjectField(TEXT("relationships"),Parse(A.RelationshipsJson));Actor->SetObjectField(TEXT("emotions"),Parse(A.EmotionsJson));Actor->SetObjectField(TEXT("quest_state"),Parse(A.QuestStateJson));Actor->SetObjectField(TEXT("flags"),Parse(A.FlagsJson));
        for(const auto& K:A.Knowledge) { Obj V=MakeShared<FJsonObject>();V->SetStringField(TEXT("fact_id"),K.FactId);V->SetBoolField(TEXT("value"),K.bValue);V->SetNumberField(TEXT("updated_revision"),K.UpdatedRevision);Knowledge->SetObjectField(K.FactId,V); }
        for(const auto& M:A.Memories)
        {
            Obj V=MakeShared<FJsonObject>();V->SetStringField(TEXT("id"),M.Id);V->SetStringField(TEXT("owner_actor_id"),M.OwnerActorId);V->SetStringField(TEXT("status"),M.Status);V->SetArrayField(TEXT("fact_ids"),Array(M.FactIds));V->SetStringField(TEXT("source_actor_id"),M.SourceActorId);V->SetObjectField(TEXT("content"),Parse(M.ContentJson));
            V->SetNumberField(TEXT("created_revision"),M.CreatedRevision);V->SetNumberField(TEXT("removed_revision"),M.RemovedRevision);V->SetNumberField(TEXT("last_removed_revision"),M.LastRemovedRevision);V->SetNumberField(TEXT("restored_revision"),M.RestoredRevision);Memories->SetObjectField(M.Id,V);
        }
        Actor->SetObjectField(TEXT("knowledge"),Knowledge);Actor->SetObjectField(TEXT("memories"),Memories);Actors->SetObjectField(A.ActorId,Actor);
    }
    O->SetObjectField(TEXT("actors"),Actors);return Write(O);
}
bool UMemoriaWorldCognition::Decode(const FString& Text,FMemoriaWorldSnapshot& Out)
{
    const Obj O=Parse(Text); FMemoriaWorldSnapshot S; int64 Schema;const Obj* Actors=nullptr;
    if(!O || !Integer(O,TEXT("schema_version"),Schema) || Schema!=1 || !Integer(O,TEXT("revision"),S.Revision) || !Integer(O,TEXT("event_sequence"),S.EventSequence) || !Bag(O,TEXT("world_flags"),S.WorldFlagsJson) || !Bag(O,TEXT("quest_states"),S.QuestStatesJson) || !O->TryGetObjectField(TEXT("actors"),Actors))return false;
    for(const auto& Entry:(*Actors)->Values)
    {
        FMemoriaActorCognition A;A.ActorId=FString(Entry.Key.ToView());const Obj* Value=nullptr;
        if(!MemoriaWorldIds::IsActor(A.ActorId) || !Entry.Value->TryGetObject(Value))return false;
        const Obj V=*Value;const Obj *Knowledge=nullptr,*Memories=nullptr;
        if(!Bag(V,TEXT("location"),A.LocationJson) || !Bag(V,TEXT("relationships"),A.RelationshipsJson) || !Bag(V,TEXT("emotions"),A.EmotionsJson) || !Bag(V,TEXT("quest_state"),A.QuestStateJson) || !Bag(V,TEXT("flags"),A.FlagsJson) || !V->TryGetObjectField(TEXT("knowledge"),Knowledge) || !V->TryGetObjectField(TEXT("memories"),Memories))return false;
        for(const auto& E:(*Knowledge)->Values)
        {
            FMemoriaWorldKnowledge K;const Obj* R=nullptr;const FString Id(E.Key.ToView());
            if(!MemoriaWorldIds::IsFact(Id) || !E.Value->TryGetObject(R) || !(*R)->TryGetStringField(TEXT("fact_id"),K.FactId) || !Eq(K.FactId,Id) || !(*R)->TryGetBoolField(TEXT("value"),K.bValue) || !Integer(*R,TEXT("updated_revision"),K.UpdatedRevision))return false;
            A.Knowledge.Add(K);
        }
        for(const auto& E:(*Memories)->Values)
        {
            FMemoriaWorldMemory M;const Obj* R=nullptr;const FString Id(E.Key.ToView());
            if(!MemoriaWorldIds::IsOwnedMemory(Id,A.ActorId) || !E.Value->TryGetObject(R))return false;
            const Obj B=*R;const TArray<Val>* Facts=nullptr;
            if(!B->TryGetStringField(TEXT("id"),M.Id) || !Eq(M.Id,Id) || !B->TryGetStringField(TEXT("owner_actor_id"),M.OwnerActorId) || !Eq(M.OwnerActorId,A.ActorId) || !B->TryGetStringField(TEXT("status"),M.Status) || !(Eq(M.Status,TEXT("active")) || Eq(M.Status,TEXT("removed"))) || !B->TryGetStringField(TEXT("source_actor_id"),M.SourceActorId) || (!M.SourceActorId.IsEmpty() && !MemoriaWorldIds::IsActor(M.SourceActorId)) || !Bag(B,TEXT("content"),M.ContentJson) || !B->TryGetArrayField(TEXT("fact_ids"),Facts))return false;
            for(const auto& F:*Facts) { FString IdValue;if(!F->TryGetString(IdValue) || !MemoriaWorldIds::IsFact(IdValue))return false;M.FactIds.Add(IdValue); }
            if(!Integer(B,TEXT("created_revision"),M.CreatedRevision) || !Integer(B,TEXT("removed_revision"),M.RemovedRevision) || !Integer(B,TEXT("last_removed_revision"),M.LastRemovedRevision) || !Integer(B,TEXT("restored_revision"),M.RestoredRevision))return false;
            A.Memories.Add(M);
        }
        S.Actors.Add(A);
    }
    Out=MoveTemp(S);return true;
}
bool UMemoriaWorldCognition::Restore(const FMemoriaWorldSnapshot& S)
{
    if(bDispatching)return false;
    // Reject duplicate typed keys before JSON can collapse them. Validate bags
    // before Encode so malformed DTOs cannot feed null objects to the writer.
    TSet<FString> Actors;
    if(!Parse(S.WorldFlagsJson) || !Parse(S.QuestStatesJson))return false;
    for(const auto& A:S.Actors)
    {
        if(!Unique(Actors,A.ActorId) || !Parse(A.LocationJson) || !Parse(A.RelationshipsJson) || !Parse(A.EmotionsJson) || !Parse(A.QuestStateJson) || !Parse(A.FlagsJson))return false;
        TSet<FString> Facts,Memories;
        for(const auto& K:A.Knowledge)if(!Unique(Facts,K.FactId))return false;
        for(const auto& M:A.Memories)if(!Unique(Memories,M.Id) || !Parse(M.ContentJson))return false;
    }
    FMemoriaWorldSnapshot Candidate;if(!Decode(Encode(S),Candidate))return false;State=MoveTemp(Candidate);return true;
}
FMemoriaActorCognition* UMemoriaWorldCognition::FindActor(const FString& Id)
{ return State.Actors.FindByPredicate([&](const auto& A){return Eq(A.ActorId,Id);}); }
const FMemoriaActorCognition* UMemoriaWorldCognition::FindActor(const FString& Id) const
{ return State.Actors.FindByPredicate([&](const auto& A){return Eq(A.ActorId,Id);}); }
bool UMemoriaWorldCognition::HasActor(const FString& Id) const { return MemoriaWorldIds::IsActor(Id) && FindActor(Id); }
bool UMemoriaWorldCognition::HasKnowledge(const FString& Actor,const FString& Fact) const
{ const auto* A=FindActor(Actor);return A && A->Knowledge.ContainsByPredicate([&](const auto& K){return Eq(K.FactId,Fact);}); }
bool UMemoriaWorldCognition::HasMemory(const FString& Actor,const FString& Memory) const
{ const auto* A=FindActor(Actor);return A && A->Memories.ContainsByPredicate([&](const auto& M){return Eq(M.Id,Memory);}); }
bool UMemoriaWorldCognition::LearnFact(const FString& Actor,const FString& Fact)
{
    if(bDispatching || !HasActor(Actor) || !MemoriaWorldIds::IsFact(Fact) || State.Revision>=9007199254740991LL || State.EventSequence>=9007199254740991LL)return false;
    auto* A=FindActor(Actor);auto* K=A->Knowledge.FindByPredicate([&](const auto& V){return Eq(V.FactId,Fact);});
    if(K && K->bValue)return false;
    FMemoriaWorldKnowledge Value;Value.FactId=Fact;Value.bValue=true;Value.UpdatedRevision=++State.Revision;
    if(K)*K=Value;else A->Knowledge.Add(Value);
    FMemoriaWorldEvent E;E.Type=TEXT("knowledge.learned");E.ActorId=Actor;E.TargetId=Fact;E.bKnowledge=true;Commit(E);return true;
}
bool UMemoriaWorldCognition::AddMemory(const FString& Actor,const FString& Memory,const TArray<FString>& Facts,const FString& Source,const FString& Content)
{
    if(bDispatching || !HasActor(Actor) || !MemoriaWorldIds::IsOwnedMemory(Memory,Actor) || HasMemory(Actor,Memory) || (!Source.IsEmpty() && !MemoriaWorldIds::IsActor(Source)) || !Parse(Content) || State.Revision>=9007199254740991LL || State.EventSequence>=9007199254740991LL)return false;
    for(const auto& F:Facts)if(!MemoriaWorldIds::IsFact(F))return false;
    FMemoriaWorldMemory M;M.Id=Memory;M.OwnerActorId=Actor;M.FactIds=Facts;M.SourceActorId=Source;M.ContentJson=Content;M.CreatedRevision=++State.Revision;FindActor(Actor)->Memories.Add(M);
    FMemoriaWorldEvent E;E.Type=TEXT("memory.added");E.ActorId=Actor;E.TargetId=Memory;E.FactIds=Facts;E.SourceActorId=Source;Commit(E);return true;
}
void UMemoriaWorldCognition::Commit(FMemoriaWorldEvent E)
{
    E.Revision=State.Revision;E.Sequence=++State.EventSequence;
    // State is fully committed before observers. Copies cannot mutate authority.
    const auto Snapshot=State;TGuardValue<bool> Guard(bDispatching,true);OnCommitted.Broadcast(E,Snapshot);
}
FString FMemoriaWorldEvent::ToJson() const
{
    Obj O=MakeShared<FJsonObject>(),P=MakeShared<FJsonObject>();O->SetNumberField(TEXT("schema_version"),1);O->SetStringField(TEXT("event_id"),FString::Printf(TEXT("world.%08lld"),Sequence));O->SetStringField(TEXT("event_type"),Type);O->SetNumberField(TEXT("event_sequence"),Sequence);O->SetNumberField(TEXT("revision"),Revision);O->SetStringField(TEXT("actor_id"),ActorId);O->SetStringField(TEXT("target_id"),TargetId);
    if(bKnowledge)P->SetBoolField(TEXT("value"),true);
    else {P->SetStringField(TEXT("status"),TEXT("active"));P->SetArrayField(TEXT("fact_ids"),Array(FactIds));P->SetStringField(TEXT("source_actor_id"),SourceActorId);}
    O->SetObjectField(TEXT("payload"),P);return Write(O);
}
void UMemoriaWorldCognition::SeedMaletRoute(bool bDone)
{
    if(!bDone || !HasActor(MemoriaWorldIds::Malet))return;
    // Presence, not truth/activity: retain forgotten knowledge and tombstones.
    if(!HasKnowledge(MemoriaWorldIds::Malet,MemoriaWorldIds::RouteFact))LearnFact(MemoriaWorldIds::Malet,MemoriaWorldIds::RouteFact);
    if(!HasMemory(MemoriaWorldIds::Malet,MemoriaWorldIds::RouteMemory))AddMemory(MemoriaWorldIds::Malet,MemoriaWorldIds::RouteMemory,{MemoriaWorldIds::RouteFact},MemoriaWorldIds::Arrel,TEXT("{\"kind\":\"information_source\",\"subject\":\"bl07_route_request\"}"));
}
