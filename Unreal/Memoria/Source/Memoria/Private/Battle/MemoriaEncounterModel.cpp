#include "Battle/MemoriaEncounterModel.h"

FMemoriaEncounterRng FMemoriaEncounterRng::Random()
{
    return {
        [](double Min, double Max) { return Min + (Max - Min) * FMath::FRand(); },
        [](int32 Min, int32 Max) { return FMath::RandRange(Min, Max); }
    };
}
void FMemoriaEncounterModel::Reset(double InitialThreshold)
{
    *this = FMemoriaEncounterModel{};
    Threshold = InitialThreshold;
}
double FMemoriaEncounterModel::GetPressure() const
{
    const double Ratio = FMath::Clamp(StepCount / FMath::Max(Threshold, .001), 0., 1.);
    return FMath::Clamp((Ratio - .42) / .58, 0., 1.);
}
FMemoriaEncounterStep FMemoriaEncounterModel::Advance(const FVector2D& Position,
    bool bExploration, FMemoriaEncounterRng& Rng, bool bFieldDashing, double TileSize)
{
    FMemoriaEncounterStep Result;
    if (!bEnabled) return Result;
    if (!bExploration)
    {
        LastPosition = Position;
        return Result;
    }
    if (LastPosition == FVector2D::ZeroVector)
    {
        LastPosition = Position;
        Result.Pressure = GetPressure();
        return Result;
    }
    if (!FMath::IsFinite(TileSize) || TileSize <= 0. || Position.ContainsNaN()) return Result;
    const double Distance = FVector2D::Distance(Position, LastPosition) / TileSize;
    LastPosition = Position;
    if (Distance < .01)
    {
        Result.Pressure = GetPressure();
        return Result;
    }
    StepCount += Distance;
    if (bFieldDashing && bWarningEmitted)
    {
        StepCount = FMath::Max(0., StepCount - Distance * 4.4);
        if (StepCount < Threshold * .54)
        {
            bWarningEmitted = false;
            if (!bTrailBrokenFeedback)
            {
                bTrailBrokenFeedback = true;
                Result.bTrailBroken = true;
            }
        }
        Result.Pressure = GetPressure();
        return Result;
    }
    if (!bWarningEmitted && StepCount >= Threshold * .72)
    {
        bWarningEmitted = true;
        bTrailBrokenFeedback = false;
        Result.bWarningStarted = true;
    }
    Result.Pressure = GetPressure();
    if (StepCount >= Threshold && Rng.Real && Rng.Integer)
    {
        StepCount = 0.;
        Threshold = Rng.Real(60., 100.);
        bWarningEmitted = false;
        bTrailBrokenFeedback = false;
        Result.Pressure = 0.;
        Result.EnemyIndex = Rng.Integer(0, 1);
        Result.bTriggered = true;
    }
    return Result;
}
