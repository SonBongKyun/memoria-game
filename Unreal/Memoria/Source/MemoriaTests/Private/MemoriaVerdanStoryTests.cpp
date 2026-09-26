#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Narrative/MemoriaVerdanStory.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Interaction/MemoriaMaletActor.h"
#include "Narrative/MemoriaSumpLedger.h"
#include "Run/MemoriaRunTypes.h"
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
    // Sump Ledger: quest definition, areas, requested groups and rewards match the source.
    namespace L=MemoriaSumpLedger;const auto Q=Root->GetObjectField(TEXT("sump_ledger"));
    TestEqual(TEXT("Quest title"),L::Title(false),Q->GetStringField(TEXT("title")));TestEqual(TEXT("Quest title ko"),L::Title(true),Q->GetStringField(TEXT("title_ko")));
    TestEqual(TEXT("Chapter gate"),L::ChapterRequired,int64(Q->GetNumberField(TEXT("chapter_req"))));
    const auto& Steps=Q->GetArrayField(TEXT("steps"));if(TestEqual(TEXT("Step count"),L::Steps().Num(),Steps.Num()))
        for(int32 I=0;I<Steps.Num();++I)
        {
            TestEqual(TEXT("Step flag"),FString(L::Steps()[I].Flag),Steps[I]->AsString());
            TestEqual(TEXT("Step desc"),FString(L::Steps()[I].Desc),Q->GetArrayField(TEXT("step_desc"))[I]->AsString());
            TestEqual(TEXT("Step desc ko"),FString(L::Steps()[I].DescKo),Q->GetArrayField(TEXT("step_desc_ko"))[I]->AsString());
        }
    TestEqual(TEXT("Reward Grains"),L::RewardGrains,int64(Q->GetNumberField(TEXT("reward_grains"))));
    TestEqual(TEXT("Reward item count"),L::RewardItemCount,int64(Q->GetObjectField(TEXT("reward_items"))->GetNumberField(L::RewardItem)));
    const auto M=Q->GetObjectField(TEXT("reward_memory"));const auto D=L::RewardMemory();
    TestEqual(TEXT("Memory id"),D.Id,M->GetStringField(TEXT("id")));TestEqual(TEXT("Memory title"),D.Title,M->GetStringField(TEXT("title")));
    TestEqual(TEXT("Memory desc"),D.Description,M->GetStringField(TEXT("desc")));TestEqual(TEXT("Memory effect"),D.StoryEffect,M->GetStringField(TEXT("effect")));
    TestEqual(TEXT("Memory grade"),int32(D.RawGrade),int32(M->GetNumberField(TEXT("grade"))));TestEqual(TEXT("Memory power"),D.BurnPower,int64(M->GetNumberField(TEXT("burn_power"))));
    const auto Rect=[](const TArray<TSharedPtr<FJsonValue>>& R){return FIntRect(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber(),R[3]->AsNumber());};
    TestEqual(TEXT("Trader area"),L::TraderSourceRect,Rect(Q->GetArrayField(TEXT("trader_rect"))));TestEqual(TEXT("Ledger area"),L::LedgerSourceRect,Rect(Q->GetArrayField(TEXT("ledger_rect"))));
    for(const auto& G:{TEXT("sq_sump_ledger_start"),TEXT("sq_sump_ledger_found"),TEXT("sq_sump_ledger_return")})
    {
        TestTrue(TEXT("Group requested by the source"),Q->GetArrayField(TEXT("requested_groups")).ContainsByPredicate([&](const auto& V){return V->AsString()==G;}));
        const FString Name=FString(TEXT("DA_Field_SumpLedger"))+(FString(G).EndsWith(TEXT("start"))?TEXT("Start"):FString(G).EndsWith(TEXT("found"))?TEXT("Found"):TEXT("Return"));
        const auto* Asset=LoadObject<UMemoriaFieldAsset>(nullptr,*(TEXT("/Game/Memoria/Generated/Narrative/")+Name+TEXT(".")+Name));
        if(TestNotNull(G,Asset))TestEqual(TEXT("Quest rows"),Asset->Definition.Rows.Num(),int32(Q->GetObjectField(TEXT("rows"))->GetNumberField(G)));
    }
    for(const FVector& P:{L::TraderLocation,L::LedgerLocation})
    {
        TestTrue(TEXT("Quest point clear of Malet"),FVector::Dist2D(P,AMemoriaMaletActor::DevelopmentLocation())>2*MemoriaVerdanStory::InteractionRange);
        for(const auto& B:Beats)TestTrue(TEXT("Quest point clear of story beats"),FVector::Dist2D(P,B.Location)>2*MemoriaVerdanStory::InteractionRange);
        TestTrue(TEXT("Quest point inside the courtyard"),FMath::Abs(P.X)<860&&FMath::Abs(P.Y)<560);
    }
    // State machine over flags, in the trader's source branch order.
    FMemoriaRunSnapshot S;S.CurrentChapter=2;
    TestEqual(TEXT("Chapter 2 has no quest"),int32(L::TraderAction(S)),int32(L::ETraderAction::None));
    S.CurrentChapter=3;TestEqual(TEXT("Chapter 3 offers the quest"),int32(L::TraderAction(S)),int32(L::ETraderAction::Start));
    S.StoryFlags.Add({TEXT("sq_sump_ledger_started"),true});TestEqual(TEXT("Started without ledger reminds"),int32(L::TraderAction(S)),int32(L::ETraderAction::Remind));
    TestEqual(TEXT("Tracker names the ledger step"),L::CurrentStepText(S,false),FString(L::Steps()[1].Desc));
    S.StoryFlags.Add({TEXT("sq_sump_ledger_found"),true});TestEqual(TEXT("Found ledger returns"),int32(L::TraderAction(S)),int32(L::ETraderAction::Return));
    S.StoryFlags.Add({TEXT("sq_sump_ledger_done"),true});TestTrue(TEXT("Done completes"),L::IsComplete(S)&&!L::IsActive(S)&&L::TraderAction(S)==L::ETraderAction::None);
    return !HasAnyErrors();
}
#endif
