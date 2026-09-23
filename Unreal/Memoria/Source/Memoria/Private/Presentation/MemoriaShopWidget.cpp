#include "Presentation/MemoriaShopWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
namespace
{
const FLinearColor Ink(0.001f,0.002f,0.003f,0.48f), Gold(0.80f,0.66f,0.42f), Paper(0.89f,0.87f,0.79f);
FButtonStyle RowStyle(bool Selected)
{
    FButtonStyle Style;
    const FLinearColor Fill=Selected?FLinearColor(0.025f,0.018f,0.008f,0.92f):FLinearColor(0.003f,0.005f,0.006f,0.88f);
    Style.SetNormal(FSlateRoundedBoxBrush(Fill,3.f,FLinearColor(0.23f,0.16f,0.065f,0.75f),1.f));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.035f,0.025f,0.012f,0.95f),3.f,Gold,1.f));
    Style.SetPressed(Style.Hovered);
    return Style;
}
void Place(UCanvasPanel* Canvas,UWidget* Widget,float L,float T,float R,float B)
{
    auto* Slot=Canvas->AddChildToCanvas(Widget);Slot->SetAnchors(FAnchors(L,T,R,B));Slot->SetOffsets(FMargin(0));
}
UTextBlock* Text(UWidgetTree* Tree,const FString& Value,int32 Size,FLinearColor Color=Paper)
{
    auto* W=Tree->ConstructWidget<UTextBlock>(); W->SetText(FText::FromString(Value));W->SetAutoWrapText(true);
    auto Font=W->GetFont();Font.Size=Size;W->SetFont(Font);W->SetColorAndOpacity(Color);return W;
}
UBorder* Panel(UWidgetTree* Tree,UCanvasPanel* Canvas,float L,float T,float R,float B)
{
    auto* W=Tree->ConstructWidget<UBorder>();W->SetBrushColor(Ink);W->SetPadding(FMargin(18));Place(Canvas,W,L,T,R,B);return W;
}
}
void UMemoriaShopRowButton::Preview(){if(Shop) Shop->Preview(RowIndex);}
TSharedRef<SWidget> UMemoriaShopWidget::RebuildWidget()
{
    if(!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        auto* Back=WidgetTree->ConstructWidget<UImage>();BackdropImage=Back;
        Back->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Memoria/Presentation/Shop/T_ShopBackdrop.T_ShopBackdrop")));
        Back->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Canvas,Back,0,0,1,1);
        Title=Text(WidgetTree,TEXT(""),27,Gold);Place(Canvas,Title,0.105f,0.07f,0.77f,0.14f);
        SellButton=WidgetTree->ConstructWidget<UMemoriaShopRowButton>(); SellButton->SetContent(Text(WidgetTree,TEXT("SELL"),16,Gold));
        SellButton->OnClicked.AddDynamic(this,&UMemoriaShopWidget::SellTab);Place(Canvas,SellButton,.106f,.18f,.237f,.24f);
        BuyButton=WidgetTree->ConstructWidget<UMemoriaShopRowButton>(); BuyButton->SetContent(Text(WidgetTree,TEXT("BUY"),16,Gold));
        BuyButton->OnClicked.AddDynamic(this,&UMemoriaShopWidget::BuyTab);Place(Canvas,BuyButton,.244f,.18f,.375f,.24f);
        auto* Close=WidgetTree->ConstructWidget<UMemoriaShopRowButton>();Close->SetStyle(RowStyle(false));Close->SetContent(Text(WidgetTree,TEXT("CLOSE / ESC"),15,Gold));
        Close->OnClicked.AddDynamic(this,&UMemoriaShopWidget::CloseIntent);Place(Canvas,Close,.803f,.76f,.935f,.83f);
        Action=WidgetTree->ConstructWidget<UMemoriaShopRowButton>();Action->SetStyle(RowStyle(false));
        Action->OnClicked.AddDynamic(this,&UMemoriaShopWidget::ConfirmIntent);Place(Canvas,Action,.425f,.77f,.76f,.83f);
        Feedback=Text(WidgetTree,TEXT(""),14,Gold);Place(Canvas,Feedback,.42f,.84f,.765f,.975f);
        auto* ListPanel=Panel(WidgetTree,Canvas,0.106f,0.255f,0.375f,0.875f);
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();ListPanel->SetContent(Scroll);
        Rows=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Rows);
        auto* Details=Panel(WidgetTree,Canvas,0.417f,0.26f,0.765f,0.76f);
        auto* DetailScroll=WidgetTree->ConstructWidget<UScrollBox>();Details->SetContent(DetailScroll);
        Detail=Text(WidgetTree,TEXT(""),21);DetailScroll->AddChild(Detail);
        Place(Canvas,Text(WidgetTree,TEXT("THE MEMORY EXCHANGE"),15,Gold),0.43f,0.185f,0.755f,0.24f);
        auto* Portrait=WidgetTree->ConstructWidget<UImage>();PortraitImage=Portrait;
        Portrait->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Memoria/Presentation/Shop/T_MaletPortrait.T_MaletPortrait")));
        Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Canvas,Portrait,0.805f,0.135f,0.905f,0.313f);
        Caption=Text(WidgetTree,TEXT(""),15,Paper);Place(Canvas,Caption,0.797f,0.385f,0.927f,0.52f);
        Grains=Text(WidgetTree,TEXT(""),22,Gold);Place(Canvas,Grains,0.803f,0.57f,0.93f,0.68f);

    }
    Refresh();return Super::RebuildWidget();
}
void UMemoriaShopWidget::Display(const FMemoriaShopView& InView)
{
    const FString Previous=View.Rows.IsValidIndex(Selection)?View.Rows[Selection].Id:FString();
    const bool Same = View.Revision == InView.Revision && View.Mode == InView.Mode;
    View=InView;Selection=Same ? View.Rows.IndexOfByPredicate([&](const auto& R){return R.Id==Previous;}) : INDEX_NONE;Refresh();
}
void UMemoriaShopWidget::Navigate(int32 Direction)
{
    if(View.Rows.IsEmpty())return;
    Preview(Selection==INDEX_NONE?(Direction>0?0:View.Rows.Num()-1):(Selection+Direction+View.Rows.Num())%View.Rows.Num());
}
void UMemoriaShopWidget::Preview(int32 Index)
{
    if(!View.Rows.IsValidIndex(Index))return;
    Selection=Index;
    const auto& R=View.Rows[Index];
    Detail->SetText(FText::FromString(R.Title+TEXT("\n\n")+R.Description+TEXT("\n\n")+FString::Printf(TEXT("%lld Grains"),R.Price)+(R.StoryEffect.IsEmpty()?FString():TEXT("\n\n")+R.StoryEffect)));
    Action->SetIsEnabled(View.bOpen && (View.Mode==TEXT("sell") || View.Grains>=R.Price));
    const FString ActionText = View.Mode==TEXT("sell") ? FString::Printf(TEXT("Sell for %lld G / Enter"),R.Price)
        : (View.Grains>=R.Price ? FString::Printf(TEXT("Buy for %lld G / Enter"),R.Price) : TEXT("Not enough Grains"));
    Action->SetContent(Text(WidgetTree,ActionText,17,Gold));
    for(int32 I=0;I<Buttons.Num();++I)Buttons[I]->SetStyle(RowStyle(I==Index));
}
void UMemoriaShopWidget::Refresh()
{
    if(!Rows)return;
    Title->SetText(FText::FromString(View.Title));Caption->SetText(FText::FromString(View.Caption));Grains->SetText(FText::FromString(View.GrainsText));
    Detail->SetText(FText::FromString(View.EmptyDetail));Rows->ClearChildren();Buttons.Reset();
    Action->SetIsEnabled(false);Action->SetContent(Text(WidgetTree,TEXT("Select a memory"),17,Gold));
    SellButton->SetStyle(RowStyle(View.Mode==TEXT("sell")));BuyButton->SetStyle(RowStyle(View.Mode==TEXT("buy")));
    Feedback->SetText(FText::FromString(View.Feedback.IsEmpty()?TEXT("Up / Down: select   Left / Right: tab\nSelling burns the selected memory."):View.Feedback));
    if(View.bEmptyAvailable)Rows->AddChild(Text(WidgetTree,View.Mode==TEXT("sell")?TEXT("No memories available to sell."):TEXT("No memories available to buy."),18));
    for(int32 I=0;I<View.Rows.Num();++I)
    {
        const auto& R=View.Rows[I];auto* B=WidgetTree->ConstructWidget<UMemoriaShopRowButton>();B->Shop=this;B->RowIndex=I;
        B->SetStyle(RowStyle(false));B->OnClicked.AddDynamic(B,&UMemoriaShopRowButton::Preview);
        auto* Label=Text(WidgetTree,R.Title+FString::Printf(TEXT("\n%lld Grains"),R.Price),18);
        auto* Pad=WidgetTree->ConstructWidget<UBorder>();Pad->SetBrushColor(FLinearColor::Transparent);Pad->SetPadding(FMargin(10,12));Pad->SetContent(Label);B->SetContent(Pad);
        Rows->AddChildToVerticalBox(B)->SetPadding(FMargin(0,0,0,12));Buttons.Add(B);
    }
    if(View.Rows.IsValidIndex(Selection))Preview(Selection);
}
FString UMemoriaShopWidget::VisibleText() const
{
    FString Out=View.Title+TEXT("\n")+View.Caption+TEXT("\n")+View.GrainsText;
    for(const auto& R:View.Rows)Out+=TEXT("\n")+R.Title+FString::Printf(TEXT(" | %lld Grains"),R.Price);
    return Out+TEXT("\n")+View.Feedback+TEXT("\n")+(Detail?Detail->GetText().ToString():View.EmptyDetail);
}

