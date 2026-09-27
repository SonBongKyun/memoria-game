#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MemoriaCombatHudWidget.generated.h"
class UMemoriaFieldCombatSubsystem;

// S311 field combat HUD, painted directly: Arrel's HP, husk health over their heads, rising damage
// numbers, the defeat veil and the control hint while a fight is on.
UCLASS()
class MEMORIA_API UMemoriaCombatHudWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Bind(UMemoriaFieldCombatSubsystem* InCombat) { Combat = InCombat; }
    // True while anything is drawn (a fight, a wound, a popup or the defeat veil).
    bool IsShowing() const;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;
};
