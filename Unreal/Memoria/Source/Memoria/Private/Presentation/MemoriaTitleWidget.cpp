#include "Presentation/MemoriaTitleWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Presentation/MemoriaUiKit.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
namespace
{
using MemoriaUiKit::Srgb;
using EFont = MemoriaFonts::EStyle;
const TCHAR* TitleArt = TEXT("res://assets/cg/generated/ui_title_memoria_premium.png");
// main.gd sizes are Godot pixels at 1280x720. Slate lays out in 1080p units (DPI scale 2/3 at 720p),
// so lengths scale by 1.5; font points render about 0.89 px each at 720p.
int32 Pt(float Px) { return FMath::RoundToInt(Px * 1.12f); }
constexpr float U = 1.5f;
// A button's content fills it, left to right, as the source's left-aligned labels do.
void FillContent(UButton* Button)
{ if (auto* S = Cast<UButtonSlot>(Button->GetContentSlot())) { S->SetHorizontalAlignment(HAlign_Fill); S->SetVerticalAlignment(VAlign_Fill); S->SetPadding(FMargin(0)); } }
UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, float L, float T, float R, float B)
{
    auto* Slot = Canvas->AddChildToCanvas(Widget); Slot->SetAnchors(FAnchors(L, T, R, B)); Slot->SetOffsets(FMargin(0)); return Slot;
}
UBorder* Fill(UWidgetTree* Tree, const FLinearColor& Color)
{ auto* B = Tree->ConstructWidget<UBorder>(); B->SetBrushColor(Color); B->SetVisibility(ESlateVisibility::HitTestInvisible); return B; }
UImage* Picture(UWidgetTree* Tree, UTexture2D* Texture)
{ auto* I = Tree->ConstructWidget<UImage>(); I->SetBrushFromTexture(Texture, false); I->SetVisibility(ESlateVisibility::HitTestInvisible); return I; }
UTextBlock* Label(UWidgetTree* Tree, EFont Font, int32 Size, const FLinearColor& Color, float Shadow = .85f)
{
    auto* W = Tree->ConstructWidget<UTextBlock>(); W->SetFont(MemoriaFonts::Get(Font, Size)); W->SetColorAndOpacity(Color);
    W->SetShadowOffset(FVector2D(1.f, 1.f)); W->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, Shadow)); return W;
}
float Sine(float T) { T = FMath::Clamp(T, 0.f, 1.f); return .5f - .5f * FMath::Cos(PI * T); }
float Quint(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - FMath::Pow(1.f - T, 5.f); }
// _style_title_button: normal, hover/focus and disabled boxes; the thick left edge is an accent bar here.
FButtonStyle ItemStyle(bool bSelected, bool bEnabled)
{
    const FLinearColor Bg = !bEnabled ? Srgb(.025f, .025f, .030f, .45f) : bSelected ? Srgb(.15f, .105f, .065f, .88f) : Srgb(.030f, .024f, .040f, .64f);
    const FLinearColor Edge = !bEnabled ? Srgb(.24f, .22f, .20f, .32f) : bSelected ? Srgb(1.f, .76f, .36f, .94f) : Srgb(.48f, .36f, .22f, .48f);
    FButtonStyle Style; const FSlateRoundedBoxBrush Box(Bg, 2.f, Edge, 1.f);
    Style.SetNormal(Box); Style.SetHovered(Box); Style.SetPressed(FSlateRoundedBoxBrush(bSelected ? Srgb(.16f, .12f, .08f, .94f) : Bg, 2.f, Edge, 1.f));
    Style.SetDisabled(Box); Style.SetNormalPadding(FMargin(0)); Style.SetPressedPadding(FMargin(0)); return Style;
}
FLinearColor ItemInk(bool bSelected, bool bEnabled)
{ return !bEnabled ? Srgb(.38f, .36f, .34f, .85f) : bSelected ? Srgb(1.f, .86f, .52f) : Srgb(.84f, .80f, .73f, .98f); }
}
void UMemoriaTitleButton::Activate() { if (Owner) { if (Role == 0) Owner->ActivateItem(Index); else Owner->ActivateOption(Index, Role == 2 ? -1 : Role == 3 ? 1 : 0); } }
void UMemoriaTitleButton::Hover() { if (Owner && Role == 0) Owner->Select(Index); else if (Owner && Role == 1) Owner->SelectOption(Index); }

