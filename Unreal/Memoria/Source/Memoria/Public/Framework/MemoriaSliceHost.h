#pragma once
#include "Framework/MemoriaGameMode.h"
#include "Framework/MemoriaPlayerController.h"
#include "MemoriaSliceHost.generated.h"
class UMemoriaNarrativeSubsystem;
class UMemoriaDevelopmentNarrativeWidget;

UCLASS()
class MEMORIA_API AMemoriaSliceGameMode : public AMemoriaGameMode
{
    GENERATED_BODY()
public:
    AMemoriaSliceGameMode();
    virtual void StartPlay() override;
};
UCLASS()
class MEMORIA_API AMemoriaSliceController : public AMemoriaPlayerController
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    UMemoriaDevelopmentNarrativeWidget* GetNarrativeWidget() const { return NarrativeWidget; }
protected:
    virtual void SetupInputComponent() override;
    virtual void Move(const FInputActionValue& Value) override;
    virtual void Confirm() override;
    virtual void Back() override;
    virtual void OpenModal() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Navigate(const FInputActionValue& Value);
    void ForwardConfirm(int32 OriginalIndex);
    UMemoriaNarrativeSubsystem* Host() const;
    UPROPERTY(Transient) TObjectPtr<UMemoriaDevelopmentNarrativeWidget> NarrativeWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaDevelopmentNarrativeWidget> StatusWidget;
    int32 LastRevision = INDEX_NONE;
};
