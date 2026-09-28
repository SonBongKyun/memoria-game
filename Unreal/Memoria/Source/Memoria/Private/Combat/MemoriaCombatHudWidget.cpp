#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
namespace
{
using MemoriaUiKit::Srgb;
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Center, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D At = Center - Size * .5;
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1.5, 1.5)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .8f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
// Text anchored by its left (Align 0), centre (.5) or right (1) edge, vertically centred on Anchor.Y.
void TextAt(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Anchor, float Align, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    Text(Elements, Layer, G, S, Anchor + FVector2D(Size.X * (.5 - Align), 0), Font, Color);
}
// A world circle on the floor, projected to the screen as a closed polyline.
void Ring(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, APlayerController* PC, const FVector& Center, float Radius, float Thickness, const FLinearColor& Color)
{
    TArray<FVector2f> Points;
    for (int32 I = 0; I <= 72; ++I)
    {
        const float A = 2.f * PI * I / 72.f;
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Center + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Radius, At, false)) return;
        Points.Add(FVector2f(At));
    }
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
}
FLinearColor Toward(const FLinearColor& Hue, float White, float Alpha)
{ return FLinearColor(FMath::Lerp(Hue.R, 1.f, White), FMath::Lerp(Hue.G, 1.f, White), FMath::Lerp(Hue.B, 1.f, White), Alpha); }
}
bool UMemoriaCombatHudWidget::IsShowing() const
{
    const auto* C = Combat.Get();
    return C && (C->LiveMonsterCount() > 0 || C->GetPopups().Num() > 0 || C->IsDefeated() || C->GetPlayerHp() < C->GetPlayerMaxHp() ||
        C->IsPickingBurn() || C->IsCasting() || C->GetBurnWave().bLive);
}
int32 UMemoriaCombatHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* C = Combat.Get();
    APlayerController* PC = GetOwningPlayer();
    if (!C || !PC || !IsShowing()) return Layer;
    const FVector2D Size = Geometry.GetLocalSize();
    // The run's locale, like the memory titles and the story text (a new game takes it from the settings).
    const auto* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr;
    const bool Ko = !Run || Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    // S312: the released burn. The ring spreads over the floor in the grade's colour, the screen flares,
    // and the banner names the skill and the memory it cost.
    const FMemoriaBurnWave& Wave = C->GetBurnWave();
    if (Wave.bLive)
    {
        using namespace MemoriaCombatTuning;
        const FLinearColor Hue = BurnColor(Wave.Grade);
        const float Spread = FMath::Clamp(Wave.Age / BurnRingTime, 0.f, 1.f), Reach = Wave.Radius * (1.f - FMath::Square(1.f - Spread));
        const float Fade = FMath::Clamp(1.f - (Wave.Age - BurnRingTime) / .6f, 0.f, 1.f);
        if (Fade > 0.f)
        {
            Ring(Elements, Layer, Geometry, PC, Wave.Center, Reach, 14.f, Toward(Hue, 0.f, .22f * Fade));
            Ring(Elements, Layer + 1, Geometry, PC, Wave.Center, Reach, 4.f, Toward(Hue, .5f, .95f * Fade));
            Ring(Elements, Layer + 1, Geometry, PC, Wave.Center, Reach * .82f, 2.f, Toward(Hue, 0.f, .6f * Fade));
            Ring(Elements, Layer + 1, Geometry, PC, Wave.Center, Reach * .6f, 1.5f, Toward(Hue, 0.f, .35f * Fade));
        }
        const float Flash = FMath::Clamp(1.f - Wave.Age / .5f, 0.f, 1.f);
        if (Flash > 0.f) Box(Elements, Layer, Geometry, FVector2D::ZeroVector, Size, Toward(Hue, 0.f, .30f * Flash * Flash));
        const float Banner = FMath::Clamp(FMath::Min(Wave.Age / .15f, (BurnAfterglow - Wave.Age) / .5f), 0.f, 1.f);
        Text(Elements, Layer + 6, Geometry, Wave.Skill, FVector2D(Size.X * .5, 190), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 46), Toward(Hue, .6f, Banner));
        const FString Cost = Ko ? FString::Printf(TEXT("「%s」 — 연소"), *Wave.Title) : FString::Printf(TEXT("\"%s\" — burned"), *Wave.Title);
        Text(Elements, Layer + 6, Geometry, Cost, FVector2D(Size.X * .5, 248), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 19), Srgb(.86f, .80f, .72f, Banner * .95f));
    }
    // Husk health above each head.
    for (const auto& Weak : C->GetMonsters())
    {
        const AMemoriaFieldMonster* M = Weak.Get();
        if (!M || M->IsDead()) continue;
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, M->GetActorLocation() + FVector(0, 0, MemoriaCombatTuning::HuskHeight + 25.f), At, false)) continue;
        const FVector2D Bar(96, 9);
        Box(Elements, Layer, Geometry, At - FVector2D(Bar.X * .5 + 2, 2), Bar + FVector2D(4, 4), FLinearColor(0, 0, 0, .75f));
        Box(Elements, Layer + 1, Geometry, At - FVector2D(Bar.X * .5, 0), FVector2D(Bar.X * M->GetHealth() / M->GetMaxHealth(), Bar.Y), Srgb(.62f, .12f, .78f));
    }
    // Rising damage numbers: ember on a husk, red on Arrel.
    for (const auto& P : C->GetPopups())
    {
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, P.Location, At, false)) continue;
        const float Alpha = FMath::Clamp(1.4f - P.Age * 1.4f, 0.f, 1.f);
        Text(Elements, Layer + 2, Geometry, FString::Printf(TEXT("%d"), FMath::RoundToInt(P.Amount)), At - FVector2D(0, 60.f * P.Age),
            MemoriaFonts::Get(MemoriaFonts::EStyle::Title, P.bPlayer ? 30 : 34), P.bPlayer ? Srgb(1.f, .28f, .24f, Alpha) : Srgb(1.f, .82f, .46f, Alpha));
    }
    // Arrel's HP, bottom centre.
    const float Hp = float(C->GetPlayerHp()), Max = FMath::Max(1.f, float(C->GetPlayerMaxHp()));
    const FVector2D Bar(520, 18), At(Size.X * .5 - Bar.X * .5, Size.Y - 150);
    Box(Elements, Layer, Geometry, At - FVector2D(3, 3), Bar + FVector2D(6, 6), FLinearColor(0, 0, 0, .8f));
    Box(Elements, Layer + 1, Geometry, At, FVector2D(Bar.X * Hp / Max, Bar.Y), Srgb(.78f, .12f, .14f));
    Text(Elements, Layer + 3, Geometry, FString::Printf(TEXT("HP  %d / %d"), int32(Hp), int32(Max)), At + FVector2D(Bar.X * .5, Bar.Y * .5),
        MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15), FLinearColor(.95f, .92f, .88f));
    if (C->LiveMonsterCount() > 0)
        Text(Elements, Layer + 3, Geometry, Ko ? TEXT("좌클릭 / J  공격     Shift  회피     R  기억 연소") : TEXT("LMB / J  Attack     Shift  Dodge     R  Burn a memory"),
            At + FVector2D(Bar.X * .5, Bar.Y + 26), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.80f, .76f, .70f, .9f));
    if (C->IsDefeated())
    {
        Box(Elements, Layer + 4, Geometry, FVector2D::ZeroVector, Size, FLinearColor(.02f, 0, 0, .55f));
        Text(Elements, Layer + 5, Geometry, Ko ? TEXT("아렐이 쓰러졌다...") : TEXT("Arrel falls..."), Size * .5, MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 40), Srgb(.95f, .80f, .70f));
    }
    if (C->IsPickingBurn()) PaintBurnPicker(Elements, Layer + 7, Geometry, Ko);
    return Layer + 14;
}
void UMemoriaCombatHudWidget::PaintBurnPicker(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, bool Ko) const
{
    // The world slows behind an ember-dark veil; the memories that can burn are listed weakest first.
    const auto* C = Combat.Get();
    const FVector2D Size = Geometry.GetLocalSize();
    const auto& Choices = C->GetBurnChoices();
    const int32 Selected = C->GetBurnSelection();
    Box(Elements, Layer, Geometry, FVector2D::ZeroVector, Size, FLinearColor(.02f, .006f, .004f, .52f));
    const float RowH = 64.f, Width = 760.f, Height = 150.f + RowH * Choices.Num() + 64.f;
    const FVector2D Panel(Size.X * .5 - Width * .5, FMath::Max(40.f, Size.Y * .5 - Height * .5 - 30.f));
    Box(Elements, Layer + 1, Geometry, Panel - FVector2D(2, 2), FVector2D(Width, Height) + FVector2D(4, 4), Srgb(.55f, .24f, .10f, .85f));
    Box(Elements, Layer + 1, Geometry, Panel, FVector2D(Width, Height), Srgb(.055f, .035f, .03f, .96f));
    Text(Elements, Layer + 3, Geometry, Ko ? TEXT("무엇을 태우겠는가") : TEXT("What will you burn?"), Panel + FVector2D(Width * .5, 52),
        MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 34), Srgb(1.f, .80f, .56f));
    Text(Elements, Layer + 3, Geometry, Ko ? TEXT("태운 기억은 돌아오지 않는다") : TEXT("A burned memory never returns"), Panel + FVector2D(Width * .5, 98),
        MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15), Srgb(.78f, .56f, .44f));
    for (int32 I = 0; I < Choices.Num(); ++I)
    {
        const FMemoriaBurnChoice& Row = Choices[I];
        const FVector2D At = Panel + FVector2D(24, 132 + RowH * I);
        const bool bOn = I == Selected;
        if (bOn)
        {
            Box(Elements, Layer + 2, Geometry, At, FVector2D(Width - 48, RowH - 6), Srgb(.34f, .13f, .06f, .92f));
            Box(Elements, Layer + 2, Geometry, At, FVector2D(5, RowH - 6), Row.Accent);
        }
        // Two lines: the memory, then its grade and the skill it becomes; the blow's power on the right.
        const float Mid = At.Y + (RowH - 6) * .5f;
        const FLinearColor Ink = bOn ? Srgb(1.f, .94f, .86f) : Srgb(.80f, .74f, .68f);
        if (I < 9) TextAt(Elements, Layer + 3, Geometry, FString::FromInt(I + 1), FVector2D(At.X + 22, Mid), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 16), Srgb(.62f, .50f, .40f));
        Box(Elements, Layer + 3, Geometry, FVector2D(At.X + 44, Mid - 5), FVector2D(10, 10), Row.Accent);
        TextAt(Elements, Layer + 3, Geometry, Row.Title, FVector2D(At.X + 66, Mid - 11), 0.f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 19), Ink);
        TextAt(Elements, Layer + 3, Geometry, FString::Printf(TEXT("%s  ·  %s"), *Row.GradeLabel, MemoriaCombatTuning::BurnSkillName(Row.Grade, Ko)),
            FVector2D(At.X + 66, Mid + 14), 0.f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), bOn ? Srgb(1.f, .72f, .42f) : Srgb(.62f, .54f, .47f));
        TextAt(Elements, Layer + 3, Geometry, FString::FromInt(FMath::RoundToInt(Row.Damage)), FVector2D(At.X + Width - 72, Mid), 1.f,
            MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 24), bOn ? Srgb(1.f, .80f, .52f) : Srgb(.70f, .60f, .50f));
    }
    const FVector2D Foot = Panel + FVector2D(Width * .5, Height - 38);
    if (C->IsBurnArmed() && Choices.IsValidIndex(Selected))
    {
        const FString Warn = Ko ? FString::Printf(TEXT("한 번 더 누르면 「%s」은(는) 영원히 사라진다"), *Choices[Selected].Title)
            : FString::Printf(TEXT("Press again and \"%s\" is gone forever"), *Choices[Selected].Title);
        Text(Elements, Layer + 3, Geometry, Warn, Foot, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 16), Srgb(1.f, .36f, .30f));
    }
    else
        Text(Elements, Layer + 3, Geometry, Ko ? TEXT("1–9 · ↑↓  고르기      R · Enter  태우기      Esc  그만두기") : TEXT("1–9 · ↑↓  Choose      R · Enter  Burn      Esc  Let it be"), Foot,
            MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 14), Srgb(.72f, .64f, .56f));
}
