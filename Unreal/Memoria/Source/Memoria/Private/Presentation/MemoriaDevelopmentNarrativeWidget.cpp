#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Presentation/MemoriaShopWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
namespace
{
const FLinearColor Gold(.66f,.49f,.27f), Paper(.88f,.85f,.77f), Ink(.006f,.008f,.012f,.97f);
void Position(UWidget* Widget, float L,float T,float R,float B)
{
    auto* Slot=Cast<UCanvasPanelSlot>(Widget->Slot); Slot->SetAnchors(FAnchors(L,T,R,B)); Slot->SetOffsets(FMargin(0));
}
void Place(UCanvasPanel* Canvas,UWidget* Widget,float L,float T,float R,float B)
{ Canvas->AddChildToCanvas(Widget); Position(Widget,L,T,R,B); }
UTextBlock* Text(UWidgetTree* Tree,int32 Size,FLinearColor Color)
{
    auto* W=Tree->ConstructWidget<UTextBlock>();W->SetAutoWrapText(true);W->SetColorAndOpacity(Color);
    W->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),Size));return W;
}
FButtonStyle ChoiceStyle(bool Selected)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateRoundedBoxBrush(Selected?FLinearColor(.06f,.044f,.023f):FLinearColor(.012f,.016f,.022f),2.f,Selected?Gold:FLinearColor(.12f,.12f,.12f),1.f));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.06f,.044f,.023f),2.f,Gold,1.f));Style.SetPressed(Style.Hovered);return Style;
}
}
void UMemoriaNarrativeChoiceButton::Choose(){if(Owner)Owner->Choose(ChoiceIndex);}
TSharedRef<SWidget> UMemoriaDevelopmentNarrativeWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        Illustration=WidgetTree->ConstructWidget<UCanvasPanel>();Place(Canvas,Illustration,0,0,1,1);
        auto* Black=WidgetTree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor::Black);Place(Illustration,Black,0,0,1,1);
        // vn_scene.gd crossfade: the next CG fades in over the previous one.
        auto* PreviousFit=WidgetTree->ConstructWidget<UScaleBox>();PreviousFit->SetStretch(EStretch::ScaleToFit);Place(Illustration,PreviousFit,0,0,1,1);
        BackdropPrevious=WidgetTree->ConstructWidget<UImage>();PreviousFit->AddChild(BackdropPrevious);
        BackdropFit=WidgetTree->ConstructWidget<UScaleBox>();BackdropFit->SetStretch(EStretch::ScaleToFit);Place(Illustration,BackdropFit,0,0,1,1);
        Backdrop=WidgetTree->ConstructWidget<UImage>();BackdropFit->AddChild(Backdrop);
        Illustration->SetVisibility(ESlateVisibility::HitTestInvisible);
        Flash=WidgetTree->ConstructWidget<UBorder>();Flash->SetBrushColor(FLinearColor::Transparent);Place(Canvas,Flash,0,0,1,1);
        Flash->SetVisibility(ESlateVisibility::Collapsed);
        NarrativePanel=WidgetTree->ConstructWidget<UBorder>();NarrativePanel->SetBrush(FSlateRoundedBoxBrush(Ink,3.f,Gold,1.f));
        Place(Canvas,NarrativePanel,.055f,.66f,.945f,.96f);
        auto* Interior=WidgetTree->ConstructWidget<UCanvasPanel>();NarrativePanel->SetContent(Interior);NarrativePanel->SetPadding(FMargin(0));
        Speaker=Text(WidgetTree,18,Gold);Place(Interior,Speaker,.045f,.085f,.94f,.23f);
        BodyScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,BodyScroll,.045f,.28f,.95f,.78f);
        Message=Text(WidgetTree,21,Paper);BodyScroll->AddChild(Message);
        Hint=Text(WidgetTree,12,FLinearColor(.42f,.43f,.44f));Place(Interior,Hint,.045f,.84f,.95f,.98f);
        ChoiceNote=Text(WidgetTree,15,FLinearColor(.62f,.6f,.56f));Place(Interior,ChoiceNote,.045f,.2f,.95f,.3f);
        ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,ChoiceScroll,.04f,.26f,.96f,.83f);
        Choices=WidgetTree->ConstructWidget<UVerticalBox>();ChoiceScroll->AddChild(Choices);
        PortraitFrame=WidgetTree->ConstructWidget<UBorder>();PortraitFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Black,1.f,Gold,1.f));PortraitFrame->SetPadding(FMargin(2));
        Place(Interior,PortraitFrame,.04f,.28f,.155f,.89f);
        auto* PortraitFit=WidgetTree->ConstructWidget<UScaleBox>();PortraitFit->SetStretch(EStretch::ScaleToFit);PortraitFrame->SetContent(PortraitFit);
        Portrait=WidgetTree->ConstructWidget<UImage>();PortraitFit->AddChild(Portrait);
        PortraitFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
        Location=Text(WidgetTree,13,Gold);Place(Canvas,Location,.058f,.045f,.9f,.10f);
        ShopWidget=WidgetTree->ConstructWidget<UMemoriaShopWidget>();Place(Canvas,ShopWidget,0,0,1,1);
        // SceneFlow._show_chapter_ledger: a top-centre panel over the scene.
        Ledger=WidgetTree->ConstructWidget<USizeBox>();Ledger->SetMinDesiredWidth(560.f);Canvas->AddChildToCanvas(Ledger);
        if(auto* LedgerSlot=Cast<UCanvasPanelSlot>(Ledger->Slot)){LedgerSlot->SetAnchors(FAnchors(.5f,.064f));LedgerSlot->SetAlignment(FVector2D(.5f,0.f));LedgerSlot->SetAutoSize(true);}
        auto* LedgerFrame=WidgetTree->ConstructWidget<UBorder>();LedgerFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.035f,.03f,.045f,.92f),4.f,FLinearColor(.62f,.5f,.3f,.7f),1.f));
        LedgerFrame->SetPadding(FMargin(18,14));Ledger->SetContent(LedgerFrame);
        auto* LedgerRows=WidgetTree->ConstructWidget<UVerticalBox>();LedgerFrame->SetContent(LedgerRows);
        LedgerTitle=Text(WidgetTree,20,FLinearColor(.9f,.78f,.5f));LedgerBody=Text(WidgetTree,16,FLinearColor(.82f,.78f,.72f));LedgerThread=Text(WidgetTree,16,FLinearColor(.45f,.85f,.8f));
        for(auto* Row:{LedgerTitle.Get(),LedgerBody.Get(),LedgerThread.Get()}){Row->SetJustification(ETextJustify::Center);LedgerRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,2));}
        Ledger->SetVisibility(ESlateVisibility::Collapsed);
    }
    Refresh(); return Super::RebuildWidget();
}
void UMemoriaDevelopmentNarrativeWidget::Display(const FMemoriaNarrativeView& InView)
{
    const int32 Previous=SelectedOriginalIndex();const bool SameStep=View.Header==InView.Header;
    const bool First=!bSeenView;bSeenView=true;
    View=InView;Selection=0;
    if(SameStep)for(int32 I=0;I<View.Choices.Num();++I)if(View.Choices[I].OriginalIndex==Previous)Selection=I;
    Refresh();if(!SameStep && BodyScroll)BodyScroll->ScrollToStart();
    if(!View.CueKey.IsEmpty() && View.CueKey!=ActiveCue)StartCues();
    // A ledger raised before this widget existed is not replayed.
    if(First)ShownLedger=View.LedgerSerial;
    else if(View.LedgerSerial!=ShownLedger && Ledger)
    {
        ShownLedger=View.LedgerSerial;LedgerAge=0.f;
        LedgerTitle->SetText(FText::FromString(View.LedgerTitle));
        TArray<FString> Rows=View.LedgerLines;const FString Thread=Rows.Num()?Rows.Pop():FString();
        LedgerBody->SetText(FText::FromString(FString::Join(Rows,TEXT("\n"))));LedgerThread->SetText(FText::FromString(Thread));
        LedgerThread->SetColorAndOpacity(View.bLedgerThreadHolds?FLinearColor(.45f,.85f,.8f):FLinearColor(.85f,.55f,.4f));
    }
    ApplyCues();
}
void UMemoriaDevelopmentNarrativeWidget::StartCues()
{
    ActiveCue=View.CueKey;CueAge=0.f;
    const float Fade=View.bStepHasCg?FMath::Max(0.f,View.CgFadeSeconds):0.f;
    CrossFade=bPendingCrossFade?Fade:0.f;bPendingCrossFade=false;
    // _play_cinematic_step waits for 72% of the CG fade before the flash and nudge.
    CueDelay=View.bStepHasCg?FMath::Max(.05f,Fade*.72f):0.f;
    if(View.bStepHasCg)
    {
        // _swap_cg starts the motion once the crossfade completes; ambient pans a fixed offset per step.
        Motion=View.CgMotion;MotionAge=-CrossFade;
        const uint32 Hash=GetTypeHash(ActiveCue);Pan=FVector2D(float(Hash%37)-18.f,float((Hash/37)%21)-10.f);
    }
    FlashDuration=0.f;NudgeStrength=0.f;
    if(View.Impact==TEXT("void")){FlashColor=FLinearColor(.46f,.22f,.78f,.28f);FlashDuration=.65f;}
    else if(View.Impact==TEXT("memory")){FlashColor=FLinearColor(1.f,.72f,.32f,.52f);FlashDuration=.9f;}
    else if(View.Impact==TEXT("heavy")){FlashColor=FLinearColor(.92f,.94f,1.f,.38f);FlashDuration=.48f;}
    if(FlashDuration>0.f)NudgeStrength=View.Impact==TEXT("heavy")?4.f:2.5f;
}
void UMemoriaDevelopmentNarrativeWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
    Super::NativeTick(MyGeometry,InDeltaTime);
    // Cues run on game time like the source tweens: they hold while the world is paused.
    const UWorld* World=GetWorld();AdvancePresentation(World?World->GetDeltaSeconds():InDeltaTime);
}
void UMemoriaDevelopmentNarrativeWidget::AdvancePresentation(float DeltaSeconds)
{
    const float Dt=FMath::Max(0.f,DeltaSeconds);CueAge+=Dt;MotionAge+=Dt;if(LedgerAge>=0.f)LedgerAge+=Dt;ApplyCues();
}
FMemoriaPresentationProbe UMemoriaDevelopmentNarrativeWidget::GetPresentationProbe() const { return Probe; }
void UMemoriaDevelopmentNarrativeWidget::ApplyCues()
{
    if(!Backdrop)return;
    const float FadeT=CrossFade>0.f?FMath::Clamp(CueAge/CrossFade,0.f,1.f):1.f;
    Backdrop->SetRenderOpacity(FadeT);BackdropPrevious->SetRenderOpacity(1.f-FadeT);
    BackdropPrevious->SetVisibility(FadeT<1.f?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    // _start_cg_motion / _start_ken_burns scale curves.
    const float T=FMath::Max(0.f,MotionAge);float Scale=1.f;FVector2D Offset=FVector2D::ZeroVector;
    if(Motion==TEXT("pull_back"))Scale=1.09f-.09f*FMath::Sin(FMath::Min(T/8.f,1.f)*HALF_PI);
    else if(Motion==TEXT("push_in"))Scale=1.f+.085f*(.5f-.5f*FMath::Cos(PI*FMath::Min(T/7.f,1.f)));
    else if(Motion==TEXT("strike"))Scale=1.045f-.045f*(1.f-FMath::Pow(2.f,-10.f*FMath::Min(T/1.1f,1.f)));
    else if(!Motion.IsEmpty() && Motion!=TEXT("still"))
    { const float U=.5f-.5f*FMath::Cos(PI*FMath::Min(T/11.f,1.f));Scale=1.f+.05f*U;Offset=Pan*U; }
    // _play_cinematic_nudge: two quick offsets, then a sine return.
    FVector2D Nudge=FVector2D::ZeroVector;const float N=CueAge-CueDelay;
    if(NudgeStrength>0.f && N>=0.f)
    {
        const FVector2D A(-NudgeStrength,NudgeStrength*.35f),B(NudgeStrength,-NudgeStrength*.25f);
        if(N<.045f)Nudge=A*(N/.045f);else if(N<.1f)Nudge=FMath::Lerp(A,B,(N-.045f)/.055f);
        else if(N<.21f)Nudge=B*(.5f+.5f*FMath::Cos(PI*(N-.1f)/.11f));
    }
    BackdropFit->SetRenderTransform(FWidgetTransform(Offset+Nudge,FVector2D(Scale,Scale),FVector2D::ZeroVector,0.f));
    // The flash overlay decays on an exponential ease-out.
    const float Alpha=FlashDuration>0.f && N>=0.f && N<FlashDuration?FlashColor.A*FMath::Pow(2.f,-10.f*N/FlashDuration):0.f;
    Flash->SetBrushColor(FLinearColor(FlashColor.R,FlashColor.G,FlashColor.B,Alpha));
    Flash->SetVisibility(Alpha>0.f?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    // Ledger: 0.4 s in, 4.6 s hold, 0.6 s out.
    float LedgerAlpha=0.f;
    if(LedgerAge>=0.f){LedgerAlpha=LedgerAge<.4f?LedgerAge/.4f:LedgerAge<5.f?1.f:FMath::Max(0.f,1.f-(LedgerAge-5.f)/.6f);if(LedgerAge>=5.6f)LedgerAge=-1.f;}
    Ledger->SetRenderOpacity(LedgerAlpha);Ledger->SetVisibility(LedgerAlpha>0.f?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    Probe.FlashAlpha=Alpha;Probe.BackdropScale=Scale;Probe.BackdropOpacity=FadeT;Probe.LedgerAlpha=LedgerAlpha;Probe.Nudge=Nudge;Probe.Motion=Motion;
    Probe.LedgerText=LedgerAlpha>0.f?LedgerTitle->GetText().ToString()+TEXT("\n")+LedgerBody->GetText().ToString()+TEXT("\n")+LedgerThread->GetText().ToString():FString();
    Probe.bDistortedStyle=View.bDistorted;Probe.bSystemStyle=View.bSystemLog;
}
void UMemoriaDevelopmentNarrativeWidget::Navigate(int32 Direction)
{
    if(View.bShopPresentation && ShopWidget){ShopWidget->Navigate(Direction);return;}
    if(!View.bPaused && !View.Choices.IsEmpty())
    {
        Selection=(Selection+Direction+View.Choices.Num())%View.Choices.Num();
        for(int32 I=0;I<Buttons.Num();++I)Buttons[I]->SetStyle(ChoiceStyle(I==Selection));
        if(Buttons.IsValidIndex(Selection))ChoiceScroll->ScrollWidgetIntoView(Buttons[Selection],false);
    }
}
int32 UMemoriaDevelopmentNarrativeWidget::SelectedOriginalIndex() const
{ return View.Choices.IsValidIndex(Selection)?View.Choices[Selection].OriginalIndex:INDEX_NONE; }
void UMemoriaDevelopmentNarrativeWidget::Choose(int32 Index)
{ if(!View.bPaused && View.Choices.IsValidIndex(Index)){Selection=Index;ConfirmIntent();} }
void UMemoriaDevelopmentNarrativeWidget::ConfirmIntent()
{ if(View.bShopPresentation && ShopWidget)ShopWidget->ConfirmIntent();else OnConfirm.ExecuteIfBound(SelectedOriginalIndex()); }
FString UMemoriaDevelopmentNarrativeWidget::VisibleText() const
{
    if(View.bShopPresentation && ShopWidget)return ShopWidget->VisibleText();
    if(!Message)return FString();
    FString Out=Location->GetText().ToString()+TEXT("\n")+Speaker->GetText().ToString();
    if(BodyScroll->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+Message->GetText().ToString();
    if(ChoiceNote->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+ChoiceNote->GetText().ToString();
    if(ChoiceScroll->GetVisibility()!=ESlateVisibility::Collapsed)for(const auto& C:View.Choices){Out+=TEXT("\n")+C.Text;if(!C.Effect.IsEmpty())Out+=TEXT("\n")+C.Effect;}
    return Out+TEXT("\n")+Hint->GetText().ToString();
}
UTexture2D* UMemoriaDevelopmentNarrativeWidget::DisplayedBackdrop() const
{ return Backdrop && Illustration->GetVisibility()!=ESlateVisibility::Collapsed ? Cast<UTexture2D>(Backdrop->GetBrush().GetResourceObject()):nullptr; }
UTexture2D* UMemoriaDevelopmentNarrativeWidget::DisplayedPortrait() const
{ return Portrait && PortraitFrame->GetVisibility()!=ESlateVisibility::Collapsed ? Cast<UTexture2D>(Portrait->GetBrush().GetResourceObject()):nullptr; }
void UMemoriaDevelopmentNarrativeWidget::Refresh()
{
    if(!Message)return;
    NarrativePanel->SetVisibility(View.bShopPresentation?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    ShopWidget->SetVisibility(View.bShopPresentation?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Location->SetVisibility(View.bShopPresentation || View.bCompactStatus?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    Illustration->SetVisibility(View.bShopPresentation || View.bCompactStatus || View.bDevelopmentStop?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    if(View.bShopPresentation){PortraitFrame->SetVisibility(ESlateVisibility::Collapsed);ShopWidget->Display(View.Shop);return;}
    auto* CG=MemoriaNarrativeArtwork::Load(View.BackdropSource);
    if(Backdrop->GetBrush().GetResourceObject()!=CG)
    {
        // A new step's CG crossfades from the previous picture; a cleared picture does not.
        auto* Old=Cast<UTexture2D>(Backdrop->GetBrush().GetResourceObject());
        bPendingCrossFade=Old && CG && !View.CueKey.IsEmpty() && View.CueKey!=ActiveCue;
        if(bPendingCrossFade)BackdropPrevious->SetBrushFromTexture(Old,true);
        Backdrop->SetBrushFromTexture(CG,true);
    }
    auto* Face=View.Speaker.IsEmpty() || View.bPaused || View.bCompactStatus || View.bDevelopmentStop || !View.Choices.IsEmpty()?nullptr:MemoriaNarrativeArtwork::Load(View.PortraitSource);
    Portrait->SetBrushFromTexture(Face,true);PortraitFrame->SetVisibility(Face?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    const bool Right=View.PortraitSide==TEXT("right");
    Position(PortraitFrame,Right?.845f:.04f,.28f,Right?.96f:.155f,.89f);
    const bool Choosing=!View.Choices.IsEmpty() && !View.bPaused;
    // The exploration status grows with its lines (quest tracker, toasts) instead of clipping them.
    float CompactBottom=.19f;
    if(View.bCompactStatus)
    {
        TArray<FString> Rows;View.Body.ParseIntoArray(Rows,TEXT("\n"),false);int32 Lines=0;
        for(const auto& Row:Rows)Lines+=FMath::Max(1,FMath::DivideAndRoundUp(Row.Len(),44));
        CompactBottom=FMath::Clamp(.085f+.047f*Lines,.19f,.46f);
    }
    Position(NarrativePanel,View.bCompactStatus?.035f:.055f,View.bCompactStatus?.035f:View.bDevelopmentStop?.14f:Choosing?.49f:.66f,View.bCompactStatus?.37f:.945f,View.bCompactStatus?CompactBottom:.96f);
    Position(BodyScroll,Face && !Right?.185f:.045f,View.bCompactStatus?.30f:.28f,Face && Right?.815f:.95f,View.bCompactStatus?.96f:.78f);
    Position(Hint,Face && !Right?.185f:.045f,.84f,Face && Right?.815f:.95f,.98f);
    Location->SetText(FText::FromString(View.bDevelopmentStop?View.Header:View.LocationTitle));
    Speaker->SetText(FText::FromString(View.bCompactStatus?View.Header:View.bPaused?TEXT("PAUSED"):Choosing?(View.ChoiceTitle.IsEmpty()?TEXT("YOUR CHOICE"):View.ChoiceTitle):View.Speaker.IsEmpty()?TEXT("MEMORIA"):View.Speaker));
    // _show_system_log tints the name cyan; distorted lines read in a colder, bruised tone.
    Speaker->SetColorAndOpacity(View.bSystemLog && !Choosing?FLinearColor(.5f,.85f,.95f):Gold);
    Message->SetColorAndOpacity(View.bDistorted?FLinearColor(.78f,.7f,.92f):Paper);
    FString Body=View.bPaused?TEXT("Enter / A or Back: return to the current line"):View.Narration;
    if(!View.bPaused && !View.Body.IsEmpty()){if(!Body.IsEmpty())Body+=TEXT("\n\n");Body+=View.Body;}
    Message->SetText(FText::FromString(Body));
    const bool CheckpointChoices=Choosing && View.bDevelopmentStop;
    if (CheckpointChoices) Position(BodyScroll,.045f,.24f,.95f,.57f);
    const bool Note=Choosing && !CheckpointChoices && !View.ChoiceHint.IsEmpty();
    ChoiceNote->SetText(FText::FromString(Note?View.ChoiceHint:FString()));ChoiceNote->SetVisibility(Note?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    Position(ChoiceScroll,.04f,CheckpointChoices?.59f:Note?.31f:.26f,.96f,.83f);
    BodyScroll->SetVisibility(Choosing && !CheckpointChoices?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    ChoiceScroll->SetVisibility(Choosing?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Choices->ClearChildren();Buttons.Reset();
    if(Choosing)for(int32 I=0;I<View.Choices.Num();++I)
    {
        auto* B=WidgetTree->ConstructWidget<UMemoriaNarrativeChoiceButton>();B->Owner=this;B->ChoiceIndex=I;B->SetStyle(ChoiceStyle(I==Selection));
        B->OnClicked.AddDynamic(B,&UMemoriaNarrativeChoiceButton::Choose);
        auto* Label=Text(WidgetTree,18,Paper);Label->SetText(FText::FromString(View.Choices[I].Text));
        auto* Lines=WidgetTree->ConstructWidget<UVerticalBox>();Lines->AddChildToVerticalBox(Label);
        // Source choice effect previews sit under the option in a quieter tone.
        if(!View.Choices[I].Effect.IsEmpty()){auto* Effect=Text(WidgetTree,14,FLinearColor(.72f,.6f,.4f));Effect->SetText(FText::FromString(View.Choices[I].Effect));Lines->AddChildToVerticalBox(Effect)->SetPadding(FMargin(0,3,0,0));}
        auto* RowPadding=WidgetTree->ConstructWidget<UBorder>();RowPadding->SetBrushColor(FLinearColor::Transparent);RowPadding->SetPadding(FMargin(18,11));RowPadding->SetContent(Lines);B->SetContent(RowPadding);
        Cast<UButtonSlot>(RowPadding->Slot)->SetHorizontalAlignment(HAlign_Fill);
        Cast<UButtonSlot>(RowPadding->Slot)->SetVerticalAlignment(VAlign_Center);
        Choices->AddChildToVerticalBox(B)->SetPadding(FMargin(0,0,0,7));Buttons.Add(B);
    }
    Hint->SetText(FText::FromString(View.bCompactStatus?TEXT(""):View.bDevelopmentStop && !Choosing?TEXT("Recorded development observation"):View.bPaused?TEXT(""):Choosing?TEXT("UP / DOWN  Select     ENTER / A  Confirm     or click a choice"):TEXT("ENTER / E / SPACE / A  Continue")));
}
