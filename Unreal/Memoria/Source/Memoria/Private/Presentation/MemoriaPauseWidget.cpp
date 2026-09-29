#include "Presentation/MemoriaPauseWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace
{
using MemoriaUiKit::Srgb;
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
// Border of a rectangle, Width thick.
void Frame(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Width, const FLinearColor& Color)
{
    Box(Elements, Layer, G, At, FVector2D(Size.X, Width), Color); Box(Elements, Layer, G, At + FVector2D(0, Size.Y - Width), FVector2D(Size.X, Width), Color);
    Box(Elements, Layer, G, At, FVector2D(Width, Size.Y), Color); Box(Elements, Layer, G, At + FVector2D(Size.X - Width, 0), FVector2D(Width, Size.Y), Color);
}
// Text anchored at its left (0), centre (.5) or right (1), vertically centred on Anchor.Y.
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Anchor, float Align, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D At = Anchor - FVector2D(Size.X * Align, Size.Y * .5);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1.5, 1.5)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .85f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
constexpr float RowH = 62.f, RowGap = 10.f;
}
bool UMemoriaPauseWidget::Ko() const { return !Settings || Settings->GetLocale() != TEXT("en"); }
void UMemoriaPauseWidget::Configure(UMemoriaSettingsSubsystem* InSettings, bool bInCanSave, bool bInCanLoad, const FString& InInfo)
{
    Settings = InSettings; bCanSave = bInCanSave; bCanLoad = bInCanLoad; Info = InInfo;
    Selected = 0; bOptionsOpen = bAskQuit = false; Notice.Reset(); NoticeAge = 99.f;
}
bool UMemoriaPauseWidget::IsItemEnabled(int32 Index) const
{ return Index == 2 ? bCanSave : Index == 3 ? bCanLoad : Index >= 0 && Index < ItemCount; }
FString UMemoriaPauseWidget::ItemLabel(int32 Index) const
{
    static const TCHAR* En[ItemCount] = {TEXT("Resume"), TEXT("Options"), TEXT("Save"), TEXT("Load"), TEXT("Return to Title"), TEXT("Quit Game")};
    static const TCHAR* KoText[ItemCount] = {TEXT("계속하기"), TEXT("옵션"), TEXT("저장"), TEXT("불러오기"), TEXT("타이틀로"), TEXT("게임 종료")};
    return Index >= 0 && Index < ItemCount ? Loc(En[Index], KoText[Index]) : FString();
}
void UMemoriaPauseWidget::Navigate(int32 Direction)
{
    if (bAskQuit) { QuitChoice = 1 - QuitChoice; return; }
    if (bOptionsOpen) { OptionRow = (OptionRow + Direction + OptionRowCount) % OptionRowCount; return; }
    // Focus skips disabled rows (Save outside exploration, Load without a save), wrapping.
    for (int32 N = 0, I = Selected; N < ItemCount; ++N) { I = (I + Direction + ItemCount) % ItemCount; if (IsItemEnabled(I)) { Selected = I; return; } }
}
void UMemoriaPauseWidget::Select(int32 Index)
{ if (!bOptionsOpen && !bAskQuit && IsItemEnabled(Index)) Selected = Index; }
void UMemoriaPauseWidget::Adjust(int32 Direction)
{
    if (bAskQuit) { QuitChoice = 1 - QuitChoice; return; }
    if (bOptionsOpen) ActivateOption(OptionRow, Direction);
}
void UMemoriaPauseWidget::Confirm()
{
    if (bAskQuit)
    {
        if (QuitChoice == 0) OnAction.ExecuteIfBound(EMemoriaPauseAction::Quit);
        else bAskQuit = false;
        return;
    }
    if (bOptionsOpen) { ActivateOption(OptionRow, 0); return; }
    Activate(Selected);
}
bool UMemoriaPauseWidget::Back()
{
    if (bAskQuit) { bAskQuit = false; return true; }
    if (bOptionsOpen) { bOptionsOpen = false; Selected = 1; return true; }
    return false;
}
void UMemoriaPauseWidget::Activate(int32 Index)
{
    if (!IsItemEnabled(Index)) return;
    Selected = Index;
    const auto Action = static_cast<EMemoriaPauseAction>(Index);
    if (Action == EMemoriaPauseAction::Options) { bOptionsOpen = true; OptionRow = 0; return; }
    // "Are you sure you want to quit?" (the source's confirmation), defaulting to stay.
    if (Action == EMemoriaPauseAction::Quit) { bAskQuit = true; QuitChoice = 1; return; }
    OnAction.ExecuteIfBound(Action);
}
void UMemoriaPauseWidget::ActivateOption(int32 Row, int32 Step)
{
    if (!Settings) return;
    OptionRow = Row;
    // The title's rules: sliders move in tens, a confirm toggles the switches.
    switch (Row)
    {
    case 0: if (Step) Settings->SetMasterVolume(Settings->GetMasterVolume() + 10 * Step); break;
    case 1: if (Step) Settings->SetMusicVolume(Settings->GetMusicVolume() + 10 * Step); break;
    case 2: if (Step) Settings->SetSfxVolume(Settings->GetSfxVolume() + 10 * Step); break;
    case 3: Settings->SetFullscreen(!Settings->IsFullscreen()); break;
    case 4: Settings->SetLocale(Settings->GetLocale() == TEXT("en") ? TEXT("ko") : TEXT("en")); OnAction.ExecuteIfBound(EMemoriaPauseAction::Language); break;
    default: if (Step == 0) Back(); break;
    }
}
void UMemoriaPauseWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    Clock += DeltaTime; NoticeAge += DeltaTime;
}
int32 UMemoriaPauseWidget::RowAt(const FGeometry& Geometry, const FVector2D& Screen) const
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Screen);
    for (int32 I = 0; I < Rows.Num(); ++I) if (Rows[I].IsInside(Local)) return I;
    return INDEX_NONE;
}
FReply UMemoriaPauseWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const int32 Row = RowAt(Geometry, Event.GetScreenSpacePosition());
    if (Row != INDEX_NONE)
    {
        if (bAskQuit) QuitChoice = Row;
        else if (bOptionsOpen) OptionRow = Row;
        else Select(Row);
    }
    return FReply::Handled();
}
FReply UMemoriaPauseWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const int32 Row = RowAt(Geometry, Event.GetScreenSpacePosition());
    if (Event.GetEffectingButton() == EKeys::RightMouseButton) { if (!Back()) OnAction.ExecuteIfBound(EMemoriaPauseAction::Resume); return FReply::Handled(); }
    if (Row == INDEX_NONE) return FReply::Handled();
    if (bAskQuit) { QuitChoice = Row; Confirm(); }
    else if (bOptionsOpen) ActivateOption(Row, Row <= 2 ? 1 : 0);
    else Activate(Row);
    return FReply::Handled();
}
int32 UMemoriaPauseWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    Rows.Reset();
    // The world dims behind a 0.58 veil; the panel sits on the right (anchors 0.585-0.92 x, 0.12-0.88 y).
    Box(Elements, L, Geometry, FVector2D::ZeroVector, Size, FLinearColor(0, 0, 0, .58f));
    const FVector2D At(Size.X * .585, Size.Y * .12), Panel(Size.X * .335, Size.Y * .76);
    Box(Elements, L + 1, Geometry, At, Panel, Srgb(.030f, .026f, .040f, .90f));
    Frame(Elements, L + 2, Geometry, At, Panel, 2.f, Srgb(.72f, .54f, .30f, .46f));
    const float Pad = 36.f, W = Panel.X - Pad * 2;
    const FSlateFontInfo Heading = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 34), Body = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 18),
        Small = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 14);
    float Y = At.Y + 52;
    const bool bSaved = NoticeAge < 1.4f && !Notice.IsEmpty();
    Text(Elements, L + 3, Geometry, bOptionsOpen ? Loc(TEXT("OPTIONS"), TEXT("옵션")) : Loc(TEXT("PAUSED"), TEXT("일시정지")), FVector2D(At.X + Panel.X * .5, Y), .5f, Heading, Srgb(.92f, .75f, .45f));
    Y += 44;
    Box(Elements, L + 2, Geometry, FVector2D(At.X + Pad, Y), FVector2D(W, 1.5f), Srgb(.72f, .54f, .30f, .35f));
    Y += 20;
    auto Row = [&](int32 Index, const FString& Label, const FString& Value, bool bOn, bool bEnabled)
    {
        const FVector2D R(At.X + Pad, Y), RS(W, RowH);
        Box(Elements, L + 2, Geometry, R, RS, bOn ? Srgb(.15f, .12f, .18f, .95f) : Srgb(.10f, .08f, .12f, .90f));
        Frame(Elements, L + 3, Geometry, R, RS, 1.f, bOn ? Srgb(.70f, .55f, .30f, .80f) : Srgb(.35f, .28f, .20f, .50f));
        if (bOn) Box(Elements, L + 3, Geometry, R, FVector2D(4, RS.Y), Srgb(1.f, .76f, .36f, .94f));
        const FLinearColor Ink = !bEnabled ? Srgb(.40f, .37f, .34f) : bOn ? Srgb(.95f, .82f, .50f) : Srgb(.70f, .65f, .55f);
        Text(Elements, L + 4, Geometry, Label, R + FVector2D(24, RS.Y * .5), 0.f, Body, Ink);
        if (!Value.IsEmpty()) Text(Elements, L + 4, Geometry, Value, R + FVector2D(RS.X - 24, RS.Y * .5), 1.f, Body, Ink);
        Rows.Add(FBox2D(R, R + RS));
        Y += RowH + RowGap;
    };
    if (!bOptionsOpen)
    {
        // The source's info card: chapter and place, then the run's numbers.
        const FVector2D I(At.X + Pad, Y), IS(W, 92);
        Box(Elements, L + 2, Geometry, I, IS, Srgb(.08f, .07f, .10f, .8f));
        TArray<FString> Lines; Info.ParseIntoArrayLines(Lines);
        for (int32 N = 0; N < Lines.Num() && N < 3; ++N) Text(Elements, L + 3, Geometry, Lines[N], I + FVector2D(16, 22 + 26.f * N), 0.f, Small, Srgb(.60f, .55f, .50f));
        Y += IS.Y + 24;
        // The ported save point is the closed Chapter 2 boundary; elsewhere Save says why it is dark.
        for (int32 N = 0; N < ItemCount; ++N)
            Row(N, FString::Printf(TEXT("%02d    %s"), N + 1, *ItemLabel(N)), N == 2 && !bCanSave ? Loc(TEXT("Not here"), TEXT("여기서는 불가")) : FString(), N == Selected && !bAskQuit, IsItemEnabled(N));
    }
    else
    {
        const FString Labels[OptionRowCount] = {Loc(TEXT("Master Volume"), TEXT("전체 음량")), Loc(TEXT("BGM Volume"), TEXT("배경음악 음량")), Loc(TEXT("SFX Volume"), TEXT("효과음 음량")),
            Loc(TEXT("Fullscreen"), TEXT("전체 화면")), Loc(TEXT("Language"), TEXT("언어")), Loc(TEXT("Back"), TEXT("뒤로"))};
        FString Values[OptionRowCount];
        if (Settings)
        {
            Values[0] = FString::Printf(TEXT("%d%%"), Settings->GetMasterVolume()); Values[1] = FString::Printf(TEXT("%d%%"), Settings->GetMusicVolume());
            Values[2] = FString::Printf(TEXT("%d%%"), Settings->GetSfxVolume()); Values[3] = Settings->IsFullscreen() ? Loc(TEXT("On"), TEXT("켜짐")) : Loc(TEXT("Off"), TEXT("꺼짐"));
            Values[4] = Settings->GetLocale() == TEXT("en") ? TEXT("English") : TEXT("한국어");
        }
        for (int32 N = 0; N < OptionRowCount; ++N) Row(N, Labels[N], Values[N], N == OptionRow, true);
    }
    // The last-saved line (the source's green note), then the controls hint.
    if (bSaved) Text(Elements, L + 3, Geometry, Notice, FVector2D(At.X + Panel.X * .5, Y + 8), .5f, Small, Srgb(.55f, .75f, .45f, FMath::Clamp((1.4f - NoticeAge) / .4f, 0.f, 1.f)));
    Text(Elements, L + 3, Geometry, bOptionsOpen ? Loc(TEXT("↑↓ Choose    ←→ Change    ESC Back"), TEXT("↑↓ 선택    ←→ 변경    ESC 뒤로"))
        : Loc(TEXT("↑↓ Choose    ENTER Confirm    ESC Resume"), TEXT("↑↓ 선택    ENTER 결정    ESC 계속하기")),
        FVector2D(At.X + Panel.X * .5, At.Y + Panel.Y - 30), .5f, Small, Srgb(.45f, .42f, .38f));
    if (bAskQuit)
    {
        // The quit question, over everything; its two buttons replace the rows for the mouse.
        Rows.Reset();
        const FVector2D Q(Size.X * .5 - 300, Size.Y * .5 - 130), QS(600, 260);
        Box(Elements, L + 6, Geometry, FVector2D::ZeroVector, Size, FLinearColor(0, 0, 0, .45f));
        Box(Elements, L + 7, Geometry, Q, QS, Srgb(.04f, .035f, .05f, .97f));
        Frame(Elements, L + 8, Geometry, Q, QS, 2.f, Srgb(.72f, .54f, .30f, .6f));
        Text(Elements, L + 9, Geometry, Loc(TEXT("Are you sure you want to quit?"), TEXT("정말 종료할까요?")), Q + FVector2D(QS.X * .5, 62), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 26), Srgb(.92f, .80f, .60f));
        Text(Elements, L + 9, Geometry, Loc(TEXT("Unsaved progress will be lost."), TEXT("저장하지 않은 진행은 사라집니다.")), Q + FVector2D(QS.X * .5, 110), .5f, Small, Srgb(.70f, .55f, .45f));
        const FString Choices[2] = {Loc(TEXT("Yes, Quit"), TEXT("예, 종료")), Loc(TEXT("No, Stay"), TEXT("아니요"))};
        for (int32 N = 0; N < 2; ++N)
        {
            const FVector2D B(Q.X + 60 + N * 250, Q.Y + 160), BS(230, 58);
            const bool bOn = N == QuitChoice;
            Box(Elements, L + 8, Geometry, B, BS, bOn ? Srgb(.18f, .14f, .10f, .95f) : Srgb(.10f, .08f, .12f, .9f));
            Frame(Elements, L + 9, Geometry, B, BS, 1.f, bOn ? Srgb(.85f, .65f, .30f) : Srgb(.35f, .28f, .20f, .5f));
            Text(Elements, L + 10, Geometry, Choices[N], B + BS * .5, .5f, Body, bOn ? Srgb(.95f, .82f, .50f) : Srgb(.70f, .65f, .55f));
            Rows.Add(FBox2D(B, B + BS));
        }
    }
    return L + 12;
}
