#pragma once
#include "CoreMinimal.h"
class UMemoriaFieldCombatSubsystem;

// Shared draw-space layout. No world, settings, inventory or input state is changed.
namespace MemoriaHudLayoutS363
{
inline constexpr float RewardShownSeconds = 3.5f;
inline constexpr float TrayClearance = 12.f;
struct FRibbon
{
    float Scale = .6f;
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Extent = FVector2D::ZeroVector;
    FBox2D ArtBounds{ForceInit};
    // Includes the painted HP gauge, active attack hint and victory panel.
    FBox2D OccupiedBounds{ForceInit};
};
struct FQuickTray
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Cell = FVector2D(90, 104);
    float Gap = 7.f, Width = 0.f;
    FBox2D OriginalBounds{ForceInit}, Bounds{ForceInit};
    bool bLifted = false;
};
MEMORIA_API bool IsRibbonShowing(const UMemoriaFieldCombatSubsystem& Combat);
MEMORIA_API FRibbon Ribbon(const FVector2D& Viewport, bool bAttackHint, bool bVictory);
MEMORIA_API FQuickTray QuickTray(const FVector2D& Viewport, const FBox2D* RibbonBounds);
}

// Evidence is recorded by the actual native paint using its final draw coordinates.
struct FMemoriaRibbonPaintProbe
{
    bool bDrawn = false, bUsingGamepad = false;
    uint64 Frame = 0;
    FVector2D Viewport = FVector2D::ZeroVector;
    FBox2D ArtBounds{ForceInit}, OccupiedBounds{ForceInit}, MemoryOrbBounds{ForceInit};
    TArray<FBox2D> ActionCells;
    TArray<FString> ActionKeys;
};
struct FMemoriaQuickTrayPaintProbe
{
    bool bDrawn = false, bLifted = false, bUsingGamepad = false;
    uint64 Frame = 0;
    FVector2D Viewport = FVector2D::ZeroVector;
    FBox2D Bounds{ForceInit}, OriginalBounds{ForceInit};
    TArray<FBox2D> Cells, KeyBounds, NameBounds;
    TArray<FString> Keys, Names;
};
