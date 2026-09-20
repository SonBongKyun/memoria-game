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
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Place(Illustration,Fit,0,0,1,1);
        Backdrop=WidgetTree->ConstructWidget<UImage>();Fit->AddChild(Backdrop);
        Illustration->SetVisibility(ESlateVisibility::HitTestInvisible);
        NarrativePanel=WidgetTree->ConstructWidget<UBorder>();NarrativePanel->SetBrush(FSlateRoundedBoxBrush(Ink,3.f,Gold,1.f));
        Place(Canvas,NarrativePanel,.055f,.66f,.945f,.96f);
        auto* Interior=WidgetTree->ConstructWidget<UCanvasPanel>();NarrativePanel->SetContent(Interior);NarrativePanel->SetPadding(FMargin(0));
        Speaker=Text(WidgetTree,18,Gold);Place(Interior,Speaker,.045f,.085f,.94f,.23f);
        BodyScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,BodyScroll,.045f,.28f,.95f,.78f);
        Message=Text(WidgetTree,21,Paper);BodyScroll->AddChild(Message);
        Hint=Text(WidgetTree,12,FLinearColor(.42f,.43f,.44f));Place(Interior,Hint,.045f,.84f,.95f,.98f);
        ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,ChoiceScroll,.04f,.26f,.96f,.83f);
        Choices=WidgetTree->ConstructWidget<UVerticalBox>();ChoiceScroll->AddChild(Choices);
        PortraitFrame=WidgetTree->ConstructWidget<UBorder>();PortraitFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Black,1.f,Gold,1.f));PortraitFrame->SetPadding(FMargin(2));
        Place(Interior,PortraitFrame,.04f,.28f,.155f,.89f);
        auto* PortraitFit=WidgetTree->ConstructWidget<UScaleBox>();PortraitFit->SetStretch(EStretch::ScaleToFit);PortraitFrame->SetContent(PortraitFit);
        Portrait=WidgetTree->ConstructWidget<UImage>();PortraitFit->AddChild(Portrait);
        PortraitFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
        Location=Text(WidgetTree,13,Gold);Place(Canvas,Location,.058f,.045f,.9f,.10f);
        ShopWidget=WidgetTree->ConstructWidget<UMemoriaShopWidget>();Place(Canvas,ShopWidget,0,0,1,1);
    }
    Refresh(); return Super::RebuildWidget();
}
void UMemoriaDevelopmentNarrativeWidget::Display(const FMemoriaNarrativeView& InView)
{
    const int32 Previous=SelectedOriginalIndex();const bool SameStep=View.Header==InView.Header;
    View=InView;Selection=0;
    if(SameStep)for(int32 I=0;I<View.Choices.Num();++I)if(View.Choices[I].OriginalIndex==Previous)Selection=I;
    Refresh();if(!SameStep && BodyScroll)BodyScroll->ScrollToStart();
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
{ if(!View.bShopPresentation)OnConfirm.ExecuteIfBound(SelectedOriginalIndex()); }
FString UMemoriaDevelopmentNarrativeWidget::VisibleText() const
{
    if(View.bShopPresentation && ShopWidget)return ShopWidget->VisibleText();
    if(!Message)return FString();
    FString Out=Location->GetText().ToString()+TEXT("\n")+Speaker->GetText().ToString();
    if(BodyScroll->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+Message->GetText().ToString();
    if(ChoiceScroll->GetVisibility()!=ESlateVisibility::Collapsed)for(const auto& C:View.Choices)Out+=TEXT("\n")+C.Text;
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
    if(Backdrop->GetBrush().GetResourceObject()!=CG)Backdrop->SetBrushFromTexture(CG,true);
    auto* Face=View.Speaker.IsEmpty() || View.bPaused || View.bCompactStatus || View.bDevelopmentStop || !View.Choices.IsEmpty()?nullptr:MemoriaNarrativeArtwork::Load(View.PortraitSource);
    Portrait->SetBrushFromTexture(Face,true);PortraitFrame->SetVisibility(Face?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    const bool Right=View.PortraitSide==TEXT("right");
    Position(PortraitFrame,Right?.845f:.04f,.28f,Right?.96f:.155f,.89f);
    const bool Choosing=!View.Choices.IsEmpty() && !View.bPaused;
    Position(NarrativePanel,View.bCompactStatus?.035f:.055f,View.bCompactStatus?.035f:View.bDevelopmentStop?.14f:Choosing?.49f:.66f,View.bCompactStatus?.37f:.945f,View.bCompactStatus?.19f:.96f);
    Position(BodyScroll,Face && !Right?.185f:.045f,.28f,Face && Right?.815f:.95f,.78f);
    Position(Hint,Face && !Right?.185f:.045f,.84f,Face && Right?.815f:.95f,.98f);
    Location->SetText(FText::FromString(View.bDevelopmentStop?View.Header:View.LocationTitle));
    Speaker->SetText(FText::FromString(View.bCompactStatus?View.Header:View.bPaused?TEXT("PAUSED"):Choosing?TEXT("YOUR CHOICE"):View.Speaker.IsEmpty()?TEXT("MEMORIA"):View.Speaker));
    FString Body=View.bPaused?TEXT("Enter / A or Back: return to the current line"):View.Narration;
    if(!View.bPaused && !View.Body.IsEmpty()){if(!Body.IsEmpty())Body+=TEXT("\n\n");Body+=View.Body;}
    Message->SetText(FText::FromString(Body));
    BodyScroll->SetVisibility(Choosing?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    ChoiceScroll->SetVisibility(Choosing?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Choices->ClearChildren();Buttons.Reset();
    if(Choosing)for(int32 I=0;I<View.Choices.Num();++I)
    {
        auto* B=WidgetTree->ConstructWidget<UMemoriaNarrativeChoiceButton>();B->Owner=this;B->ChoiceIndex=I;B->SetStyle(ChoiceStyle(I==Selection));
        B->OnClicked.AddDynamic(B,&UMemoriaNarrativeChoiceButton::Choose);
        auto* Label=Text(WidgetTree,18,Paper);Label->SetText(FText::FromString(View.Choices[I].Text));
        auto* RowPadding=WidgetTree->ConstructWidget<UBorder>();RowPadding->SetBrushColor(FLinearColor::Transparent);RowPadding->SetPadding(FMargin(18,11));RowPadding->SetContent(Label);B->SetContent(RowPadding);
        Cast<UButtonSlot>(RowPadding->Slot)->SetHorizontalAlignment(HAlign_Fill);
        Cast<UButtonSlot>(RowPadding->Slot)->SetVerticalAlignment(VAlign_Center);
        Choices->AddChildToVerticalBox(B)->SetPadding(FMargin(0,0,0,7));Buttons.Add(B);
    }
    Hint->SetText(FText::FromString(View.bCompactStatus?TEXT(""):View.bDevelopmentStop?TEXT("Recorded development observation"):View.bPaused?TEXT(""):Choosing?TEXT("UP / DOWN  Select     ENTER / A  Confirm     or click a choice"):TEXT("ENTER / E / SPACE / A  Continue")));
}
