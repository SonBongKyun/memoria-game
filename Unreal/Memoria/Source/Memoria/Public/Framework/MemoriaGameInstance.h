#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MemoriaGameInstance.generated.h"

// Engine bootstrap only. Gameplay authority lives in owned subsystem domains.
UCLASS()
class MEMORIA_API UMemoriaGameInstance : public UGameInstance
{
    GENERATED_BODY()
};
