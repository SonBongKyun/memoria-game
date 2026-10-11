#include "Combat/MemoriaAttackTelegraph.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace
{
bool Project(APlayerController* PC, const TArray<FVector>& World, TArray<FVector2f>& Screen)
{
    Screen.Reserve(World.Num());
    for (const FVector& Point : World)
    {
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Point, At, false)) return false;
        Screen.Add(FVector2f(At));
    }
    return true;
}
void Stroke(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G,
    const TArray<FVector2f>& Points, const FLinearColor& Color, float Width, bool bClose = false)
{
    if (Points.Num() < 2) return;
    TArray<FVector2f> Line = Points;
    if (bClose)
    {
        const FVector2f First = Line[0];
        Line.Add(First);
    }
    // A quiet dark edge keeps the warning legible on both wet paving and pale sand.
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Line,
        ESlateDrawEffect::None, FLinearColor(.035f, .018f, .014f, .72f), true, Width + 2.f);
    FSlateDrawElement::MakeLines(Elements, Layer + 1, G.ToPaintGeometry(), Line,
        ESlateDrawEffect::None, Color, true, Width);
}
void Fill(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G,
    const TArray<FVector2f>& Points, const FLinearColor& Color)
{
    if (Points.Num() < 3) return;
    FVector2f Center = FVector2f::ZeroVector;
    for (const auto& Point : Points) Center += Point;
    Center /= float(Points.Num());
    TArray<FSlateVertex> Verts;
    TArray<SlateIndex> Indices;
    const FColor Ink = Color.ToFColorSRGB();
    const auto Transform = G.GetAccumulatedRenderTransform();
    Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Center, FVector2f(.5f, .5f), Ink));
    for (const auto& Point : Points)
        Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Point, FVector2f(.5f, .5f), Ink));
    for (int32 I = 0; I < Points.Num(); ++I)
    {
        Indices.Add(0); Indices.Add(SlateIndex(I + 1)); Indices.Add(SlateIndex((I + 1) % Points.Num() + 1));
    }
    const auto Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
    FSlateDrawElement::MakeCustomVerts(Elements, Layer, Handle, Verts, Indices, nullptr, 0, 0);
}
TArray<FVector> Circle(const FVector& Center, float Radius, float Fraction = 1.f)
{
    TArray<FVector> Points;
    constexpr int32 Steps = 64;
    const int32 Count = FMath::Max(2, FMath::CeilToInt(Steps * Fraction));
    for (int32 I = 0; I <= Count; ++I)
    {
        const float Angle = -PI * .5f + 2.f * PI * Fraction * I / Count;
        Points.Add(Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius);
    }
    return Points;
}
TArray<FVector> Capsule(const FMemoriaAttackCue& Cue)
{
    TArray<FVector> Points;
    const FVector Side(-Cue.Direction.Y, Cue.Direction.X, 0);
    // Two semicircles preserve the full contact width at both ends of the rush.
    for (int32 End = 0; End < 2; ++End)
        for (int32 I = 0; I <= 24; ++I)
        {
            const float Angle = -PI * .5f + PI * I / 24.f + PI * End;
            const FVector Center = Cue.Origin + Cue.Direction * (End == 0 ? Cue.Length : 0.f);
            Points.Add(Center + (Cue.Direction * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * Cue.Radius);
        }
    if (Cue.bBlocked && !Cue.BlockNormal.IsNearlyZero())
    {
        // Trim the front cap at the actual wall plane; shortening the center line alone
        // would still paint a contact radius on the far side of a thin wall.
        TArray<FVector> Clipped;
        for (int32 I = 0; I < Points.Num(); ++I)
        {
            const FVector A = Points[I], B = Points[(I + 1) % Points.Num()];
            const float DA = FVector::DotProduct(A - Cue.BlockPoint, Cue.BlockNormal);
            const float DB = FVector::DotProduct(B - Cue.BlockPoint, Cue.BlockNormal);
            if (DA >= 0.f) Clipped.Add(A);
            if ((DA >= 0.f) != (DB >= 0.f)) Clipped.Add(FMath::Lerp(A, B, DA / (DA - DB)));
        }
        return Clipped;
    }
    return Points;
}
}
void MemoriaAttackTelegraph::Paint(FSlateWindowElementList& Elements, int32 Layer,
    const FGeometry& G, APlayerController* PC, const UMemoriaFieldCombatSubsystem& Combat)
{
    if (!PC) return;
    for (const auto& Weak : Combat.GetMonsters())
    {
        const auto* Monster = Weak.Get();
        if (!Monster) continue;
        const FMemoriaAttackCue Cue = Monster->GetAttackCue();
        if (!Cue.bActive) continue;
        const bool bCaster = Cue.Behaviour == EMemoriaFoeBehaviour::Caster;
        const bool bCharger = Cue.Behaviour == EMemoriaFoeBehaviour::Charger;
        const FLinearColor Hue = bCaster ? FLinearColor(.42f, .70f, 1.f) :
            bCharger ? FLinearColor(1.f, .22f, .10f) : FLinearColor(1.f, .48f, .13f);
        auto Ink = [&](float Alpha) { return FLinearColor(Hue.R, Hue.G, Hue.B, Alpha); };
        TArray<FVector2f> Outline;
        if (bCaster)
        {
            const FVector Tip = Cue.Origin + Cue.Direction * Cue.Length;
            const FVector Side(-Cue.Direction.Y, Cue.Direction.X, 0);
            // Open arrow, not a damage cone: the caster continues tracking until release.
            if (Project(PC, {Cue.Origin, Tip - Cue.Direction * 22.f}, Outline))
                Stroke(Elements, Layer, G, Outline, Ink(.65f), 2.f);
            TArray<FVector2f> Arrow;
            if (Project(PC, {Tip - Cue.Direction * 52.f + Side * Cue.Radius, Tip,
                Tip - Cue.Direction * 52.f - Side * Cue.Radius}, Arrow))
                Stroke(Elements, Layer + 2, G, Arrow, Ink(.95f), 3.f);
        }
        else if (Project(PC, bCharger ? Capsule(Cue) : Circle(Cue.Origin, Cue.Radius), Outline))
        {
            Fill(Elements, Layer, G, Outline, Ink(.055f + .045f * Cue.Progress));
            Stroke(Elements, Layer + 1, G, Outline, Ink(.9f), 2.5f, true);
            if (bCharger)
            {
                // Moving chevrons indicate committed travel without covering the ground.
                const FVector Side(-Cue.Direction.Y, Cue.Direction.X, 0);
                for (int32 I = 1; I <= 3; ++I)
                {
                    const FVector Tip = Cue.Origin + Cue.Direction * (Cue.Length * I / 4.f);
                    TArray<FVector2f> Arrow;
                    if (Project(PC, {Tip - Cue.Direction * 20.f + Side * 22.f, Tip,
                        Tip - Cue.Direction * 20.f - Side * 22.f}, Arrow))
                        Stroke(Elements, Layer + 2, G, Arrow, Ink(.3f + .6f * Cue.Progress), 2.f);
                }
            }
        }
        // The same timing ring communicates remaining windup for all three behaviours.
        TArray<FVector2f> Clock;
        const float ClockRadius = bCaster ? 42.f : bCharger ? 44.f : Cue.Radius;
        if (Project(PC, Circle(Cue.Origin, ClockRadius, FMath::Max(.01f, Cue.Progress)), Clock))
            Stroke(Elements, Layer + 3, G, Clock, Ink(1.f), 4.f);
    }
}
