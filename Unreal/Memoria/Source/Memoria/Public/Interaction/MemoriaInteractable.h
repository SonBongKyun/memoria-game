#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MemoriaInteractable.generated.h"
class APawn;
UINTERFACE(MinimalAPI)
class UMemoriaInteractable : public UInterface { GENERATED_BODY() };
class MEMORIA_API IMemoriaInteractable
{
    GENERATED_BODY()
public:
    virtual bool CanInteract(const APawn& Pawn) const = 0;
    virtual FString InteractionPrompt() const = 0;
    virtual bool Interact(APawn& Pawn) = 0;
};
