#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Settings/MemoriaSettingsSubsystem.h"
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
}
bool UMemoriaCombatHudWidget::IsShowing() const
{
    const auto* C = Combat.Get();
    return C && (C->LiveMonsterCount() > 0 || C->GetPopups().Num() > 0 || C->IsDefeated() || C->GetPlayerHp() < C->GetPlayerMaxHp());
}
int32 UMemoriaCombatHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* C = Combat.Get();
    APlayerController* PC = GetOwningPlayer();
    if (!C || !PC || !IsShowing()) return Layer;
    const FVector2D Size = Geometry.GetLocalSize();
    const auto* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
    const bool Ko = !Settings || Settings->GetLocale() != TEXT("en");
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
        Text(Elements, Layer + 3, Geometry, Ko ? TEXT("좌클릭 / J  공격     Shift  회피") : TEXT("LMB / J  Attack     Shift  Dodge"),
            At + FVector2D(Bar.X * .5, Bar.Y + 26), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.80f, .76f, .70f, .9f));
    if (C->IsDefeated())
    {
        Box(Elements, Layer + 4, Geometry, FVector2D::ZeroVector, Size, FLinearColor(.02f, 0, 0, .55f));
        Text(Elements, Layer + 5, Geometry, Ko ? TEXT("아렐이 쓰러졌다...") : TEXT("Arrel falls..."), Size * .5, MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 40), Srgb(.95f, .80f, .70f));
    }
    return Layer + 6;
}
