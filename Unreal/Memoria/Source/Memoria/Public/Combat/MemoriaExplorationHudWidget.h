#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MemoriaExplorationHudWidget.generated.h"

// S316: the field status panel after exploration_hud.gd, painted at the top right while exploring:
// HP with the trailing ghost bar, the chapter and place, memories held and burned, grains and items.
// (The source's pulse, weapon and quest rows have no Unreal systems yet.)
UCLASS()
class MEMORIA_API UMemoriaExplorationHudWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool IsShowing() const;
    // The rows as last painted (tests read them back).
    const TArray<FString>& GetLines() const { return Lines; }
    float GetGhostHp() const { return GhostHp; }
    // S320: the place in the chapter row (Verdan Market unless a chapter map names its own).
    void SetPlace(const FString& En, const FString& Ko) { PlaceEn = En; PlaceKo = Ko; }
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    mutable TArray<FString> Lines;
    float GhostHp = -1.f;
    FString PlaceEn = TEXT("Verdan Market"), PlaceKo = TEXT("베르단 시장");
};
