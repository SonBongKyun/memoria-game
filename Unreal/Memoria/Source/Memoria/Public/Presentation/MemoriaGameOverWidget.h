#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaGameOverWidget.generated.h"

enum class EMemoriaGameOverAction : uint8 { StaggerOn, Load, Title };
DECLARE_DELEGATE_OneParam(FMemoriaGameOverActionEvent, EMemoriaGameOverAction);

// S318: the defeat screen after game_over.gd: the void backdrop under a dark veil, the red-bordered panel,
// "You fell." and three choices: Stagger On (HP 30%), Load Save, Return to Title. Keys and mouse.
UCLASS()
class MEMORIA_API UMemoriaGameOverWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static constexpr int32 ItemCount = 3;
    FMemoriaGameOverActionEvent OnAction;
    void Configure(bool bInCanLoad, bool bInKo);
    void Navigate(int32 Direction);
    void Confirm() { Activate(Selected); }
    void Select(int32 Index) { if (Index >= 0 && Index < ItemCount) Selected = Index; }
    int32 GetSelected() const { return Selected; }
    bool CanLoad() const { return bCanLoad; }
    FString ItemLabel(int32 Index) const;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    FSlateBrush Backdrop;
    float Age = 0.f;
    int32 Selected = 0;
    bool bCanLoad = false, bKo = true;
    mutable TArray<FBox2D> Rows;
    void Activate(int32 Index);
};
