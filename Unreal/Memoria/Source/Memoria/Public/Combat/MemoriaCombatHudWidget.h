#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Combat/MemoriaHudLayoutS363.h"
#include "Combat/MemoriaDefenseFeedbackS364.h"
#include "Combat/MemoriaCombatFeedbackS365.h"
#include "MemoriaCombatHudWidget.generated.h"
class UMemoriaFieldCombatSubsystem;
// Read-only evidence of what the native picker actually laid out. No gameplay or saved state.
struct FMemoriaBurnPickerPaintProbe
{
    bool bDrawn = false, bClipped = false, bDescriptionClipped = false, bConsequenceClipped = false;
    FVector2D Viewport = FVector2D::ZeroVector;
    FBox2D PanelBounds{ForceInit}, DescriptionBounds{ForceInit}, ConsequenceBounds{ForceInit}, WarningBounds{ForceInit}, ControlsBounds{ForceInit};
    FString Description, AuthoredConsequence, PaintedConsequence;
    TArray<FString> DescriptionLines, ConsequenceLines;
    int32 Selected = 0, First = 0, Visible = 0, Pages = 0, Page = 0, InfoFontSize = 0;
    float TextScale = 1.f;
};

// S311 field combat HUD, painted directly: Arrel's HP, husk health over their heads, rising damage
// numbers, the defeat veil and the control hint while a fight is on. S312 adds the memory burn picker and
// the released burn's ring, flare and banner.
UCLASS()
class MEMORIA_API UMemoriaCombatHudWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Bind(UMemoriaFieldCombatSubsystem* InCombat);
    // Called synchronously by the authoritative picker state event, before Slate arranges its Overlay.
    void SyncViewportLayer();
    // True while anything is drawn (a fight, a wound, a popup or the defeat veil).
    bool IsShowing() const;
    // S339: the command ribbon (ui_battle_command_ribbon) under the HP gauge.
    bool HasRibbon() const { return Ribbon != nullptr; }
    const FMemoriaBurnPickerPaintProbe& GetBurnPickerPaintProbe() const { return PickerProbe; }
    const FMemoriaRibbonPaintProbe& GetRibbonPaintProbe() const { return RibbonProbe; }
    const FMemoriaDefensePaintProbe& GetDefensePaintProbe() const { return DefenseProbe; }
    const FMemoriaCombatFeedbackPaintProbe& GetCombatFeedbackPaintProbe() const { return FeedbackProbe; }
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    TWeakObjectPtr<UMemoriaFieldCombatSubsystem> Combat;
    mutable FMemoriaBurnPickerPaintProbe PickerProbe;
    mutable FMemoriaRibbonPaintProbe RibbonProbe;
    mutable FMemoriaDefensePaintProbe DefenseProbe;
    mutable FMemoriaCombatFeedbackPaintProbe FeedbackProbe;
    TMap<TWeakObjectPtr<AMemoriaFieldMonster>, MemoriaCombatFeedbackS365::FHealthTrail> HealthTrails;
    uint64 HealthTrailFrame = MAX_uint64;
    FDelegateHandle PickerVisibilityHandle;
    void UnbindPickerState();
    bool bPickerRaised = false, bChangingViewportLayer = false;
    int32 PreviousViewportOrder = 4;
    UPROPERTY(Transient) TObjectPtr<class UTexture2D> Ribbon;
    FSlateBrush RibbonBrush;
    void PaintBurnPicker(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, bool Ko) const;
};
