#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MemoriaTitleWidget.generated.h"

class UMemoriaTitleWidget;
class UMemoriaSettingsSubsystem;
class UBorder;
class UCanvasPanel;
class UImage;
class UScaleBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

enum class EMemoriaTitleAction : uint8 { NewGame, Continue, Options, Quit };
DECLARE_DELEGATE_OneParam(FMemoriaTitleActionEvent, EMemoriaTitleAction);
DECLARE_DELEGATE_TwoParams(FMemoriaTitleKeyGesture, const FKey&, EInputEvent);

UCLASS()
class MEMORIA_API UMemoriaTitleButton : public UButton
{
    GENERATED_BODY()
public:
    UMemoriaTitleButton() { InitIsFocusable(false); }
    UPROPERTY() TObjectPtr<UMemoriaTitleWidget> Owner;
    // Role 0: menu item; 1: options row; 2: options value down; 3: options value up.
    int32 Index = 0, Role = 0;
    UFUNCTION() void Activate();
    UFUNCTION() void Hover();
};

// main.gd title composition: painterly key art with rift glow, ash, vignette and letterbox; the
// MEMORIA wordmark stack on the left and the numbered menu panel on the right; and a compact
// Options panel for the ported settings. The source's Aftermath preview (Part 2) is not listed.
UCLASS()
class MEMORIA_API UMemoriaTitleWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static constexpr int32 ItemCount = 4;
    static constexpr int32 OptionRowCount = 6; // master, bgm, sfx, fullscreen, language, back
    FMemoriaTitleActionEvent OnAction;
    FMemoriaTitleKeyGesture OnConsumedKey;
    void Configure(bool bInCanContinue, UMemoriaSettingsSubsystem* InSettings);
    void Navigate(int32 Direction);
    void Adjust(int32 Direction);
    void ConfirmIntent();
    // Escape: closes Options; on the menu it does nothing, as in the source.
    bool Back();
    void OpenOptions();
    void Select(int32 Index);
    void SelectOption(int32 Row);
    void ActivateItem(int32 Index);
    void ActivateOption(int32 Row, int32 Step);
    // Deterministic presentation clock for tests; the widget otherwise advances with world time.
    void AdvancePresentation(float Seconds);
    int32 GetSelected() const { return Selected; }
    int32 GetOptionRow() const { return OptionRow; }
    bool IsOptionsOpen() const { return bOptionsOpen; }
    bool IsItemEnabled(int32 Index) const { return Index != 1 || bCanContinue; }
    bool IsIntroComplete() const { return Clock >= IntroSeconds; }
    FString VisibleText() const;
    UTexture2D* DisplayedArtwork() const;
    static constexpr float IntroSeconds = 1.4f;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Key) override;
    virtual FReply NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Key) override;
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaSettingsSubsystem> Settings;
    UPROPERTY(Transient) TObjectPtr<UScaleBox> BackdropFit;
    UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UImage> RiftGlow;
    UPROPERTY(Transient) TObjectPtr<UImage> TitleRail;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> TitleStack;
    UPROPERTY(Transient) TObjectPtr<UBorder> LetterboxTop;
    UPROPERTY(Transient) TObjectPtr<UBorder> LetterboxBottom;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> MenuChrome;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Menu;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaTitleButton>> Items;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ItemLabels;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> ItemAccents;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> OptionsLayer;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaTitleButton>> OptionRows;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptionValues;
    UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Ash;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Copy;
    struct FMote { FVector2D P, V; float Age = 0, Life = 9, Size = 4; };
    TArray<FMote> Motes;
    FRandomStream Rng{0x5ea1};
    FVector2D Parallax = FVector2D::ZeroVector, Area = FVector2D(1280, 720);
    float Clock = 0.f, Ambient = 0.f;
    TArray<float> Emphasis;
    int32 Selected = 0, OptionRow = 0;
    bool bCanContinue = false, bOptionsOpen = false;
    FString Loc(const TCHAR* En, const TCHAR* Ko) const;
    void Refresh();
    void Animate(float DeltaTime);
    int32 Step(int32 From, int32 Direction) const;
};
