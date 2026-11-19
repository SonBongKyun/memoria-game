#pragma once
#include "CoreMinimal.h"
class AMemoriaFieldMonster;

struct FMemoriaPopupPaintRecordS365
{
    int32 SourceIndex = INDEX_NONE;
    FString Text;
    float Amount = 0.f, Alpha = 0.f;
    FVector2D DesiredCenter = FVector2D::ZeroVector, PaintedCenter = FVector2D::ZeroVector;
    FBox2D Bounds{ForceInit};
    bool bOverloaded = false;
};
struct FMemoriaHealthPaintRecordS365
{
    TWeakObjectPtr<AMemoriaFieldMonster> Target;
    float Current = 0.f, Trail = 0.f, Max = 0.f;
    FBox2D Bounds{ForceInit}, RecentDamageBounds{ForceInit};
    bool bTrailVisible = false;
};
struct FMemoriaCombatFeedbackPaintProbe
{
    uint64 Frame = 0;
    bool bDrawn = false, bEnabled = false;
    FVector2D Viewport = FVector2D::ZeroVector;
    TArray<FMemoriaPopupPaintRecordS365> Popups;
    TArray<FMemoriaHealthPaintRecordS365> Health;
};

namespace MemoriaCombatFeedbackS365
{
    MEMORIA_API bool IsEnabled();
    // Actual font extents, including the existing 1.5-unit lower-right shadow.
    // Chronological priority keeps old entries ahead of newly arriving entries.
    MEMORIA_API void PlacePopups(TArray<FMemoriaPopupPaintRecordS365>& Entries, const FVector2D& Viewport);
    struct FHealthTrail
    {
        float Last = 0.f, Value = 0.f, Max = 0.f, Hold = 0.f;
        bool bReady = false;
        MEMORIA_API void Observe(float Health, float MaxHealth, float DeltaSeconds, bool bSnap);
    };
}
