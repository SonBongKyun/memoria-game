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
DECLARE_DELEGATE_OneParam(FMemoriaPresentationConfirm, int32);
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
protected:
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
};
