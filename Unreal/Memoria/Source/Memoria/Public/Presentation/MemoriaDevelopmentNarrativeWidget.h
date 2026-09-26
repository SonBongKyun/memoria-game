#pragma once
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "MemoriaDevelopmentNarrativeWidget.generated.h"
class UTextBlock;
class URichTextBlock;
class UTexture2D;
class UDataTable;
class UBorder;
class UImage;
class UCanvasPanel;
class UVerticalBox;
class UScrollBox;
class UScaleBox;
class USizeBox;
class UMemoriaShopWidget;
class UMemoriaDevelopmentNarrativeWidget;
DECLARE_DELEGATE_OneParam(FMemoriaPresentationConfirm, int32);
// Observable state of the vn_scene.gd presentation, for tests and captures.
struct FMemoriaPresentationProbe
{
    float FlashAlpha = 0.f, BackdropScale = 1.f, BackdropOpacity = 1.f, LedgerAlpha = 0.f, ChromaAlpha = 0.f, EmberAlpha = 0.f;
    FVector2D Nudge = FVector2D::ZeroVector;
    FString Motion, LedgerText, ActiveSide;
    bool bDistortedStyle = false, bSystemStyle = false, bStoryFrame = false, bChoiceFrame = false, bTyping = false;
    int32 TypedChars = 0, TotalChars = 0;
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
    UFUNCTION() void Hover();
};
// Native illustrated presentation after vn_scene.gd: cover-filled CG with cinematic
// layers, left/right portraits, the memory frame dialogue box and the archive choice
// frame. Input still submits original source choice IDs.
UCLASS()
class MEMORIA_API UMemoriaDevelopmentNarrativeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Display(const FMemoriaNarrativeView& InView);
    void Navigate(int32 Direction);
    // A line still typing completes first (vn_scene.gd); otherwise confirms.
    void ConfirmIntent();
    void Choose(int32 Index);
    void HoverChoice(int32 Index);
    int32 SelectedOriginalIndex() const;
    FString VisibleText() const;
    UTexture2D* DisplayedBackdrop() const;
    UTexture2D* DisplayedPortrait() const;
    UMemoriaShopWidget* GetShopWidget() const { return ShopWidget; }
    FMemoriaPresentationConfirm OnConfirm;
    // Deterministic cue clock; NativeTick feeds it game time.
    void AdvancePresentation(float DeltaSeconds);
    FMemoriaPresentationProbe GetPresentationProbe() const;
    // The typewriter is off under automation unless a test turns it on.
    void SetTypewriter(bool bEnabled) { TypewriterOverride = bEnabled; }
    bool IsTyping() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Unhandled(); }
    virtual FNavigationReply NativeOnNavigation(const FGeometry&, const FNavigationEvent&, const FNavigationReply&) override { return FNavigationReply::Escape(); }
private:
    // Development, status and pause panel (unchanged layout).
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Speaker;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Location;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Hint;
    UPROPERTY(Transient) TObjectPtr<UBorder> NarrativePanel;
    UPROPERTY(Transient) TObjectPtr<UImage> Portrait;
    UPROPERTY(Transient) TObjectPtr<UBorder> PortraitFrame;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> BodyScroll;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ChoiceScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Choices;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChoiceNote;
    // Illustration and its cinematic layers.
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Illustration;
    UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UImage> BackdropPrevious;
    UPROPERTY(Transient) TObjectPtr<UScaleBox> BackdropFit;
    UPROPERTY(Transient) TObjectPtr<UImage> ChromaRed;
    UPROPERTY(Transient) TObjectPtr<UImage> ChromaBlue;
    UPROPERTY(Transient) TObjectPtr<UScaleBox> ChromaRedFit;
    UPROPERTY(Transient) TObjectPtr<UScaleBox> ChromaBlueFit;
    UPROPERTY(Transient) TObjectPtr<UImage> Ember;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Cinema;
    UPROPERTY(Transient) TObjectPtr<UBorder> Flash;
    // Story layer: portraits, memory frame dialogue box, archive choice frame.
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Story;
    UPROPERTY(Transient) TObjectPtr<UImage> PortraitImage[2];
    UPROPERTY(Transient) TObjectPtr<UBorder> PortraitPlate[2];
    UPROPERTY(Transient) TObjectPtr<UImage> PortraitShadow[2];
    UPROPERTY(Transient) TObjectPtr<UImage> PortraitVignette[2];
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> DialogueLayer;
    UPROPERTY(Transient) TObjectPtr<UBorder> NameFill;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StoryName;
    UPROPERTY(Transient) TObjectPtr<URichTextBlock> StoryText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StoryNext;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StoryLocation;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StoryHint;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ChoiceLayer;
    UPROPERTY(Transient) TObjectPtr<UImage> ChoiceFrameArt;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChoiceTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChoiceHintText;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ChoiceSlots;
    UPROPERTY(Transient) TObjectPtr<UDataTable> TextStyles;
    UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Gradients;
    UPROPERTY(Transient) TObjectPtr<UMemoriaShopWidget> ShopWidget;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaNarrativeChoiceButton>> Buttons;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Ledger;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerBody;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LedgerThread;
    FMemoriaNarrativeView View;
    int32 Selection = 0;
    void Refresh();
    void RefreshStory(bool bChoosing);
    void BuildChoices(bool bFramed);
    void StyleChoices();
    bool IsStoryMode() const;
    FString StoryBody() const;
    // Cue timeline (seconds since the step was shown) and the CG it started.
    FString ActiveCue, Motion;
    float CueAge = 0.f, CueDelay = 0.f, CrossFade = 0.f, FlashDuration = 0.f, NudgeStrength = 0.f;
    FLinearColor FlashColor = FLinearColor::Transparent;
    FVector2D Pan = FVector2D::ZeroVector;
    bool bPendingCrossFade = false, bSeenView = false;
    int32 ShownLedger = 0, ShownBurn = 0;
    float LedgerAge = -1.f, MotionAge = 0.f, BurnAge = -1.f, Clock = 0.f;
    // Portrait slots (vn_scene.gd _left_portrait_id/_right_portrait_id); 0 left, 1 right.
    UPROPERTY(Transient) TObjectPtr<UTexture2D> SlotTexture[2];
    int32 ActiveSlot = INDEX_NONE;
    float SlotAge[2] = {0.f, 0.f};
    // Typewriter (TYPEWRITER_SPEED 0.025 s per character).
    TOptional<bool> TypewriterOverride;
    float Typed = 0.f;
    FString ShownBody;
    FMemoriaPresentationProbe Probe;
    void StartCues();
    void ApplyCues();
    void ApplyStoryText();
};
