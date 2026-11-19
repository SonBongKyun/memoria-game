#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presentation/MemoriaDemoCompletion.h"
#include "MemoriaDemoCompletionWidget.generated.h"
class UMemoriaRunSubsystem;
class UMemoriaCheckpointSubsystem;

enum class EMemoriaDemoCompletionAction : uint8 { ReturnToField, Title };
DECLARE_DELEGATE_OneParam(FMemoriaDemoCompletionEvent, EMemoriaDemoCompletionAction);

// Opened explicitly from Pause after the existing completion card, so chapter playback is not interrupted.
UCLASS()
class MEMORIA_API UMemoriaDemoCompletionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(UMemoriaRunSubsystem* InRun, UMemoriaCheckpointSubsystem* InCheckpoint, const FVector2D& InPosition, bool bInKo);
    FMemoriaDemoCompletionEvent OnAction;
    void Navigate(int32 Delta);
    void Scroll(int32 Delta);
    void Confirm();
    void Back();
    void Activate(int32 Index);
    bool SaveCurrentProgress();
    void SetSourcePosition(const FVector2D& Position) { SourcePosition = Position; }
    FString ItemLabel(int32 Index) const;
    FString VisibleText() const;
    const FMemoriaDemoCompletionView& GetCompletionView() const { return View; }
    bool IsReviewing() const { return bReview; }
    bool HasSaveAttempt() const { return bSaveAttempted; }
    bool WasLastSaveSuccessful() const { return bSaveAttempted && bSaveSucceeded; }
    const FString& GetDiskStatus() const { return DiskStatus; }
    int32 GetSelected() const { return Selected; }
    int32 GetScroll() const { return ScrollLine; }
    static constexpr int32 ItemCount = 4;
    const TArray<FBox2D>& GetActionBounds() const { return Rows; }
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaCheckpointSubsystem> Checkpoint;
    FMemoriaDemoCompletionView View;
    FVector2D SourcePosition = FVector2D::ZeroVector;
    FString DiskStatus;
    bool bKo = true, bReview = false, bSaveAttempted = false, bSaveSucceeded = false;
    int32 Selected = 0, ScrollLine = 0;
    mutable int32 MaxScroll = 0;
    mutable TArray<FBox2D> Rows;
    int32 RowAt(const FGeometry& Geometry, const FVector2D& Screen) const;
};
