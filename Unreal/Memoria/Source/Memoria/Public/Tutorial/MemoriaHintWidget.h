#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaHintWidget.generated.h"
class UMemoriaTutorialSubsystem;
class UTexture2D;

// S323: tutorial_hints.gd's panel, painted over everything but the game over screen. The source's banner
// frame (ui_tutorial_hint_banner) across the top, a dark rounded panel inside it with the hint wrapped and
// centred, sliding down and fading in, then up and out. Layout in the source's 720p units.
UCLASS()
class MEMORIA_API UMemoriaHintWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Bind(UMemoriaTutorialSubsystem* InTutorial, bool bInKo);
    // The wrapped lines last painted (tests read what the player saw).
    const TArray<FString>& GetLines() const { return Lines; }
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    TWeakObjectPtr<UMemoriaTutorialSubsystem> Tutorial;
    bool bKo = true;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Banner;
    FSlateBrush BannerBrush;
    mutable TArray<FString> Lines;
};
