#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MemoriaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class MEMORIA_API AMemoriaPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    // Authored editor assets in Phase 1B. Axis2D: right +X, up +Y in UE plane.
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputMappingContext> ExplorationContext;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputAction> MoveAction;
private:
    void Move(const FInputActionValue& Value);
};
