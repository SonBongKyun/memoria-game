#pragma once
#include "CoreMinimal.h"

// Typed, synchronous draws preserve source RNG order and allow a recorded oracle stream.
struct MEMORIA_API FMemoriaEncounterRng
{
    TFunction<double(double, double)> Real;
    TFunction<int32(int32, int32)> Integer;
    static FMemoriaEncounterRng Random();
};
struct MEMORIA_API FMemoriaEncounterStep
{
    bool bTriggered = false, bWarningStarted = false, bTrailBroken = false;
    double Pressure = 0.;
    int32 EnemyIndex = INDEX_NONE;
};
// Source RandomEncounter distance model. Units are source pixels, including the
// source zero-position sentinel. State is transient and never enters a checkpoint.
struct MEMORIA_API FMemoriaEncounterModel
{
    double StepCount = 0., Threshold = 0.;
    FVector2D LastPosition = FVector2D::ZeroVector;
    bool bEnabled = true, bWarningEmitted = false, bTrailBrokenFeedback = false;
    // RandomEncounter.setup's min_steps, max_steps and pool size. The defaults are verdan_market.gd's.
    double MinSteps = 60., MaxSteps = 100.;
    int32 PoolSize = 2;
    void Reset(double InitialThreshold);
    FMemoriaEncounterStep Advance(const FVector2D& Position, bool bExploration,
        FMemoriaEncounterRng& Rng, bool bFieldDashing = false, double TileSize = 32.);
    double GetPressure() const;
};
