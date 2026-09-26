#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaShopWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
namespace
{
const FLinearColor Gold(.66f,.49f,.27f), Paper(.88f,.85f,.77f), Ink(.006f,.008f,.012f,.97f);
// Godot 2D colors are sRGB; Slate colors and tints are linear. Source colors pass through this.
FLinearColor Srgb(float R,float G,float B,float A=1.f)
{ FLinearColor C=FLinearColor::FromSRGBColor(FColor(uint8(R*255.f+.5f),uint8(G*255.f+.5f),uint8(B*255.f+.5f)));C.A=A;return C; }
// vn_scene.gd PORTRAIT_DIM / PORTRAIT_BRIGHT and the name / system label colors.
const FLinearColor Dim=Srgb(.45f,.45f,.5f), NameGold=Srgb(.97f,.86f,.55f), SystemCyan=Srgb(.5f,.85f,.95f), Distorted=Srgb(.74f,.62f,.95f);
using EFont=MemoriaFonts::EStyle;
void Position(UWidget* Widget, float L,float T,float R,float B)
{
    auto* Slot=Cast<UCanvasPanelSlot>(Widget->Slot); Slot->SetAnchors(FAnchors(L,T,R,B)); Slot->SetOffsets(FMargin(0));
}
void Place(UCanvasPanel* Canvas,UWidget* Widget,float L,float T,float R,float B)
{ Canvas->AddChildToCanvas(Widget); Position(Widget,L,T,R,B); }
UTextBlock* Text(UWidgetTree* Tree,int32 Size,FLinearColor Color,EFont Font=EFont::Body)
{
    auto* W=Tree->ConstructWidget<UTextBlock>();W->SetAutoWrapText(true);W->SetColorAndOpacity(Color);
    W->SetFont(MemoriaFonts::Get(Font,Size));return W;
}
void Shadow(UTextBlock* W,float Alpha=.85f){W->SetShadowOffset(FVector2D(1.f,1.f));W->SetShadowColorAndOpacity(FLinearColor(0,0,0,Alpha));}
UCanvasPanel* Layer(UWidgetTree* Tree,UCanvasPanel* Parent)
{ auto* C=Tree->ConstructWidget<UCanvasPanel>();Place(Parent,C,0,0,1,1);C->SetVisibility(ESlateVisibility::HitTestInvisible);return C; }
UBorder* Fill(UWidgetTree* Tree,const FLinearColor& Color)
{ auto* B=Tree->ConstructWidget<UBorder>();B->SetBrushColor(Color);B->SetVisibility(ESlateVisibility::HitTestInvisible);return B; }
FButtonStyle ChoiceStyle(bool Selected)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateRoundedBoxBrush(Selected?FLinearColor(.06f,.044f,.023f):FLinearColor(.012f,.016f,.022f),2.f,Selected?Gold:FLinearColor(.12f,.12f,.12f),1.f));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.06f,.044f,.023f),2.f,Gold,1.f));Style.SetPressed(Style.Hovered);return Style;
}
// Story choices: inside the archive frame the art draws the borders, so a slot only warms when selected.
FButtonStyle StoryChoiceStyle(bool Selected,bool Framed)
{
    FButtonStyle Style;
    const FLinearColor Warm=Srgb(.95f,.62f,.25f,.16f),Line=Srgb(.86f,.69f,.4f,.75f);
    if(Framed)
    {
        Style.SetNormal(FSlateRoundedBoxBrush(Selected?Warm:FLinearColor(0,0,0,.18f),3.f,Selected?Line:FLinearColor::Transparent,1.f));
        Style.SetHovered(FSlateRoundedBoxBrush(Warm,3.f,Line,1.f));
    }
    else
    {
        Style.SetNormal(FSlateRoundedBoxBrush(Selected?Srgb(.12f,.085f,.04f,.92f):Srgb(.02f,.018f,.028f,.84f),3.f,Selected?Line:Srgb(.35f,.28f,.18f,.8f),1.f));
        Style.SetHovered(FSlateRoundedBoxBrush(Srgb(.12f,.085f,.04f,.92f),3.f,Line,1.f));
    }
    Style.SetPressed(Style.Hovered);return Style;
}
struct FStop { float T; FLinearColor C; };
// Godot GradientTexture2D: linear or radial fill from From to To in UV space; colors are authored sRGB bytes.
UTexture2D* Gradient(int32 W,int32 H,FVector2D From,FVector2D To,bool bRadial,std::initializer_list<FStop> Stops)
{
    auto* Texture=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);
    if(!Texture)return nullptr;
    Texture->SRGB=true;Texture->Filter=TF_Bilinear;Texture->AddressX=TA_Clamp;Texture->AddressY=TA_Clamp;
    FColor* Pixels=static_cast<FColor*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    const FVector2D Axis=To-From;const double Length=FMath::Max(Axis.Size(),1e-6),Length2=FMath::Max(Axis.SizeSquared(),1e-9);
    const TArray<FStop> S(Stops);
    for(int32 Y=0;Y<H;++Y)for(int32 X=0;X<W;++X)
    {
        const FVector2D UV((X+.5)/W,(Y+.5)/H);
        const float T=FMath::Clamp(float(bRadial?(UV-From).Size()/Length:FVector2D::DotProduct(UV-From,Axis)/Length2),0.f,1.f);
        FLinearColor C=S.Last().C;
        for(int32 I=1;I<S.Num();++I)if(T<=S[I].T){const float U=(T-S[I-1].T)/FMath::Max(S[I].T-S[I-1].T,1e-6f);C=FMath::Lerp(S[I-1].C,S[I].C,U);break;}
        if(T<=S[0].T)C=S[0].C;
        Pixels[Y*W+X]=FColor(uint8(FMath::Clamp(C.R,0.f,1.f)*255.f+.5f),uint8(FMath::Clamp(C.G,0.f,1.f)*255.f+.5f),uint8(FMath::Clamp(C.B,0.f,1.f)*255.f+.5f),uint8(FMath::Clamp(C.A,0.f,1.f)*255.f+.5f));
    }
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock();Texture->UpdateResource();return Texture;
}
UImage* GradientImage(UWidgetTree* Tree,UTexture2D* Texture,const FLinearColor& Tint)
{ auto* I=Tree->ConstructWidget<UImage>();I->SetBrushFromTexture(Texture,false);I->SetColorAndOpacity(Tint);I->SetVisibility(ESlateVisibility::HitTestInvisible);return I; }
FString Escape(const FString& S)
{ return S.Replace(TEXT("&"),TEXT("&amp;")).Replace(TEXT("<"),TEXT("&lt;")).Replace(TEXT(">"),TEXT("&gt;")).Replace(TEXT("\""),TEXT("&quot;")); }
// vn_scene.gd _scramble_text glyphs, chosen by position so a capture is repeatable.
FString Scramble(const FString& S)
{
    static const FString Glyphs=TEXT("▓▒░█▄▀#@%&*?!");FString Out=S;
    for(int32 I=0;I<Out.Len();++I)if(Out[I]!=TCHAR(' ')&&Out[I]!=TCHAR('\n'))Out[I]=Glyphs[(I*7+3)%Glyphs.Len()];
    return Out;
}
// _portrait_accent_for_id: each character's frame tint.
FLinearColor PortraitAccent(const FString& Source)
{
    const FString Key=FPaths::GetBaseFilename(Source).ToLower();
    if(Key.StartsWith(TEXT("arrel")))return Srgb(.72f,.76f,.95f);
    if(Key.StartsWith(TEXT("elia")))return Srgb(.62f,.82f,.92f);
    if(Key.StartsWith(TEXT("sable")))return Srgb(.74f,.62f,.92f);
    if(Key.StartsWith(TEXT("malet")))return Srgb(.92f,.64f,.34f);
    return Srgb(.72f,.64f,.46f);
}
// Portrait slots, 1280x720 source offsets as anchors (PORTRAIT_SIZE 300). The source hangs them
// from y 232..548, behind its name plate; here they sit above the plate and the frame.
const float PlateX[2][2]={{.009f,.256f},{.744f,.991f}},ImageX[2][2]={{.0156f,.25f},{.75f,.9844f}},ShadowX[2][2]={{-.005f,.284f},{.716f,1.005f}};
// The archive frame art is placed at (.07,.13)-(.93,.99); its three slots in that rect.
const float FrameL=.07f,FrameT=.13f,FrameR=.93f,FrameB=.99f;
const float SlotV[3][2]={{.192f,.41f},{.44f,.656f},{.682f,.862f}};
}
void UMemoriaNarrativeChoiceButton::Choose(){if(Owner)Owner->Choose(ChoiceIndex);}
void UMemoriaNarrativeChoiceButton::Hover(){if(Owner)Owner->HoverChoice(ChoiceIndex);}
TSharedRef<SWidget> UMemoriaDevelopmentNarrativeWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        // Illustration: cover-filled CG (STRETCH_KEEP_ASPECT_COVERED) with the crossfade source and chroma copies.
        Illustration=WidgetTree->ConstructWidget<UCanvasPanel>();Place(Canvas,Illustration,0,0,1,1);
        Place(Illustration,Fill(WidgetTree,Srgb(.02f,.02f,.03f)),0,0,1,1);
        auto Cover=[&](TObjectPtr<UImage>& Image)->UScaleBox*
        { auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFill);Place(Illustration,Fit,0,0,1,1);Image=WidgetTree->ConstructWidget<UImage>();Fit->AddChild(Image);return Fit; };
        Cover(BackdropPrevious);BackdropFit=Cover(Backdrop);ChromaRedFit=Cover(ChromaRed);ChromaBlueFit=Cover(ChromaBlue);
        ChromaRed->SetVisibility(ESlateVisibility::Collapsed);ChromaBlue->SetVisibility(ESlateVisibility::Collapsed);
        Illustration->SetVisibility(ESlateVisibility::HitTestInvisible);
        // Cinematic layers (vn_scene.gd): focus glow, lower wash, vignette, ember, letterbox.
        Cinema=Layer(WidgetTree,Canvas);
        Gradients.Add(Gradient(256,128,FVector2D(.5,.907),FVector2D(.85,.582),true,{{0.f,FLinearColor(.72f,.42f,.16f,.24f)},{.45f,FLinearColor(.3f,.18f,.1f,.12f)},{1.f,FLinearColor(0,0,0,0)}}));
        Gradients.Add(Gradient(4,256,FVector2D(0,0),FVector2D(0,1),false,{{0.f,FLinearColor(.018f,.014f,.022f,0)},{.55f,FLinearColor(.018f,.014f,.022f,1)},{1.f,FLinearColor(.018f,.014f,.022f,1)}}));
        Gradients.Add(Gradient(256,144,FVector2D(.5,.48),FVector2D(1.04,.52),true,{{0.f,FLinearColor(0,0,0,0)},{.58f,FLinearColor(0,0,0,.03f)},{.84f,FLinearColor(0,0,0,.22f)},{1.f,FLinearColor(0,0,0,.42f)}}));
        Gradients.Add(Gradient(128,128,FVector2D(.5,.5),FVector2D(1,.5),true,{{0.f,FLinearColor(.85f,.35f,.2f,0)},{.55f,FLinearColor(.85f,.3f,.15f,0)},{.85f,FLinearColor(.7f,.2f,.1f,.55f)},{1.f,FLinearColor(.4f,.1f,.05f,.85f)}}));
        Gradients.Add(Gradient(192,64,FVector2D(.5,.5),FVector2D(.98,.5),true,{{0.f,FLinearColor(0,0,0,.52f)},{.55f,FLinearColor(0,0,0,.3f)},{1.f,FLinearColor(0,0,0,0)}}));
        // Addition: the face portraits are painted on light parchment; an inner vignette settles them into the dark frame.
        Gradients.Add(Gradient(128,128,FVector2D(.5,.42),FVector2D(1.02,.5),true,{{0.f,FLinearColor(0,0,0,0)},{.55f,FLinearColor(0,0,0,0)},{.86f,FLinearColor(.05f,.04f,.06f,.62f)},{1.f,FLinearColor(.03f,.025f,.04f,.92f)}}));
        Place(Cinema,GradientImage(WidgetTree,Gradients[0],Srgb(1.f,.78f,.46f,.18f)),0,0,1,1);
        Place(Cinema,GradientImage(WidgetTree,Gradients[1],FLinearColor(1,1,1,.3f)),0,.54f,1,1);
        Place(Cinema,GradientImage(WidgetTree,Gradients[2],Srgb(.72f,.64f,.58f,.62f)),0,0,1,1);
        Ember=GradientImage(WidgetTree,Gradients[3],FLinearColor(1,1,1,0));Place(Cinema,Ember,0,0,1,1);
        Place(Cinema,Fill(WidgetTree,FLinearColor(0,0,0,.85f)),0,0,1,.083f);
        Place(Cinema,Fill(WidgetTree,FLinearColor(0,0,0,.85f)),0,.917f,1,1);
        // Story: portraits behind the memory frame dialogue box, and the archive choice frame.
        Story=WidgetTree->ConstructWidget<UCanvasPanel>();Place(Canvas,Story,0,0,1,1);
        for(int32 S=0;S<2;++S)
        {
            PortraitShadow[S]=GradientImage(WidgetTree,Gradients[4],FLinearColor::White);Place(Story,PortraitShadow[S],ShadowX[S][0],.5f,ShadowX[S][1],.66f);
            PortraitPlate[S]=WidgetTree->ConstructWidget<UBorder>();PortraitPlate[S]->SetBrush(FSlateRoundedBoxBrush(Srgb(.018f,.014f,.022f,.3f),6.f,Srgb(.72f,.64f,.46f,.72f),1.5f));
            PortraitPlate[S]->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Story,PortraitPlate[S],PlateX[S][0],.19f,PlateX[S][1],.592f);
            auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Place(Story,Fit,ImageX[S][0],.2f,ImageX[S][1],.582f);
            PortraitImage[S]=WidgetTree->ConstructWidget<UImage>();Fit->AddChild(PortraitImage[S]);Fit->SetVisibility(ESlateVisibility::HitTestInvisible);
            PortraitVignette[S]=GradientImage(WidgetTree,Gradients[5],FLinearColor::White);Place(Story,PortraitVignette[S],ImageX[S][0],.2f,ImageX[S][1],.582f);
        }
        DialogueLayer=Layer(WidgetTree,Story);
        auto* DialogueFill=WidgetTree->ConstructWidget<UBorder>();DialogueFill->SetBrush(FSlateRoundedBoxBrush(Srgb(.022f,.02f,.03f,.9f),5.f));
        Place(DialogueLayer,DialogueFill,.058f,.688f,.942f,.948f);
        NameFill=WidgetTree->ConstructWidget<UBorder>();NameFill->SetBrush(FSlateRoundedBoxBrush(Srgb(.12f,.09f,.05f,.92f),3.f));
        Place(DialogueLayer,NameFill,.072f,.607f,.243f,.649f);
        auto* DialogueArt=WidgetTree->ConstructWidget<UImage>();DialogueArt->SetBrushFromTexture(MemoriaNarrativeArtwork::Load(TEXT("res://assets/cg/generated/ui_vn_memory_frame_overlay.png")),false);
        Place(DialogueLayer,DialogueArt,0,0,1,1);
        StoryName=Text(WidgetTree,22,NameGold,EFont::Title);StoryName->SetJustification(ETextJustify::Center);StoryName->SetAutoWrapText(false);Shadow(StoryName,.8f);
        Place(DialogueLayer,StoryName,.075f,.609f,.24f,.652f);
        // Rich text keeps the untyped remainder laid out but invisible, so words never reflow while typing.
        TextStyles=NewObject<UDataTable>(this);TextStyles->RowStruct=FRichTextStyleRow::StaticStruct();
        auto AddStyle=[&](const TCHAR* Name,const FLinearColor& Color)
        {
            FRichTextStyleRow Row;Row.TextStyle.SetFont(MemoriaFonts::Get(EFont::Body,25)).SetColorAndOpacity(FSlateColor(Color));
            Row.TextStyle.SetShadowOffset(FVector2D(1.f,1.f)).SetShadowColorAndOpacity(FLinearColor(0,0,0,Color.A>0.f?.9f:0.f));
            TextStyles->AddRow(Name,Row);
        };
        AddStyle(TEXT("Default"),Paper);AddStyle(TEXT("Hidden"),FLinearColor(0,0,0,0));AddStyle(TEXT("Distorted"),Distorted);
        StoryText=WidgetTree->ConstructWidget<URichTextBlock>();StoryText->SetTextStyleSet(TextStyles);StoryText->SetAutoWrapText(true);
        Place(DialogueLayer,StoryText,.085f,.712f,.915f,.93f);
        StoryNext=Text(WidgetTree,14,Srgb(.94f,.78f,.48f,.94f),EFont::Ui);StoryNext->SetText(FText::FromString(TEXT("NEXT  ▼")));StoryNext->SetJustification(ETextJustify::Right);
        Place(DialogueLayer,StoryNext,.8f,.905f,.925f,.945f);
        StoryLocation=Text(WidgetTree,15,Srgb(.86f,.72f,.46f),EFont::Title);StoryLocation->SetAutoWrapText(false);
        { auto Font=MemoriaFonts::Get(EFont::Title,15);Font.LetterSpacing=140;StoryLocation->SetFont(Font); }
        Place(Story,StoryLocation,.045f,.024f,.8f,.075f);
        StoryHint=Text(WidgetTree,12,Srgb(.55f,.53f,.5f),EFont::Ui);StoryHint->SetJustification(ETextJustify::Right);StoryHint->SetAutoWrapText(false);
        Place(Story,StoryHint,.45f,.962f,.955f,.998f);
        ChoiceLayer=WidgetTree->ConstructWidget<UCanvasPanel>();Place(Story,ChoiceLayer,0,0,1,1);
        Place(ChoiceLayer,Fill(WidgetTree,FLinearColor(0,0,0,.4f)),0,0,1,1);
        ChoiceTitleText=Text(WidgetTree,26,Srgb(.96f,.78f,.45f,.95f),EFont::Title);ChoiceTitleText->SetJustification(ETextJustify::Center);Shadow(ChoiceTitleText,.9f);
        Place(ChoiceLayer,ChoiceTitleText,.1f,.095f,.9f,.15f);
        ChoiceHintText=Text(WidgetTree,16,Srgb(.8f,.76f,.7f),EFont::Body);ChoiceHintText->SetJustification(ETextJustify::Center);Shadow(ChoiceHintText,.8f);
        Place(ChoiceLayer,ChoiceHintText,.12f,.15f,.88f,.2f);
        ChoiceFrameArt=WidgetTree->ConstructWidget<UImage>();ChoiceFrameArt->SetBrushFromTexture(MemoriaNarrativeArtwork::Load(TEXT("res://assets/cg/generated/ui_vn_choice_archive_overlay.png")),false);
        ChoiceFrameArt->SetVisibility(ESlateVisibility::HitTestInvisible);Place(ChoiceLayer,ChoiceFrameArt,FrameL,FrameT,FrameR,FrameB);
        ChoiceSlots=WidgetTree->ConstructWidget<UCanvasPanel>();Place(ChoiceLayer,ChoiceSlots,0,0,1,1);
        // Development, status and pause panel.
        NarrativePanel=WidgetTree->ConstructWidget<UBorder>();NarrativePanel->SetBrush(FSlateRoundedBoxBrush(Ink,3.f,Gold,1.f));
        Place(Canvas,NarrativePanel,.055f,.66f,.945f,.96f);
        auto* Interior=WidgetTree->ConstructWidget<UCanvasPanel>();NarrativePanel->SetContent(Interior);NarrativePanel->SetPadding(FMargin(0));
        Speaker=Text(WidgetTree,18,Gold,EFont::Title);Place(Interior,Speaker,.045f,.085f,.94f,.23f);
        BodyScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,BodyScroll,.045f,.28f,.95f,.78f);
        Message=Text(WidgetTree,21,Paper);BodyScroll->AddChild(Message);
        Hint=Text(WidgetTree,12,FLinearColor(.42f,.43f,.44f),EFont::Ui);Place(Interior,Hint,.045f,.84f,.95f,.98f);
        ChoiceNote=Text(WidgetTree,15,FLinearColor(.62f,.6f,.56f));Place(Interior,ChoiceNote,.045f,.2f,.95f,.3f);
        ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>();Place(Interior,ChoiceScroll,.04f,.26f,.96f,.83f);
        Choices=WidgetTree->ConstructWidget<UVerticalBox>();ChoiceScroll->AddChild(Choices);
        PortraitFrame=WidgetTree->ConstructWidget<UBorder>();PortraitFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Black,1.f,Gold,1.f));PortraitFrame->SetPadding(FMargin(2));
        Place(Interior,PortraitFrame,.04f,.28f,.155f,.89f);
        auto* PortraitFit=WidgetTree->ConstructWidget<UScaleBox>();PortraitFit->SetStretch(EStretch::ScaleToFit);PortraitFrame->SetContent(PortraitFit);
        Portrait=WidgetTree->ConstructWidget<UImage>();PortraitFit->AddChild(Portrait);
        PortraitFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
        Location=Text(WidgetTree,13,Gold,EFont::Title);Place(Canvas,Location,.058f,.045f,.9f,.10f);
        // vn_scene.gd _glitch_overlay: impact and burn flashes over the scene.
        Flash=Fill(WidgetTree,FLinearColor::Transparent);Place(Canvas,Flash,0,0,1,1);Flash->SetVisibility(ESlateVisibility::Collapsed);
        ShopWidget=WidgetTree->ConstructWidget<UMemoriaShopWidget>();Place(Canvas,ShopWidget,0,0,1,1);
        // SceneFlow._show_chapter_ledger: a top-centre panel over the scene.
        Ledger=WidgetTree->ConstructWidget<USizeBox>();Ledger->SetMinDesiredWidth(560.f);Canvas->AddChildToCanvas(Ledger);
        if(auto* LedgerSlot=Cast<UCanvasPanelSlot>(Ledger->Slot)){LedgerSlot->SetAnchors(FAnchors(.5f,.1f));LedgerSlot->SetAlignment(FVector2D(.5f,0.f));LedgerSlot->SetAutoSize(true);}
        auto* LedgerFrame=WidgetTree->ConstructWidget<UBorder>();LedgerFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.035f,.03f,.045f,.92f),4.f,FLinearColor(.62f,.5f,.3f,.7f),1.f));
        LedgerFrame->SetPadding(FMargin(22,16));Ledger->SetContent(LedgerFrame);
        auto* LedgerRows=WidgetTree->ConstructWidget<UVerticalBox>();LedgerFrame->SetContent(LedgerRows);
        LedgerTitle=Text(WidgetTree,20,FLinearColor(.9f,.78f,.5f),EFont::Title);LedgerBody=Text(WidgetTree,16,FLinearColor(.82f,.78f,.72f));LedgerThread=Text(WidgetTree,16,FLinearColor(.45f,.85f,.8f));
        for(auto* Row:{LedgerTitle.Get(),LedgerBody.Get(),LedgerThread.Get()}){Row->SetJustification(ETextJustify::Center);LedgerRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,2));}
        Ledger->SetVisibility(ESlateVisibility::Collapsed);
    }
    Refresh(); return Super::RebuildWidget();
}
bool UMemoriaDevelopmentNarrativeWidget::IsStoryMode() const
{ return !View.bShopPresentation && !View.bCompactStatus && !View.bDevelopmentStop; }
FString UMemoriaDevelopmentNarrativeWidget::StoryBody() const
{
    FString Body=View.Narration;
    if(!View.Body.IsEmpty()){if(!Body.IsEmpty())Body+=TEXT("\n\n");Body+=View.Body;}
    return Body;
}
bool UMemoriaDevelopmentNarrativeWidget::IsTyping() const
{
    const bool Enabled=TypewriterOverride.Get(!GIsAutomationTesting);
    return Enabled && IsStoryMode() && !View.bPaused && View.Choices.IsEmpty() && Typed<ShownBody.Len();
}
void UMemoriaDevelopmentNarrativeWidget::Display(const FMemoriaNarrativeView& InView)
{
    const int32 Previous=SelectedOriginalIndex();const bool SameStep=View.Header==InView.Header;
    const bool First=!bSeenView;bSeenView=true;
    View=InView;Selection=0;
    if(SameStep)for(int32 I=0;I<View.Choices.Num();++I)if(View.Choices[I].OriginalIndex==Previous)Selection=I;
    const bool NewCue=!View.CueKey.IsEmpty() && View.CueKey!=ActiveCue;
    Refresh();if(!SameStep && BodyScroll)BodyScroll->ScrollToStart();
    if(NewCue)StartCues();
    // A ledger or burn raised before this widget existed is not replayed.
    if(First){ShownLedger=View.LedgerSerial;ShownBurn=View.BurnSerial;}
    else
    {
        if(View.BurnSerial!=ShownBurn){ShownBurn=View.BurnSerial;BurnAge=0.f;}
        if(View.LedgerSerial!=ShownLedger && Ledger)
        {
            ShownLedger=View.LedgerSerial;LedgerAge=0.f;
            LedgerTitle->SetText(FText::FromString(View.LedgerTitle));
            TArray<FString> Rows=View.LedgerLines;const FString Thread=Rows.Num()?Rows.Pop():FString();
            LedgerBody->SetText(FText::FromString(FString::Join(Rows,TEXT("\n"))));LedgerThread->SetText(FText::FromString(Thread));
            LedgerThread->SetColorAndOpacity(View.bLedgerThreadHolds?FLinearColor(.45f,.85f,.8f):FLinearColor(.85f,.55f,.4f));
        }
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
        Motion=View.CgMotion.IsEmpty()?TEXT("ambient"):View.CgMotion;MotionAge=-CrossFade;
        const uint32 Hash=GetTypeHash(ActiveCue);Pan=FVector2D(float(Hash%37)-18.f,float((Hash/37)%21)-10.f);
    }
    FlashDuration=0.f;NudgeStrength=0.f;
    if(View.Impact==TEXT("void")){FlashColor=Srgb(.46f,.22f,.78f,.28f);FlashDuration=.65f;}
    else if(View.Impact==TEXT("memory")){FlashColor=Srgb(1.f,.72f,.32f,.52f);FlashDuration=.9f;}
    else if(View.Impact==TEXT("heavy")){FlashColor=Srgb(.92f,.94f,1.f,.38f);FlashDuration=.48f;}
    if(FlashDuration>0.f)NudgeStrength=View.Impact==TEXT("heavy")?4.f:2.5f;
    // Portrait slots (_on_step_changed): a line over a full-scene story CG clears the stage
    // (_should_hide_portraits_for_cg_line); Arrel and Elia hold the stage alone
    // (_uses_single_portrait_composition); narration dims both sides.
    const FString Cg=View.BackdropSource.ToLower(),Who=View.Speaker.ToLower();
    const bool Single=View.Speaker==TEXT("Arrel") || View.Speaker==TEXT("Elia");
    const bool HideForCg=View.bStepHasCg && (Cg.Contains(TEXT("/generated/story_")) || Cg.Contains(TEXT("/generated/dialogue_")) ||
        (Single && (Cg.Contains(Who) || Cg.Contains(TEXT("arrel_elia")) || Cg.Contains(TEXT("duo")))));
    if(HideForCg){SlotTexture[0]=SlotTexture[1]=nullptr;ActiveSlot=INDEX_NONE;}
    else if(!View.Speaker.IsEmpty() && !View.PortraitSource.IsEmpty() && !View.bSystemLog)
    {
        const int32 Side=View.PortraitSide==TEXT("right")?1:0;
        if(auto* Face=MemoriaNarrativeArtwork::Load(View.PortraitSource))
        {
            if(SlotTexture[Side]!=Face)
            {
                SlotTexture[Side]=Face;SlotAge[Side]=0.f;PortraitImage[Side]->SetBrushFromTexture(Face,true);
                const FLinearColor Accent=PortraitAccent(View.PortraitSource);
                PortraitPlate[Side]->SetBrush(FSlateRoundedBoxBrush(Srgb(.018f,.014f,.022f,.3f),6.f,FLinearColor(Accent.R,Accent.G,Accent.B,.72f),1.5f));
                // Light parchment face art takes the inner vignette; dark character shots do not need it.
                PortraitVignette[Side]->SetRenderOpacity(Face->GetSizeX()<400?1.f:0.f);
            }
            if(Single)SlotTexture[1-Side]=nullptr;
            ActiveSlot=Side;
        }
    }
    else if(View.Speaker.IsEmpty())ActiveSlot=INDEX_NONE;
}
void UMemoriaDevelopmentNarrativeWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
    Super::NativeTick(MyGeometry,InDeltaTime);
    // Cues run on game time like the source tweens: they hold while the world is paused.
    const UWorld* World=GetWorld();AdvancePresentation(World?World->GetDeltaSeconds():InDeltaTime);
}
void UMemoriaDevelopmentNarrativeWidget::AdvancePresentation(float DeltaSeconds)
{
    const float Dt=FMath::Max(0.f,DeltaSeconds);CueAge+=Dt;MotionAge+=Dt;Clock+=Dt;
    if(LedgerAge>=0.f)LedgerAge+=Dt;
    if(BurnAge>=0.f)BurnAge+=Dt;
    SlotAge[0]+=Dt;SlotAge[1]+=Dt;
    // TYPEWRITER_SPEED: one character every 0.025 s.
    if(IsTyping())Typed=FMath::Min<float>(ShownBody.Len(),Typed+Dt/.025f);
    ApplyCues();
}
FMemoriaPresentationProbe UMemoriaDevelopmentNarrativeWidget::GetPresentationProbe() const { return Probe; }
void UMemoriaDevelopmentNarrativeWidget::ApplyStoryText()
{
    if(!StoryText)return;
    const bool Enabled=TypewriterOverride.Get(!GIsAutomationTesting);
    const int32 Count=Enabled?FMath::Clamp(FMath::FloorToInt(Typed),0,ShownBody.Len()):ShownBody.Len();
    FString Shown=ShownBody.Left(Count);const FString Rest=ShownBody.Mid(Count);
    // _play_subtle_distortion pre-glitch: the line reads scrambled for its first 0.12 s.
    if(View.bDistorted && CueAge<.12f)Shown=Scramble(Shown);
    FString Markup=Shown.IsEmpty()?FString():(View.bDistorted?TEXT("<Distorted>")+Escape(Shown)+TEXT("</>"):Escape(Shown));
    if(!Rest.IsEmpty())Markup+=TEXT("<Hidden>")+Escape(Rest)+TEXT("</>");
    if(StoryText->GetText().ToString()!=Markup)StoryText->SetText(FText::FromString(Markup));
    Probe.TypedChars=Count;Probe.TotalChars=ShownBody.Len();Probe.bTyping=Count<ShownBody.Len();
}
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
    // The flash overlay decays on an exponential ease-out; a memory burn flashes red (_play_burn_glitch).
    float Alpha=FlashDuration>0.f && N>=0.f && N<FlashDuration?FlashColor.A*FMath::Pow(2.f,-10.f*N/FlashDuration):0.f;
    FLinearColor FlashNow=FlashColor;
    const float BurnFlash=BurnAge>=0.f && BurnAge<.9f?.55f*FMath::Pow(2.f,-10.f*BurnAge/.9f):0.f;
    if(BurnFlash>Alpha){Alpha=BurnFlash;FlashNow=Srgb(.95f,.25f,.15f);}
    Flash->SetBrushColor(FLinearColor(FlashNow.R,FlashNow.G,FlashNow.B,Alpha));
    Flash->SetVisibility(Alpha>.001f?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    // Chroma split: a burn tears the CG apart (8 px, 0.7 s); a distorted line holds a faint 3 px split for 1.2 s.
    float Chroma=0.f;FVector2D Split=FVector2D::ZeroVector;
    if(BurnAge>=0.f && BurnAge<.7f){const float E=1.f-FMath::Pow(2.f,-10.f*BurnAge/.7f);Chroma=.55f*(1.f-BurnAge/.7f);Split=FVector2D(8.f,2.f)*(1.f-E);}
    else if(View.bDistorted && IsStoryMode() && CueAge<1.6f){const float K=CueAge<1.2f?1.f:1.f-(CueAge-1.2f)/.4f;Chroma=.25f*K;Split=FVector2D(3.f,0.f)*K;}
    UTexture2D* Current=Cast<UTexture2D>(Backdrop->GetBrush().GetResourceObject());
    const bool ShowChroma=Chroma>.001f && Current;
    for(UImage* Copy:{ChromaRed.Get(),ChromaBlue.Get()})
    {
        if(ShowChroma && Copy->GetBrush().GetResourceObject()!=Current)Copy->SetBrushFromTexture(Current,true);
        Copy->SetVisibility(ShowChroma?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    }
    ChromaRed->SetColorAndOpacity(FLinearColor(1,0,0,Chroma));ChromaBlue->SetColorAndOpacity(FLinearColor(0,.4f,1,Chroma));
    ChromaRedFit->SetRenderTransform(FWidgetTransform(Offset+Nudge-Split,FVector2D(Scale,Scale),FVector2D::ZeroVector,0.f));
    ChromaBlueFit->SetRenderTransform(FWidgetTransform(Offset+Nudge+Split,FVector2D(Scale,Scale),FVector2D::ZeroVector,0.f));
    // Ember vignette: rises in 0.5 s, holds 1.2 s, cools over 3.5 s.
    float EmberAlpha=0.f;
    if(BurnAge>=0.f)
    {
        // The source peak (0.85) floods a cover-filled frame edge to edge; the same curve peaks at 0.6 here.
        EmberAlpha=BurnAge<.5f?.6f*FMath::Square(BurnAge/.5f):BurnAge<1.7f?.6f:BurnAge<5.2f?.6f*(.5f+.5f*FMath::Cos(PI*(BurnAge-1.7f)/3.5f)):0.f;
        if(BurnAge>=5.2f)BurnAge=-1.f;
    }
    Ember->SetColorAndOpacity(FLinearColor(1,1,1,EmberAlpha));
    // Portraits: the speaker bright, the other side dimmed; a new face fades up from 12 px lower.
    const bool bStory=IsStoryMode(),Choosing=!View.Choices.IsEmpty() && !View.bPaused;
    for(int32 S=0;S<2;++S)
    {
        const bool Show=bStory && !Choosing && !View.bPaused && SlotTexture[S];
        const bool Active=ActiveSlot==S;const float In=FMath::Clamp(SlotAge[S]/.25f,0.f,1.f);
        const ESlateVisibility Visible=Show?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed;
        PortraitImage[S]->GetParent()->SetVisibility(Visible);PortraitPlate[S]->SetVisibility(Visible);PortraitShadow[S]->SetVisibility(Visible);PortraitVignette[S]->SetVisibility(Visible);
        PortraitImage[S]->SetColorAndOpacity(Active?FLinearColor::White:Dim);
        PortraitImage[S]->GetParent()->SetRenderOpacity(In);PortraitImage[S]->GetParent()->SetRenderTranslation(FVector2D(0.f,12.f*(1.f-In)));
        PortraitVignette[S]->SetRenderTranslation(FVector2D(0.f,12.f*(1.f-In)));PortraitVignette[S]->SetColorAndOpacity(FLinearColor(1,1,1,In));
        PortraitPlate[S]->SetRenderOpacity((Active?.92f:.38f)*In);PortraitShadow[S]->SetColorAndOpacity(FLinearColor(1,1,1,(Active?.42f/.52f:.18f/.52f)*In));
    }
    // Ledger: 0.4 s in, 4.6 s hold, 0.6 s out.
    float LedgerAlpha=0.f;
    if(LedgerAge>=0.f){LedgerAlpha=LedgerAge<.4f?LedgerAge/.4f:LedgerAge<5.f?1.f:FMath::Max(0.f,1.f-(LedgerAge-5.f)/.6f);if(LedgerAge>=5.6f)LedgerAge=-1.f;}
    Ledger->SetRenderOpacity(LedgerAlpha);Ledger->SetVisibility(LedgerAlpha>0.f?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(bStory)
    {
        ApplyStoryText();
        const bool Next=!Choosing && !View.bPaused && !IsTyping();
        StoryNext->SetVisibility(Next?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
        StoryNext->SetRenderOpacity(.55f+.45f*FMath::Sin(Clock*4.f));
    }
    Probe.FlashAlpha=Alpha;Probe.BackdropScale=Scale;Probe.BackdropOpacity=FadeT;Probe.LedgerAlpha=LedgerAlpha;Probe.Nudge=Nudge;Probe.Motion=Motion;
    Probe.ChromaAlpha=ShowChroma?Chroma:0.f;Probe.EmberAlpha=EmberAlpha;
    Probe.LedgerText=LedgerAlpha>0.f?LedgerTitle->GetText().ToString()+TEXT("\n")+LedgerBody->GetText().ToString()+TEXT("\n")+LedgerThread->GetText().ToString():FString();
    Probe.bDistortedStyle=View.bDistorted;Probe.bSystemStyle=View.bSystemLog;Probe.bStoryFrame=bStory && !Choosing && !View.bPaused;
    Probe.bChoiceFrame=bStory && Choosing && ChoiceFrameArt->GetVisibility()!=ESlateVisibility::Collapsed;
    Probe.ActiveSide=ActiveSlot==0?TEXT("left"):ActiveSlot==1?TEXT("right"):TEXT("");
}
void UMemoriaDevelopmentNarrativeWidget::Navigate(int32 Direction)
{
    if(View.bShopPresentation && ShopWidget){ShopWidget->Navigate(Direction);return;}
    if(!View.bPaused && !View.Choices.IsEmpty())
    {
        Selection=(Selection+Direction+View.Choices.Num())%View.Choices.Num();StyleChoices();
        if(!IsStoryMode() && Buttons.IsValidIndex(Selection))ChoiceScroll->ScrollWidgetIntoView(Buttons[Selection],false);
    }
}
void UMemoriaDevelopmentNarrativeWidget::HoverChoice(int32 Index)
{ if(!View.bPaused && View.Choices.IsValidIndex(Index) && Index!=Selection){Selection=Index;StyleChoices();} }
int32 UMemoriaDevelopmentNarrativeWidget::SelectedOriginalIndex() const
{ return View.Choices.IsValidIndex(Selection)?View.Choices[Selection].OriginalIndex:INDEX_NONE; }
void UMemoriaDevelopmentNarrativeWidget::Choose(int32 Index)
{ if(!View.bPaused && View.Choices.IsValidIndex(Index)){Selection=Index;ConfirmIntent();} }
void UMemoriaDevelopmentNarrativeWidget::ConfirmIntent()
{
    if(View.bShopPresentation && ShopWidget){ShopWidget->ConfirmIntent();return;}
    // vn_scene.gd: input while a line types completes it; the next input advances.
    if(IsTyping()){Typed=ShownBody.Len();ApplyCues();return;}
    OnConfirm.ExecuteIfBound(SelectedOriginalIndex());
}
FString UMemoriaDevelopmentNarrativeWidget::VisibleText() const
{
    if(View.bShopPresentation && ShopWidget)return ShopWidget->VisibleText();
    if(!Message)return FString();
    if(IsStoryMode() && !View.bPaused)
    {
        // The complete line is reported; the typewriter is presentation only.
        FString Out=StoryLocation->GetText().ToString()+TEXT("\n")+StoryName->GetText().ToString();
        if(DialogueLayer->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+ShownBody;
        if(ChoiceLayer->GetVisibility()!=ESlateVisibility::Collapsed)
        {
            Out+=TEXT("\n")+ChoiceTitleText->GetText().ToString()+TEXT("\n")+ChoiceHintText->GetText().ToString();
            for(const auto& C:View.Choices){Out+=TEXT("\n")+C.Text;if(!C.Effect.IsEmpty())Out+=TEXT("\n")+C.Effect;}
        }
        return Out+TEXT("\n")+StoryHint->GetText().ToString();
    }
    FString Out=Location->GetText().ToString()+TEXT("\n")+Speaker->GetText().ToString();
    if(BodyScroll->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+Message->GetText().ToString();
    if(ChoiceNote->GetVisibility()!=ESlateVisibility::Collapsed)Out+=TEXT("\n")+ChoiceNote->GetText().ToString();
    if(ChoiceScroll->GetVisibility()!=ESlateVisibility::Collapsed)for(const auto& C:View.Choices){Out+=TEXT("\n")+C.Text;if(!C.Effect.IsEmpty())Out+=TEXT("\n")+C.Effect;}
    return Out+TEXT("\n")+Hint->GetText().ToString();
}
UTexture2D* UMemoriaDevelopmentNarrativeWidget::DisplayedBackdrop() const
{ return Backdrop && Illustration->GetVisibility()!=ESlateVisibility::Collapsed ? Cast<UTexture2D>(Backdrop->GetBrush().GetResourceObject()):nullptr; }
UTexture2D* UMemoriaDevelopmentNarrativeWidget::DisplayedPortrait() const
{
    if(!Portrait)return nullptr;
    if(IsStoryMode())
    {
        const bool Choosing=!View.Choices.IsEmpty() && !View.bPaused;
        return ActiveSlot!=INDEX_NONE && !Choosing && !View.bPaused && SlotTexture[ActiveSlot] ? SlotTexture[ActiveSlot].Get() : nullptr;
    }
    return PortraitFrame->GetVisibility()!=ESlateVisibility::Collapsed ? Cast<UTexture2D>(Portrait->GetBrush().GetResourceObject()):nullptr;
}
void UMemoriaDevelopmentNarrativeWidget::StyleChoices()
{
    const bool bStory=IsStoryMode(),Framed=bStory && View.Choices.Num()==3;
    for(int32 I=0;I<Buttons.Num();++I)Buttons[I]->SetStyle(bStory?StoryChoiceStyle(I==Selection,Framed):ChoiceStyle(I==Selection));
}
void UMemoriaDevelopmentNarrativeWidget::BuildChoices(bool bFramed)
{
    ChoiceSlots->ClearChildren();Buttons.Reset();
    const int32 Count=View.Choices.Num();
    // Unframed stacks share the frame's footprint; heights shrink for long lists.
    const float Height=FMath::Min(.13f,.54f/FMath::Max(1,Count)),Gap=.018f,Top=.3f+FMath::Max(0.f,(.56f-Count*(Height+Gap))*.5f);
    for(int32 I=0;I<Count;++I)
    {
        auto* B=WidgetTree->ConstructWidget<UMemoriaNarrativeChoiceButton>();B->Owner=this;B->ChoiceIndex=I;
        B->OnClicked.AddDynamic(B,&UMemoriaNarrativeChoiceButton::Choose);B->OnHovered.AddDynamic(B,&UMemoriaNarrativeChoiceButton::Hover);
        auto* Lines=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Label=Text(WidgetTree,21,Paper);Label->SetText(FText::FromString(View.Choices[I].Text));Label->SetJustification(ETextJustify::Center);Shadow(Label,.8f);
        Lines->AddChildToVerticalBox(Label)->SetHorizontalAlignment(HAlign_Fill);
        if(!View.Choices[I].Effect.IsEmpty())
        {
            auto* Effect=Text(WidgetTree,15,FLinearColor(.86f,.7f,.44f));Effect->SetText(FText::FromString(View.Choices[I].Effect));Effect->SetJustification(ETextJustify::Center);Shadow(Effect,.8f);
            auto* EffectSlot=Lines->AddChildToVerticalBox(Effect);EffectSlot->SetPadding(FMargin(0,5,0,0));EffectSlot->SetHorizontalAlignment(HAlign_Fill);
        }
        auto* Pad=WidgetTree->ConstructWidget<UBorder>();Pad->SetBrushColor(FLinearColor::Transparent);Pad->SetPadding(FMargin(28,8));Pad->SetContent(Lines);
        B->SetContent(Pad);
        Cast<UButtonSlot>(Pad->Slot)->SetHorizontalAlignment(HAlign_Fill);Cast<UButtonSlot>(Pad->Slot)->SetVerticalAlignment(VAlign_Center);
        if(bFramed)
            Place(ChoiceSlots,B,FrameL+(FrameR-FrameL)*.095f,FrameT+(FrameB-FrameT)*SlotV[I][0],FrameL+(FrameR-FrameL)*.905f,FrameT+(FrameB-FrameT)*SlotV[I][1]);
        else Place(ChoiceSlots,B,.18f,Top+I*(Height+Gap),.82f,Top+I*(Height+Gap)+Height);
        Buttons.Add(B);
    }
    StyleChoices();
}
void UMemoriaDevelopmentNarrativeWidget::RefreshStory(bool bChoosing)
{
    DialogueLayer->SetVisibility(!bChoosing && !View.bPaused?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    ChoiceLayer->SetVisibility(bChoosing?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
    StoryLocation->SetText(FText::FromString(View.LocationTitle));
    StoryName->SetText(FText::FromString(View.Speaker));StoryName->SetColorAndOpacity(View.bSystemLog?SystemCyan:NameGold);
    NameFill->SetVisibility(View.Speaker.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    const FString Body=StoryBody();
    if(Body!=ShownBody){ShownBody=Body;Typed=0.f;}
    StoryHint->SetText(FText::FromString(View.bPaused?TEXT(""):bChoosing?TEXT("↑ ↓  Select      ENTER  Confirm"):TEXT("ENTER  Next      ESC  Pause      TAB  Memories")));
    if(bChoosing)
    {
        const bool Framed=View.Choices.Num()==3;
        ChoiceFrameArt->SetVisibility(Framed?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        ChoiceTitleText->SetText(FText::FromString(View.ChoiceTitle.IsEmpty()?TEXT("YOUR CHOICE"):View.ChoiceTitle));
        ChoiceHintText->SetText(FText::FromString(View.ChoiceHint));
        BuildChoices(Framed);
    }
    else {ChoiceSlots->ClearChildren();Buttons.Reset();}
}
void UMemoriaDevelopmentNarrativeWidget::Refresh()
{
    if(!Message)return;
    const bool bStory=IsStoryMode();
    const bool Choosing=!View.Choices.IsEmpty() && !View.bPaused;
    ShopWidget->SetVisibility(View.bShopPresentation?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Illustration->SetVisibility(View.bShopPresentation || View.bCompactStatus || View.bDevelopmentStop?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    Cinema->SetVisibility(bStory?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    Story->SetVisibility(bStory?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
    // The development panel serves status, development screens and the pause card.
    NarrativePanel->SetVisibility(!View.bShopPresentation && (!bStory || View.bPaused)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Location->SetVisibility(View.bShopPresentation || View.bCompactStatus || bStory?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
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
    if(bStory)
    {
        RefreshStory(Choosing);PortraitFrame->SetVisibility(ESlateVisibility::Collapsed);
        if(View.bPaused)
        {
            Position(NarrativePanel,.3f,.38f,.7f,.62f);
            Speaker->SetText(FText::FromString(TEXT("PAUSED")));Message->SetText(FText::FromString(TEXT("Enter / A or Back: return to the current line")));
            BodyScroll->SetVisibility(ESlateVisibility::Visible);ChoiceScroll->SetVisibility(ESlateVisibility::Collapsed);ChoiceNote->SetVisibility(ESlateVisibility::Collapsed);
            Position(BodyScroll,.08f,.34f,.92f,.8f);Hint->SetText(FText::GetEmpty());Choices->ClearChildren();
        }
        return;
    }
    auto* Face=View.Speaker.IsEmpty() || View.bPaused || View.bCompactStatus || View.bDevelopmentStop || !View.Choices.IsEmpty()?nullptr:MemoriaNarrativeArtwork::Load(View.PortraitSource);
    Portrait->SetBrushFromTexture(Face,true);PortraitFrame->SetVisibility(Face?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    const bool Right=View.PortraitSide==TEXT("right");
    Position(PortraitFrame,Right?.845f:.04f,.28f,Right?.96f:.155f,.89f);
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
    Speaker->SetColorAndOpacity(View.bSystemLog && !Choosing?SystemCyan:Gold);
    Message->SetColorAndOpacity(View.bDistorted?Distorted:Paper);
    FString Body=View.bPaused?TEXT("Enter / A or Back: return to the current line"):View.Narration;
    if(!View.bPaused && !View.Body.IsEmpty()){if(!Body.IsEmpty())Body+=TEXT("\n\n");Body+=View.Body;}
    // The exploration status is interface text: sans, smaller, so control hints do not wrap.
    Message->SetFont(MemoriaFonts::Get(View.bCompactStatus?EFont::Ui:EFont::Body,View.bCompactStatus?15:21));
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
