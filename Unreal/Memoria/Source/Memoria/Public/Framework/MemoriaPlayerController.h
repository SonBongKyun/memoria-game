#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MemoriaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
struct FInputActionValue;

UCLASS()
class MEMORIA_API AMemoriaPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    bool IsModalOpen() const { return Modal != nullptr; }
    UUserWidget* GetModal() const { return Modal; }
    int32 GetBackDispatchCount() const { return BackDispatchCount; }
    int32 GetConfirmCount() const { return ConfirmCount; }
    int32 GetModalTransitionCount() const { return ModalTransitionCount; }
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    // Authored editor assets in Phase 1B. Axis2D: right +X, up +Y in UE plane.
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputMappingContext> ExplorationContext;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputMappingContext> ModalContext;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputAction> ConfirmAction;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputAction> BackAction;
    UPROPERTY(EditDefaultsOnly, Category="Memoria|Input") TObjectPtr<UInputAction> MenuAction;
protected:
    virtual void Move(const FInputActionValue& Value);
    virtual void OpenModal();
    virtual void Back();
    virtual void Confirm();
    void PresentModal(UUserWidget* Widget);
    void DismissModal();
private:
    UPROPERTY(Transient) TObjectPtr<UUserWidget> Modal;
    int32 BackDispatchCount = 0;
    int32 ConfirmCount = 0;
    int32 ModalTransitionCount = 0;
};
