#include "Combat/MemoriaCombatFeedbackS365.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool MemoriaCombatFeedbackS365::IsEnabled()
{ return !FParse::Param(FCommandLine::Get(), TEXT("MemoriaGraphicsS365Baseline")); }

void MemoriaCombatFeedbackS365::PlacePopups(TArray<FMemoriaPopupPaintRecordS365>& Entries, const FVector2D& Viewport)
{
    constexpr double Margin = 4., Gap = 4.;
    TArray<FBox2D> Placed;
    for (auto& Entry : Entries)
    {
        const FVector2D Extent = Entry.Bounds.GetSize();
        const FVector2D Min(Margin, Margin), Max(FMath::Max(Margin, Viewport.X-Margin-Extent.X), FMath::Max(Margin, Viewport.Y-Margin-Extent.Y));
        const FVector2D Desired = Entry.Bounds.Min;
        const auto Clamp = [&](const FVector2D& At) { return FVector2D(FMath::Clamp(At.X,Min.X,Max.X),FMath::Clamp(At.Y,Min.Y,Max.Y)); };
        const FVector2D Initial = Clamp(Desired);
        TArray<double> X{Initial.X}, Y{Initial.Y};
        // Every nearest free rectangle has an edge on the viewport or on one of
        // the already placed rectangles. Search those bounded candidates; no
        // pixel stepping, font shrinking, dropping or invented aggregate text.
        X.AddUnique(Min.X); X.AddUnique(Max.X); Y.AddUnique(Min.Y); Y.AddUnique(Max.Y);
        for (const auto& Other : Placed)
        { X.AddUnique(FMath::Clamp(Other.Min.X-Gap-Extent.X,Min.X,Max.X)); X.AddUnique(FMath::Clamp(Other.Max.X+Gap,Min.X,Max.X));
          Y.AddUnique(FMath::Clamp(Other.Min.Y-Gap-Extent.Y,Min.Y,Max.Y)); Y.AddUnique(FMath::Clamp(Other.Max.Y+Gap,Min.Y,Max.Y)); }
        FVector2D Best = Initial; double BestDistance = TNumericLimits<double>::Max(); bool Found = false;
        for (double CY : Y) for (double CX : X)
        {
            const FVector2D At(CX,CY); const FBox2D Candidate(At,At+Extent);
            bool Clear = Extent.X <= Viewport.X-2.*Margin && Extent.Y <= Viewport.Y-2.*Margin;
            for (const auto& Other : Placed)
                if (Candidate.Min.X < Other.Max.X+Gap && Candidate.Max.X > Other.Min.X-Gap && Candidate.Min.Y < Other.Max.Y+Gap && Candidate.Max.Y > Other.Min.Y-Gap) { Clear=false; break; }
            const double Distance = (At-Desired).SizeSquared();
            if (Clear && Distance < BestDistance) { Best=At; BestDistance=Distance; Found=true; }
        }
        // A finite viewport cannot fit unbounded simultaneous text. Keep every
        // truthful entry visible at its clamped anchor and expose overload in
        // the paint probe instead of silently discarding or coalescing events.
        Entry.bOverloaded = !Found;
        Entry.Bounds = FBox2D(Best,Best+Extent);
        Entry.PaintedCenter = Entry.DesiredCenter + (Best-Desired);
        Placed.Add(Entry.Bounds);
    }
}

void MemoriaCombatFeedbackS365::FHealthTrail::Observe(float Health, float MaxHealth, float DeltaSeconds, bool bSnap)
{
    Health=FMath::Clamp(Health,0.f,FMath::Max(0.f,MaxHealth));
    if (!bReady || bSnap || MaxHealth<=0.f || MaxHealth!=Max || Health>Last || Health<=0.f)
    { Last=Value=Health; Max=MaxHealth; Hold=0.f; bReady=true; return; }
    const float Delta=FMath::Max(0.f,DeltaSeconds);
    if (Health<Last) { Value=FMath::Max(Value,Last); Hold=.16f; }
    else
    {
        const float Drain=FMath::Max(0.f,Delta-Hold); Hold=FMath::Max(0.f,Hold-Delta);
        Value=FMath::Max(Health,Value-MaxHealth*1.8f*Drain);
    }
    Last=Health; Max=MaxHealth;
}
