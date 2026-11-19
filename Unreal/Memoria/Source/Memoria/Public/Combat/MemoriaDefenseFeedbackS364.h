#pragma once
#include "CoreMinimal.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"

// Read-only observations of lines and glyphs actually submitted by the HUD.
struct FMemoriaDefensePopupPaintRecord
{
    EMemoriaDefenseOutcome Outcome = EMemoriaDefenseOutcome::None;
    FString Text;
    float Amount = 0.f, Alpha = 0.f;
    FBox2D Bounds{ForceInit};
};
struct FMemoriaDefenseShapePaintRecord
{
    EMemoriaDefenseOutcome Outcome = EMemoriaDefenseOutcome::None;
    FBox2D Bounds{ForceInit};
    int32 Segments = 0;
    bool bReduceMotion = false;
    float FlashScale = 1.f, Age = 0.f;
};
struct FMemoriaDefensePaintProbe
{
    uint64 Frame = 0;
    bool bDrawn = false, bEnabled = false, bReduceMotion = false;
    float FlashScale = 1.f;
    FVector2D Viewport = FVector2D::ZeroVector;
    TArray<FMemoriaDefensePopupPaintRecord> Popups;
    TArray<FMemoriaDefenseShapePaintRecord> Shapes;
};

namespace MemoriaDefenseFeedbackS364
{
struct FLine
{
    FVector2D A, B;
    FLinearColor Color;
    float Thickness = 2.f;
};
struct FShape
{
    TArray<FLine> Lines;
    FBox2D Bounds{ForceInit};
};
MEMORIA_API bool IsEnabled();
MEMORIA_API FString PopupText(const FMemoriaCombatPopup& Popup, bool bKo);
// Local Slate offsets around the projected contact. No world, UObject, time,
// random, saved state or gameplay reads occur while constructing this geometry.
MEMORIA_API FShape Shape(const FMemoriaImpact& Impact, bool bReduceMotion, float FlashScale);
}
