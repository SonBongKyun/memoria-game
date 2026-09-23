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
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElementTypes.h"
#include "Engine/Texture2D.h"
namespace
{
const FLinearColor BattlePaper(.86f,.85f,.80f),BattleMuted(.43f,.49f,.51f),BattleGold(.66f,.47f,.25f),BattleInk(.018f,.030f,.038f,.93f);
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
void UMemoriaBattleEntryButton::Activate(){if(Owner)Owner->ConfirmIntent();}
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
        EnemyName=BattleText(WidgetTree,TEXT(""),22,BattlePaper);BattlePlace(Canvas,EnemyName,.7f,.707f,.938f,.752f);
        EnemyHealth=BattleText(WidgetTree,TEXT(""),13,BattleMuted);BattlePlace(Canvas,EnemyHealth,.702f,.76f,.938f,.788f);
        EnemyBar=BattleHealthBar(WidgetTree,FLinearColor(.48f,.23f,.19f));BattlePlace(Canvas,EnemyBar,.702f,.794f,.938f,.803f);

        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor(.010f,.022f,.032f,.94f)),.055f,.782f,.655f,.927f);
        LogScroll=WidgetTree->ConstructWidget<UScrollBox>();LogScroll->SetScrollbarThickness(FVector2D(3,3));BattlePlace(Canvas,LogScroll,.071f,.798f,.638f,.914f);
        BattleLog=BattleText(WidgetTree,TEXT(""),13,BattlePaper);LogScroll->AddChild(BattleLog);
        FleeButton=WidgetTree->ConstructWidget<UMemoriaBattleEntryButton>();FleeButton->Owner=this;FleeButton->OnClicked.AddDynamic(FleeButton,&UMemoriaBattleEntryButton::Activate);
        FButtonStyle ButtonStyle;
        ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.060f,.083f,.093f,.98f),2.f,BattleGold,1.f));
        ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.10f,.13f,.14f),2.f,FLinearColor(.86f,.66f,.36f),1.5f));ButtonStyle.SetPressed(ButtonStyle.Hovered);
        ButtonStyle.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.025f,.040f,.048f),2.f,FLinearColor(.22f,.25f,.25f),1.f));FleeButton->SetStyle(ButtonStyle);
        FleeLabel=BattleText(WidgetTree,TEXT(""),20,BattlePaper);FleeLabel->SetJustification(ETextJustify::Center);FleeButton->SetContent(FleeLabel);
        BattlePlace(Canvas,FleeButton,.704f,.849f,.945f,.927f);
        auto* Keys=BattleText(WidgetTree,TEXT("ENTER / E / SPACE   |   PAD A"),10,BattleMuted);Keys->SetJustification(ETextJustify::Center);BattlePlace(Canvas,Keys,.705f,.937f,.945f,.963f);
        BoundaryNote=BattleText(WidgetTree,TEXT(""),11,BattleMuted);BattlePlace(Canvas,BoundaryNote,.055f,.941f,.678f,.98f);
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
    View=InView;Draw();
}
void UMemoriaBattleEntryWidget::ConfirmIntent()
{
    if(!Battle || bFleeRequested || !View.bCanFlee || !OnFlee.IsBound())return;
    const auto Current=Battle->GetView();
    if(!Current.bActive || Current.bReturning || !Current.bCanFlee || Current.Revision!=View.Revision)return;
    const uint64 SubmittedRevision=View.Revision;bFleeRequested=true;Draw();OnFlee.Execute(SubmittedRevision);
    // A controller may reject a press inherited from the previous owner. Allow a fresh gesture.
    if(Battle && Battle->GetRevision()==SubmittedRevision && Battle->GetView().bCanFlee){bFleeRequested=false;Draw();}
}
void UMemoriaBattleEntryWidget::Draw()
{
    if(!Heading)return;
    Heading->SetText(FText::FromString(View.EnvironmentName));
    Turn->SetText(FText::FromString(View.bReturning?View.BattleState:(View.bKo?TEXT("\ub2f9\uc2e0\uc758 \ud134"):TEXT("YOUR TURN"))));
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
    Modifier->SetText(FText::FromString(View.ModifierName+(View.ModifierDescription.IsEmpty()?FString():TEXT("\n")+View.ModifierDescription)));
    BattleLog->SetText(FText::FromString(FString::Join(View.Logs,TEXT("\n"))));
    // Small truthful scope text, not new dialogue or a new story fact.
    BoundaryNote->SetText(FText::FromString(View.bKo?TEXT("\uac1c\ubc1c \uad6c\uac04: \uc804\ud22c \uc9c4\uc785\u00b7\ub3c4\uc8fc\ub9cc \uc9c0\uc6d0. \uacf5\uaca9\u00b7\uc5f0\uc18c \ubbf8\uad6c\ud604."):TEXT("Development slice: entry and withdrawal only. Attack / burn unavailable.")));
    FleeLabel->SetText(FText::FromString(View.bKo?TEXT("\ub3c4\uc8fc"):TEXT("FLEE")));
    FleeButton->SetIsEnabled(View.bActive && View.bCanFlee && !View.bReturning && !bFleeRequested);
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
        if(!Event.IsRepeat() && BattleConfirmKey(Key))ConfirmIntent();
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
    SetRenderOpacity(Entry*Exit);
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
    return LastLayer+1;
}
FString UMemoriaBattleEntryWidget::VisibleText() const
{
    FString Text=View.EnvironmentName+TEXT("\n")+View.EnemyName+TEXT("\n")+View.ObjectiveTitle+TEXT("\n")+View.ObjectiveDescription+TEXT("\n")+FString::Join(View.Logs,TEXT("\n"));
    if(PlayerHealth)Text+=TEXT("\n")+PlayerHealth->GetText().ToString();if(EnemyHealth)Text+=TEXT("\n")+EnemyHealth->GetText().ToString();
    if(BoundaryNote)Text+=TEXT("\n")+BoundaryNote->GetText().ToString();if(FleeLabel)Text+=TEXT("\n")+FleeLabel->GetText().ToString();return Text;
}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedBackdrop() const{return BattleTexture(Backdrop);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedPlayerArtwork() const{return BattleTexture(PlayerArt);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedEnemyArtwork() const{return BattleTexture(EnemyArt);}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedAllyArtwork() const{return View.bEliaInParty?BattleTexture(AllyArt):nullptr;}
bool UMemoriaBattleEntryWidget::IsFleeEnabled() const{return FleeButton && FleeButton->GetIsEnabled();}
