#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaFieldHudWidget.generated.h"
class UTexture2D;

// What the field shows beside the status plate while Arrel explores.
struct FMemoriaFieldHudView
{
    FString Title, Subtitle;      // the place, on the ribbon at the top left
    FString Quest;                // the quest tracker's line, under the ribbon
    TArray<FString> Notices;      // toasts (rewards, memories, warnings), newest last
    FString Prompt;               // the interaction in reach ("Elia  |  E / A: talk")
    bool bKorean = true;
};

// S339: the field's own HUD in place of the development status box. Until now the place name, the control
// hint, the interaction prompt and every toast shared one plain panel at the top left (a compact view of the
// development narrative widget). Here the place stands on the source's toast frame
// (ui_notification_toast_frame), toasts stack under it as chips, the prompt sits over the command ribbon
// where the eye already is, and the control hint is a faint line at the bottom left.
UCLASS()
class MEMORIA_API UMemoriaFieldHudWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Display(const FMemoriaFieldHudView& InView) { View = InView; }
    const FMemoriaFieldHudView& GetView() const { return View; }
    bool HasRibbon() const { return Toast != nullptr; }
    // S341: the quick items' tray at the bottom right: each slot's icon, key and count.
    int32 GetIconCount() const { return Icons.Num(); }
protected:
    virtual void NativeConstruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    FMemoriaFieldHudView View;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Toast;
    FSlateBrush ToastBrush;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UTexture2D>> Icons;
    TMap<FString, FSlateBrush> IconBrushes;
};
