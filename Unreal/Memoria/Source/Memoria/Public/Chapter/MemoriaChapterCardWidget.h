#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MemoriaChapterCardWidget.generated.h"

// S320: MapEffects.show_chapter_title and the chapter-complete card, painted over the field: a dark band
// across the screen with the chapter, its name and its subtitle, fading in and out.
UCLASS()
class MEMORIA_API UMemoriaChapterCardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Show(const FString& InEyebrow, const FString& InTitle, const FString& InSubtitle, float InSeconds);
    bool IsShowing() const { return Age < Seconds; }
    const FString& GetTitle() const { return Title; }
    const FString& GetEyebrow() const { return Eyebrow; }
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    FString Eyebrow, Title, Subtitle;
    float Age = 99.f, Seconds = 0.f;
};
