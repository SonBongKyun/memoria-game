#include "Combat/MemoriaHudLayoutS363.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"

bool MemoriaHudLayoutS363::IsRibbonShowing(const UMemoriaFieldCombatSubsystem& C)
{
    return C.LiveMonsterCount() > 0 || C.GetPopups().Num() > 0 || C.IsDefeated() || C.GetPlayerHp() < C.GetPlayerMaxHp() ||
        C.IsPickingBurn() || C.IsCasting() || C.GetBurnWave().bLive || C.GetLastReward().Age < RewardShownSeconds || C.IsWeakened() || C.IsPoisoned() ||
        C.GetSparks().Num() > 0 || C.GetTrail().Num() > 0 || C.GetImpacts().Num() > 0 || C.GetThrown().Num() > 0 || C.IsWarded() || C.GetOrbs().Num() > 0 || C.IsScorched() || C.GetCharge() > 0.f || C.IsBlocking() || C.GetEliaNoticeAge() < 3.5f;
}
MemoriaHudLayoutS363::FRibbon MemoriaHudLayoutS363::Ribbon(const FVector2D& Viewport, bool bAttackHint, bool bVictory)
{
    FRibbon Out;
    Out.Extent = FVector2D(1840, 386) * Out.Scale;
    Out.Origin = FVector2D(Viewport.X * .5 - Out.Extent.X * .5, Viewport.Y - Out.Extent.Y + 14);
    Out.ArtBounds = FBox2D(Out.Origin, Out.Origin + Out.Extent);
    // Original gauge border reaches 19 units above the art, and a live attack hint
    // reaches about 44. Reserve its full line and the victory panel when present.
    const float Top = bVictory ? 148.f : bAttackHint ? 48.f : 22.f;
    Out.OccupiedBounds = FBox2D(Out.Origin - FVector2D(0, Top), Out.Origin + Out.Extent);
    return Out;
}
MemoriaHudLayoutS363::FQuickTray MemoriaHudLayoutS363::QuickTray(const FVector2D& Viewport, const FBox2D* RibbonBounds)
{
    FQuickTray Out;
    Out.Width = MemoriaCombatTuning::QuickSlots * (Out.Cell.X + Out.Gap) - Out.Gap;
    Out.Origin = FVector2D(Viewport.X - 26 - Out.Width, Viewport.Y - Out.Cell.Y - 28);
    const FVector2D Border(7, 7), PaintedExtent(Out.Width + 14, Out.Cell.Y + 14);
    Out.OriginalBounds = FBox2D(Out.Origin - Border, Out.Origin - Border + PaintedExtent);
    Out.Bounds = Out.OriginalBounds;
    if (RibbonBounds && RibbonBounds->bIsValid && Out.Bounds.Intersect(*RibbonBounds))
    {
        Out.Origin.Y = RibbonBounds->Min.Y - TrayClearance - Out.Cell.Y - Border.Y;
        Out.Bounds = FBox2D(Out.Origin - Border, Out.Origin - Border + PaintedExtent);
        Out.bLifted = true;
    }
    return Out;
}
