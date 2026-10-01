#include "Presentation/MemoriaVerdanArt.h"
#include "PaperSprite.h"
namespace MemoriaVerdanArt
{
FString Package(const FString& Name) { return TEXT("/Game/Memoria/Presentation/Verdan/") + Name; }
const TArray<FTextureSource>& Textures()
{
    static const TArray<FTextureSource> Values = {
        {TEXT("ArrelDown"), TEXT("assets/sprites/field/arrel/down.png")},
        {TEXT("ArrelWalkDown"), TEXT("Unreal/ArtSource/Verdan/ArrelWalk_down.png")},
        {TEXT("ArrelUp"), TEXT("assets/sprites/field/arrel/up.png")},
        {TEXT("ArrelWalkUp"), TEXT("Unreal/ArtSource/Verdan/ArrelWalk_up.png")},
        {TEXT("ArrelLeft"), TEXT("assets/sprites/field/arrel/left.png")},
        {TEXT("ArrelWalkLeft"), TEXT("Unreal/ArtSource/Verdan/ArrelWalk_left.png")},
        {TEXT("ArrelRight"), TEXT("assets/sprites/field/arrel/right.png")},
        {TEXT("ArrelWalkRight"), TEXT("Unreal/ArtSource/Verdan/ArrelWalk_right.png")},
        {TEXT("Malet"), TEXT("assets/sprites/field/malet/down.png")},
        {TEXT("Market"), TEXT("assets/environment/map_canvases/map_verdan_market_canvas_v1.png")},
        {TEXT("Lantern"), TEXT("assets/environment/hybrid_depth/motif_memory_lantern_v1.png")},
        // S300 companion: the source field sprites companion.gd shows for Elia.
        {TEXT("EliaDown"), TEXT("assets/sprites/field/elia/down.png")},
        {TEXT("EliaUp"), TEXT("assets/sprites/field/elia/up.png")},
        {TEXT("EliaLeft"), TEXT("assets/sprites/field/elia/left.png")},
        {TEXT("EliaRight"), TEXT("assets/sprites/field/elia/right.png")},
        // S331 ambient NPCs: the source's procedural presets (Unreal/Tools/export_ambient_npcs.py).
        {TEXT("TravelerDown"), TEXT("Unreal/ArtSource/Ambient/traveler_down.png")},
        {TEXT("TravelerUp"), TEXT("Unreal/ArtSource/Ambient/traveler_up.png")},
        {TEXT("TravelerLeft"), TEXT("Unreal/ArtSource/Ambient/traveler_left.png")},
        {TEXT("TravelerRight"), TEXT("Unreal/ArtSource/Ambient/traveler_right.png")},
        {TEXT("BureauagentDown"), TEXT("Unreal/ArtSource/Ambient/bureau_agent_down.png")},
        {TEXT("BureauagentUp"), TEXT("Unreal/ArtSource/Ambient/bureau_agent_up.png")},
        {TEXT("BureauagentLeft"), TEXT("Unreal/ArtSource/Ambient/bureau_agent_left.png")},
        {TEXT("BureauagentRight"), TEXT("Unreal/ArtSource/Ambient/bureau_agent_right.png")},
        {TEXT("GuardDown"), TEXT("Unreal/ArtSource/Ambient/guard_down.png")},
        {TEXT("GuardUp"), TEXT("Unreal/ArtSource/Ambient/guard_up.png")},
        {TEXT("GuardLeft"), TEXT("Unreal/ArtSource/Ambient/guard_left.png")},
        {TEXT("GuardRight"), TEXT("Unreal/ArtSource/Ambient/guard_right.png")},
    };
    return Values;
}
const TArray<FSpriteRegion>& Sprites()
{
    static const TArray<FSpriteRegion> Values = {
        {TEXT("ArrelDown"), TEXT("ArrelDown"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkDown0"), TEXT("ArrelWalkDown"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkDown1"), TEXT("ArrelWalkDown"), {128, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkDown2"), TEXT("ArrelWalkDown"), {256, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkDown3"), TEXT("ArrelWalkDown"), {384, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelUp"), TEXT("ArrelUp"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkUp0"), TEXT("ArrelWalkUp"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkUp1"), TEXT("ArrelWalkUp"), {128, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkUp2"), TEXT("ArrelWalkUp"), {256, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkUp3"), TEXT("ArrelWalkUp"), {384, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelLeft"), TEXT("ArrelLeft"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkLeft0"), TEXT("ArrelWalkLeft"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkLeft1"), TEXT("ArrelWalkLeft"), {128, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkLeft2"), TEXT("ArrelWalkLeft"), {256, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkLeft3"), TEXT("ArrelWalkLeft"), {384, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelRight"), TEXT("ArrelRight"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkRight0"), TEXT("ArrelWalkRight"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkRight1"), TEXT("ArrelWalkRight"), {128, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkRight2"), TEXT("ArrelWalkRight"), {256, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("ArrelWalkRight3"), TEXT("ArrelWalkRight"), {384, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("Malet"), TEXT("Malet"), {0, 0}, {128, 160}, {64, 151}, 1.3f},
        {TEXT("Floor"), TEXT("Market"), {180, 225}, {256, 256}, {128, 128}, 1.0f},
        {TEXT("FloorInterior"), TEXT("Market"), {250, 205}, {256, 256}, {128, 128}, 1.0f},
        {TEXT("North"), TEXT("Market"), {0, 0}, {1448, 200}, {724, 200}, 1.0f},
        {TEXT("South"), TEXT("Market"), {0, 875}, {1448, 211}, {724, 0}, 1.0f},
        {TEXT("West"), TEXT("Market"), {0, 270}, {180, 540}, {180, 270}, 1.0f},
        {TEXT("East"), TEXT("Market"), {1268, 270}, {180, 540}, {0, 270}, 1.0f},
        {TEXT("Plinth"), TEXT("Market"), {730, 300}, {180, 60}, {90, 30}, 1.0f},
        {TEXT("Lantern"), TEXT("Lantern"), {0, 0}, {1024, 1536}, {512, 1381}, 10.0f},
        {TEXT("EliaDown"), TEXT("EliaDown"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("EliaUp"), TEXT("EliaUp"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("EliaLeft"), TEXT("EliaLeft"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        {TEXT("EliaRight"), TEXT("EliaRight"), {0, 0}, {128, 160}, {64, 152}, 1.3f},
        // S331 ambient NPCs: the 48 px procedural figure, pivot at its feet.
        {TEXT("TravelerDown"), TEXT("TravelerDown"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("TravelerUp"), TEXT("TravelerUp"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("TravelerLeft"), TEXT("TravelerLeft"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("TravelerRight"), TEXT("TravelerRight"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("BureauagentDown"), TEXT("BureauagentDown"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("BureauagentUp"), TEXT("BureauagentUp"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("BureauagentLeft"), TEXT("BureauagentLeft"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("BureauagentRight"), TEXT("BureauagentRight"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("GuardDown"), TEXT("GuardDown"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("GuardUp"), TEXT("GuardUp"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("GuardLeft"), TEXT("GuardLeft"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
        {TEXT("GuardRight"), TEXT("GuardRight"), {0, 0}, {48, 48}, {24, 46}, 1.0f},
    };
    return Values;
}
UPaperSprite* LoadSprite(const FString& Name)
{
    // Only the table's sprites exist; asking for another (a figure without walk frames) is not an error.
    if (!Sprites().ContainsByPredicate([&](const FSpriteRegion& Region) { return Name == Region.Name; })) return nullptr;
    const FString Asset = TEXT("SPR_") + Name;
    return LoadObject<UPaperSprite>(nullptr, *(Package(Asset) + TEXT(".") + Asset));
}
}
