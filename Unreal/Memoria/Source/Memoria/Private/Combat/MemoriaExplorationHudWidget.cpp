#include "Combat/MemoriaExplorationHudWidget.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaHudKit.h"
#include "Presentation/MemoriaUiKit.h"
#include "Engine/GameInstance.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace
{
using MemoriaUiKit::Srgb;
const UMemoriaRunSubsystem* RunOf(const UUserWidget& W) { return W.GetGameInstance() ? W.GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr; }
}
bool UMemoriaExplorationHudWidget::IsShowing() const
{
    const auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
    const auto* Run = RunOf(*this);
    return Narrative && Run && Run->HasActiveRun() && Narrative->GetState() == EMemoriaSliceState::Exploration;
}
void UMemoriaExplorationHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Plate = MemoriaHudKit::Load(MemoriaHudKit::EArt::Plate);
    MemoriaHudKit::Brush(PlateBrush, Plate);
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
    const FVector2D Size = Geometry.GetLocalSize();
    const FSlateFontInfo Font = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15);
    const float Max = FMath::Max(1.f, float(State.Player.MaxHp));
    const FString HpText = FString::Printf(TEXT("%lld / %lld"), State.Player.Hp, State.Player.MaxHp);
    const FLinearColor Colors[4] = {Srgb(.88f, .82f, .70f), Srgb(.70f, .70f, .80f), Srgb(.92f, .74f, .36f), Srgb(.60f, .80f, .60f)};
    if (!Plate)
    {
        // Without the plate (-run=MemoriaHudAssets): the plain dark panel, as before S339.
        const FVector2D PanelSize(330, 172), At(Size.X - PanelSize.X - 36, 34);
        MemoriaHudKit::Panel(Elements, Layer, Geometry, At, PanelSize);
        MemoriaHudKit::Gauge(Elements, Layer + 1, Geometry, At + FVector2D(16, 16), FVector2D(150, 12), float(State.Player.Hp) / Max, GhostHp / Max, Srgb(.78f, .14f, .16f));
        MemoriaHudKit::Text(Elements, Layer + 4, Geometry, HpText, At + FVector2D(178, 22), 0.f, Font, Srgb(.93f, .90f, .86f));
        for (int32 I = 1; I < Lines.Num(); ++I)
            MemoriaHudKit::Text(Elements, Layer + 4, Geometry, Lines[I], At + FVector2D(16, 22 + 30.f * I), 0.f, Font, Colors[I - 1]);
        return Layer + 6;
    }
    // exploration_hud.gd lays ui_exploration_hud_plate behind its panel. Here the plate is the panel: the gauge
    // sits in its slot, and each row's text over its rule, beside its icon frame. K takes the plate's own
    // pixels (844 x 438) to the viewport's units.
    constexpr float K = .62f;
    const FVector2D At(Size.X - 844 * K - 22, 12);
    MemoriaHudKit::Image(Elements, Layer, Geometry, PlateBrush, At, FVector2D(844, 438) * K, FLinearColor(1.f, 1.f, 1.f, .96f));
    MemoriaHudKit::Gauge(Elements, Layer + 1, Geometry, At + FVector2D(213, 95) * K, FVector2D(391, 21) * K, float(State.Player.Hp) / Max, GhostHp / Max, Srgb(.74f, .13f, .15f));
    MemoriaHudKit::Text(Elements, Layer + 4, Geometry, HpText, At + FVector2D(408, 105.5) * K, .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 12), Srgb(.96f, .92f, .86f));
    const float Rules[4] = {166.f, 225.f, 283.5f, 342.f};
    for (int32 I = 1; I < Lines.Num() && I <= 4; ++I)
    {
        const FVector2D Icon = At + FVector2D(204, Rules[I - 1] + 2.5f) * K;
        const FLinearColor Ink = Colors[I - 1];
        switch (I)
        {
        case 1: // the place: a waymark
            MemoriaHudKit::Diamond(Elements, Layer + 3, Geometry, Icon, FVector2D(7, 10), 1.5f, Ink);
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon - FVector2D(1.5, 1.5), FVector2D(3, 3), Ink);
            break;
        case 2: // memories: a flame of two gems
            MemoriaHudKit::Diamond(Elements, Layer + 3, Geometry, Icon, FVector2D(6, 10), 1.5f, Ink);
            MemoriaHudKit::Diamond(Elements, Layer + 3, Geometry, Icon + FVector2D(0, 2), FVector2D(3, 5), 1.5f, Srgb(.95f, .62f, .30f));
            break;
        case 3: // grains
            for (const FVector2D& Grain : {FVector2D(-5, 3), FVector2D(4, 4), FVector2D(0, -4)})
                MemoriaHudKit::Diamond(Elements, Layer + 3, Geometry, Icon + Grain, FVector2D(3.5, 3.5), 1.5f, Ink);
            break;
        default: // items: a satchel
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon + FVector2D(-7, -3), FVector2D(14, 1.5), Ink);
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon + FVector2D(-7, 7), FVector2D(14, 1.5), Ink);
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon + FVector2D(-7, -3), FVector2D(1.5, 11), Ink);
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon + FVector2D(5.5, -3), FVector2D(1.5, 11), Ink);
            MemoriaHudKit::Box(Elements, Layer + 3, Geometry, Icon + FVector2D(-3, -7), FVector2D(6, 1.5), Ink);
            break;
        }
        MemoriaHudKit::Text(Elements, Layer + 4, Geometry, Lines[I], At + FVector2D(254, Rules[I - 1] - 15.f) * K, 0.f, Font, Ink);
    }
    return Layer + 6;
}
