#pragma once
#include "CoreMinimal.h"

// Presentation only. The interaction component remains the sole target resolver.
namespace MemoriaInteractionFocusS364
{
MEMORIA_API bool IsEnabled();
// Returns the bounds of all twelve thin cue strokes. Partial/offscreen cues hide.
MEMORIA_API bool CueBounds(const FVector2D& Anchor, const FVector2D& Viewport, FBox2D& Out);
}
