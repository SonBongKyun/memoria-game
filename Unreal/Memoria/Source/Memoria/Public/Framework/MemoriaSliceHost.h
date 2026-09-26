#pragma once
#include "Framework/MemoriaGameMode.h"
#include "Framework/MemoriaPlayerController.h"
#include "Battle/MemoriaEncounterModel.h"
#include "MemoriaSliceHost.generated.h"
class UMemoriaNarrativeSubsystem;
class UMemoriaInteractionComponent;
class UMemoriaDevelopmentNarrativeWidget;
class UMemoriaArchiveWidget;
class UMemoriaBattleEntryWidget;

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
    AMemoriaSliceController();
    virtual void Tick(float DeltaSeconds) override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
    UMemoriaInteractionComponent* GetInteraction() const { return Interaction; }
    FString GetInteractionPrompt() const;
    void ToggleArchive();
    void CloseArchive();
    UMemoriaBattleEntryWidget* GetBattleWidget() const { return BattleWidget; }
    double GetEncounterPressure() const { return EncounterPressure; }
    const FMemoriaEncounterModel& GetEncounterModel() const { return Encounter; }
    UMemoriaArchiveWidget* GetArchiveWidget() const { return ArchiveWidget; }
    UMemoriaDevelopmentNarrativeWidget* GetNarrativeWidget() const { return NarrativeWidget; }
protected:
    virtual void SetupInputComponent() override;
    virtual void Move(const FInputActionValue& Value) override;
    virtual void Confirm() override;
    virtual void Back() override;
    virtual void OpenModal() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    FMemoriaEncounterModel Encounter;
    FMemoriaEncounterRng EncounterRng = FMemoriaEncounterRng::Random();
    bool bEncounterInitialized = false;
    double EncounterPressure = 0.;
    void RequestBattleFlee(uint64 Revision);
    void RequestBattleAction(const FString& Action,const FString& Id,uint64 Revision);
    void BattleReturned();
    void ClearBattleWidget();
    UPROPERTY(Transient) TObjectPtr<UMemoriaBattleEntryWidget> BattleWidget;
    TSet<FKey> ArchiveConsumedKeys;
    void TrackArchiveGesture(const FKey& Key,EInputEvent Event);
    void TrackConfirmGesture(const FKey& Key,EInputEvent Event);
    void Navigate(const FInputActionValue& Value);
    void ForwardConfirm(int32 OriginalIndex);
    UMemoriaNarrativeSubsystem* Host() const;
    void Cue(const TCHAR* Id) const;
    UPROPERTY(Transient) TObjectPtr<UMemoriaArchiveWidget> ArchiveWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaDevelopmentNarrativeWidget> NarrativeWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaDevelopmentNarrativeWidget> StatusWidget;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UMemoriaInteractionComponent> Interaction;
    FString LastPrompt; FString LastNotice;
    TSet<FKey> HeldConfirmKeys;
    bool bAwaitConfirmRelease = false;
    int32 LastRevision = INDEX_NONE;
};
