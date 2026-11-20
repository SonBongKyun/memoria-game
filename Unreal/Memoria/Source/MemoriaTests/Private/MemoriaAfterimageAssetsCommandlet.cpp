#include "MemoriaAfterimageAssetsCommandlet.h"
#include "Combat/MemoriaBurnArtS368.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "TextureCompiler.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
namespace
{
const TCHAR* AfterimagePackage = TEXT("/Game/Memoria/Presentation/Combat/M_Afterimage");
const TCHAR* MemoryCardPackage = TEXT("/Game/Memoria/Presentation/Combat/M_MemoryCard");
template<class T> T* Expression(UMaterial* Material)
{
    auto* Node = NewObject<T>(Material);
    Material->GetExpressionCollection().AddExpression(Node);
    return Node;
}
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object);
    Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("AFTERIMAGE_ASSET %s %s"), *Object->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved;
}
FCustomInput Input(const TCHAR* Name, UMaterialExpression* From, int32 Output = 0)
{ FCustomInput Value; Value.InputName = Name; Value.Input.Connect(Output, From); return Value; }
// The card's burn line: noise-broken, rising from the bottom edge. Shared by its colour and its opacity.
const TCHAR* BurnHeight = TEXT(
    "float2 p = UV * float2(7.0, 5.0); float2 i = floor(p); float2 f = frac(p); f = f * f * (3.0 - 2.0 * f);\n"
    "float a = frac(sin(dot(i, float2(127.1, 311.7))) * 43758.5453);\n"
    "float b = frac(sin(dot(i + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);\n"
    "float c = frac(sin(dot(i + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);\n"
    "float d = frac(sin(dot(i + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);\n"
    "float n = lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);\n"
    "float h = (1.0 - UV.y) * 0.78 + n * 0.22;\n"
    "float t = Burn * 1.12 - 0.06;\n");
