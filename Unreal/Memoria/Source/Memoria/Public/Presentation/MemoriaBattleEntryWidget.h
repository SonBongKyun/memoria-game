#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Battle/MemoriaBattleEntrySubsystem.h"
#include "MemoriaBattleEntryWidget.generated.h"
class UMemoriaBattleEntryWidget;
class UTexture2D;
class UTextBlock;
class UImage;
class UProgressBar;
class UBorder;
class UScrollBox;
DECLARE_DELEGATE_OneParam(FMemoriaBattleEntryFlee,uint64);
DECLARE_DELEGATE_TwoParams(FMemoriaBattleEntryKeyGesture,const FKey&,EInputEvent);

UCLASS()
class MEMORIA_API UMemoriaBattleEntryButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaBattleEntryButton(){InitIsFocusable(false);}
    UPROPERTY() TObjectPtr<UMemoriaBattleEntryWidget> Owner;
    UFUNCTION() void Activate();
};

// Illustration-based entry presentation. The battle subsystem owns all state and return timing.
UCLASS()
class MEMORIA_API UMemoriaBattleEntryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FMemoriaBattleEntryFlee OnFlee;
    FMemoriaBattleEntryKeyGesture OnConsumedKey;
    void BindBattle(UMemoriaBattleEntrySubsystem* InBattle);
    void ConfirmIntent();
    void Display(const FMemoriaBattleEntryView& InView);
    const FMemoriaBattleEntryView& GetView() const {return View;}
    FString VisibleText() const;
    UTexture2D* DisplayedBackdrop() const;
    UTexture2D* DisplayedPlayerArtwork() const;
    UTexture2D* DisplayedEnemyArtwork() const;
    UTexture2D* DisplayedAllyArtwork() const;
    bool IsFleeEnabled() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements,int32 LayerId,const FWidgetStyle& Style,bool bParentEnabled) const override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaBattleEntrySubsystem> Battle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Heading;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Turn;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerHealth;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AllyName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EnemyName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EnemyHealth;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveBody;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Modifier;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BattleLog;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BoundaryNote;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FleeLabel;
    UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UImage> PlayerArt;
    UPROPERTY(Transient) TObjectPtr<UImage> AllyArt;
    UPROPERTY(Transient) TObjectPtr<UImage> EnemyArt;
    UPROPERTY(Transient) TObjectPtr<UBorder> AllyFrame;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> PlayerBar;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> EnemyBar;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> LogScroll;
    UPROPERTY(Transient) TObjectPtr<UMemoriaBattleEntryButton> FleeButton;
    FMemoriaBattleEntryView View;
    bool bFleeRequested=false;
    float Age=0.f,ReturnAge=0.f;
    void Refresh();
    void Draw();
    void UnbindBattle();
};
