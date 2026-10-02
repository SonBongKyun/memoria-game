#include "Combat/MemoriaCombatHudWidget.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaHudKit.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaUiKit.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
namespace
{
using MemoriaUiKit::Srgb;
constexpr float RewardShown = 3.5f; // seconds the victory rewards stay up
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
// Part of a world circle on the floor, from angle 0 through Fraction of a full turn.
void Arc(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, APlayerController* PC, const FVector& Center, float Radius, float Fraction, float Thickness, const FLinearColor& Color)
{
    TArray<FVector2f> Points;
    const int32 Steps = FMath::Max(2, FMath::CeilToInt(48 * Fraction));
    for (int32 I = 0; I <= Steps; ++I)
    {
        const float A = 2.f * PI * Fraction * I / Steps - PI * .5f;
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Center + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Radius, At, false)) return;
        Points.Add(FVector2f(At));
    }
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
}
void Segment(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, APlayerController* PC, const FVector& A, const FVector& B, float Thickness, const FLinearColor& Color)
{
    FVector2D PA, PB;
    if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, A, PA, false) || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, B, PB, false)) return;
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), TArray<FVector2f>{FVector2f(PA), FVector2f(PB)}, ESlateDrawEffect::None, Color, true, Thickness);
}
FLinearColor Toward(const FLinearColor& Hue, float White, float Alpha)
{ return FLinearColor(FMath::Lerp(Hue.R, 1.f, White), FMath::Lerp(Hue.G, 1.f, White), FMath::Lerp(Hue.B, 1.f, White), Alpha); }
}
void UMemoriaCombatHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Ribbon = MemoriaHudKit::Load(MemoriaHudKit::EArt::Ribbon);
    MemoriaHudKit::Brush(RibbonBrush, Ribbon);
}
bool UMemoriaCombatHudWidget::IsShowing() const
{
    const auto* C = Combat.Get();
    return C && (C->LiveMonsterCount() > 0 || C->GetPopups().Num() > 0 || C->IsDefeated() || C->GetPlayerHp() < C->GetPlayerMaxHp() ||
        C->IsPickingBurn() || C->IsCasting() || C->GetBurnWave().bLive || C->GetLastReward().Age < RewardShown || C->IsWeakened() || C->IsPoisoned() ||
        C->GetSparks().Num() > 0 || C->GetTrail().Num() > 0 || C->GetImpacts().Num() > 0 || C->GetCharge() > 0.f || C->IsBlocking() || C->GetEliaNoticeAge() < 3.5f);
}
int32 UMemoriaCombatHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* C = Combat.Get();
    APlayerController* PC = GetOwningPlayer();
    // The game over screen (S318) takes over from the fall's veil.
    if (!C || !PC || !IsShowing() || C->IsAwaitingGameOver()) return Layer;
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
    // S315: the blade's trail (a fan of streaks through the recent blade positions), sparks, the charge ring.
    const auto& Trail = C->GetTrail();
    for (int32 I = 1; I < Trail.Num(); ++I)
    {
        const float Fade = FMath::Clamp(1.f - Trail[I].Age / MemoriaCombatTuning::TrailLife, 0.f, 1.f);
        for (const float Along : {1.f, .8f, .6f})
        {
            const FVector A = FMath::Lerp(Trail[I - 1].Base, Trail[I - 1].Tip, Along), B = FMath::Lerp(Trail[I].Base, Trail[I].Tip, Along);
            Segment(Elements, Layer, Geometry, PC, A, B, Along == 1.f ? 5.f : 3.f, FLinearColor(.85f, .92f, 1.f, Fade * (Along == 1.f ? .85f : .45f)));
        }
        Segment(Elements, Layer, Geometry, PC, Trail[I].Base, Trail[I].Tip, 1.5f, FLinearColor(.8f, .88f, 1.f, Fade * .18f));
    }
    for (const FMemoriaSpark& Spark : C->GetSparks())
    {
        // A short streak along the spark's flight, with a soft glow under it.
        const float Life = FMath::Clamp(1.f - Spark.Age / Spark.Life, 0.f, 1.f), Width = Spark.Size * (.5f + .5f * Life);
        const FLinearColor Hot(FMath::Lerp(Spark.Color.R, 1.f, Life * .6f), FMath::Lerp(Spark.Color.G, 1.f, Life * .6f), FMath::Lerp(Spark.Color.B, 1.f, Life * .45f), Life);
        const FVector Tail = Spark.Location - Spark.Velocity * .035f;
        Segment(Elements, Layer, Geometry, PC, Tail, Spark.Location, Width * 2.4f, FLinearColor(Spark.Color.R, Spark.Color.G, Spark.Color.B, .28f * Life));
        Segment(Elements, Layer + 1, Geometry, PC, Tail, Spark.Location, Width, Hot);
    }
    // S340: where a blow landed, a ring opens and fades; a heavy one also throws a cross of light.
    for (const FMemoriaImpact& Mark : C->GetImpacts())
    {
        const float T = FMath::Clamp(Mark.Age / Mark.Life, 0.f, 1.f), Open = 1.f - FMath::Square(1.f - T), Fade = 1.f - T;
        Ring(Elements, Layer + 1, Geometry, PC, Mark.Location, Mark.Radius * (.25f + .75f * Open), (Mark.bHeavy ? 7.f : 4.5f) * Fade + 1.f, Toward(Mark.Color, .45f, .9f * Fade));
        Ring(Elements, Layer, Geometry, PC, Mark.Location, Mark.Radius * (.15f + .55f * Open), 10.f * Fade + 1.f, Toward(Mark.Color, 0.f, .22f * Fade));
        FVector2D At;
        if (Mark.bHeavy && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Mark.Location, At, false))
        {
            const float Reach = (60.f + 150.f * Open) * FMath::Clamp(Mark.Radius / 120.f, .8f, 1.5f);
            for (const FVector2D& Along : {FVector2D(.94, -.34), FVector2D(-.5, -.86)})
                FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(), TArray<FVector2f>{FVector2f(At - Along * Reach), FVector2f(At + Along * Reach)},
                    ESlateDrawEffect::None, Toward(Mark.Color, .8f, .85f * Fade * Fade), true, 5.f * Fade + 1.f);
        }
    }
    if (const APawn* Arrel = C->GetPlayer(); Arrel && C->GetCharge() > .15f)
    {
        const float Charge = C->GetCharge();
        Arc(Elements, Layer, Geometry, PC, Arrel->GetActorLocation() - FVector(0, 0, 6.f), 70.f, 1.f, 2.f, FLinearColor(1.f, .9f, .6f, .25f));
        Arc(Elements, Layer + 1, Geometry, PC, Arrel->GetActorLocation() - FVector(0, 0, 6.f), 70.f, Charge, Charge >= 1.f ? 6.f : 4.f, FLinearColor(1.f, .82f, .45f, .95f));
    }
    // Husk health above each head.
    for (const auto& Weak : C->GetMonsters())
    {
        const AMemoriaFieldMonster* M = Weak.Get();
        if (!M || M->IsDead()) continue;
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, M->GetActorLocation() + FVector(0, 0, M->Spec().Height + 25.f), At, false)) continue;
        const FVector2D Bar(96, 9);
        Box(Elements, Layer, Geometry, At - FVector2D(Bar.X * .5 + 2, 2), Bar + FVector2D(4, 4), FLinearColor(0, 0, 0, .75f));
        Box(Elements, Layer + 1, Geometry, At - FVector2D(Bar.X * .5, 0), FVector2D(Bar.X * M->GetHealth() / M->GetMaxHealth(), Bar.Y), Srgb(.62f, .12f, .78f));
    }
    // Rising damage numbers: ember on a husk, red on Arrel.
    for (const auto& P : C->GetPopups())
    {
        FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, P.Location, At, false)) continue;
        // Words (grains, statuses, healing) linger a little longer than numbers.
        const bool bWord = !P.Label.IsEmpty();
        const float Alpha = FMath::Clamp((bWord ? 2.2f : 1.4f) - P.Age * 1.4f, 0.f, 1.f);
        const FLinearColor Base = P.Tint.A > 0.f ? P.Tint : P.bPlayer ? FLinearColor(1.f, .28f, .24f) : FLinearColor(1.f, .82f, .46f);
        // S340: a number lands large and settles; a heavy or killing blow's stays larger and brighter.
        const float Pop = 1.f + .55f * FMath::Square(FMath::Clamp(1.f - P.Age / .16f, 0.f, 1.f));
        const int32 Points = FMath::RoundToInt((bWord ? 22 : P.bPlayer ? 30 : P.bBig ? 46 : 34) * Pop);
        const FLinearColor Ink = P.bBig && !bWord ? FLinearColor(FMath::Lerp(Base.R, 1.f, .35f), FMath::Lerp(Base.G, 1.f, .35f), FMath::Lerp(Base.B, 1.f, .35f)) : Base;
        Text(Elements, Layer + 2, Geometry, bWord ? P.Label : FString::Printf(TEXT("%d"), FMath::RoundToInt(P.Amount)), At - FVector2D(0, (bWord ? 40.f : 60.f) * P.Age),
            MemoriaFonts::Get(bWord ? MemoriaFonts::EStyle::Ui : MemoriaFonts::EStyle::Title, Points), Srgb(Ink.R, Ink.G, Ink.B, Alpha));
    }
    // S340: a wound reddens the screen's edge for a moment; under a third of his HP the edge keeps a slow pulse.
    {
        const float Share = C->GetPlayerMaxHp() > 0 ? float(C->GetPlayerHp()) / float(C->GetPlayerMaxHp()) : 1.f;
        const float Struck = FMath::Square(FMath::Clamp(1.f - C->GetHurtAge() / UMemoriaFieldCombatSubsystem::HurtTime, 0.f, 1.f)) * .5f;
        const float Low = Share < .3f && !C->IsDefeated() ? .16f + .1f * FMath::Sin(float(FPlatformTime::Seconds()) * 4.6f) : 0.f;
        const float Edge = FMath::Max(Struck, Low);
        if (Edge > .01f)
        {
            const FLinearColor Red(.55f, .02f, .02f, Edge), Clear(.55f, .02f, .02f, 0.f);
            const float Wide = Size.X * .17f, Tall = Size.Y * .2f;
            auto Band = [&](const FVector2D& At, const FVector2D& Extent, EOrientation Way, bool bFromStart)
            {
                FSlateDrawElement::MakeGradient(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(Extent), FSlateLayoutTransform(FVector2f(At))),
                    TArray<FSlateGradientStop>{FSlateGradientStop(FVector2D::ZeroVector, bFromStart ? Red : Clear), FSlateGradientStop(Extent, bFromStart ? Clear : Red)}, Way, ESlateDrawEffect::None);
            };
            Band(FVector2D::ZeroVector, FVector2D(Wide, Size.Y), Orient_Vertical, true);
            Band(FVector2D(Size.X - Wide, 0), FVector2D(Wide, Size.Y), Orient_Vertical, false);
            Band(FVector2D::ZeroVector, FVector2D(Size.X, Tall), Orient_Horizontal, true);
            Band(FVector2D(0, Size.Y - Tall), FVector2D(Size.X, Tall), Orient_Horizontal, false);
        }
    }
    // S339: the command ribbon at the bottom centre (the source's ui_battle_command_ribbon). Its seven cells hold
    // Elia's four techniques, the burn, the guard and the dodge, each with its key; its left orb holds Arrel's HP
    // and its right orb the memories he still has to burn. K takes the ribbon's own pixels (1840 x 386) to the
    // viewport's units.
    constexpr float K = .6f;
    const FVector2D RibbonAt(Size.X * .5 - 1840 * K * .5, Size.Y - 386 * K + 14);
    const float Hp = float(C->GetPlayerHp()), Max = FMath::Max(1.f, float(C->GetPlayerMaxHp()));
    // Arrel's HP as a gauge over the ribbon.
    const FVector2D Bar(520, 14), At(Size.X * .5 - Bar.X * .5, RibbonAt.Y - 16);
    Box(Elements, Layer, Geometry, At - FVector2D(3, 3), Bar + FVector2D(6, 6), Srgb(.62f, .50f, .32f, .55f));
    MemoriaHudKit::Gauge(Elements, Layer + 1, Geometry, At, Bar, Hp / Max, Hp / Max, Srgb(.74f, .13f, .15f));
    Text(Elements, Layer + 4, Geometry, FString::Printf(TEXT("HP  %d / %d"), int32(Hp), int32(Max)), At + FVector2D(Bar.X * .5, Bar.Y * .5),
        MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 12), FLinearColor(.97f, .94f, .90f));
    if (Ribbon)
    {
        MemoriaHudKit::Image(Elements, Layer, Geometry, RibbonBrush, RibbonAt, FVector2D(1840, 386) * K);
        // The orbs: a number, a word under it, and a ring that empties with it.
        int32 Held = 0;
        if (Run && Run->GetPlayerMemory()) for (const auto& M : Run->GetPlayerMemory()->GetSnapshot().Owned) Held += !M.bBurned && !M.bFaded ? 1 : 0;
        const FVector2D Left = RibbonAt + FVector2D(160, 188) * K, Right = RibbonAt + FVector2D(1680, 188) * K;
        MemoriaHudKit::Arc(Elements, Layer + 1, Geometry, Left, 70 * K, Hp / Max, 3.5f, Srgb(.92f, .26f, .22f, .95f));
        Text(Elements, Layer + 2, Geometry, FString::FromInt(int32(Hp)), Left - FVector2D(0, 5), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 22), Srgb(1.f, .90f, .84f));
        Text(Elements, Layer + 2, Geometry, TEXT("HP"), Left + FVector2D(0, 16), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 10), Srgb(.86f, .62f, .56f));
        MemoriaHudKit::Arc(Elements, Layer + 1, Geometry, Right, 70 * K, FMath::Clamp(Held / 12.f, 0.f, 1.f), 3.5f, Srgb(.56f, .78f, 1.f, .95f));
        Text(Elements, Layer + 2, Geometry, FString::FromInt(Held), Right - FVector2D(0, 5), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 22), Srgb(.90f, .95f, 1.f));
        Text(Elements, Layer + 2, Geometry, Ko ? TEXT("기억") : TEXT("MEM"), Right + FVector2D(0, 16), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 10), Srgb(.62f, .76f, .92f));
    }
    else Box(Elements, Layer, Geometry, RibbonAt + FVector2D(150, 70), FVector2D(1840 * K - 300, 110), Srgb(.03f, .025f, .04f, .82f));
    // S314: Arrel's statuses beside the gauge.
    float TagX = At.X - 12.f;
    auto Tag = [&](const FString& Label, const FLinearColor& Color)
    {
        const FSlateFontInfo Font = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 14);
        const float W = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font).X + 18.f;
        TagX -= W;
        Box(Elements, Layer + 1, Geometry, FVector2D(TagX, At.Y - 4), FVector2D(W, Bar.Y + 8), FLinearColor(Color.R * .25f, Color.G * .25f, Color.B * .25f, .9f));
        TextAt(Elements, Layer + 3, Geometry, Label, FVector2D(TagX + W * .5f, At.Y + Bar.Y * .5f), .5f, Font, Color);
        TagX -= 8.f;
    };
    const int32 WeakSeconds = FMath::CeilToInt(C->GetWeakenLeft()), PoisonTicks = C->GetPoisonTicksLeft();
    if (C->IsWeakened()) Tag(Ko ? FString::Printf(TEXT("약화 %d초"), WeakSeconds) : FString::Printf(TEXT("Weak %ds"), WeakSeconds), Srgb(.80f, .66f, 1.f));
    if (C->IsPoisoned()) Tag(Ko ? FString::Printf(TEXT("중독 ×%d"), PoisonTicks) : FString::Printf(TEXT("Poison ×%d"), PoisonTicks), Srgb(.60f, 1.f, .42f));
    if (C->IsBlocking()) Tag(Ko ? TEXT("막기") : TEXT("Guard"), Srgb(.78f, .86f, 1.f));
    // The cells. S319: Elia's techniques show their name (??? until her diary unlocks it) and the cooldown
    // draining from the cell; then the burn, the guard and the dodge.
    struct FCell { float X0, X1; };
    static const FCell Cells[7] = {{290, 461}, {478, 650}, {675, 829}, {847, 996}, {1011, 1160}, {1178, 1359}, {1378, 1545}};
    const FSlateFontInfo KeyFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 11), NameFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13);
    for (int32 I = 0; I < 7; ++I)
    {
        const FVector2D CellAt = RibbonAt + FVector2D(Cells[I].X0 + 6, 131) * K, Cell = FVector2D(Cells[I].X1 - Cells[I].X0 - 12, 155) * K;
        const FVector2D Centre = CellAt + Cell * .5;
        FString Key, Name; bool bOpen = true, bLit = false; float Cool = 0.f; FLinearColor Hue = Srgb(.62f, .82f, 1.f);
        if (I < 4)
        {
            bOpen = C->IsEliaSkillUnlocked(I);
            Cool = bOpen ? C->GetEliaCooldown(I) / MemoriaCombatTuning::EliaSkills[I].Cooldown : 0.f;
            Key = FString::FromInt(I + 1);
            Name = bOpen ? FString(Ko ? MemoriaCombatTuning::EliaSkills[I].NameKo : MemoriaCombatTuning::EliaSkills[I].Name) : FString(TEXT("???"));
        }
        else if (I == 4) { Key = TEXT("R"); Name = Ko ? TEXT("기억 연소") : TEXT("Burn"); Hue = Srgb(1.f, .62f, .30f); bLit = C->IsPickingBurn() || C->IsCasting(); }
        else if (I == 5) { Key = Ko ? TEXT("우클릭") : TEXT("RMB"); Name = Ko ? TEXT("막기") : TEXT("Guard"); Hue = Srgb(.82f, .88f, 1.f); bLit = C->IsBlocking(); }
        else { Key = TEXT("Shift"); Name = Ko ? TEXT("회피") : TEXT("Dodge"); Hue = Srgb(.82f, .88f, 1.f); }
        if (bLit) Box(Elements, Layer + 1, Geometry, CellAt, Cell, FLinearColor(Hue.R, Hue.G, Hue.B, .16f));
        if (Cool > 0.f) Box(Elements, Layer + 1, Geometry, CellAt, FVector2D(Cell.X, Cell.Y * Cool), Srgb(.0f, .0f, .01f, .62f));
        Box(Elements, Layer + 1, Geometry, CellAt + FVector2D(Cell.X * .2, Cell.Y - 3), FVector2D(Cell.X * .6, 2), !bOpen ? Srgb(.3f, .32f, .36f, .4f) : Cool > 0.f ? Srgb(.4f, .44f, .5f, .6f) : FLinearColor(Hue.R, Hue.G, Hue.B, .9f));
        TextAt(Elements, Layer + 3, Geometry, Key, CellAt + FVector2D(Cell.X * .5, 13), .5f, KeyFont, !bOpen ? Srgb(.42f, .42f, .44f) : Srgb(.74f, .70f, .62f));
        // A name of two words stands on two lines.
        FString First = Name, Second;
        const FLinearColor Ink = !bOpen ? Srgb(.40f, .40f, .42f) : Cool > 0.f ? Srgb(.55f, .60f, .66f) : FLinearColor(FMath::Lerp(Hue.R, 1.f, .5f), FMath::Lerp(Hue.G, 1.f, .5f), FMath::Lerp(Hue.B, 1.f, .5f));
        if (MemoriaHudKit::Width(Name, NameFont) > Cell.X - 6 && Name.Split(TEXT(" "), &First, &Second, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
        {
            TextAt(Elements, Layer + 3, Geometry, First, Centre + FVector2D(0, -2), .5f, NameFont, Ink);
            TextAt(Elements, Layer + 3, Geometry, Second, Centre + FVector2D(0, 17), .5f, NameFont, Ink);
        }
        else TextAt(Elements, Layer + 3, Geometry, Name, Centre + FVector2D(0, 8), .5f, NameFont, Ink);
    }
    if (const APawn* Arrel = C->GetPlayer(); Arrel && C->IsShielded())
        Arc(Elements, Layer, Geometry, PC, Arrel->GetActorLocation() - FVector(0, 0, 4.f), 64.f, 1.f, 3.f, FLinearColor(.62f, .82f, 1.f, .8f));
    // The diary notice after a burn (EliaDiary toasts), under the burn banner.
    if (C->GetEliaNoticeAge() < 3.5f && !C->GetEliaNotice().IsEmpty())
    {
        const float A = FMath::Clamp(FMath::Min(C->GetEliaNoticeAge() / .3f, (3.5f - C->GetEliaNoticeAge()) / .6f), 0.f, 1.f);
        TArray<FString> NoteLines; C->GetEliaNotice().ParseIntoArrayLines(NoteLines);
        for (int32 N = 0; N < NoteLines.Num(); ++N)
            Text(Elements, Layer + 6, Geometry, NoteLines[N], FVector2D(Size.X * .5, 300 + 30.f * N), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, N == 0 ? 17 : 19),
                N == 0 ? Srgb(.72f, .80f, .90f, A) : Srgb(.62f, .85f, 1.f, A));
    }
    // S314: the won fight's rewards (source Win), above the bar for a few seconds.
    const FMemoriaFieldReward& Won = C->GetLastReward();
    if (Won.Age < RewardShown)
    {
        const float A = FMath::Clamp(FMath::Min(Won.Age / .2f, (RewardShown - Won.Age) / .6f), 0.f, 1.f);
        FString Line = FString::Printf(TEXT("Grains +%lld"), Won.Grains);
        if (Won.Heal > 0) Line += FString::Printf(TEXT("    HP +%lld"), Won.Heal);
        if (!Won.ItemName.IsEmpty()) Line += Ko ? FString::Printf(TEXT("    %s 획득"), *Won.ItemName) : FString::Printf(TEXT("    %s found"), *Won.ItemName);
        const FVector2D Center(Size.X * .5, At.Y - 92.f);
        Box(Elements, Layer + 4, Geometry, Center - FVector2D(300, 40), FVector2D(600, 80), FLinearColor(.03f, .02f, .015f, .82f * A));
        Box(Elements, Layer + 4, Geometry, Center - FVector2D(300, 40), FVector2D(600, 2), Srgb(.85f, .66f, .30f, A));
        Text(Elements, Layer + 5, Geometry, Ko ? TEXT("전투 승리") : TEXT("Victory"), Center - FVector2D(0, 16), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 24), Srgb(1.f, .86f, .52f, A));
        Text(Elements, Layer + 5, Geometry, Line, Center + FVector2D(0, 18), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 16), Srgb(.92f, .88f, .80f, A));
    }
    // The attack has no cell: its hint stands over the gauge while a fight is on.
    if (C->LiveMonsterCount() > 0)
        Text(Elements, Layer + 3, Geometry, Ko ? TEXT("좌클릭 / J  공격     길게 눌러 회전베기") : TEXT("LMB / J  Attack     hold to spin"),
            At + FVector2D(Bar.X * .5, -18), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.86f, .82f, .76f, .9f));
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
