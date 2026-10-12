#include "MemoriaEnvironmentS356Commandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "TextureCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace
{
const FString Base = TEXT("/Game/Memoria/Presentation/EnvironmentS356/");
const TCHAR* Names[] = {TEXT("T_ArchitectureAtlas"), TEXT("M_ArchitectureSurface"),
    TEXT("SM_TiledRoof"), TEXT("SM_RoofEaves"), TEXT("SM_DoorWood"), TEXT("SM_DoorStone"),
    TEXT("SM_WindowWood"), TEXT("SM_WindowStone"), TEXT("SM_ClothCanopy"), TEXT("SM_CanopyRope"),
    TEXT("SM_WallCourse"), TEXT("SM_ChippedCap"), TEXT("SM_RubbleCluster"), TEXT("SM_RuinTuft"), TEXT("SM_ShelterRigging")};

bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("ENV_S356 %s saved=%d"), *Object->GetPathName(), bSaved);
    return bSaved;
}
template<class T> T* Node(UMaterial* M)
{ auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
void Input(UMaterialExpressionCustom* N, const TCHAR* Name, UMaterialExpression* Expression)
{ FCustomInput I; I.InputName = Name; I.Input.Connect(0, Expression); N->Inputs.Add(I); }

bool Material()
{
    const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/EnvironmentS356/ArchitectureAtlas.png"));
    auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface"));
    if (!Source || !FPaths::FileExists(File))
    { UE_LOG(LogTemp, Error, TEXT("ENV_S356 atlas or immutable focus source missing: %s"), *File); return false; }
    auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
    auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*(Base + Names[0])), Names[0],
        RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
    if (!Texture || bCancelled) return false;
    FTextureCompilingManager::Get().FinishCompilation({Texture}); Texture->PreEditChange(nullptr);
    Texture->SRGB = true; Texture->LODGroup = TEXTUREGROUP_World; Texture->NeverStream = true;
    Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::StretchToPowerOfTwo;
    Texture->Filter = TF_Trilinear; Texture->CompressionSettings = TC_Default;
    Texture->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Texture});
    if (!Save(Texture)) return false;
    // Copy, never rewrite, the existing focus shader. The dithered occlusion ray,
    // parameter names, normal/roughness/metallic graph and ISM usage stay intact.
    auto* M = DuplicateObject<UMaterial>(Source, CreatePackage(*(Base + Names[1])), Names[1]);
    M->SetFlags(RF_Public | RF_Standalone); M->bUsedWithInstancedStaticMeshes = true;
    auto* Region = Node<UMaterialExpressionScalarParameter>(M); Region->ParameterName = TEXT("AtlasRegion"); Region->DefaultValue = 0;
    auto* UV = Node<UMaterialExpressionCustom>(M); UV->OutputType = CMOT_Float2;
    // The generated panels meet within a few percent of the half-frame. A generous
    // inset keeps roof/plaster/stone/timber separate even in distant mip levels.
    UV->Code = TEXT("float r=clamp(floor(Region+.5),0,3);return float2(fmod(r,2),floor(r/2))*.5+.02+frac(UV)*.44;");
    Input(UV, TEXT("UV"), Node<UMaterialExpressionTextureCoordinate>(M)); Input(UV, TEXT("Region"), Region);
    auto* Paint = Node<UMaterialExpressionTextureSampleParameter2D>(M); Paint->ParameterName = TEXT("ArchitectureAtlas");
    Paint->Texture = Texture; Paint->SamplerType = SAMPLERTYPE_Color; Paint->Coordinates.Connect(0, UV);
    // A separate architecture tint leaves the cloned Tint expression and defaults
    // intact, without duplicate parameter definitions inside the preserved graph.
    auto* Tint = Node<UMaterialExpressionVectorParameter>(M); Tint->ParameterName = TEXT("ArchitectureTint"); Tint->DefaultValue = FLinearColor::White;
    auto* Color = Node<UMaterialExpressionMultiply>(M); Color->A.Connect(0, Paint); Color->B.Connect(0, Tint);
    M->GetEditorOnlyData()->BaseColor.Connect(0, Color);
    auto* Fill = Node<UMaterialExpressionMultiply>(M); Fill->A.Connect(0, Color); Fill->ConstB = .035f;
    M->GetEditorOnlyData()->EmissiveColor.Connect(0, Fill);
    M->PostEditChange(); return Save(M);
}

