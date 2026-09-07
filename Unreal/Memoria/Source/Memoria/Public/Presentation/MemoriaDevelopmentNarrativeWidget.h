#pragma once
#include "Blueprint/UserWidget.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "MemoriaDevelopmentNarrativeWidget.generated.h"
class UTextBlock;
DECLARE_DELEGATE_OneParam(FMemoriaPresentationConfirm, int32);

// Temporary native UMG, shared rendering only. Selection retains source IDs.
UCLASS()
class MEMORIA_API UMemoriaDevelopmentNarrativeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Display(const FMemoriaNarrativeView& InView);
    void Navigate(int32 Direction);
    void ConfirmIntent();
    int32 SelectedOriginalIndex() const;
    FString VisibleText() const;
    FMemoriaPresentationConfirm OnConfirm;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    // Let Enhanced Input own keys; Slate must not consume arrow/A first.
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Unhandled(); }
    virtual FNavigationReply NativeOnNavigation(const FGeometry&, const FNavigationEvent&, const FNavigationReply&) override { return FNavigationReply::Escape(); }
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    FMemoriaNarrativeView View;
    int32 Selection = 0;
    void Refresh();
};
