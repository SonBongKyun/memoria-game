#include "Narrative/MemoriaClassifier.h"
#include "World/MemoriaWorldCognition.h"

namespace MemoriaClassifier
{
FString PersistentReport(const UMemoriaWorldCognition& World)
{
    if (World.KnowsFact(Kairos, IdentifiedFact)) return Identified;
    if (World.KnowsFact(Kairos, UnknownFact)) return Unknown;
    return FString();
}
FString ResolveReport(UMemoriaWorldCognition& World)
{
    const bool bIdentified = World.KnowsFact(Kairos, IdentifiedFact), bUnknown = World.KnowsFact(Kairos, UnknownFact);
    if (bIdentified || bUnknown)
    {
        if (bIdentified && bUnknown) UE_LOG(LogTemp, Warning, TEXT("[Ch5Classifier] Conflicting historical report facts; preserving identified outcome"));
        return bIdentified ? Identified : Unknown;
    }
    const bool bRequest = World.KnowsFact(MemoriaWorldIds::Malet, MemoriaWorldIds::RouteFact);
    const FMemoriaWorldMemory* Source = World.FindMemory(MemoriaWorldIds::Malet, MemoriaWorldIds::RouteMemory);
    const bool bActive = Source && Source->Status.Equals(TEXT("active"), ESearchCase::CaseSensitive);
    const bool bArrel = Source && Source->SourceActorId.Equals(MemoriaWorldIds::Arrel, ESearchCase::CaseSensitive);
    const bool bNamed = bRequest && bActive && bArrel;
    if (!World.LearnFact(Kairos, bNamed ? IdentifiedFact : UnknownFact)) { UE_LOG(LogTemp, Error, TEXT("[Ch5Classifier] Could not commit historical Malet report outcome")); return FString(); }
    return bNamed ? Identified : Unknown;
}
}
