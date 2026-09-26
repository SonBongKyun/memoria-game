#include "Misc/AutomationTest.h"
#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "JsonObjectConverter.h"
#include "Blueprint/WidgetTree.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualCoverage,"MemoriaVisual.ArtworkCoverage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVisualCoverage::RunTest(const FString&)
{
    TestEqual(TEXT("Thirty-four illustrations, two frame overlays and seventeen unique portraits"),MemoriaNarrativeArtwork::Sources().Num(),53);
    for(const auto& Entry:MemoriaNarrativeArtwork::Sources())
    {
        auto* Texture=MemoriaNarrativeArtwork::Load(Entry.Source);
        if(TestNotNull(Entry.Source,Texture))TestTrue(*FString::Printf(TEXT("Decoded source dimensions %s %dx%d"),Entry.Source,Texture->GetSizeX(),Texture->GetSizeY()),Texture->GetSizeX()>200 && Texture->GetSizeY()>200);
    }
    auto Check=[&](const FMemoriaNarrativePresentation& P)
    {
        if(P.bHasCg && !P.Cg.IsEmpty())TestNotNull(*(TEXT("Every imported CG resolves: ")+P.Cg),MemoriaNarrativeArtwork::Load(MemoriaNarrativeArtwork::CgSource(P.Cg)));
        if(P.bHasDistortedCg && !P.DistortedCg.IsEmpty())TestNotNull(*(TEXT("Every distorted CG resolves: ")+P.DistortedCg),MemoriaNarrativeArtwork::Load(MemoriaNarrativeArtwork::CgSource(P.DistortedCg)));
        if(P.bHasPortrait && !P.Portrait.IsEmpty())TestNotNull(*(TEXT("Every imported expression resolves: ")+P.Portrait),MemoriaNarrativeArtwork::Load(MemoriaNarrativeArtwork::PortraitSource(P.Portrait)));
        if(P.bHasDistortedPortrait && !P.DistortedPortrait.IsEmpty())TestNotNull(*(TEXT("Every distorted expression resolves: ")+P.DistortedPortrait),MemoriaNarrativeArtwork::Load(MemoriaNarrativeArtwork::PortraitSource(P.DistortedPortrait)));
    };
    auto* VN=LoadObject<UMemoriaVNAsset>(nullptr,TEXT("/Game/Memoria/Generated/Narrative/DA_VN_Ch2MarketArrival.DA_VN_Ch2MarketArrival"));
    if(!TestNotNull(TEXT("Production VN"),VN))return false;
    for(const auto& Row:VN->Definition.Steps)Check(Row.Presentation);
    for(const auto* Name:{TEXT("Ch1ColdOpen"),TEXT("Ch1Prologue"),TEXT("Ch1ForestWalk"),TEXT("Ch1VoidBeast"),TEXT("Ch1AfterForest")})
    {
        const FString Asset=FString(TEXT("DA_VN_"))+Name;
        auto* Chapter1=LoadObject<UMemoriaVNAsset>(nullptr,*(TEXT("/Game/Memoria/Generated/Narrative/")+Asset+TEXT(".")+Asset));
        if(TestNotNull(Name,Chapter1))for(const auto& Row:Chapter1->Definition.Steps)Check(Row.Presentation);
    }
    TestEqual(TEXT("Source CG alias"),MemoriaNarrativeArtwork::CgSource(TEXT("ch1_stump2")),FString(TEXT("res://assets/cg/generated/story_ch1_memory_shrine.png")));
    TestEqual(TEXT("Unknown short ref takes the source default"),MemoriaNarrativeArtwork::CgSource(TEXT("no_such_cg")),FString(TEXT("res://assets/cg/generated/chapter_splash_rim_forest.png")));
    for(const auto* Name:{TEXT("VerdanArrival"),TEXT("MaletTasteBurned"),TEXT("MaletEncounter"),TEXT("MaletRefused"),TEXT("MaletDeal"),TEXT("MaletReward"),TEXT("VerdanMarketWalk"),TEXT("VerdanOldBurner"),TEXT("MaletBackstory"),TEXT("EliaSumpConcern"),TEXT("SumpAtmosphere"),TEXT("SumpLedgerStart"),TEXT("SumpLedgerFound"),TEXT("SumpLedgerReturn"),TEXT("EliaCh2Talk"),TEXT("EliaSongBurned"),TEXT("EliaSwordBurned")})
    {
        const FString Asset=FString(TEXT("DA_Field_"))+Name;
        auto* Field=LoadObject<UMemoriaFieldAsset>(nullptr,*(TEXT("/Game/Memoria/Generated/Narrative/")+Asset+TEXT(".")+Asset));
        if(TestNotNull(Name,Field))for(const auto& Row:Field->Definition.Rows)Check(Row.Presentation);
    }
    TestNull(TEXT("Unknown source never aliases another picture"),MemoriaNarrativeArtwork::Load(TEXT("res://unknown.png")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVisualInteraction,"MemoriaVisual.DialogueInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVisualInteraction::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();
    auto* Host=Game->GetSubsystem<UMemoriaNarrativeSubsystem>();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    if(!TestTrue(TEXT("Start through production source contract"),Host->StartDevelopmentVN())){Game->Shutdown();return false;}
    auto* Widget=CreateWidget<UMemoriaDevelopmentNarrativeWidget>(Game.Get());Widget->TakeWidget();
    const auto Initial=Host->GetView();Widget->Display(Initial);
    TestNotNull(TEXT("First CG"),Widget->DisplayedBackdrop());TestNull(TEXT("Narrator has no stale portrait"),Widget->DisplayedPortrait());
    for(int32 I=0;I<3;++I)Host->Confirm(INDEX_NONE);
    const auto Elia=Host->GetView();Widget->Display(Elia);
    TestEqual(TEXT("CG persists over rows without a CG"),Elia.BackdropSource,Initial.BackdropSource);
    TestEqual(TEXT("Source right-side speaker"),Elia.PortraitSide,FString(TEXT("right")));
    // _should_hide_portraits_for_cg_line: Elia's first line lands on the gate story CG, so the stage stays clear.
    TestNull(TEXT("Story CG line hides portraits"),Widget->DisplayedPortrait());
    Host->Confirm(INDEX_NONE);Widget->Display(Host->GetView());
    TestNotNull(TEXT("Arrel rendered on the next line"),Widget->DisplayedPortrait());TestEqual(TEXT("Arrel lit on the left"),Widget->GetPresentationProbe().ActiveSide,FString(TEXT("left")));
    Host->Confirm(INDEX_NONE);Widget->Display(Host->GetView());
    TestNotNull(TEXT("Elia rendered"),Widget->DisplayedPortrait());TestEqual(TEXT("Single composition: Elia alone on the right"),Widget->GetPresentationProbe().ActiveSide,FString(TEXT("right")));
    FString Before,After;FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(),Before);
    for(int32 I=0;I<5;++I)Widget->Display(Host->GetView());
    FJsonObjectConverter::UStructToJsonObjectString(Run->GetRunSnapshot(),After);TestEqual(TEXT("Presentation redraw cannot mutate run"),After,Before);
    auto Choice=Elia;Choice.Header=TEXT("filtered");Choice.Choices={{0,TEXT("First")},{2,TEXT("Third")}};Widget->Display(Choice);
    TestNull(TEXT("Choices clear portrait"),Widget->DisplayedPortrait());Widget->Navigate(1);TestEqual(TEXT("Filtered original index retained"),Widget->SelectedOriginalIndex(),2);
    Widget->Display(Choice);TestEqual(TEXT("Redraw retains selection"),Widget->SelectedOriginalIndex(),2);
    int32 Confirmed=INDEX_NONE;Widget->OnConfirm.BindLambda([&](int32 Index){Confirmed=Index;});
    TArray<UWidget*> All;Widget->WidgetTree->GetAllWidgets(All);
    for(auto* W:All)if(auto* B=Cast<UMemoriaNarrativeChoiceButton>(W))if(B->ChoiceIndex==1)B->OnClicked.Broadcast();
    TestEqual(TEXT("Actual button submits original index"),Confirmed,2);
    Choice.bPaused=true;Widget->Display(Choice);Widget->Navigate(1);Widget->Choose(0);TestEqual(TEXT("Pause blocks choice clicks"),Confirmed,2);
    auto Blank=Initial;Blank.BackdropSource=TEXT("res://unknown.png");Widget->Display(Blank);TestNull(TEXT("Missing picture clears previous CG"),Widget->DisplayedBackdrop());
    Widget->OnConfirm.Unbind();Game->Shutdown();return !HasAnyErrors();
}
#endif
