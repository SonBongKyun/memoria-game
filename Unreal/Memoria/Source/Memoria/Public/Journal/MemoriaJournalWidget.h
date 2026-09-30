#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaJournalWidget.generated.h"
class UMemoriaRunSubsystem;
class UTexture2D;
struct FMemoriaJournalEntry;

// S327: story_journal.gd's screen, painted: the journal backdrop under a veil, "일지 · 기억 운반자의 현장 기록",
// the run's summary (chapter, held, burned, illustrated), the Events / People / World / Choices tabs, the list
// (with the source's chapter headers) and the chosen entry's detail with its illustration when it has one.
// The source's Quests, Losses and Leads tabs wait for side quests, the world rewrite's loss records and curios.
UCLASS()
class MEMORIA_API UMemoriaJournalWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    struct FLine { FString Label; FLinearColor Color; int32 Entry = -1; };   // Entry -1: a header or note
    static constexpr int32 TabCount = 4;
    void Configure(const UMemoriaRunSubsystem* InRun, bool bInKo);
    void SetTab(int32 InTab);
    void CycleTab(int32 Delta) { SetTab((Tab + Delta + TabCount) % TabCount); }
    int32 GetTab() const { return Tab; }
    void Move(int32 Delta);
    int32 GetSelected() const { return Selected; }
    TArray<FLine> ListLines() const;
    const FMemoriaJournalEntry* SelectedEntry() const;
    FString DetailTitle() const;
    FString DetailBody() const;
    FString SummaryText() const;
    bool HasDetailArt() const;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    TWeakObjectPtr<const UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Backdrop;
    FSlateBrush BackdropBrush;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Art;   // the chosen entry's illustration, set as the choice changes
    FSlateBrush ArtBrush;
    FString Summary;   // computed as the journal opens (the game is paused while it is read)
    bool bKo = true;
    int32 Tab = 0, Selected = 0;
    mutable int32 FirstLine = 0;
    FString Loc(const TCHAR* En, const TCHAR* Ko) const { return bKo ? FString(Ko) : FString(En); }
    bool Flag(const FString& Id) const;
    const TArray<FMemoriaJournalEntry>& TabEntries() const;
    TArray<int32> SelectableLines() const;
    void UpdateArt();
};
