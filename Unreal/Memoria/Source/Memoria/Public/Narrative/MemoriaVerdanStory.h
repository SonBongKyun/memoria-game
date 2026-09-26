#pragma once
#include "CoreMinimal.h"

// Source verdan_market.gd _setup_exploration_events, in authored order.
// SourceRect keeps the Godot Area2D (pixels, tile 32) for parity checks; the development
// courtyard is not the Godot layout, so Location places each beat where it reads there.
struct FMemoriaVerdanStoryBeat
{
    const TCHAR* Group;
    const TCHAR* Flag;
    const TCHAR* RequiresFlag; // Evaluated once when Verdan is entered, as the source _ready guard.
    const TCHAR* Asset;
    FIntRect SourceRect;
    FVector Location;
    const TCHAR* Prompt;
    const TCHAR* Title; // Dialogue header naming where the beat happens.
};

// verdan_market.gd sets these on Elia in this order; PerceptionFilter takes the first
// burned, unheard one before her regular talk.
struct FMemoriaEliaReaction { const TCHAR* Memory; const TCHAR* File; const TCHAR* Group; const TCHAR* Asset; };

namespace MemoriaVerdanStory
{
    MEMORIA_API const TArray<FMemoriaEliaReaction>& EliaReactions();
    inline const TCHAR* EliaDialogueKey = TEXT("elia_ch2_talk");
    inline const TCHAR* EliaTalkFlag = TEXT("talked_Elia_elia_ch2_talk");
    inline const TCHAR* EliaRepeatLine = TEXT("This market smells like rust and regret.");
    MEMORIA_API const TArray<FMemoriaVerdanStoryBeat>& Beats();
    MEMORIA_API const FMemoriaVerdanStoryBeat* Find(const FString& Group);
    constexpr double InteractionRange = 80.0;
}
