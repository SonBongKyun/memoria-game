#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaCharacterUpgradeS356Commandlet.generated.h"
// Imports immutable S356 revision packages with one explicit legacy FBX basis.
UCLASS()
class UMemoriaCharacterUpgradeS356Commandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaCharacterUpgradeS356Commandlet();
    virtual int32 Main(const FString& Params) override;
};