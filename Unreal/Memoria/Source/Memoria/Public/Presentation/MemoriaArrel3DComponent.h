#pragma once
#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "MemoriaArrel3DComponent.generated.h"
class USkeletalMesh;
// Gait bone indices resolved once per skeletal asset. A replacement rig may lack
// any of them; INDEX_NONE entries keep their reference pose instead of crashing.
struct FMemoriaArrelGaitBones
{
    int32 Pelvis=INDEX_NONE,Chest=INDEX_NONE,CapeUpper=INDEX_NONE,CapeMid=INDEX_NONE,CapeTip=INDEX_NONE;
    int32 Thigh[2]={INDEX_NONE,INDEX_NONE},Calf[2]={INDEX_NONE,INDEX_NONE},Foot[2]={INDEX_NONE,INDEX_NONE};
    int32 UpperArm[2]={INDEX_NONE,INDEX_NONE},Forearm[2]={INDEX_NONE,INDEX_NONE};
};
// Replaceable visual prototype; no movement, collision, equipment or story ownership.
UCLASS()
class MEMORIA_API UMemoriaArrel3DComponent : public UPoseableMeshComponent
{
    GENERATED_BODY()
public:
    bool InitializePrototype();
    void AdvanceLocomotion(const FVector& Displacement,float DeltaSeconds);
    FVector FocusPosition() const { return GetComponentLocation()+FVector(0,0,62); }
    float LocomotionWeight() const { return WalkWeight; }
    // Gait cycle in [0,1): the left foot plants at 0, the right at 0.5.
    float GaitPhase() const { return Phase; }
    const FMemoriaArrelGaitBones& GaitBones() const { return Bones; }
private:
    float Phase=0,WalkWeight=0,IdleTime=0;
    FMemoriaArrelGaitBones Bones;
    TWeakObjectPtr<USkeletalMesh> BonesFor;
    void CacheGaitBones(USkeletalMesh& Mesh);
};
