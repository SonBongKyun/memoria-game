#include "Combat/MemoriaDefenseFeedbackS364.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool MemoriaDefenseFeedbackS364::IsEnabled()
{
    // Same-code presentation A/B; this is not a game option or saved preference.
    return !FParse::Param(FCommandLine::Get(), TEXT("MemoriaGraphicsS364Baseline"));
}
FString MemoriaDefenseFeedbackS364::PopupText(const FMemoriaCombatPopup& Popup, bool bKo)
{
    if (Popup.Outcome != EMemoriaDefenseOutcome::Block || Popup.Amount <= 0.f) return Popup.Label;
    const FString Label = Popup.Label.IsEmpty() ? (bKo ? TEXT("막음") : TEXT("Blocked")) : Popup.Label;
    // Amount is the actual HP loss already committed by StrikePlayer, not the
    // incoming damage or an independently recomputed mitigation prediction.
    return Label + FString::Printf(TEXT(" · -%d HP"), FMath::RoundToInt(Popup.Amount));
}
MemoriaDefenseFeedbackS364::FShape MemoriaDefenseFeedbackS364::Shape(const FMemoriaImpact& Impact, bool bReduceMotion, float FlashScale)
{
    FShape Out;
    if (Impact.Outcome == EMemoriaDefenseOutcome::None || Impact.Life <= 0.f) return Out;
    const float T = FMath::Clamp(Impact.Age / Impact.Life, 0.f, 1.f), Fade = 1.f - T;
    if (Fade <= 0.f) return Out;
    const float Open = bReduceMotion ? 1.f : .78f + .22f * (1.f - FMath::Square(1.f - T));
    const float R = (Impact.Outcome == EMemoriaDefenseOutcome::Block ? 31.f : Impact.Outcome == EMemoriaDefenseOutcome::Parry ? 39.f : 35.f) * Open;
    const FLinearColor Hue = Impact.Outcome == EMemoriaDefenseOutcome::Block ? FLinearColor(.38f,.72f,1.f) :
        Impact.Outcome == EMemoriaDefenseOutcome::Parry ? FLinearColor(1.f,.79f,.30f) : FLinearColor(.66f,.76f,1.f);
    auto Line = [&](FVector2D A, FVector2D B, bool bCore = false)
    {
        FLine L; L.A = A * R; L.B = B * R; L.Thickness = bCore ? 1.8f : 2.5f;
        L.Color = Hue; L.Color.A = Fade * (bCore ? .70f * FMath::Clamp(FlashScale,0.f,1.f) : .9f);
        if (L.Color.A <= .001f) return;
        Out.Lines.Add(L);
        const FVector2D Pad(L.Thickness*.5f,L.Thickness*.5f);
        Out.Bounds += L.A-Pad; Out.Bounds += L.A+Pad; Out.Bounds += L.B-Pad; Out.Bounds += L.B+Pad;
    };
    if (Impact.Outcome == EMemoriaDefenseOutcome::Block)
    {
        // A contained shield, rather than the attack's expanding floor ring.
        const FVector2D P[] = {{-.65,-.78},{.65,-.78},{.73,-.12},{.45,.55},{0,.88},{-.45,.55},{-.73,-.12},{-.65,-.78}};
        for (int32 I=1;I<UE_ARRAY_COUNT(P);++I) Line(P[I-1],P[I]);
        Line({0,-.52},{0,.46},true); Line({-.32,-.12},{.32,-.12},true);
    }
    else if (Impact.Outcome == EMemoriaDefenseOutcome::Parry)
    {
        // Open gold wings and a central crest distinguish precise parries.
        const FVector2D P[] = {{0,-.9},{.32,-.18},{0,.48},{-.32,-.18},{0,-.9}};
        for (int32 I=1;I<UE_ARRAY_COUNT(P);++I) Line(P[I-1],P[I]);
        Line({-.30,-.12},{-.88,-.52}); Line({-.88,-.52},{-.67,.10});
        Line({.30,-.12},{.88,-.52}); Line({.88,-.52},{.67,.10});
        Line({0,-.46},{0,.16},true); Line({-.16,-.16},{.16,-.16},true);
    }
    else
    {
        // Ink's spent ward has a double diamond, not the parry crest.
        const FVector2D P[] = {{0,-.90},{.76,0},{0,.90},{-.76,0},{0,-.90}};
        for (int32 I=1;I<UE_ARRAY_COUNT(P);++I) Line(P[I-1],P[I]);
        Line({0,-.48},{.39,0},true); Line({.39,0},{0,.48},true);
        Line({0,.48},{-.39,0},true); Line({-.39,0},{0,-.48},true);
        Line({-.96,0},{-.82,0}); Line({.82,0},{.96,0});
    }
    return Out;
}
