#include "Presentation/MemoriaBattleEntryWidget.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElementTypes.h"
#include "Engine/Texture2D.h"
namespace
{
const FLinearColor BattlePaper(.86f,.85f,.80f),BattleMuted(.43f,.49f,.51f),BattleGold(.66f,.47f,.25f),BattleInk(.018f,.030f,.038f,.93f);
// Witness reads as cold archive ink, distinct from the ember of a burn.
const FLinearColor BattleWitness(.46f,.80f,.78f),BattleEmber(.86f,.44f,.18f),BattleLimit(.45f,.66f,.84f);
void BattlePlace(UCanvasPanel* Canvas,UWidget* Widget,float Left,float Top,float Right,float Bottom)
{
    auto* Slot=Canvas->AddChildToCanvas(Widget);Slot->SetAnchors(FAnchors(Left,Top,Right,Bottom));Slot->SetOffsets(FMargin(0));
}
UTextBlock* BattleText(UWidgetTree* Tree,const FString& Value,int32 Size,FLinearColor Color)
{
    auto* Text=Tree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Value));Text->SetAutoWrapText(true);
    Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),Size));Text->SetColorAndOpacity(Color);return Text;
}
UBorder* BattlePanel(UWidgetTree* Tree,FLinearColor Color,float Outline=0)
{
    auto* Panel=Tree->ConstructWidget<UBorder>();Panel->SetBrush(FSlateRoundedBoxBrush(Color,2.f,BattleGold,Outline));
    Panel->SetPadding(FMargin(0));return Panel;
}
UImage* BattlePicture(UWidgetTree* Tree,UBorder* Frame,const FString& Source)
{
    auto* Fit=Tree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Frame->SetContent(Fit);
    auto* Image=Tree->ConstructWidget<UImage>();Image->SetBrushFromTexture(MemoriaBattleEntryArt::Load(Source),true);Fit->AddChild(Image);
    return Image;
}
UProgressBar* BattleHealthBar(UWidgetTree* Tree,FLinearColor Color)
{
    auto* Bar=Tree->ConstructWidget<UProgressBar>();FProgressBarStyle Style;
    Style.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(.075f,.10f,.11f),1.f));
    Style.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White,1.f));Bar->SetWidgetStyle(Style);
    Bar->SetFillColorAndOpacity(Color);Bar->SetBorderPadding(FVector2D::ZeroVector);return Bar;
}
bool BattleConfirmKey(const FKey& Key)
{return Key==EKeys::Enter || Key==EKeys::E || Key==EKeys::SpaceBar || Key==EKeys::Gamepad_FaceButton_Bottom;}
bool BattleOwnedKey(const FKey& Key)
{
    return BattleConfirmKey(Key) || Key==EKeys::Escape || Key==EKeys::Tab || Key==EKeys::M || Key==EKeys::Gamepad_FaceButton_Right ||
        Key==EKeys::Up || Key==EKeys::Down || Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::W || Key==EKeys::A || Key==EKeys::S || Key==EKeys::D;
}
UTexture2D* BattleTexture(const UImage* Image)
{return Image?Cast<UTexture2D>(Image->GetBrush().GetResourceObject()):nullptr;}
}
void UMemoriaBattleEntryButton::Activate(){if(Owner)Owner->ClickAction(Index,bChoice);}
TSharedRef<SWidget> UMemoriaBattleEntryWidget::RebuildWidget()
{
    if(!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        Backdrop=WidgetTree->ConstructWidget<UImage>();Backdrop->SetBrushFromTexture(MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png")),true);
        auto* BackFit=WidgetTree->ConstructWidget<UScaleBox>();BackFit->SetStretch(EStretch::ScaleToFill);BackFit->AddChild(Backdrop);BattlePlace(Canvas,BackFit,0,0,1,1);
        Backdrop->SetColorAndOpacity(FLinearColor(.44f,.48f,.51f,1));
        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.005f,.009f,.014f,.56f)),0,0,1,1);
        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.009f,.018f,.027f,.88f)),.025f,.035f,.975f,.165f);
        BattlePlace(Canvas,BattleText(WidgetTree,TEXT("M E M O R I A   /   B A T T L E"),11,BattleGold),.055f,.052f,.48f,.08f);
        Heading=BattleText(WidgetTree,TEXT(""),27,BattlePaper);BattlePlace(Canvas,Heading,.055f,.087f,.69f,.143f);
        Turn=BattleText(WidgetTree,TEXT(""),13,BattleGold);Turn->SetJustification(ETextJustify::Right);BattlePlace(Canvas,Turn,.715f,.091f,.946f,.137f);
        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.44f,.33f,.19f,.7f)),.055f,.164f,.945f,.166f);

        auto* PlayerFrame=BattlePanel(WidgetTree,BattleInk,.7f);PlayerFrame->SetPadding(FMargin(3));BattlePlace(Canvas,PlayerFrame,.055f,.208f,.346f,.658f);
        PlayerArt=BattlePicture(WidgetTree,PlayerFrame,TEXT("res://assets/portraits/character_shots/arrel_battle_v3.png"));
        PlayerName=BattleText(WidgetTree,TEXT(""),20,BattlePaper);BattlePlace(Canvas,PlayerName,.067f,.68f,.24f,.721f);
        PlayerHealth=BattleText(WidgetTree,TEXT(""),13,BattleMuted);PlayerHealth->SetJustification(ETextJustify::Right);BattlePlace(Canvas,PlayerHealth,.216f,.686f,.343f,.719f);
        PlayerBar=BattleHealthBar(WidgetTree,FLinearColor(.28f,.47f,.56f));BattlePlace(Canvas,PlayerBar,.067f,.729f,.343f,.739f);

        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.012f,.026f,.035f,.86f)),.374f,.209f,.653f,.426f);
        ObjectiveTitle=BattleText(WidgetTree,TEXT(""),16,BattleGold);BattlePlace(Canvas,ObjectiveTitle,.389f,.225f,.637f,.272f);
        ObjectiveBody=BattleText(WidgetTree,TEXT(""),13,BattlePaper);BattlePlace(Canvas,ObjectiveBody,.389f,.284f,.637f,.401f);
        Modifier=BattleText(WidgetTree,TEXT(""),11,BattleMuted);BattlePlace(Canvas,Modifier,.376f,.430f,.654f,.497f);
        AllyFrame=BattlePanel(WidgetTree,BattleInk,.5f);AllyFrame->SetPadding(FMargin(2));BattlePlace(Canvas,AllyFrame,.378f,.5f,.508f,.727f);
        AllyArt=BattlePicture(WidgetTree,AllyFrame,TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png"));
        AllyName=BattleText(WidgetTree,TEXT(""),12,BattlePaper);BattlePlace(Canvas,AllyName,.519f,.666f,.643f,.724f);

        auto* EnemyFrame=BattlePanel(WidgetTree,FLinearColor(.012f,.016f,.019f,.90f),.7f);EnemyFrame->SetPadding(FMargin(3));BattlePlace(Canvas,EnemyFrame,.688f,.209f,.945f,.687f);
        EnemyArt=BattlePicture(WidgetTree,EnemyFrame,FString());
        EchoBand=BattlePanel(WidgetTree,FLinearColor(.008f,.022f,.026f,.84f));EchoBand->SetPadding(FMargin(10,6));BattlePlace(Canvas,EchoBand,.692f,.582f,.941f,.683f);
        EchoText=BattleText(WidgetTree,TEXT(""),12,BattleWitness);EchoBand->SetContent(EchoText);
        EnemyName=BattleText(WidgetTree,TEXT(""),22,BattlePaper);BattlePlace(Canvas,EnemyName,.7f,.700f,.938f,.742f);
        EnemyHealth=BattleText(WidgetTree,TEXT(""),13,BattleMuted);BattlePlace(Canvas,EnemyHealth,.702f,.746f,.938f,.772f);
        EnemyBar=BattleHealthBar(WidgetTree,FLinearColor(.48f,.23f,.19f));BattlePlace(Canvas,EnemyBar,.702f,.776f,.938f,.785f);
        WitnessLabel=BattleText(WidgetTree,TEXT(""),12,BattleWitness);BattlePlace(Canvas,WitnessLabel,.702f,.797f,.81f,.83f);

        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.010f,.022f,.032f,.94f)),.055f,.788f,.665f,.858f);
        LogScroll=WidgetTree->ConstructWidget<UScrollBox>();LogScroll->SetScrollbarThickness(FVector2D(3,3));BattlePlace(Canvas,LogScroll,.071f,.798f,.65f,.852f);
        BattleLog=BattleText(WidgetTree,TEXT(""),13,BattlePaper);LogScroll->AddChild(BattleLog);
        for(int32 I=0;I<6;++I)
        {
            auto* Button=WidgetTree->ConstructWidget<UMemoriaBattleEntryButton>();Button->Owner=this;Button->Index=I;
            Button->OnClicked.AddDynamic(Button,&UMemoriaBattleEntryButton::Activate);
            auto* Label=BattleText(WidgetTree,TEXT(""),16,BattlePaper);Label->SetJustification(ETextJustify::Center);Button->SetContent(Label);
            BattlePlace(Canvas,Button,.055f+I*.1495f,.874f,.1945f+I*.1495f,.938f);ActionButtons.Add(Button);ActionLabels.Add(Label);
        }
        FleeButton=ActionButtons[5];FleeLabel=ActionLabels[5];
        BoundaryNote=BattleText(WidgetTree,TEXT(""),11,BattleMuted);BattlePlace(Canvas,BoundaryNote,.055f,.95f,.94f,.99f);
        ChoicePanel=BattlePanel(WidgetTree,FLinearColor(.015f,.025f,.035f,.99f),1.f);ChoicePanel->SetPadding(FMargin(16));BattlePlace(Canvas,ChoicePanel,.27f,.21f,.73f,.77f);
        auto* ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>();ChoicePanel->SetContent(ChoiceScroll);
        ChoiceList=WidgetTree->ConstructWidget<UVerticalBox>();ChoiceScroll->AddChild(ChoiceList);
        // Three readable gauges replace the single text line; combo and statuses keep a short tail.
        const auto Gauge=[&](UTextBlock*& Label,UProgressBar*& Bar,FLinearColor Color,float Left)
        {
            Label=BattleText(WidgetTree,TEXT(""),11,Color);BattlePlace(Canvas,Label,Left,.742f,Left+.15f,.762f);
            Bar=BattleHealthBar(WidgetTree,Color);BattlePlace(Canvas,Bar,Left,.765f,Left+.15f,.772f);
        };
        {UTextBlock* L=nullptr;UProgressBar* B=nullptr;Gauge(L,B,BattleEmber,.067f);BreakLabel=L;BreakBar=B;}
        {UTextBlock* L=nullptr;UProgressBar* B=nullptr;Gauge(L,B,BattleGold,.232f);MomentumLabel=L;MomentumBar=B;}
        {UTextBlock* L=nullptr;UProgressBar* B=nullptr;Gauge(L,B,BattleLimit,.397f);LimitLabel=L;LimitBar=B;}
        Gauges=BattleText(WidgetTree,TEXT(""),11,BattlePaper);BattlePlace(Canvas,Gauges,.56f,.742f,.668f,.782f);
        // Telegraph band: the cue owns its own plate so it never lands on portrait text.
        CueBand=BattlePanel(WidgetTree,FLinearColor(.006f,.012f,.018f,.90f),.6f);BattlePlace(Canvas,CueBand,.30f,.455f,.70f,.585f);
        CueText=BattleText(WidgetTree,TEXT(""),28,BattlePaper);CueText->SetJustification(ETextJustify::Center);BattlePlace(Canvas,CueText,.30f,.468f,.70f,.53f);
        BurnCost=BattleText(WidgetTree,TEXT(""),13,BattleGold);BurnCost->SetJustification(ETextJustify::Center);BattlePlace(Canvas,BurnCost,.31f,.535f,.69f,.578f);
        ImpactText=BattleText(WidgetTree,TEXT(""),36,BattlePaper);ImpactText->SetJustification(ETextJustify::Center);BattlePlace(Canvas,ImpactText,.32f,.45f,.68f,.57f);
        VictoryCard=BattlePanel(WidgetTree,FLinearColor(.008f,.016f,.022f,.95f),1.f);VictoryCard->SetPadding(FMargin(26,20));BattlePlace(Canvas,VictoryCard,.30f,.215f,.70f,.52f);
        auto* VictoryBox=WidgetTree->ConstructWidget<UVerticalBox>();VictoryCard->SetContent(VictoryBox);
        VictoryTitle=BattleText(WidgetTree,TEXT(""),30,BattlePaper);VictoryGrade=BattleText(WidgetTree,TEXT(""),15,BattleGold);VictoryBreakdown=BattleText(WidgetTree,TEXT(""),14,BattlePaper);
        for(UTextBlock* Line:{VictoryTitle.Get(),VictoryGrade.Get(),VictoryBreakdown.Get()}){Line->SetJustification(ETextJustify::Center);VictoryBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0,4));}

    }
    Draw();return Super::RebuildWidget();
}
void UMemoriaBattleEntryWidget::BindBattle(UMemoriaBattleEntrySubsystem* InBattle)
{
    UnbindBattle();Battle=InBattle;Age=0;ReturnAge=0;bFleeRequested=false;
    if(Battle)Battle->OnChanged.AddUObject(this,&UMemoriaBattleEntryWidget::Refresh);
    Refresh();
}
void UMemoriaBattleEntryWidget::UnbindBattle(){if(Battle)Battle->OnChanged.RemoveAll(this);Battle=nullptr;}
void UMemoriaBattleEntryWidget::NativeDestruct(){UnbindBattle();Super::NativeDestruct();}
void UMemoriaBattleEntryWidget::Refresh(){Display(Battle?Battle->GetView():FMemoriaBattleEntryView());}
void UMemoriaBattleEntryWidget::Display(const FMemoriaBattleEntryView& InView)
{
    if(View.Revision!=InView.Revision)bFleeRequested=false;
    if(!View.bReturning && InView.bReturning)ReturnAge=0;
    if(InView.ImpactSerial!=View.ImpactSerial && !InView.Hits.IsEmpty())
    {
        ImpactAge=0;ImpactDamage=0;bHitPlayer=false;
        for(const auto& Hit:InView.Hits)if(FMath::Abs(Hit.Amount)>=FMath::Abs(ImpactDamage)){ImpactDamage=Hit.Amount;bHitPlayer=Hit.Target==TEXT("Arrel");}
    }
    if(InView.bVictory||InView.bDefeat){PanelMode=0;if(!View.bVictory&&!View.bDefeat)Selected=0;}
    if(InView.WitnessProgress>View.WitnessProgress&&InView.Revision!=View.Revision)WitnessAge=0;
    if(InView.bVictory&&!View.bVictory)VictoryAge=0;
    View=InView;Draw();
}
void UMemoriaBattleEntryWidget::ClickAction(int32 Index,bool bChoice)
{
    if(bChoice){ChoiceSelected=Index;ConfirmIntent();return;}
    Selected=Index;PanelMode=0;ConfirmIntent();
}
void UMemoriaBattleEntryWidget::Navigate(const FKey& Key)
{
    if(View.bResolving||View.bReturning)return;
    if(Key==EKeys::Escape||Key==EKeys::Gamepad_FaceButton_Right){PanelMode=0;Draw();return;}
    const int32 Delta=(Key==EKeys::Left||Key==EKeys::Up||Key==EKeys::A||Key==EKeys::W)?-1:(Key==EKeys::Right||Key==EKeys::Down||Key==EKeys::D||Key==EKeys::S)?1:0;
    if(!Delta)return;
    if(PanelMode){const int32 Count=PanelMode==1?View.Memories.Num():View.Items.Num();if(Count)ChoiceSelected=(ChoiceSelected+Delta+Count)%Count;}
    else{const int32 Count=View.bVictory?1:View.bDefeat?2:6;Selected=(Selected+Delta+Count)%Count;}
    Draw();
}
void UMemoriaBattleEntryWidget::ConfirmIntent()
{
    if(!Battle||View.bResolving||View.bReturning||!View.bActive||Battle->GetRevision()!=View.Revision)return;
    if(View.bVictory){OnAction.ExecuteIfBound(TEXT("continue"),TEXT(""),View.Revision);return;}
    if(View.bDefeat){OnAction.ExecuteIfBound(Selected==0?TEXT("checkpoint"):TEXT("recover"),TEXT(""),View.Revision);return;}
    if(PanelMode)
    {
        const auto& Choices=PanelMode==1?View.Memories:View.Items;
        if(!Choices.IsValidIndex(ChoiceSelected)||!Choices[ChoiceSelected].bAvailable)return;
        const FString Action=PanelMode==1?TEXT("burn"):TEXT("item"),Id=Choices[ChoiceSelected].Id;
        PanelMode=0;OnAction.ExecuteIfBound(Action,Id,View.Revision);Draw();return;
    }
    // ATTACK, BURN, WITNESS, GUARD, ITEM, FLEE. Attack and Burn keep their original slots.
    if(Selected==1||Selected==4){PanelMode=Selected==1?1:2;ChoiceSelected=0;Draw();return;}
    if(Selected==5){OnFlee.ExecuteIfBound(View.Revision);return;}
    if(Selected==2&&View.WitnessProgress>=View.WitnessRequired)return;
    OnAction.ExecuteIfBound(Selected==0?TEXT("attack"):Selected==2?TEXT("witness"):TEXT("defend"),TEXT(""),View.Revision);
}
void UMemoriaBattleEntryWidget::DrawActions()
{
    const FString Reading=FString::Printf(TEXT("%s %d/%d"),View.bKo?TEXT("증언"):TEXT("WITNESS"),View.WitnessProgress,View.WitnessRequired);
    const TArray<FString> Labels=View.bKo?TArray<FString>{TEXT("공격"),TEXT("기억 연소"),Reading,TEXT("방어"),TEXT("아이템"),TEXT("도주")}:TArray<FString>{TEXT("ATTACK"),TEXT("BURN"),Reading,TEXT("GUARD"),TEXT("ITEM"),TEXT("FLEE")};
    for(int32 I=0;I<ActionButtons.Num();++I)
    {
        const bool Visible=View.bVictory?I==0:View.bDefeat?I<2:true;
        ActionButtons[I]->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        ActionLabels[I]->SetText(FText::FromString(View.bVictory?(View.bKo?TEXT("베르단으로"):TEXT("CONTINUE")):View.bDefeat?(I==0?(View.bKo?TEXT("체크포인트"):TEXT("CHECKPOINT")):(View.bKo?TEXT("베르단으로"):TEXT("VERDAN"))):Labels[I]));
        ActionButtons[I]->SetIsEnabled(View.bActive&&!View.bResolving&&!View.bReturning&&(I!=2||View.bVictory||View.bDefeat||View.WitnessProgress<View.WitnessRequired));
        ActionLabels[I]->SetColorAndOpacity(I==2&&!View.bVictory&&!View.bDefeat?BattleWitness:BattlePaper);
        ActionButtons[I]->SetBackgroundColor(I==Selected?FLinearColor(.42f,.28f,.13f):FLinearColor(.08f,.12f,.15f));
    }
    BoundaryNote->SetText(FText::FromString(View.bKo?TEXT("방향키  선택  ·  Enter / E  확인  ·  Esc  목록 닫기"):TEXT("ARROWS  select  ·  ENTER / E  confirm  ·  ESC  close list")));
    ChoicePanel->SetVisibility(PanelMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);ChoiceList->ClearChildren();
    if(PanelMode)
    {
        const auto& Choices=PanelMode==1?View.Memories:View.Items;
        for(int32 I=0;I<Choices.Num();++I)
        {
            const auto& C=Choices[I];auto* B=WidgetTree->ConstructWidget<UMemoriaBattleEntryButton>();B->Owner=this;B->Index=I;B->bChoice=true;B->SetIsEnabled(C.bAvailable);
            B->OnClicked.AddDynamic(B,&UMemoriaBattleEntryButton::Activate);B->SetBackgroundColor(I==ChoiceSelected?FLinearColor(.38f,.24f,.11f):FLinearColor(.035f,.05f,.065f));
            const FString Label=(PanelMode==1?FString::Printf(TEXT("G%d  "),5-C.Grade):FString())+C.Label+(C.bAvailable?TEXT(""):(View.bKo?TEXT("  · 사용 불가"):TEXT("  · unavailable")));
            auto* T=BattleText(WidgetTree,Label,15,C.bAvailable?(PanelMode==1?C.Accent:BattlePaper):BattleMuted);B->SetContent(T);auto* ChoiceSlot=ChoiceList->AddChildToVerticalBox(B);ChoiceSlot->SetPadding(FMargin(0,4));
        }
    }
    FString Status;
    for(const auto& S:View.PlayerStatuses)Status+=FString::Printf(TEXT("  %s %d"),S.Effect==0?(View.bKo?TEXT("독"):TEXT("POISON")):S.Effect==1?(View.bKo?TEXT("약화"):TEXT("WEAK")):(View.bKo?TEXT("화상"):TEXT("BURN")),S.Turns);
    Gauges->SetText(FText::FromString(FString::Printf(TEXT("%s %d%s"),View.bKo?TEXT("콤보"):TEXT("COMBO"),View.Combo,*Status)));
    BreakLabel->SetText(FText::FromString(View.BrokenTurns>0?(View.bKo?FString::Printf(TEXT("브레이크  %d턴"),View.BrokenTurns):FString::Printf(TEXT("BROKEN  %d"),View.BrokenTurns)):FString::Printf(TEXT("BREAK  %.0f"),View.BreakGauge)));
    BreakBar->SetPercent(View.BrokenTurns>0?1.f:FMath::Clamp(float(View.BreakGauge)/100.f,0.f,1.f));
    MomentumLabel->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0f"),View.bKo?TEXT("기세"):TEXT("MOMENTUM"),View.Momentum)));
    MomentumBar->SetPercent(FMath::Clamp(float(View.Momentum)/100.f,0.f,1.f));
    LimitLabel->SetText(FText::FromString(View.LimitGauge>=100?(View.bKo?TEXT("리밋  MAX"):TEXT("LIMIT  MAX")):FString::Printf(TEXT("%s  %.0f"),View.bKo?TEXT("리밋"):TEXT("LIMIT"),View.LimitGauge)));
    LimitBar->SetPercent(FMath::Clamp(float(View.LimitGauge)/100.f,0.f,1.f));
    const bool Fighting=View.bActive&&!View.bVictory&&!View.bDefeat;
    WitnessLabel->SetText(FText::FromString(Fighting?(View.bKo?TEXT("증언"):TEXT("WITNESS")):FString()));
    EchoText->SetText(FText::FromString(View.WitnessLine));
    EchoBand->SetVisibility(Fighting&&View.WitnessProgress>0&&!View.WitnessLine.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    VictoryCard->SetVisibility(View.bVictory?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(View.bVictory)
    {
        const auto& R=View.Reward;const bool Released=View.bResolvedByWitness;
        VictoryTitle->SetText(FText::FromString(Released?(View.bKo?TEXT("해방"):TEXT("RELEASED")):(View.bKo?TEXT("승리"):TEXT("VICTORY"))));
        VictoryTitle->SetColorAndOpacity(Released?BattleWitness:BattlePaper);
        VictoryGrade->SetText(FText::FromString(FString::Printf(TEXT("%s  %s   ·   %d"),View.bKo?TEXT("전투 등급"):TEXT("BATTLE GRADE"),*R.Grade,R.Score)));
        TArray<FString> Lines;
        Lines.Add(FString::Printf(TEXT("+%lld %s   ·   HP +%lld"),R.Grains,View.bKo?TEXT("그레인"):TEXT("Grains"),R.Heal));
        TArray<FString> Parts;
        if(R.PreservationBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),View.bKo?TEXT("보존"):TEXT("Preservation"),R.PreservationBonus));
        if(R.TacticalBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),View.bKo?TEXT("기록"):TEXT("Record"),R.TacticalBonus));
        if(R.ObjectiveBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),*View.ObjectiveTitle,R.ObjectiveBonus));
        if(R.GradeBonus>0)Parts.Add(FString::Printf(TEXT("%s %s +%lld"),View.bKo?TEXT("등급"):TEXT("Grade"),*R.Grade,R.GradeBonus));
        if(!Parts.IsEmpty())Lines.Add(FString::Join(Parts,TEXT("   ·   ")));
        if(R.FocusGained>0)Lines.Add(FString::Printf(TEXT("%s +%lld"),View.bKo?TEXT("필드 집중"):TEXT("Field Focus"),R.FocusGained));
        if(!R.Item.IsEmpty())Lines.Add(FString::Printf(TEXT("%s %s"),View.bKo?TEXT("획득:"):TEXT("Found:"),*R.Item));
        // Source aftermath line for a fight ended by listening instead of burning.
        if(Released)Lines.Add(View.bKo?TEXT("\n이곳에서 태운 것은 없다. 이름은 제자리로 돌아갔다."):TEXT("\nNothing was burned here. The name went back where it belonged."));
        VictoryBreakdown->SetText(FText::FromString(FString::Join(Lines,TEXT("\n"))));
    }
    if(View.bDefeat)BattleLog->SetText(FText::FromString(View.bKo?TEXT("아렐이 쓰러졌다. 체크포인트를 불러오거나, 잃은 기억을 안고 베르단으로 돌아간다."):TEXT("Arrel falls. Load a checkpoint, or return to Verdan carrying the cost of the memories burned.")));
}
void UMemoriaBattleEntryWidget::Draw()
{
    if(!Heading)return;
    Heading->SetText(FText::FromString(View.EnvironmentName));
    Turn->SetText(FText::FromString(View.bReturning?(View.bKo?TEXT("돌아가는 중"):TEXT("RETURNING")):View.bVictory?(View.bKo?TEXT("승리"):TEXT("VICTORY")):View.bDefeat?(View.bKo?TEXT("쓰러졌습니다"):TEXT("DEFEAT")):View.bResolving?(View.bKo?TEXT("행동 진행 중"):TEXT("ACTION IN PROGRESS")):(View.bKo?TEXT("당신의 턴"):TEXT("YOUR TURN"))));
    PlayerName->SetText(FText::FromString(View.bKo?TEXT("\uc544\ub810"):TEXT("ARREL")));
    PlayerHealth->SetText(FText::FromString(FString::Printf(TEXT("HP  %lld / %lld"),View.PlayerHp,View.PlayerMaxHp)));
    PlayerBar->SetPercent(View.PlayerMaxHp>0?FMath::Clamp(float(View.PlayerHp)/View.PlayerMaxHp,0.f,1.f):0.f);
    AllyName->SetText(FText::FromString(View.bKo?TEXT("\uc5d8\ub9ac\uc544"):TEXT("ELIA")));
    AllyFrame->SetVisibility(View.bEliaInParty?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    AllyName->SetVisibility(View.bEliaInParty?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    EnemyName->SetText(FText::FromString(View.EnemyName));EnemyHealth->SetText(FText::FromString(FString::Printf(TEXT("HP  %lld / %lld"),View.EnemyHp,View.EnemyMaxHp)));
    EnemyBar->SetPercent(View.EnemyMaxHp>0?FMath::Clamp(float(View.EnemyHp)/View.EnemyMaxHp,0.f,1.f):0.f);
    ObjectiveTitle->SetText(FText::FromString(View.ObjectiveTitle));
    ObjectiveBody->SetText(FText::FromString(View.ObjectiveDescription+(View.ObjectiveProgress.IsEmpty()?FString():TEXT("\n")+View.ObjectiveProgress)));
    if(!View.bObjectiveSupported)ObjectiveBody->SetText(FText::FromString(View.bKo?TEXT("이번 단계 미지원 · 보상 없음"):TEXT("Unavailable in this chapter slice · no reward")));
    else if(View.bObjectiveComplete||View.bObjectiveFailed)ObjectiveBody->SetText(FText::FromString(View.bObjectiveComplete?(View.bKo?TEXT("목표 달성"):TEXT("COMPLETE")):(View.bKo?TEXT("목표 실패"):TEXT("FAILED"))));
    Modifier->SetText(FText::FromString(View.ModifierName+(View.ModifierDescription.IsEmpty()?FString():TEXT("\n")+View.ModifierDescription)));
    BattleLog->SetText(FText::FromString(FString::Join(View.Logs,TEXT("\n"))));
    DrawActions();
    FString EnemySource=View.EnemyImageSource;
    if(EnemySource.IsEmpty() && View.EnemyIndex==1)
    {
        EnemySource=MemoriaBattleEntryArt::MarketThiefStudySource();
        if(!MemoriaBattleEntryArt::Load(EnemySource))EnemySource=MemoriaBattleEntryArt::MarketThiefSource();
    }
    if(View.EnemyIndex==0 && EnemySource==TEXT("res://assets/cg/generated/battle_stage_v2/enemy_ash_hound_stage_v1.png"))
    {
        const FString Study=MemoriaBattleEntryArt::AlleyRatStudySource();
        if(MemoriaBattleEntryArt::Load(Study))EnemySource=Study;
    }
    EnemyArt->SetBrushFromTexture(MemoriaBattleEntryArt::Load(EnemySource),true);
    // The source procedural fallback is a provisional 128px sprite. Keep it at
    // native scale inside the plate instead of magnifying it to portrait size.
    if(auto* Fit=Cast<UScaleBox>(EnemyArt->GetParent()))
        Fit->SetStretchDirection(EnemySource==MemoriaBattleEntryArt::MarketThiefSource()?EStretchDirection::DownOnly:EStretchDirection::Both);
    const FString Background=View.BackgroundSource.IsEmpty()?TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png"):View.BackgroundSource;
    Backdrop->SetBrushFromTexture(MemoriaBattleEntryArt::Load(Background),true);
    if(LogScroll)LogScroll->ScrollToEnd();
}
FReply UMemoriaBattleEntryWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=Event.GetKey();
    if(BattleOwnedKey(Key))
    {
        OnConsumedKey.ExecuteIfBound(Key,Event.IsRepeat()?IE_Repeat:IE_Pressed);
        if(!Event.IsRepeat()){if(BattleConfirmKey(Key))ConfirmIntent();else Navigate(Key);}
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
FReply UMemoriaBattleEntryWidget::NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(BattleOwnedKey(Event.GetKey())){OnConsumedKey.ExecuteIfBound(Event.GetKey(),IE_Released);return FReply::Handled();}
    return Super::NativeOnKeyUp(Geometry,Event);
}
void UMemoriaBattleEntryWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);Age+=DeltaTime;
    if(Battle && Battle->GetRevision()!=View.Revision)Refresh();
    if(View.bReturning)ReturnAge+=DeltaTime;
    // Visual timing never advances combat or schedules field return.
    const float Entry=FMath::Clamp(Age/.22f,0.f,1.f);
    const float Exit=View.bReturning?1.f-.65f*FMath::Clamp(ReturnAge/.4f,0.f,1.f):1.f;
    SetRenderOpacity(Entry*Exit);ImpactAge+=DeltaTime;
    const float Stop=ImpactDamage>=200?.12f:ImpactDamage>=80?.08f:ImpactDamage>=30?.05f:0.f;
    const float T=FMath::Max(0.f,ImpactAge-Stop);const float Force=FMath::Clamp(float(FMath::Abs(ImpactDamage))/60.f,.5f,3.f);
    const float Shake=T<.23f?FMath::Sin(T*110.f)*Force*(1-T/.23f):0.f;
    SetRenderTranslation(FVector2D(Shake,Shake*.45f));
    const float Punch=ImpactDamage>=200&&T<.2f?1.f+.035f*(1.f-T/.2f):1.f;SetRenderScale(FVector2D(Punch));
    WitnessAge+=DeltaTime;VictoryAge+=DeltaTime;
    const bool Burning=View.bResolving&&View.Telegraph==TEXT("burn"),Reading=View.bResolving&&View.Telegraph==TEXT("witness");
    if(CueBand)CueBand->SetVisibility(Burning||Reading?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(CueText){CueText->SetText(FText::FromString(Burning?(View.bKo?TEXT("기억 연소"):TEXT("MEMORY BURN")):Reading?(View.bKo?TEXT("증언"):TEXT("WITNESS")):FString()));CueText->SetColorAndOpacity(Reading?BattleWitness:BattleEmber);}
    if(BurnCost)
    {
        BurnCost->SetText(FText::FromString(Burning?(View.bKo?TEXT("힘은 오르지만, 대가는 전투 뒤에도 남습니다"):TEXT("Power rises. The cost remains after battle."))
            :Reading?(View.bKo?TEXT("끝까지 들어준다. 기억은 쓰지 않는다."):TEXT("Hear it out. No memory is spent.")):FString()));
        BurnCost->SetColorAndOpacity(Reading?BattleWitness:BattleGold);
    }
    // A released echo fades into archive ink instead of breaking apart.
    if(EnemyArt)EnemyArt->SetColorAndOpacity(View.bVictory&&View.bResolvedByWitness?FMath::Lerp(FLinearColor::White,FLinearColor(.55f,.86f,.84f,.32f),FMath::Clamp(VictoryAge/.8f,0.f,1.f)):FLinearColor::White);
    UImage* Target=bHitPlayer?PlayerArt.Get():EnemyArt.Get();
    if(PlayerArt)PlayerArt->SetRenderScale(FVector2D(1,1));if(EnemyArt)EnemyArt->SetRenderScale(FVector2D(1,1));
    if(Target && T<.18f && ImpactDamage>0)Target->SetRenderScale(T<.04f?FVector2D(1.15,.85):T<.10f?FVector2D(.95,1.05):FVector2D(1,1));
    if(ImpactText){ImpactText->SetText(FText::FromString(ImpactAge<1.f&&ImpactDamage!=0?FString::Printf(TEXT("%s%lld"),ImpactDamage<0?TEXT("+"):TEXT(""),FMath::Abs(ImpactDamage)):FString()));ImpactText->SetRenderTranslation(FVector2D(bHitPlayer?-260:260,-T*70));ImpactText->SetRenderOpacity(FMath::Clamp(1.f-T,0.f,1.f));}
    // Damage numbers never sit on top of a telegraph or the result card.
    if(ImpactText&&(Burning||Reading||View.bVictory))ImpactText->SetRenderOpacity(0.f);

}
int32 UMemoriaBattleEntryWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements,int32 LayerId,const FWidgetStyle& Style,bool bParentEnabled) const
{
    const int32 LastLayer=Super::NativePaint(Args,Geometry,CullingRect,Elements,LayerId,Style,bParentEnabled);
    const FVector2D Size=Geometry.GetLocalSize();const auto* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    for(int32 I=0;I<8;++I)
    {
        const float Phase=FMath::Fmod(Age*.014f+I*.137f,1.f);
        const FVector2f Position(float(Size.X)*(I%2?.985f:.014f),float(Size.Y)*(.23f+.58f*(1-Phase)));
        const float Alpha=.13f*FMath::Sin(Phase*PI)*Style.GetColorAndOpacityTint().A*GetRenderOpacity();
        FSlateDrawElement::MakeBox(Elements,LastLayer+1,Geometry.ToPaintGeometry(FVector2f(1.5f,3.f),FSlateLayoutTransform(Position)),Brush,
            ESlateDrawEffect::None,FLinearColor(.76f,.45f,.18f,Alpha));
    }
    const float Opacity=Style.GetColorAndOpacityTint().A*GetRenderOpacity();
    const bool Fighting=View.bActive&&!View.bVictory&&!View.bDefeat;
    if(Fighting)
    {
        // Witness pips beside the label: filled once heard, dim while still unread.
        const float Pip=float(Size.Y)*.018f;
        for(int32 I=0;I<View.WitnessRequired;++I)
        {
            const FVector2f P(float(Size.X)*.812f+I*Pip*1.9f,float(Size.Y)*.803f);
            const bool Heard=I<View.WitnessProgress;
            FSlateDrawElement::MakeBox(Elements,LastLayer+1,Geometry.ToPaintGeometry(FVector2f(Pip,Pip),FSlateLayoutTransform(P)),Brush,ESlateDrawEffect::None,
                Heard?FLinearColor(BattleWitness.R,BattleWitness.G,BattleWitness.B,.95f*Opacity):FLinearColor(.30f,.44f,.45f,.55f*Opacity));
            if(!Heard)FSlateDrawElement::MakeBox(Elements,LastLayer+1,Geometry.ToPaintGeometry(FVector2f(Pip-4,Pip-4),FSlateLayoutTransform(P+FVector2f(2,2))),Brush,ESlateDrawEffect::None,FLinearColor(.01f,.02f,.025f,.95f*Opacity));
        }
    }
    const FVector2f EnemyOrigin(float(Size.X)*.688f,float(Size.Y)*.209f),EnemySize(float(Size.X)*.257f,float(Size.Y)*.478f);
    if(WitnessAge<.6f&&!View.bDefeat)
        FSlateDrawElement::MakeBox(Elements,LastLayer+2,Geometry.ToPaintGeometry(EnemySize,FSlateLayoutTransform(EnemyOrigin)),Brush,ESlateDrawEffect::None,
            FLinearColor(BattleWitness.R,BattleWitness.G,BattleWitness.B,.20f*(1-WitnessAge/.6f)*Opacity));
    if(View.bVictory&&View.bResolvedByWitness&&VictoryAge<3.f)
        for(int32 I=0;I<14;++I)
        {
            // Released names drift upward out of the enemy plate.
            const float T=FMath::Fmod(VictoryAge*.45f+I*.071f,1.f);
            const FVector2f P(EnemyOrigin.X+EnemySize.X*(.08f+.84f*FMath::Frac(I*.618f)),EnemyOrigin.Y+EnemySize.Y*(1-T));
            FSlateDrawElement::MakeBox(Elements,LastLayer+2,Geometry.ToPaintGeometry(FVector2f(2.f,4.f),FSlateLayoutTransform(P)),Brush,ESlateDrawEffect::None,
                FLinearColor(.70f,.95f,.92f,.55f*FMath::Sin(T*PI)*FMath::Clamp(1.5f-VictoryAge/2.f,0.f,1.f)*Opacity));
        }
    if(ImpactDamage>=80 && ImpactAge<.12f)FSlateDrawElement::MakeBox(Elements,LastLayer+2,Geometry.ToPaintGeometry(),Brush,ESlateDrawEffect::None,FLinearColor(1.f,.78f,.45f,.20f*(1-ImpactAge/.12f)));
    return LastLayer+2;
}
FString UMemoriaBattleEntryWidget::VisibleText() const
{
    FString Text=View.EnvironmentName+TEXT("\n")+View.EnemyName+TEXT("\n")+View.ObjectiveTitle+TEXT("\n")+View.ObjectiveDescription+TEXT("\n")+FString::Join(View.Logs,TEXT("\n"));
    if(PlayerHealth)Text+=TEXT("\n")+PlayerHealth->GetText().ToString();if(EnemyHealth)Text+=TEXT("\n")+EnemyHealth->GetText().ToString();
    if(BoundaryNote)Text+=TEXT("\n")+BoundaryNote->GetText().ToString();if(FleeLabel)Text+=TEXT("\n")+FleeLabel->GetText().ToString();
    for(const UTextBlock* Extra:{WitnessLabel.Get(),EchoText.Get(),BreakLabel.Get(),MomentumLabel.Get(),LimitLabel.Get()})if(Extra)Text+=TEXT("\n")+Extra->GetText().ToString();
    if(IsVictoryCardVisible())Text+=TEXT("\n")+VictoryTitle->GetText().ToString()+TEXT("\n")+VictoryGrade->GetText().ToString()+TEXT("\n")+VictoryBreakdown->GetText().ToString();
    for(const auto& Label:ActionLabels)if(Label)Text+=TEXT("\n")+Label->GetText().ToString();return Text;
}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedBackdrop() const{return BattleTexture(Backdrop);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedPlayerArtwork() const{return BattleTexture(PlayerArt);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedEnemyArtwork() const{return BattleTexture(EnemyArt);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedAllyArtwork() const{return View.bEliaInParty?BattleTexture(AllyArt):nullptr;}
bool UMemoriaBattleEntryWidget::IsFleeEnabled() const{return FleeButton && FleeButton->GetIsEnabled();}
bool UMemoriaBattleEntryWidget::IsWitnessEnabled() const{return ActionButtons.IsValidIndex(2)&&ActionButtons[2]->GetIsEnabled();}
bool UMemoriaBattleEntryWidget::IsVictoryCardVisible() const{return VictoryCard&&VictoryCard->GetVisibility()!=ESlateVisibility::Collapsed;}
