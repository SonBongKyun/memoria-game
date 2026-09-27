#include "Presentation/MemoriaBattleEntryWidget.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Presentation/MemoriaUiKit.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElementTypes.h"
#include "Engine/Texture2D.h"
namespace
{
using MemoriaUiKit::Srgb;
using EFont=MemoriaFonts::EStyle;
const FLinearColor BattlePaper(.86f,.85f,.80f),BattleMuted(.43f,.49f,.51f),BattleGold(.66f,.47f,.25f);
// Witness reads as cold archive ink, distinct from the ember of a burn.
const FLinearColor BattleWitness(.46f,.80f,.78f),BattleEmber(.86f,.44f,.18f),BattleLimit(.45f,.66f,.84f);
// battle_scene.gd is laid out on a 1280x720 canvas; Slate lays out in 1080p units at 720p (x1.5),
// and a font point renders about 0.89 px there.
constexpr float U=1.5f;
int32 Pt(float Px){return FMath::RoundToInt(Px*1.12f);}
// BATTLE_ROLE_PROFILES: each plate's max box with its feet on STAGE_BASELINE_Y 424, as canvas anchors.
struct FStageRole{float L,T,R,B,Edge,Oval;FLinearColor Modulate;};
const FStageRole PlayerRole{.0953f,.2444f,.2891f,.5889f,.12f,.16f,FLinearColor(1.10f,1.06f,1.02f,1.f)};
const FStageRole AllyRole{.0258f,.3111f,.1508f,.5667f,.24f,.86f,FLinearColor(1.f,.98f,.95f,.92f)};
const FStageRole EnemyRole{.6570f,.1778f,.9305f,.5889f,.30f,.80f,FLinearColor(1.50f,1.45f,1.44f,.96f)};
UCanvasPanelSlot* BattlePlace(UCanvasPanel* Canvas,UWidget* Widget,float Left,float Top,float Right,float Bottom)
{
    auto* Slot=Canvas->AddChildToCanvas(Widget);Slot->SetAnchors(FAnchors(Left,Top,Right,Bottom));Slot->SetOffsets(FMargin(0));return Slot;
}
UTextBlock* BattleText(UWidgetTree* Tree,const FString& Value,int32 Px,FLinearColor Color,EFont Font=EFont::Ui)
{
    auto* Text=Tree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Value));Text->SetAutoWrapText(true);
    Text->SetFont(MemoriaFonts::Get(Font,Pt(Px)));Text->SetColorAndOpacity(Color);
    Text->SetShadowOffset(FVector2D(1,1));Text->SetShadowColorAndOpacity(FLinearColor(0,0,0,.8f));return Text;
}
UBorder* BattlePanel(UWidgetTree* Tree,FLinearColor Color,FLinearColor Edge=FLinearColor::Transparent,float Outline=0)
{
    auto* Panel=Tree->ConstructWidget<UBorder>();Panel->SetBrush(FSlateRoundedBoxBrush(Color,4.f,Edge,Outline));
    Panel->SetPadding(FMargin(0));return Panel;
}
UImage* BattleArt(UWidgetTree* Tree,UTexture2D* Texture,const FBox2f& Region=FBox2f(FVector2f(0,0),FVector2f(1,1)),float Alpha=1.f)
{
    auto* Image=Tree->ConstructWidget<UImage>();Image->SetBrushFromTexture(Texture,false);
    FSlateBrush Brush=Image->GetBrush();Brush.SetUVRegion(Region);Image->SetBrush(Brush);
    Image->SetColorAndOpacity(FLinearColor(1,1,1,Alpha));Image->SetVisibility(ESlateVisibility::HitTestInvisible);return Image;
}
UProgressBar* BattleHealthBar(UWidgetTree* Tree,FLinearColor Color)
{
    auto* Bar=Tree->ConstructWidget<UProgressBar>();FProgressBarStyle Style;
    Style.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(.02f,.022f,.03f,.95f),2.f));
    Style.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White,2.f));Bar->SetWidgetStyle(Style);
    Bar->SetFillColorAndOpacity(Color);Bar->SetBorderPadding(FVector2D::ZeroVector);return Bar;
}
USizeBox* Sized(UWidgetTree* Tree,UWidget* Child,float Width,float Height)
{
    auto* Box=Tree->ConstructWidget<USizeBox>();if(Width>0)Box->SetWidthOverride(Width);if(Height>0)Box->SetHeightOverride(Height);Box->AddChild(Child);return Box;
}
bool BattleConfirmKey(const FKey& Key)
{return Key==EKeys::Enter || Key==EKeys::E || Key==EKeys::SpaceBar || Key==EKeys::Gamepad_FaceButton_Bottom;}
bool BattleOwnedKey(const FKey& Key)
{
    return BattleConfirmKey(Key) || Key==EKeys::Escape || Key==EKeys::Tab || Key==EKeys::M || Key==EKeys::Gamepad_FaceButton_Right ||
        Key==EKeys::Up || Key==EKeys::Down || Key==EKeys::Left || Key==EKeys::Right || Key==EKeys::W || Key==EKeys::A || Key==EKeys::S || Key==EKeys::D;
}
// _action_base_color and the command button box; the focused command takes the source hover box.
FButtonStyle CommandStyle(const FLinearColor& Base,bool bSelected)
{
    FButtonStyle Style;
    const FSlateRoundedBoxBrush Normal(Base,4.f*U,Srgb(.60f,.45f,.26f,.42f),1.f*U),Hover(Srgb(.18f,.13f,.20f,.92f),4.f*U,Srgb(.95f,.68f,.34f,.90f),2.f*U);
    Style.SetNormal(bSelected?Hover:Normal);Style.SetHovered(Hover);Style.SetPressed(FSlateRoundedBoxBrush(Srgb(.32f,.20f,.26f,.98f),4.f*U,Srgb(.9f,.65f,.35f),1.f*U));
    Style.SetDisabled(FSlateRoundedBoxBrush(Srgb(.030f,.026f,.038f,.96f),4.f*U,Srgb(.34f,.28f,.20f,.40f),1.f*U));
    Style.SetNormalPadding(FMargin(8*U));Style.SetPressedPadding(FMargin(8*U));return Style;
}
UMaterialInterface* PlateMaterial()
{
    static const TCHAR* Path=TEXT("/Game/Memoria/Presentation/BattleEntry/M_BattlePlate.M_BattlePlate");
    return LoadObject<UMaterialInterface>(nullptr,Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
}
}
void UMemoriaBattleEntryButton::Activate(){if(Owner)Owner->ClickAction(Index,bChoice);}
namespace
{
// A stage plate: the art fitted into its role box, feet on the baseline, through the stage blend.
UImage* StagePlate(UWidgetTree* Tree,UCanvasPanel* Canvas,const FStageRole& Role)
{
    auto* Fit=Tree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetVisibility(ESlateVisibility::HitTestInvisible);
    BattlePlace(Canvas,Fit,Role.L,Role.T,Role.R,Role.B);
    auto* Image=Tree->ConstructWidget<UImage>();auto* S=Cast<UScaleBoxSlot>(Fit->AddChild(Image));
    S->SetHorizontalAlignment(HAlign_Center);S->SetVerticalAlignment(VAlign_Bottom);
    Image->SetRenderTransformPivot(FVector2D(.5,1.));return Image;
}
// _make_role_shadow / _make_role_glow: soft ellipses under the feet.
void StageEllipse(UWidgetTree* Tree,UCanvasPanel* Canvas,UTexture2D* Soft,float FootX,float FootY,float RadiusX,float RadiusY,const FLinearColor& Color)
{
    auto* E=BattleArt(Tree,Soft);E->SetColorAndOpacity(Color);
    BattlePlace(Canvas,E,(FootX-RadiusX)/1280.f,(FootY-RadiusY)/720.f,(FootX+RadiusX)/1280.f,(FootY+RadiusY)/720.f);
}
}
void UMemoriaBattleEntryWidget::SetPlate(UImage* Image,TObjectPtr<UMaterialInstanceDynamic>& Plate,UTexture2D* Texture,float Edge,float Oval,const FLinearColor& Modulate,const FLinearColor& Region)
{
    if(!Image)return;
    UMaterialInterface* Material=PlateMaterial();
    if(!Texture || !Material){Image->SetBrushFromTexture(Texture,true);return;}
    if(!Plate)Plate=UMaterialInstanceDynamic::Create(Material,this);
    Plate->SetTextureParameterValue(TEXT("Plate"),Texture);Plate->SetScalarParameterValue(TEXT("EdgeSoftness"),Edge);
    Plate->SetScalarParameterValue(TEXT("OvalMask"),Oval);Plate->SetScalarParameterValue(TEXT("LowerFade"),.92f);
    Plate->SetVectorParameterValue(TEXT("Modulate"),Modulate);Plate->SetVectorParameterValue(TEXT("Region"),Region);
    FSlateBrush Brush;Brush.SetResourceObject(Plate);
    const float W=Texture->GetSurfaceWidth(),H=Texture->GetSurfaceHeight();
    Brush.ImageSize=W>0&&H>0?FVector2D(W*(Region.B-Region.R),H*(Region.A-Region.G)):FVector2D(512,512);Image->SetBrush(Brush);
}
TSharedRef<SWidget> UMemoriaBattleEntryWidget::RebuildWidget()
{
    if(!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        BattlePlace(Canvas,BattlePanel(WidgetTree,FLinearColor::Black),0,0,1,1);
        // The stage: the encounter's painted ground, darkened toward the floor and edges as the arena reads.
        Backdrop=WidgetTree->ConstructWidget<UImage>();
        auto* BackFit=WidgetTree->ConstructWidget<UScaleBox>();BackFit->SetStretch(EStretch::ScaleToFill);BackFit->AddChild(Backdrop);BattlePlace(Canvas,BackFit,0,0,1,1);
        Backdrop->SetColorAndOpacity(FLinearColor(.50f,.50f,.56f,1));
        BattlePlace(Canvas,BattleArt(WidgetTree,MemoriaUiKit::Gradient(8,256,{.5,0},{.5,1},false,
            {{0.f,{.02f,.015f,.04f,.62f}},{.24f,{.02f,.015f,.04f,.08f}},{.55f,{.02f,.015f,.04f,0.f}},{.66f,{.02f,.015f,.04f,.35f}},{1.f,{.01f,.008f,.02f,.88f}}})),0,0,1,1);
        BattlePlace(Canvas,BattleArt(WidgetTree,MemoriaUiKit::Paint(160,90,[](const FVector2D& UV)
        {
            const float R=FVector2D((UV.X-.5)*1.1,(UV.Y-.46)).Size();
            return FLinearColor(.01f,.008f,.02f,FMath::Clamp(FMath::SmoothStep(.36f,.86f,R)*.72f,0.f,.72f));
        })),0,0,1,1);
        UTexture2D* Soft=MemoriaUiKit::Gradient(64,64,{.5,.5},{1.,.5},true,{{0.f,{1,1,1,1}},{.55f,{1,1,1,.45f}},{1.f,{1,1,1,0}}});
        // Rear line first: Elia behind Arrel, then Arrel, then the enemy.
        StageEllipse(WidgetTree,Canvas,Soft,113,408,58,10,Srgb(.54f,.38f,.68f,.08f));StageEllipse(WidgetTree,Canvas,Soft,113,408,50,8,FLinearColor(0,0,0,.27f));
        AllyArt=StagePlate(WidgetTree,Canvas,AllyRole);
        AllyName=BattleText(WidgetTree,TEXT(""),11,Srgb(.78f,.74f,.84f,.9f));BattlePlace(Canvas,AllyName,.028f,.566f,.15f,.592f);
        AllyStage={AllyArt->GetParent(),AllyName};
        StageEllipse(WidgetTree,Canvas,Soft,246,424,91,16,Srgb(.20f,.38f,.68f,.13f));StageEllipse(WidgetTree,Canvas,Soft,246,424,78,12,FLinearColor(0,0,0,.42f));
        PlayerArt=StagePlate(WidgetTree,Canvas,PlayerRole);
        StageEllipse(WidgetTree,Canvas,Soft,1016,424,116,17,Srgb(.50f,.15f,.50f,.10f));StageEllipse(WidgetTree,Canvas,Soft,1016,424,108,15,FLinearColor(0,0,0,.40f));
        EnemyArt=StagePlate(WidgetTree,Canvas,EnemyRole);
        // _build_tactical_objective_panel: the tactical plate art frames the objective card.
        BattlePlace(Canvas,BattleArt(WidgetTree,MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_battle_tactical_plate.png")),FBox2f(FVector2f(0,0),FVector2f(1,1)),.70f),.002f,.067f,.337f,.231f);
        auto* Objective=BattlePanel(WidgetTree,Srgb(.020f,.016f,.026f,.94f),Srgb(.82f,.64f,.34f,.72f),1.f*U);Objective->SetPadding(FMargin(12*U,7*U));
        BattlePlace(Canvas,Objective,.011f,.081f,.328f,.217f);
        auto* ObjectiveRows=WidgetTree->ConstructWidget<UVerticalBox>();Objective->SetContent(ObjectiveRows);
        ObjectiveTitle=BattleText(WidgetTree,TEXT(""),15,Srgb(.95f,.72f,.40f));ObjectiveRows->AddChildToVerticalBox(ObjectiveTitle)->SetPadding(FMargin(0,0,0,3*U));
        ObjectiveBody=BattleText(WidgetTree,TEXT(""),13,BattlePaper,EFont::Body);ObjectiveRows->AddChildToVerticalBox(ObjectiveBody);
        // Field observation: what the fight is doing now, with the enemy in miniature.
        auto* Observe=BattlePanel(WidgetTree,Srgb(.018f,.022f,.036f,.90f),Srgb(.55f,.62f,.85f,.62f),1.f*U);Observe->SetPadding(FMargin(8*U,5*U));
        BattlePlace(Canvas,Observe,.36f,.053f,.649f,.132f);
        auto* ObserveRow=WidgetTree->ConstructWidget<UHorizontalBox>();Observe->SetContent(ObserveRow);
        EnemyThumb=WidgetTree->ConstructWidget<UImage>();auto* ThumbFit=WidgetTree->ConstructWidget<UScaleBox>();ThumbFit->SetStretch(EStretch::ScaleToFill);ThumbFit->AddChild(EnemyThumb);
        auto* ThumbSlot=ObserveRow->AddChildToHorizontalBox(Sized(WidgetTree,ThumbFit,82*U,44*U));ThumbSlot->SetVerticalAlignment(VAlign_Center);ThumbSlot->SetPadding(FMargin(0,0,9*U,0));
        auto* ObserveText=WidgetTree->ConstructWidget<UVerticalBox>();auto* TextSlot=ObserveRow->AddChildToHorizontalBox(ObserveText);TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TextSlot->SetVerticalAlignment(VAlign_Center);
        ObservationTitle=BattleText(WidgetTree,TEXT(""),11,Srgb(.62f,.70f,.90f));ObserveText->AddChildToVerticalBox(ObservationTitle);
        Heading=BattleText(WidgetTree,TEXT(""),11,Srgb(.62f,.70f,.90f));Heading->SetVisibility(ESlateVisibility::Collapsed);ObserveText->AddChildToVerticalBox(Heading);
        Modifier=BattleText(WidgetTree,TEXT(""),13,BattlePaper,EFont::Body);ObserveText->AddChildToVerticalBox(Modifier);
        EchoBand=BattlePanel(WidgetTree,FLinearColor::Transparent);EchoText=BattleText(WidgetTree,TEXT(""),13,BattleWitness,EFont::Body);EchoBand->SetContent(EchoText);
        ObserveText->AddChildToVerticalBox(EchoBand);
        // _build_enemy_panel: name, HP, and the break / witness reading.
        auto* EnemyPanel=BattlePanel(WidgetTree,Srgb(.040f,.022f,.026f,.93f),Srgb(.72f,.30f,.28f,.66f),1.f*U);EnemyPanel->SetPadding(FMargin(10*U,6*U));
        BattlePlace(Canvas,EnemyPanel,.66f,.011f,.965f,.125f);
        auto* EnemyRows=WidgetTree->ConstructWidget<UVerticalBox>();EnemyPanel->SetContent(EnemyRows);
        auto* EnemyTop=WidgetTree->ConstructWidget<UHorizontalBox>();EnemyRows->AddChildToVerticalBox(EnemyTop);
        EnemyName=BattleText(WidgetTree,TEXT(""),16,Srgb(.96f,.76f,.62f),EFont::Title);EnemyTop->AddChildToHorizontalBox(EnemyName)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        EnemyHealth=BattleText(WidgetTree,TEXT(""),12,Srgb(.86f,.72f,.68f));EnemyHealth->SetAutoWrapText(false);EnemyHealth->SetJustification(ETextJustify::Right);EnemyTop->AddChildToHorizontalBox(EnemyHealth)->SetVerticalAlignment(VAlign_Center);
        EnemyBar=BattleHealthBar(WidgetTree,Srgb(.70f,.16f,.16f));EnemyRows->AddChildToVerticalBox(Sized(WidgetTree,EnemyBar,0,12*U))->SetPadding(FMargin(0,3*U));
        auto* EnemyBottom=WidgetTree->ConstructWidget<UHorizontalBox>();EnemyRows->AddChildToVerticalBox(EnemyBottom);
        BreakLabel=BattleText(WidgetTree,TEXT(""),11,BattleEmber);BreakLabel->SetAutoWrapText(false);EnemyBottom->AddChildToHorizontalBox(Sized(WidgetTree,BreakLabel,92*U,0))->SetVerticalAlignment(VAlign_Center);
        BreakBar=BattleHealthBar(WidgetTree,BattleEmber);auto* BreakSlot=EnemyBottom->AddChildToHorizontalBox(Sized(WidgetTree,BreakBar,0,7*U));BreakSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));BreakSlot->SetVerticalAlignment(VAlign_Center);
        WitnessLabel=BattleText(WidgetTree,TEXT(""),11,BattleWitness);WitnessLabel->SetAutoWrapText(false);auto* WitnessSlot=EnemyBottom->AddChildToHorizontalBox(WitnessLabel);WitnessSlot->SetVerticalAlignment(VAlign_Center);WitnessSlot->SetPadding(FMargin(10*U,0,5*U,0));
        WitnessPips=WidgetTree->ConstructWidget<UHorizontalBox>();EnemyBottom->AddChildToHorizontalBox(WitnessPips)->SetVerticalAlignment(VAlign_Center);
        BoundaryNote=BattleText(WidgetTree,TEXT(""),10,Srgb(.60f,.58f,.62f,.85f));BoundaryNote->SetJustification(ETextJustify::Right);BattlePlace(Canvas,BoundaryNote,.66f,.13f,.965f,.162f);
        // _build_turn_label: the centred turn banner.
        TurnBanner=BattlePanel(WidgetTree,Srgb(.012f,.010f,.020f,.86f),Srgb(.82f,.70f,.46f,.85f),1.f*U);BattlePlace(Canvas,TurnBanner,.36f,.417f,.64f,.468f);
        Turn=BattleText(WidgetTree,TEXT(""),17,Srgb(.95f,.86f,.62f),EFont::Title);Turn->SetJustification(ETextJustify::Center);TurnBanner->SetContent(Turn);TurnBanner->SetVerticalAlignment(VAlign_Center);
        // _build_player_panel: portrait, HP, and the limit / momentum / combo reading.
        auto* PlayerPanel=BattlePanel(WidgetTree,Srgb(.030f,.034f,.052f,.93f),Srgb(.36f,.50f,.74f,.62f),1.f*U);PlayerPanel->SetPadding(FMargin(8*U,6*U));
        BattlePlace(Canvas,PlayerPanel,.02f,.6f,.345f,.742f);
        auto* PlayerRow=WidgetTree->ConstructWidget<UHorizontalBox>();PlayerPanel->SetContent(PlayerRow);
        auto* Face=BattleArt(WidgetTree,MemoriaNarrativeArtwork::Load(TEXT("res://assets/portraits/arrel_face_determined.png")));auto* FaceFit=WidgetTree->ConstructWidget<UScaleBox>();FaceFit->SetStretch(EStretch::ScaleToFill);FaceFit->AddChild(Face);
        auto* FaceSlot=PlayerRow->AddChildToHorizontalBox(Sized(WidgetTree,FaceFit,50*U,50*U));FaceSlot->SetVerticalAlignment(VAlign_Center);FaceSlot->SetPadding(FMargin(0,0,9*U,0));
        auto* PlayerRows=WidgetTree->ConstructWidget<UVerticalBox>();auto* RowsSlot=PlayerRow->AddChildToHorizontalBox(PlayerRows);RowsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));RowsSlot->SetVerticalAlignment(VAlign_Center);
        auto* PlayerTop=WidgetTree->ConstructWidget<UHorizontalBox>();PlayerRows->AddChildToVerticalBox(PlayerTop);
        PlayerName=BattleText(WidgetTree,TEXT(""),16,BattlePaper,EFont::Title);PlayerTop->AddChildToHorizontalBox(PlayerName)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        PlayerHealth=BattleText(WidgetTree,TEXT(""),12,Srgb(.72f,.80f,.90f));PlayerHealth->SetAutoWrapText(false);PlayerHealth->SetJustification(ETextJustify::Right);PlayerTop->AddChildToHorizontalBox(PlayerHealth)->SetVerticalAlignment(VAlign_Center);
        PlayerBar=BattleHealthBar(WidgetTree,Srgb(.24f,.47f,.66f));PlayerRows->AddChildToVerticalBox(Sized(WidgetTree,PlayerBar,0,12*U))->SetPadding(FMargin(0,3*U));
        const auto Gauge=[&](UTextBlock*& Label,UProgressBar*& Bar,FLinearColor Color)
        {
            auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();PlayerRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,1*U));
            Label=BattleText(WidgetTree,TEXT(""),11,Color);Label->SetAutoWrapText(false);Row->AddChildToHorizontalBox(Sized(WidgetTree,Label,110*U,0))->SetVerticalAlignment(VAlign_Center);
            Bar=BattleHealthBar(WidgetTree,Color);auto* S=Row->AddChildToHorizontalBox(Sized(WidgetTree,Bar,0,6*U));S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetVerticalAlignment(VAlign_Center);
        };
        {UTextBlock* L=nullptr;UProgressBar* B=nullptr;Gauge(L,B,BattleLimit);LimitLabel=L;LimitBar=B;}
        {UTextBlock* L=nullptr;UProgressBar* B=nullptr;Gauge(L,B,Srgb(.86f,.66f,.36f));MomentumLabel=L;MomentumBar=B;}
        Gauges=BattleText(WidgetTree,TEXT(""),11,Srgb(.80f,.78f,.74f));PlayerRows->AddChildToVerticalBox(Gauges);
        // _build_log_panel: the field readout frame and the latest lines.
        const FBox2f ReadoutRegion(FVector2f(34.f/1962.f,282.f/801.f),FVector2f(1928.f/1962.f,508.f/801.f));
        BattlePlace(Canvas,BattleArt(WidgetTree,MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_battle_field_readout_v4.png")),ReadoutRegion),.35f,.665f,.85f,.772f);
        auto* LogPanel=BattlePanel(WidgetTree,Srgb(.012f,.018f,.028f,.88f));LogPanel->SetPadding(FMargin(8*U,2*U));BattlePlace(Canvas,LogPanel,.385f,.688f,.815f,.748f);
        auto* LogRow=WidgetTree->ConstructWidget<UHorizontalBox>();LogPanel->SetContent(LogRow);
        auto* LogHeader=BattleText(WidgetTree,TEXT(""),12,Srgb(.80f,.70f,.52f));LogHeader->SetAutoWrapText(false);ReadoutHeader=LogHeader;
        LogRow->AddChildToHorizontalBox(Sized(WidgetTree,LogHeader,104*U,0))->SetVerticalAlignment(VAlign_Center);
        LogScroll=WidgetTree->ConstructWidget<UScrollBox>();LogScroll->SetScrollbarThickness(FVector2D(2,2));LogScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
        auto* LogSlot=LogRow->AddChildToHorizontalBox(LogScroll);LogSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BattleLog=BattleText(WidgetTree,TEXT(""),12,BattlePaper,EFont::Body);BattleLog->SetAutoWrapText(false);LogScroll->AddChild(BattleLog);
        // _build_action_buttons: the command deck art and its numbered two-line commands.
        const FBox2f DeckRegion(FVector2f(8.f/1672.f,284.f/941.f),FVector2f(1664.f/1672.f,640.f/941.f));
        BattlePlace(Canvas,BattleArt(WidgetTree,MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_battle_command_deck_v4.png")),DeckRegion,.96f),.035f,.758f,.965f,.995f);
        auto* Deck=WidgetTree->ConstructWidget<UUniformGridPanel>();Deck->SetSlotPadding(FMargin(5*U,3.5f*U));BattlePlace(Canvas,Deck,.145f,.802f,.855f,.978f);
        for(int32 I=0;I<6;++I)
        {
            auto* Button=WidgetTree->ConstructWidget<UMemoriaBattleEntryButton>();Button->Owner=this;Button->Index=I;
            Button->OnClicked.AddDynamic(Button,&UMemoriaBattleEntryButton::Activate);
            auto* Label=BattleText(WidgetTree,TEXT(""),15,BattlePaper);Label->SetJustification(ETextJustify::Center);Button->SetContent(Label);
            {FSlateFontInfo Font=MemoriaFonts::Get(EFont::Ui,Pt(14));Font.OutlineSettings.OutlineSize=1;Font.OutlineSettings.OutlineColor=FLinearColor(0,0,0,.72f);Label->SetFont(Font);}
            if(auto* S=Cast<UButtonSlot>(Button->GetContentSlot())){S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Center);}
            auto* Cell=Deck->AddChildToUniformGrid(Button,I/3,I%3);Cell->SetHorizontalAlignment(HAlign_Fill);Cell->SetVerticalAlignment(VAlign_Fill);
            ActionButtons.Add(Button);ActionLabels.Add(Label);
        }
        FleeButton=ActionButtons[5];FleeLabel=ActionLabels[5];
        // Burn and item lists open over the stage in the tactical plate.
        ChoicePanel=BattlePanel(WidgetTree,Srgb(.018f,.014f,.026f,.96f),Srgb(.82f,.64f,.34f,.72f),1.f*U);ChoicePanel->SetPadding(FMargin(18*U,14*U));BattlePlace(Canvas,ChoicePanel,.30f,.16f,.70f,.66f);
        auto* ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>();ChoicePanel->SetContent(ChoiceScroll);
        ChoiceList=WidgetTree->ConstructWidget<UVerticalBox>();ChoiceScroll->AddChild(ChoiceList);
        // Telegraph band: the cue owns its own plate over the stage centre, where the turn banner sits.
        CueBand=BattlePanel(WidgetTree,Srgb(.012f,.008f,.014f,.90f),Srgb(.86f,.52f,.26f,.70f),1.f*U);BattlePlace(Canvas,CueBand,.27f,.37f,.73f,.53f);
        CueText=BattleText(WidgetTree,TEXT(""),30,BattlePaper,EFont::Title);CueText->SetJustification(ETextJustify::Center);BattlePlace(Canvas,CueText,.27f,.385f,.73f,.46f);
        BurnCost=BattleText(WidgetTree,TEXT(""),14,BattleGold,EFont::Body);BurnCost->SetJustification(ETextJustify::Center);BattlePlace(Canvas,BurnCost,.28f,.465f,.72f,.52f);
        ImpactText=BattleText(WidgetTree,TEXT(""),40,BattlePaper,EFont::Title);ImpactText->SetJustification(ETextJustify::Center);BattlePlace(Canvas,ImpactText,.40f,.26f,.60f,.36f);
        // ui_battle_victory_reward_panel: the result card.
        // _show_victory: the art is centred at 520x490 and cover-fitted, which shows its middle frame (alpha .86).
        VictoryCard=BattlePanel(WidgetTree,FLinearColor::Transparent);VictoryCard->SetPadding(FMargin(0));BattlePlace(Canvas,VictoryCard,.297f,.16f,.703f,.84f);
        auto* VictoryLayer=WidgetTree->ConstructWidget<UCanvasPanel>();VictoryCard->SetContent(VictoryLayer);
        const float CoverU=(520.f/490.f)/(1672.f/941.f);
        // Its black ground feathers into the stage through the plate blend instead of reading as a box.
        auto* VictoryArt=WidgetTree->ConstructWidget<UImage>();VictoryArt->SetVisibility(ESlateVisibility::HitTestInvisible);BattlePlace(VictoryLayer,VictoryArt,0,0,1,1);
        SetPlate(VictoryArt,VictoryPlate,MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_battle_victory_reward_panel.png")),.16f,.35f,FLinearColor(1,1,1,.86f),
            FLinearColor(.5f-CoverU*.5f,0,.5f+CoverU*.5f,1));
        BattlePlace(VictoryLayer,BattlePanel(WidgetTree,Srgb(.035f,.030f,.052f,.55f)),.25f,.19f,.75f,.87f);
        auto* VictoryBox=WidgetTree->ConstructWidget<UVerticalBox>();BattlePlace(VictoryLayer,VictoryBox,.27f,.22f,.73f,.86f);
        VictoryTitle=BattleText(WidgetTree,TEXT(""),30,BattlePaper,EFont::Title);VictoryGrade=BattleText(WidgetTree,TEXT(""),15,Srgb(.95f,.72f,.40f));VictoryBreakdown=BattleText(WidgetTree,TEXT(""),14,BattlePaper,EFont::Body);
        for(UTextBlock* Line:{VictoryTitle.Get(),VictoryGrade.Get(),VictoryBreakdown.Get()}){Line->SetJustification(ETextJustify::Center);VictoryBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0,4*U));}
        SetPlate(PlayerArt,PlayerPlate,MemoriaBattleEntryArt::Load(TEXT("res://assets/portraits/character_shots/arrel_battle_v3.png")),PlayerRole.Edge,PlayerRole.Oval,PlayerRole.Modulate);
        PlayerTexture=MemoriaBattleEntryArt::Load(TEXT("res://assets/portraits/character_shots/arrel_battle_v3.png"));
        AllyTexture=MemoriaBattleEntryArt::Load(TEXT("res://assets/portraits/character_shots/elia_anchor_v3.png"));
        SetPlate(AllyArt,AllyPlate,AllyTexture,AllyRole.Edge,AllyRole.Oval,AllyRole.Modulate);
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
    const bool Ko=View.bKo;
    const FString Reading=FString::Printf(TEXT("%s %d/%d"),Ko?TEXT("증언"):TEXT("WITNESS"),View.WitnessProgress,View.WitnessRequired);
    const TArray<FString> Labels=Ko?TArray<FString>{TEXT("공격"),TEXT("기억 연소"),Reading,TEXT("방어"),TEXT("아이템"),TEXT("도주")}:TArray<FString>{TEXT("ATTACK"),TEXT("BURN"),Reading,TEXT("GUARD"),TEXT("ITEM"),TEXT("FLEE")};
    // _format_action_button: the command and what it costs or buys, on two lines.
    const TArray<FString> Roles=Ko?TArray<FString>{TEXT("피해 + 브레이크"),TEXT("화력 / 영구 대가"),TEXT("정체성 보존"),TEXT("피해 절반 + 리밋"),TEXT("전투 도구 사용"),TEXT("전투 이탈")}
        :TArray<FString>{TEXT("Damage + BREAK"),TEXT("Power / permanent cost"),TEXT("Preserve identity"),TEXT("Half damage + Limit"),TEXT("Use a field kit"),TEXT("Leave encounter")};
    const FLinearColor Bases[6]={Srgb(.045f,.040f,.058f,.88f),Srgb(.115f,.038f,.060f,.91f),Srgb(.035f,.075f,.105f,.90f),Srgb(.040f,.070f,.090f,.90f),Srgb(.045f,.040f,.058f,.88f),Srgb(.045f,.040f,.058f,.88f)};
    for(int32 I=0;I<ActionButtons.Num();++I)
    {
        // Result choices keep the deck's cell size; the unused cells stay empty.
        const bool Visible=View.bVictory?I==0:View.bDefeat?I<2:true;
        ActionButtons[I]->SetVisibility(Visible?ESlateVisibility::Visible:ESlateVisibility::Hidden);
        FString Label;
        if(View.bVictory)Label=Ko?TEXT("베르단으로"):TEXT("CONTINUE");
        else if(View.bDefeat)Label=I==0?(Ko?TEXT("체크포인트"):TEXT("CHECKPOINT")):(Ko?TEXT("베르단으로"):TEXT("VERDAN"));
        else Label=FString::Printf(TEXT("%d · %s\n%s"),I+1,*Labels[I],*Roles[I]);
        ActionLabels[I]->SetText(FText::FromString(Label));
        ActionButtons[I]->SetIsEnabled(View.bActive&&!View.bResolving&&!View.bReturning&&(I!=2||View.bVictory||View.bDefeat||View.WitnessProgress<View.WitnessRequired));
        const bool bSel=I==Selected;
        ActionLabels[I]->SetColorAndOpacity(I==2&&!View.bVictory&&!View.bDefeat?BattleWitness:bSel?Srgb(.95f,.8f,.5f):BattlePaper);
        ActionButtons[I]->SetStyle(CommandStyle(View.bVictory||View.bDefeat?Bases[0]:Bases[I],bSel));
    }
    BoundaryNote->SetText(FText::FromString(Ko?TEXT("방향키  선택  ·  Enter / E  확인  ·  Esc  목록 닫기"):TEXT("ARROWS  select  ·  ENTER / E  confirm  ·  ESC  close list")));
    ChoicePanel->SetVisibility(PanelMode?ESlateVisibility::Visible:ESlateVisibility::Collapsed);ChoiceList->ClearChildren();
    if(PanelMode)
    {
        auto* Title=BattleText(WidgetTree,PanelMode==1?(Ko?TEXT("태울 기억을 고른다"):TEXT("CHOOSE A MEMORY TO BURN")):(Ko?TEXT("전투 도구"):TEXT("FIELD KIT")),15,Srgb(.95f,.72f,.40f),EFont::Title);
        ChoiceList->AddChildToVerticalBox(Title)->SetPadding(FMargin(0,0,0,6*U));
        const auto& Choices=PanelMode==1?View.Memories:View.Items;
        for(int32 I=0;I<Choices.Num();++I)
        {
            const auto& C=Choices[I];auto* B=WidgetTree->ConstructWidget<UMemoriaBattleEntryButton>();B->Owner=this;B->Index=I;B->bChoice=true;B->SetIsEnabled(C.bAvailable);
            B->OnClicked.AddDynamic(B,&UMemoriaBattleEntryButton::Activate);B->SetStyle(CommandStyle(Srgb(.035f,.030f,.048f,.92f),I==ChoiceSelected));
            const FString Label=(PanelMode==1?FString::Printf(TEXT("G%d  "),5-C.Grade):FString())+C.Label+(C.bAvailable?TEXT(""):(Ko?TEXT("  · 사용 불가"):TEXT("  · unavailable")));
            auto* T=BattleText(WidgetTree,Label,14,C.bAvailable?(PanelMode==1?C.Accent:BattlePaper):BattleMuted);B->SetContent(T);auto* ChoiceSlot=ChoiceList->AddChildToVerticalBox(B);ChoiceSlot->SetPadding(FMargin(0,3*U));
        }
    }
    FString Status;
    for(const auto& S:View.PlayerStatuses)Status+=FString::Printf(TEXT("  %s %d"),S.Effect==0?(Ko?TEXT("독"):TEXT("POISON")):S.Effect==1?(Ko?TEXT("약화"):TEXT("WEAK")):(Ko?TEXT("화상"):TEXT("BURN")),S.Turns);
    Gauges->SetText(FText::FromString(FString::Printf(TEXT("%s %d%s"),Ko?TEXT("콤보"):TEXT("COMBO"),View.Combo,*Status)));
    BreakLabel->SetText(FText::FromString(View.BrokenTurns>0?(Ko?FString::Printf(TEXT("브레이크  %d턴"),View.BrokenTurns):FString::Printf(TEXT("BROKEN  %d"),View.BrokenTurns)):FString::Printf(TEXT("%s  %.0f"),Ko?TEXT("브레이크"):TEXT("BREAK"),View.BreakGauge)));
    BreakBar->SetPercent(View.BrokenTurns>0?1.f:FMath::Clamp(float(View.BreakGauge)/100.f,0.f,1.f));
    MomentumLabel->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0f"),Ko?TEXT("기세"):TEXT("MOMENTUM"),View.Momentum)));
    MomentumBar->SetPercent(FMath::Clamp(float(View.Momentum)/100.f,0.f,1.f));
    LimitLabel->SetText(FText::FromString(View.LimitGauge>=100?(Ko?TEXT("리밋  MAX"):TEXT("LIMIT  MAX")):FString::Printf(TEXT("%s  %.0f"),Ko?TEXT("리밋"):TEXT("LIMIT"),View.LimitGauge)));
    LimitBar->SetPercent(FMath::Clamp(float(View.LimitGauge)/100.f,0.f,1.f));
    const bool Fighting=View.bActive&&!View.bVictory&&!View.bDefeat;
    WitnessLabel->SetText(FText::FromString(Fighting?(Ko?TEXT("증언"):TEXT("WITNESS")):FString()));
    // Witness pips: filled once heard, dim while still unread.
    const int32 Pips=Fighting?View.WitnessRequired:0;
    if(Pips!=PipCount||View.WitnessProgress!=PipHeard)
    {
        PipCount=Pips;PipHeard=View.WitnessProgress;WitnessPips->ClearChildren();
        for(int32 I=0;I<Pips;++I)
        {
            const bool Heard=I<View.WitnessProgress;
            auto* Pip=BattlePanel(WidgetTree,Heard?BattleWitness:FLinearColor(.01f,.02f,.025f,.95f),FLinearColor(.30f,.44f,.45f,.9f),Heard?0.f:1.f*U);
            WitnessPips->AddChildToHorizontalBox(Sized(WidgetTree,Pip,11*U,11*U))->SetPadding(FMargin(0,0,4*U,0));
        }
    }
    EchoText->SetText(FText::FromString(View.WitnessLine));
    const bool Echo=Fighting&&View.WitnessProgress>0&&!View.WitnessLine.IsEmpty();
    EchoBand->SetVisibility(Echo?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    Modifier->SetVisibility(Echo?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    VictoryCard->SetVisibility(View.bVictory?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(View.bVictory)
    {
        const auto& R=View.Reward;const bool Released=View.bResolvedByWitness;
        VictoryTitle->SetText(FText::FromString(Released?(Ko?TEXT("해방"):TEXT("RELEASED")):(Ko?TEXT("승리"):TEXT("VICTORY"))));
        VictoryTitle->SetColorAndOpacity(Released?BattleWitness:Srgb(.97f,.86f,.55f));
        VictoryGrade->SetText(FText::FromString(FString::Printf(TEXT("%s  %s   ·   %d"),Ko?TEXT("전투 등급"):TEXT("BATTLE GRADE"),*R.Grade,R.Score)));
        TArray<FString> Lines;
        Lines.Add(FString::Printf(TEXT("+%lld %s   ·   HP +%lld"),R.Grains,Ko?TEXT("그레인"):TEXT("Grains"),R.Heal));
        TArray<FString> Parts;
        if(R.PreservationBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),Ko?TEXT("보존"):TEXT("Preservation"),R.PreservationBonus));
        if(R.TacticalBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),Ko?TEXT("기록"):TEXT("Record"),R.TacticalBonus));
        if(R.ObjectiveBonus>0)Parts.Add(FString::Printf(TEXT("%s +%lld"),*View.ObjectiveTitle,R.ObjectiveBonus));
        if(R.GradeBonus>0)Parts.Add(FString::Printf(TEXT("%s %s +%lld"),Ko?TEXT("등급"):TEXT("Grade"),*R.Grade,R.GradeBonus));
        if(!Parts.IsEmpty())Lines.Add(FString::Join(Parts,TEXT("   ·   ")));
        if(R.FocusGained>0)Lines.Add(FString::Printf(TEXT("%s +%lld"),Ko?TEXT("필드 집중"):TEXT("Field Focus"),R.FocusGained));
        if(!R.Item.IsEmpty())Lines.Add(FString::Printf(TEXT("%s %s"),Ko?TEXT("획득:"):TEXT("Found:"),*R.Item));
        // Source aftermath line for a fight ended by listening instead of burning.
        if(Released)Lines.Add(Ko?TEXT("\n이곳에서 태운 것은 없다. 이름은 제자리로 돌아갔다."):TEXT("\nNothing was burned here. The name went back where it belonged."));
        VictoryBreakdown->SetText(FText::FromString(FString::Join(Lines,TEXT("\n"))));
    }
    if(View.bDefeat)BattleLog->SetText(FText::FromString(Ko?TEXT("아렐이 쓰러졌다. 체크포인트를 불러오거나, 잃은 기억을 안고 베르단으로 돌아간다."):TEXT("Arrel falls. Load a checkpoint, or return to Verdan carrying the cost of the memories burned.")));
}
void UMemoriaBattleEntryWidget::Draw()
{
    if(!Heading)return;
    const bool Ko=View.bKo;
    Heading->SetText(FText::FromString(View.EnvironmentName));
    ObservationTitle->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %s"),Ko?TEXT("전장 관측"):TEXT("FIELD READ"),*View.EnvironmentName)));
    if(ReadoutHeader)ReadoutHeader->SetText(FText::FromString(Ko?TEXT("최근 전황"):TEXT("FIELD LOG")));
    Turn->SetText(FText::FromString(View.bReturning?(Ko?TEXT("돌아가는 중"):TEXT("RETURNING")):View.bVictory?(Ko?TEXT("승리"):TEXT("VICTORY")):View.bDefeat?(Ko?TEXT("쓰러졌습니다"):TEXT("DEFEAT")):View.bResolving?(Ko?TEXT("행동 진행 중"):TEXT("ACTION IN PROGRESS")):(Ko?TEXT("당신의 턴"):TEXT("YOUR TURN"))));
    PlayerName->SetText(FText::FromString(Ko?TEXT("아렐"):TEXT("ARREL")));
    PlayerHealth->SetText(FText::FromString(FString::Printf(TEXT("HP  %lld / %lld"),View.PlayerHp,View.PlayerMaxHp)));
    PlayerBar->SetPercent(View.PlayerMaxHp>0?FMath::Clamp(float(View.PlayerHp)/View.PlayerMaxHp,0.f,1.f):0.f);
    AllyName->SetText(FText::FromString(Ko?TEXT("엘리아"):TEXT("ELIA")));
    for(UWidget* W:AllyStage)if(W)W->SetVisibility(View.bEliaInParty?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    EnemyName->SetText(FText::FromString(View.EnemyName));EnemyHealth->SetText(FText::FromString(FString::Printf(TEXT("HP  %lld / %lld"),View.EnemyHp,View.EnemyMaxHp)));
    EnemyBar->SetPercent(View.EnemyMaxHp>0?FMath::Clamp(float(View.EnemyHp)/View.EnemyMaxHp,0.f,1.f):0.f);
    ObjectiveTitle->SetText(FText::FromString(View.ObjectiveTitle.IsEmpty()?FString():FString::Printf(TEXT("%s  -  %s"),Ko?TEXT("목표"):TEXT("OBJECTIVE"),*View.ObjectiveTitle)));
    ObjectiveBody->SetText(FText::FromString(View.ObjectiveDescription+(View.ObjectiveProgress.IsEmpty()?FString():TEXT("\n")+View.ObjectiveProgress)));
    if(!View.bObjectiveSupported)ObjectiveBody->SetText(FText::FromString(Ko?TEXT("이번 단계 미지원 · 보상 없음"):TEXT("Unavailable in this chapter slice · no reward")));
    else if(View.bObjectiveComplete||View.bObjectiveFailed)ObjectiveBody->SetText(FText::FromString(View.bObjectiveComplete?(Ko?TEXT("목표 달성"):TEXT("COMPLETE")):(Ko?TEXT("목표 실패"):TEXT("FAILED"))));
    Modifier->SetText(FText::FromString(View.ModifierName.IsEmpty()?(Ko?TEXT("어둠 속에서 무언가 꿈틀거린다..."):TEXT("Something stirs in the dark...")):
        View.ModifierName+(View.ModifierDescription.IsEmpty()?FString():TEXT(" — ")+View.ModifierDescription)));
    // The readout strip holds the two latest lines, as the source's recent-feed label does.
    TArray<FString> Recent;for(int32 I=FMath::Max(0,View.Logs.Num()-2);I<View.Logs.Num();++I)Recent.Add(View.Logs[I]);
    BattleLog->SetText(FText::FromString(FString::Join(Recent,TEXT("\n"))));
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
    if(EnemySource!=EnemyShown||!EnemyTexture)
    {
        EnemyShown=EnemySource;EnemyTexture=MemoriaBattleEntryArt::Load(EnemySource);
        // The source procedural fallback is a provisional 128px sprite: kept crisp and at native scale,
        // with the fallback's thin edge (_apply_battle_portrait_blend 0.02, no oval) instead of the plate's.
        const bool Pixel=EnemySource==MemoriaBattleEntryArt::MarketThiefSource();
        SetPlate(EnemyArt,EnemyPlate,EnemyTexture,Pixel?.02f:EnemyRole.Edge,Pixel?0.f:EnemyRole.Oval,Pixel?FLinearColor::White:EnemyRole.Modulate);
        if(auto* Fit=Cast<UScaleBox>(EnemyArt->GetParent()))Fit->SetStretchDirection(Pixel?EStretchDirection::DownOnly:EStretchDirection::Both);
        if(EnemyThumb)EnemyThumb->SetBrushFromTexture(EnemyTexture,true);
    }
    const FString Background=View.BackgroundSource.IsEmpty()?TEXT("res://assets/cg/generated/chapter_splash_verdan_market.png"):View.BackgroundSource;
    BackdropTexture=MemoriaBattleEntryArt::Load(Background);Backdrop->SetBrushFromTexture(BackdropTexture,true);
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
    if(Geometry.GetLocalSize().X>0)Area=Geometry.GetLocalSize();
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
    // The turn banner yields its place to a telegraph and to the result card.
    if(TurnBanner)TurnBanner->SetVisibility(Burning||Reading||View.bVictory?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
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
    if(ImpactText)
    {
        // The number rises over whoever was hit: Arrel's plate centre or the enemy's.
        const float TargetX=bHitPlayer?(PlayerRole.L+PlayerRole.R)*.5f:(EnemyRole.L+EnemyRole.R)*.5f;
        ImpactText->SetText(FText::FromString(ImpactAge<1.f&&ImpactDamage!=0?FString::Printf(TEXT("%s%lld"),ImpactDamage<0?TEXT("+"):TEXT(""),FMath::Abs(ImpactDamage)):FString()));
        ImpactText->SetRenderTranslation(FVector2D((TargetX-.5f)*Area.X,-T*70*U));ImpactText->SetRenderOpacity(FMath::Clamp(1.f-T,0.f,1.f));
    }
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
    const FVector2f EnemyOrigin(float(Size.X)*EnemyRole.L,float(Size.Y)*EnemyRole.T),EnemySize(float(Size.X)*(EnemyRole.R-EnemyRole.L),float(Size.Y)*(EnemyRole.B-EnemyRole.T));
    // A witness reading washes the enemy with archive light; bright only at the centre.
    if(WitnessAge<.6f&&!View.bDefeat)
        FSlateDrawElement::MakeBox(Elements,LastLayer+2,Geometry.ToPaintGeometry(EnemySize*FVector2f(.6f,.8f),FSlateLayoutTransform(EnemyOrigin+EnemySize*FVector2f(.2f,.1f))),Brush,ESlateDrawEffect::None,
            FLinearColor(BattleWitness.R,BattleWitness.G,BattleWitness.B,.16f*(1-WitnessAge/.6f)*Opacity));
    if(View.bVictory&&View.bResolvedByWitness&&VictoryAge<3.f)
        for(int32 I=0;I<14;++I)
        {
            // Released names drift upward out of the enemy.
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
UTexture2D* UMemoriaBattleEntryWidget::DisplayedBackdrop() const{return BackdropTexture;}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedPlayerArtwork() const{return PlayerArt?PlayerTexture.Get():nullptr;}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedEnemyArtwork() const{return EnemyTexture;}
UTexture2D* UMemoriaBattleEntryWidget::DisplayedAllyArtwork() const{return View.bEliaInParty?AllyTexture.Get():nullptr;}
bool UMemoriaBattleEntryWidget::IsFleeEnabled() const{return FleeButton && FleeButton->GetIsEnabled();}
bool UMemoriaBattleEntryWidget::IsWitnessEnabled() const{return ActionButtons.IsValidIndex(2)&&ActionButtons[2]->GetIsEnabled();}
bool UMemoriaBattleEntryWidget::IsVictoryCardVisible() const{return VictoryCard&&VictoryCard->GetVisibility()!=ESlateVisibility::Collapsed;}
