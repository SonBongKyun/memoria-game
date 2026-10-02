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
const TCHAR* EnvArt = TEXT("/Game/Memoria/Presentation/Environment/");
const TCHAR* RubblePath = TEXT("/Game/Memoria/Presentation/Field3D/Props/SM_Rubble.SM_Rubble");
// Codex's S337 polish assets (its lane, -run=MemoriaVisualPolishAssets): a 100 cm block with a bevelled edge and
// a weathered stone material. The walls and slabs wear them where they are imported; a plain cube stands in.
const TCHAR* BevelPath = TEXT("/Game/Memoria/Presentation/VisualPolish/SM_BeveledBlock.SM_BeveledBlock");
const TCHAR* StonePath = TEXT("/Game/Memoria/Presentation/VisualPolish/M_StoneSurface.M_StoneSurface");
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
    return InMap == TEXT("drift_shelter") ? Drift : Belt;
}
FVector AMemoriaChapterPresentation::TileCentre(float X, float Y) const
{ return MemoriaChapterMaps::ToWorld(FVector2D((X + .5f) * Spec->TileSize, (Y + .5f) * Spec->TileSize)); }
UMaterialInstanceDynamic* AMemoriaChapterPresentation::Focus(const TCHAR* Name, const FLinearColor& Tint, float Mode, float Roughness, float Metallic)
{
    auto* Base = LoadObject<UMaterialInterface>(nullptr, FocusSurfacePath);
    auto* Result = UMaterialInstanceDynamic::Create(Base, this, FName(*(FString(TEXT("Chapter")) + Name)));
    Result->SetVectorParameterValue(TEXT("Tint"), Tint); Result->SetScalarParameterValue(TEXT("Mode"), Mode);
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
    UMaterialInstanceDynamic* Slab = nullptr;
    if (auto* Weathered = LoadObject<UMaterialInterface>(nullptr, StonePath, nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        Slab = UMaterialInstanceDynamic::Create(Weathered, this, TEXT("ChapterSlab"));
        Slab->SetVectorParameterValue(TEXT("Tint"), Look.RuinTint * 1.25f); Slab->SetScalarParameterValue(TEXT("Mode"), 2.f); Slab->SetScalarParameterValue(TEXT("Roughness"), .9f);
    }
    else Slab = Focus(TEXT("Slab"), Look.RuinTint * 1.25f, 0, .9f);
    const TCHAR* Block = BlockMesh();
    auto Stand = [&](UMaterialInterface* Material, const FVector& Position, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
    { Solid(Block, Material, Position, Size / 100.0, Rotation); };
    auto* Timber = Focus(TEXT("Timber"), Look.TimberTint, 1);
    auto* Iron = Focus(TEXT("Iron"), Look.IronTint, 3, .5f, .65f);
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
                Stand(Masonry, C + FVector(0, 0, Height * .5f + Feet), FVector(T, T, Height));
                Stand(Masonry, C + FVector(0, 0, 7.f + Feet), FVector(T + 9, T + 9, 14));
                if (bTall || N > .4f) Stand(Cap, C + FVector(0, 0, Height + 3.5f + Feet), FVector(T + 4, T + 4, 7));
                if (bJamb || bDoorSide)
                {
                    // A lantern on an iron bracket at each side of the door, on the face the camera sees.
                    const float Side = Open(X - 1, Y) ? -1.f : 1.f;
                    const FVector Hook = C + FVector(Side * (T * .5f - 14.f), -T * .5f - 16.f, 132.f);
                    Beam(Iron, Hook + FVector(0, 16, 14), Hook + FVector(0, 0, 14), 3.f);
                    Beam(Iron, Hook + FVector(0, 0, 14), Hook + FVector(0, 0, 7), 2.f);
                    Lamp(Hook, 420.f, Look.LampIntensity);
                }
                continue;
            }
            if (Type == Concrete && !Border(X, Y))
            {
                // The fallen overpass: low slabs lying a little out of true.
                const float Height = 34.f + 18.f * N;
                Stand(Slab, C + FVector(0, 0, Height * .5f + Feet), FVector(T * .97f, T * .97f, Height), FRotator(0, (N - .5f) * 7.f, (Hash(X, Y, 3) - .5f) * 5.f));
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
                if (Free(X + 1, Y)) { Taken.Add(FIntPoint(X + 1, Y)); Prop(TEXT("RuinWall"), RuinKit, (C + TileCentre(X + 1, Y)) * .5 + Ground, Turn); }
                else if (Free(X, Y + 1)) { Taken.Add(FIntPoint(X, Y + 1)); Prop(TEXT("RuinWall"), RuinKit, (C + TileCentre(X, Y + 1)) * .5 + Ground, 90.f + Turn); }
                else Prop(TEXT("RuinWall"), RuinKit, C + Ground, 360.f * Hash(X, Y, 7), FVector(.6f));
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
        const int32 Row = FloorMin.Y - 1;
        int32 From = FloorMin.X;
        for (int32 X = FloorMin.X; X <= FloorMax.X + 1; ++X)
        {
            if (X <= FloorMax.X && !Open(X, Row)) continue;
            if (X - From >= 2)
            {
                const FVector P = (TileCentre(From, Row) + TileCentre(X - 1, Row)) * .5 + FVector(0, 0, Feet);
                const float Wide = (X - From) * T / 384.f, Deep = .6f;
                Prop(TEXT("TarpCanopy"), Canvas, P, From == FloorMin.X ? 0.f : 180.f, FVector(Wide, Deep, 1));
                for (float Side : {-180.f, 180.f})
                {
                    const FVector Pole = P + FVector(Side * Wide, -130.f * Deep, 0);
                    Beam(Iron, Pole + FVector(0, 0, 176.f), Pole + FVector(0, -13, 170.f), 2.f);
                    Beam(Iron, Pole + FVector(0, -13, 170.f), Pole + FVector(0, -13, 160.f), 1.5f);
                    Lamp(Pole + FVector(0, -13, 153.f), 430.f, Look.LampIntensity);
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
                Stand(Stone, C + FVector(0, 0, Height * .5f + Feet), FVector(T, T, Height));
                if (N > .45f && !bSouth) Stand(Stone, C + FVector((N - .7f) * 30.f, 0, Height + 14.f + Feet), FVector(T * .55f, T * .8f, 28));
                if (N < .3f) Box(Cap, C + FVector(0, 0, Height + 3.f + Feet), FVector(T + 3, T + 3, 6));
                continue;
            }
            if (bNorth)
            {
                // The Belt: the rail line's embankment along the north.
                Stand(Stone, C + FVector(0, 0, 23.f + Feet), FVector(T, T, 46));
                Box(Cap, C + FVector(0, -T * .5f + 9.f, 49.f + Feet), FVector(T, 20, 7));
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
            Lamp(P + FVector(0, -30, 32), 430.f, Look.LampIntensity, false);
        }
        // The platform shelter between the embankment and the rails, on a plinth where it overhangs the bank.
        {
            const FVector P((TileCentre(17.f, 0.f).X + TileCentre(20.f, 0.f).X) * .5, 30.f, Bank.Z);
            Box(Stone, FVector(P.X, 82.f, (Bank.Z + Feet) * .5f), FVector(380, 164, Bank.Z - Feet));
            Prop(TEXT("PlatformShelter"), Lit, P);
            for (float Side : {-152.f, 152.f}) Lamp(P + FVector(Side, -114, 182), 460.f, Look.LampIntensity * 1.1f, false);
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
    else
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
}
void AMemoriaChapterPresentation::BuildGrass(const FDressing& Look)
{
    // Dry tufts on the open soil, as the canvases paint them: thick in the Belt's dead yard, thin in Drift's mud.
    auto* Grass = Kit(TEXT("Grass"), Look.bRain ? FLinearColor(.8f, .84f, .92f) : FLinearColor(1.5f, 1.3f, 1.f));
    if (!Grass) return;
    // The cards stand edge-on to the low light and would go black; they give back some of their own colour.
    Grass->SetScalarParameterValue(TEXT("Fill"), Look.bRain ? .22f : .3f);
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale, Share = Look.bRain ? .16f : .36f;
    for (int32 Y = 1; Y < Spec->Height - 1; ++Y)
        for (int32 X = 1; X < Spec->Width - 1; ++X)
        {
            const int32 Type = Spec->TileAt(X, Y);
            const FString Detail = Spec->TileDetails.IsValidIndex(Type) ? Spec->TileDetails[Type] : FString();
            if ((Detail != TEXT("dead_soil") && Detail != TEXT("mud")) || Hash(X, Y, 41) > Share) continue;
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
        auto* L = Key->GetComponent(); L->SetMobility(EComponentMobility::Movable);
        Key->SetActorRotation(FRotator(Look.KeyPitch, Look.KeyYaw, 0));
        // The source's light colour, drawn a little towards white: at full strength it paints the whole map one hue.
        const FLinearColor Colour = Srgb(Spec->Light);
        L->SetLightColor(FMath::Lerp(Colour, FLinearColor::White * Colour.GetLuminance(), .35f)); L->SetIntensity(Look.KeyIntensity + 4.f * Spec->Brightness); L->SetLightingChannels(true, true, false);
        L->ForwardShadingPriority = 1; // the key light is the one forward shading, translucency and fog use
        L->SetLightSourceAngle(2.5f); L->SetShadowAmount(.84f); L->DynamicShadowDistanceMovableLight = 6000; L->ShadowBias = .4f;
    }
    if (auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 800), FRotator(-35, Look.KeyYaw + 175.f, 0), Params))
    {
        auto* L = Fill->GetComponent(); L->SetMobility(EComponentMobility::Movable); L->SetCastShadows(false);
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
            auto* L = Ambient->GetComponent(); L->SetMobility(EComponentMobility::Movable); L->SetCastShadows(false);
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
