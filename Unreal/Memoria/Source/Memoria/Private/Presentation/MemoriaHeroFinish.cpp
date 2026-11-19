#include "Presentation/MemoriaHeroFinish.h"
#include "Animation/MorphTarget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

const FName FMemoriaHeroFinish::BlinkLeft(TEXT("HeroBlink_L"));
const FName FMemoriaHeroFinish::BlinkRight(TEXT("HeroBlink_R"));

bool FMemoriaHeroFinish::HasDedicatedLids(const USkeletalMesh* Mesh)
{
    return Mesh && Mesh->FindMorphTarget(BlinkLeft) && Mesh->FindMorphTarget(BlinkRight);
}

float FMemoriaHeroFinish::BlinkWeight(double ElapsedSeconds, uint32 StableSeed, bool bSuppressed)
{
    if (bSuppressed || !FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.) return 0.f;
    // A repeatable irregular sequence avoids synchronized metronome blinking.
    // Eight intervals are bounded, even after many hours of game time.
    double Intervals[8], Period = 0.;
    uint32 Hash = StableSeed ^ 0x9E3779B9u;
    for (int32 I = 0; I < 8; ++I)
    {
        Hash ^= Hash << 13; Hash ^= Hash >> 17; Hash ^= Hash << 5;
        Intervals[I] = 3.05 + double(Hash % 1701u) / 1000.;
        Period += Intervals[I];
    }
    double Phase = FMath::Fmod(ElapsedSeconds, Period);
    for (double Interval : Intervals)
    {
        if (Phase >= Interval) { Phase -= Interval; continue; }
        // First close after the resting interval. Smooth closure .075s,
        // held contact .025s, gentle opening .115s; never scale an eyeball.
        constexpr double Duration = .215;
        const double Local = Phase - (Interval - Duration);
        if (Local < 0.) return 0.f;
        if (Local < .075)
        {
            const float T = float(Local / .075);
            return T * T * (3.f - 2.f * T);
        }
        if (Local < .100) return 1.f;
        const float T = FMath::Clamp(float((Local - .100) / .115), 0.f, 1.f);
        return 1.f - T * T * (3.f - 2.f * T);
    }
    return 0.f;
}

bool FMemoriaHeroFinish::ApplyBlink(USkeletalMeshComponent* Mesh, float Weight, bool bSuppressed)
{
    if (!Mesh || !HasDedicatedLids(Mesh->GetSkeletalMeshAsset())) return false;
    const float Value = bSuppressed || !FMath::IsFinite(Weight) ? 0.f : FMath::Clamp(Weight, 0.f, 1.f);
    Mesh->SetMorphTarget(BlinkLeft, Value, false);
    Mesh->SetMorphTarget(BlinkRight, Value, false);
    return true;
}

void FMemoriaHeroFinish::ClearBlink(USkeletalMeshComponent* Mesh)
{
    if (!Mesh) return;
    Mesh->SetMorphTarget(BlinkLeft, 0.f, true);
    Mesh->SetMorphTarget(BlinkRight, 0.f, true);
}
