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
class UVerticalBox;
DECLARE_DELEGATE_ThreeParams(FMemoriaBattleAction,const FString&,const FString&,uint64);
DECLARE_DELEGATE_OneParam(FMemoriaBattleEntryFlee,uint64);
DECLARE_DELEGATE_TwoParams(FMemoriaBattleEntryKeyGesture,const FKey&,EInputEvent);

UCLASS()
class MEMORIA_API UMemoriaBattleEntryButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaBattleEntryButton(){InitIsFocusable(false);}
    UPROPERTY() TObjectPtr<UMemoriaBattleEntryWidget> Owner;
    int32 Index=0;
    bool bChoice=false;
    UFUNCTION() void Activate();
};

// Illustration-based entry presentation. The battle subsystem owns all state and return timing.
UCLASS()
class MEMORIA_API UMemoriaBattleEntryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FMemoriaBattleEntryFlee OnFlee;
    FMemoriaBattleAction OnAction;
    void ClickAction(int32 Index,bool bChoice);
    void Navigate(const FKey& Key);
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
    bool IsWitnessEnabled() const;
    bool IsVictoryCardVisible() const;
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
    float Age=0.f,ReturnAge=0.f,ImpactAge=2.f;
    int32 Selected=0,ChoiceSelected=0,PanelMode=0;
    int64 ImpactDamage=0;
    bool bHitPlayer=false;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaBattleEntryButton>> ActionButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ActionLabels;
    UPROPERTY(Transient) TObjectPtr<UBorder> ChoicePanel;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ChoiceList;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ImpactText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BurnCost;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Gauges;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> BreakBar;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> MomentumBar;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> LimitBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BreakLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MomentumLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LimitLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WitnessLabel;
    UPROPERTY(Transient) TObjectPtr<UBorder> EchoBand;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EchoText;
    UPROPERTY(Transient) TObjectPtr<UBorder> CueBand;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CueText;
    UPROPERTY(Transient) TObjectPtr<UBorder> VictoryCard;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> VictoryTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> VictoryGrade;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> VictoryBreakdown;
    float WitnessAge=2.f,VictoryAge=0.f;
    void DrawActions();
    void Refresh();
    void Draw();
    void UnbindBattle();
};
