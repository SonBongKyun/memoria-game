#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MemoriaInteractionComponent.generated.h"
class APawn;
UCLASS()
class MEMORIA_API UMemoriaInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void UpdateTarget(APawn* Pawn);
    bool Interact(APawn* Pawn);
    AActor* GetTarget() const { return Target.Get(); }
    FString GetPrompt() const;
private:
    TWeakObjectPtr<AActor> Target;
};
