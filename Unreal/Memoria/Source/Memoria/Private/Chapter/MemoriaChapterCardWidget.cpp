#include "Chapter/MemoriaChapterCardWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace
{
using MemoriaUiKit::Srgb;
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Center, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D At = Center - Size * .5;
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(2, 2)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .8f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
}
void UMemoriaChapterCardWidget::Show(const FString& InEyebrow, const FString& InTitle, const FString& InSubtitle, float InSeconds)
{ Eyebrow = InEyebrow; Title = InTitle; Subtitle = InSubtitle; Seconds = InSeconds; Age = 0.f; }
void UMemoriaChapterCardWidget::NativeTick(const FGeometry& Geometry, float DeltaTime) { Super::NativeTick(Geometry, DeltaTime); Age += DeltaTime; }
int32 UMemoriaChapterCardWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    if (!IsShowing()) return L;
    // Fade in for half a second, hold, fade out over the last second.
    const float A = FMath::Clamp(FMath::Min(Age / .5f, (Seconds - Age) / 1.f), 0.f, 1.f);
    const FVector2D Size = Geometry.GetLocalSize();
    const FVector2D Band(Size.X, 250), At(0, Size.Y * .5 - 125);
    FSlateDrawElement::MakeBox(Elements, L, Geometry.ToPaintGeometry(FVector2f(Band), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, FLinearColor(0, 0, 0, .62f * A));
    for (const float Y : {At.Y, At.Y + Band.Y - 2})
        FSlateDrawElement::MakeBox(Elements, L + 1, Geometry.ToPaintGeometry(FVector2f(Size.X * .5, 2), FSlateLayoutTransform(FVector2f(Size.X * .25, Y))),
            FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Srgb(.85f, .66f, .36f, .7f * A));
    Text(Elements, L + 2, Geometry, Eyebrow, FVector2D(Size.X * .5, At.Y + 52), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 20), Srgb(.85f, .72f, .50f, A));
    Text(Elements, L + 2, Geometry, Title, FVector2D(Size.X * .5, At.Y + 118), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 54), Srgb(.96f, .90f, .80f, A));
    Text(Elements, L + 2, Geometry, Subtitle, FVector2D(Size.X * .5, At.Y + 186), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 22), Srgb(.78f, .72f, .64f, A));
    return L + 4;
}