struct FKitMesh
{
    FMeshDescription Description;
    FPolygonGroupID Group;
    int32 Triangles = 0;
    FKitMesh()
    {
        FStaticMeshAttributes A(Description); A.Register(); A.GetVertexInstanceUVs().SetNumChannels(1);
        Group = Description.CreatePolygonGroup(); A.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Surface");
    }
    void Tri(FVector A, FVector B, FVector C)
    {
        const FVector N = FVector::CrossProduct(B - A, C - A).GetSafeNormal(); if (N.IsNearlyZero()) return;
        FStaticMeshAttributes Attributes(Description); TArray<FVertexInstanceID> Corners;
        const FVector Tangent = FVector::CrossProduct(FMath::Abs(N.Z) > .95 ? FVector::RightVector : FVector::UpVector, N).GetSafeNormal();
        for (const FVector& P : {A, B, C})
        {
            const auto Vertex = Description.CreateVertex(); Attributes.GetVertexPositions()[Vertex] = FVector3f(P);
            const auto VI = Description.CreateVertexInstance(Vertex); Corners.Add(VI);
            Attributes.GetVertexInstanceNormals()[VI] = FVector3f(N); Attributes.GetVertexInstanceTangents()[VI] = FVector3f(Tangent);
            Attributes.GetVertexInstanceBinormalSigns()[VI] = 1; Attributes.GetVertexInstanceColors()[VI] = FVector4f(1, 1, 1, 1);
            FVector2D UV;
            if (FMath::Abs(N.Z) > .6) UV = FVector2D(P.X, P.Y);
            else if (FMath::Abs(N.Y) > FMath::Abs(N.X)) UV = FVector2D(P.X, P.Z);
            else UV = FVector2D(P.Y, P.Z);
            Attributes.GetVertexInstanceUVs().Set(VI, 0, FVector2f(UV * .015));
        }
        Description.CreatePolygon(Group, Corners); ++Triangles;
    }
    void Quad(FVector A, FVector B, FVector C, FVector D)
    { Tri(A, B, C); Tri(A, C, D); }
    void Box(FVector Centre, FVector Size, double Bevel = .6, FRotator Rotation = FRotator::ZeroRotator)
    {
        const double X = Size.X * .5, Y = Size.Y * .5, Z = Size.Z * .5;
        const double B = FMath::Min(Bevel, FMath::Min3(X, Y, Z) * .45);
        const FTransform T(Rotation, Centre); TArray<TArray<FVector>> Rings;
        for (int32 L = 0; L < 4; ++L)
        {
            const double Inset = L == 0 || L == 3 ? B : 0;
            const double H = L == 0 ? -Z : L == 1 ? -Z + B : L == 2 ? Z - B : Z;
            const double RX = X - Inset, RY = Y - Inset;
            TArray<FVector> Ring;
            for (const FVector2D& P : {FVector2D(-RX + B, -RY), FVector2D(RX - B, -RY), FVector2D(RX, -RY + B), FVector2D(RX, RY - B),
                FVector2D(RX - B, RY), FVector2D(-RX + B, RY), FVector2D(-RX, RY - B), FVector2D(-RX, -RY + B)})
                Ring.Add(T.TransformPosition(FVector(P.X, P.Y, H)));
            Rings.Add(Ring);
        }
        for (int32 L = 0; L < 3; ++L) for (int32 I = 0; I < 8; ++I)
            Quad(Rings[L][I], Rings[L][(I + 1) % 8], Rings[L + 1][(I + 1) % 8], Rings[L + 1][I]);
        for (int32 I = 0; I < 8; ++I)
        {
            Tri(T.TransformPosition(FVector(0, 0, -Z)), Rings[0][(I + 1) % 8], Rings[0][I]);
            Tri(T.TransformPosition(FVector(0, 0, Z)), Rings[3][I], Rings[3][(I + 1) % 8]);
        }
    }
    void Beam(FVector A, FVector B, double Width)
    { Box((A + B) * .5, FVector(Width, Width, (B - A).Size()), Width * .17, FRotationMatrix::MakeFromZ(B - A).Rotator()); }
    bool Write(const TCHAR* Name, bool bAtlas = true)
    {
        auto* Mesh = NewObject<UStaticMesh>(CreatePackage(*(Base + Name)), Name, RF_Public | RF_Standalone);
        const FString MaterialPath = bAtlas ? Base + TEXT("M_ArchitectureSurface.M_ArchitectureSurface") : TEXT("/Game/Memoria/Presentation/Depth2/M_FocusSurface.M_FocusSurface");
        auto* Surface = LoadObject<UMaterialInterface>(nullptr, *MaterialPath); if (!Surface) return false;
        Mesh->GetStaticMaterials().Add(FStaticMaterial(Surface, TEXT("Surface")));
        UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bBuildSimpleCollision = false; Params.bFastBuild = false;
        if (!Mesh->BuildFromMeshDescriptions({&Description}, Params)) return false;
        FStaticMeshCompilingManager::Get().FinishAllCompilation();
        UE_LOG(LogTemp, Display, TEXT("ENV_S356_MESH %s triangles=%d extent=%s"), Name, Triangles, *Mesh->GetBounds().BoxExtent.ToString());
        return Save(Mesh);
    }
};

