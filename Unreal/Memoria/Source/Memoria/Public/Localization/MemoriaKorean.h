#pragma once
#include "CoreMinimal.h"
// S369 Korean patch: one place that turns the game's remaining fixed English into Korean when the run (or,
// before a run, the settings) is Korean. It is applied where text is drawn (HUD, prompts, notices, dialogue
// frame, menus, popups), so the systems underneath keep their English keys and the Godot-oracle tests keep
// comparing English. Unknown text (already Korean story lines, names) passes through unchanged.
namespace MemoriaKorean
{
// The run's locale when a run is active, else the settings' locale (Korean by default).
MEMORIA_API bool IsKorean(const UObject* WorldContext);
// The Korean for a fixed English string (exact table, then the prompt / notice / header patterns, then the
// chapter maps' own table); the input unchanged when none is known.
MEMORIA_API FString Text(const FString& English);
inline FString For(const UObject* WorldContext, const FString& English) { return IsKorean(WorldContext) ? Text(English) : English; }
// The memory's Korean title and description (the catalog's localized text, then the chapter memories').
MEMORIA_API bool MemoryText(const FString& MemoryId, FString& OutTitle, FString& OutDescription);
// "N Grains" in the run's language.
MEMORIA_API FString Grains(int64 Amount, bool bKorean);
}
