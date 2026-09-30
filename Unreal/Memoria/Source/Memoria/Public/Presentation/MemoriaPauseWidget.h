#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MemoriaPauseWidget.generated.h"
class UMemoriaSettingsSubsystem;

// Menu rows first (their order is the row order); Language reports a language switch in Options.
enum class EMemoriaPauseAction : uint8 { Resume, Options, Save, Load, Journal, Codex, Achievements, Title, Quit, Language };
DECLARE_DELEGATE_OneParam(FMemoriaPauseActionEvent, EMemoriaPauseAction);

// S317: the ESC menu after pause_menu.gd, painted like the combat HUD: a dark veil, the amber-bordered
// panel on the right with the chapter card, and the menu. Achievements joined in S324, the Codex in S325 and
// the Journal in S327; the source's Artbook and Endings have no Unreal systems yet, so their rows are not listed.
// Options edits the same settings as the title; Quit asks first, as the source does.
UCLASS()
class MEMORIA_API UMemoriaPauseWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static constexpr int32 ItemCount = 9;
    static constexpr int32 OptionRowCount = 6; // master, bgm, sfx, fullscreen, language, back
    FMemoriaPauseActionEvent OnAction;
    void Configure(UMemoriaSettingsSubsystem* InSettings, bool bInCanSave, bool bInCanLoad, const FString& InInfo);
    void Navigate(int32 Direction);
    void Adjust(int32 Direction);
    void Confirm();
    // Escape: leaves Options or the quit question; returns false on the menu (the caller then resumes).
    bool Back();
    void Select(int32 Index);
    void SetNotice(const FString& Text) { Notice = Text; NoticeAge = 0.f; }
    void SetCanLoad(bool bValue) { bCanLoad = bValue; }
    void SetInfo(const FString& Text) { Info = Text; }
    int32 GetSelected() const { return Selected; }
    int32 GetOptionRow() const { return OptionRow; }
    bool IsOptionsOpen() const { return bOptionsOpen; }
    bool IsAskingQuit() const { return bAskQuit; }
    bool IsItemEnabled(int32 Index) const;
    const FString& GetNotice() const { return Notice; }
    FString ItemLabel(int32 Index) const;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaSettingsSubsystem> Settings;
    FString Info, Notice;
    FSlateBrush Backdrop, Slab;
    float NoticeAge = 99.f, Clock = 0.f;
    int32 Selected = 0, OptionRow = 0, QuitChoice = 1;
    bool bCanSave = false, bCanLoad = false, bOptionsOpen = false, bAskQuit = false;
    // Row rectangles from the last paint, in local units, for the mouse.
    mutable TArray<FBox2D> Rows;
    bool Ko() const;
    FString Loc(const TCHAR* En, const TCHAR* KoText) const { return Ko() ? FString(KoText) : FString(En); }
    void Activate(int32 Index);
    void ActivateOption(int32 Row, int32 Step);
    int32 RowAt(const FGeometry& Geometry, const FVector2D& Screen) const;
};
