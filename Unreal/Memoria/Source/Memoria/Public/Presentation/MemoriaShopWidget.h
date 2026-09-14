#pragma once
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Shop/MemoriaShopSubsystem.h"
#include "MemoriaShopWidget.generated.h"
class UTextBlock;
class UImage;
class UVerticalBox;
class UMemoriaShopWidget;
UCLASS()
class MEMORIA_API UMemoriaShopRowButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaShopRowButton() { InitIsFocusable(false); }
    int32 RowIndex = INDEX_NONE;
    UPROPERTY(Transient) TObjectPtr<UMemoriaShopWidget> Shop;
    UFUNCTION() void Preview();
};
UCLASS()
class MEMORIA_API UMemoriaShopWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Display(const FMemoriaShopView& InView);
    void Navigate(int32 Direction);
    void Preview(int32 Index);
    FString VisibleText() const;
    bool HasArtwork() const;
    int32 SelectedRow() const { return Selection; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY(Transient) TObjectPtr<UImage> BackdropImage;
    UPROPERTY(Transient) TObjectPtr<UImage> PortraitImage;
    FMemoriaShopView View;
    int32 Selection = INDEX_NONE;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Caption;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Grains;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Detail;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Rows;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaShopRowButton>> Buttons;
    void Refresh();
};
