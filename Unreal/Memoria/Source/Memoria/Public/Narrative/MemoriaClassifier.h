#pragma once
#include "CoreMinimal.h"
class UMemoriaWorldCognition;

// S322: Chapter 5, The Classifier (scenes/story/ch5_classifier_entry.gd). Kairos reads Malet's report once;
// the outcome is frozen as one of two Kairos facts, so later changes to Malet's memory do not rewrite what
// crossed the gap in Chapter 5. The two story flags only select the VN's authored lines.
namespace MemoriaClassifier
{
    inline constexpr const TCHAR* Kairos = TEXT("npc.kairos");
    inline constexpr const TCHAR* IdentifiedFact = TEXT("fact.kairos.malet_report_identified_arrel");
    inline constexpr const TCHAR* UnknownFact = TEXT("fact.kairos.malet_report_requester_unknown");
    inline constexpr const TCHAR* Identified = TEXT("identified_arrel");
    inline constexpr const TCHAR* Unknown = TEXT("requester_unknown");
    inline constexpr const TCHAR* Scene = TEXT("ch5_classifier");
    // resolve_malet_report_outcome: the historical fact if one exists, otherwise decide it now: identified
    // when Malet recorded the route request, still holds its source memory, and that source is Arrel.
    // Empty when the fact cannot be committed.
    MEMORIA_API FString ResolveReport(UMemoriaWorldCognition& World);
    // get_persistent_report_outcome.
    MEMORIA_API FString PersistentReport(const UMemoriaWorldCognition& World);
}
