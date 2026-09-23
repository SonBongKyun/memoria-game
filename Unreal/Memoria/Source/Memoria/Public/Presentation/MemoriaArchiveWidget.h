#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Presentation/MemoriaArchiveView.h"
#include "MemoriaArchiveWidget.generated.h"

class UMemoriaArchiveWidget;
class UTexture2D;
class UMemoriaRunSubsystem;
class UMemoriaPlayerMemoryDomain;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UImage;
class UScrollBox;
DECLARE_DELEGATE(FMemoriaArchiveClose);
DECLARE_DELEGATE_TwoParams(FMemoriaArchiveKeyGesture,const FKey&,EInputEvent);

UCLASS()
class MEMORIA_API UMemoriaArchiveButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaArchiveButton() { InitIsFocusable(false); }
    UPROPERTY() TObjectPtr<UMemoriaArchiveWidget> Owner;
    int32 Index=0, Role=0;
    UFUNCTION() void Activate();
};

// Read-only presentation; memory/run authority remains in existing domains.
UCLASS()
class MEMORIA_API UMemoriaArchiveWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FMemoriaArchiveClose OnClose;
    FMemoriaArchiveKeyGesture OnConsumedKey;
    void BindRun(UMemoriaRunSubsystem* InRun);
    void Select(int32 Index);
    void SetFilter(int32 Grade);
    void Navigate(int32 Direction);
    void CycleFilter(int32 Direction);
    void RequestClose();
    FString VisibleText() const;
    const FMemoriaArchiveView& GetView() const { return View; }
    const FString& GetSelectedId() const { return SelectedId; }
    int32 GetFilter() const { return FilterGrade; }
    UTexture2D* DisplayedArtwork() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Key) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Key) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaPlayerMemoryDomain> Memory;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Heading;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Summary;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailState;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailBody;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Hint;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Cards;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> Filters;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> DetailScroll;
    UPROPERTY(Transient) TObjectPtr<UImage> Artwork;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaArchiveButton>> CardButtons;
    FMemoriaArchiveView View;
    FString SelectedId;
    int32 FilterGrade=-1;
    bool bDirty=false;
    void Refresh();
    void Draw();
    void UnbindRun();
};
