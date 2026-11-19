#pragma once
#include "CoreMinimal.h"
#include "BonePose.h"

// Value-only game-thread result, copied into the animation proxy before any
// parallel evaluation. No UObject, random state, save field or gameplay timer.
struct MEMORIA_API FMemoriaIdlePoseS362
{
    FRotator Chest=FRotator::ZeroRotator,Neck=FRotator::ZeroRotator,Head=FRotator::ZeroRotator;
    FRotator LeftShoulder=FRotator::ZeroRotator,RightShoulder=FRotator::ZeroRotator;
    bool IsIdentity() const;
};
namespace MemoriaCharacterPersonalityS362
{
    // Excludes Arrel/Elia and every foe/unknown id. Existing combat and authored
    // hero motion are independent owners; only named/ambient human idle is added.
    MEMORIA_API bool Supports(const FString& Id);
    MEMORIA_API FMemoriaIdlePoseS362 Sample(const FString& Id,double Age,float WalkWeight,bool bActing,bool bCombatAttention,bool bReduceMotion);
    // Rotation-only in mesh component space. Never edits root, pelvis, legs,
    // translations, scales, clips, phase, facing, hands or prop attachment data.
    MEMORIA_API void Apply(FCompactPose& Pose,const FMemoriaIdlePoseS362& Offsets);
}
