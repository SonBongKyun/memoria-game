// S337: the chapter maps dressed past their tiles (AMemoriaChapterPresentation's terrain, light and air).
//
// The Godot maps do not show their tiles: each hides them under one painted canvas (MapEffects.add_map_canvas,
// terrain_alpha 0.0). A flat painting cannot lie under the quarter-view camera, so the port builds what the
// canvas paints: the ground from its open patches (M_ChapterGround, -run=MemoriaChapterGroundAssets), masonry
// walls with broken tops, a border that reads as a place (the Belt's rail line, Drift's ruined walls), lamps,
// dust or rain, and a world that goes on beyond the map. The props the canvas paints (signal post, platform
// shelter, crates, chain fences, tarps) stand here as stand-ins built from boxes until Codex's models arrive
// (claude-handoff.md, 2026-10-02). None of this changes where Arrel can walk: the blocks stay one per solid tile.
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
void AMemoriaChapterPresentation::Lamp(const FVector& Position, float Radius, float Intensity)
{
    // A lantern: a glowing pane in an iron cage, and the light it throws.
    if (GlowMaterial) Box(GlowMaterial, Position, FVector(7, 7, 10), FRotator::ZeroRotator, false);
    auto* Light = NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Light); Light->SetupAttachment(GetRootComponent()); Light->SetMobility(EComponentMobility::Movable);
    Light->SetWorldLocation(Position + FVector(0, -6, 2));
    Light->bUseInverseSquaredFalloff = false; Light->LightFalloffExponent = 2.2f;
    Light->SetLightColor(DressingFor(Map).LampColor); Light->SetIntensity(Intensity); Light->SetAttenuationRadius(Radius);
    Light->SetSourceRadius(10); Light->SetSoftSourceRadius(22); Light->SetCastShadows(false);
    Light->RegisterComponent(); Lamps.Add(Light); LampBase.Add(Intensity);
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
    BuildWalls(Look); BuildBorder(Look); BuildSetPieces(Look); BuildAir(Look);
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
                // A ruin: what is left of a wall, and the stones that fell from it.
                const bool bTurn = Hash(X, Y, 5) > .5f;
                const float Height = 46.f + 74.f * N, Long = T * (.62f + .3f * Hash(X, Y, 7)), Thick = T * (.34f + .14f * Hash(X, Y, 9));
                const FVector Offset((Hash(X, Y, 11) - .5f) * T * .2f, (Hash(X, Y, 13) - .5f) * T * .2f, 0);
                Box(RuinStone, C + Offset + FVector(0, 0, Height * .5f + Feet), FVector(Long, Thick, Height), FRotator(0, (bTurn ? 90.f : 0.f) + (N - .5f) * 14.f, 0));
                Box(RuinStone, C + Offset + FVector(0, 0, Height * .28f + Feet), FVector(Long * .5f, Thick * 1.5f, Height * .56f), FRotator(0, (bTurn ? 0.f : 90.f) + (N - .5f) * 20.f, 0));
                for (int32 I = 0; I < 4; ++I)
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
        // Drift's shelter: poles at the corners of the slab ring, ropes between their heads, a patched tarp
        // leaning over the back row, and a lantern on each pole.
        auto* Cloth = Focus(TEXT("Tarp"), Look.ClothTint, 2, .95f);
        const FVector NW = TileCentre(FloorMin.X, FloorMin.Y - 1), NE = TileCentre(FloorMax.X, FloorMin.Y - 1);
        const FVector SW = TileCentre(FloorMin.X, FloorMax.Y + 1), SE = TileCentre(FloorMax.X - 1, FloorMax.Y + 1);
        const float Head = 214.f + Feet;
        for (const FVector& Pole : {NW, NE, SW, SE})
        {
            Beam(Timber, Pole + FVector(0, 0, Feet + 30), Pole + FVector(0, 0, Head), 9.f);
            Lamp(Pole + FVector(0, -12, Head - 46.f), 430.f, Look.LampIntensity);
            Beam(Iron, Pole + FVector(0, -12, Head - 36.f), Pole + FVector(0, 0, Head - 22.f), 2.f);
        }
        Beam(Iron, NW + FVector(0, 0, Head), NE + FVector(0, 0, Head), 2.5f);
        const FVector Mid = (NW + NE) * .5;
        const float Span = (NE - NW).X;
        for (int32 I = 0; I < 3; ++I)
        {
            // Three sheets lashed side by side, each sagging its own way.
            const float U = (I - 1) * Span / 3.f;
            Box(Cloth, Mid + FVector(U, T * .42f, Head - 34.f - 6.f * I), FVector(Span / 3.f - 6.f, T * 1.25f, 2.5f), FRotator(0, 0, -24.f + 5.f * I));
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
    auto* Iron = Focus(TEXT("BorderIron"), Look.IronTint, 3, .5f, .65f);
    const TCHAR* Block = BlockMesh();
    auto Stand = [&](UMaterialInterface* Material, const FVector& Position, const FVector& Size) { Solid(Block, Material, Position, Size / 100.0); };
    auto Rim = [&](int32 X, int32 Y) { return (X == 0 || Y == 0 || X == Spec->Width - 1 || Y == Spec->Height - 1) && Spec->TileAt(X, Y) == WallType; };
    auto Post = [&](const FVector& C) { Box(Stone, C + FVector(0, 0, 42.f + Feet), FVector(24, 24, 84)); Box(Iron, C + FVector(0, 0, 87.f + Feet), FVector(29, 29, 6)); Box(Iron, C + FVector(0, 0, 94.f + Feet), FVector(11, 11, 9)); };
    auto Chain = [&](const FVector& A, const FVector& B)
    {
        const FVector Top(0, 0, 70.f + Feet), Sag = (A + B) * .5 + FVector(0, 0, 50.f + Feet);
        Beam(Iron, A + Top, Sag, 3.f); Beam(Iron, Sag, B + Top, 3.f);
    };
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
            const int32 Along = bVertical ? Y : X;
            if (Along % 2 == 0)
            {
                Post(C);
                const int32 NX = bVertical ? X : X + 2, NY = bVertical ? Y + 2 : Y;
                if (Rim(NX, NY) && Rim(bVertical ? X : X + 1, bVertical ? Y + 1 : Y)) Chain(C, TileCentre(NX, NY));
            }
        }
}
void AMemoriaChapterPresentation::BuildSetPieces(const FDressing& Look)
{
    // Stand-ins for what the canvas paints, built from boxes. Each stands on a solid tile or beyond the border,
    // so none of them is in Arrel's way. Codex's models replace them.
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    auto* Stone = Focus(TEXT("SetStone"), Look.WallTint * .9f, 0, .93f);
    auto* Timber = Focus(TEXT("SetTimber"), Look.TimberTint * 1.25f, 1);
    auto* Sleeper = Focus(TEXT("Sleeper"), Look.TimberTint * .8f, 1, .95f);
    auto* Iron = Focus(TEXT("SetIron"), Look.IronTint, 3, .45f, .7f);
    auto* Rust = Focus(TEXT("Rust"), FLinearColor(.16f, .085f, .045f), 3, .7f, .35f);
    auto* Cloth = Focus(TEXT("SetCloth"), Look.ClothTint, 2, .95f);
    auto* Slate = Focus(TEXT("Slate"), Look.IronTint * 1.9f, 0, .7f);
    auto* Dark = Focus(TEXT("Far"), Look.RuinTint * .42f, 3, 1.f);
    auto* Ballast = Focus(TEXT("Ballast"), Look.RuinTint * .8f, 3, 1.f);
    auto Crate = [&](const FVector& P, float Size, float Yaw)
    {
        Box(Timber, P + FVector(0, 0, Size * .5f + Feet), FVector(Size), FRotator(0, Yaw, 0));
        Box(Iron, P + FVector(0, 0, Size * .5f + Feet), FVector(Size + 2.f, Size * .12f, Size + 2.f), FRotator(0, Yaw, 0));
        Box(Iron, P + FVector(0, 0, Size * .5f + Feet), FVector(Size * .12f, Size + 2.f, Size + 2.f), FRotator(0, Yaw, 0));
    };
    auto Barrel = [&](const FVector& P)
    {
        Solid(TEXT("Cylinder"), Timber, P + FVector(0, 0, 28.f + Feet), FVector(.42f, .42f, .56f));
        for (float Z : {12.f, 44.f}) Solid(TEXT("Cylinder"), Iron, P + FVector(0, 0, Z + Feet), FVector(.445f, .445f, .05f));
    };
    auto LampPost = [&](const FVector& P, float Tall)
    {
        Box(Stone, P + FVector(0, 0, 9.f + Feet), FVector(26, 26, 18));
        Beam(Iron, P + FVector(0, 0, Feet), P + FVector(0, 0, Tall + Feet), 7.f);
        Beam(Iron, P + FVector(0, 0, Tall + Feet - 4.f), P + FVector(0, -30, Tall + Feet - 4.f), 4.f);
        Beam(Iron, P + FVector(0, -30, Tall + Feet - 4.f), P + FVector(0, -30, Tall + Feet - 16.f), 2.f);
        Lamp(P + FVector(0, -30, Tall + Feet - 24.f), 470.f, Look.LampIntensity * 1.15f);
    };
    if (Map == TEXT("belt_waystation"))
    {
        // The rail line beyond the embankment: ballast, sleepers and two rails, running off both ways.
        const float RailY = TileCentre(0, -2.f).Y, West = -14.f * T, East = (Spec->Width + 14.f) * T;
        Box(Ballast, FVector((West + East) * .5f, RailY, 4.f + Feet), FVector(East - West, 210, 22));
        for (float X = West; X < East; X += 54.f) Box(Sleeper, FVector(X, RailY, 18.f + Feet), FVector(20, 168, 9), FRotator(0, (Scatter(int32(X), 1) - .5) * 5., 0));
        for (float Side : {-48.f, 48.f}) Box(Iron, FVector((West + East) * .5f, RailY + Side, 26.f + Feet), FVector(East - West, 7, 9));
        // The far side: relay pylons carrying a sagging line, and slag ridges fading into the dust.
        const float PylonY = TileCentre(0, -4.6f).Y;
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
        // The signal post on the embankment: a mast, its spoked wheel, three pennants and the lamp at its foot.
        {
            const FVector P = TileCentre(5.f, -.15f);
            Box(Stone, P + FVector(0, 0, 46.f + 14.f + Feet), FVector(54, 54, 28));
            Beam(Iron, P + FVector(0, 0, 46.f + Feet), P + FVector(0, 0, 372.f), 13.f);
            const FVector Hub = P + FVector(0, -11, 300.f);
            Solid(TEXT("Cylinder"), Rust, Hub, FVector(1.12f, 1.12f, .07f), FRotator(0, 0, 90));
            Solid(TEXT("Cylinder"), Iron, Hub + FVector(0, -3, 0), FVector(.86f, .86f, .05f), FRotator(0, 0, 90));
            Solid(TEXT("Cylinder"), Rust, Hub + FVector(0, -6, 0), FVector(.3f, .3f, .08f), FRotator(0, 0, 90));
            for (int32 I = 0; I < 4; ++I) Box(Rust, Hub + FVector(0, -6, 0), FVector(104, 4, 6), FRotator(I * 45.f, 0, 0));
            Box(Iron, P + FVector(0, -6, 206.f), FVector(150, 6, 6));
            for (int32 I = 0; I < 3; ++I) Box(Cloth, P + FVector(-56.f + I * 56.f, -8, 206.f - 26.f - 6.f * (I % 2)), FVector(26, 2, 46.f + 12.f * (I % 2)), FRotator((I - 1) * 5.f, 0, 0));
            Lamp(P + FVector(0, -34, 70.f + Feet), 430.f, Look.LampIntensity);
        }
        // The platform shelter on the embankment: a stone platform, four posts, a curved slatted canopy.
        {
            const FVector P = (TileCentre(17.f, 0.f) + TileCentre(20.f, 0.f)) * .5 + FVector(0, 26, 0);
            Box(Stone, P + FVector(0, 0, 29.f + Feet), FVector(410, 150, 58));
            for (float X : {-185.f, -62.f, 62.f, 185.f}) for (float Y : {-58.f, 58.f}) Beam(Timber, P + FVector(X, Y, 58.f + Feet), P + FVector(X, Y, 226.f), 9.f);
            Solid(TEXT("Roof"), Slate, P + FVector(0, 0, 226.f), FVector(4.5f, 1.85f, .62f));
            Box(Timber, P + FVector(0, -92, 226.f), FVector(450, 6, 12));
            for (int32 I = 0; I < 9; ++I) Box(Timber, P + FVector(-200.f + I * 50.f, -94, 214.f), FVector(10, 3, 18.f + 8.f * (I % 2)));
            Box(Timber, P + FVector(0, 52, 100.f), FVector(380, 5, 70));     // the back screen
            Box(Timber, P + FVector(-110, -10, 78.f + Feet), FVector(120, 30, 8));  // a bench
            for (float X : {-160.f, -60.f}) Box(Iron, P + FVector(X, -10, 66.f + Feet), FVector(6, 28, 18));
            Lamp(P + FVector(120, -84, 176.f), 520.f, Look.LampIntensity * 1.2f);
            Beam(Iron, P + FVector(120, -84, 222.f), P + FVector(120, -84, 184.f), 2.f);
            Crate(P + FVector(150, -20, 58.f), 46.f, 12.f); Crate(P + FVector(196, 10, 58.f), 36.f, -20.f);
        }
        // The banner pole at the north-east corner, its long cloth torn.
        {
            const FVector P = TileCentre(22.6f, -.1f);
            Beam(Iron, P + FVector(0, 0, 46.f + Feet), P + FVector(0, 0, 368.f), 11.f);
            Beam(Iron, P + FVector(0, 0, 350.f), P + FVector(-64, -10, 340.f), 5.f);
            Box(Cloth, P + FVector(-34, -12, 286.f), FVector(34, 2, 104), FRotator(3, 0, 0)); Box(Cloth, P + FVector(-44, -13, 214.f), FVector(16, 2, 40), FRotator(8, 0, 0));
        }
        // The freight left on the south-west ruin, and a cart's worth on the south-east.
        {
            const FVector P = TileCentre(3.f, 14.f);
            Crate(P + FVector(-18, 14, 0), 52.f, 8.f); Crate(P + FVector(30, 20, 0), 40.f, -14.f); Crate(P + FVector(-12, 16, 52.f), 36.f, 24.f);
            Barrel(P + FVector(34, -24, 0)); Barrel(P + FVector(-28, -30, 0));
            Beam(Timber, P + FVector(-44, 40, Feet), P + FVector(-44, 40, 130.f), 6.f); Beam(Timber, P + FVector(44, 40, Feet), P + FVector(44, 40, 130.f), 6.f);
            Beam(Timber, P + FVector(-44, 40, 126.f), P + FVector(44, 40, 126.f), 5.f);
            Box(Cloth, P + FVector(-6, 38, 92.f), FVector(58, 2, 62), FRotator(0, 0, 4));
            const FVector Q = TileCentre(20.f, 14.f);
            Crate(Q + FVector(8, 6, 0), 46.f, 30.f); Barrel(Q + FVector(-30, -18, 0)); Crate(Q + FVector(-22, 24, 0), 30.f, -8.f);
        }
        // Lamp posts where the road leaves the yard.
        for (float Y : {7.f, 11.f}) LampPost(TileCentre(Spec->Width - 1.f, Y), 205.f);
        LampPost(TileCentre(0.f, 12.f) + FVector(10, 0, 0), 205.f);
    }
    else
    {
        // Drift: dead trees beyond the walls, and what the camp left on its ruins.
        auto Tree = [&](const FVector& P, int32 Seed)
        {
            const float Tall = 330.f + 170.f * Scatter(Seed, 11);
            const FVector Crown = P + FVector((Scatter(Seed, 12) - .5) * 90., (Scatter(Seed, 13) - .5) * 60., Tall);
            Beam(Dark, P + FVector(0, 0, Feet - 10.f), Crown, 22.f);
            for (int32 I = 0; I < 5; ++I)
            {
                const FVector From = FMath::Lerp(P, Crown, .42f + .12f * I);
                const double Turn = Scatter(Seed * 7 + I, 14) * 6.283;
                const FVector To = From + FVector(FMath::Cos(Turn) * (130. - 14. * I), FMath::Sin(Turn) * 60., 70. + 30. * Scatter(Seed + I, 15));
                Beam(Dark, From, To, 9.f - I);
                Beam(Dark, To, To + FVector(FMath::Cos(Turn + 1.) * 60., 20., 46.), 4.f);
            }
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
        // The camp's stores on the rubble.
        const FVector P = TileCentre(18.f, 2.f);
        Crate(P + FVector(-10, 4, 0), 44.f, 16.f); Crate(P + FVector(34, -12, 0), 32.f, -22.f); Barrel(P + FVector(-40, -26, 0));
        const FVector Q = TileCentre(3.f, 3.f);
        Crate(Q + FVector(0, 0, 0), 40.f, -10.f); Barrel(Q + FVector(36, 22, 0));
        // A lamp post at the road's two ends.
        LampPost(TileCentre(8.f, 0.f) + FVector(-20, -T * .5f - 10.f, 0), 215.f);
        LampPost(TileCentre(Spec->Width - 1.f, 7.f), 215.f);
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
