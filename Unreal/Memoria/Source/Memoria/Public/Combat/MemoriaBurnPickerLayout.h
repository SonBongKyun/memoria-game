#pragma once
#include "CoreMinimal.h"

// S352: one geometry contract for the painted picker and its runtime regression. The selected memory
// always remains in a bounded window; footer space is reserved before any rows are allocated.
namespace MemoriaBurnPicker
{
struct FLayout
{
    FVector2D Panel = FVector2D::ZeroVector;
    FVector2D Extent = FVector2D::ZeroVector;
    float Scale = 1.f, RowHeight = 64.f, ListTop = 144.f, FooterHeight = 82.f;
    int32 First = 0, Visible = 0;

    bool Shows(int32 Index) const { return Index >= First && Index < First + Visible; }
    FVector2D RowAt(int32 Index) const { return Panel + FVector2D(24.f * Scale, ListTop + (Index - First) * RowHeight); }
    float FooterTop() const { return Panel.Y + Extent.Y - FooterHeight; }
    FVector2D WarningAt() const { return Panel + FVector2D(Extent.X * .5f, Extent.Y - 56.f * Scale); }
    FVector2D ControlsAt() const { return Panel + FVector2D(Extent.X * .5f, Extent.Y - 26.f * Scale); }
};

inline FLayout Build(const FVector2D& Viewport, int32 Count, int32 Selection)
{
    FLayout L;
    const FVector2D Size(FMath::Max(1.f, float(Viewport.X)), FMath::Max(1.f, float(Viewport.Y)));
    L.Scale = FMath::Clamp(FMath::Min(float(Size.X) / 840.f, float(Size.Y) / 540.f), .1f, 1.f);
    const float Margin = 24.f * L.Scale;
    L.RowHeight *= L.Scale; L.ListTop *= L.Scale; L.FooterHeight *= L.Scale;
    const float MaxHeight = FMath::Max(0.f, float(Size.Y) - 2.f * Margin);
    const int32 Capacity = FMath::Clamp(FMath::FloorToInt((MaxHeight - L.ListTop - L.FooterHeight) / L.RowHeight), 0, 6);
    L.Visible = FMath::Min(FMath::Max(0, Count), Capacity);
    L.First = FMath::Clamp(Selection - L.Visible / 2, 0, FMath::Max(0, Count - L.Visible));
    L.Extent = FVector2D(FMath::Min(760.f * L.Scale, FMath::Max(0.f, float(Size.X) - 2.f * Margin)),
        FMath::Min(MaxHeight, L.ListTop + L.Visible * L.RowHeight + L.FooterHeight));
    L.Panel = (Size - L.Extent) * .5f;
    return L;
}
}
