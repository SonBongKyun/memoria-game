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
class UMemoriaTitleWidget;
class UMemoriaPauseWidget;
class UMemoriaGameOverWidget;
class UMemoriaHintWidget;
class UMemoriaAchievementPopupWidget;
class UMemoriaAchievementsWidget;
class UMemoriaCodexWidget;
enum class EMemoriaGameOverAction : uint8;
enum class EMemoriaPauseAction : uint8;
enum class EMemoriaTitleAction : uint8;

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
    UMemoriaTitleWidget* GetTitleWidget() const { return TitleWidget; }
    // S317: the ESC pause menu (pause_menu.gd) while exploring.
    UMemoriaPauseWidget* GetPauseWidget() const { return PauseWidget; }
    bool OpenPause();
    void ClosePause();
    // S318: the defeat screen (game_over.gd) once Arrel's fall has played out.
    UMemoriaGameOverWidget* GetGameOverWidget() const { return GameOverWidget; }
    // S323: the tutorial hint on screen (tutorial_hints.gd).
    UMemoriaHintWidget* GetHintWidget() const { return HintWidget; }
    // S324: the achievement popup and the pause menu's achievements list.
    UMemoriaAchievementPopupWidget* GetAchievementPopup() const { return AchievementPopup; }
    UMemoriaAchievementsWidget* GetAchievementsWidget() const { return AchievementsWidget; }
    // S325: the codex (도감) from the pause menu.
    UMemoriaCodexWidget* GetCodexWidget() const { return CodexWidget; }
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
    UPROPERTY(Transient) TObjectPtr<UMemoriaTitleWidget> TitleWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaPauseWidget> PauseWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaGameOverWidget> GameOverWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaHintWidget> HintWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaAchievementPopupWidget> AchievementPopup;
    UPROPERTY(Transient) TObjectPtr<UMemoriaAchievementsWidget> AchievementsWidget;
    UPROPERTY(Transient) TObjectPtr<UMemoriaCodexWidget> CodexWidget;
    void CloseCodex();
    void UpdateAchievements();
    void CloseAchievements();
    void UpdateHints(class UMemoriaFieldCombatSubsystem* Combat);
    void GameOverAction(EMemoriaGameOverAction Action);
    void CloseGameOver();
    void LoadNewest();
    void PauseAction(EMemoriaPauseAction Action);
    FString PauseInfo() const;
    void TitleAction(EMemoriaTitleAction Action);
    void ClearTitle();
    FVector CursorOnFloor() const;
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
