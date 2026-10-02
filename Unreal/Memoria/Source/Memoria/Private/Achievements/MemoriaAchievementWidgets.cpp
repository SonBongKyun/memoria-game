#include "Achievements/MemoriaAchievementWidgets.h"
#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
namespace
{
using MemoriaUiKit::Srgb;
FVector2D Measure(const FString& S, const FSlateFontInfo& Font) { return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font); }
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& At, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = Measure(S, Font);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1, 1)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .6f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
// StyleBoxFlat with an even border.
void Panel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Fill, const FLinearColor& Border, float Width)
{
    Box(Elements, Layer, G, At, Size, Fill);
    Box(Elements, Layer + 1, G, At, FVector2D(Size.X, Width), Border); Box(Elements, Layer + 1, G, At + FVector2D(0, Size.Y - Width), FVector2D(Size.X, Width), Border);
    Box(Elements, Layer + 1, G, At, FVector2D(Width, Size.Y), Border); Box(Elements, Layer + 1, G, At + FVector2D(Size.X - Width, 0), FVector2D(Width, Size.Y), Border);
}
// The viewer's single-colour glyphs (icon_map), within the bundled fonts' coverage.
FString Glyph(const TCHAR* Icon)
{
    static const TMap<FString, FString> Glyphs = {
        {TEXT("sword"), TEXT("†")}, {TEXT("skull"), TEXT("▼")}, {TEXT("crown"), TEXT("◆")}, {TEXT("shield"), TEXT("■")}, {TEXT("heart"), TEXT("♥")},
        {TEXT("potion"), TEXT("◇")}, {TEXT("flame"), TEXT("▲")}, {TEXT("eye"), TEXT("◉")}, {TEXT("map"), TEXT("◎")}, {TEXT("book"), TEXT("▤")},
        {TEXT("star"), TEXT("★")}, {TEXT("coin"), TEXT("●")}, {TEXT("cycle"), TEXT("○")}};
    const FString* Found = Glyphs.Find(Icon); return Found ? *Found : TEXT("•");
}
}
int32 UMemoriaAchievementPopupWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* A = Achievements.Get();
    const FMemoriaAchievement* Shown = A ? UMemoriaAchievementSubsystem::Find(A->GetPopup()) : nullptr;
    if (!Shown) { ShownTitle.Reset(); return L; }
    ShownTitle = Shown->Title;
    const FVector2D Size = Geometry.GetLocalSize();
    const float S = Size.Y / 720.f, Age = A->GetPopupAge();
    // offset_top -80 -> 12 over .4 s (back-ease), hold 4 s, back to -80 over .3 s.
    const float In = FMath::Clamp(Age / UMemoriaAchievementSubsystem::PopupIn, 0.f, 1.f);
    const float Out = FMath::Clamp((Age - UMemoriaAchievementSubsystem::PopupIn - UMemoriaAchievementSubsystem::PopupHold) / UMemoriaAchievementSubsystem::PopupOut, 0.f, 1.f);
    const float Ease = [](float T) { const float C1 = 1.70158f, C3 = C1 + 1.f; T -= 1.f; return 1.f + C3 * T * T * T + C1 * T * T; }(In);
    // S339: the status plate holds the top right down to about 190, so the popup comes down to rest under it
    // (the same travel, 92, from above its resting place).
    const float Top = FMath::Lerp(FMath::Lerp(104.f, 196.f, Ease), 104.f, Out) * S;
    const float Alpha = FMath::Min(FMath::Clamp(Age / .3f, 0.f, 1.f), 1.f - Out);
    const FVector2D At(Size.X - 320.f * S, Top), Panel2(308.f * S, 68.f * S);
    Panel(Elements, L, Geometry, At, Panel2, Srgb(.08f, .07f, .06f, .95f * Alpha), Srgb(.7f, .55f, .25f, .9f * Alpha), 2.f * S);
    Text(Elements, L + 2, Geometry, Glyph(Shown->Icon), At + FVector2D(16, 18) * S, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(24 * S)), Srgb(.95f, .85f, .55f, Alpha));
    const FVector2D TextAt = At + FVector2D(60, 8) * S;
    Text(Elements, L + 2, Geometry, TEXT("ACHIEVEMENT UNLOCKED"), TextAt, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(11 * S)), Srgb(.6f, .55f, .4f, Alpha));
    Text(Elements, L + 2, Geometry, Shown->Title, TextAt + FVector2D(0, 17) * S, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(15 * S)), Srgb(.95f, .85f, .55f, Alpha));
    Text(Elements, L + 2, Geometry, Shown->Desc, TextAt + FVector2D(0, 38) * S, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(11 * S)), Srgb(.55f, .5f, .45f, Alpha));
    return L + 4;
}
void UMemoriaAchievementsWidget::Configure(UMemoriaAchievementSubsystem* InAchievements, bool bInKo)
{
    Achievements = InAchievements; bKo = bInKo; FirstRow = 0;
    if (!Backdrop && (Backdrop = MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_achievements_chronicle_backdrop.png"))))
    { BackdropBrush.SetResourceObject(Backdrop); BackdropBrush.ImageSize = FVector2D(Backdrop->GetSizeX(), Backdrop->GetSizeY()); BackdropBrush.DrawAs = ESlateBrushDrawType::Image; }
}
void UMemoriaAchievementsWidget::Scroll(int32 Rows)
{
    const int32 Last = FMath::Max(0, UMemoriaAchievementSubsystem::All().Num() - VisibleRows);
    FirstRow = FMath::Clamp(FirstRow + Rows, 0, Last);
}
FReply UMemoriaAchievementsWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{ Scroll(Event.GetWheelDelta() > 0 ? -1 : 1); return FReply::Handled(); }
FString UMemoriaAchievementsWidget::HeaderText() const
{
    const auto* A = Achievements.Get();
    return FString::Printf(TEXT("ACHIEVEMENTS  (%d / %d)"), A ? A->NumUnlocked() : 0, UMemoriaAchievementSubsystem::All().Num());
}
FString UMemoriaAchievementsWidget::ProgressText() const
{
    const auto* A = Achievements.Get();
    const float Pct = float(A ? A->NumUnlocked() : 0) / FMath::Max(1, UMemoriaAchievementSubsystem::All().Num()) * 100.f;
    return bKo ? FString::Printf(TEXT("기억 속에 새겨진 이정표  ·  달성률 %.0f%%"), Pct) : FString::Printf(TEXT("Milestones engraved in memory  ·  %.0f%% complete"), Pct);
}
FString UMemoriaAchievementsWidget::RowTitle(int32 Index) const
{
    const auto& All = UMemoriaAchievementSubsystem::All();
    if (!All.IsValidIndex(Index)) return FString();
    const auto* A = Achievements.Get();
    return A && A->IsUnlocked(All[Index].Id) ? FString(All[Index].Title) : TEXT("???");
}
int32 UMemoriaAchievementsWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const float S = Size.Y / 720.f;
    // _add_modal_backdrop: the chronicle art covering the screen under a violet-black veil.
    if (Backdrop)
    {
        const float Scale = FMath::Max(Size.X / Backdrop->GetSizeX(), Size.Y / Backdrop->GetSizeY());
        const FVector2D Art(Backdrop->GetSizeX() * Scale, Backdrop->GetSizeY() * Scale);
        FSlateDrawElement::MakeBox(Elements, L, Geometry.ToPaintGeometry(FVector2f(Art), FSlateLayoutTransform(FVector2f((Size - Art) * .5))), &BackdropBrush, ESlateDrawEffect::None, FLinearColor::White);
    }
    Box(Elements, L + 1, Geometry, FVector2D::ZeroVector, Size, Srgb(.01f, .008f, .018f, Backdrop ? .42f : .85f));
    // The panel: anchors .12-.88 by .05-.95, 16 margins.
    const FVector2D At(Size.X * .12f, Size.Y * .05f), Panel2(Size.X * .76f, Size.Y * .90f);
    Panel(Elements, L + 2, Geometry, At, Panel2, Srgb(.035f, .03f, .05f, .80f), Srgb(.55f, .42f, .25f, .78f), 2.f * S);
    const float M = 16.f * S, Inner = Panel2.X - M * 2.f;
    const FSlateFontInfo HeaderFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, FMath::RoundToInt(22 * S));
    const FString Header = HeaderText();
    Text(Elements, L + 4, Geometry, Header, FVector2D(At.X + (Panel2.X - Measure(Header, HeaderFont).X) * .5f, At.Y + M), HeaderFont, Srgb(.85f, .7f, .45f));
    const FSlateFontInfo Small = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(13 * S));
    const FString Progress = ProgressText();
    Text(Elements, L + 4, Geometry, Progress, FVector2D(At.X + (Panel2.X - Measure(Progress, Small).X) * .5f, At.Y + M + 44 * S), Small, Srgb(.56f, .54f, .52f));
    Box(Elements, L + 3, Geometry, FVector2D(At.X + M, At.Y + M + 68 * S), FVector2D(Inner, 1.f * S), Srgb(.55f, .42f, .25f, .45f));
    // The list: each row an 8-margin panel with a 3 px left rule; titles 14, descriptions 13.
    const float ListTop = At.Y + M + 78 * S, ListBottom = At.Y + Panel2.Y - M - 26 * S, RowH = 48.f * S, Gap = 4.f * S;
    VisibleRows = FMath::Max(1, int32((ListBottom - ListTop + Gap) / (RowH + Gap)));
    const auto& All = UMemoriaAchievementSubsystem::All();
    const auto* A = Achievements.Get();
    const FSlateFontInfo GlyphFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(16 * S));
    const FSlateFontInfo TitleFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(14 * S));
    for (int32 N = 0; N < VisibleRows && All.IsValidIndex(FirstRow + N); ++N)
    {
        const int32 I = FirstRow + N;
        const bool bOn = A && A->IsUnlocked(All[I].Id);
        const FVector2D RowAt(At.X + M, ListTop + N * (RowH + Gap));
        Box(Elements, L + 3, Geometry, RowAt, FVector2D(Inner, RowH), bOn ? Srgb(.08f, .065f, .09f, .66f) : Srgb(.035f, .03f, .045f, .56f));
        Box(Elements, L + 4, Geometry, RowAt, FVector2D(3.f * S, RowH), bOn ? Srgb(.58f, .44f, .24f, .42f) : Srgb(.2f, .18f, .2f, .28f));
        Text(Elements, L + 5, Geometry, Glyph(All[I].Icon), RowAt + FVector2D(14, 12) * S, GlyphFont, bOn ? Srgb(.82f, .67f, .38f) : Srgb(.3f, .28f, .3f));
        Text(Elements, L + 5, Geometry, RowTitle(I), RowAt + FVector2D(46, 6) * S, TitleFont, bOn ? Srgb(.85f, .75f, .5f) : Srgb(.35f, .3f, .28f));
        Text(Elements, L + 5, Geometry, All[I].Desc, RowAt + FVector2D(46, 26) * S, Small, bOn ? Srgb(.6f, .55f, .5f) : Srgb(.3f, .28f, .25f));
    }
    // Where the list stands, and the close hint.
    const FString Range = FString::Printf(TEXT("%d-%d / %d"), FirstRow + 1, FMath::Min(FirstRow + VisibleRows, All.Num()), All.Num());
    Text(Elements, L + 4, Geometry, Range, FVector2D(At.X + M, ListBottom + 6 * S), Small, Srgb(.4f, .35f, .3f));
    const FString Close = bKo ? TEXT("[ESC] 닫기") : TEXT("[ESC] Close");
    Text(Elements, L + 4, Geometry, Close, FVector2D(At.X + Panel2.X - M - Measure(Close, Small).X, ListBottom + 6 * S), Small, Srgb(.4f, .35f, .3f));
    return L + 7;
}
