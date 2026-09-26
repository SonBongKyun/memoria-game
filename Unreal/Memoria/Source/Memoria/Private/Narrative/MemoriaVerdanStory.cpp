#include "Narrative/MemoriaVerdanStory.h"

namespace MemoriaVerdanStory
{
const TArray<FMemoriaVerdanStoryBeat>& Beats()
{
    // Rects are Vector2(x * TILE_SIZE, y * TILE_SIZE) with the authored size, TILE_SIZE 32.
    // Locations stay clear of Malet's range, the arrival spawn and the courtyard edges.
    static const TArray<FMemoriaVerdanStoryBeat> Values = {
        {TEXT("verdan_market_walk"), TEXT("ch2_market_walk"), nullptr, TEXT("DA_Field_VerdanMarketWalk"),
            FIntRect(320, 256, 416, 320), FVector(0, 270, 0), TEXT("Memory stalls  |  E / A: look"), TEXT("VERDAN  /  MEMORY MARKET")},
        {TEXT("verdan_old_burner"), TEXT("ch2_old_burner"), nullptr, TEXT("DA_Field_VerdanOldBurner"),
            FIntRect(96, 160, 160, 224), FVector(-700, 120, 0), TEXT("Old man  |  E / A: talk"), TEXT("VERDAN  /  MARKET EDGE")},
        {TEXT("malet_backstory"), TEXT("ch2_malet_backstory"), TEXT("malet_deal_accepted"), TEXT("DA_Field_MaletBackstory"),
            FIntRect(448, 384, 512, 448), FVector(420, -160, 0), TEXT("Malet's table  |  E / A: sit"), TEXT("THE SUMP  /  MALET")},
        {TEXT("elia_sump_concern"), TEXT("ch2_elia_concern"), nullptr, TEXT("DA_Field_EliaSumpConcern"),
            FIntRect(512, 448, 576, 512), FVector(620, -380, 0), TEXT("Elia  |  E / A: listen"), TEXT("THE SUMP  /  ENTRANCE")},
        {TEXT("sump_atmosphere"), TEXT("ch2_sump_atmos"), nullptr, TEXT("DA_Field_SumpAtmosphere"),
            FIntRect(448, 512, 544, 576), FVector(420, -470, 0), TEXT("Sump stairs  |  E / A: look"), TEXT("THE SUMP")},
    };
    return Values;
}
const TArray<FMemoriaEliaReaction>& EliaReactions()
{
    static const TArray<FMemoriaEliaReaction> Values = {
        {TEXT("daily_campfire_song"), TEXT("data/chapter1_dialogue.json"), TEXT("elia_song_burned"), TEXT("DA_Field_EliaSongBurned")},
        {TEXT("identity_first_sword"), TEXT("data/chapter1_dialogue.json"), TEXT("elia_sword_burned"), TEXT("DA_Field_EliaSwordBurned")},
    };
    return Values;
}
const FMemoriaVerdanStoryBeat* Find(const FString& Group)
{
    return Beats().FindByPredicate([&](const FMemoriaVerdanStoryBeat& B){ return Group.Equals(B.Group, ESearchCase::CaseSensitive); });
}
}
