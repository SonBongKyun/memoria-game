#pragma once

#include "CoreMinimal.h"

namespace Memoria::Coordinates
{
// Coordinate schema 1: one source pixel per UE unit, XY plane, camera -Z.
inline FVector FromSource(const FVector2D& Pixel) { return FVector(Pixel.X, -Pixel.Y, 0.0); }
inline FVector2D ToSource(const FVector& World) { return FVector2D(World.X, -World.Y); }
}
