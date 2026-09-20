#pragma once
#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "MemoriaArrel3DComponent.generated.h"
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
private:
    float Phase=0,WalkWeight=0,IdleTime=0;
};
