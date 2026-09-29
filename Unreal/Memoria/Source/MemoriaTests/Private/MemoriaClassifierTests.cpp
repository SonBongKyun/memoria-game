#include "Misc/AutomationTest.h"
#include "Narrative/MemoriaClassifier.h"
#include "World/MemoriaWorldCognition.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_DEV_AUTOMATION_TESTS
// S322: ch5_classifier_entry.gd resolve_malet_report_outcome. Malet's report names Arrel only when Malet
// recorded the BL-07 route request and still holds its source memory, sourced from Arrel. The outcome is
// decided once, as a Kairos fact, and later changes to Malet's memory never rewrite it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMemoriaClassifierReportTest, "Memoria.Narrative.ClassifierReport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMemoriaClassifierReportTest::RunTest(const FString&)
{
    auto Fresh = [] { TStrongObjectPtr<UMemoriaWorldCognition> W(NewObject<UMemoriaWorldCognition>()); W->Restore(UMemoriaWorldCognition::Defaults()); return W; };
    using namespace MemoriaClassifier;
    {
        auto W = Fresh();
        TestTrue(TEXT("Kairos is a world actor"), W->HasActor(Kairos));
        TestEqual(TEXT("No report yet"), PersistentReport(*W), FString());
        // The route request was made: Malet knows it and keeps the source memory, from Arrel.
        W->SeedMaletRoute(true);
        TestTrue(TEXT("Malet knows the request"), W->KnowsFact(MemoriaWorldIds::Malet, MemoriaWorldIds::RouteFact));
        const FMemoriaWorldMemory* Source = W->FindMemory(MemoriaWorldIds::Malet, MemoriaWorldIds::RouteMemory);
        TestTrue(TEXT("Its source is Arrel"), Source && Source->SourceActorId == MemoriaWorldIds::Arrel && Source->Status == TEXT("active"));
        TestEqual(TEXT("The report names Arrel"), ResolveReport(*W), FString(Identified));
        TestTrue(TEXT("Kairos keeps the identified fact"), W->KnowsFact(Kairos, IdentifiedFact) && !W->KnowsFact(Kairos, UnknownFact));
        TestEqual(TEXT("The outcome persists"), PersistentReport(*W), FString(Identified));
        TestEqual(TEXT("Resolving again reads history"), ResolveReport(*W), FString(Identified));
    }
    {
        auto W = Fresh();
        // No route request: the market kept the event, not the name.
        TestEqual(TEXT("The requester stays unknown"), ResolveReport(*W), FString(Unknown));
        TestTrue(TEXT("Kairos keeps the unknown fact"), W->KnowsFact(Kairos, UnknownFact) && !W->KnowsFact(Kairos, IdentifiedFact));
        // A later route memory does not rewrite what already crossed the gap.
        W->SeedMaletRoute(true);
        TestEqual(TEXT("History is not rewritten"), ResolveReport(*W), FString(Unknown));
    }
    return true;
}
#endif
