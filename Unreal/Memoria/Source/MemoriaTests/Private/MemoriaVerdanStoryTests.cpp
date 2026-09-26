#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Narrative/MemoriaVerdanStory.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Interaction/MemoriaMaletActor.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVerdanStorySourceTable,"Memoria.VerdanStory.SourceTable",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVerdanStorySourceTable::RunTest(const FString&)
{
    FString Text;TSharedPtr<FJsonObject> Root;
    const FString File=FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/verdan_story/source_triggers.v1.json");
    if(!TestTrue(TEXT("Source trigger fixture"),FFileHelper::LoadFileToString(Text,*File)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)&&Root.IsValid()))return false;
    const auto& Source=Root->GetArrayField(TEXT("beats"));const auto& Beats=MemoriaVerdanStory::Beats();
    if(!TestEqual(TEXT("Every source beat, in authored order"),Beats.Num(),Source.Num()))return false;
    for(int32 I=0;I<Beats.Num();++I)
    {
        const auto S=Source[I]->AsObject();const auto& B=Beats[I];
        TestEqual(TEXT("Group"),FString(B.Group),S->GetStringField(TEXT("group")));
        TestEqual(TEXT("One-time flag"),FString(B.Flag),S->GetStringField(TEXT("flag")));
        FString Requires;const bool bRequires=S->TryGetStringField(TEXT("requires_flag"),Requires);
        TestEqual(TEXT("Entry guard"),B.RequiresFlag?FString(B.RequiresFlag):FString(),bRequires?Requires:FString());
        const auto& R=S->GetArrayField(TEXT("rect"));
        TestEqual(TEXT("Source Area2D rect"),B.SourceRect,FIntRect(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber(),R[3]->AsNumber()));
        const FString Path=FString(TEXT("/Game/Memoria/Generated/Narrative/"))+B.Asset+TEXT(".")+B.Asset;
        const auto* Asset=LoadObject<UMemoriaFieldAsset>(nullptr,*Path);
        if(TestNotNull(B.Group,Asset))
        {
            TestEqual(TEXT("Imported rows match the authored group"),Asset->Definition.Rows.Num(),int32(S->GetNumberField(TEXT("rows"))));
            TestEqual(TEXT("Imported group identity"),Asset->Definition.Id,FString(B.Group));
        }
        // Development placement: outside Malet's range and each other's, inside the curb.
        TestTrue(TEXT("Clear of Malet"),FVector::Dist2D(B.Location,AMemoriaMaletActor::DevelopmentLocation())>2*MemoriaVerdanStory::InteractionRange);
        TestTrue(TEXT("Inside the walkable courtyard"),FMath::Abs(B.Location.X)<860&&FMath::Abs(B.Location.Y)<560);
        for(int32 J=I+1;J<Beats.Num();++J)TestTrue(TEXT("Points do not share a range"),FVector::Dist2D(B.Location,Beats[J].Location)>2*MemoriaVerdanStory::InteractionRange);
        TestTrue(TEXT("Prompt names an action"),FString(B.Prompt).Contains(TEXT("E / A")));
    }
    return !HasAnyErrors();
}
#endif