int32 MakeAfterimage()
{
    if (FPackageName::DoesPackageExist(AfterimagePackage))
    { UE_LOG(LogTemp, Error, TEXT("Refusing existing presentation package %s"), AfterimagePackage); return 1; }
    auto* Material = NewObject<UMaterial>(CreatePackage(AfterimagePackage), TEXT("M_Afterimage"), RF_Public | RF_Standalone);
    // A ghost: unlit and see-through, brightest at its silhouette's edge so the pose reads without hiding the ground.
    Material->BlendMode = BLEND_Translucent; Material->SetShadingModel(MSM_Unlit); Material->TwoSided = false;
    Material->SetMaterialUsage(MATUSAGE_SkeletalMesh);
    auto* Rim = Expression<UMaterialExpressionFresnel>(Material); Rim->Exponent = 2.6f; Rim->BaseReflectFraction = .06f;
    auto* Tint = Expression<UMaterialExpressionVectorParameter>(Material); Tint->ParameterName = TEXT("Tint"); Tint->DefaultValue = FLinearColor(.62f, .74f, 1.f);
    auto* Alpha = Expression<UMaterialExpressionScalarParameter>(Material); Alpha->ParameterName = TEXT("Alpha"); Alpha->DefaultValue = .2f;
    auto* Glow = Expression<UMaterialExpressionCustom>(Material);
    Glow->Code = TEXT("return Tint * (0.55 + 1.9 * Rim);"); Glow->OutputType = CMOT_Float3;
    Glow->Inputs = {Input(TEXT("Tint"), Tint), Input(TEXT("Rim"), Rim)};
    auto* Fade = Expression<UMaterialExpressionCustom>(Material);
    Fade->Code = TEXT("return saturate(Alpha * (0.3 + 0.7 * Rim));"); Fade->OutputType = CMOT_Float1;
    Fade->Inputs = {Input(TEXT("Alpha"), Alpha), Input(TEXT("Rim"), Rim)};
    Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Glow);
    Material->GetEditorOnlyData()->Opacity.Connect(0, Fade);
    Material->PostEditChange();
    return Save(Material) ? 0 : 1;
}
// S368: the burned memory's illustrations and the card they burn on.
int32 MakeBurnArt()
{
    TArray<FString> Packages{MemoryCardPackage};
    for (const auto& Entry : MemoriaBurnArt::Entries) Packages.Add(MemoriaBurnArt::Package(Entry.Name));
    for (const FString& Package : Packages) if (FPackageName::DoesPackageExist(Package))
    { UE_LOG(LogTemp, Error, TEXT("Refusing existing presentation package %s"), *Package); return 1; }
    for (const auto& Entry : MemoriaBurnArt::Entries)
        if (!FPaths::FileExists(FPaths::ProjectDir() / TEXT("../..") / Entry.File))
        { UE_LOG(LogTemp, Error, TEXT("Missing burn art %s"), Entry.File); return 1; }
    for (const auto& Entry : MemoriaBurnArt::Entries)
    {
        const FString Package = MemoriaBurnArt::Package(Entry.Name), Name = FPackageName::GetLongPackageAssetName(Package);
        auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
        const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../..") / Entry.File);
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*Package), *Name, RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
        if (!Texture || bCancelled) return 1;
        // Shown small on a card above Arrel: 1024 is plenty, and keeps the packages light.
        Texture->LODGroup = TEXTUREGROUP_UI; Texture->MaxTextureSize = 1024; Texture->SRGB = true;
        Texture->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Texture});
        if (!Save(Texture)) return 1;
    }
    auto* Material = NewObject<UMaterial>(CreatePackage(MemoryCardPackage), TEXT("M_MemoryCard"), RF_Public | RF_Standalone);
    // The memory's picture (or, without one, a smoke of its grade's colour), burning away from the bottom edge
    // behind a glowing seam. Unlit and see-through, so it reads as a vision, not an object.
    Material->BlendMode = BLEND_Translucent; Material->SetShadingModel(MSM_Unlit); Material->TwoSided = true;
    auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material);
    auto* Card = Expression<UMaterialExpressionTextureSampleParameter2D>(Material); Card->ParameterName = TEXT("Card");
    Card->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
    Card->Coordinates.Connect(0, UV);
    auto* Burn = Expression<UMaterialExpressionScalarParameter>(Material); Burn->ParameterName = TEXT("Burn"); Burn->DefaultValue = 0.f;
    auto* Alpha = Expression<UMaterialExpressionScalarParameter>(Material); Alpha->ParameterName = TEXT("Alpha"); Alpha->DefaultValue = 1.f;
    auto* HasArt = Expression<UMaterialExpressionScalarParameter>(Material); HasArt->ParameterName = TEXT("HasArt"); HasArt->DefaultValue = 1.f;
    auto* Edge = Expression<UMaterialExpressionVectorParameter>(Material); Edge->ParameterName = TEXT("Edge"); Edge->DefaultValue = FLinearColor(1.f, .52f, .16f);
    auto* Colour = Expression<UMaterialExpressionCustom>(Material); Colour->OutputType = CMOT_Float3;
    Colour->Code = FString(BurnHeight) + TEXT(
        "float seam = step(t, h) * (1.0 - saturate((h - t) / 0.07));\n"
        "float border = min(min(UV.x, 1.0 - UV.x), min(UV.y, 1.0 - UV.y));\n"
        "float rim = pow(1.0 - saturate(border * 9.0), 2.0);\n"
        "float3 body = HasArt > 0.5 ? Art * 1.15 : Edge * (0.08 + 0.32 * n * n);\n"
        "return body + Edge * (seam * 4.0 + rim * 0.55);");
    Colour->Inputs = {Input(TEXT("Art"), Card), Input(TEXT("UV"), UV), Input(TEXT("Burn"), Burn), Input(TEXT("Edge"), Edge), Input(TEXT("HasArt"), HasArt)};
    auto* Opacity = Expression<UMaterialExpressionCustom>(Material); Opacity->OutputType = CMOT_Float1;
    Opacity->Code = FString(BurnHeight) + TEXT(
        "float border = saturate(min(min(UV.x, 1.0 - UV.x), min(UV.y, 1.0 - UV.y)) * 14.0);\n"
        "float smoke = HasArt > 0.5 ? 1.0 : (0.1 + 0.5 * n * n) * (1.0 - 0.6 * (1.0 - border));\n"
        "return saturate(step(t, h) * Alpha * smoke * (0.35 + 0.65 * border));");
    Opacity->Inputs = {Input(TEXT("UV"), UV), Input(TEXT("Burn"), Burn), Input(TEXT("Alpha"), Alpha), Input(TEXT("HasArt"), HasArt)};
    Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Colour);
    Material->GetEditorOnlyData()->Opacity.Connect(0, Opacity);
    Material->PostEditChange();
    return Save(Material) ? 0 : 1;
}
}
UMemoriaAfterimageAssetsCommandlet::UMemoriaAfterimageAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaAfterimageAssetsCommandlet::Main(const FString& Params)
{
    // Additive modes: -BurnArt (S368) adds the burn illustrations and M_MemoryCard; the default makes M_Afterimage (S366).
    return FParse::Param(*Params, TEXT("BurnArt")) ? MakeBurnArt() : MakeAfterimage();
}