bool Roofs()
{
    FKitMesh Tiles, Eaves;
    // 192 overlapping, individually curved terracotta tiles on two slopes.
    for (double Side : {-1.0, 1.0}) for (int32 Row = 0; Row < 6; ++Row) for (int32 Col = 0; Col < 16; ++Col)
    {
        const double X0 = -50 + Col * 6.25, Y0 = Row * (50.0 / 6), Y1 = FMath::Min(50.0, (Row + 1) * (50.0 / 6) + .8);
        auto P = [&](double U, double Y) { return FVector(X0 + U * 6.05, Side * Y, 42 - .84 * Y + .9 + .65 * FMath::Sin(U * PI)); };
        for (int32 Arc = 0; Arc < 4; ++Arc)
        {
            const double U = Arc / 4.0, V = (Arc + 1) / 4.0;
            if (Side > 0) Tiles.Quad(P(U, Y0), P(V, Y0), P(V, Y1), P(U, Y1));
            else Tiles.Quad(P(V, Y0), P(U, Y0), P(U, Y1), P(V, Y1));
            Tiles.Quad(P(U, Y1), P(V, Y1), P(V, Y1) - FVector(0, 0, .65), P(U, Y1) - FVector(0, 0, .65));
        }
    }
    // The ridge wears overlapping half-round caps, with visible end profiles.
    for (int32 Col = 0; Col < 16; ++Col) for (int32 Arc = 0; Arc < 8; ++Arc)
    {
        const double A = Arc * PI / 8, B = (Arc + 1) * PI / 8, X0 = -50 + Col * 6.25, X1 = X0 + 6.0;
        auto P = [](double X, double Angle) { return FVector(X, 2.25 * FMath::Cos(Angle), 42.5 + 2.25 * FMath::Sin(Angle)); };
        Tiles.Quad(P(X1, A), P(X0, A), P(X0, B), P(X1, B));
    }
    for (double Side : {-1.0, 1.0})
    {
        Eaves.Box(FVector(0, Side * 50, -.4), FVector(103, 2.8, 3.8), .4);
        for (double X : {-50.0, 50.0}) Eaves.Beam(FVector(X, 0, 42), FVector(X, Side * 50, 0), 2.8);
    }
    Eaves.Box(FVector(0, 0, 40), FVector(104, 3, 3), .4);
    for (int32 I = 0; I < 9; ++I) for (double Side : {-1.0, 1.0})
        Eaves.Beam(FVector(-46 + I * 11.5, Side * 45, 0), FVector(-46 + I * 11.5, Side * 51, -3), 1.8);
    return Tiles.Write(TEXT("SM_TiledRoof")) && Eaves.Write(TEXT("SM_RoofEaves"));
}
bool Facades()
{
    FKitMesh Door, DoorStone, Window, WindowStone;
    for (int32 I = 0; I < 7; ++I) Door.Box(FVector(-27 + I * 9, 1, 63), FVector(8.5, 4, 124), .6);
    for (double X : {-33.0, 33.0}) Door.Box(FVector(X, -2, 65), FVector(6, 8, 132), .8);
    for (double Z : {5.0, 120.0}) Door.Box(FVector(0, -3, Z), FVector(63, 5, 7), .5);
    Door.Beam(FVector(-26, -4.5, 12), FVector(26, -4.5, 114), 4);
    for (double Z : {30.0, 96.0}) Door.Box(FVector(-18, -6, Z), FVector(25, 2, 4), .35);
    Door.Box(FVector(18, -7, 64), FVector(4, 5, 8), .8);
    for (double Side : {-1.0, 1.0}) for (int32 I = 0; I < 5; ++I)
        DoorStone.Box(FVector(Side * 41, 0, 13 + I * 25), FVector(13 + I % 2, 14, 24), 1.8, FRotator(0, 0, Side * (I % 2) * .6));
    for (int32 I = 0; I < 3; ++I) DoorStone.Box(FVector((I - 1) * 28, 0, 135), FVector(27, 16, 15 + (I == 1 ? 3 : 0)), 1.7);
    DoorStone.Box(FVector(0, -4, 1.5), FVector(88, 25, 7), 1.1);
    for (double Side : {-1.0, 1.0})
    {
        Window.Box(FVector(Side * 28, 0, 0), FVector(5, 7, 86), .5);
        Window.Box(FVector(0, -3, Side * 42), FVector(61, 6, 5), .5);
        for (int32 I = 0; I < 7; ++I) Window.Box(FVector(Side * 42, -1, -36 + I * 12), FVector(20, 5, 11), .65, FRotator(0, Side * 10, 0));
        for (double Z : {-26.0, 26.0}) Window.Box(FVector(Side * 42, -5, Z), FVector(21, 2, 4), .3);
        for (int32 I = 0; I < 4; ++I) WindowStone.Box(FVector(Side * 34, 4, -35 + I * 23), FVector(11, 12, 22), 1.3);
    }
    Window.Box(FVector(0, -4, 0), FVector(4, 4, 80), .4); Window.Box(FVector(0, -4, 0), FVector(52, 4, 4), .4);
    for (int32 I = 0; I < 3; ++I) WindowStone.Box(FVector((I - 1) * 25, 4, 48), FVector(24, 12, 12), 1.3);
    WindowStone.Box(FVector(0, -3, -49), FVector(105, 23, 8), 1.7);
    for (double X : {-27.0, 27.0}) WindowStone.Box(FVector(X, 1, -57), FVector(13, 14, 10), 1.2);
    return Door.Write(TEXT("SM_DoorWood")) && DoorStone.Write(TEXT("SM_DoorStone")) &&
        Window.Write(TEXT("SM_WindowWood")) && WindowStone.Write(TEXT("SM_WindowStone"));
}
double CanopyHeight(double X, double Y)
{ return 31.5 * (1 - FMath::Abs(Y) / 64) - 4.5 * (1 - FMath::Square(X / 70)) * FMath::Sin(FMath::Abs(Y) / 64 * PI); }
bool Canopy()
{
    FKitMesh Cloth, Rope;
    for (int32 X = 0; X < 20; ++X) for (int32 Y = 0; Y < 16; ++Y)
    {
        auto P = [](int32 U, int32 V) { const double Xp = -70 + U * 7, Yp = -64 + V * 8; return FVector(Xp, Yp, CanopyHeight(Xp, Yp)); };
        Cloth.Quad(P(X, Y), P(X + 1, Y), P(X + 1, Y + 1), P(X, Y + 1));
    }
    for (double Side : {-1.0, 1.0}) for (int32 I = 0; I < 40; ++I)
    {
        const double X0 = -70 + I * 3.5, X1 = X0 + 3.5;
        const double A = -10 - 3 * FMath::Cos(I * PI / 4), B = -10 - 3 * FMath::Cos((I + 1) * PI / 4);
        Cloth.Quad(FVector(X0, Side * 64, 0), FVector(X1, Side * 64, 0), FVector(X1, Side * 64, B), FVector(X0, Side * 64, A));
        Rope.Beam(FVector(X0, Side * 64.2, A), FVector(X1, Side * 64.2, B), .65);
    }
    for (double X : {-56.0, -28.0, 0.0, 28.0, 56.0}) for (int32 I = 0; I < 16; ++I)
    {
        const double A = -64 + I * 8, B = A + 8;
        Rope.Beam(FVector(X, A, CanopyHeight(X, A) + .25), FVector(X, B, CanopyHeight(X, B) + .25), .55);
    }
    Rope.Beam(FVector(-71, 0, 32), FVector(71, 0, 32), 1.15);
    for (double X : {-42.0, 42.0}) for (double Y : {-38.0, 38.0})
    {
        Rope.Box(FVector(X, Y, CanopyHeight(X, Y)), FVector(8.5, 8.5, 2), .5);
        Rope.Beam(FVector(X, Y, CanopyHeight(X, Y)), FVector(X, Y * 1.08, -18), .9);
    }
    return Cloth.Write(TEXT("SM_ClothCanopy"), false) && Rope.Write(TEXT("SM_CanopyRope"));
}
bool Ruins()
{
    FKitMesh Wall, Cap, Rubble, Tuft;
    // A 100cm course unit retains the exact old wall envelope after per-cell scaling.
    for (int32 Row = 0; Row < 4; ++Row)
    {
        const double Height = 24.1, Z = 12.5 + Row * 25;
        const int32 Count = Row % 2 == 0 ? 3 : 4;
        for (int32 I = 0; I < Count; ++I)
        {
            const double Width = 100.0 / Count;
            Wall.Box(FVector(-50 + Width * (I + .5), 0, Z), FVector(Width - .9, 99.5, Height), 2.0 + (I % 2) * .7);
        }
    }
    for (int32 X = 0; X < 3; ++X) for (int32 Y = 0; Y < 2; ++Y)
        Cap.Box(FVector(-33.3 + X * 33.3, -25 + Y * 50, 4.0 + ((X + Y) % 2) * .7),
            FVector(32.4, 48.8, 7.9 + ((X + Y) % 2) * 1.4), 2.0, FRotator(0, (X + Y - 2) * .9, (X - 1) * .7));
    for (int32 I = 0; I < 10; ++I)
    {
        const double A = I * 2.39996, R = 8 + (I % 4) * 7.5;
        Rubble.Box(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 4 + (I % 3) * 2), FVector(15 + (I % 3) * 4, 12 + (I % 2) * 5, 8 + (I % 3) * 4),
            2.5, FRotator((I % 3 - 1) * 9, I * 37, (I % 4 - 2) * 6));
    }
    for (int32 I = 0; I < 19; ++I)
    {
        const double A = I * 2.39996, R = (I % 4) * 3.5, H = 16 + (I % 5) * 4;
        const FVector BaseP(FMath::Cos(A) * R, FMath::Sin(A) * R, 0), Bend(FMath::Cos(A) * (8 + I % 4), FMath::Sin(A) * (8 + I % 4), H);
        const FVector Across(-FMath::Sin(A) * 1.8, FMath::Cos(A) * 1.8, 0);
        const FVector Mid = BaseP + Bend * .65;
        Tuft.Quad(BaseP - Across, BaseP + Across, Mid + Across * .5, Mid - Across * .5);
        Tuft.Tri(Mid - Across * .5, Mid + Across * .5, BaseP + Bend);
    }
    return Wall.Write(TEXT("SM_WallCourse")) && Cap.Write(TEXT("SM_ChippedCap")) &&
        Rubble.Write(TEXT("SM_RubbleCluster")) && Tuft.Write(TEXT("SM_RuinTuft"), false);
}
bool ShelterRigging()
{
    FKitMesh Joinery;
    // The delivered canopy's four poles are at +/-180, +/-130. Braces and rope
    // bindings sit on those same supports, entirely above its existing footprint.
    for (double X : {-180.0, 180.0}) for (double Y : {-130.0, 130.0})
    {
        const double SX = FMath::Sign(X), SY = FMath::Sign(Y);
        Joinery.Beam(FVector(X, Y, 153), FVector(X - SX * 42, Y, 196), 5.5);
        Joinery.Beam(FVector(X, Y, 156), FVector(X, Y - SY * 36, 196), 5);
        for (double Z : {43.0, 164.0, 194.0})
            for (int32 Turn = 0; Turn < 3; ++Turn) Joinery.Box(FVector(X, Y, Z + Turn * 2.2), FVector(13, 13, 1.5), .55);
    }
    for (double Y : {-130.0, 130.0})
    {
        FVector Last(-180, Y, 198);
        for (int32 I = 1; I <= 16; ++I)
        {
            const double X = -180 + I * 22.5;
            const FVector Next(X, Y, 198 - 11 * (1 - FMath::Square(X / 180)));
            Joinery.Beam(Last, Next, 1.3); Last = Next;
        }
    }
    return Joinery.Write(TEXT("SM_ShelterRigging"));
}
}
UMemoriaEnvironmentS356Commandlet::UMemoriaEnvironmentS356Commandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaEnvironmentS356Commandlet::Main(const FString&)
{
    for (const TCHAR* Name : Names) if (FPackageName::DoesPackageExist(Base + Name))
    { UE_LOG(LogTemp, Error, TEXT("Refusing to overwrite immutable S356 environment package %s"), Name); return 1; }
    return Material() && Roofs() && Facades() && Canopy() && Ruins() && ShelterRigging() ? 0 : 1;
}
