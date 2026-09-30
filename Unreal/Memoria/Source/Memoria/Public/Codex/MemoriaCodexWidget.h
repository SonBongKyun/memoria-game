#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaCodexWidget.generated.h"
class UMemoriaCodexSubsystem;
class UTexture2D;

// S325: codex.gd's screen, painted: the archive backdrop under a veil, the panel with "도감 / CODEX", its
// subtitle and record counts, the Bestiary and Memory Archive tabs, the list on the left (recorded foes with
// their defeat badges, then the unmet as "???"; or the memories with their stars and a burned mark) and the
// selected entry's detail on the right. Tab or the side arrows switch tabs, up and down choose, ESC closes.
UCLASS()
class MEMORIA_API UMemoriaCodexWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    struct FLine { FString Label; FLinearColor Color; bool bSelectable = true; };
    void Configure(UMemoriaCodexSubsystem* InCodex, bool bInKo);
    void SetTab(int32 Tab);   // 0 bestiary, 1 memory archive
    int32 GetTab() const { return Tab; }
    void Move(int32 Delta);
    int32 GetSelected() const { return Selected; }
    // The list as shown (labels, including the progress and divider lines) and the selected entry's detail.
    TArray<FLine> ListLines() const;
    FString DetailTitle() const;
    FString DetailBody() const;
    FString StatusText() const;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    TWeakObjectPtr<UMemoriaCodexSubsystem> Codex;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Backdrop;
    FSlateBrush BackdropBrush;
    bool bKo = true;
    int32 Tab = 0, Selected = 0;
    mutable int32 FirstLine = 0;
    FString Loc(const TCHAR* En, const TCHAR* Ko) const { return bKo ? FString(Ko) : FString(En); }
    FString EnemyName(const FString& Name) const;
    // The list index of each selectable line, in order.
    TArray<int32> SelectableLines() const;
};
