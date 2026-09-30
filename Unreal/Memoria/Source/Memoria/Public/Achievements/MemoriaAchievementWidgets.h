#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaAchievementWidgets.generated.h"
class UMemoriaAchievementSubsystem;
class UTexture2D;

// S324: achievement_manager.gd's popup: top right, a dark panel with a gold border, "ACHIEVEMENT UNLOCKED",
// the title and the description, sliding down from above the screen, holding, then leaving. 720p units.
UCLASS()
class MEMORIA_API UMemoriaAchievementPopupWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Bind(UMemoriaAchievementSubsystem* InAchievements) { Achievements = InAchievements; }
    const FString& GetShownTitle() const { return ShownTitle; }
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    TWeakObjectPtr<UMemoriaAchievementSubsystem> Achievements;
    mutable FString ShownTitle;
};

// pause_menu.gd _show_achievements_panel: the chronicle backdrop, a panel with "ACHIEVEMENTS (n / 38)", the
// completion line, and every achievement as a row (its glyph, the title or "???" while locked, and the
// description), scrolled with the arrows or the wheel; ESC closes it.
UCLASS()
class MEMORIA_API UMemoriaAchievementsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(UMemoriaAchievementSubsystem* InAchievements, bool bInKo);
    void Scroll(int32 Rows);
    int32 GetFirstRow() const { return FirstRow; }
    int32 GetVisibleRows() const { return VisibleRows; }
    FString HeaderText() const;
    FString ProgressText() const;
    // A row's shown title: the achievement's, or "???" while locked.
    FString RowTitle(int32 Index) const;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    TWeakObjectPtr<UMemoriaAchievementSubsystem> Achievements;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Backdrop;
    FSlateBrush BackdropBrush;
    bool bKo = true;
    int32 FirstRow = 0;
    mutable int32 VisibleRows = 8;
};
