#pragma once
#include "CoreMinimal.h"
class UTexture2D;
// Shared presentation helpers for the native widgets (VN, title).
namespace MemoriaUiKit
{
    // Godot 2D colors are sRGB; Slate colors and tints are linear. Source colors pass through this.
    MEMORIA_API FLinearColor Srgb(float R, float G, float B, float A = 1.f);
    struct FStop { float T; FLinearColor C; };
    // Godot GradientTexture2D: linear or radial fill from From to To in UV space; colors are authored sRGB bytes.
    MEMORIA_API UTexture2D* Gradient(int32 W, int32 H, FVector2D From, FVector2D To, bool bRadial, std::initializer_list<FStop> Stops);
    // A transient texture painted per pixel from UV; used to bake a source canvas_item shader once.
    MEMORIA_API UTexture2D* Paint(int32 W, int32 H, TFunctionRef<FLinearColor(const FVector2D&)> Color);
}
