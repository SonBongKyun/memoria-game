#include "Combat/MemoriaCombatHudWidget.h"
#include "Localization/MemoriaKorean.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaCombatInputRouter.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaDefenseFeedbackS364.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Combat/MemoriaAttackTelegraph.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaHudKit.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaUiKit.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
namespace
{
using MemoriaUiKit::Srgb;
TArray<FString> WrapBurnText(const FString& Text, const FSlateFontInfo& Font, float MaxWidth)
{
    TArray<FString> Lines, Paragraphs;
    Text.Replace(TEXT("\r"), TEXT("")).ParseIntoArray(Paragraphs, TEXT("\n"), false);
    for (const FString& Paragraph : Paragraphs)
    {
        FString Line;
        for (TCHAR Character : Paragraph)
        {
            if (!Line.IsEmpty() && MemoriaHudKit::Width(Line + FString::Chr(Character), Font) > MaxWidth)
            {
                int32 Break = INDEX_NONE;
                for (int32 I = Line.Len() - 1; I > 0; --I) if (FChar::IsWhitespace(Line[I])) { Break = I; break; }
                if (Break > 0) { Lines.Add(Line.Left(Break)); Line = Line.Mid(Break + 1); }
                else { Lines.Add(Line); Line.Reset(); }
            }
            Line.AppendChar(Character);
        }
        Lines.Add(Line);
    }
    return Lines;
}
constexpr float RewardShown = MemoriaHudLayoutS363::RewardShownSeconds; // seconds the victory rewards stay up
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
void UMemoriaCombatHudWidget::UnbindPickerState()
{
    if (auto* Bound = Combat.Get(); Bound && PickerVisibilityHandle.IsValid())
        Bound->OnBurnPickerVisibilityChanged.Remove(PickerVisibilityHandle);
    PickerVisibilityHandle.Reset();
}
void UMemoriaCombatHudWidget::Bind(UMemoriaFieldCombatSubsystem* InCombat)
{
    if (Combat.Get() != InCombat || !PickerVisibilityHandle.IsValid())
    {
        if (Combat.Get() != InCombat) { HealthTrails.Reset(); HealthTrailFrame = MAX_uint64; }
        UnbindPickerState(); Combat = InCombat;
        if (InCombat) PickerVisibilityHandle = InCombat->OnBurnPickerVisibilityChanged.AddUObject(this, &UMemoriaCombatHudWidget::SyncViewportLayer);
    }
    SyncViewportLayer();
}
void UMemoriaCombatHudWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    // Presentation follows world/gameplay time, not Slate's wall-clock delta.
    if (GetWorld() && HealthTrailFrame != GFrameCounter)
    {
        HealthTrailFrame = GFrameCounter;
        const auto* C = Combat.Get();
        const auto* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
        const bool bSnap = !MemoriaCombatFeedbackS365::IsEnabled() || (Settings && Settings->IsReduceMotion());
        TSet<TWeakObjectPtr<AMemoriaFieldMonster>> Current;
        if (C) for (const auto& Weak : C->GetMonsters())
            if (auto* M = Weak.Get(); M && !M->IsDead())
            {
                Current.Add(Weak);
                HealthTrails.FindOrAdd(Weak).Observe(M->GetHealth(),M->GetMaxHealth(),GetWorld()->IsPaused()?0.f:GetWorld()->GetDeltaSeconds(),bSnap);
            }
        for (auto It=HealthTrails.CreateIterator();It;++It) if (!Current.Contains(It.Key())) It.RemoveCurrent();
    }
    // Fallback for initial attachment; normal Open/Close synchronizes in the game tick.
    SyncViewportLayer();
}
void UMemoriaCombatHudWidget::SyncViewportLayer()
{
    const auto* Owner = GetOwningPlayer();
    if (bChangingViewportLayer || !GetWorld() || GetWorld()->bIsTearingDown || !Owner || Owner->IsActorBeingDestroyed()) return;
    auto* Viewport = UGameViewportSubsystem::Get();
    if (!Viewport || !Viewport->IsWidgetAdded(this)) return;
    const bool bWantPicker = Combat.IsValid() && Combat->IsPickingBurn();
    if (bWantPicker == bPickerRaised) return;
    auto Placement = Viewport->GetWidgetSlot(this);
    if (bWantPicker) { PreviousViewportOrder = Placement.ZOrder; Placement.ZOrder = 100; }
    else Placement.ZOrder = PreviousViewportOrder;
    // UE5.8 SetWidgetSlot updates the inner Canvas only. Reinsert the viewport
    // host to change its actual sibling Overlay order, then keep exact layout.
    // The transient reinsert must not be mistaken for an owner/world teardown.
    bChangingViewportLayer = true; bPickerRaised = bWantPicker;
    RemoveFromParent();
    AddToViewport(Placement.ZOrder);
    if (Viewport->IsWidgetAdded(this)) Viewport->SetWidgetSlot(this, Placement);
    bChangingViewportLayer = false;
}
void UMemoriaCombatHudWidget::NativeDestruct()
{
    if (!bChangingViewportLayer) { bPickerRaised = false; UnbindPickerState(); HealthTrails.Reset(); HealthTrailFrame = MAX_uint64; }
    Super::NativeDestruct();
}
void UMemoriaCombatHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Bind(Combat.Get());
    Ribbon = MemoriaHudKit::Load(MemoriaHudKit::EArt::Ribbon);
    MemoriaHudKit::Brush(RibbonBrush, Ribbon);
}
bool UMemoriaCombatHudWidget::IsShowing() const
{
    const auto* C = Combat.Get();
    return C && MemoriaHudLayoutS363::IsRibbonShowing(*C);
}
int32 UMemoriaCombatHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const auto* C = Combat.Get();
    APlayerController* PC = GetOwningPlayer();
    const auto* SliceController = Cast<AMemoriaSliceController>(PC);
    const auto* InputRouter = SliceController ? SliceController->GetCombatInputRouter() : nullptr;
    const bool bPad = InputRouter && InputRouter->IsUsingGamepad();
    // The game over screen (S318) takes over from the fall's veil.
    PickerProbe = FMemoriaBurnPickerPaintProbe();
    RibbonProbe = FMemoriaRibbonPaintProbe();
    DefenseProbe = FMemoriaDefensePaintProbe(); DefenseProbe.Frame = GFrameCounter;
    FeedbackProbe = FMemoriaCombatFeedbackPaintProbe(); FeedbackProbe.Frame = GFrameCounter;
    if (!C || !PC || !IsShowing() || C->IsAwaitingGameOver()) return Layer;
    const auto* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
    const float FlashScale = Settings ? Settings->FlashScale() : 1.f;
    const float DecorativeOpacity = Settings && Settings->IsReduceMotion() ? .2f : 1.f;
    const FVector2D Size = Geometry.GetLocalSize();
    const bool bFeedbackEnabled = MemoriaCombatFeedbackS365::IsEnabled();
    FeedbackProbe.bDrawn = true; FeedbackProbe.bEnabled = bFeedbackEnabled; FeedbackProbe.Viewport = Size;
    const bool bDefenseEnabled = MemoriaDefenseFeedbackS364::IsEnabled();
    const bool bReduceMotion = Settings && Settings->IsReduceMotion();
    DefenseProbe.bDrawn = true; DefenseProbe.bEnabled = bDefenseEnabled;
    DefenseProbe.bReduceMotion = bReduceMotion; DefenseProbe.FlashScale = FlashScale; DefenseProbe.Viewport = Size;
    // The run's locale, like the memory titles and the story text (a new game takes it from the settings).
    const auto* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>() : nullptr;
    const bool Ko = !Run || Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    MemoriaAttackTelegraph::Paint(Elements, Layer, Geometry, PC, *C);
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
        if (Flash > 0.f) Box(Elements, Layer, Geometry, FVector2D::ZeroVector, Size, Toward(Hue, 0.f, .30f * Flash * Flash * FlashScale));
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
            Segment(Elements, Layer, Geometry, PC, A, B, Along == 1.f ? 5.f : 3.f, FLinearColor(.85f, .92f, 1.f, Fade * DecorativeOpacity * (Along == 1.f ? .85f : .45f)));
        }
        Segment(Elements, Layer, Geometry, PC, Trail[I].Base, Trail[I].Tip, 1.5f, FLinearColor(.8f, .88f, 1.f, Fade * .18f * DecorativeOpacity));
    }
    for (const FMemoriaSpark& Spark : C->GetSparks())
    {
        // A short streak along the spark's flight, with a soft glow under it.
        const float Life = FMath::Clamp(1.f - Spark.Age / Spark.Life, 0.f, 1.f), Width = Spark.Size * (.5f + .5f * Life);
        const FLinearColor Hot(FMath::Lerp(Spark.Color.R, 1.f, Life * .6f), FMath::Lerp(Spark.Color.G, 1.f, Life * .6f), FMath::Lerp(Spark.Color.B, 1.f, Life * .45f), Life * DecorativeOpacity);
        const FVector Tail = Spark.Location - Spark.Velocity * .035f;
        Segment(Elements, Layer, Geometry, PC, Tail, Spark.Location, Width * 2.4f, FLinearColor(Spark.Color.R, Spark.Color.G, Spark.Color.B, .28f * Life * DecorativeOpacity));
        Segment(Elements, Layer + 1, Geometry, PC, Tail, Spark.Location, Width, Hot);
    }
    // S340: where a blow landed, a ring opens and fades; a heavy one also throws a cross of light.
    for (const FMemoriaImpact& Mark : C->GetImpacts())
    {
        if (bDefenseEnabled && Mark.Outcome != EMemoriaDefenseOutcome::None)
        {
            FVector2D At;
            if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Mark.Location, At, false))
            {
                const auto Shape = MemoriaDefenseFeedbackS364::Shape(Mark, bReduceMotion, FlashScale);
                if (!Shape.Lines.IsEmpty())
                {
                    for (const auto& Line : Shape.Lines)
                        FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(),
                            TArray<FVector2f>{FVector2f(At+Line.A),FVector2f(At+Line.B)}, ESlateDrawEffect::None, Line.Color, true, Line.Thickness);
                    FMemoriaDefenseShapePaintRecord Record; Record.Outcome = Mark.Outcome;
                    Record.Bounds = FBox2D(At+Shape.Bounds.Min,At+Shape.Bounds.Max); Record.Segments = Shape.Lines.Num();
                    Record.bReduceMotion = bReduceMotion; Record.FlashScale = FlashScale; Record.Age = Mark.Age;
                    DefenseProbe.Shapes.Add(MoveTemp(Record));
                }
            }
            continue;
        }
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
        const float Current = M->GetHealth(), Maximum = FMath::Max(1.f,M->GetMaxHealth());
        const auto* TrailState = HealthTrails.Find(Weak);
        const float Recent = bFeedbackEnabled && !bReduceMotion && TrailState ? FMath::Clamp(TrailState->Value,Current,Maximum) : Current;
        const FVector2D BarAt = At-FVector2D(Bar.X*.5,0);
        const float FillWidth = Bar.X*Current/Maximum, TrailWidth = Bar.X*Recent/Maximum;
        if (TrailWidth>FillWidth+.01f)
            Box(Elements,Layer+1,Geometry,BarAt+FVector2D(FillWidth,0),FVector2D(TrailWidth-FillWidth,Bar.Y),Srgb(.94f,.77f,.48f,.85f));
        Box(Elements, Layer + 2, Geometry, BarAt, FVector2D(FillWidth, Bar.Y), M->Spec().bVoid ? Srgb(.62f, .12f, .78f) : Srgb(.80f, .42f, .16f));
        FMemoriaHealthPaintRecordS365 Health; Health.Target=Weak; Health.Current=Current; Health.Trail=Recent; Health.Max=Maximum;
        Health.Bounds=FBox2D(BarAt,BarAt+Bar); Health.bTrailVisible=TrailWidth>FillWidth+.01f;
        if(Health.bTrailVisible) Health.RecentDamageBounds=FBox2D(BarAt+FVector2D(FillWidth,0),BarAt+FVector2D(TrailWidth,Bar.Y));
        FeedbackProbe.Health.Add(MoveTemp(Health));
        // S342: the pools' foes are named over their bar, since two kinds can share a model.
        if (M->Spec().bRetint)
            Text(Elements, Layer + 2, Geometry, Ko ? M->Spec().NameKo : M->Spec().Name, At - FVector2D(0, 12), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.90f, .86f, .80f, .92f));
    }
    // Collect the exact glyphs before painting, so defense words, damage,
    // healing and status text share one measured screen-space collision pass.
    struct FPopupStyle { FSlateFontInfo Font; FLinearColor Ink; EMemoriaDefenseOutcome Outcome; };
    TArray<FPopupStyle> PopupStyles;
    const auto& Popups = C->GetPopups();
    for (int32 I=0; I<Popups.Num(); ++I)
    {
        const auto& P=Popups[I]; FVector2D At;
        if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC,P.Location,At,false)) continue;
        const bool bWord=!P.Label.IsEmpty(), bDefense=P.Outcome!=EMemoriaDefenseOutcome::None;
        const float Alpha=FMath::Clamp((bWord?2.2f:1.4f)-P.Age*1.4f,0.f,1.f);
        if(Alpha<=.001f) continue;
        const auto Base=P.Tint.A>0.f?P.Tint:P.bPlayer?FLinearColor(1.f,.28f,.24f):FLinearColor(1.f,.82f,.46f);
        const float Pop=1.f+.55f*FMath::Square(FMath::Clamp(1.f-P.Age/.16f,0.f,1.f));
        const auto Ink=P.bBig&&!bWord?FLinearColor(FMath::Lerp(Base.R,1.f,.35f),FMath::Lerp(Base.G,1.f,.35f),FMath::Lerp(Base.B,1.f,.35f)):Base;
        const FString Painted=bDefense?(bDefenseEnabled?MemoriaDefenseFeedbackS364::PopupText(P,Ko):P.Label):bWord?(Ko?MemoriaKorean::Text(P.Label):P.Label):FString::Printf(TEXT("%d"),FMath::RoundToInt(P.Amount));
        const float DefensePop=bDefenseEnabled&&bReduceMotion?1.f:Pop;
        const int32 Points=bDefense?FMath::RoundToInt(22.f*DefensePop*(bDefenseEnabled&&Settings?Settings->TextScale():1.f)):
            FMath::RoundToInt((bWord?22:P.bPlayer?30:P.bBig?46:34)*Pop);
        const auto Font=MemoriaFonts::Get(bWord?MemoriaFonts::EStyle::Ui:MemoriaFonts::EStyle::Title,Points);
        const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Painted,Font);
        FVector2D Center=At-FVector2D(0,bDefense&&bDefenseEnabled&&bReduceMotion?0.f:(bWord?40.f:60.f)*P.Age);
        if(bDefense&&bDefenseEnabled)
        {
            Center.X=FMath::Clamp(Center.X,Extent.X*.5+4.,FMath::Max(Extent.X*.5+4.,Size.X-Extent.X*.5-5.5));
            Center.Y=FMath::Clamp(Center.Y,Extent.Y*.5+4.,FMath::Max(Extent.Y*.5+4.,Size.Y-Extent.Y*.5-5.5));
        }
        FMemoriaPopupPaintRecordS365 Record; Record.SourceIndex=I; Record.Text=Painted; Record.Amount=P.Amount; Record.Alpha=Alpha;
        Record.DesiredCenter=Record.PaintedCenter=Center; Record.Bounds=FBox2D(Center-Extent*.5,Center+Extent*.5+FVector2D(1.5,1.5));
        FeedbackProbe.Popups.Add(MoveTemp(Record));
        PopupStyles.Add(FPopupStyle{Font,Srgb(Ink.R,Ink.G,Ink.B,Alpha),P.Outcome});
    }
    if(bFeedbackEnabled) MemoriaCombatFeedbackS365::PlacePopups(FeedbackProbe.Popups,Size);
    for(int32 I=0;I<FeedbackProbe.Popups.Num();++I)
    {
        const auto& P=FeedbackProbe.Popups[I]; const auto& Paint=PopupStyles[I];
        Text(Elements,Layer+2,Geometry,P.Text,P.PaintedCenter,Paint.Font,Paint.Ink);
        if(Paint.Outcome!=EMemoriaDefenseOutcome::None)
        {
            FMemoriaDefensePopupPaintRecord Record;Record.Outcome=Paint.Outcome;Record.Text=P.Text;Record.Amount=P.Amount;Record.Alpha=P.Alpha;Record.Bounds=P.Bounds;
            DefenseProbe.Popups.Add(MoveTemp(Record));
        }
    }
    // S340: a wound reddens the screen's edge for a moment; under a third of his HP the edge keeps a slow pulse.
    {
        const float Share = C->GetPlayerMaxHp() > 0 ? float(C->GetPlayerHp()) / float(C->GetPlayerMaxHp()) : 1.f;
        const float Struck = FMath::Square(FMath::Clamp(1.f - C->GetHurtAge() / UMemoriaFieldCombatSubsystem::HurtTime, 0.f, 1.f)) * .5f * FlashScale;
        const float Low = Share < .3f && !C->IsDefeated() ? (Settings && Settings->IsReduceFlash() ? .06f : .16f + .1f * FMath::Sin(float(FPlatformTime::Seconds()) * 4.6f)) : 0.f;
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
    const auto RibbonLayout = MemoriaHudLayoutS363::Ribbon(Size, C->LiveMonsterCount() > 0, C->GetLastReward().Age < RewardShown);
    const float K = RibbonLayout.Scale;
    const FVector2D RibbonAt = RibbonLayout.Origin;
    RibbonProbe.bDrawn = true; RibbonProbe.Frame = GFrameCounter; RibbonProbe.Viewport = Size; RibbonProbe.bUsingGamepad = bPad;
    RibbonProbe.ArtBounds = FBox2D(RibbonAt, RibbonAt + FVector2D(1840, 386) * K);
    RibbonProbe.OccupiedBounds = RibbonLayout.OccupiedBounds;
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
        RibbonProbe.MemoryOrbBounds = FBox2D(Right - FVector2D(70, 70) * K, Right + FVector2D(70, 70) * K);
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
    if (C->IsScorched()) Tag(Ko ? FString::Printf(TEXT("화상 ×%d"), C->GetScorchTicksLeft()) : FString::Printf(TEXT("Scorch ×%d"), C->GetScorchTicksLeft()), Srgb(1.f, .62f, .30f));
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
            static const TCHAR* PadSkills[] = {TEXT("LT+↑"),TEXT("LT+→"),TEXT("LT+↓"),TEXT("LT+←")};
            Key = bPad ? FString(PadSkills[I]) : FString::FromInt(I + 1);
            Name = bOpen ? FString(Ko ? MemoriaCombatTuning::EliaSkills[I].NameKo : MemoriaCombatTuning::EliaSkills[I].Name) : FString(TEXT("???"));
        }
        else if (I == 4) { Key = bPad ? TEXT("Y") : TEXT("R"); Name = Ko ? TEXT("기억 연소") : TEXT("Burn"); Hue = Srgb(1.f, .62f, .30f); bLit = C->IsPickingBurn() || C->IsCasting(); }
        else if (I == 5) { Key = bPad ? TEXT("LB") : Ko ? TEXT("우클릭") : TEXT("RMB"); Name = Ko ? TEXT("막기") : TEXT("Guard"); Hue = Srgb(.82f, .88f, 1.f); bLit = C->IsBlocking(); }
        else { Key = bPad ? TEXT("B") : TEXT("Shift"); Name = Ko ? TEXT("회피") : TEXT("Dodge"); Hue = Srgb(.82f, .88f, 1.f); }
        RibbonProbe.ActionCells.Add(FBox2D(CellAt, CellAt + Cell)); RibbonProbe.ActionKeys.Add(Key);
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
    // S342: a caster's orb: a bright point with a tail, and its shadow ring on the floor so its path can be read.
    for (const FMemoriaOrb& Orb : C->GetOrbs())
    {
        const float Fade = FMath::Clamp((MemoriaCombatTuning::OrbLife - Orb.Age) / .3f, 0.f, 1.f);
        const FVector Tail = Orb.Location - Orb.Velocity * .09f;
        Segment(Elements, Layer + 1, Geometry, PC, Tail, Orb.Location, 16.f, FLinearColor(Orb.Color.R, Orb.Color.G, Orb.Color.B, .30f * Fade));
        // The orb itself: a bright core inside a soft halo (rings in the orb's own plane, thick enough to fill).
        Ring(Elements, Layer + 1, Geometry, PC, Orb.Location, 30.f, 7.f, FLinearColor(Orb.Color.R, Orb.Color.G, Orb.Color.B, .40f * Fade));
        Ring(Elements, Layer + 2, Geometry, PC, Orb.Location, 15.f, 10.f, Toward(Orb.Color, .45f, .95f * Fade));
        Ring(Elements, Layer + 2, Geometry, PC, Orb.Location, 5.f, 8.f, Toward(Orb.Color, .9f, Fade));
        Ring(Elements, Layer, Geometry, PC, FVector(Orb.Location.X, Orb.Location.Y, -6.f), MemoriaCombatTuning::OrbRadius * .6f, 2.f, FLinearColor(Orb.Color.R, Orb.Color.G, Orb.Color.B, .45f * Fade));
    }
    // S341: a firebomb in the air (a hot point on a low arc, with a short tail), and the ink's ward round Arrel.
    for (const FMemoriaThrown& Bomb : C->GetThrown())
    {
        auto Fly = [&](float T) { return FMath::Lerp(Bomb.From, Bomb.To, T) + FVector(0, 0, 150.f * FMath::Sin(T * PI)); };
        const float T = FMath::Clamp(Bomb.Age / MemoriaCombatTuning::BombFlight, 0.f, 1.f);
        Segment(Elements, Layer + 1, Geometry, PC, Fly(FMath::Max(0.f, T - .16f)), Fly(T), 9.f, FLinearColor(1.f, .5f, .15f, .35f));
        Segment(Elements, Layer + 2, Geometry, PC, Fly(FMath::Max(0.f, T - .05f)), Fly(T), 6.f, FLinearColor(1.f, .86f, .5f, 1.f));
        Ring(Elements, Layer, Geometry, PC, Bomb.To, MemoriaCombatTuning::BombRadius, 2.f, FLinearColor(1.f, .45f, .15f, .35f));
    }
    if (const APawn* Arrel = C->GetPlayer(); Arrel && C->IsWarded())
    {
        Arc(Elements, Layer, Geometry, PC, Arrel->GetActorLocation() - FVector(0, 0, 4.f), 78.f, 1.f, 2.f, FLinearColor(.72f, .8f, 1.f, .7f));
        Arc(Elements, Layer, Geometry, PC, Arrel->GetActorLocation() + FVector(0, 0, 60.f), 58.f, 1.f, 1.5f, FLinearColor(.72f, .8f, 1.f, .35f));
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
        FString Line = Ko ? FString::Printf(TEXT("그레인 +%lld"), Won.Grains) : FString::Printf(TEXT("Grains +%lld"), Won.Grains);
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
        Text(Elements, Layer + 3, Geometry, bPad ? (Ko ? TEXT("RB  공격     길게 눌러 회전베기") : TEXT("RB  Attack     hold to spin")) :
                (Ko ? TEXT("좌클릭 / J  공격     길게 눌러 회전베기") : TEXT("LMB / J  Attack     hold to spin")),
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
    const auto* Fight = Combat.Get(); const auto& Choices = Fight->GetBurnChoices();
    const int32 SelectedIndex = Fight->GetBurnSelection();
    if (!Choices.IsValidIndex(SelectedIndex)) return;
    const auto& Choice = Choices[SelectedIndex];
    const auto* SliceController = Cast<AMemoriaSliceController>(GetOwningPlayer());
    const auto* InputRouter = SliceController ? SliceController->GetCombatInputRouter() : nullptr;
    const bool bPad = InputRouter && InputRouter->IsUsingGamepad();
    const auto* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaSettingsSubsystem>() : nullptr;
    const float TextScale = Settings ? Settings->TextScale() : 1.f;
    const FVector2D Size = Geometry.GetLocalSize();
    const float K = FMath::Clamp(FMath::Min(float(Size.X) / 1040.f, float(Size.Y) / 720.f), .1f, 1.f);
    const float Margin = 24.f * K, PanelPadding = 24.f * K;
    const float Width = FMath::Min(1200.f * K, float(Size.X) - 2.f * Margin), MaxHeight = float(Size.Y) - 2.f * Margin;
    const float TextWidth = Width - 2.f * PanelPadding;
    const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    auto Font = [&](int32 Base, MemoriaFonts::EStyle Style = MemoriaFonts::EStyle::Ui)
    { return MemoriaFonts::Get(Style, FMath::Max(8, FMath::RoundToInt((Settings ? Settings->ScaledTextSize(Base) : Base) * K))); };
    auto Height = [&](const FSlateFontInfo& Face) { return float(Measure->Measure(TEXT("Ag가"), Face).Y); };
    auto FitsFont = [&](const FString& Line, FSlateFontInfo Face, float Available)
    { while (Face.Size > 8 && MemoriaHudKit::Width(Line, Face) > Available) --Face.Size; return Face; };
    const auto TitleFont = Font(28, MemoriaFonts::EStyle::Title), DangerFont = Font(13), LabelFont = Font(12), MetaFont = Font(12);
    const auto RowTitleFont = Font(19), RowDetailFont = Font(13), RowDamageFont = Font(24, MemoriaFonts::EStyle::Title);
    FSlateFontInfo InfoFont = Font(18);
    const FString Consequence = Choice.KnownConsequence.IsEmpty() ? (Ko ? TEXT("기록된 영향 없음") : TEXT("No recorded consequence")) : Choice.KnownConsequence;
    const FString Warning = Fight->IsBurnArmed()
        ? (Ko ? FString::Printf(TEXT("한 번 더 누르면 「%s」은(는) 영원히 사라진다"), *Choice.Title) : FString::Printf(TEXT("Press again and \"%s\" is gone forever"), *Choice.Title))
        : bPad ? (Ko ? TEXT("방향키 ↑↓로 전체 기억 살펴보기") : TEXT("D-pad ↑↓ to browse every memory"))
        : (Ko ? TEXT("↑↓ · W/S · 휠로 전체 기억 살펴보기") : TEXT("↑↓ · W/S · wheel to browse every memory"));
    const FString Controls = bPad ? (Ko ? TEXT("A / Y 확인     B 뒤로") : TEXT("A / Y Confirm     B Back"))
        : (Ko ? TEXT("1–9 고르기     R · Enter 태우기     Esc · 우클릭 그만두기") : TEXT("1–9 Choose     R · Enter Burn     Esc · RMB Let it be"));
    const auto WarningFont = FitsFont(Warning, Font(16), TextWidth), ControlsFont = FitsFont(Controls, Font(14), TextWidth);
    const float LabelHeight = Height(LabelFont), MetaHeight = Height(MetaFont);
    const float TopHeight = PanelPadding + Height(TitleFont) + 3.f * K + Height(DangerFont) + 12.f * K;
    const float FooterHeight = 14.f * K + Height(WarningFont) + 6.f * K + Height(ControlsFont) + PanelPadding;
    const float RowHeight = FMath::Max(64.f * K, Height(RowTitleFont) + Height(RowDetailFont) + 14.f * K);
    TArray<FString> DescriptionLines, ConsequenceLines; float LineHeight = 0.f, InfoHeight = 0.f;
    auto Reflow = [&]
    {
        LineHeight = Height(InfoFont) + 3.f * K;
        DescriptionLines = WrapBurnText(Choice.Description, InfoFont, TextWidth);
        ConsequenceLines = WrapBurnText(Consequence, InfoFont, TextWidth);
        InfoHeight = 2.f * LabelHeight + 6.f * K + (DescriptionLines.Num() + ConsequenceLines.Num()) * LineHeight + 8.f * K;
    };
    Reflow();
    const int32 MinimumRows = FMath::Min(2, Choices.Num());
    auto FixedHeight = [&] { return TopHeight + InfoHeight + 10.f * K + MetaHeight + 8.f * K + FooterHeight; };
    // Allocate the entire selected meaning before rows. On a constrained viewport, reduce the number
    // of visible rows first; an 8-point emergency fallback still wraps every authored character.
    while (InfoFont.Size > 8 && MaxHeight - FixedHeight() < MinimumRows * RowHeight) { --InfoFont.Size; Reflow(); }
    const int32 Capacity = FMath::Clamp(FMath::FloorToInt((MaxHeight - FixedHeight()) / RowHeight), MinimumRows, 6);
    const int32 VisibleRows = FMath::Min(Capacity, Choices.Num());
    const int32 FirstRow = (SelectedIndex / FMath::Max(1, VisibleRows)) * VisibleRows;
    const int32 Pages = FMath::DivideAndRoundUp(Choices.Num(), FMath::Max(1, VisibleRows)), Page = SelectedIndex / FMath::Max(1, VisibleRows) + 1;
    const FVector2D Extent(Width, FixedHeight() + VisibleRows * RowHeight), Panel = (Size - Extent) * .5f;
    PickerProbe.bDrawn = true; PickerProbe.Viewport = Size; PickerProbe.PanelBounds = FBox2D(Panel, Panel + Extent);
    PickerProbe.Description = Choice.Description; PickerProbe.AuthoredConsequence = Choice.KnownConsequence; PickerProbe.PaintedConsequence = Consequence;
    PickerProbe.DescriptionLines = DescriptionLines; PickerProbe.ConsequenceLines = ConsequenceLines;
    PickerProbe.Selected = SelectedIndex; PickerProbe.First = FirstRow; PickerProbe.Visible = FMath::Min(VisibleRows, Choices.Num() - FirstRow);
    PickerProbe.Pages = Pages; PickerProbe.Page = Page; PickerProbe.TextScale = TextScale; PickerProbe.InfoFontSize = FMath::RoundToInt(InfoFont.Size);
    Box(Elements, Layer, Geometry, FVector2D::ZeroVector, Size, FLinearColor(.02f, .006f, .004f, .52f));
    Box(Elements, Layer + 1, Geometry, Panel - FVector2D(2, 2) * K, Extent + FVector2D(4, 4) * K, Srgb(.55f, .24f, .10f, .85f));
    Box(Elements, Layer + 1, Geometry, Panel, Extent, Srgb(.055f, .035f, .03f, .97f));
    float Y = Panel.Y + PanelPadding;
    TextAt(Elements, Layer + 3, Geometry, Ko ? TEXT("무엇을 태우겠는가") : TEXT("What will you burn?"), FVector2D(Panel.X + Width * .5f, Y + Height(TitleFont) * .5f), .5f, TitleFont, Srgb(1.f, .80f, .56f));
    Y += Height(TitleFont) + 3.f * K;
    TextAt(Elements, Layer + 3, Geometry, Ko ? TEXT("태운 기억은 돌아오지 않는다") : TEXT("A burned memory never returns"), FVector2D(Panel.X + Width * .5f, Y + Height(DangerFont) * .5f), .5f, DangerFont, Srgb(.78f, .56f, .44f));
    Y += Height(DangerFont) + 12.f * K;
    TextAt(Elements, Layer + 3, Geometry, Ko ? TEXT("기억") : TEXT("Memory"), FVector2D(Panel.X + PanelPadding, Y + LabelHeight * .5f), 0.f, LabelFont, Srgb(.80f, .65f, .48f));
    Y += LabelHeight + 3.f * K;
    PickerProbe.DescriptionBounds = FBox2D(FVector2D(Panel.X + PanelPadding, Y), FVector2D(Panel.X + Width - PanelPadding, Y + DescriptionLines.Num() * LineHeight));
    for (const auto& Line : DescriptionLines) { TextAt(Elements, Layer + 3, Geometry, Line, FVector2D(Panel.X + PanelPadding, Y + LineHeight * .5f), 0.f, InfoFont, Srgb(.90f, .84f, .75f)); Y += LineHeight; }
    Y += 8.f * K;
    TextAt(Elements, Layer + 3, Geometry, Ko ? TEXT("기록된 대가") : TEXT("Recorded cost"), FVector2D(Panel.X + PanelPadding, Y + LabelHeight * .5f), 0.f, LabelFont, Srgb(.87f, .60f, .40f));
    Y += LabelHeight + 3.f * K;
    PickerProbe.ConsequenceBounds = FBox2D(FVector2D(Panel.X + PanelPadding, Y), FVector2D(Panel.X + Width - PanelPadding, Y + ConsequenceLines.Num() * LineHeight));
    for (const auto& Line : ConsequenceLines) { TextAt(Elements, Layer + 3, Geometry, Line, FVector2D(Panel.X + PanelPadding, Y + LineHeight * .5f), 0.f, InfoFont, Srgb(.95f, .71f, .53f)); Y += LineHeight; }
    Y += 10.f * K;
    const FString Position = Ko ? FString::Printf(TEXT("선택 %d / %d  ·  페이지 %d / %d  ·  예상 피해 %d"), SelectedIndex + 1, Choices.Num(), Page, Pages, FMath::RoundToInt(Choice.Damage))
        : FString::Printf(TEXT("Selected %d / %d  ·  Page %d / %d  ·  Expected damage %d"), SelectedIndex + 1, Choices.Num(), Page, Pages, FMath::RoundToInt(Choice.Damage));
    TextAt(Elements, Layer + 3, Geometry, Position, FVector2D(Panel.X + Width * .5f, Y + MetaHeight * .5f), .5f, FitsFont(Position, MetaFont, TextWidth), Srgb(.84f, .72f, .60f));
    Y += MetaHeight + 8.f * K;
    for (int32 I = FirstRow; I < Choices.Num() && I < FirstRow + VisibleRows; ++I)
    {
        const auto& Row = Choices[I]; const bool bSelected = I == SelectedIndex; const FVector2D At(Panel.X + PanelPadding, Y + (I - FirstRow) * RowHeight);
        if (bSelected) { Box(Elements, Layer + 2, Geometry, At, FVector2D(TextWidth, RowHeight - 6.f * K), Srgb(.34f, .13f, .06f, .92f)); Box(Elements, Layer + 2, Geometry, At, FVector2D(5.f * K, RowHeight - 6.f * K), Row.Accent); }
        const float Middle = At.Y + (RowHeight - 6.f * K) * .5f;
        const FLinearColor Ink = bSelected ? Srgb(1.f, .94f, .86f) : Srgb(.80f, .74f, .68f);
        if (I < 9) TextAt(Elements, Layer + 3, Geometry, FString::FromInt(I + 1), FVector2D(At.X + 22.f * K, Middle), .5f, RowDetailFont, Srgb(.62f, .50f, .40f));
        Box(Elements, Layer + 3, Geometry, FVector2D(At.X + 44.f * K, Middle - 5.f * K), FVector2D(10, 10) * K, Row.Accent);
        TextAt(Elements, Layer + 3, Geometry, Row.Title, FVector2D(At.X + 66.f * K, At.Y + Height(RowTitleFont) * .5f + 3.f * K), 0.f, FitsFont(Row.Title, RowTitleFont, TextWidth - 190.f * K), Ink);
        const FString RowInfo = FString::Printf(TEXT("%s · %s"), *Row.GradeLabel, MemoriaCombatTuning::BurnSkillName(Row.Grade, Ko));
        TextAt(Elements, Layer + 3, Geometry, RowInfo, FVector2D(At.X + 66.f * K, At.Y + Height(RowTitleFont) + Height(RowDetailFont) * .5f + 5.f * K), 0.f, FitsFont(RowInfo, RowDetailFont, TextWidth - 190.f * K), Srgb(.72f, .60f, .47f));
        TextAt(Elements, Layer + 3, Geometry, FString::FromInt(FMath::RoundToInt(Row.Damage)), FVector2D(At.X + TextWidth - 14.f * K, Middle), 1.f, RowDamageFont, bSelected ? Srgb(1.f, .80f, .52f) : Srgb(.70f, .60f, .50f));
    }
    Y += VisibleRows * RowHeight + 14.f * K;
    Box(Elements, Layer + 2, Geometry, FVector2D(Panel.X + PanelPadding, Y - 7.f * K), FVector2D(TextWidth, K), Srgb(.42f, .24f, .14f, .8f));
    const float WarningWidth = MemoriaHudKit::Width(Warning, WarningFont);
    PickerProbe.WarningBounds = FBox2D(FVector2D(Panel.X + (Width - WarningWidth) * .5f, Y), FVector2D(Panel.X + (Width + WarningWidth) * .5f, Y + Height(WarningFont)));
    TextAt(Elements, Layer + 3, Geometry, Warning, FVector2D(Panel.X + Width * .5f, Y + Height(WarningFont) * .5f), .5f, WarningFont, Fight->IsBurnArmed() ? Srgb(1.f, .36f, .30f) : Srgb(.72f, .64f, .56f));
    Y += Height(WarningFont) + 6.f * K;
    const float ControlsWidth = MemoriaHudKit::Width(Controls, ControlsFont);
    PickerProbe.ControlsBounds = FBox2D(FVector2D(Panel.X + (Width - ControlsWidth) * .5f, Y), FVector2D(Panel.X + (Width + ControlsWidth) * .5f, Y + Height(ControlsFont)));
    TextAt(Elements, Layer + 3, Geometry, Controls, FVector2D(Panel.X + Width * .5f, Y + Height(ControlsFont) * .5f), .5f, ControlsFont, Srgb(.80f, .72f, .64f));
    auto Outside = [&](const FBox2D& Bounds)
    { return Bounds.Min.X < Panel.X - .5f || Bounds.Min.Y < Panel.Y - .5f || Bounds.Max.X > Panel.X + Width + .5f || Bounds.Max.Y > Panel.Y + Extent.Y + .5f; };
    for (const auto& Line : DescriptionLines) PickerProbe.bDescriptionClipped |= MemoriaHudKit::Width(Line, InfoFont) > TextWidth + .5f;
    for (const auto& Line : ConsequenceLines) PickerProbe.bConsequenceClipped |= MemoriaHudKit::Width(Line, InfoFont) > TextWidth + .5f;
    PickerProbe.bDescriptionClipped |= Outside(PickerProbe.DescriptionBounds);
    PickerProbe.bConsequenceClipped |= Outside(PickerProbe.ConsequenceBounds);
    PickerProbe.bClipped = Panel.X < 0.f || Panel.Y < 0.f || Panel.X + Width > Size.X || Panel.Y + Extent.Y > Size.Y || PickerProbe.bDescriptionClipped || PickerProbe.bConsequenceClipped || Outside(PickerProbe.WarningBounds) || Outside(PickerProbe.ControlsBounds);
}
