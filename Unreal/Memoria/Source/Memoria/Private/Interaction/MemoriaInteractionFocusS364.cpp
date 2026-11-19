#include "Interaction/MemoriaInteractionFocusS364.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool MemoriaInteractionFocusS364::IsEnabled()
{
    // Same-code visual A/B only; this is not a setting or saved preference.
    return !FParse::Param(FCommandLine::Get(), TEXT("MemoriaGraphicsS364Baseline"));
}
bool MemoriaInteractionFocusS364::CueBounds(const FVector2D& Anchor, const FVector2D& Viewport, FBox2D& Out)
{
    Out = FBox2D(ForceInit);
    if (Anchor.ContainsNaN() || Viewport.ContainsNaN() || Viewport.X <= 0. || Viewport.Y <= 0.) return false;
    // Includes the dark under-stroke around the brackets and the small diamond.
    const FBox2D Candidate(Anchor - FVector2D(27, 37), Anchor + FVector2D(27, 17));
    if (Candidate.Min.X < 8. || Candidate.Min.Y < 8. || Candidate.Max.X > Viewport.X - 8. || Candidate.Max.Y > Viewport.Y - 8.) return false;
    Out = Candidate;
    return true;
}
