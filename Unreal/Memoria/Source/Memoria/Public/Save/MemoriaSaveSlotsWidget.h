#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "MemoriaSaveSlotsWidget.generated.h"

class UMemoriaSettingsSubsystem;
enum class EMemoriaSaveBrowserMode : uint8 { Save, Load };
DECLARE_DELEGATE_OneParam(FMemoriaSaveSlotEvent, int32);

// S362: explicit manual slots above the paused field. Controller input calls these same navigation methods.
// Loading only requests a slot; the host performs the correctly owned level/narrative restore.
UCLASS()
class MEMORIA_API UMemoriaSaveSlotsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FMemoriaSaveSlotEvent OnLoadRequested;
    FMemoriaSaveSlotEvent OnSaved;
    FSimpleDelegate OnClose;
    void Configure(UMemoriaCheckpointSubsystem* InCheckpoint, UMemoriaSettingsSubsystem* InSettings,
        EMemoriaSaveBrowserMode InMode, const FString& InMap, const FVector2D& InPosition, bool bInCanSave, float InTextScale = 1.f);
    void Refresh();
    void Navigate(int32 Direction);
    void Adjust(int32 Direction);
    void Select(int32 Row);
    void Confirm();
    // Cancels the overwrite question first; returns false when the host should close this browser.
    bool Back();
    int32 GetSelectedRow() const { return Selected; }
    int32 GetSelectedSlot() const { return RowToSlot(Selected); }
    int32 GetRowCount() const { return Mode == EMemoriaSaveBrowserMode::Save ? 4 : 5; }
    bool IsRowEnabled(int32 Row) const;
    bool IsAskingOverwrite() const { return bAskOverwrite; }
    bool WillReplaceOnConfirm() const { return bAskOverwrite && OverwriteChoice == 0; }
    EMemoriaSaveBrowserMode GetMode() const { return Mode; }
    const FString& GetNotice() const { return Notice; }
    void SetNotice(const FString& Text) { Notice = Text; }
    const TArray<FMemoriaSaveSlotSummary>& GetSummaries() const { return Summaries; }
    static FString RegionLabel(const FString& Map, bool bKo);
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaCheckpointSubsystem> Checkpoint;
    UPROPERTY(Transient) TObjectPtr<UMemoriaSettingsSubsystem> Settings;
    TArray<FMemoriaSaveSlotSummary> Summaries;
    FString Map, Notice;
    FVector2D Position = FVector2D::ZeroVector;
    EMemoriaSaveBrowserMode Mode = EMemoriaSaveBrowserMode::Save;
    int32 Selected = 0, OverwriteChoice = 1;
    bool bCanSave = false, bAskOverwrite = false;
    float TextScale = 1.f;
    mutable TArray<FBox2D> Rows;
    bool Ko() const;
    FString Loc(const TCHAR* En, const TCHAR* KoText) const { return Ko() ? FString(KoText) : FString(En); }
    int32 RowToSlot(int32 Row) const;
    int32 RowAt(const FGeometry& Geometry, const FVector2D& Screen) const;
    void WriteSelected(bool bConfirmed);
    FString StateLabel(EMemoriaSaveSlotState State) const;
};
