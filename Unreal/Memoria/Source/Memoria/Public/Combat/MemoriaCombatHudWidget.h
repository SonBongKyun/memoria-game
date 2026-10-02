#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaCombatHudWidget.generated.h"
class UMemoriaFieldCombatSubsystem;

// S311 field combat HUD, painted directly: Arrel's HP, husk health over their heads, rising damage
// numbers, the defeat veil and the control hint while a fight is on. S312 adds the memory burn picker and
// the released burn's ring, flare and banner.
UCLASS()
class MEMORIA_API UMemoriaCombatHudWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Bind(UMemoriaFieldCombatSubsystem* InCombat) { Combat = InCombat; }
    // True while anything is drawn (a fight, a wound, a popup or the defeat veil).
    bool IsShowing() const;
    // S339: the command ribbon (ui_battle_command_ribbon) under the HP gauge.
    bool HasRibbon() const { return Ribbon != nullptr; }
protected:
    virtual void NativeConstruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;
    UPROPERTY(Transient) TObjectPtr<class UTexture2D> Ribbon;
    FSlateBrush RibbonBrush;
    void PaintBurnPicker(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, bool Ko) const;
};
