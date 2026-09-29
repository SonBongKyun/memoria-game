#include "Combat/MemoriaExplorationHudWidget.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Engine/GameInstance.h"
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
// Left-aligned text, vertically centred on Y.
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& At, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D Top = At - FVector2D(0, Size.Y * .5);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Top + FVector2D(1.5, 1.5)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .8f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Top))), S, Font, ESlateDrawEffect::None, Color);
}
const UMemoriaRunSubsystem* RunOf(const UUserWidget& W) { return W.GetGameInstance() ? W.GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr; }
}
bool UMemoriaExplorationHudWidget::IsShowing() const
{
    const auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
    const auto* Run = RunOf(*this);
    return Narrative && Run && Run->HasActiveRun() && Narrative->GetState() == EMemoriaSliceState::Exploration;
}
void UMemoriaExplorationHudWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    // S57's ghost bar: it holds the old value briefly, then drains toward the real HP.
    if (const auto* Run = RunOf(*this))
    {
        const float Hp = float(Run->GetRunSnapshot().Player.Hp);
        GhostHp = GhostHp < 0.f || GhostHp < Hp ? Hp : FMath::FInterpConstantTo(GhostHp, Hp, DeltaSeconds, 30.f);
    }
}
int32 UMemoriaExplorationHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* Run = RunOf(*this);
    if (!Run || !IsShowing()) return Layer;
    const auto State = Run->GetRunSnapshot();
    const bool Ko = State.CurrentLocale == TEXT("ko");
    int64 Held = 0, Burned = 0, Items = 0;
    if (const auto* Memory = Run->GetPlayerMemory())
    {
        const auto Snapshot = Memory->GetSnapshot();
        for (const auto& M : Snapshot.Owned) if (!M.bBurned && !M.bFaded) ++Held;
        Burned = Snapshot.BurnedHistory.Num();
    }
    for (const auto& Item : State.Player.Items) Items += Item.Count;
    // exploration_hud.gd rows: HP, chapter and place, memories, grains, items.
    Lines.Reset();
    Lines.Add(FString::Printf(TEXT("HP  %lld / %lld"), State.Player.Hp, State.Player.MaxHp));
    Lines.Add(FString::Printf(TEXT("Ch.%lld — %s"), State.CurrentChapter, Ko ? *PlaceKo : *PlaceEn));
    Lines.Add(Ko ? FString::Printf(TEXT("기억  보유 %lld · 연소 %lld"), Held, Burned) : FString::Printf(TEXT("Memories: %lld held, %lld burned"), Held, Burned));
    Lines.Add(FString::Printf(TEXT("Grains  %lld"), State.Player.Grains));
    Lines.Add(Ko ? FString::Printf(TEXT("아이템  %lld"), Items) : FString::Printf(TEXT("Items: %lld"), Items));
    // The source panel: dark, a thin amber border, compact rows.
    const FVector2D Size = Geometry.GetLocalSize(), PanelSize(330, 172), At(Size.X - PanelSize.X - 36, 34);
    Box(Elements, Layer, Geometry, At - FVector2D(1.5, 1.5), PanelSize + FVector2D(3, 3), Srgb(.70f, .56f, .34f, .42f));
    Box(Elements, Layer, Geometry, At, PanelSize, Srgb(.030f, .024f, .040f, .72f));
    const FSlateFontInfo Font = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15);
    // HP: the ghost (lighter, trailing) under the real bar.
    const float Max = FMath::Max(1.f, float(State.Player.MaxHp));
    const FVector2D Bar(150, 12), BarAt = At + FVector2D(16, 22 - 6);
    Box(Elements, Layer + 1, Geometry, BarAt, Bar, Srgb(.08f, .07f, .10f, .9f));
    Box(Elements, Layer + 2, Geometry, BarAt, FVector2D(Bar.X * FMath::Clamp(GhostHp / Max, 0.f, 1.f), Bar.Y), Srgb(.85f, .35f, .30f, .6f));
    Box(Elements, Layer + 3, Geometry, BarAt, FVector2D(Bar.X * FMath::Clamp(float(State.Player.Hp) / Max, 0.f, 1.f), Bar.Y), Srgb(.78f, .14f, .16f));
    Text(Elements, Layer + 4, Geometry, FString::Printf(TEXT("%lld/%lld"), State.Player.Hp, State.Player.MaxHp), At + FVector2D(178, 22), Font, Srgb(.93f, .90f, .86f));
    const FLinearColor Colors[4] = {Srgb(.80f, .76f, .70f), Srgb(.62f, .60f, .66f), Srgb(.85f, .70f, .35f), Srgb(.55f, .75f, .55f)};
    for (int32 I = 1; I < Lines.Num(); ++I)
        Text(Elements, Layer + 4, Geometry, Lines[I], At + FVector2D(16, 22 + 30.f * I), Font, Colors[I - 1]);
    return Layer + 6;
}
