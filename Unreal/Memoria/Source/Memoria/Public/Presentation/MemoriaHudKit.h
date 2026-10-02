#pragma once
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
class UTexture2D;
class FSlateWindowElementList;
struct FGeometry;

// S339: what the field HUD's painted widgets share. The plates are the source's own UI paintings
// (exploration_hud.gd, notification_toast.gd, battle_scene.gd), cut free of their black ground by
// Unreal/Tools/export_hud_art.py and imported by -run=MemoriaHudAssets. The HUD is laid out in the viewport's
// 1080p units, like the widgets it replaces.
namespace MemoriaHudKit
{
    enum class EArt : uint8 { Plate, Toast, Ribbon };
    MEMORIA_API const TCHAR* Package(EArt Art);
    MEMORIA_API UTexture2D* Load(EArt Art);
    // Points the brush at the texture (an image brush at the texture's size). False when the texture is missing.
    MEMORIA_API bool Brush(FSlateBrush& Out, UTexture2D* Texture);
    MEMORIA_API void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color);
    MEMORIA_API void Image(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FSlateBrush& Brush, const FVector2D& At, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White);
    MEMORIA_API float Width(const FString& S, const FSlateFontInfo& Font);
    // Text anchored by its left (Align 0), centre (.5) or right (1) edge, vertically centred on Anchor.Y, over a soft shadow.
    MEMORIA_API void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Anchor, float Align, const FSlateFontInfo& Font, const FLinearColor& Color);
    // The plates' gem: a diamond outline.
    MEMORIA_API void Diamond(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& Centre, const FVector2D& Half, float Thickness, const FLinearColor& Color);
    // Part of a ring, clockwise from the top, Fraction of a full turn.
    MEMORIA_API void Arc(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& Centre, float Radius, float Fraction, float Thickness, const FLinearColor& Color);
    // A gauge: a dark track, a trailing ghost, the fill with a bright top edge. Uses three layers.
    MEMORIA_API void Gauge(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Fraction, float Ghost, const FLinearColor& Fill);
    // A plain dark panel with a thin amber edge, for when a plate is not imported and for small chips.
    MEMORIA_API void Panel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Alpha = 1.f);
}
