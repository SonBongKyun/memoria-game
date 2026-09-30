#include "Codex/MemoriaCodexWidget.h"
#include "Codex/MemoriaCodexSubsystem.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
namespace
{
using MemoriaUiKit::Srgb;
FVector2D Measure(const FString& S, const FSlateFontInfo& Font) { return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font); }
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& At, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = Measure(S, Font);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1, 1)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .6f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{ FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color); }
void Panel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Fill, const FLinearColor& Border, float W)
{
    Box(Elements, Layer, G, At, Size, Fill);
    Box(Elements, Layer + 1, G, At, FVector2D(Size.X, W), Border); Box(Elements, Layer + 1, G, At + FVector2D(0, Size.Y - W), FVector2D(Size.X, W), Border);
    Box(Elements, Layer + 1, G, At, FVector2D(W, Size.Y), Border); Box(Elements, Layer + 1, G, At + FVector2D(Size.X - W, 0), FVector2D(W, Size.Y), Border);
}
TArray<FString> Wrap(const FString& Text, const FSlateFontInfo& Font, float Width)
{
    TArray<FString> Out, Paragraphs; Text.ParseIntoArray(Paragraphs, TEXT("\n"), false);
    for (const FString& P : Paragraphs)
    {
        TArray<FString> Words; P.ParseIntoArray(Words, TEXT(" "), true);
        FString Line;
        for (const FString& W : Words)
        {
            const FString Try = Line.IsEmpty() ? W : Line + TEXT(" ") + W;
            if (!Line.IsEmpty() && Measure(Try, Font).X > Width) { Out.Add(Line); Line = W; } else Line = Try;
        }
        Out.Add(Line);
    }
    return Out;
}
// GRADE_COLORS_LOCAL and GRADE_NAMES_LOCAL, by raw grade (0 = Grade 5, sensory).
const FLinearColor GradeColors[5] = {Srgb(.5f, .5f, .45f), Srgb(.55f, .5f, .35f), Srgb(.4f, .5f, .6f), Srgb(.6f, .45f, .55f), Srgb(.7f, .55f, .3f)};
const TCHAR* GradeEn[5] = {TEXT("Grade 5, Sensory"), TEXT("Grade 4, Daily"), TEXT("Grade 3, Relational"), TEXT("Grade 2, Identity"), TEXT("Grade 1, Core")};
const TCHAR* GradeKo[5] = {TEXT("5등급 · 감각"), TEXT("4등급 · 일상"), TEXT("3등급 · 관계"), TEXT("2등급 · 정체성"), TEXT("1등급 · 핵심")};
}
void UMemoriaCodexWidget::Configure(UMemoriaCodexSubsystem* InCodex, bool bInKo)
{
    Codex = InCodex; bKo = bInKo; Tab = 0; Selected = 0; FirstLine = 0;
    if (!Backdrop && (Backdrop = MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_codex_archive_backdrop_v2.png"))))
    { BackdropBrush.SetResourceObject(Backdrop); BackdropBrush.ImageSize = FVector2D(Backdrop->GetSizeX(), Backdrop->GetSizeY()); BackdropBrush.DrawAs = ESlateBrushDrawType::Image; }
}
void UMemoriaCodexWidget::SetTab(int32 InTab) { Tab = FMath::Clamp(InTab, 0, 1); Selected = 0; FirstLine = 0; }
void UMemoriaCodexWidget::Move(int32 Delta)
{
    const int32 Count = SelectableLines().Num();
    if (Count) Selected = FMath::Clamp(Selected + Delta, 0, Count - 1);
}
FReply UMemoriaCodexWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{ Move(Event.GetWheelDelta() > 0 ? -1 : 1); return FReply::Handled(); }
FString UMemoriaCodexWidget::EnemyName(const FString& Name) const { return bKo ? UMemoriaCodexSubsystem::EnemyNameKo(Name) : Name; }
FString UMemoriaCodexWidget::StatusText() const
{
    const auto* C = Codex.Get();
    const int32 E = C ? C->GetEnemies().Num() : 0, M = C ? C->GetMemories().Num() : 0;
    return bKo ? FString::Printf(TEXT("생물 기록 %d  ·  기억 기록 %d"), E, M) : FString::Printf(TEXT("CREATURE RECORDS %d  ·  MEMORY RECORDS %d"), E, M);
}
TArray<UMemoriaCodexWidget::FLine> UMemoriaCodexWidget::ListLines() const
{
    TArray<FLine> Out;
    const auto* C = Codex.Get();
    if (!C) return Out;
    if (Tab == 0)
    {
        // _populate_bestiary: the progress first, the recorded foes, then the unmet with their names hidden.
        Out.Add({bKo ? FString::Printf(TEXT("기록 %d / %d"), C->GetEnemies().Num(), C->KnownTotal()) : FString::Printf(TEXT("Recorded %d / %d"), C->GetEnemies().Num(), C->KnownTotal()), Srgb(.72f, .63f, .45f), false});
        for (const auto& E : C->GetEnemies())
            Out.Add({EnemyName(E.Name) + UMemoriaCodexSubsystem::DefeatBadge(E.Defeated), E.bBoss ? Srgb(.7f, .5f, .3f) : E.bVoid ? Srgb(.6f, .3f, .5f) : Srgb(.5f, .55f, .45f)});
        const TArray<FString> Unmet = C->Unmet();
        if (!Unmet.IsEmpty())
        {
            Out.Add({bKo ? FString::Printf(TEXT("미조우 %d"), Unmet.Num()) : FString::Printf(TEXT("Unrecorded %d"), Unmet.Num()), Srgb(.5f, .47f, .52f), false});
            for (int32 I = 0; I < Unmet.Num(); ++I) Out.Add({TEXT("???"), Srgb(.34f, .32f, .36f)});
        }
    }
    else
    {
        // _populate_memory_archive.
        if (C->GetMemories().IsEmpty()) { Out.Add({Loc(TEXT("No memories collected yet."), TEXT("아직 모은 기억이 없습니다.")), Srgb(.55f, .5f, .45f), false}); return Out; }
        for (const auto& M : C->GetMemories())
            Out.Add({UMemoriaCodexSubsystem::Stars(M.Grade) + TEXT(" ") + M.Title + (M.bBurned ? Loc(TEXT(" [BURNED]"), TEXT(" [연소됨]")) : FString()), GradeColors[FMath::Clamp(M.Grade, 0, 4)]});
    }
    return Out;
}
TArray<int32> UMemoriaCodexWidget::SelectableLines() const
{
    TArray<int32> Out; const auto Lines = ListLines();
    for (int32 I = 0; I < Lines.Num(); ++I) if (Lines[I].bSelectable) Out.Add(I);
    return Out;
}
FString UMemoriaCodexWidget::DetailTitle() const
{
    const auto* C = Codex.Get();
    if (!C || SelectableLines().IsEmpty()) return Loc(TEXT("Select an entry..."), TEXT("항목을 고르세요..."));
    if (Tab == 1) return C->GetMemories().IsValidIndex(Selected) ? C->GetMemories()[Selected].Title : FString();
    return C->GetEnemies().IsValidIndex(Selected) ? EnemyName(C->GetEnemies()[Selected].Name) : TEXT("???");
}
FString UMemoriaCodexWidget::DetailBody() const
{
    const auto* C = Codex.Get();
    if (!C || SelectableLines().IsEmpty()) return FString();
    if (Tab == 1)
    {
        // _show_memory_detail: the grade and its stars, whether it is held or burned, then its description.
        if (!C->GetMemories().IsValidIndex(Selected)) return FString();
        const auto& M = C->GetMemories()[Selected];
        const int32 G = FMath::Clamp(M.Grade, 0, 4);
        return FString::Printf(TEXT("%s  %s\n%s\n\n%s"), bKo ? GradeKo[G] : GradeEn[G], *UMemoriaCodexSubsystem::Stars(M.Grade),
            *(bKo ? FString(TEXT("상태: ")) + (M.bBurned ? TEXT("연소됨") : TEXT("보유")) : FString(TEXT("Status: ")) + (M.bBurned ? TEXT("BURNED") : TEXT("Held"))), *M.Desc);
    }
    if (!C->GetEnemies().IsValidIndex(Selected))
        // _show_unmet_detail; scans are not carried, so only the first half of its second line.
        return Loc(TEXT("Not yet encountered.\nEngaging records the basics."), TEXT("아직 마주치지 않았습니다.\n교전을 시작하면 기본 정보가 기록됩니다."));
    const auto& E = C->GetEnemies()[Selected];
    const FString Type = E.bBoss ? Loc(TEXT("BOSS"), TEXT("보스")) : E.bVoid ? Loc(TEXT("Void Beast"), TEXT("보이드")) : Loc(TEXT("Normal"), TEXT("일반"));
    return bKo ? FString::Printf(TEXT("종류: %s\n기본 HP: %d\n기본 공격력: %d\n\n조우: %d\n처치: %d%s"), *Type, E.MaxHp, E.Atk, E.Encounters, E.Defeated, *UMemoriaCodexSubsystem::DefeatBadge(E.Defeated))
               : FString::Printf(TEXT("Type: %s\nBase HP: %d\nBase ATK: %d\n\nEncounters: %d\nDefeated: %d%s"), *Type, E.MaxHp, E.Atk, E.Encounters, E.Defeated, *UMemoriaCodexSubsystem::DefeatBadge(E.Defeated));
}
int32 UMemoriaCodexWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const float S = Size.Y / 720.f;
    if (Backdrop)
    {
        const float Cover = FMath::Max(Size.X / Backdrop->GetSizeX(), Size.Y / Backdrop->GetSizeY());
        const FVector2D Art(Backdrop->GetSizeX() * Cover, Backdrop->GetSizeY() * Cover);
        FSlateDrawElement::MakeBox(Elements, L, Geometry.ToPaintGeometry(FVector2f(Art), FSlateLayoutTransform(FVector2f((Size - Art) * .5))), &BackdropBrush, ESlateDrawEffect::None, FLinearColor::White);
    }
    Box(Elements, L + 1, Geometry, FVector2D::ZeroVector, Size, Srgb(.015f, .012f, .025f, Backdrop ? .46f : .88f));
    // The panel: anchors .10-.90 by .05-.95, 18 margins.
    const FVector2D At(Size.X * .10f, Size.Y * .05f), P(Size.X * .80f, Size.Y * .90f);
    Panel(Elements, L + 2, Geometry, At, P, Srgb(.035f, .03f, .05f, .82f), Srgb(.5f, .38f, .22f, .78f), 2.f * S);
    const float M = 18.f * S, Inner = P.X - M * 2.f;
    auto Centre = [&](const FString& T, float Y, const FSlateFontInfo& F, const FLinearColor& C) { Text(Elements, L + 4, Geometry, T, FVector2D(At.X + (P.X - Measure(T, F).X) * .5f, Y), F, C); };
    const FSlateFontInfo Head = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, FMath::RoundToInt(24 * S)), Small = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(13 * S));
    float Y = At.Y + M;
    Centre(Loc(TEXT("CODEX"), TEXT("도감")), Y, Head, Srgb(.8f, .7f, .5f)); Y += 38 * S;
    Centre(Loc(TEXT("Creatures encountered. Memories carried and spent."), TEXT("조우한 존재와 잃어버린 기억의 기록")), Y, Small, Srgb(.56f, .52f, .49f)); Y += 22 * S;
    Centre(StatusText(), Y, Small, Srgb(.55f, .62f, .72f)); Y += 28 * S;
    // The tabs, centred, 150 by 30 each with 8 between.
    const FString Tabs[2] = {Loc(TEXT("Bestiary"), TEXT("조우 기록")), Loc(TEXT("Memory Archive"), TEXT("기억 서고"))};
    const FVector2D TabSize(150 * S, 30 * S);
    for (int32 I = 0; I < 2; ++I)
    {
        const FVector2D T(At.X + P.X * .5f - TabSize.X - 4 * S + I * (TabSize.X + 8 * S), Y);
        Panel(Elements, L + 3, Geometry, T, TabSize, I == Tab ? Srgb(.16f, .12f, .10f, .95f) : Srgb(.08f, .07f, .09f, .85f), I == Tab ? Srgb(.85f, .65f, .35f, .9f) : Srgb(.35f, .28f, .2f, .5f), 1.f * S);
        Text(Elements, L + 5, Geometry, Tabs[I], T + (TabSize - Measure(Tabs[I], Small)) * .5f, Small, I == Tab ? Srgb(.95f, .82f, .5f) : Srgb(.6f, .56f, .5f));
    }
    Y += TabSize.Y + 10 * S;
    Box(Elements, L + 3, Geometry, FVector2D(At.X + M, Y), FVector2D(Inner, S), Srgb(.5f, .38f, .22f, .45f)); Y += 10 * S;
    // The list on the left, the detail panel (350 wide) on the right.
    const float Bottom = At.Y + P.Y - M - 22 * S, DetailW = 350 * S, ListW = Inner - DetailW - 12 * S, RowH = 28 * S, Gap = 3 * S;
    const auto Lines = ListLines();
    const auto Pickable = SelectableLines();
    const int32 Chosen = Pickable.IsValidIndex(Selected) ? Pickable[Selected] : -1;
    const int32 Visible = FMath::Max(1, int32((Bottom - Y + Gap) / (RowH + Gap)));
    // Keep the chosen line in view.
    if (Chosen >= 0) { if (Chosen < FirstLine) FirstLine = Chosen; if (Chosen >= FirstLine + Visible) FirstLine = Chosen - Visible + 1; }
    FirstLine = FMath::Clamp(FirstLine, 0, FMath::Max(0, Lines.Num() - Visible));
    const FSlateFontInfo Row = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(14 * S));
    for (int32 N = 0; N < Visible && Lines.IsValidIndex(FirstLine + N); ++N)
    {
        const int32 I = FirstLine + N; const auto& Line = Lines[I];
        const FVector2D R(At.X + M, Y + N * (RowH + Gap));
        if (Line.bSelectable)
        {
            // _make_list_btn: the colour at a fifth as the fill and a 3 px left rule; the chosen one lit.
            const bool bOn = I == Chosen; const FLinearColor C = Line.Color;
            Box(Elements, L + 3, Geometry, R, FVector2D(ListW, RowH), FLinearColor(C.R * (bOn ? .35f : .2f), C.G * (bOn ? .35f : .2f), C.B * (bOn ? .35f : .2f), bOn ? .7f : .5f));
            Box(Elements, L + 4, Geometry, R, FVector2D(3 * S, RowH), bOn ? C : FLinearColor(C.R * .4f, C.G * .4f, C.B * .4f, .3f));
        }
        Text(Elements, L + 5, Geometry, Line.Label, R + FVector2D(10 * S, (RowH - Measure(Line.Label, Row).Y) * .5f), Row, Line.bSelectable ? FLinearColor(FMath::Min(1.f, Line.Color.R * 1.5f), FMath::Min(1.f, Line.Color.G * 1.5f), FMath::Min(1.f, Line.Color.B * 1.5f)) : Line.Color);
    }
    // The detail: its title, a rule, and the body wrapped to the panel.
    const FVector2D D(At.X + M + ListW + 12 * S, Y), DS(DetailW, Bottom - Y);
    Panel(Elements, L + 3, Geometry, D, DS, Srgb(.05f, .04f, .06f, .8f), Srgb(.3f, .26f, .22f, .6f), 1.f * S);
    const FSlateFontInfo DetailHead = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(16 * S));
    Text(Elements, L + 5, Geometry, DetailTitle(), D + FVector2D(14, 12) * S, DetailHead, Srgb(.8f, .7f, .55f));
    Box(Elements, L + 5, Geometry, D + FVector2D(14 * S, 40 * S), FVector2D(DS.X - 28 * S, S), Srgb(.5f, .38f, .22f, .4f));
    float DY = D.Y + 50 * S;
    for (const FString& Line : Wrap(DetailBody(), Small, DS.X - 28 * S))
    {
        if (DY > D.Y + DS.Y - 20 * S) break;
        Text(Elements, L + 5, Geometry, Line, FVector2D(D.X + 14 * S, DY), Small, Srgb(.65f, .6f, .55f));
        DY += Measure(TEXT("가"), Small).Y * 1.25f;
    }
    const FString Hint = Loc(TEXT("TAB / ←→ Tabs    ↑↓ Choose    [ESC] Close"), TEXT("TAB / ←→ 탭    ↑↓ 선택    [ESC] 닫기"));
    Text(Elements, L + 4, Geometry, Hint, FVector2D(At.X + P.X - M - Measure(Hint, Small).X, Bottom + 4 * S), Small, Srgb(.4f, .35f, .3f));
    return L + 7;
}
