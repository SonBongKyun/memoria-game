#include "Misc/AutomationTest.h"
#include "Import/MemoriaNarrativeImport.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Current Chapter 1 route, in scene_flow.gd goto_scene order, ending at the Chapter 2 arrival.
const TCHAR* Chapter1Route[] = {TEXT("ch1_cold_open"),TEXT("ch1_prologue"),TEXT("ch1_forest_walk"),TEXT("ch1_void_beast"),TEXT("ch1_after_forest")};
FString Chapter1Ir(const FString& Id)
{ return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/ir/narrative/")/(Id+TEXT(".vn.v1.json"))); }
const UMemoriaVNAsset* Chapter1Asset(const FString& Id)
{ return LoadObject<UMemoriaVNAsset>(nullptr,*MemoriaNarrativeImport::ObjectPath(true,Id)); }
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FChapter1ImportParity,"Memoria.Chapter1.ImportParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FChapter1ImportParity::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const
{ for (const TCHAR* Id:Chapter1Route) { Names.Add(Id); Commands.Add(Id); } }
bool FChapter1ImportParity::RunTest(const FString& Id)
{
    // The saved production asset equals a fresh strict read of the source-attested IR.
    const auto* Actual=Chapter1Asset(Id); if (!TestNotNull(TEXT("Saved Chapter 1 asset"),Actual)) return false;
    TStrongObjectPtr<UMemoriaVNAsset> Expected(NewObject<UMemoriaVNAsset>()); FString Error;
    if (!TestTrue(TEXT("Strict IR + direct source attestation"),MemoriaNarrativeImport::ReadIr(Chapter1Ir(Id),*Expected,Error))) { AddError(Error); return false; }
    for (TFieldIterator<FProperty> P(UMemoriaVNAsset::StaticClass(),EFieldIterationFlags::None);P;++P) TestTrue(*P->GetName(),P->Identical_InContainer(Actual,Expected.Get()));
    TestEqual(TEXT("Typed semantic fingerprint"),MemoriaNarrativeImport::Fingerprint(*Actual),Expected->ImportMetadata.SemanticSha256);
    TestEqual(TEXT("Chapter 1 metadata"),Actual->Definition.Metadata.Chapter,1);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter1RouteChain,"Memoria.Chapter1.RouteChain",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FChapter1RouteChain::RunTest(const FString&)
{
    // Every scene hands off to the next with goto_scene; the last reaches the Chapter 2 arrival.
    int32 Distorted=0,SetChapter=0,CompleteChapter=0,Autosave=0,Impacts=0,ChoiceFraming=0,ChoiceEffects=0;
    for (int32 I=0;I<UE_ARRAY_COUNT(Chapter1Route);++I)
    {
        const auto* A=Chapter1Asset(Chapter1Route[I]); if (!TestNotNull(Chapter1Route[I],A)) return false;
        const FString Next=I+1<UE_ARRAY_COUNT(Chapter1Route)?FString(Chapter1Route[I+1]):FString(TEXT("ch2_market_arrival"));
        bool bHandsOff=false;
        for (const auto& S:A->Definition.Steps)
        {
            if (S.Action.bHasAction && S.Action.Action==TEXT("goto_scene") && S.Action.Id==Next) bHandsOff=true;
            if (S.Action.bHasAction && S.Action.Action==TEXT("goto_scene"))
                TestTrue(TEXT("goto_scene target is in the imported cohort"),Chapter1Asset(S.Action.Id)!=nullptr);
            Distorted+=S.Text.bHasDistortIfBurned; SetChapter+=S.Effects.bHasSetChapter; CompleteChapter+=S.Effects.bHasCompleteChapter;
            Autosave+=S.Effects.bHasAutosaveChapterTransition; Impacts+=S.Presentation.bHasImpact; ChoiceFraming+=S.Text.bHasChoiceTitle;
            for (const auto& C:S.Choices) ChoiceEffects+=C.Text.bHasEffect;
        }
        TestTrue(*FString::Printf(TEXT("%s hands off to %s"),Chapter1Route[I],*Next),bHandsOff);
    }
    // Source counts from data/vn_scenes/ch1_*.json (see the S302 survey).
    TestEqual(TEXT("Distortion steps"),Distorted,4);
    TestEqual(TEXT("set_chapter"),SetChapter,1);TestEqual(TEXT("complete_chapter"),CompleteChapter,1);TestEqual(TEXT("autosave"),Autosave,1);
    TestEqual(TEXT("Impact cues"),Impacts,5);TestEqual(TEXT("Choice titles"),ChoiceFraming,3);TestEqual(TEXT("Choice effect texts"),ChoiceEffects,8);
    TestNotNull(TEXT("Chapter 2 arrival still imported"),Chapter1Asset(TEXT("ch2_market_arrival")));
    return !HasAnyErrors();
}
#endif
