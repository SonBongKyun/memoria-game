// S337: the chapter maps dressed past their tiles (AMemoriaChapterPresentation's terrain, light and air).
//
// The Godot maps do not show their tiles: each hides them under one painted canvas (MapEffects.add_map_canvas,
// terrain_alpha 0.0). A flat painting cannot lie under the quarter-view camera, so the port builds what the
// canvas paints: the ground from its open patches (M_ChapterGround, -run=MemoriaChapterGroundAssets), masonry
// walls with broken tops, a border that reads as a place (the Belt's rail line, Drift's ruined walls), lamps,
// dust or rain, and a world that goes on beyond the map. The props the canvas paints (signal post, platform
// shelter, crates, chain fences, tarps, the gramophone, dead trees, dry grass) are Codex's S343 models (S344,
// -run=MemoriaEnvironmentAssets). None of this changes where Arrel can walk: the blocks stay one per solid tile.
#include "Chapter/MemoriaChapterPresentation.h"
#include "Presentation/MemoriaRegionalAtmosphereS363.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"
namespace
{
const TCHAR* ChapterArt = TEXT("/Game/Memoria/Presentation/Chapter/");
const TCHAR* FocusSurfacePath = TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface");
const TCHAR* RoofPath = TEXT("/Game/Memoria/Presentation/Depth/SM_PitchedRoof.SM_PitchedRoof");
const TCHAR* ArchitecturePath = TEXT("/Game/Memoria/Presentation/EnvironmentS356/M_ArchitectureSurface.M_ArchitectureSurface");
FString ArchitectureMesh(const TCHAR* Name)
{ return FString::Printf(TEXT("/Game/Memoria/Presentation/EnvironmentS356/%s.%s"), Name, Name); }
const TCHAR* EnvArt = TEXT("/Game/Memoria/Presentation/Environment/");
const TCHAR* RubblePath = TEXT("/Game/Memoria/Presentation/Field3D/Props/SM_Rubble.SM_Rubble");
// The retained S337 100cm beveled mesh keeps the exact concrete slab transforms.
// S356 supplies its stone atlas through the copied focus shader; the old package is untouched.
const TCHAR* BevelPath = TEXT("/Game/Memoria/Presentation/VisualPolish/SM_BeveledBlock.SM_BeveledBlock");
const TCHAR* BlockMesh() { return LoadObject<UStaticMesh>(nullptr, BevelPath, nullptr, LOAD_NoWarn | LOAD_Quiet) ? BevelPath : TEXT("Cube"); }
constexpr float Feet = -8.f;   // the figures stand on z -8
// Stable per-tile and per-index scatter, so a map looks the same every visit and captures repeat.
float Hash(int32 X, int32 Y, int32 Salt = 0) { return FMath::Frac(FMath::Sin(X * 127.1f + Y * 311.7f + Salt * 74.7f) * 43758.5453f); }
double Scatter(int32 I, double Salt) { return FMath::Frac(FMath::Sin(I * 12.9898 + Salt * 78.233) * 43758.5453); }
void Blur(TArray<float>& Values, int32 W, int32 H, int32 Radius, int32 Passes)
{
    TArray<float> Other; Other.SetNumUninitialized(Values.Num());
    for (int32 Pass = 0; Pass < Passes; ++Pass)
        for (int32 Axis = 0; Axis < 2; ++Axis)
        {
            for (int32 Y = 0; Y < H; ++Y)
                for (int32 X = 0; X < W; ++X)
                {
                    float Sum = 0.f;
                    for (int32 D = -Radius; D <= Radius; ++D)
                    {
                        const int32 SX = FMath::Clamp(Axis == 0 ? X + D : X, 0, W - 1), SY = FMath::Clamp(Axis == 1 ? Y + D : Y, 0, H - 1);
                        Sum += Values[SY * W + SX];
                    }
                    Other[Y * W + X] = Sum / (2 * Radius + 1);
                }
            Swap(Values, Other);
        }
}
}
// The look of one map: not from the map script (which only names a hue, a light and a mood) but read off its
// canvas. Colours are linear.
struct AMemoriaChapterPresentation::FDressing
{
    const TCHAR* Soil; const TCHAR* Paved;
    FLinearColor GroundTint, FloorTint, WallTint, RuinTint, TimberTint, ClothTint, IronTint;
    FLinearColor LampColor, SkyColor, FogColor, MoteTint;
    float SoilRepeat, PavedRepeat, Wet, FloorMode;
    float KeyIntensity, KeyPitch, KeyYaw, FillIntensity, SkyIntensity, FogDensity, LampIntensity, Vignette;
    int32 MoteCount;
    bool bRain, bRuinBorder;
};
const AMemoriaChapterPresentation::FDressing& AMemoriaChapterPresentation::DressingFor(const FString& InMap)
{
    // The Belt Waystation: a dry relay yard on the rail line at dusk. Warm low light, dust, amber lamps.
    static const FDressing Belt{TEXT("T_BeltSoil"), TEXT("T_BeltPaved"),
        FLinearColor(.86f, .84f, .82f), FLinearColor(.085f, .052f, .03f), FLinearColor(.085f, .072f, .06f), FLinearColor(.07f, .062f, .054f),
        FLinearColor(.055f, .034f, .02f), FLinearColor(.12f, .1f, .08f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .56f, .24f), FLinearColor(.50f, .44f, .40f), FLinearColor(.075f, .056f, .04f), FLinearColor(.95f, .8f, .58f),
        230.f, 250.f, 0.f, 0.f,
        5.f, -34.f, 38.f, 1.3f, .55f, .012f, 5.2f, .5f,
        54, false, false};
    // Drift Shelter: a broken overpass camp in night rain. Cold key, wet ground, lantern light.
    static const FDressing Drift{TEXT("T_DriftSoil"), TEXT("T_DriftPaved"),
        FLinearColor(1.3f, 1.3f, 1.36f), FLinearColor(.5f, .52f, .58f), FLinearColor(.06f, .064f, .075f), FLinearColor(.05f, .053f, .062f),
        FLinearColor(.04f, .03f, .022f), FLinearColor(.09f, .082f, .07f), FLinearColor(.026f, .027f, .032f),
        FLinearColor(1.f, .6f, .27f), FLinearColor(.30f, .36f, .52f), FLinearColor(.03f, .038f, .056f), FLinearColor(.62f, .7f, .86f),
        230.f, 210.f, 1.f, 1.f,
        3.1f, -46.f, -32.f, 1.1f, .5f, .008f, 6.4f, .58f,
        120, true, true};
    // S371 The Seam: a village between cliffs where things still grow (6화: "초록이 있었다"). Warm lanterns,
    // moss on the stones, amber motes like the source's fireflies, a violet dusk over the cliffs.
    static const FDressing Seam{TEXT("T_SeamSoil"), TEXT("T_SeamPaved"),
        FLinearColor(.9f, 1.f, .86f), FLinearColor(.07f, .06f, .045f), FLinearColor(.07f, .066f, .062f), FLinearColor(.06f, .058f, .055f),
        FLinearColor(.07f, .042f, .024f), FLinearColor(.16f, .06f, .05f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .62f, .3f), FLinearColor(.42f, .36f, .5f), FLinearColor(.05f, .045f, .062f), FLinearColor(1.f, .78f, .42f),
        230.f, 230.f, .15f, 0.f,
        4.4f, -40.f, 30.f, 1.25f, .55f, .01f, 6.f, .52f,
        90, false, true};
    // S372 The listening forest: green under a closed canopy (8화: "머리 위에는 초록빛이 남아 있었다"), pale
    // mushroom light, no birds, no wind; cool, dim and still.
    static const FDressing Forest{TEXT("T_ForestSoil"), TEXT("T_ForestPaved"),
        FLinearColor(.9f, 1.f, .9f), FLinearColor(.05f, .06f, .045f), FLinearColor(.05f, .06f, .05f), FLinearColor(.045f, .05f, .042f),
        FLinearColor(.04f, .036f, .026f), FLinearColor(.06f, .07f, .05f), FLinearColor(.03f, .032f, .03f),
        FLinearColor(.78f, .92f, .86f), FLinearColor(.30f, .42f, .36f), FLinearColor(.03f, .05f, .04f), FLinearColor(.8f, .95f, .85f),
        220.f, 220.f, .35f, 0.f,
        2.6f, -58.f, 20.f, 1.05f, .5f, .02f, 3.4f, .6f,
        70, false, true};
    if (InMap == TEXT("forgotten_forest")) return Forest;
    // S373 The Crumbling Coast: grey gravel without wind or water, colours draining, the hole's blue-white light.
    static const FDressing Coast{TEXT("T_CoastSoil"), TEXT("T_CoastPaved"),
        FLinearColor(.94f, .94f, .96f), FLinearColor(.06f, .06f, .062f), FLinearColor(.06f, .06f, .064f), FLinearColor(.055f, .055f, .058f),
        FLinearColor(.045f, .04f, .036f), FLinearColor(.07f, .07f, .072f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(.74f, .86f, 1.f), FLinearColor(.52f, .54f, .6f), FLinearColor(.06f, .062f, .07f), FLinearColor(.8f, .84f, .9f),
        240.f, 230.f, .1f, 0.f,
        4.2f, -36.f, -20.f, 1.2f, .62f, .012f, 4.4f, .5f,
        24, false, true};
    if (InMap == TEXT("crumbling_coast")) return Coast;
    // S374 Haren's clearing: daylight into a living forest (13화: "햇빛이 숲 안쪽까지 들어왔다"), green but paler
    // than the Seam after the coast; a small fire's amber under the slab past the north exit (14화).
    static const FDressing Clearing{TEXT("T_ClearingSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(.95f, 1.f, .9f), FLinearColor(.05f, .06f, .045f), FLinearColor(.05f, .058f, .048f), FLinearColor(.055f, .058f, .05f),
        FLinearColor(.06f, .045f, .03f), FLinearColor(.07f, .07f, .05f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .6f, .28f), FLinearColor(.46f, .54f, .44f), FLinearColor(.05f, .062f, .045f), FLinearColor(.95f, .92f, .72f),
        220.f, 220.f, .1f, 0.f,
        3.9f, -44.f, 30.f, 1.12f, .56f, .012f, 4.f, .52f,
        40, false, true};
    if (InMap == TEXT("haren_clearing")) return Clearing;
    // S375 The Seam, leaving: the same village by morning (15화), the cloths gone from the walls, the lamps out;
    // a cooler key, a paler sky, the warm motes thinned.
    static const FDressing SeamLeaving{TEXT("T_SeamSoil"), TEXT("T_SeamPaved"),
        FLinearColor(.92f, .98f, .9f), FLinearColor(.07f, .06f, .045f), FLinearColor(.07f, .066f, .062f), FLinearColor(.06f, .058f, .055f),
        FLinearColor(.07f, .042f, .024f), FLinearColor(.12f, .07f, .06f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .7f, .4f), FLinearColor(.52f, .54f, .58f), FLinearColor(.055f, .055f, .06f), FLinearColor(.95f, .9f, .78f),
        230.f, 230.f, .1f, 0.f,
        4.6f, -46.f, 40.f, 1.2f, .6f, .011f, 3.2f, .5f,
        36, false, true};
    if (InMap == TEXT("seam_return")) return SeamLeaving;
    // S377 The roofless farmhouses: dry fields toward the Belt road in late light (17화: "해가 낮아졌을 때"), straw and
    // dust; the dry yard's earth and the clearing's trodden path; no lamps (18화: "불은 피우지 않았다").
    static const FDressing Fields{TEXT("T_BeltSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(1.02f, .98f, .86f), FLinearColor(.07f, .06f, .045f), FLinearColor(.075f, .068f, .058f), FLinearColor(.068f, .062f, .054f),
        FLinearColor(.06f, .045f, .03f), FLinearColor(.09f, .08f, .06f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .62f, .32f), FLinearColor(.58f, .52f, .44f), FLinearColor(.07f, .062f, .05f), FLinearColor(.95f, .86f, .66f),
        230.f, 220.f, 0.f, 0.f,
        4.2f, -30.f, -48.f, 1.15f, .58f, .012f, 2.4f, .5f,
        46, false, true};
    if (InMap == TEXT("roofless_farms")) return Fields;
    // S379 The basin waystation: grey daylight over a dry basin (20화), the Belt yard's earth and cracked road; low
    // broken field walls round the rim ("낮은 돌담들이 밭을 나누고"); no lamps lit by day.
    static const FDressing Basin{TEXT("T_BeltSoil"), TEXT("T_BeltPaved"),
        FLinearColor(.9f, .9f, .9f), FLinearColor(.07f, .065f, .06f), FLinearColor(.075f, .072f, .068f), FLinearColor(.066f, .064f, .06f),
        FLinearColor(.05f, .04f, .03f), FLinearColor(.08f, .08f, .08f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .7f, .4f), FLinearColor(.56f, .57f, .6f), FLinearColor(.07f, .07f, .072f), FLinearColor(.85f, .85f, .82f),
        230.f, 250.f, 0.f, 0.f,
        3.9f, -50.f, 26.f, 1.25f, .66f, .013f, 1.6f, .48f,
        30, false, true};
    if (InMap == TEXT("basin_station")) return Basin;
    // S382 Verdan, again: a market town with no wall by day (21화), cobbles, smoke from the few stalls still frying,
    // the rest packing; warm but thinning.
    static const FDressing VerdanAgain{TEXT("T_SeamPaved"), TEXT("T_BeltPaved"),
        FLinearColor(.95f, .92f, .88f), FLinearColor(.07f, .06f, .05f), FLinearColor(.08f, .07f, .06f), FLinearColor(.068f, .062f, .056f),
        FLinearColor(.07f, .045f, .028f), FLinearColor(.14f, .09f, .06f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .6f, .3f), FLinearColor(.6f, .56f, .5f), FLinearColor(.08f, .07f, .06f), FLinearColor(.95f, .82f, .62f),
        210.f, 230.f, 0.f, 0.f,
        4.4f, -46.f, 34.f, 1.2f, .6f, .012f, 2.6f, .48f,
        40, false, true};
    if (InMap == TEXT("verdan_return")) return VerdanAgain;
    // S385 The farmhouse under the ridge: dry fields and stumps under a grey sky going to a night with no fire
    // (22화: "불은 피우지 않았다"); the farm earth, a trodden road, cool light; no lamps.
    static const FDressing Ridge{TEXT("T_BeltSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(.94f, .93f, .86f), FLinearColor(.065f, .058f, .046f), FLinearColor(.075f, .07f, .062f), FLinearColor(.066f, .062f, .056f),
        FLinearColor(.055f, .042f, .03f), FLinearColor(.08f, .075f, .065f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .7f, .4f), FLinearColor(.5f, .52f, .58f), FLinearColor(.06f, .062f, .068f), FLinearColor(.84f, .84f, .86f),
        230.f, 220.f, 0.f, 0.f,
        3.6f, -42.f, -36.f, 1.15f, .62f, .013f, 1.6f, .52f,
        26, false, true};
    if (InMap == TEXT("ridge_farmhouse")) return Ridge;
    // S389 The dry wells: a dry pasture past noon (24화: "정오가 지난 뒤"), wind with little to break it; warm, plain
    // daylight; no lamps.
    static const FDressing Wells{TEXT("T_BeltSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(1.f, .97f, .88f), FLinearColor(.07f, .062f, .048f), FLinearColor(.08f, .074f, .064f), FLinearColor(.07f, .066f, .058f),
        FLinearColor(.06f, .045f, .03f), FLinearColor(.09f, .08f, .065f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .7f, .4f), FLinearColor(.6f, .62f, .66f), FLinearColor(.07f, .068f, .066f), FLinearColor(.9f, .88f, .82f),
        230.f, 220.f, 0.f, 0.f,
        4.4f, -48.f, 30.f, 1.25f, .66f, .011f, 1.6f, .46f,
        34, false, true};
    if (InMap == TEXT("dry_wells")) return Wells;
    // S398 The east road: late afternoon sinking toward evening on an open road (32화: "길 위로 비치는 빛이 낮아졌다";
    // 35화: "해는 수레 뒤쪽으로 더 낮아져"); the light low and warm from the west, behind the cart; no lamps lit.
    static const FDressing East{TEXT("T_BeltSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(1.f, .93f, .8f), FLinearColor(.07f, .06f, .045f), FLinearColor(.08f, .072f, .06f), FLinearColor(.07f, .064f, .055f),
        FLinearColor(.06f, .045f, .03f), FLinearColor(.09f, .08f, .065f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .62f, .34f), FLinearColor(.58f, .56f, .6f), FLinearColor(.07f, .065f, .06f), FLinearColor(.92f, .82f, .68f),
        230.f, 220.f, 0.f, 0.f,
        4.2f, -30.f, 160.f, 1.2f, .64f, .011f, 1.6f, .46f,
        34, false, true};
    if (InMap == TEXT("east_road")) return East;
    // S401 The storm road: evening after the yard, purple left low in the eastern sky after the storm (37화); cooler,
    // dimmer light than the east road; no lamps.
    static const FDressing Storm{TEXT("T_BeltSoil"), TEXT("T_ClearingPaved"),
        FLinearColor(.9f, .86f, .9f), FLinearColor(.065f, .058f, .05f), FLinearColor(.075f, .068f, .062f), FLinearColor(.066f, .06f, .056f),
        FLinearColor(.055f, .042f, .03f), FLinearColor(.085f, .075f, .07f), FLinearColor(.03f, .03f, .034f),
        FLinearColor(1.f, .62f, .4f), FLinearColor(.56f, .52f, .62f), FLinearColor(.066f, .062f, .066f), FLinearColor(.84f, .76f, .82f),
        230.f, 220.f, 0.f, 0.f,
        3.6f, -24.f, 160.f, 1.15f, .62f, .012f, 1.6f, .48f,
        30, false, true};
    if (InMap == TEXT("storm_road")) return Storm;
    // S393 Arcane: grey stone streets at noon under a grey sky (25화); cold, even light; no lamps by day.
    static const FDressing Arcane{TEXT("T_BeltSoil"), TEXT("T_SeamPaved"),
        FLinearColor(.82f, .84f, .88f), FLinearColor(.06f, .062f, .066f), FLinearColor(.075f, .076f, .08f), FLinearColor(.066f, .067f, .07f),
        FLinearColor(.05f, .042f, .034f), FLinearColor(.07f, .07f, .075f), FLinearColor(.03f, .03f, .032f),
        FLinearColor(1.f, .74f, .46f), FLinearColor(.56f, .58f, .64f), FLinearColor(.06f, .062f, .068f), FLinearColor(.8f, .82f, .86f),
        230.f, 210.f, 0.f, 0.f,
        3.8f, -52.f, 22.f, 1.3f, .7f, .013f, 1.2f, .5f,
        18, false, true};
    if (InMap == TEXT("arcane_city") || InMap == TEXT("arcane_leave")) return Arcane;
    return InMap == TEXT("drift_shelter") ? Drift : InMap == TEXT("the_seam") ? Seam : Belt;
}
FVector AMemoriaChapterPresentation::TileCentre(float X, float Y) const
{ return MemoriaChapterMaps::ToWorld(FVector2D((X + .5f) * Spec->TileSize, (Y + .5f) * Spec->TileSize)); }
UMaterialInstanceDynamic* AMemoriaChapterPresentation::Focus(const TCHAR* Name, const FLinearColor& Tint, float Mode, float Roughness, float Metallic)
{
    const FString Kind(Name);
    const bool bStone = Kind == TEXT("Masonry") || Kind == TEXT("WallCap") || Kind == TEXT("RuinStone") || Kind == TEXT("Slab") ||
        Kind == TEXT("BorderStone") || Kind == TEXT("BorderCap") || Kind == TEXT("SetStone") || Kind == TEXT("FieldStone");
    const bool bTimber = Kind == TEXT("Timber") || Kind == TEXT("SetTimber");
    auto* Base = LoadObject<UMaterialInterface>(nullptr, bStone || bTimber ? ArchitecturePath : FocusSurfacePath);
    auto* Result = UMaterialInstanceDynamic::Create(Base, this, FName(*(FString(TEXT("Chapter")) + Name)));
    if (bStone || bTimber) Result->SetScalarParameterValue(TEXT("AtlasRegion"), bStone ? 2.f : 3.f);
    if (bStone || bTimber) Result->SetVectorParameterValue(TEXT("ArchitectureTint"), Tint * 5.5f);
    Result->SetVectorParameterValue(TEXT("Tint"), bStone || bTimber ? Tint * 5.5f : Tint); Result->SetScalarParameterValue(TEXT("Mode"), Mode);
    Result->SetScalarParameterValue(TEXT("Roughness"), Roughness); Result->SetScalarParameterValue(TEXT("Metallic"), Metallic);
    FocusMaterials.Add(Result); return Result;
}
void AMemoriaChapterPresentation::Solid(const TCHAR* Mesh, UMaterialInterface* Material, const FVector& Position, const FVector& Scale, const FRotator& Rotation, bool bShadow)
{
    const FString Key = FString(Mesh) + TEXT("|") + Material->GetName() + (bShadow ? TEXT("") : TEXT("|flat"));
    auto& Batch = Batches.FindOrAdd(Key);
    if (!Batch)
    {
        Batch = NewObject<UInstancedStaticMeshComponent>(this);
        AddInstanceComponent(Batch); Batch->SetupAttachment(GetRootComponent()); Batch->SetMobility(EComponentMobility::Movable);
        Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision); Batch->SetGenerateOverlapEvents(false); Batch->SetCanEverAffectNavigation(false);
        const FString Path = Mesh[0] == TEXT('/') ? FString(Mesh) : FString(Mesh) == TEXT("Roof") ? FString(RoofPath) : FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Mesh, Mesh);
        Batch->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path)); Batch->SetMaterial(0, Material);
        Batch->SetCastShadow(bShadow);
        Batch->RegisterComponent();
    }
    Batch->AddInstance(FTransform(Rotation, Position, Scale), true);
}
void AMemoriaChapterPresentation::Box(UMaterialInterface* Material, const FVector& Position, const FVector& Size, const FRotator& Rotation, bool bShadow)
{ Solid(TEXT("Cube"), Material, Position, Size / 100.0, Rotation, bShadow); }
void AMemoriaChapterPresentation::Beam(UMaterialInterface* Material, const FVector& A, const FVector& B, float Width)
{ Box(Material, (A + B) * .5, FVector(Width, Width, (B - A).Size()), FRotationMatrix::MakeFromZ(B - A).Rotator()); }
void AMemoriaChapterPresentation::Lamp(const FVector& Position, float Radius, float Intensity, bool bPane)
{
    // A lantern: a glowing pane in an iron cage, and the light it throws. A model's lantern glows by itself
    // (its mask) and takes the light alone.
    if (bPane && GlowMaterial) Box(GlowMaterial, Position, FVector(7, 7, 10), FRotator::ZeroRotator, false);
    auto* Light = NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Light); Light->SetupAttachment(GetRootComponent()); Light->SetMobility(EComponentMobility::Movable);
    Light->SetWorldLocation(Position + FVector(0, -6, 2));
    Light->bUseInverseSquaredFalloff = false; Light->LightFalloffExponent = 2.2f;
    Light->SetLightColor(DressingFor(Map).LampColor); Light->SetIntensity(Intensity); Light->SetAttenuationRadius(Radius);
    Light->SetSourceRadius(10); Light->SetSoftSourceRadius(22); Light->SetCastShadows(false);
    Light->RegisterComponent(); Lamps.Add(Light); LampBase.Add(Intensity);
}
UMaterialInstanceDynamic* AMemoriaChapterPresentation::Kit(const TCHAR* Atlas, const FLinearColor& Tint, const TCHAR* Glow)
{
    const FString Key = FString::Printf(TEXT("Kit%s%s%s"), Atlas, *Tint.ToFColor(false).ToHex(), Glow ? Glow : TEXT(""));
    if (auto* Found = Kits.Find(Key)) return *Found;
    const auto Art = [](const FString& Name) { return LoadObject<UTexture2D>(nullptr, *(FString(EnvArt) + Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet); };
    auto* Base = LoadObject<UMaterialInterface>(nullptr, *(FString(EnvArt) + TEXT("M_EnvProp.M_EnvProp")), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UTexture2D* Color = Art(FString(TEXT("T_Env")) + Atlas);
    if (!Base || !Color) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_CHAPTER environment kit missing (-run=MemoriaEnvironmentAssets): %s"), Atlas); return nullptr; }
    auto* Result = UMaterialInstanceDynamic::Create(Base, this, FName(*Key));
    Result->SetTextureParameterValue(TEXT("Color"), Color); Result->SetVectorParameterValue(TEXT("Tint"), Tint);
    if (UTexture2D* Mask = Glow ? Art(FString(TEXT("T_Env")) + Glow) : nullptr)
    {
        // The lanterns' glass: the map's lamp colour, bright enough to bloom.
        const FLinearColor Lamp = DressingFor(Map).LampColor;
        Result->SetTextureParameterValue(TEXT("Glow"), Mask); Result->SetVectorParameterValue(TEXT("GlowColor"), FLinearColor(Lamp.R, Lamp.G, Lamp.B, 14.f));
    }
    Kits.Add(Key, Result); FocusMaterials.Add(Result);
    return Result;
}
void AMemoriaChapterPresentation::Prop(const TCHAR* Name, UMaterialInterface* Material, const FVector& Ground, float Yaw, const FVector& Scale, bool bShadow)
{
    // The models are in centimetres with their origin on the ground at the centre of their footprint; their
    // front faces the camera (south) at yaw 0.
    const FString Path = FString::Printf(TEXT("%sSM_Env%s.SM_Env%s"), EnvArt, Name, Name);
    if (!Material || !LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet)) return;
    Solid(*Path, Material, Ground, Scale, FRotator(0, Yaw, 0), bShadow);
    ++KitPropCount; KitKinds.Add(Name);
}
void AMemoriaChapterPresentation::Fence(UMaterialInterface* Material, const FVector& A, const FVector& B)
{
    // The fence's posts stand 160 cm apart and neighbours share one, so a run is cut into the nearest whole
    // number of lengths and each is stretched or shortened a little to end on a post.
    const FVector Along = B - A;
    const int32 Count = FMath::Max(1, FMath::RoundToInt32(Along.Size2D() / 160.f));
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
    for (int32 I = 0; I < Count; ++I)
        Prop(TEXT("FenceChain"), Material, A + Along * ((I + .5f) / Count), Yaw, FVector(Along.Size2D() / Count / 160.f, 1, 1));
}
bool AMemoriaChapterPresentation::Claimed(int32 X, int32 Y) const
{
    // The Belt's freight on its two southern ruins; Drift's stores, its gramophone and the stores by the west wall.
    static const FIntPoint Belt[] = {{3, 14}, {4, 14}, {19, 14}, {20, 14}};
    static const FIntPoint Drift[] = {{17, 2}, {18, 2}, {3, 4}, {2, 13}, {3, 13}};
    const FIntPoint Tile(X, Y);
    if (Map == TEXT("belt_waystation")) { for (const FIntPoint& P : Belt) if (P == Tile) return true; }
    else for (const FIntPoint& P : Drift) if (P == Tile) return true;
    return false;
}
void AMemoriaChapterPresentation::BuildTerrain()
{
    const FDressing& Look = DressingFor(Map);
    bRain = Look.bRain;
    GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Depth/M_Glow.M_Glow"));
    // An invisible block stands on every solid tile, as TilePainter.add_collisions does.
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    Blockers = Layer(TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr, true); Blockers->SetHiddenInGame(true);
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
            if (Spec->IsSolid(Spec->TileAt(X, Y)))
                Blockers->AddInstance(FTransform(FRotator::ZeroRotator, TileCentre(X, Y) + FVector(0, 0, 60.f), FVector(T / 100.f, T / 100.f, 1.6f)), true);
    BuildGround(Look);
    if (!LoadObject<UMaterialInterface>(nullptr, FocusSurfacePath) || !LoadObject<UStaticMesh>(nullptr, RoofPath))
    { UE_LOG(LogTemp, Error, TEXT("MEMORIA_CHAPTER surface assets missing; the map stands undressed")); return; }
    BuildWalls(Look); BuildBorder(Look); BuildSetPieces(Look); BuildGrass(Look); BuildAir(Look);
    UMaterialInstanceDynamic* OriginalStone = nullptr;
    for (const auto& Material : FocusMaterials)
        if (Material && Material->GetFName()==TEXT("ChapterRuinStone")) { OriginalStone=Material; break; }
    const auto Grounded=MemoriaRegionalAtmosphereS363::BuildChapter(*this,*Spec,Map,OriginalStone);
    for (auto* Material : Grounded.Materials) FocusMaterials.Add(Material);
}
void AMemoriaChapterPresentation::BuildGround(const FDressing& Look)
{
    // One surface for the whole ground and the land around it. The tile grid reaches the material as a mask:
    // R paved (road, path, the fallen overpass's slabs), G the interior floor, B the building walls (for grime).
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale, W = Spec->Width * T, H = Spec->Height * T, Margin = 5200.f;
    auto* Mesh = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Mesh); Mesh->SetupAttachment(GetRootComponent()); Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCastShadow(false);
    Mesh->SetWorldLocation(FVector(W * .5f, -H * .5f, Feet)); Mesh->SetWorldScale3D(FVector((W + 2 * Margin) / 100.f, (H + 2 * Margin) / 100.f, 1.f));
    auto* Base = LoadObject<UMaterialInterface>(nullptr, *(FString(ChapterArt) + TEXT("M_ChapterGround.M_ChapterGround")));
    auto Art = [](const TCHAR* Name) { return LoadObject<UTexture2D>(nullptr, *(FString(ChapterArt) + Name + TEXT(".") + Name)); };
    UTexture2D* Soil = Art(Look.Soil); UTexture2D* Paved = Art(Look.Paved);
    if (!Base || !Soil || !Paved)
    {
        // Without the painted ground (-run=MemoriaChapterGroundAssets) the map stands on its soil colour.
        UE_LOG(LogTemp, Error, TEXT("MEMORIA_CHAPTER ground assets missing"));
        Mesh->SetMaterial(0, Surface(Spec->TileColors.IsValidIndex(0) ? Spec->TileColors[0] : FLinearColor::Gray));
        Mesh->RegisterComponent(); return;
    }
    constexpr int32 R = 8;
    const int32 MW = Spec->Width * R, MH = Spec->Height * R;
    TArray<float> PavedMask, FloorMask, WallMask;
    PavedMask.Init(0.f, MW * MH); FloorMask.Init(0.f, MW * MH); WallMask.Init(0.f, MW * MH);
    const int32 WallType = Spec->TileNames.IndexOfByKey(TEXT("WALL"));
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
        {
            const int32 Type = Spec->TileAt(X, Y);
            const FString Detail = Spec->TileDetails.IsValidIndex(Type) ? Spec->TileDetails[Type] : FString();
            const bool bSolid = Spec->IsSolid(Type), bBorder = X == 0 || Y == 0 || X == Spec->Width - 1 || Y == Spec->Height - 1;
            const bool bStone = Detail == TEXT("stone_floor");
            const float P = Detail == TEXT("road") || Detail == TEXT("path") || (bStone && bSolid) ? 1.f : 0.f;
            const float F = bStone && !bSolid ? 1.f : 0.f, Wl = Type == WallType && !bBorder ? 1.f : 0.f;
            for (int32 V = 0; V < R; ++V)
                for (int32 U = 0; U < R; ++U)
                { const int32 I = (Y * R + V) * MW + X * R + U; PavedMask[I] = P; FloorMask[I] = F; WallMask[I] = Wl; }
        }
    Blur(PavedMask, MW, MH, 2, 2); Blur(FloorMask, MW, MH, 1, 1); Blur(WallMask, MW, MH, 5, 2);
    GroundMask = UTexture2D::CreateTransient(MW, MH, PF_B8G8R8A8);
    GroundMask->SRGB = false; GroundMask->Filter = TF_Bilinear; GroundMask->AddressX = TA_Clamp; GroundMask->AddressY = TA_Clamp;
    {
        auto& Mip = GroundMask->GetPlatformData()->Mips[0];
        uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
        for (int32 I = 0; I < MW * MH; ++I)
        {
            Data[I * 4 + 0] = uint8(FMath::Clamp(WallMask[I] * 1.5f, 0.f, 1.f) * 255.f);
            Data[I * 4 + 1] = uint8(FMath::Clamp(FloorMask[I], 0.f, 1.f) * 255.f);
            Data[I * 4 + 2] = uint8(FMath::Clamp(PavedMask[I], 0.f, 1.f) * 255.f);
            Data[I * 4 + 3] = 255;
        }
        Mip.BulkData.Unlock();
    }
    GroundMask->UpdateResource();
    GroundMaterial = UMaterialInstanceDynamic::Create(Base, this, TEXT("ChapterGround"));
    GroundMaterial->SetTextureParameterValue(TEXT("Soil"), Soil); GroundMaterial->SetTextureParameterValue(TEXT("Paved"), Paved);
    GroundMaterial->SetTextureParameterValue(TEXT("Mask"), GroundMask);
    GroundMaterial->SetVectorParameterValue(TEXT("MapSize"), FLinearColor(W, H, Spec->Width, Spec->Height));
    GroundMaterial->SetVectorParameterValue(TEXT("Scales"), FLinearColor(Look.SoilRepeat, Look.PavedRepeat, Look.Wet, Look.FloorMode));
    GroundMaterial->SetVectorParameterValue(TEXT("Tint"), Look.GroundTint);
    GroundMaterial->SetVectorParameterValue(TEXT("FloorTint"), Look.FloorTint);
    GroundMaterial->SetScalarParameterValue(TEXT("Relief"), 1.6f);
    // The Belt canvas's round dial lies where the yard's road meets the path to the relay house's door.
    if (Map == TEXT("belt_waystation"))
    { const FVector C = TileCentre(12.f, 12.f); GroundMaterial->SetVectorParameterValue(TEXT("InlayRect"), FLinearColor(C.X, C.Y, 150.f, 1.f)); }
    Mesh->SetMaterial(0, GroundMaterial); Mesh->RegisterComponent();
}
void AMemoriaChapterPresentation::BuildWalls(const FDressing& Look)
{
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    const int32 WallType = Spec->TileNames.IndexOfByKey(TEXT("WALL")), Concrete = Spec->TileNames.IndexOfByKey(TEXT("CONCRETE"));
    const int32 Ruin = Spec->TileNames.Contains(TEXT("RUIN")) ? Spec->TileNames.IndexOfByKey(TEXT("RUIN")) : Spec->TileNames.IndexOfByKey(TEXT("RUBBLE"));
    auto* Masonry = Focus(TEXT("Masonry"), Look.WallTint, 0, .9f);
    auto* Cap = Focus(TEXT("WallCap"), Look.WallTint * 1.3f, 0, .86f);
    auto* RuinStone = Focus(TEXT("RuinStone"), Look.RuinTint, 0, .93f);
    // Keep the S337 beveled concrete geometry, but give its broad faces the same
    // worn stone texture as the dressed walls. Focus also registers its occlusion
    // parameters, so this cosmetic binding cannot hide Arrel beside the slabs.
    auto* Slab = Focus(TEXT("Slab"), Look.RuinTint * 1.25f, 0, .9f);
    const TCHAR* Block = BlockMesh();
    auto Stand = [&](UMaterialInterface* Material, const FVector& Position, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
    { Solid(Block, Material, Position, Size / 100.0, Rotation); };
    auto* Timber = Focus(TEXT("Timber"), Look.TimberTint, 1);
    auto* Iron = Focus(TEXT("Iron"), Look.IronTint, 3, .5f, .65f);
    auto* Weeds = Focus(TEXT("RuinWeeds"), Look.bRain ? FLinearColor(.055f,.073f,.057f) : FLinearColor(.12f,.095f,.045f), 2, .96f);
    // S362: only new edge/joint geometry receives these sibling MIDs. The parent
    // atlas, original wall/slab bindings and all old package bytes stay intact.
    // Register the copies alongside the old focus MIDs so every added edge
    // opens on the same camera-to-Arrel ray as its underlying masonry.
    auto EdgeMaterial = [&](const TCHAR* Name, UMaterialInterface* Parent, const FLinearColor& Tint, float Roughness)
    {
        // UE only accepts a material or constant instance as a MID parent.
        // Preserve the source's atlas region/Mode/Metallic before the new finish.
        auto* Source = Cast<UMaterialInstanceDynamic>(Parent);
        auto* M = UMaterialInstanceDynamic::Create(Source ? Source->Parent.Get() : Parent,this,FName(Name));
        if (Source) M->CopyInterpParameters(Source);
        M->SetVectorParameterValue(TEXT("ArchitectureTint"),Tint);
        M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
        FocusMaterials.Add(M); return M;
    };
    auto* CutStone = EdgeMaterial(TEXT("S362CutStone"),RuinStone,Look.RuinTint * (Look.bRain ? 8.6f : 7.9f),.76f);
    auto* WetJoint = EdgeMaterial(TEXT("S362WetJoint"),Slab,Look.RuinTint * 3.25f,Look.bRain ? .27f : .94f);
    auto DressRuin = [&](const FVector& At,float Yaw,float Scale)
    {
        const FRotator Turn(0,Yaw,0);
        // Partial coping and offset end-grain stones sit on the existing
        // 192x96x150 ruin model. Broken lengths preserve its irregular outline.
        for (int32 I=0;I<3;++I)
        {
            const FVector Offset((-65.f+I*59.f)*Scale,-39.f*Scale,(133.f+(I%2)*9.f)*Scale);
            Solid(Block,CutStone,At+Turn.RotateVector(Offset),FVector(46,13,9)*Scale/100.f,Turn);
        }
        for (float Side : {-1.f,1.f})
            Solid(Block,CutStone,At+Turn.RotateVector(FVector(Side*76.f,-40.f,43.f)*Scale),FVector(15,9,29)*Scale/100.f,Turn);
    };
    const bool bRubbleModel = LoadObject<UStaticMesh>(nullptr, RubblePath, nullptr, LOAD_NoWarn | LOAD_Quiet) != nullptr;
    auto Border = [&](int32 X, int32 Y) { return X <= 0 || Y <= 0 || X >= Spec->Width - 1 || Y >= Spec->Height - 1; };
    auto IsWall = [&](int32 X, int32 Y) { return Spec->TileAt(X, Y) == WallType && !Border(X, Y); };
    auto Open = [&](int32 X, int32 Y) { const int32 Type = Spec->TileAt(X, Y); return Type >= 0 && !Spec->IsSolid(Type); };
    FIntPoint FloorMin(MAX_int32, MAX_int32), FloorMax(MIN_int32, MIN_int32);
    // The ruins' stone is cool; the Belt's dusk warms it.
    auto* RuinKit = Kit(TEXT("Drift"), Look.bRuinBorder ? FLinearColor::White : FLinearColor(1.12f, 1.f, .86f));
    TSet<FIntPoint> Taken;
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
        {
            const int32 Type = Spec->TileAt(X, Y);
            const FVector C = TileCentre(X, Y);
            const float N = Hash(X, Y, Spec->Chapter);
            if (IsWall(X, Y))
            {
                // The relay house's walls: masonry on a plinth, the corners and the door's jambs standing tall,
                // the runs between them worn down unevenly, with a capstone where the top survives.
                const bool bAcross = IsWall(X - 1, Y) || IsWall(X + 1, Y), bAlong = IsWall(X, Y - 1) || IsWall(X, Y + 1);
                const bool bJamb = (Open(X - 1, Y) && IsWall(X + 1, Y) && IsWall(X - 2, Y)) || (Open(X + 1, Y) && IsWall(X - 1, Y) && IsWall(X + 2, Y));
                const bool bDoorSide = (Open(X - 1, Y) && Open(X - 2, Y) && IsWall(X - 3, Y)) || (Open(X + 1, Y) && Open(X + 2, Y) && IsWall(X + 3, Y));
                const bool bTall = (bAcross && bAlong) || bJamb || bDoorSide;
                const float Height = bTall ? 170.f : N < .16f ? 62.f + 120.f * N : 116.f + 44.f * N;
                // Authored staggered courses replace the uninterrupted block. Physical
                // blockers remain the untouched, source-derived layer in BuildTerrain.
                Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Masonry, C + FVector(0,0,Feet), FVector(T/100.f,T/100.f,Height/100.f));
                Stand(Masonry, C + FVector(0, 0, 7.f + Feet), FVector(T + 9, T + 9, 14));
                if (bTall || N > .4f) Solid(*ArchitectureMesh(TEXT("SM_ChippedCap")), Cap, C + FVector(0,0,Height+Feet), FVector((T+4)/100.f,(T+4)/100.f,.7f));
                // Small broken stones and tufts crown existing solid cells only.
                if (N < .36f) Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), RuinStone, C + FVector(0,0,Height+Feet), FVector(.52f), FRotator(0,N*360,0));
                if (N > .55f) Solid(*ArchitectureMesh(TEXT("SM_RuinTuft")), Weeds, C + FVector((N-.7f)*T*.4f,0,Height+Feet), FVector(.7f), FRotator(0,N*360,0));
                if (bJamb || bDoorSide)
                {
                    // A lantern on an iron bracket at each side of the door, on the face the camera sees.
                    const float Side = Open(X - 1, Y) ? -1.f : 1.f;
                    const FVector Hook = C + FVector(Side * (T * .5f - 14.f), -T * .5f - 16.f, 132.f);
                    Beam(Iron, Hook + FVector(0, 16, 14), Hook + FVector(0, 0, 14), 3.f);
                    Beam(Iron, Hook + FVector(0, 0, 14), Hook + FVector(0, 0, 7), 2.f);
                    // S393: Arcane at noon (25화) lights no lamp by a door.
                    if (Map != TEXT("arcane_city") && Map != TEXT("arcane_leave")) Lamp(Hook, 420.f, Look.LampIntensity);
                }
                continue;
            }
            if (Type == Concrete && !Border(X, Y))
            {
                // The fallen overpass: low slabs lying a little out of true.
                const float Height = 34.f + 18.f * N;
                const FRotator Turn(0,(N-.5f)*7.f,(Hash(X,Y,3)-.5f)*5.f);
                const FVector Centre = C + FVector(0,0,Height*.5f+Feet);
                Stand(Slab,Centre,FVector(T*.97f,T*.97f,Height),Turn);
                // Wet arrises and recessed construction joints break the broad
                // concrete faces at gameplay distance. They stay inside their
                // original rotated slab and never bridge the path opening.
                for (float Side : {-1.f,1.f})
                {
                    const FVector Lip(Side*T*.235f,-T*.448f,Height*.5f-3.5f);
                    Solid(Block,CutStone,Centre+Turn.RotateVector(Lip),FVector(T*.43f,5.5f,5.f)/100.f,Turn);
                    const FVector Joint(Side*T*.22f,-T*.470f,-1.5f);
                    Solid(Block,WetJoint,Centre+Turn.RotateVector(Joint),FVector(2.8f,1.2f,Height*.66f)/100.f,Turn,false);
                }
                if (Look.bRain)
                    Solid(Block,WetJoint,Centre+Turn.RotateVector(FVector(0,-T*.465f,-Height*.5f+4.f)),FVector(T*.88f,3.f,5.f)/100.f,Turn,false);
                continue;
            }
            if (Type == Ruin && !Border(X, Y))
            {
                // A ruin: what is left of a wall (two tiles long where two ruin tiles lie together, a stub of it on
                // a lone tile), and the stones that fell from it. A tile a set piece stands on is left to it.
                const FIntPoint Here(X, Y);
                if (Claimed(X, Y) || Taken.Contains(Here)) continue;
                const auto Free = [&](int32 AX, int32 AY) { return Spec->TileAt(AX, AY) == Ruin && !Border(AX, AY) && !Claimed(AX, AY) && !Taken.Contains(FIntPoint(AX, AY)); };
                const FVector Ground(0, 0, Feet);
                const float Turn = (N - .5f) * 8.f + (Hash(X, Y, 5) > .5f ? 180.f : 0.f);
                if (Free(X + 1, Y))
                {
                    Taken.Add(FIntPoint(X+1,Y)); const FVector At=(C+TileCentre(X+1,Y))*.5+Ground;
                    Prop(TEXT("RuinWall"),RuinKit,At,Turn); DressRuin(At,Turn,1.f);
                }
                else if (Free(X, Y + 1))
                {
                    Taken.Add(FIntPoint(X,Y+1)); const FVector At=(C+TileCentre(X,Y+1))*.5+Ground;
                    Prop(TEXT("RuinWall"),RuinKit,At,90.f+Turn); DressRuin(At,90.f+Turn,1.f);
                }
                else
                {
                    const float Yaw=360.f*Hash(X,Y,7);
                    Prop(TEXT("RuinWall"),RuinKit,C+Ground,Yaw,FVector(.6f)); DressRuin(C+Ground,Yaw,.6f);
                }
                Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), RuinStone, C + Ground, FVector(T/120.f), FRotator(0,N*360,0));
                if (N > .35f) Solid(*ArchitectureMesh(TEXT("SM_RuinTuft")), Weeds, C + Ground + FVector(T*.18f,0,0), FVector(.85f), FRotator(0,N*360,0));
                for (int32 I = 0; I < 3; ++I)
                {
                    const FVector At = C + FVector((Hash(X * 7 + I, Y, 15) - .5f) * T * .8f, (Hash(X, Y * 7 + I, 17) - .5f) * T * .8f, Feet);
                    const float S = .9f + 1.1f * Hash(X + I, Y + I, 19);
                    if (bRubbleModel) Solid(RubblePath, RuinStone, At, FVector(S), FRotator(0, 360.f * Hash(X + I, Y, 21), 0));
                    else Box(RuinStone, At + FVector(0, 0, 9.f * S), FVector(26, 20, 18) * S, FRotator(8.f * S, 360.f * Hash(X + I, Y, 21), 0));
                }
            }
            if (Spec->TileDetails.IsValidIndex(Type) && Spec->TileDetails[Type] == TEXT("stone_floor") && !Spec->IsSolid(Type))
            { FloorMin = FloorMin.ComponentMin(FIntPoint(X, Y)); FloorMax = FloorMax.ComponentMax(FIntPoint(X, Y)); }
        }
    // The door's lintel: a timber over each gap in a wall run.
    for (int32 Y = 1; Y < Spec->Height - 1; ++Y)
        for (int32 X = 1; X < Spec->Width - 1; ++X)
            if (Open(X, Y) && IsWall(X - 1, Y))
            {
                int32 End = X; while (End < Spec->Width - 1 && Open(End, Y)) ++End;
                if (IsWall(End, Y) && End - X <= 3) Beam(Timber, TileCentre(X - 1, Y) + FVector(0, 0, 176.f + Feet), TileCentre(End, Y) + FVector(0, 0, 176.f + Feet), 15.f);
            }
    if (FloorMin.X > FloorMax.X) return;
    // S385: the farmhouse under the ridge has no roof left at all (22화: "남아 있는 벽 위로 지붕이 없었다"), no awning
    // and no lantern (23화: "불빛은 없었다").
    if (Map == TEXT("ridge_farmhouse")) return;
    if (WallType >= 0 && IsWall(FloorMin.X - 1, FloorMin.Y - 1))
    {
        // What is left of the relay house's roof: the wall plates on the long walls and a few rafters across,
        // two of them fallen in. They throw the low light into bars on the floor.
        const float North = FloorMin.Y - 1, South = FloorMax.Y + 1, Top = 178.f + Feet;
        for (float Row : {North, South})
            Beam(Timber, TileCentre(FloorMin.X - 1, Row) + FVector(0, 0, Top), TileCentre(FloorMax.X + 1, Row) + FVector(0, 0, Top), 13.f);
        int32 Index = 0;
        for (float X = FloorMin.X - .5f; X <= FloorMax.X + .6f; X += 1.5f, ++Index)
        {
            const FVector A = TileCentre(X, North) + FVector(0, 0, Top + 12.f), B = TileCentre(X, South) + FVector(0, 0, Top + 12.f);
            if (Index % 3 == 1) Beam(Timber, A, FMath::Lerp(A, B, .56f) + FVector(14, 0, -(Top + 6.f - Feet)), 11.f);   // fallen: one end on the floor
            else if (Index % 3 == 2) Beam(Timber, A, FMath::Lerp(A, B, .42f) + FVector(0, 0, -6), 11.f);                    // snapped short
            else Beam(Timber, A, B, 11.f);
        }
    }
    else
    {
        // Drift's shelter: a patched awning on poles over the back row of the slab ring, one each side of the
        // path, and a lantern on each front pole. The awnings stand over the ring, where nobody walks: a roof
        // over the floor would hide the heads of those under it from this camera.
        auto* Canvas = Kit(TEXT("Drift"));
        auto* Hem = UMaterialInstanceDynamic::Create(Canvas->Parent.Get(),this,TEXT("S362DriftCanvasHem"));
        Hem->CopyInterpParameters(Canvas); // Retain Drift Color/Glow and reveal, not the base Belt defaults.
        Hem->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.82f,.89f,1.04f));
        Hem->SetScalarParameterValue(TEXT("Roughness"),.67f); FocusMaterials.Add(Hem);
        const int32 Row = FloorMin.Y - 1;
        int32 From = FloorMin.X;
        for (int32 X = FloorMin.X; X <= FloorMax.X + 1; ++X)
        {
            if (X <= FloorMax.X && !Open(X, Row)) continue;
            if (X - From >= 2)
            {
                const FVector P = (TileCentre(From, Row) + TileCentre(X - 1, Row)) * .5 + FVector(0, 0, Feet);
                const float Wide = (X - From) * T / 384.f, Deep = .6f;
                const FRotator Turn(0, From == FloorMin.X ? 0.f : 180.f, 0);
                Prop(TEXT("TarpCanopy"), Canvas, P, Turn.Yaw, FVector(Wide, Deep, 1));
                Solid(*ArchitectureMesh(TEXT("SM_ShelterRigging")), Timber, P, FVector(Wide, Deep, 1), Turn);
                // A rolled, shallow-sagging front hem gives the existing cloth
                // an actual layered edge without putting a roof over the floor.
                // Only the occupied slab row carries this canopy construction.
                for (int32 I=0;I<12;++I)
                {
                    const float U0=-1.f+I/6.f,U1=-1.f+(I+1)/6.f;
                    const FVector A(U0*174.f*Wide,-136.f*Deep,189.f+14.f*U0*U0);
                    const FVector B(U1*174.f*Wide,-136.f*Deep,189.f+14.f*U1*U1);
                    Beam(Hem,P+Turn.RotateVector(A),P+Turn.RotateVector(B),4.2f);
                }
                for (float Side : {-180.f, 180.f})
                {
                    const FVector Pole = P + FVector(Side * Wide, -130.f * Deep, 0);
                    // Metal collars and a timber foot shoe articulate the old
                    // posts; neither changes the pole positions or their light.
                    for (float Z : {14.f,94.f,143.f}) Box(Iron,Pole+FVector(0,0,Z),FVector(12,12,7));
                    Box(Timber,Pole+FVector(0,0,7),FVector(19,18,14));
                    Beam(Iron, Pole + FVector(0, 0, 176.f), Pole + FVector(0, -13, 170.f), 2.f);
                    Beam(Iron, Pole + FVector(0, -13, 170.f), Pole + FVector(0, -13, 160.f), 1.5f);
                    Lamp(Pole + FVector(0, -13, 153.f), 430.f, Look.LampIntensity*1.16f);
                }
            }
            From = X + 1;
        }
    }
}
void AMemoriaChapterPresentation::BuildBorder(const FDressing& Look)
{
    // The map's rim. It was a row of boxes; it is now the edge of a place.
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    const int32 WallType = Spec->TileNames.IndexOfByKey(TEXT("WALL"));
    auto* Stone = Focus(TEXT("BorderStone"), Look.RuinTint * 1.1f, 0, .93f);
    auto* Cap = Focus(TEXT("BorderCap"), Look.RuinTint * 1.35f, 0, .88f);
    const TCHAR* Block = BlockMesh();
    auto Stand = [&](UMaterialInterface* Material, const FVector& Position, const FVector& Size) { Solid(Block, Material, Position, Size / 100.0); };
    auto Rim = [&](int32 X, int32 Y) { return (X == 0 || Y == 0 || X == Spec->Width - 1 || Y == Spec->Height - 1) && Spec->TileAt(X, Y) == WallType; };
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
        {
            if (!Rim(X, Y)) continue;
            const FVector C = TileCentre(X, Y);
            const float N = Hash(X, Y, 31);
            const bool bNorth = Y == 0, bSouth = Y == Spec->Height - 1;
            if (Look.bRuinBorder)
            {
                // Drift: broken walls all round, high at the back, low at the front where the camera looks over.
                const float Height = bNorth ? 118.f + 86.f * N : bSouth ? 26.f + 30.f * N : 64.f + 96.f * N;
                Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Stone, C + FVector(0,0,Feet), FVector(T/100.f,T/100.f,Height/100.f));
                if (N > .45f && !bSouth) Solid(*ArchitectureMesh(TEXT("SM_ChippedCap")), Stone, C + FVector((N-.7f)*30.f,0,Height+Feet), FVector(T*.55f/100.f,T*.8f/100.f,2.8f));
                if (N < .3f) Solid(*ArchitectureMesh(TEXT("SM_ChippedCap")), Cap, C + FVector(0,0,Height+Feet), FVector((T+3)/100.f,(T+3)/100.f,.6f));
                continue;
            }
            if (bNorth)
            {
                // The Belt: the rail line's embankment along the north.
                Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Stone, C + FVector(0,0,Feet), FVector(T/100.f,T/100.f,.46f));
                Solid(*ArchitectureMesh(TEXT("SM_ChippedCap")), Cap, C + FVector(0,-T*.5f+9.f,46.f+Feet), FVector(T/100.f,.2f,.7f));
                continue;
            }
            // The other three sides: a worn kerb, and stone posts joined by chains.
            const bool bVertical = X == 0 || X == Spec->Width - 1;
            const float Height = 14.f + 16.f * N;
            Box(Stone, C + FVector(0, 0, Height * .5f + Feet), bVertical ? FVector(T * .6f, T, Height) : FVector(T, T * .6f, Height));
        }
    if (Look.bRuinBorder) return;
    // The Belt's chain fence on the kerb, along the west, the south and the east, broken where the road leaves.
    auto* Chain = Kit(TEXT("Belt"));
    const auto Run = [&](FIntPoint At, const FIntPoint& Step, int32 Count)
    {
        FIntPoint First = At; int32 Length = 0;
        for (int32 I = 0; I <= Count; ++I, At += Step)
        {
            if (I < Count && Rim(At.X, At.Y)) { if (Length++ == 0) First = At; continue; }
            if (Length >= 2) { const FIntPoint Last = At - Step; Fence(Chain, TileCentre(First.X, First.Y) + FVector(0, 0, Feet + 10.f), TileCentre(Last.X, Last.Y) + FVector(0, 0, Feet + 10.f)); }
            Length = 0;
        }
    };
    Run(FIntPoint(0, 1), FIntPoint(0, 1), Spec->Height - 1);
    Run(FIntPoint(0, Spec->Height - 1), FIntPoint(1, 0), Spec->Width);
    Run(FIntPoint(Spec->Width - 1, 1), FIntPoint(0, 1), Spec->Height - 1);
}
void AMemoriaChapterPresentation::BuildSetPieces(const FDressing& Look)
{
    // What the canvas paints, as Codex's S343 models. Each stands on a solid tile or beyond the border, so none
    // of them is in Arrel's way, and none roofs a place where a figure can stand. The far things the kit has no
    // model for (the relay pylons, the slag ridges, the ruin masses) stay built from boxes.
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    auto* Stone = Focus(TEXT("SetStone"), Look.WallTint * .9f, 0, .93f);
    auto* Timber = Focus(TEXT("SetTimber"), Look.TimberTint * 1.25f, 1);
    auto* Iron = Focus(TEXT("SetIron"), Look.IronTint, 3, .45f, .7f);
    auto* Dark = Focus(TEXT("Far"), Look.RuinTint * .42f, 3, 1.f);
    auto* Small = Kit(TEXT("Small"), FLinearColor::White, TEXT("SmallGlow"));
    const FVector Ground(0, 0, Feet);
    auto LanternPost = [&](const FVector& P, float Yaw)
    {
        // The lantern hangs 17 cm to the post's right and 153 cm up.
        Prop(TEXT("LanternPost"), Small, P + Ground, Yaw);
        Lamp(P + Ground + FRotator(0, Yaw, 0).RotateVector(FVector(17, -6, 150)), 470.f, Look.LampIntensity * 1.15f, false);
    };
    if (Map == TEXT("belt_waystation"))
    {
        // The glow mask belongs to the two models with lanterns; the others share the atlas without it.
        auto* Belt = Kit(TEXT("Belt"));
        auto* Lit = Kit(TEXT("Belt"), FLinearColor::White, TEXT("BeltGlow"));
        const FVector Bank(0, 0, 46.f + Feet);   // the embankment's top
        // The rail line beyond the embankment, running off both ways.
        const float RailY = TileCentre(0, -3.f).Y, West = -14.f * T, East = (Spec->Width + 14.f) * T;
        for (float X = West; X < East; X += 192.f) Prop(TEXT("RailTrack"), Belt, FVector(X + 96.f, RailY, Feet));
        // The far side: relay pylons carrying a sagging line, and slag ridges fading into the dust.
        const float PylonY = TileCentre(0, -5.4f).Y;
        FVector Last = FVector::ZeroVector;
        for (int32 I = -2; I <= 6; ++I)
        {
            const FVector P(I * 5.5f * T + 60.f, PylonY, 0);
            Beam(Timber, P + FVector(0, 0, Feet), P + FVector((Scatter(I, 2) - .5) * 22., 0, 360.f), 14.f);
            Box(Timber, P + FVector(0, 0, 318.f), FVector(120, 9, 9), FRotator((Scatter(I, 3) - .5) * 8., 0, 0));
            for (float Side : {-52.f, 52.f}) Box(Iron, P + FVector(Side, 0, 328.f), FVector(6, 6, 13));
            const FVector Head = P + FVector(0, 0, 334.f);
            if (I > -2) { const FVector Sag = (Last + Head) * .5 - FVector(0, 0, 44); Beam(Iron, Last, Sag, 2.f); Beam(Iron, Sag, Head, 2.f); }
            Last = Head;
        }
        for (int32 I = 0; I < 7; ++I)
            Solid(TEXT("Roof"), Dark, FVector(-1800.f + I * 1150.f + 300.f * Scatter(I, 4), 1900.f + 900.f * Scatter(I, 5), -20.f),
                FVector(15.f + 12.f * Scatter(I, 6), 7.f + 4.f * Scatter(I, 7), 5.f + 6.f * Scatter(I, 8)), FRotator(0, (Scatter(I, 9) - .5) * 30., 0), false);
        // The signal post on the embankment: its wheel, three pennants and the lamp at its foot.
        {
            const FVector P = TileCentre(5.f, 0.f) + Bank;
            Prop(TEXT("SignalPost"), Lit, P);
            // S362: the retained relay signal reads as assembled timber and
            // iron rather than one flat silhouette in the low evening key.
            for (float Z : {58.f,142.f,225.f}) Box(Iron,P+FVector(0,-2,Z),FVector(22,22,7));
            Box(Stone,P+FVector(0,0,7),FVector(40,34,14));
            Beam(Timber,P+FVector(-32,-9,52),P+FVector(-5,-9,126),5.f);
            Beam(Timber,P+FVector(32,-9,52),P+FVector(5,-9,126),5.f);
            Lamp(P + FVector(0, -30, 32), 430.f, Look.LampIntensity*1.18f, false);
        }
        // The platform shelter between the embankment and the rails, on a plinth where it overhangs the bank.
        {
            const FVector P((TileCentre(17.f, 0.f).X + TileCentre(20.f, 0.f).X) * .5, 30.f, Bank.Z);
            Box(Stone, FVector(P.X, 82.f, (Bank.Z + Feet) * .5f), FVector(380, 164, Bank.Z - Feet));
            Prop(TEXT("PlatformShelter"), Lit, P);
            Solid(*ArchitectureMesh(TEXT("SM_ShelterRigging")), Timber, P, FVector(.845f,.877f,1.08f));
            // A doubled fascia, short end blocks and pole collars make a deep
            // eave silhouette within the original 380x260 platform footprint.
            Box(Timber,P+FVector(0,-124,218),FVector(350,10,15));
            Box(Iron,P+FVector(0,-129,215),FVector(342,2.5,3));
            for (float Side : {-152.f,152.f})
            {
                Box(Stone,P+FVector(Side,-114,7),FVector(28,28,14));
                for (float Z : {72.f,154.f}) Box(Iron,P+FVector(Side,-114,Z),FVector(19,19,7));
                Beam(Timber,P+FVector(Side,-114,170),P+FVector(Side*.72f,-114,211),7.f);
                Lamp(P+FVector(Side,-114,182),460.f,Look.LampIntensity*1.28f,false);
            }
        }
        // The banner pole at the north-east corner, its long cloth torn.
        Prop(TEXT("BannerPole"), Belt, TileCentre(22.6f, 0.f) + Bank);
        // The freight left on the two southern ruins.
        Prop(TEXT("CrateStack"), Belt, TileCentre(3.5f, 14.f) + Ground);
        Prop(TEXT("CrateStack"), Belt, TileCentre(19.5f, 14.f) + Ground, 0.f, FVector(.94f));
        // Lantern posts where the road leaves the yard, and one on the west kerb.
        for (float Y : {7.f, 11.f}) LanternPost(TileCentre(Spec->Width - 1.f, Y) + FVector(-30, 0, 0), 180.f);
        LanternPost(TileCentre(0.f, 12.f) + FVector(30, 0, 0), 0.f);
    }
    else if (Map == TEXT("drift_shelter"))
    {
        auto* Camp = Kit(TEXT("Drift"));
        auto* Far = Kit(TEXT("Drift"), FLinearColor(.5f, .52f, .6f));
        auto* Freight = Kit(TEXT("Belt"), FLinearColor(1.1f, 1.16f, 1.34f));
        // The camp beyond the east wall, north of the road: a patched canopy on poles over its stores, and a lantern.
        const FVector Beyond(Spec->Width * T + 230.f, TileCentre(0, 4.6f).Y, Feet);
        // Drift: dead trees beyond the walls.
        auto Tree = [&](const FVector& P, int32 Seed)
        {
            if (FMath::Abs(P.X - Beyond.X) < 330.f && FMath::Abs(P.Y - Beyond.Y) < 300.f) return;
            const float S = .8f + .42f * float(Scatter(Seed, 11));
            Prop(TEXT("DeadTree"), Far, FVector(P.X, P.Y, Feet - 6.f), 360.f * float(Scatter(Seed, 12)), FVector(S, S, S * (.9f + .25f * float(Scatter(Seed, 13)))));
        };
        int32 Seed = 0;
        for (float X = -3.f; X <= Spec->Width + 3.f; X += 2.6f, ++Seed)
        {
            Tree(TileCentre(X + float(Scatter(Seed, 16)), -2.2f - 2.f * float(Scatter(Seed, 17))), Seed);
            if (Seed % 2 == 0) Tree(TileCentre(X + float(Scatter(Seed, 18)), -5.8f - 2.f * float(Scatter(Seed, 19))), Seed + 50);
        }
        for (float Y = 1.f; Y < Spec->Height; Y += 3.4f, ++Seed)
        {
            Tree(TileCentre(-2.6f - 1.5f * float(Scatter(Seed, 20)), Y), Seed + 100);
            Tree(TileCentre(Spec->Width + 1.6f + 1.5f * float(Scatter(Seed, 21)), Y + 1.2f), Seed + 150);
        }
        // Far ruin masses behind the trees.
        for (int32 I = 0; I < 6; ++I)
            Box(Dark, FVector(-900.f + I * 760.f, 1150.f + 500.f * Scatter(I, 22), 120.f), FVector(420.f + 300.f * Scatter(I, 23), 300, 300.f + 320.f * Scatter(I, 24)),
                FRotator(0, (Scatter(I, 25) - .5) * 24., (Scatter(I, 26) - .5) * 9.), false);
        Prop(TEXT("TarpCanopy"), Camp, Beyond);
        Solid(*ArchitectureMesh(TEXT("SM_ShelterRigging")), Timber, Beyond, FVector::OneVector);
        Prop(TEXT("CrateStack"), Freight, Beyond + FVector(24, 26, 0), 180.f, FVector(.9f));
        Beam(Iron, Beyond + FVector(-180, -130, 190), Beyond + FVector(-166, -140, 176), 2.f);
        Lamp(Beyond + FVector(-166, -140, 168), 520.f, Look.LampIntensity);
        // The camp's stores on the rubble in the north-east and by the west wall, and the gramophone on its ruin.
        Prop(TEXT("CrateStack"), Freight, TileCentre(17.5f, 2.f) + Ground, 0.f, FVector(.92f));
        Prop(TEXT("CrateStack"), Freight, TileCentre(2.5f, 13.f) + Ground, 180.f, FVector(.92f));
        Prop(TEXT("Gramophone"), Kit(TEXT("Small")), TileCentre(3.f, 4.f) + Ground, 24.f);
        // A lantern post at the road's two ends.
        LanternPost(TileCentre(8.f, 0.f) + FVector(-20, -T * .5f - 14.f, 0), 0.f);
        LanternPost(TileCentre(Spec->Width - 1.f, 7.f) + FVector(-T * .5f - 14.f, 0, 0), 180.f);
    }
    else if (Map == TEXT("crumbling_coast"))
    {
        // S373: the hole on the pool's tiles: a flat pale disc of light, its edge not holding still (11화);
        // the young tree on the north path with its mark (11화); the empty chair and the flat stone where the road
        // forks, just past the north exit (12화).
        const int32 Water = Spec->TileNames.IndexOfByKey(TEXT("WATER"));
        auto* Light = Focus(TEXT("HoleLight"), FLinearColor(.16f, .24f, .4f), 2, .2f);
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
                if (Spec->TileAt(X, Y) == Water)
                    Box(Light, TileCentre(X, Y) + FVector(0, 0, Feet + 1.f), FVector(T * .98f, T * .98f, 2.f), FRotator::ZeroRotator, false);
        Prop(TEXT("DeadTree"), Kit(TEXT("Drift"), FLinearColor(.95f, .95f, .9f)), TileCentre(3.6f, 3.f) + Ground, 20.f, FVector(.62f, .62f, .7f));
        const FVector Chair = TileCentre(7.2f, .8f) + Ground;
        Box(Stone, Chair + FVector(-70, 0, 14), FVector(80, 54, 28), FRotator(0, 8, 0));
        for (float Side : {-1.f, 1.f}) { Beam(Timber, Chair + FVector(Side * 16, -14, 0), Chair + FVector(-Side * 16, 14, 44), 3.f); }
        Box(Timber, Chair + FVector(0, 0, 44), FVector(36, 30, 3), FRotator::ZeroRotator);
    }
    else if (Map == TEXT("verdan_return"))
    {
        // S382: the stalls on the STALL tiles, half of them packed bare; the oil stall on the east corner still frying;
        // the Sump's door on the DOOR tiles south of the alley ring, sealed with cord and red wax; the public reservoir's
        // stone basin in the square; a cart going out with chair legs under its cover (21화).
        const int32 Stall = Spec->TileNames.IndexOfByKey(TEXT("STALL")), DoorTile = Spec->TileNames.IndexOfByKey(TEXT("DOOR"));
        auto* Awning = Focus(TEXT("VerdanAwning"), FLinearColor(.2f, .1f, .06f), 1, .95f);
        auto* Reservoir = Focus(TEXT("VerdanWater"), FLinearColor(.06f, .08f, .09f), 2, .15f);
        auto* Wax = Focus(TEXT("VerdanWax"), FLinearColor(.4f, .03f, .02f), 2, .5f);
        auto* Recess = Focus(TEXT("VerdanRecess"), FLinearColor(.012f, .011f, .01f), 2, .95f);
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const FVector C = TileCentre(X, Y) + Ground;
                const float N = Hash(X, Y, 91);
                if (Spec->TileAt(X, Y) == Stall)
                {
                    Box(Timber, C + FVector(0, 0, 45.f), FVector(T * .9f, T * .5f, 90.f));
                    for (const float Side : {-1.f, 1.f}) Beam(Timber, C + FVector(Side * T * .42f, T * .3f, 0), C + FVector(Side * T * .42f, T * .3f, 210.f), 4.f);
                    // Half of them packed: no awning, the counter bare.
                    if (N < .5f || (X == 24 && Y == 10)) Box(Awning, C + FVector(0, T * .1f, 205.f), FVector(T * 1.f, T * .8f, 4.f), FRotator(0, 0, -12.f));
                    if (X == 24 && Y == 10) Lamp(C + FVector(0, -T * .2f, 105.f), 300.f, Look.LampIntensity * 1.4f, false);
                }
                else if (Spec->TileAt(X, Y) == DoorTile)
                {
                    Box(Recess, C + FVector(0, 0, 2.f), FVector(T * .9f, T * .9f, 4.f), FRotator::ZeroRotator, false);
                    Box(Wax, C + FVector(0, -T * .35f, 60.f), FVector(10, 4, 10), FRotator::ZeroRotator, false);
                    Beam(Timber, C + FVector(-T * .45f, -T * .35f, 60.f), C + FVector(T * .45f, -T * .35f, 60.f), 1.5f);
                }
            }
        const FVector Square = TileCentre(14.f, 11.5f) + Ground;
        Box(Stone, Square + FVector(0, 0, 22.f), FVector(150, 100, 44));
        Box(Reservoir, Square + FVector(0, 0, 45.f), FVector(130, 80, 2), FRotator::ZeroRotator, false);
        const FVector Cart = TileCentre(17.2f, 2.6f) + Ground;
        Prop(TEXT("CrateStack"), Kit(TEXT("Belt"), FLinearColor(1.f, .9f, .8f)), Cart, 15.f, FVector(.8f));
        for (int32 I = 0; I < 3; ++I) Beam(Timber, Cart + FVector(-20.f + 20.f * I, 10.f, 110.f), Cart + FVector(-26.f + 20.f * I, 18.f, 160.f), 2.f);
    }
    else if (Map == TEXT("basin_station"))
    {
        // S379: the signal tower on the east ridge with its flag at half-mast (20화); the empty trough sunk on one leg
        // and the fallen mile stone by the north road; the ration window cut in the stone house's south wall, a flat
        // stone sill under it.
        auto* Pole = Timber;   // the set pieces' timber; another SetTimber MID would collide by name
        auto* Cloth = Focus(TEXT("BasinFlag"), FLinearColor(.32f, .3f, .27f), 2, .9f);
        auto* Shutter = Focus(TEXT("BasinWindow"), FLinearColor(.015f, .013f, .012f), 2, .95f);
        const FVector Tower = TileCentre(Spec->Width + 2.2f, 3.5f) + Ground;
        for (const float Side : {-1.f, 1.f})
            for (const float Depth : {-1.f, 1.f})
                Beam(Pole, Tower + FVector(Side * 70.f, Depth * 70.f, 0), Tower + FVector(Side * 22.f, Depth * 22.f, 620.f), 7.f);
        Box(Pole, Tower + FVector(0, 0, 610.f), FVector(70, 70, 10));
        Beam(Pole, Tower + FVector(0, 0, 610.f), Tower + FVector(0, 0, 900.f), 4.f);
        Box(Cloth, Tower + FVector(-46.f, 0, 760.f), FVector(88, 3, 52), FRotator(0, 0, 4));
        const FVector Trough = TileCentre(10.6f, 2.4f) + Ground;
        Box(Timber, Trough + FVector(0, 0, 20.f), FVector(110, 46, 40), FRotator(0, 8, -9));
        const FVector Mile = TileCentre(14.2f, 3.2f) + Ground;
        Box(Stone, Mile + FVector(0, 0, 12.f), FVector(32, 76, 24), FRotator(0, 18, 0));
        const FVector Window = TileCentre(11.2f, 11.f) + Ground;
        Box(Shutter, Window + FVector(0, -T * .52f, 95.f), FVector(54, 4, 38), FRotator::ZeroRotator, false);
        Box(Stone, Window + FVector(0, -T * .62f, 58.f), FVector(70, 18, 8));
    }
    // S396: Chapter 17 walks the same streets (27-30화), so the same set pieces stand.
    else if (Map == TEXT("arcane_city") || Map == TEXT("arcane_leave"))
    {
        // S393 (25화): the five one-person wooden booths on the second square's south side, a small bell on each door
        // frame, the third standing open on its metal arm; the low bench beside them; the bread window in the first
        // square's east wall; the cistern's stone wall with steps down to the water; the door with the brass ring;
        // the city gate with the clerk's table and register board; the small tower inside the city, over the roofs.
        const int32 BoothType = Spec->TileNames.IndexOfByKey(TEXT("BOOTH")), BenchType = Spec->TileNames.IndexOfByKey(TEXT("BENCH"));
        const int32 WindowType = Spec->TileNames.IndexOfByKey(TEXT("WINDOW")), CisternType = Spec->TileNames.IndexOfByKey(TEXT("CISTERN"));
        const int32 DoorType = Spec->TileNames.IndexOfByKey(TEXT("DOOR"));
        auto* Plank = Focus(TEXT("BoothWood"), FLinearColor(.11f, .075f, .045f), 1, .9f);
        auto* Gap = Focus(TEXT("BoothDark"), FLinearColor(.01f, .009f, .008f), 2, .95f);
        auto* Brass = Focus(TEXT("Brass"), FLinearColor(.42f, .3f, .1f), 3, .35f, .9f);
        auto* Water = Focus(TEXT("CisternWater"), FLinearColor(.04f, .06f, .07f), 2, .12f);
        auto* Bread = Focus(TEXT("Loaf"), FLinearColor(.4f, .24f, .1f), 1, .8f);
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Type = Spec->TileAt(X, Y);
                const FVector C = TileCentre(X, Y) + Ground;
                const float N = Hash(X, Y, 97);
                if (Type == BoothType)
                {
                    // One person sits inside. The door faces the square (north, +Y); a small bell on its frame.
                    Box(Plank, C + FVector(0, 0, 115.f), FVector(T * .82f, T * .82f, 230.f));
                    Box(Plank, C + FVector(0, 0, 236.f), FVector(T * .9f, T * .9f, 12.f));
                    Box(Gap, C + FVector(0, T * .42f, 100.f), FVector(T * .5f, 4, 190.f), FRotator::ZeroRotator, false);
                    Box(Brass, C + FVector(T * .3f, T * .44f, 200.f), FVector(9, 9, 11));
                    // The third booth stands open: its metal arm at the height of a seated temple.
                    if (X == 6) Beam(Iron, C + FVector(0, -T * .2f, 190.f), C + FVector(0, T * .18f, 140.f), 3.f);
                }
                else if (Type == BenchType)
                {
                    Box(Plank, C + FVector(0, 0, 42.f), FVector(T, T * .42f, 8.f));
                    for (const float Side : {-1.f, 1.f}) Box(Plank, C + FVector(Side * T * .38f, 0, 20.f), FVector(8, T * .36f, 40.f));
                }
                else if (Type == WindowType)
                {
                    // The bread window in the wall: a dark opening, a counter, loaves on it.
                    Box(Gap, C + FVector(-T * .5f - 1.f, 0, 120.f), FVector(4, T * .7f, 80.f), FRotator::ZeroRotator, false);
                    Box(Plank, C + FVector(-T * .5f - 10.f, 0, 78.f), FVector(22, T * .8f, 6.f));
                    for (int32 I = 0; I < 3; ++I) Box(Bread, C + FVector(-T * .5f - 10.f, (I - 1) * 16.f, 88.f), FVector(12, 10, 8), FRotator(0, 30.f * I, 0));
                }
                else if (Type == CisternType)
                {
                    // A broad stone wall round the water, the water low inside.
                    Box(Stone, C + FVector(0, 0, 40.f), FVector(T, T, 80.f));
                    Box(Water, C + FVector(0, 0, 82.f), FVector(T * .96f, T * .96f, 2.f), FRotator::ZeroRotator, false);
                }
                else if (Type == DoorType)
                {
                    // The door with the brass ring, on the face toward the alley (south, -Y).
                    Box(Plank, C + FVector(0, -T * .5f - 3.f, 105.f), FVector(T * .62f, 6, 200.f));
                    Box(Brass, C + FVector(T * .14f, -T * .5f - 8.f, 100.f), FVector(10, 3, 10));
                }
            }
        // The steps down into the cistern, on its east side.
        const FVector Steps = TileCentre(5.5f, 3.f) + Ground;
        for (int32 I = 0; I < 3; ++I) Box(Stone, Steps + FVector(-I * 10.f, 0, 60.f - 18.f * I), FVector(20, T * .7f, 10));
        // The gate: two stone piers either side of the opening and a lintel above; the clerk's table and the register board.
        const FVector Gate = TileCentre(12.f, 17.f) + Ground;
        for (const float Side : {-1.f, 1.f}) Box(Stone, Gate + FVector(Side * T, 0, 210.f), FVector(T * .9f, T * .9f, 420.f));
        Box(Stone, Gate + FVector(0, 0, 440.f), FVector(T * 2.9f, T * .9f, 50.f));
        const FVector Table = TileCentre(13.4f, 15.2f) + Ground;
        Box(Plank, Table + FVector(0, 0, 70.f), FVector(50, 70, 6));
        for (const float Side : {-1.f, 1.f}) Beam(Plank, Table + FVector(Side * 18.f, -26.f, 0), Table + FVector(Side * 18.f, -26.f, 68.f), 4.f);
        Box(Plank, Table + FVector(20.f, 30.f, 60.f), FVector(4, 44, 120), FRotator(0, 0, 8));
        // The small tower inside the city, over the roofs to the north-east.
        const FVector Tower = TileCentre(18.5f, -2.5f) + Ground;
        Box(Stone, Tower + FVector(0, 0, 500.f), FVector(150, 150, 1000.f));
        Box(Stone, Tower + FVector(0, 0, 1060.f), FVector(180, 180, 40.f));
        Beam(Iron, Tower + FVector(0, 0, 1080.f), Tower + FVector(0, 0, 1320.f), 5.f);
    }
    else if (Map == TEXT("dry_wells"))
    {
        // S389: the two dry wells (24화): square rims of stacked stone, the corners worn, no bucket frame, dark inside;
        // the topped poplars on the TREE tiles, thin branches grown again above the cut; field stone on the RIDGE
        // ledges; the broad flat stone where Tobias set his book, between the wells.
        const int32 TreeType = Spec->TileNames.IndexOfByKey(TEXT("TREE")), RidgeType = Spec->TileNames.IndexOfByKey(TEXT("RIDGE"));
        auto* Rim = Focus(TEXT("WellStone"), FLinearColor(.14f, .13f, .115f), 0, .93f);
        auto* Shaft = Focus(TEXT("WellShaft"), FLinearColor(.008f, .007f, .006f), 2, .95f);
        auto* Bark = Focus(TEXT("PoplarBark"), FLinearColor(.1f, .09f, .075f), 1, .9f);
        auto* Twig = Focus(TEXT("PoplarTwig"), FLinearColor(.07f, .075f, .05f), 1, .9f);
        auto* Field = Focus(TEXT("FieldStone"), FLinearColor(.15f, .14f, .12f), 0, .93f);
        auto* Far = Kit(TEXT("Drift"), FLinearColor(.86f, .84f, .72f));
        for (const FVector2D& W : {FVector2D(6.5f, 6.5f), FVector2D(11.5f, 6.5f)})
        {
            const FVector C = TileCentre(W.X, W.Y) + Ground;
            const float Half = T * .62f, Side = T * 1.38f;
            Box(Rim, C + FVector(0, Half, 26.f), FVector(Side, 26, 52));
            Box(Rim, C + FVector(0, -Half, 22.f), FVector(Side, 26, 44));
            Box(Rim, C + FVector(Half, 0, 24.f), FVector(26, Side - 52, 48));
            Box(Rim, C + FVector(-Half, 0, 25.f), FVector(26, Side - 52, 50));
            Box(Shaft, C + FVector(0, 0, 3.f), FVector(Side - 52, Side - 52, 4), FRotator::ZeroRotator, false);
        }
        // The broad stone between the wells, swept of earth.
        Box(Field, TileCentre(9.8f, 6.55f) + Ground + FVector(0, 0, 4.f), FVector(70, 46, 8), FRotator(0, 14.f, 0), false);
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Type = Spec->TileAt(X, Y);
                const FVector C = TileCentre(X, Y) + Ground;
                const float N = Hash(X, Y, 93);
                if (Type == TreeType)
                {
                    // A thick trunk cut off at a height, thin branches grown back above the cut.
                    const float Cut = 230.f + 70.f * N;
                    Beam(Bark, C, C + FVector(0, 0, Cut), 17.f + 5.f * N);
                    for (int32 I = 0; I < 6; ++I)
                    {
                        const float A = 60.f * I + 360.f * N, L = 110.f + 60.f * float(Scatter(I + X * 7, Y));
                        const FVector From = C + FVector(0, 0, Cut - 6.f);
                        Beam(Twig, From, From + FRotator(0, A, 0).RotateVector(FVector(26.f, 0, L)), 2.5f);
                    }
                }
                else if (Type == RidgeType)
                    Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), Field, C, FVector(.75f + .3f * N), FRotator(0, 360.f * N, 0));
            }
        // Beyond the rim: dead trees along the north and east, none along the south where the camera looks north.
        int32 Seed = 0;
        for (float X = -3.f; X <= Spec->Width + 3.f; X += 2.7f, ++Seed)
        {
            const float S = .85f + .4f * float(Scatter(Seed, 94));
            Prop(TEXT("DeadTree"), Far, TileCentre(X + float(Scatter(Seed, 95)), -2.2f - 2.f * float(Scatter(Seed, 96))) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 97)), FVector(S, S, S * 1.1f));
        }
        for (float Y = 1.f; Y < Spec->Height - 4; Y += 3.4f, ++Seed)
            for (const float X : {-2.6f, Spec->Width + 1.6f})
            {
                const float S = .8f + .4f * float(Scatter(Seed, 98));
                Prop(TEXT("DeadTree"), Far, TileCentre(X + float(Scatter(Seed, 99)), Y) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 100)), FVector(S, S, S * 1.1f));
            }
    }
    else if (Map == TEXT("east_road"))
    {
        // S398 (31-35화): mile stones by the road; low field walls; a few trees outside the fourth-mile bend; the
        // seventh-mile post with its lamp hanging unlit under the eaves; the eighth-mile yard: the doorless shed, the low
        // rail, the dry water trough, the bench, three other carts (one with its tarpaulin half drawn back).
        const auto Type = [&](const TCHAR* Name) { return Spec->TileNames.IndexOfByKey(Name); };
        const int32 MileType = Type(TEXT("MILESTONE")), WallType = Type(TEXT("LOWWALL")), TreeType = Type(TEXT("TREE")), RailType = Type(TEXT("RAIL"));
        const int32 TroughType = Type(TEXT("TROUGH")), BenchType = Type(TEXT("BENCH")), CartType = Type(TEXT("CART"));
        auto* Mile = Focus(TEXT("MileStone"), FLinearColor(.2f, .19f, .17f), 0, .93f);
        auto* Cut = Focus(TEXT("MileCut"), FLinearColor(.02f, .018f, .016f), 2, .95f);
        auto* Field = Focus(TEXT("FieldWall"), FLinearColor(.15f, .14f, .12f), 0, .93f);
        auto* Bark = Focus(TEXT("RoadBark"), FLinearColor(.09f, .075f, .055f), 1, .9f);
        auto* Leaf = Focus(TEXT("RoadLeaf"), FLinearColor(.06f, .085f, .035f), 1, .95f);
        auto* Board = Focus(TEXT("YardBoard"), FLinearColor(.12f, .085f, .05f), 1, .9f);
        auto* Shade = Focus(TEXT("YardShade"), FLinearColor(.01f, .009f, .008f), 2, .95f);
        auto* Silt = Focus(TEXT("TroughSilt"), FLinearColor(.1f, .08f, .05f), 2, .97f);
        auto* Canvas = Focus(TEXT("CartCanvas"), FLinearColor(.2f, .19f, .15f), 1, .95f);
        auto* Glass = Focus(TEXT("UnlitLamp"), FLinearColor(.05f, .05f, .045f), 3, .3f, .4f);
        auto Cart = [&](const FVector& C, float Yaw, bool bCovered)
        {
            const FRotator R(0, Yaw, 0);
            auto At = [&](float X, float Y, float Z) { return C + R.RotateVector(FVector(X, Y, Z)); };
            Box(Board, At(0, 0, 72.f), FVector(T * 1.05f, T * .62f, 8.f), R);
            for (const float Side : {-1.f, 1.f})
            {
                Box(Board, At(0, Side * T * .3f, 86.f), FVector(T * 1.05f, 5.f, 22.f), R);
                // A wheel each side: a disc standing on edge.
                Solid(TEXT("Cylinder"), Board, At(-T * .1f, Side * T * .35f, 44.f), FVector(.86f, .86f, .06f), FRotator(0, Yaw, 90.f));
                // The shafts forward, resting on the ground.
                Beam(Board, At(T * .5f, Side * T * .2f, 66.f), At(T * 1.25f, Side * T * .22f, 8.f), 3.f);
            }
            if (bCovered) Box(Canvas, At(-T * .12f, 0, 100.f), FVector(T * .72f, T * .58f, 34.f), R);
        };
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Here = Spec->TileAt(X, Y);
                const FVector C = TileCentre(X, Y) + Ground;
                const float N = Hash(X, Y, 101);
                if (Here == MileType)
                {
                    // An upright stone, the mile cut on the face toward the road.
                    Box(Mile, C + FVector(0, 0, 46.f), FVector(30, 20, 92));
                    Box(Mile, C + FVector(0, 0, 95.f), FVector(34, 24, 8));
                    Box(Cut, C + FVector(0, 11.f, 62.f), FVector(14, 2, 22), FRotator::ZeroRotator, false);
                }
                else if (Here == WallType)
                {
                    // A low field wall of stacked stone, knee high.
                    Box(Field, C + FVector(0, 0, 26.f + 6.f * N), FVector(T, T * .46f, 52.f + 12.f * N), FRotator(0, (N - .5f) * 6.f, 0));
                    if (N > .5f) Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), Field, C + FVector(0, 0, 58.f), FVector(.45f), FRotator(0, 360.f * N, 0));
                }
                else if (Here == TreeType)
                {
                    // A tree in leaf: a trunk and a rounded crown of a few overlapping masses.
                    const float Top = 200.f + 50.f * N;
                    Beam(Bark, C, C + FVector(0, 0, Top + 20.f), 15.f + 5.f * N);
                    Solid(TEXT("Sphere"), Leaf, C + FVector(0, 0, Top + 50.f), FVector(1.35f, 1.25f, 1.05f), FRotator(0, 360.f * N, 0));
                    for (int32 I = 0; I < 4; ++I)
                    {
                        const float A = 90.f * I + 360.f * N, S = .75f + .2f * float(Scatter(I + X * 5, Y + 104));
                        const FVector Off = FRotator(0, A, 0).RotateVector(FVector(52.f, 0, 0));
                        Solid(TEXT("Sphere"), Leaf, C + Off + FVector(0, 0, Top + 20.f + 18.f * float(I % 2)), FVector(S, S, S * .85f), FRotator(0, A, 0));
                    }
                }
                else if (Here == RailType)
                {
                    // The yard's low rail: a post on each tile, the rails between neighbours.
                    Beam(Board, C, C + FVector(0, 0, 82.f), 5.f);
                    if (Spec->TileAt(X + 1, Y) == RailType)
                        for (const float H : {44.f, 76.f}) Beam(Board, C + FVector(0, 0, H), TileCentre(X + 1, Y) + Ground + FVector(0, 0, H), 3.f);
                    if (Spec->TileAt(X, Y + 1) == RailType)
                        for (const float H : {44.f, 76.f}) Beam(Board, C + FVector(0, 0, H), TileCentre(X, Y + 1) + Ground + FVector(0, 0, H), 3.f);
                }
                else if (Here == TroughType)
                {
                    // The water trough by the shed, dry to the bottom (35화).
                    Box(Board, C + FVector(0, 0, 30.f), FVector(T * .9f, T * .5f, 60.f));
                    Box(Silt, C + FVector(0, 0, 61.f), FVector(T * .8f, T * .4f, 2.f), FRotator::ZeroRotator, false);
                }
                else if (Here == BenchType)
                {
                    Box(Board, C + FVector(0, 0, 42.f), FVector(T, T * .42f, 8.f));
                    for (const float Side : {-1.f, 1.f}) Box(Board, C + FVector(Side * T * .38f, 0, 20.f), FVector(8, T * .36f, 40.f));
                }
                else if (Here == CartType)
                    // The goods cart by the shed's mouth has its tarpaulin half drawn back; the small one by the rail has no horse.
                    Cart(C, X == 22 && Y == 4 ? 180.f : 90.f, X == 22 && Y == 4);
            }
        // The seventh-mile post, set back a tile south of the road (the cart's right, going east): a low house, its eaves
        // toward the road, the lamp under them unlit, a dry wick inside (34화). No light is made. Kept low so it does not
        // hide the road behind it from a camera looking north.
        const FVector Post = TileCentre(17.5f, 10.5f) + Ground;
        Box(Stone, Post + FVector(0, 0, 80.f), FVector(T * 1.84f, T * .9f, 160.f));
        Box(Timber, Post + FVector(0, 0, 168.f), FVector(T * 2.05f, T * 1.1f, 16.f));
        Box(Timber, Post + FVector(0, T * .6f, 161.f), FVector(T * 2.05f, 34.f, 8.f));
        Box(Shade, Post + FVector(-T * .35f, T * .46f, 72.f), FVector(52, 4, 140), FRotator::ZeroRotator, false);
        const FVector Hook = Post + FVector(T * .45f, T * .46f + 22.f, 142.f);
        Beam(Iron, Hook, Hook + FVector(0, 0, 22.f), 2.f);
        Box(Glass, Hook + FVector(0, 0, -12.f), FVector(14, 14, 22));
        // The doorless shed across the back of the yard: back and side walls, a roof over its own tiles, dark inside.
        const FVector Shed = TileCentre(21.f, 1.f) + Ground;
        Box(Board, Shed + FVector(0, T * .45f, 140.f), FVector(T * 5.f, 10, 280.f));
        for (const float Side : {-1.f, 1.f}) Box(Board, Shed + FVector(Side * T * 2.5f, 0, 140.f), FVector(10, T, 280.f));
        Box(Timber, Shed + FVector(0, 0, 290.f), FVector(T * 5.2f, T * 1.05f, 16.f));
        Box(Shade, Shed + FVector(0, T * .38f, 130.f), FVector(T * 4.8f, 4, 250.f), FRotator::ZeroRotator, false);
        // Far trees beyond the north rim, none along the south where the camera looks north.
        for (float X = -2.f; X <= Spec->Width + 2.f; X += 3.1f)
        {
            const FVector C = TileCentre(X + float(Scatter(int32(X * 7), 102)), -2.4f - 1.6f * float(Scatter(int32(X * 5), 103))) + Ground;
            Beam(Bark, C, C + FVector(0, 0, 260.f), 18.f);
            Box(Leaf, C + FVector(0, 0, 330.f), FVector(190, 170, 140), FRotator(0, 40.f * X, 0), false);
        }
    }
    else if (Map == TEXT("storm_road"))
    {
        // S401 (37-38화): low mounds in the grassland; the stone the wheel caught on; boulders throwing the shade where
        // the shell went still; thin trees by the road with a few leaves on their upper branches.
        const auto Type = [&](const TCHAR* Name) { return Spec->TileNames.IndexOfByKey(Name); };
        const int32 MoundType = Type(TEXT("MOUND")), StoneType = Type(TEXT("STONE")), BoulderType = Type(TEXT("BOULDER")), ThinType = Type(TEXT("THINTREE"));
        auto* Turf = Focus(TEXT("Mound"), FLinearColor(.1f, .09f, .06f), 2, .97f);
        auto* Rock = Focus(TEXT("StormRock"), FLinearColor(.14f, .13f, .12f), 0, .93f);
        auto* Bark = Focus(TEXT("ThinBark"), FLinearColor(.08f, .07f, .055f), 1, .9f);
        auto* Leaf = Focus(TEXT("ThinLeaf"), FLinearColor(.07f, .085f, .04f), 1, .95f);
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Here = Spec->TileAt(X, Y);
                const FVector C = TileCentre(X, Y) + Ground;
                const float N = Hash(X, Y, 111);
                if (Here == MoundType)
                    Solid(TEXT("Sphere"), Turf, C + FVector(0, 0, -8.f), FVector(1.15f + .2f * N, 1.f + .2f * N, .42f + .1f * N), FRotator(0, 360.f * N, 0));
                else if (Here == StoneType)
                    Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), Rock, C, FVector(.7f), FRotator(0, 360.f * N, 0));
                else if (Here == BoulderType)
                    Solid(TEXT("Sphere"), Rock, C + FVector(0, 0, 40.f), FVector(1.05f + .25f * N, .9f + .2f * N, .85f + .2f * N), FRotator(0, 360.f * N, 10.f * N));
                else if (Here == ThinType)
                {
                    // A slender trunk, a few thin branches, a few leaves only on the upper ones (38화).
                    const float Top = 250.f + 60.f * N;
                    Beam(Bark, C, C + FVector(0, 0, Top), 7.f + 3.f * N);
                    for (int32 I = 0; I < 3; ++I)
                    {
                        const float A = 120.f * I + 360.f * N;
                        const FVector From = C + FVector(0, 0, Top - 50.f - 30.f * I);
                        const FVector To = From + FRotator(0, A, 0).RotateVector(FVector(46.f, 0, 60.f));
                        Beam(Bark, From, To, 2.5f);
                        Solid(TEXT("Sphere"), Leaf, To, FVector(.32f, .28f, .24f), FRotator(0, A, 0));
                    }
                }
            }
    }
    else if (Map == TEXT("ridge_farmhouse"))
    {
        // S385: inside the roofless farmhouse (22화): the shelf warped by rain on the west wall with two round marks,
        // a broken bowl, leaves under the high wall, dry wood nobody burns. West beyond the rim the signal tower, its
        // flag halfway up, a low building at its foot and a cart; north past the rim the rows of stumps below the
        // ridge, and the ridge's trees. No lamp anywhere.
        auto* Leaf = Focus(TEXT("RidgeLeaves"), FLinearColor(.08f, .055f, .03f), 1, .95f);
        auto* Clay = Focus(TEXT("RidgeBowl"), FLinearColor(.16f, .12f, .09f), 1, .8f);
        auto* Cloth = Focus(TEXT("RidgeFlag"), FLinearColor(.3f, .28f, .25f), 2, .9f);
        auto* Far = Kit(TEXT("Drift"), FLinearColor(.8f, .78f, .66f));
        // The shelf against the west wall's inner face, warped down at one end.
        const FVector Shelf = TileCentre(5.f, 7.4f) + Ground + FVector(T * .5f + 7.f, 0, 0);
        Box(Timber, Shelf + FVector(0, 0, 118.f), FVector(12, T * 1.5f, 4), FRotator(0, 0, 5.f));
        for (const float Side : {-1.f, 1.f}) Beam(Timber, Shelf + FVector(0, Side * T * .62f, 0), Shelf + FVector(0, Side * T * .62f, 120.f), 3.f);
        for (const float Side : {-1.f, 1.f}) Box(Clay, Shelf + FVector(0, Side * 18.f, 121.f), FVector(9, 9, .6f), FRotator::ZeroRotator, false);
        // The broken bowl Elia pushed aside, and leaves heaped under the high north wall.
        for (int32 I = 0; I < 4; ++I)
        {
            const FVector At = TileCentre(7.6f + .3f * float(Scatter(I, 61)), 8.3f + .3f * float(Scatter(I, 62))) + Ground;
            Box(Clay, At + FVector(0, 0, 2), FVector(11, 8, 3), FRotator(0, 360.f * float(Scatter(I, 63)), 10.f), false);
        }
        for (int32 I = 0; I < 14; ++I)
        {
            const FVector At = TileCentre(6.6f + 7.6f * float(Scatter(I, 64)), 5.85f + .25f * float(Scatter(I, 65))) + Ground;
            Box(Leaf, At + FVector(0, 0, 1.5f), FVector(24, 18, 3), FRotator(0, 360.f * float(Scatter(I, 66)), 0), false);
        }
        // Dry wood under the east wall: there, and not burned.
        const FVector Wood = TileCentre(14.f, 6.4f) + Ground + FVector(T * .28f, 0, 0);
        for (int32 I = 0; I < 3; ++I) Beam(Timber, Wood + FVector(-4.f + 4.f * I, -30.f, 4.f + 6.f * I), Wood + FVector(4.f - 4.f * I, 30.f, 4.f + 6.f * I), 3.f);
        // The signal tower to the west, the flag halfway up the pole; the low building at its foot, a cart before it.
        const FVector Tower = TileCentre(-3.6f, 8.5f) + Ground;
        for (const float Side : {-1.f, 1.f})
            for (const float Depth : {-1.f, 1.f})
                Beam(Timber, Tower + FVector(Side * 70.f, Depth * 70.f, 0), Tower + FVector(Side * 22.f, Depth * 22.f, 620.f), 7.f);
        Box(Timber, Tower + FVector(0, 0, 610.f), FVector(70, 70, 10));
        Beam(Timber, Tower + FVector(0, 0, 610.f), Tower + FVector(0, 0, 900.f), 4.f);
        Box(Cloth, Tower + FVector(46.f, 0, 770.f), FVector(88, 3, 52), FRotator(0, 0, 4));
        Box(Stone, Tower + FVector(150.f, -40.f, 70.f), FVector(220, 160, 140));
        Prop(TEXT("CrateStack"), Kit(TEXT("Belt"), FLinearColor(1.f, .9f, .8f)), Tower + FVector(150.f, -190.f, 0), 80.f, FVector(.7f));
        // The rows of stumps past the broken north wall, and the ridge's trees beyond them.
        int32 Seed = 0;
        for (float Y = -.4f; Y >= -3.6f; Y -= 1.1f)
            for (float X = .4f; X <= 12.f; X += 1.6f, ++Seed)
            {
                const FVector At = TileCentre(X + .5f * float(Scatter(Seed, 67)), Y) + Ground;
                const float H = 22.f + 22.f * float(Scatter(Seed, 68));
                Beam(Timber, At, At + FVector(0, 0, H), 13.f + 6.f * float(Scatter(Seed, 69)));
            }
        for (float X = -3.f; X <= Spec->Width + 3.f; X += 2.6f, ++Seed)
        {
            const float S = .85f + .4f * float(Scatter(Seed, 70));
            Prop(TEXT("DeadTree"), Far, TileCentre(X + float(Scatter(Seed, 71)), -4.6f - 2.f * float(Scatter(Seed, 72))) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 73)), FVector(S, S, S * 1.1f));
        }
        // East: hedges of dead trees along the rim; not along the south rim, where the camera looks north.
        for (float Y = 1.f; Y < Spec->Height - 4; Y += 3.4f, ++Seed)
        {
            const float S = .8f + .4f * float(Scatter(Seed, 74));
            Prop(TEXT("DeadTree"), Far, TileCentre(Spec->Width + 1.6f + float(Scatter(Seed, 75)), Y) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 76)), FVector(S, S, S * 1.1f));
        }
    }
    else if (Map == TEXT("roofless_farms"))
    {
        // S377: the first farmhouse's standing walls on the PILLAR tiles, roofless, with roof-tile shards inside (17화);
        // field-boundary stones on the RIDGE tiles; standing stones on the MARKER tiles; trees and hedges beyond the
        // rim of the fields; past the north exit, the second farmhouse: two walls leaning on each other (18화).
        const int32 Pillar = Spec->TileNames.IndexOfByKey(TEXT("PILLAR")), Ridge = Spec->TileNames.IndexOfByKey(TEXT("RIDGE"));
        const int32 Marker = Spec->TileNames.IndexOfByKey(TEXT("MARKER"));
        // Pale field stone, lighter than the set pieces' dark masonry: the farmhouse walls read against dry earth.
        auto* Wall = Focus(TEXT("FieldStone"), FLinearColor(.15f, .14f, .12f), 0, .93f);
        auto* Shard = Focus(TEXT("RoofShard"), FLinearColor(.16f, .07f, .05f), 1, .9f);
        auto* Far = Kit(TEXT("Drift"), FLinearColor(.86f, .8f, .62f));
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Type = Spec->TileAt(X, Y);
                const FVector C = TileCentre(X, Y);
                const float N = Hash(X, Y, 77);
                if (Type == Pillar)
                {
                    // Waist-high where the wall broke, full height at the standing corner.
                    const float Height = N < .35f ? 96.f + 30.f * N : 190.f + 40.f * N;
                    Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Wall, C + FVector(0, 0, Feet), FVector(T / 100.f, T / 100.f, Height / 100.f));
                    if (N > .5f) Solid(*ArchitectureMesh(TEXT("SM_ChippedCap")), Wall, C + FVector(0, 0, Height + Feet), FVector((T + 4) / 100.f, (T + 4) / 100.f, .7f));
                }
                else if (Type == Ridge)
                    Solid(*ArchitectureMesh(TEXT("SM_RubbleCluster")), Wall, C + FVector(0, 0, Feet), FVector(.7f + .3f * N), FRotator(0, 360.f * N, 0));
                else if (Type == Marker)
                    Box(Wall, C + FVector(0, 0, Feet + 55.f), FVector(30, 22, 110), FRotator(0, 30.f * N, 4.f * (N - .5f)));
            }
        for (int32 I = 0; I < 7; ++I)
        {
            const FVector At = TileCentre(17.2f + .4f * float(Scatter(I, 81)), 11.3f + .5f * float(Scatter(I, 82))) + Ground;
            Box(Shard, At + FVector(0, 0, 2), FVector(16, 11, 3), FRotator(0, 360.f * float(Scatter(I, 83)), 0), false);
        }
        int32 Seed = 0;
        for (float X = -3.f; X <= Spec->Width + 3.f; X += 2.8f, ++Seed)
        {
            const float S = .85f + .4f * float(Scatter(Seed, 84));
            Prop(TEXT("DeadTree"), Far, TileCentre(X + float(Scatter(Seed, 85)), -2.4f - 2.f * float(Scatter(Seed, 86))) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 87)), FVector(S, S, S * 1.1f));
        }
        // Not along the south rim's last rows: the camera looks north over them.
        for (float Y = 1.f; Y < Spec->Height - 4; Y += 3.6f, ++Seed)
            for (const float X : {-2.6f, Spec->Width + 1.6f})
            {
                const float S = .8f + .4f * float(Scatter(Seed, 88));
                Prop(TEXT("DeadTree"), Far, TileCentre(X + float(Scatter(Seed, 89)), Y) + FVector(0, 0, Feet - 6.f), 360.f * float(Scatter(Seed, 90)), FVector(S, S, S * 1.1f));
            }
        const FVector Second = TileCentre(10.5f, -2.f) + Ground;
        Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Wall, Second + FVector(-34, 0, 0), FVector(T * 1.6f / 100.f, T * .4f / 100.f, 1.9f), FRotator(0, 0, 6));
        Solid(*ArchitectureMesh(TEXT("SM_WallCourse")), Wall, Second + FVector(34, 10, 0), FVector(T * .4f / 100.f, T * 1.4f / 100.f, 1.7f), FRotator(-5, 0, 0));
    }
    else if (Map == TEXT("haren_clearing"))
    {
        // S374: trees on the TREE tiles, every one inside and every other one on the rim; low shrubs on BUSH.
        // In the hollow, the tree Haren planted lies felled beside its stump, the cut face pale (13화); across the north
        // path the low rock with clear water in its top; past the north exit the limestone slab over the dry channel,
        // a small fire beneath it (14화).
        auto* Wood = Kit(TEXT("Drift"), FLinearColor(.78f, .86f, .62f));
        auto* Leaf = Kit(TEXT("Grass"), FLinearColor(.55f, 1.15f, .5f));
        const int32 TreeType = Spec->TileNames.IndexOfByKey(TEXT("TREE")), Bush = Spec->TileNames.IndexOfByKey(TEXT("BUSH"));
        auto Rim = [&](int32 X, int32 Y) { return X == 0 || Y == 0 || X == Spec->Width - 1 || Y == Spec->Height - 1; };
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Type = Spec->TileAt(X, Y), Seed = X * 29 + Y * 13;
                // The camera looks north from the south: no rim trees along the south edge, and lower ones near it.
                if (Type == TreeType && Y < Spec->Height - 1 && (!Rim(X, Y) || (X + Y) % 2 == 0))
                {
                    const float Size = (Y >= Spec->Height - 3 ? .78f : Rim(X, Y) ? 1.3f : 1.1f) + .3f * float(Scatter(Seed, 61));
                    const FVector At = TileCentre(X + .3f * (float(Scatter(Seed, 62)) - .5f), Y + .3f * (float(Scatter(Seed, 63)) - .5f));
                    Prop(TEXT("DeadTree"), Wood, FVector(At.X, At.Y, Feet - 6.f), 360.f * float(Scatter(Seed, 64)), FVector(Size, Size, Size * 1.2f));
                }
                else if (Type == Bush && Leaf)
                    for (int32 I = 0; I < 4; ++I)
                        Prop(TEXT("DryGrass"), Leaf, TileCentre(X + .5f * (float(Scatter(Seed + I, 65)) - .5f), Y + .5f * (float(Scatter(Seed + I, 66)) - .5f)) + Ground,
                             360.f * float(Scatter(Seed + I, 67)), FVector(2.2f, 2.2f, 1.8f), false);
            }
        auto* CutFace = Focus(TEXT("CutFace"), FLinearColor(.34f, .3f, .22f), 1);
        const FVector Stump = TileCentre(12.f, 9.f) + Ground;
        Box(Timber, Stump + FVector(0, 0, 16), FVector(50, 50, 32));
        Box(CutFace, Stump + FVector(0, 0, 33), FVector(44, 44, 2), FRotator::ZeroRotator, false);
        Beam(Timber, Stump + FVector(40, 10, 22), TileCentre(14.8f, 9.6f) + Ground + FVector(0, 0, 18), 34.f);
        for (int32 I = 0; I < 4; ++I)
        {
            const FVector B = TileCentre(13.2f + .5f * I, 10.2f) + Ground;
            Beam(Timber, B + FVector(0, 0, 4), B + FVector(30 + 8 * I, -22, 8), 5.f);
        }
        auto* Water = Focus(TEXT("RockWater"), FLinearColor(.16f, .22f, .24f), 2, .1f);
        const FVector Rock = TileCentre(14.7f, 3.2f) + Ground;
        Box(Stone, Rock + FVector(0, 0, 22), FVector(96, 72, 44), FRotator(0, 12, 0));
        Box(Water, Rock + FVector(0, 0, 45), FVector(52, 36, 2), FRotator(0, 12, 0), false);
        const FVector Slab = TileCentre(12.f, -1.7f) + Ground;
        Box(Stone, Slab + FVector(0, 0, 70), FVector(280, 130, 24), FRotator(0, -6, 3));
        auto* Embers = Focus(TEXT("Embers"), FLinearColor(1.f, .42f, .14f), 2, .4f);
        Box(Embers, Slab + FVector(0, -40, 4), FVector(26, 26, 6), FRotator(0, 20, 0), false);
        Lamp(Slab + FVector(0, -40, 34), 360.f, Look.LampIntensity * .7f, false);
    }
    else if (Map == TEXT("forgotten_forest"))
    {
        // S372: the forest's trees stand on its TRUNK tiles and close the CANOPY border; pale mushroom rings
        // follow the roots (8화: "작은 것들이 뿌리를 따라 둥근 줄을 이루었다").
        auto* Wood = Kit(TEXT("Drift"), FLinearColor(.55f, .66f, .5f));
        auto* Cap = Focus(TEXT("Mushroom"), FLinearColor(.75f, .78f, .72f), 2, .7f);
        const int32 Trunk = Spec->TileNames.IndexOfByKey(TEXT("TRUNK")), Canopy = Spec->TileNames.IndexOfByKey(TEXT("CANOPY"));
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                const int32 Type = Spec->TileAt(X, Y), Seed = X * 31 + Y * 17;
                const bool bTree = Type == Trunk || (Type == Canopy && (X + Y) % 2 == 0);
                if (!bTree) continue;
                const float Size = (Type == Trunk ? 1.05f : 1.25f) + .3f * float(Scatter(Seed, 31));
                const FVector At = TileCentre(X + .3f * (float(Scatter(Seed, 32)) - .5f), Y + .3f * (float(Scatter(Seed, 33)) - .5f));
                Prop(TEXT("DeadTree"), Wood, FVector(At.X, At.Y, Feet - 6.f), 360.f * float(Scatter(Seed, 34)), FVector(Size, Size, Size * 1.15f));
                if (Type == Trunk)
                    for (int32 I = 0; I < 5; ++I)
                    {
                        const float A = (I / 5.f + float(Scatter(Seed, 40 + I)) * .1f) * 2.f * PI;
                        Box(Cap, At + FVector(FMath::Cos(A) * 44.f, FMath::Sin(A) * 44.f, Feet + 3.f), FVector(7, 7, 5), FRotator::ZeroRotator, false);
                    }
            }
    }
    else if (Map == TEXT("the_seam") || Map == TEXT("seam_return"))
    {
        // S371: the village's cottages on its HUT tiles (each a 2x2 block): timber walls, a pitched roof and a lit
        // window; a lantern post on each LANTERN tile; firewood under the first eaves (6화: "처마 아래에는 장작").
        const int32 Hut = Spec->TileNames.IndexOfByKey(TEXT("HUT"));
        auto* Roofing = Focus(TEXT("SeamRoof"), Look.TimberTint * .7f + FLinearColor(.02f, .012f, .01f), 1);
        int32 Count = 0;
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
            {
                if (Spec->TileAt(X, Y) != Hut || Spec->TileAt(X - 1, Y) == Hut || Spec->TileAt(X, Y - 1) == Hut) continue;
                const FVector C = TileCentre(X + .5f, Y + .5f) + Ground;
                const float Turn = (Count % 2) ? 90.f : 0.f;
                Box(Timber, C + FVector(0, 0, 66.f), FVector(T * 1.8f, T * 1.8f, 132.f), FRotator(0, Turn, 0));
                Solid(TEXT("Roof"), Roofing, C + FVector(0, 0, 130.f), FVector(T * 2.1f / 100.f, T * 2.1f / 100.f, 1.7f), FRotator(0, Turn, 0));
                Lamp(C + FRotator(0, Turn, 0).RotateVector(FVector(0, -T * .92f, 70.f)), 300.f, Look.LampIntensity * .6f);
                if (Count++ == 0) Prop(TEXT("CrateStack"), Kit(TEXT("Belt"), FLinearColor(1.1f, .9f, .7f)), C + FVector(T * 1.2f, 0, 0), 90.f, FVector(.7f));
            }
        for (int32 Y = 0; Y < Spec->Height; ++Y)
            for (int32 X = 0; X < Spec->Width; ++X)
                if (Spec->TileDetails.IsValidIndex(Spec->TileAt(X, Y)) && Spec->TileDetails[Spec->TileAt(X, Y)] == TEXT("lantern"))
                    LanternPost(TileCentre(X, Y) + FVector(-T * .3f, -T * .3f, 0), 45.f * (X % 4));
        if (Map == TEXT("seam_return"))
        {
            // S375: the village packing (15화): bundles of bedding past the bridge and at the doors; the low stone
            // step before the house where Elia drank, where Sable sits.
            auto* Bedding = Focus(TEXT("Bedding"), FLinearColor(.2f, .17f, .13f), 1, .95f);
            const FVector2D Bundles[] = {{9.4f, 6.6f}, {9.8f, 6.2f}, {14.6f, 6.6f}, {8.6f, 11.4f}, {16.4f, 12.6f}, {5.4f, 8.6f}, {17.6f, 4.4f}};
            for (int32 I = 0; I < UE_ARRAY_COUNT(Bundles); ++I)
            {
                const FVector At = TileCentre(Bundles[I].X, Bundles[I].Y) + Ground;
                const float Yaw = 360.f * float(Scatter(I, 71));
                Box(Bedding, At + FVector(0, 0, 12), FVector(54, 34, 24), FRotator(0, Yaw, 0));
                Beam(Timber, At + FRotator(0, Yaw, 0).RotateVector(FVector(-10, -18, 25)), At + FRotator(0, Yaw, 0).RotateVector(FVector(-10, 18, 25)), 2.f);
            }
            Box(Stone, TileCentre(12.5f, 9.55f) + Ground + FVector(0, 0, 10), FVector(84, 30, 20));
        }
    }
}
void AMemoriaChapterPresentation::BuildGrass(const FDressing& Look)
{
    // Dry tufts on the open soil, as the canvases paint them: thick in the Belt's dead yard, thin in Drift's mud.
    auto* Grass = Kit(TEXT("Grass"), Map == TEXT("forgotten_forest") ? FLinearColor(.5f, 1.05f, .55f) : Map == TEXT("haren_clearing") ? FLinearColor(.66f, 1.25f, .55f) : Map == TEXT("roofless_farms") ? FLinearColor(1.55f, 1.32f, .9f) : Map == TEXT("basin_station") ? FLinearColor(1.2f, 1.15f, 1.f) : Map == TEXT("the_seam") || Map == TEXT("seam_return") ? FLinearColor(.75f, 1.45f, .62f) : Look.bRain ? FLinearColor(.8f, .84f, .92f) : FLinearColor(1.5f, 1.3f, 1.f));
    if (!Grass) return;
    // The cards stand edge-on to the low light and would go black; they give back some of their own colour.
    Grass->SetScalarParameterValue(TEXT("Fill"), Look.bRain ? .22f : .3f);
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale, Share = Look.bRain ? .16f : .36f;
    for (int32 Y = 1; Y < Spec->Height - 1; ++Y)
        for (int32 X = 1; X < Spec->Width - 1; ++X)
        {
            const int32 Type = Spec->TileAt(X, Y);
            const FString Detail = Spec->TileDetails.IsValidIndex(Type) ? Spec->TileDetails[Type] : FString();
            // S371: in the Seam it is the gardens that grow, thick, and a little between the stones.
            const float Here = Map == TEXT("forgotten_forest") ? (Detail == TEXT("dead_soil") ? .62f : 0.f)
                : Map == TEXT("the_seam") || Map == TEXT("seam_return") ? (Detail == TEXT("garden") ? .9f : Detail == TEXT("stone") ? .22f : 0.f)
                : Map == TEXT("haren_clearing") ? (Detail == TEXT("grass") ? .55f : 0.f)
                : Map == TEXT("roofless_farms") ? (Detail == TEXT("dead_soil") ? .5f : 0.f)
                : (Detail == TEXT("dead_soil") || Detail == TEXT("mud")) ? Share : 0.f;
            if (Here <= 0.f || Hash(X, Y, 41) > Here) continue;
            const int32 Tufts = 1 + int32(Hash(X, Y, 43) * 3.f);
            for (int32 I = 0; I < Tufts; ++I)
            {
                const FVector At = TileCentre(X, Y) + FVector((Hash(X * 5 + I, Y, 45) - .5f) * T * .9f, (Hash(X, Y * 5 + I, 47) - .5f) * T * .9f, Feet);
                const float S = 1.1f + .9f * Hash(X + I, Y - I, 49);
                Prop(TEXT("DryGrass"), Grass, At, 360.f * Hash(X - I, Y + I, 51), FVector(S, S, S * (.8f + .5f * Hash(X, Y, 53 + I))), false);
            }
        }
}
void AMemoriaChapterPresentation::BuildAir(const FDressing& Look)
{
    // The lens: the corners fall off, the lamps bloom.
    auto* Post = NewObject<UPostProcessComponent>(this);
    AddInstanceComponent(Post); Post->SetupAttachment(GetRootComponent()); Post->bUnbound = true;
    Post->Settings.bOverride_VignetteIntensity = true; Post->Settings.VignetteIntensity = Look.Vignette;
    Post->Settings.bOverride_BloomIntensity = true; Post->Settings.BloomIntensity = .75f;
    Post->Settings.bOverride_ColorSaturation = true; Post->Settings.ColorSaturation = FVector4(1, 1, 1, .94f * Spec->Saturation);
    Post->Settings.bOverride_ColorContrast = true; Post->Settings.ColorContrast = FVector4(1, 1, 1, 1.06f);
    Post->RegisterComponent();
    // The air: dust adrift in the Belt, rain in Drift. The motes live in a box that travels with Arrel.
    auto* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* Soft = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight"));
    if (!Mesh || !Soft) return;
    auto* Instance = UMaterialInstanceDynamic::Create(Soft, this, TEXT("ChapterMote"));
    Instance->SetVectorParameterValue(TEXT("Tint"), Look.MoteTint); Instance->SetScalarParameterValue(TEXT("Alpha"), bRain ? .5f : .6f);
    for (int32 I = 0; I < Look.MoteCount; ++I)
    {
        auto* Mote = NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(Mote); Mote->SetupAttachment(GetRootComponent()); Mote->SetMobility(EComponentMobility::Movable);
        Mote->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mote->SetGenerateOverlapEvents(false); Mote->SetCanEverAffectNavigation(false); Mote->SetCastShadow(false);
        Mote->SetStaticMesh(Mesh); Mote->SetMaterial(0, Instance); Mote->RegisterComponent(); Motes.Add(Mote);
    }
}
void AMemoriaChapterPresentation::BuildLight()
{
    const FDressing& Look = DressingFor(Map);
    // The key light low and in the map's light colour, a cool fill from the other side, and a haze in its hue.
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It) It->Destroy();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const auto Srgb = [](const FLinearColor& C) { return FLinearColor::FromSRGBColor(C.ToFColor(false)); };
    // A spawned directional light keeps its class's own downward tilt under the rotation it is spawned with, so
    // each one is turned again after spawning. (Until S338 the key light fell almost straight down for this
    // reason, and every wall's face was black.)
    if (auto* Key = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), FRotator(Look.KeyPitch, Look.KeyYaw, 0), Params))
    {
        auto* L = CastChecked<UDirectionalLightComponent>(Key->GetLightComponent()); L->SetMobility(EComponentMobility::Movable);
        Key->SetActorRotation(FRotator(Look.KeyPitch, Look.KeyYaw, 0));
        // The source's light colour, drawn a little towards white: at full strength it paints the whole map one hue.
        const FLinearColor Colour = Srgb(Spec->Light);
        L->SetLightColor(FMath::Lerp(Colour, FLinearColor::White * Colour.GetLuminance(), .35f)); L->SetIntensity(Look.KeyIntensity + 4.f * Spec->Brightness); L->SetLightingChannels(true, true, false);
        L->ForwardShadingPriority = 1; // the key light is the one forward shading, translucency and fog use
        L->SetLightSourceAngle(2.5f); L->SetShadowAmount(.84f); L->DynamicShadowDistanceMovableLight = 6000; L->ShadowBias = .4f;
    }
    if (auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), FRotator(-35, Look.KeyYaw + 175.f, 0), Params))
    {
        auto* L = CastChecked<UDirectionalLightComponent>(Fill->GetLightComponent()); L->SetMobility(EComponentMobility::Movable); L->SetCastShadows(false);
        Fill->SetActorRotation(FRotator(-35, Look.KeyYaw + 175.f, 0));
        L->SetLightColor(FLinearColor(.55f, .62f, .78f)); L->SetIntensity(Look.FillIntensity); L->SetLightingChannels(true, true, false);
        L->ForwardShadingPriority = 0; L->SetAtmosphereSunLight(false);
    }
    // Two faint lights stand in for the sky: one from straight above for the ground a wall shades, one along the
    // camera's line for the faces it looks at. Without them a shadow is black. (A sky light was tried: its
    // capture arrived late in some sessions and washed the map out in others.)
    for (const FRotator& From : {FRotator(-89, 0, 0), FRotator(-28, 90, 0)})
        if (auto* Ambient = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), From, Params))
        {
            auto* L = CastChecked<UDirectionalLightComponent>(Ambient->GetLightComponent()); L->SetMobility(EComponentMobility::Movable); L->SetCastShadows(false);
            Ambient->SetActorRotation(From);
            L->SetLightColor(Look.SkyColor); L->SetIntensity(Look.SkyIntensity); L->SetLightingChannels(true, true, false);
            L->ForwardShadingPriority = 0; L->SetAtmosphereSunLight(false);
        }
    if (auto* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0, 0, -40), FRotator::ZeroRotator, Params))
    {
        auto* F = Fog->GetComponent(); F->SetFogDensity(Look.FogDensity + .01f * Spec->Mood); F->SetFogHeightFalloff(.32f);
        F->SetFogInscatteringColor(Look.FogColor); F->SetStartDistance(900.f); F->SetFogMaxOpacity(.75f);
    }
}
void AMemoriaChapterPresentation::TickEnvironment(float DeltaSeconds)
{
    EnvTime += DeltaSeconds;
    const double Time = EnvTime;
    for (int32 I = 0; I < Lamps.Num(); ++I)
        if (Lamps[I]) Lamps[I]->SetIntensity(LampBase[I] * (1.f + .05f * FMath::Sin(Time * 2.3 + I * 1.9) + .03f * FMath::Sin(Time * 7.1 + I * 4.3)));
    if (!Player.IsValid()) return;
    // The air's box wraps round Arrel, so the motes are always where the camera looks and never pop.
    const FVector Centre = Player->GetActorLocation() + FVector(0, 260, 0);
    const FVector Size = bRain ? FVector(2000, 1500, 520) : FVector(2000, 1500, 320);
    const FVector Wind = bRain ? FVector(-70, 0, -980) : FVector(34, 9, -5);
    for (int32 I = 0; I < Motes.Num(); ++I)
    {
        const FVector Seed(Scatter(I, 31) * Size.X, Scatter(I, 32) * Size.Y, Scatter(I, 33) * Size.Z);
        const double Speed = bRain ? .8 + .4 * Scatter(I, 34) : .5 + Scatter(I, 34);
        FVector Local = Seed + Wind * Speed * Time - Centre;
        Local = FVector(FMath::Fmod(Local.X, Size.X), FMath::Fmod(Local.Y, Size.Y), FMath::Fmod(Local.Z, Size.Z));
        if (Local.X < 0) Local.X += Size.X; if (Local.Y < 0) Local.Y += Size.Y; if (Local.Z < 0) Local.Z += Size.Z;
        FVector World = Centre + Local - FVector(Size.X * .5, Size.Y * .5, 0) + FVector(0, 0, Feet);
        if (!bRain) World += FVector(16. * FMath::Sin(Time * .5 + I), 0, 9. * FMath::Sin(Time * .7 + I * 1.7));
        // The planes face the quarter-view camera (it looks north and down 48 degrees).
        const FVector Scale = bRain ? FVector(.03, .5 + .3 * Scatter(I, 35), 1) : FVector(.045 + .05 * Scatter(I, 35));
        Motes[I]->SetWorldTransform(FTransform(FRotator(0, 0, -42), World, Scale));
    }
    // Walls and timbers between the camera and Arrel open round him (M_FocusSurface, as in Verdan).
    if (ArrelFigure && !FocusMaterials.IsEmpty())
    {
        const auto* Camera = Player->GetFieldCamera();
        const auto Colour = [](const FVector& V) { return FLinearColor(V.X, V.Y, V.Z, 1); };
        const FLinearColor Eye = Colour(Camera->GetComponentLocation()), Target = Colour(ArrelFigure->FocusPosition()), Up = Colour(Camera->GetUpVector());
        for (const auto& M : FocusMaterials)
        { M->SetVectorParameterValue(TEXT("OcclusionEye"), Eye); M->SetVectorParameterValue(TEXT("OcclusionFocus"), Target); M->SetVectorParameterValue(TEXT("OcclusionUp"), Up); }
    }
}
