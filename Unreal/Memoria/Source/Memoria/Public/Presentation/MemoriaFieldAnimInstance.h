#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MemoriaFieldAnimInstance.generated.h"
class UAnimSequence;

// Game-thread inputs are copied by the proxy before parallel pose evaluation.
UCLASS(Transient)
class MEMORIA_API UMemoriaFieldAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Idle;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Walk;
    float Age = 0.f, Phase = 0.f, Weight = 0.f;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