bool UMemoriaShopWidget::HasArtwork() const
{
    auto* Back=BackdropImage?Cast<UTexture2D>(BackdropImage->GetBrush().GetResourceObject()):nullptr;
    auto* Portrait=PortraitImage?Cast<UTexture2D>(PortraitImage->GetBrush().GetResourceObject()):nullptr;
    return Back && Portrait && Back->GetSizeX()==1672 && Back->GetSizeY()==941 && Portrait->GetSizeX()==256 && Portrait->GetSizeY()==256;
}

void UMemoriaShopWidget::ConfirmIntent()
{
    if (!View.bOpen || !View.Rows.IsValidIndex(Selection)) return;
    if (auto* GI=GetGameInstance())
        GI->GetSubsystem<UMemoriaShopSubsystem>()->Transact(View.Mode,View.Rows[Selection].Id,View.Revision);
}
void UMemoriaShopWidget::SellTab()
{ if(View.bOpen)if(auto* GI=GetGameInstance())GI->GetSubsystem<UMemoriaShopSubsystem>()->SetMode(TEXT("sell")); }
void UMemoriaShopWidget::BuyTab()
{ if(View.bOpen)if(auto* GI=GetGameInstance())GI->GetSubsystem<UMemoriaShopSubsystem>()->SetMode(TEXT("buy")); }
void UMemoriaShopWidget::SwitchMode(int32 Direction)
{ if(Direction>0)BuyTab();else SellTab(); }
void UMemoriaShopWidget::CloseIntent()
{ if(View.bOpen)if(auto* GI=GetGameInstance())GI->GetSubsystem<UMemoriaShopSubsystem>()->Close(View.Revision); }
