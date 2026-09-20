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
    };
    return Values;
}
UPaperSprite* LoadSprite(const FString& Name)
{
    const FString Asset = TEXT("SPR_") + Name;
    return LoadObject<UPaperSprite>(nullptr, *(Package(Asset) + TEXT(".") + Asset));
}
}