FString UMemoriaTitleWidget::Loc(const TCHAR* En, const TCHAR* Ko) const
{ return Settings && Settings->GetLocale() == TEXT("en") ? En : Ko; }
void UMemoriaTitleWidget::Configure(bool bInCanContinue, UMemoriaSettingsSubsystem* InSettings)
{
    bCanContinue = bInCanContinue; Settings = InSettings;
    // main.gd: Continue is disabled without a save; focus starts on New Game.
    Selected = 0; Refresh();
}
TSharedRef<SWidget> UMemoriaTitleWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Root;
        Place(Root, Fill(WidgetTree, FLinearColor::Black), 0, 0, 1, 1);
        // _build_background: the key art, cover-fit with an 18/14 px overscan for the breath and parallax.
        BackdropFit = WidgetTree->ConstructWidget<UScaleBox>(); BackdropFit->SetStretch(EStretch::ScaleToFill);
        Place(Root, BackdropFit, 0, 0, 1, 1)->SetOffsets(FMargin(-18, -14, -18, -14));
        BackdropFit->SetVisibility(ESlateVisibility::HitTestInvisible);
        Backdrop = Picture(WidgetTree, MemoriaNarrativeArtwork::Load(TitleArt)); BackdropFit->AddChild(Backdrop);
        // MemoryRiftGlow: a radial warm-to-violet bloom over the rift (additive in the source).
        RiftGlow = Picture(WidgetTree, MemoriaUiKit::Gradient(256, 256, {.5, .5}, {.98, .5}, true,
            {{0.f, {1.f, .77f, .34f, .18f}}, {.24f, {.66f, .38f, .85f, .12f}}, {.58f, {.19f, .09f, .34f, .045f}}, {1.f, {0, 0, 0, 0}}}));
        Place(Root, RiftGlow, .315f, -.06f, .535f, 1.04f);
        // TitleAsh: 30 soft motes of the 512 px ash texture drifting up and left.
        UTexture2D* Mote = MemoriaUiKit::Gradient(128, 128, {.5, .5}, {.98, .5}, true,
            {{0.f, {1.f, .91f, .70f, .9f}}, {.34f, {.74f, .58f, .42f, .62f}}, {1.f, {.15f, .10f, .18f, 0.f}}});
        auto* AshLayer = WidgetTree->ConstructWidget<UCanvasPanel>(); Place(Root, AshLayer, 0, 0, 1, 1); AshLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
        for (int32 I = 0; I < 30; ++I)
        {
            auto* Dot = Picture(WidgetTree, Mote); Dot->SetColorAndOpacity(Srgb(.78f, .69f, .62f, .34f));
            AshLayer->AddChildToCanvas(Dot)->SetAnchors(FAnchors(0, 0)); Ash.Add(Dot);
            FMote M; M.Life = 9.f; M.Age = Rng.FRandRange(0.f, 9.f); M.Size = 512.f * Rng.FRandRange(.45f, 1.1f);
            M.P = FVector2D(Rng.FRandRange(-40.f, 1320.f), Rng.FRandRange(-40.f, 760.f));
            const float Angle = FMath::DegreesToRadians(Rng.FRandRange(-26.f, 26.f)); const FVector2D Dir = FVector2D(-.18f, -1.f).GetSafeNormal().GetRotated(FMath::RadiansToDegrees(Angle));
            M.V = Dir * Rng.FRandRange(4.f, 12.f); Motes.Add(M);
        }
        // CinematicVignette: the source canvas_item shader, baked once.
        Place(Root, Picture(WidgetTree, MemoriaUiKit::Paint(320, 180, [](const FVector2D& UV)
        {
            const FVector2D C = UV * 2.0 - FVector2D(1, 1);
            const float Oval = FVector2D(C.X * .76, C.Y).Size();
            const float Edge = FMath::SmoothStep(.34f, 1.12f, Oval), Bars = FMath::Pow(FMath::Abs(float(C.Y)), 3.2f) * .22f;
            const float LeftShelter = (1.f - FMath::SmoothStep(0.f, .42f, float(UV.X))) * .16f, RightShelter = FMath::SmoothStep(.58f, 1.f, float(UV.X)) * .24f;
            const float Rift = FMath::Exp(-42.f * FMath::Square(float(UV.X) - .425f) - 5.f * FMath::Square(float(UV.Y) - .5f));
            return FLinearColor(.006f, .005f, .014f, FMath::Clamp(.10f + Edge * .58f + Bars + LeftShelter + RightShelter - Rift * .08f, 0.f, .84f));
        })), 0, 0, 1, 1);
        // _build_title_copy: gold rail and the wordmark stack.
        TitleRail = Picture(WidgetTree, MemoriaUiKit::Gradient(8, 256, {.5, 0}, {.5, 1}, false,
            {{0.f, {.84f, .59f, .23f, 0.f}}, {.18f, {.95f, .73f, .35f, .92f}}, {.72f, {.58f, .36f, .16f, .58f}}, {1.f, {.20f, .11f, .06f, 0.f}}}));
        Place(Root, TitleRail, .043f, .105f, .046f, .425f);
        TitleStack = WidgetTree->ConstructWidget<UVerticalBox>(); Place(Root, TitleStack, .055f, .105f, .56f, .435f); TitleStack->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto Line = [&](UWidget* W, float Gap = 5.f) { TitleStack->AddChildToVerticalBox(W)->SetPadding(FMargin(0, 0, 0, Gap * U)); };
        auto* Eyebrow = Label(WidgetTree, EFont::Ui, Pt(13), Srgb(.73f, .67f, .60f, .9f), .9f); Line(Eyebrow); Copy.Add(Eyebrow);
        auto* Wordmark = Label(WidgetTree, EFont::Title, Pt(78), Srgb(.96f, .82f, .52f), .9f);
        { FSlateFontInfo Font = MemoriaFonts::Get(EFont::Title, Pt(78)); Font.OutlineSettings.OutlineSize = 2; Font.OutlineSettings.OutlineColor = Srgb(.11f, .055f, .025f, .92f); Wordmark->SetFont(Font); }
        Wordmark->SetShadowOffset(FVector2D(3, 5)); Line(Wordmark, 0); Copy.Add(Wordmark);
        auto* Subtitle = Label(WidgetTree, EFont::Title, Pt(21), Srgb(.91f, .85f, .75f), .82f); Subtitle->SetShadowOffset(FVector2D(1, 2)); Line(Subtitle, 8); Copy.Add(Subtitle);
        auto* Divider = WidgetTree->ConstructWidget<UHorizontalBox>(); Line(Divider, 8);
        auto Rule = [&](float Width, float Alpha)
        {
            auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(Width * U); Size->SetHeightOverride(U);
            Size->AddChild(Fill(WidgetTree, Srgb(.86f, .64f, .29f, Alpha)));
            auto* S = Divider->AddChildToHorizontalBox(Size); S->SetVerticalAlignment(VAlign_Center);
        };
        Rule(116, .72f);
        auto* Diamond = Label(WidgetTree, EFont::Ui, Pt(10), Srgb(.95f, .76f, .38f, .92f)); Diamond->SetText(FText::FromString(TEXT("◆")));
        Divider->AddChildToHorizontalBox(Diamond)->SetPadding(FMargin(8 * U, 0));
        Rule(176, .42f);
        auto* Tagline = Label(WidgetTree, EFont::Body, Pt(15), Srgb(.74f, .70f, .66f), .7f); Line(Tagline); Copy.Add(Tagline);
        // _build_menu_frame: the archive panel, its kicker, heading, gold rule and key hint.
        auto* PanelShadow = Fill(WidgetTree, FLinearColor(0, 0, 0, .34f)); Place(Root, PanelShadow, .685f, .39f, .97f, .93f)->SetOffsets(FMargin(-6 * U, 8 * U, 6 * U, -8 * U));
        auto* Panel = WidgetTree->ConstructWidget<UBorder>(); Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
        Panel->SetBrush(FSlateRoundedBoxBrush(Srgb(.018f, .014f, .027f, .76f), 2.f, Srgb(.55f, .40f, .22f, .54f), 1.f));
        Place(Root, Panel, .685f, .39f, .97f, .93f);
        auto* PanelEdge = Fill(WidgetTree, Srgb(.55f, .40f, .22f, .54f)); Place(Root, PanelEdge, .685f, .39f, .685f, .93f)->SetOffsets(FMargin(0, 0, 3 * U, 0));
        auto* Kicker = Label(WidgetTree, EFont::Ui, Pt(13), Srgb(.72f, .60f, .44f, .92f)); Place(Root, Kicker, .718f, .42f, .955f, .45f); Kicker->SetText(FText::FromString(TEXT("MEMORY ARCHIVE  //  00")));
        auto* Heading = Label(WidgetTree, EFont::Title, Pt(18), Srgb(.91f, .85f, .76f)); Place(Root, Heading, .718f, .448f, .955f, .485f); Copy.Add(Heading);
        auto* GoldRule = Fill(WidgetTree, Srgb(.78f, .56f, .27f, .48f)); Place(Root, GoldRule, .718f, .482f, .955f, .484f);
        auto* Footer = Label(WidgetTree, EFont::Ui, Pt(13), Srgb(.58f, .55f, .52f, .92f)); Place(Root, Footer, .718f, .886f, .955f, .916f); Copy.Add(Footer);
        MenuChrome = {PanelShadow, Panel, PanelEdge, Heading, Kicker, GoldRule, Footer};
        // _setup_menu: numbered items, vertically centred in the panel.
        Menu = WidgetTree->ConstructWidget<UVerticalBox>(); Place(Root, Menu, .72f, .49f, .956f, .875f);
        Menu->AddChildToVerticalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        for (int32 I = 0; I < ItemCount; ++I)
        {
            auto* Button = WidgetTree->ConstructWidget<UMemoriaTitleButton>(); Button->Owner = this; Button->Index = I;
            Button->OnClicked.AddDynamic(Button, &UMemoriaTitleButton::Activate); Button->OnHovered.AddDynamic(Button, &UMemoriaTitleButton::Hover);
            auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetHeightOverride(44 * U); Size->AddChild(Button);
            Menu->AddChildToVerticalBox(Size)->SetPadding(FMargin(0, 0, 0, I + 1 < ItemCount ? 9 * U : 0));
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Button->SetContent(Row); FillContent(Button);
            auto* Accent = Fill(WidgetTree, FLinearColor::White); auto* AccentSize = WidgetTree->ConstructWidget<USizeBox>(); AccentSize->SetWidthOverride(2 * U); AccentSize->AddChild(Accent);
            Row->AddChildToHorizontalBox(AccentSize)->SetVerticalAlignment(VAlign_Fill);
            auto* Text = Label(WidgetTree, EFont::Ui, Pt(16), FLinearColor::White, .75f);
            { FSlateFontInfo Font = MemoriaFonts::Get(EFont::Ui, Pt(16)); Font.OutlineSettings.OutlineSize = 1; Font.OutlineSettings.OutlineColor = FLinearColor(0, 0, 0, .82f); Text->SetFont(Font); }
            auto* TextSlot = Row->AddChildToHorizontalBox(Text); TextSlot->SetVerticalAlignment(VAlign_Center); TextSlot->SetPadding(FMargin(14 * U, 0, 8 * U, 0));
            Items.Add(Button); ItemLabels.Add(Text); ItemAccents.Add(Accent); Emphasis.Add(0.f);
        }
        Menu->AddChildToVerticalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        // Options: the ported options_menu.gd settings over a dimmed title.
        OptionsLayer = WidgetTree->ConstructWidget<UCanvasPanel>(); Place(Root, OptionsLayer, 0, 0, 1, 1);
        Place(OptionsLayer, Fill(WidgetTree, FLinearColor(0.f, 0.f, .002f, .66f)), 0, 0, 1, 1);
        auto* Sheet = WidgetTree->ConstructWidget<UBorder>(); Sheet->SetPadding(FMargin(30 * U, 22 * U));
        Sheet->SetBrush(FSlateRoundedBoxBrush(Srgb(.018f, .014f, .027f, .94f), 3.f, Srgb(.55f, .40f, .22f, .7f), 1.f));
        Place(OptionsLayer, Sheet, .29f, .13f, .71f, .81f);
        auto* Rows = WidgetTree->ConstructWidget<UVerticalBox>(); Sheet->SetContent(Rows);
        auto* OptionsTitle = Label(WidgetTree, EFont::Title, Pt(26), Srgb(.96f, .82f, .52f)); Rows->AddChildToVerticalBox(OptionsTitle)->SetPadding(FMargin(0, 0, 0, 6 * U)); Copy.Add(OptionsTitle);
        auto* OptionsRule = WidgetTree->ConstructWidget<USizeBox>(); OptionsRule->SetHeightOverride(U); OptionsRule->AddChild(Fill(WidgetTree, Srgb(.78f, .56f, .27f, .48f)));
        Rows->AddChildToVerticalBox(OptionsRule)->SetPadding(FMargin(0, 0, 0, 6 * U));
        auto Section = [&]()
        { auto* S = Label(WidgetTree, EFont::Ui, Pt(12), Srgb(.72f, .60f, .44f, .92f)); Rows->AddChildToVerticalBox(S)->SetPadding(FMargin(0, 10 * U, 0, 4 * U)); Copy.Add(S); };
        auto Small = [&](UMemoriaTitleButton* Target, int32 Role, const TCHAR* Glyph)
        {
            auto* B = WidgetTree->ConstructWidget<UMemoriaTitleButton>(); B->Owner = this; B->Index = Target->Index; B->Role = Role;
            B->OnClicked.AddDynamic(B, &UMemoriaTitleButton::Activate);
            FButtonStyle Style; Style.SetNormal(FSlateNoResource()); Style.SetHovered(FSlateRoundedBoxBrush(Srgb(.15f, .105f, .065f, .6f), 2.f)); Style.SetPressed(Style.Hovered);
            Style.SetNormalPadding(FMargin(6 * U, 0)); Style.SetPressedPadding(FMargin(6 * U, 0)); B->SetStyle(Style);
            auto* G = Label(WidgetTree, EFont::Ui, Pt(13), Srgb(.86f, .64f, .29f)); G->SetText(FText::FromString(Glyph)); B->SetContent(G); return B;
        };
        for (int32 R = 0; R < OptionRowCount; ++R)
        {
            if (R == 0 || R == 3) Section();
            auto* Button = WidgetTree->ConstructWidget<UMemoriaTitleButton>(); Button->Owner = this; Button->Index = R; Button->Role = 1;
            Button->OnClicked.AddDynamic(Button, &UMemoriaTitleButton::Activate); Button->OnHovered.AddDynamic(Button, &UMemoriaTitleButton::Hover);
            auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetHeightOverride(40 * U); Size->AddChild(Button);
            Rows->AddChildToVerticalBox(Size)->SetPadding(FMargin(0, R == OptionRowCount - 1 ? 18 * U : 0, 0, 7 * U));
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Button->SetContent(Row); FillContent(Button);
            auto* Name = Label(WidgetTree, EFont::Ui, Pt(15), Srgb(.84f, .80f, .73f, .98f));
            auto* NameSlot = Row->AddChildToHorizontalBox(Name); NameSlot->SetVerticalAlignment(VAlign_Center); NameSlot->SetPadding(FMargin(16 * U, 0, 0, 0));
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Copy.Add(Name);
            if (R == OptionRowCount - 1) { Name->SetJustification(ETextJustify::Center); NameSlot->SetPadding(FMargin(0)); }
            auto* Value = Label(WidgetTree, EFont::Ui, Pt(15), Srgb(1.f, .86f, .52f)); Value->SetJustification(ETextJustify::Center);
            if (R < OptionRowCount - 1)
            {
                Row->AddChildToHorizontalBox(Small(Button, 2, TEXT("◀")))->SetVerticalAlignment(VAlign_Center);
                auto* ValueSize = WidgetTree->ConstructWidget<USizeBox>(); ValueSize->SetWidthOverride(96 * U); ValueSize->AddChild(Value);
                Row->AddChildToHorizontalBox(ValueSize)->SetVerticalAlignment(VAlign_Center);
                auto* Up = Row->AddChildToHorizontalBox(Small(Button, 3, TEXT("▶"))); Up->SetVerticalAlignment(VAlign_Center); Up->SetPadding(FMargin(0, 0, 10 * U, 0));
            }
            OptionRows.Add(Button); OptionValues.Add(Value);
        }
        OptionsLayer->SetVisibility(ESlateVisibility::Collapsed);
        // Letterbox strips, over everything.
        LetterboxTop = Fill(WidgetTree, Srgb(.004f, .003f, .008f, .94f)); Place(Root, LetterboxTop, 0, 0, 1, 0);
        LetterboxBottom = Fill(WidgetTree, Srgb(.004f, .003f, .008f, .94f)); Place(Root, LetterboxBottom, 0, 1, 1, 1);
        Refresh(); Animate(0.f);
    }
    return Super::RebuildWidget();
}
void UMemoriaTitleWidget::Refresh()
{
    if (Copy.Num() < 11) return;
    // Copy order: eyebrow, wordmark, subtitle, tagline, heading, footer; then the options title and, in build order,
    // the AUDIO section, three volume rows, the DISPLAY section, fullscreen, language and back.
    const FString Texts[] = {
        Loc(TEXT("A DARK FANTASY OF MEMORY AND LOSS"), TEXT("기억과 상실의 다크 판타지")), TEXT("MEMORIA"),
        Loc(TEXT("The Price of Oblivion"), TEXT("망각의 대가")), Loc(TEXT("Burn what you remember. Carry what remains."), TEXT("기억을 태워라. 남은 것을 짊어져라.")),
        Loc(TEXT("ENTER THE REMEMBERED PATH"), TEXT("기억의 문을 연다")),
        bLoadFailed ? Loc(TEXT("The save could not be loaded"), TEXT("저장을 불러오지 못했습니다")) : Loc(TEXT("↑ ↓  SELECT    ENTER  CONFIRM"), TEXT("↑ ↓  선택    ENTER  결정")),
        Loc(TEXT("OPTIONS"), TEXT("옵션")), Loc(TEXT("AUDIO"), TEXT("오디오")),
        Loc(TEXT("Master Volume"), TEXT("전체 음량")), Loc(TEXT("BGM Volume"), TEXT("배경음악 음량")), Loc(TEXT("SFX Volume"), TEXT("효과음 음량")),
        Loc(TEXT("DISPLAY"), TEXT("화면")), Loc(TEXT("Fullscreen"), TEXT("전체 화면")), Loc(TEXT("Language"), TEXT("언어")), Loc(TEXT("Back"), TEXT("뒤로"))};
    for (int32 I = 0; I < Copy.Num() && I < UE_ARRAY_COUNT(Texts); ++I) Copy[I]->SetText(FText::FromString(Texts[I]));
    Copy[5]->SetColorAndOpacity(bLoadFailed ? Srgb(.93f, .58f, .42f) : Srgb(.58f, .55f, .52f, .92f));
    const FString Labels[] = {Loc(TEXT("New Game"), TEXT("새 게임")), Loc(TEXT("Continue"), TEXT("이어하기")), Loc(TEXT("Options"), TEXT("옵션")), Loc(TEXT("Quit Game"), TEXT("게임 종료"))};
    for (int32 I = 0; I < Items.Num(); ++I)
    {
        const bool bEnabled = IsItemEnabled(I), bSel = bEnabled && I == Selected && !bOptionsOpen;
        Items[I]->SetIsEnabled(bEnabled); Items[I]->SetStyle(ItemStyle(bSel, bEnabled));
        ItemLabels[I]->SetText(FText::FromString(FString::Printf(TEXT("%02d    %s"), I + 1, *Labels[I])));
        ItemLabels[I]->SetColorAndOpacity(ItemInk(bSel, bEnabled));
        ItemAccents[I]->SetBrushColor(!bEnabled ? Srgb(.24f, .22f, .20f, .32f) : bSel ? Srgb(1.f, .76f, .36f, .94f) : Srgb(.48f, .36f, .22f, .48f));
        if (auto* Size = Cast<USizeBox>(ItemAccents[I]->GetParent())) Size->SetWidthOverride((bSel ? 4 : 2) * U);
    }
    if (Settings)
    {
        const FString Values[] = {FString::Printf(TEXT("%d%%"), Settings->GetMasterVolume()), FString::Printf(TEXT("%d%%"), Settings->GetMusicVolume()),
            FString::Printf(TEXT("%d%%"), Settings->GetSfxVolume()), Settings->IsFullscreen() ? Loc(TEXT("On"), TEXT("켜짐")) : Loc(TEXT("Off"), TEXT("꺼짐")),
            Settings->GetLocale() == TEXT("en") ? FString(TEXT("English")) : FString(TEXT("한국어")), FString()};
        for (int32 R = 0; R < OptionValues.Num(); ++R) OptionValues[R]->SetText(FText::FromString(Values[R]));
    }
    for (int32 R = 0; R < OptionRows.Num(); ++R) OptionRows[R]->SetStyle(ItemStyle(bOptionsOpen && R == OptionRow, true));
    if (OptionsLayer) OptionsLayer->SetVisibility(bOptionsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
int32 UMemoriaTitleWidget::Step(int32 From, int32 Direction) const
{
    // Focus skips the disabled Continue, wrapping like the source's focus neighbours.
    int32 I = From;
    for (int32 N = 0; N < ItemCount; ++N) { I = (I + Direction + ItemCount) % ItemCount; if (IsItemEnabled(I)) return I; }
    return From;
}
void UMemoriaTitleWidget::Navigate(int32 Direction)
{
    if (bOptionsOpen) { OptionRow = (OptionRow + Direction + OptionRowCount) % OptionRowCount; Refresh(); return; }
    Selected = Step(Selected, Direction); Refresh();
}
void UMemoriaTitleWidget::Select(int32 Index)
{ if (!bOptionsOpen && Index >= 0 && Index < ItemCount && IsItemEnabled(Index) && Index != Selected) { Selected = Index; Refresh(); } }
void UMemoriaTitleWidget::SelectOption(int32 Row)
{ if (bOptionsOpen && Row >= 0 && Row < OptionRowCount && Row != OptionRow) { OptionRow = Row; Refresh(); } }
void UMemoriaTitleWidget::Adjust(int32 Direction) { if (bOptionsOpen) ActivateOption(OptionRow, Direction); }
void UMemoriaTitleWidget::ConfirmIntent()
{
    if (bOptionsOpen) ActivateOption(OptionRow, 0); else ActivateItem(Selected);
}
bool UMemoriaTitleWidget::Back()
{
    if (!bOptionsOpen) return false;
    bOptionsOpen = false; Selected = 2; Refresh(); return true;
}
void UMemoriaTitleWidget::OpenOptions() { bOptionsOpen = true; OptionRow = 0; Refresh(); }
void UMemoriaTitleWidget::ActivateItem(int32 Index)
{
    if (bOptionsOpen || !IsItemEnabled(Index) || Index < 0 || Index >= ItemCount) return;
    Selected = Index;
    const EMemoriaTitleAction Action = static_cast<EMemoriaTitleAction>(Index);
    if (Action == EMemoriaTitleAction::Options) OpenOptions(); else Refresh();
    OnAction.ExecuteIfBound(Action);
}
void UMemoriaTitleWidget::ActivateOption(int32 Row, int32 Step)
{
    if (!bOptionsOpen || !Settings) return;
    OptionRow = Row;
    // Sliders move in tens; a confirm toggles the switches and leaves the sliders alone.
    switch (Row)
    {
    case 0: if (Step) Settings->SetMasterVolume(Settings->GetMasterVolume() + 10 * Step); break;
    case 1: if (Step) Settings->SetMusicVolume(Settings->GetMusicVolume() + 10 * Step); break;
    case 2: if (Step) Settings->SetSfxVolume(Settings->GetSfxVolume() + 10 * Step); break;
    case 3: Settings->SetFullscreen(!Settings->IsFullscreen()); break;
    case 4: Settings->SetLocale(Settings->GetLocale() == TEXT("en") ? TEXT("ko") : TEXT("en")); break;
    default: if (Step == 0) { Back(); return; }
    }
    Refresh();
}
void UMemoriaTitleWidget::AdvancePresentation(float Seconds) { Animate(Seconds); }
void UMemoriaTitleWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    Area = Geometry.GetLocalSize();
    if (Area.X > 0 && Area.Y > 0 && FSlateApplication::IsInitialized())
    {
        // _process: the key art drifts against the pointer (-11, -6 px at the edges).
        FVector2D Unit = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos()) / Area - FVector2D(.5, .5);
        Unit.X = FMath::Clamp(Unit.X, -.5, .5); Unit.Y = FMath::Clamp(Unit.Y, -.5, .5);
        Parallax = FMath::Lerp(Parallax, Unit * FVector2D(-11, -6), 1.f - FMath::Exp(-DeltaTime * 1.4f));
    }
    const UWorld* World = GetWorld();
    Animate(World ? World->GetDeltaSeconds() : DeltaTime);
}
void UMemoriaTitleWidget::Animate(float DeltaTime)
{
    if (!BackdropFit) return;
    Clock += DeltaTime; Ambient += DeltaTime;
    const float K = Area.Y / 720.f;
    // _fade_in_title: the intro choreography, then the ambient breath.
    Backdrop->SetRenderOpacity(FMath::Lerp(.72f, 1.f, Sine(Clock / 1.15f)));
    const float Breath = 1.012f + FMath::Sin(Ambient * .24f) * .0025f;
    BackdropFit->SetRenderTransformPivot(FVector2D(.5, .5));
    BackdropFit->SetRenderTransform(FWidgetTransform(Parallax * K, FVector2D(Breath, Breath), FVector2D::ZeroVector, 0.f));
    RiftGlow->SetRenderOpacity(.78f + FMath::Sin(Ambient * .72f) * .12f);
    TitleStack->SetRenderOpacity(Sine((Clock - .18f) / .72f));
    TitleStack->SetRenderTranslation(FVector2D(-34.f * (1.f - Quint((Clock - .18f) / .86f)) * K, 0));
    TitleRail->SetRenderOpacity(IsIntroComplete() ? .86f + FMath::Sin(Ambient * .58f) * .10f : Sine((Clock - .34f) / .62f));
    TitleRail->SetRenderTranslation(FVector2D(-16.f * (1.f - Quint((Clock - .26f) / .76f)) * K, 0));
    const float ChromeDelay[] = {.48f, .48f, .48f, .60f, .56f, .62f, .72f}, ChromeLength[] = {.64f, .64f, .64f, .52f, .52f, .52f, .52f};
    for (int32 I = 0; I < MenuChrome.Num(); ++I) MenuChrome[I]->SetRenderOpacity(Sine((Clock - ChromeDelay[I]) / ChromeLength[I]));
    Menu->SetRenderOpacity(Sine((Clock - .64f) / .70f));
    Menu->SetRenderTranslation(FVector2D(30.f * (1.f - Quint((Clock - .58f) / .82f)) * K, 0));
    const float Bar = FMath::Lerp(42.f, 24.f, Quint(Clock / 1.f)) * K;
    Cast<UCanvasPanelSlot>(LetterboxTop->Slot)->SetOffsets(FMargin(0, 0, 0, Bar));
    Cast<UCanvasPanelSlot>(LetterboxBottom->Slot)->SetOffsets(FMargin(0, -Bar, 0, Bar));
    // _set_title_button_emphasis: the focused item grows 1.8% and brightens over 0.14 s.
    for (int32 I = 0; I < Items.Num(); ++I)
    {
        const float Target = I == Selected && !bOptionsOpen && IsItemEnabled(I) ? 1.f : 0.f;
        Emphasis[I] = FMath::FInterpConstantTo(Emphasis[I], Target, DeltaTime, 1.f / .14f);
        const float S = 1.f + .018f * Quint(Emphasis[I]);
        Items[I]->SetRenderTransformPivot(FVector2D(0, .5)); Items[I]->SetRenderScale(FVector2D(S, S));
    }
    // TitleAsh: integrate the motes; each fades in and out over its life, then respawns in the box.
    const FVector2D Size = Area.X > 0 ? Area : FVector2D(1280, 720);
    for (int32 I = 0; I < Motes.Num(); ++I)
    {
        FMote& M = Motes[I];
        M.Age += DeltaTime; M.V += FVector2D(-.7f, -1.8f) * DeltaTime; M.P += M.V * DeltaTime;
        if (M.Age >= M.Life)
        {
            M.Age = 0; M.Size = 512.f * Rng.FRandRange(.45f, 1.1f);
            M.P = FVector2D(Rng.FRandRange(-.03f, 1.03f) * 1280.f, Rng.FRandRange(-.06f, 1.06f) * 720.f);
            M.V = FVector2D(-.18f, -1.f).GetSafeNormal().GetRotated(Rng.FRandRange(-26.f, 26.f)) * Rng.FRandRange(4.f, 12.f);
        }
        const float Fade = FMath::Sin(PI * FMath::Clamp(M.Age / M.Life, 0.f, 1.f));
        auto* MoteSlot = Cast<UCanvasPanelSlot>(Ash[I]->Slot);
        const float Px = M.Size * K;
        MoteSlot->SetPosition(M.P * FVector2D(Size.X / 1280.f, K) - FVector2D(Px, Px) * .5f); MoteSlot->SetSize(FVector2D(Px, Px));
        Ash[I]->SetRenderOpacity(Fade);
    }
}
FString UMemoriaTitleWidget::VisibleText() const
{
    TArray<FString> Out;
    for (const TObjectPtr<UTextBlock>& T : Copy) if (T && (T->IsVisible() || !bOptionsOpen)) Out.Add(T->GetText().ToString());
    for (const TObjectPtr<UTextBlock>& T : ItemLabels) if (T) Out.Add(T->GetText().ToString());
    if (bOptionsOpen) for (const TObjectPtr<UTextBlock>& T : OptionValues) if (T) Out.Add(T->GetText().ToString());
    return FString::Join(Out, TEXT("\n"));
}
UTexture2D* UMemoriaTitleWidget::DisplayedArtwork() const
{ return Backdrop ? Cast<UTexture2D>(Backdrop->GetBrush().GetResourceObject()) : nullptr; }
FReply UMemoriaTitleWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const FKey K = Event.GetKey();
    const bool Confirm = K == EKeys::Enter || K == EKeys::E || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom;
    const bool Up = K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up, Down = K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down;
    const bool Left = K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left, Right = K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right;
    const bool Cancel = K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right;
    if (!(Confirm || Up || Down || Left || Right || Cancel)) return Super::NativeOnPreviewKeyDown(Geometry, Event);
    // The owner keeps the physical confirm gesture so it cannot also advance the first VN line.
    OnConsumedKey.ExecuteIfBound(K, Event.IsRepeat() ? IE_Repeat : IE_Pressed);
    if (Confirm) { if (!Event.IsRepeat()) ConfirmIntent(); }
    else if (Up || Down) Navigate(Up ? -1 : 1);
    else if (Left || Right) Adjust(Left ? -1 : 1);
    else if (!Event.IsRepeat()) Back();
    return FReply::Handled();
}
FReply UMemoriaTitleWidget::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
    OnConsumedKey.ExecuteIfBound(Event.GetKey(), IE_Released);
    return Super::NativeOnKeyUp(Geometry, Event);
}
