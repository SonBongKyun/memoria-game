#include "Presentation/MemoriaArchiveWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/ButtonSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
namespace
{
const FLinearColor Paper(.86f,.83f,.75f),Muted(.40f,.47f,.49f),Gold(.62f,.43f,.21f),Ink(.014f,.023f,.031f,.97f);
void Place(UCanvasPanel* C,UWidget* W,float L,float T,float R,float B)
{
    auto* S=C->AddChildToCanvas(W);S->SetAnchors(FAnchors(L,T,R,B));S->SetOffsets(FMargin(0));
}
UTextBlock* Label(UWidgetTree* Tree,const FString& Value,int32 Size,FLinearColor Color)
{
    auto* T=Tree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetAutoWrapText(true);
    T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),Size));T->SetColorAndOpacity(Color);return T;
}
UBorder* Panel(UWidgetTree* Tree,FLinearColor Color,float Radius=0)
{
    auto* B=Tree->ConstructWidget<UBorder>();B->SetBrush(FSlateRoundedBoxBrush(Color,Radius));B->SetPadding(FMargin(0));return B;
}
FButtonStyle ButtonStyle(bool Selected,FLinearColor Accent)
{
    FButtonStyle S;
    S.SetNormal(FSlateRoundedBoxBrush(Selected?FLinearColor(.06f,.09f,.11f):FLinearColor(.021f,.034f,.044f),2.f,
        Selected?Accent:FLinearColor(.12f,.16f,.17f),Selected?1.5f:.6f));
    S.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.075f,.10f,.12f),2.f,Accent,1.5f));S.SetPressed(S.Hovered);return S;
}
UMemoriaArchiveButton* Button(UWidgetTree* Tree,UMemoriaArchiveWidget* Owner,int32 Role,int32 Index)
{
    auto* B=Tree->ConstructWidget<UMemoriaArchiveButton>();B->Owner=Owner;B->Role=Role;B->Index=Index;
    B->OnClicked.AddDynamic(B,&UMemoriaArchiveButton::Activate);return B;
}
}
void UMemoriaArchiveButton::Activate()
{
    if(!Owner)return;
    if(Role==0)Owner->Select(Index);else if(Role==1)Owner->SetFilter(Index);else Owner->RequestClose();
}
TSharedRef<SWidget> UMemoriaArchiveWidget::RebuildWidget()
{
    SetIsFocusable(true);
    if(!WidgetTree->RootWidget)
    {
        auto* C=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=C;
        // Existing provisional illustration, displayed without changing its source bytes.
        auto* Back=WidgetTree->ConstructWidget<UImage>();
        Back->SetBrushFromTexture(MemoriaNarrativeArtwork::Load(TEXT("res://assets/cg/generated/archive_ch2_information_price_v1.png")));
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFill);Fit->AddChild(Back);Place(C,Fit,0,0,1,1);
        Back->SetColorAndOpacity(FLinearColor(.26f,.30f,.33f,1));
        Place(C,Panel(WidgetTree,FLinearColor(.004f,.008f,.012f,.70f)),0,0,1,1);
        Place(C,Panel(WidgetTree,Ink,3),.035f,.045f,.965f,.94f);
        Place(C,Panel(WidgetTree,Gold),.061f,.202f,.935f,.204f);
        Place(C,Label(WidgetTree,TEXT("M E M O R I A   /   A R R E L"),12,Gold),.062f,.072f,.67f,.098f);
        Heading=Label(WidgetTree,TEXT("Memory Archive"),31,Paper);Place(C,Heading,.06f,.108f,.69f,.166f);
        Summary=Label(WidgetTree,TEXT(""),13,Muted);Place(C,Summary,.062f,.168f,.77f,.195f);
        auto* Close=Button(WidgetTree,this,2,0);Close->SetStyle(ButtonStyle(false,Gold));
        Close->SetContent(Label(WidgetTree,TEXT("CLOSE  /  TAB"),13,Paper));Place(C,Close,.79f,.096f,.934f,.15f);
        Filters=WidgetTree->ConstructWidget<UHorizontalBox>();Place(C,Filters,.06f,.227f,.935f,.279f);
        CardScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(C,CardScroll,.06f,.306f,.558f,.875f);
        Cards=WidgetTree->ConstructWidget<UVerticalBox>();CardScroll->AddChild(Cards);
        Place(C,Panel(WidgetTree,FLinearColor(.13f,.16f,.17f)),.575f,.305f,.576f,.875f);
        auto* Frame=Panel(WidgetTree,FLinearColor(.10f,.12f,.13f));Frame->SetPadding(FMargin(1));Place(C,Frame,.602f,.306f,.935f,.512f);
        auto* ArtFit=WidgetTree->ConstructWidget<UScaleBox>();ArtFit->SetStretch(EStretch::ScaleToFill);ArtFit->SetClipping(EWidgetClipping::ClipToBounds);Frame->SetContent(ArtFit);
        Artwork=WidgetTree->ConstructWidget<UImage>();ArtFit->AddChild(Artwork);
        DetailState=Label(WidgetTree,TEXT(""),12,Gold);Place(C,DetailState,.604f,.533f,.93f,.56f);
        DetailTitle=Label(WidgetTree,TEXT(""),24,Paper);Place(C,DetailTitle,.601f,.57f,.936f,.662f);
        DetailScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(C,DetailScroll,.604f,.678f,.935f,.874f);
        DetailBody=Label(WidgetTree,TEXT(""),16,Paper);DetailScroll->AddChild(DetailBody);
        Hint=Label(WidgetTree,TEXT("UP / DOWN  Browse     LEFT / RIGHT  Grade     TAB / M / ESC / B  Close"),11,Muted);
        Place(C,Hint,.062f,.895f,.94f,.925f);
    }
    Draw();return Super::RebuildWidget();
}
void UMemoriaArchiveWidget::UnbindRun()
{
    if(Run)Run->OnRunReplaced.RemoveAll(this);
    if(Memory)Memory->OnObserved.RemoveAll(this);
    Run=nullptr;Memory=nullptr;
}
void UMemoriaArchiveWidget::BindRun(UMemoriaRunSubsystem* InRun)
{
    UnbindRun();Run=InRun;
    if(Run)
    {
        Memory=Run->GetPlayerMemory();
        Run->OnRunReplaced.AddUObject(this,&UMemoriaArchiveWidget::RequestClose);
        Memory->OnObserved.AddWeakLambda(this,[this](const auto&){bDirty=true;});
    }
    Refresh();
}
void UMemoriaArchiveWidget::NativeDestruct() { UnbindRun();Super::NativeDestruct(); }
void UMemoriaArchiveWidget::NativeTick(const FGeometry& G,float D)
{
    Super::NativeTick(G,D);if(bDirty){bDirty=false;Refresh();}
}
void UMemoriaArchiveWidget::Refresh()
{
    View=Run?MemoriaArchive::Build(*Run,FilterGrade):FMemoriaArchiveView();
    if(!View.Rows.ContainsByPredicate([&](const auto& R){return R.Id==SelectedId;}))
        SelectedId=View.Rows.IsEmpty()?FString():View.Rows[0].Id;
    Draw();
}
void UMemoriaArchiveWidget::Select(int32 Index)
{
    if(!View.Rows.IsValidIndex(Index))return;
    SelectedId=View.Rows[Index].Id;Draw();
    if(CardScroll && CardButtons.IsValidIndex(Index))CardScroll->ScrollWidgetIntoView(CardButtons[Index],false);
    if(DetailScroll)DetailScroll->ScrollToStart();
}
void UMemoriaArchiveWidget::SetFilter(int32 Grade)
{
    if(Grade < -1 || Grade > 4)return;
    FilterGrade=Grade;Refresh();if(CardScroll)CardScroll->ScrollToStart();
}
void UMemoriaArchiveWidget::Navigate(int32 Direction)
{
    if(View.Rows.IsEmpty())return;
    int32 I=View.Rows.IndexOfByPredicate([&](const auto& R){return R.Id==SelectedId;});
    Select((FMath::Max(I,0)+Direction+View.Rows.Num())%View.Rows.Num());
}
void UMemoriaArchiveWidget::CycleFilter(int32 Direction) { SetFilter((FilterGrade+1+Direction+6)%6-1); }
void UMemoriaArchiveWidget::RequestClose() { OnClose.ExecuteIfBound(); }
FReply UMemoriaArchiveWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    const auto K=Event.GetKey();
    if(K==EKeys::Tab || K==EKeys::M || K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right){OnConsumedKey.ExecuteIfBound(K,Event.IsRepeat()?IE_Repeat:IE_Pressed);if(!Event.IsRepeat())RequestClose();return FReply::Handled();}
    if(K==EKeys::Up || K==EKeys::Down){OnConsumedKey.ExecuteIfBound(K,Event.IsRepeat()?IE_Repeat:IE_Pressed);Navigate(K==EKeys::Up?-1:1);return FReply::Handled();}
    if(K==EKeys::Left || K==EKeys::Right){OnConsumedKey.ExecuteIfBound(K,Event.IsRepeat()?IE_Repeat:IE_Pressed);CycleFilter(K==EKeys::Left?-1:1);return FReply::Handled();}
    if(K==EKeys::Enter || K==EKeys::E || K==EKeys::SpaceBar || K==EKeys::Gamepad_FaceButton_Bottom)
    {
        // Slate consumes these keys, so retain the physical gesture for the owner
        // before closing can expose the underlying shop/checkpoint again.
        OnConsumedKey.ExecuteIfBound(K,Event.IsRepeat()?IE_Repeat:IE_Pressed);
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(G,Event);
}
FReply UMemoriaArchiveWidget::NativeOnKeyUp(const FGeometry& G,const FKeyEvent& Event)
{
    const auto K=Event.GetKey();
    if(K==EKeys::Enter || K==EKeys::E || K==EKeys::SpaceBar || K==EKeys::Gamepad_FaceButton_Bottom ||
        K==EKeys::Tab || K==EKeys::M || K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right ||
        K==EKeys::Up || K==EKeys::Down || K==EKeys::Left || K==EKeys::Right)
    { OnConsumedKey.ExecuteIfBound(K,IE_Released);return FReply::Handled(); }
    return Super::NativeOnKeyUp(G,Event);
}
void UMemoriaArchiveWidget::Draw()
{
    if(!Heading)return;
    Heading->SetText(FText::FromString(View.Title));
    Summary->SetText(FText::FromString(FString::Printf(TEXT("%d HELD     /     %d BURNED     /     %d RECORDED"),View.RemainingCount,View.BurnedCount,View.OwnedCount)));
    Filters->ClearChildren();
    for(int32 I=0;I<View.FilterLabels.Num();++I)
    {
        auto* B=Button(WidgetTree,this,1,I-1);B->SetStyle(ButtonStyle(FilterGrade==I-1,Gold));
        auto* T=Label(WidgetTree,View.FilterLabels[I],12,FilterGrade==I-1?Paper:Muted);T->SetJustification(ETextJustify::Center);B->SetContent(T);
        auto* S=Filters->AddChildToHorizontalBox(B);S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(0,0,6,0));
    }
    Cards->ClearChildren();CardButtons.Reset();
    for(int32 I=0;I<View.Rows.Num();++I)
    {
        const auto& R=View.Rows[I];const bool Selected=R.Id==SelectedId;
        auto* B=Button(WidgetTree,this,0,I);B->SetStyle(ButtonStyle(Selected,R.Accent));
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetMinDesiredHeight(84);B->SetContent(Size);
        if(auto* CardSlot=Cast<UButtonSlot>(Size->Slot)){CardSlot->SetHorizontalAlignment(HAlign_Fill);CardSlot->SetVerticalAlignment(VAlign_Fill);}
        auto* Row=WidgetTree->ConstructWidget<UCanvasPanel>();Size->AddChild(Row);
        Place(Row,Panel(WidgetTree,R.bBurned?FLinearColor(.22f,.17f,.14f):R.Accent),.004f,.1f,.011f,.9f);
        Place(Row,Label(WidgetTree,FString::Printf(TEXT("%02d"),I+1),13,Muted),.04f,.16f,.10f,.78f);
        Place(Row,Label(WidgetTree,R.Title,17,R.bBurned?FLinearColor(.51f,.47f,.43f):Paper),.12f,.14f,.96f,.51f);
        Place(Row,Label(WidgetTree,R.GradeLabel+TEXT("   /   ")+R.StateLabel,11,R.Accent),.12f,.62f,.96f,.91f);
        Cards->AddChildToVerticalBox(B)->SetPadding(FMargin(0,0,0,5));CardButtons.Add(B);
    }
    const auto* Selected=View.Rows.FindByPredicate([&](const auto& R){return R.Id==SelectedId;});
    if(!Selected)
    {
        Cards->AddChildToVerticalBox(Label(WidgetTree,View.EmptyText,18,Muted));
        DetailTitle->SetText(FText::GetEmpty());DetailState->SetText(FText::GetEmpty());DetailBody->SetText(FText::FromString(View.EmptyText));
        Artwork->SetBrushFromTexture(nullptr);return;
    }
    DetailTitle->SetText(FText::FromString(Selected->Title));
    DetailState->SetText(FText::FromString(Selected->GradeLabel+TEXT("   /   ")+Selected->StateLabel));
    DetailState->SetColorAndOpacity(Selected->Accent);
    FString Body=Selected->Description;
    if(!Selected->StoryEffect.IsEmpty())Body+=TEXT("\n\n")+Selected->StoryEffect;
    DetailBody->SetText(FText::FromString(Body));
    // These existing illustrations already depict this source chapter's sword/market.
    const TCHAR* Art=Selected->Id==TEXT("identity_first_sword")?
        TEXT("res://assets/cg/generated/story_ch2_lost_instructor_grip.png"):
        TEXT("res://assets/cg/generated/archive_ch2_information_price_v1.png");
    Artwork->SetBrushFromTexture(MemoriaNarrativeArtwork::Load(Art));
    Artwork->SetColorAndOpacity(Selected->bBurned?FLinearColor(.46f,.46f,.46f,1):FLinearColor(.80f,.85f,.88f,1));
}
UTexture2D* UMemoriaArchiveWidget::DisplayedArtwork() const
{ return Artwork?Cast<UTexture2D>(Artwork->GetBrush().GetResourceObject()):nullptr; }
FString UMemoriaArchiveWidget::VisibleText() const
{
    FString S=View.Title;
    for(const auto& R:View.Rows)S+=TEXT("\n")+R.Title+TEXT(" | ")+R.StateLabel;
    if(DetailBody)S+=TEXT("\n")+DetailBody->GetText().ToString();
    return S;
}
