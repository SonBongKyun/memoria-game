#include "Tutorial/MemoriaHintWidget.h"
#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
namespace
{
using MemoriaUiKit::Srgb;
// Tween.TRANS_BACK / EASE_OUT: overshoots a little, then settles.
float BackOut(float T) { const float C1 = 1.70158f, C3 = C1 + 1.f; T -= 1.f; return 1.f + C3 * T * T * T + C1 * T * T; }
// Label.AUTOWRAP_WORD: break at spaces to fit the width.
TArray<FString> Wrap(const FString& Text, const FSlateFontInfo& Font, float Width)
{
    const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    TArray<FString> Words, Out; Text.ParseIntoArray(Words, TEXT(" "), true);
    FString Line;
    for (const FString& Word : Words)
    {
        const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        if (!Line.IsEmpty() && Measure->Measure(Try, Font).X > Width) { Out.Add(Line); Line = Word; }
        else Line = Try;
    }
    if (!Line.IsEmpty()) Out.Add(Line);
    return Out;
}
}
void UMemoriaHintWidget::Bind(UMemoriaTutorialSubsystem* InTutorial, bool bInKo)
{
    Tutorial = InTutorial; bKo = bInKo;
    if (!Banner && (Banner = MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_tutorial_hint_banner.png"))))
    { BannerBrush.SetResourceObject(Banner); BannerBrush.ImageSize = FVector2D(Banner->GetSizeX(), Banner->GetSizeY()); BannerBrush.DrawAs = ESlateBrushDrawType::Image; }
}
int32 UMemoriaHintWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* T = Tutorial.Get();
    if (!T || !T->IsShowing()) { Lines.Reset(); return L; }
    const FVector2D Size = Geometry.GetLocalSize();
    const float S = Size.Y / 720.f;
    // Slide down 70 and fade in over .35 s; on the way out, back up and fade over .25 s.
    const float In = FMath::Clamp(T->GetAge() / UMemoriaTutorialSubsystem::SlideSeconds, 0.f, 1.f);
    const float Out = T->IsLeaving() ? FMath::Clamp(T->GetOutAge() / UMemoriaTutorialSubsystem::OutSeconds, 0.f, 1.f) : 0.f;
    const float Alpha = FMath::Min(In, 1.f - Out);
    const float Slide = -70.f * S * (T->IsLeaving() ? Out : 1.f - BackOut(In));
    const float Top = 10.f * S + Slide;
    const FSlateFontInfo Font = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(15.f * S));
    // The source's top band (.20-.80) sits where nothing else is; here the status panel holds the top left
    // (to .37) and the HUD the top right (from .865), so the hint takes the band between them.
    const float Margin = 16.f * S, PanelLeft = Size.X * .395f, PanelWidth = Size.X * .44f;
    Lines = Wrap(UMemoriaTutorialSubsystem::Text(T->GetCurrent(), bKo), Font, PanelWidth - Margin * 2.f);
    const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const float LineHeight = Measure->GetMaxCharacterHeight(Font) * 1.08f;
    const float Height = FMath::Max(54.f * S, Lines.Num() * LineHeight + Margin * 2.f);
    const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    // The banner frame behind (1.5% wider each side, clear of the status panel; -12..+68 around the panel top), tinted warm at .78.
    if (Banner)
    {
        const float BannerHeight = FMath::Max(80.f * S, Height + 26.f * S);
        FSlateDrawElement::MakeBox(Elements, L, Geometry.ToPaintGeometry(FVector2f(PanelWidth + Size.X * .03f, BannerHeight), FSlateLayoutTransform(FVector2f(PanelLeft - Size.X * .015f, Top - 12.f * S))),
            &BannerBrush, ESlateDrawEffect::None, FLinearColor(1.f, .92f, .78f, .78f * Alpha));
    }
    // StyleBoxFlat: bg (0.030, 0.026, 0.038, 0.70), a thin gold border heavier on top.
    const FVector2f PanelSize(PanelWidth, Height);
    const FSlateLayoutTransform PanelAt(FVector2f(PanelLeft, Top));
    FSlateDrawElement::MakeBox(Elements, L + 1, Geometry.ToPaintGeometry(PanelSize, PanelAt), White, ESlateDrawEffect::None, Srgb(.030f, .026f, .038f, .70f * Alpha));
    const FLinearColor Border = Srgb(.70f, .56f, .34f, .36f * Alpha);
    FSlateDrawElement::MakeBox(Elements, L + 2, Geometry.ToPaintGeometry(FVector2f(PanelWidth, 2.f * S), PanelAt), White, ESlateDrawEffect::None, Border);
    for (const float X : {PanelLeft, PanelLeft + PanelWidth - S})
        FSlateDrawElement::MakeBox(Elements, L + 2, Geometry.ToPaintGeometry(FVector2f(S, Height), FSlateLayoutTransform(FVector2f(X, Top))), White, ESlateDrawEffect::None, Border);
    FSlateDrawElement::MakeBox(Elements, L + 2, Geometry.ToPaintGeometry(FVector2f(PanelWidth, S), FSlateLayoutTransform(FVector2f(PanelLeft, Top + Height - S))), White, ESlateDrawEffect::None, Border);
    // The hint, centred, with the source's one-pixel shadow.
    float Y = Top + (Height - Lines.Num() * LineHeight) * .5f;
    for (const FString& Line : Lines)
    {
        const FVector2D LineSize = Measure->Measure(Line, Font);
        const FVector2f At(PanelLeft + (PanelWidth - LineSize.X) * .5f, Y);
        FSlateDrawElement::MakeText(Elements, L + 3, Geometry.ToPaintGeometry(FVector2f(LineSize), FSlateLayoutTransform(At + FVector2f(S, S))), Line, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, .58f * Alpha));
        FSlateDrawElement::MakeText(Elements, L + 4, Geometry.ToPaintGeometry(FVector2f(LineSize), FSlateLayoutTransform(At)), Line, Font, ESlateDrawEffect::None, Srgb(.92f, .86f, .72f, Alpha));
        Y += LineHeight;
    }
    return L + 5;
}
