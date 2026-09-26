#pragma once
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "MemoriaDevelopmentNarrativeWidget.generated.h"
class UTextBlock;
class UTexture2D;
class UBorder;
class UImage;
class UCanvasPanel;
class UVerticalBox;
class UScrollBox;
class UMemoriaShopWidget;
class UMemoriaDevelopmentNarrativeWidget;
class UScaleBox;
class USizeBox;
DECLARE_DELEGATE_OneParam(FMemoriaPresentationConfirm, int32);
// Observable state of the vn_scene.gd one-shot cues, for tests and captures.
struct FMemoriaPresentationProbe
{
    float FlashAlpha = 0.f, BackdropScale = 1.f, BackdropOpacity = 1.f, LedgerAlpha = 0.f;
    FVector2D Nudge = FVector2D::ZeroVector;
    FString Motion, LedgerText;
    bool bDistortedStyle = false, bSystemStyle = false;
};
UCLASS()
class MEMORIA_API UMemoriaNarrativeChoiceButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaNarrativeChoiceButton() { InitIsFocusable(false); }
    UPROPERTY(Transient) TObjectPtr<UMemoriaDevelopmentNarrativeWidget> Owner;
    int32 ChoiceIndex = 0;
    UFUNCTION() void Choose();
};
// Native illustrated presentation. Input still submits original source choice IDs.
UCLASS()
class MEMORIA_API UMemoriaDevelopmentNarrativeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Display(const FMemoriaNarrativeView& InView);
    void Navigate(int32 Direction);
    void ConfirmIntent();
    void Choose(int32 Index);
    int32 SelectedOriginalIndex() const;
    FString VisibleText() const;
    UTexture2D* DisplayedBackdrop() const;
    UTexture2D* DisplayedPortrait() const;
    UMemoriaShopWidget* GetShopWidget() const { return ShopWidget; }
    FMemoriaPresentationConfirm OnConfirm;
    // Deterministic cue clock; NativeTick feeds it real frame time.
    void AdvancePresentation(float DeltaSeconds);
    FMemoriaPresentationProbe GetPresentationProbe() const;
protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Unhandled(); }
    virtual FNavigationReply NativeOnNavigation(const FGeometry&, const FNavigationEvent&, const FNavigationReply&) override { return FNavigationReply::Escape(); }
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Speaker;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Location;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Hint;
    UPROPERTY(Transient) TObjectPtr<UBorder> NarrativePanel;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Illustration;
    UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UImage> BackdropPrevious;
    UPROPERTY(Transient) TObjectPtr<UScaleBox> BackdropFit;
    UPROPERTY(Transient) TObjectPtr<UBorder> Flash;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChoiceNote;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Ledger;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerBody;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerThread;
    UPROPERTY(Transient) TObjectPtr<UImage> Portrait;
    UPROPERTY(Transient) TObjectPtr<UBorder> PortraitFrame;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> BodyScroll;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ChoiceScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Choices;
    UPROPERTY(Transient) TObjectPtr<UMemoriaShopWidget> ShopWidget;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaNarrativeChoiceButton>> Buttons;
    FMemoriaNarrativeView View;
    int32 Selection = 0;
    void Refresh();
    // Cue timeline (seconds since the step was shown) and the CG it started.
    FString ActiveCue, Motion;
    float CueAge = 0.f, CueDelay = 0.f, CrossFade = 0.f, FlashDuration = 0.f, NudgeStrength = 0.f;
    FLinearColor FlashColor = FLinearColor::Transparent;
    FVector2D Pan = FVector2D::ZeroVector;
    bool bPendingCrossFade = false, bSeenView = false;
    int32 ShownLedger = 0;
    float LedgerAge = -1.f, MotionAge = 0.f;
    FMemoriaPresentationProbe Probe;
    void StartCues();
    void ApplyCues();
};
