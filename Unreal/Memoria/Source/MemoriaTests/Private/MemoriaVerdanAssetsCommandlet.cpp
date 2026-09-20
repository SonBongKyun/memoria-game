#include "MemoriaVerdanAssetsCommandlet.h"
#include "Presentation/MemoriaVerdanArt.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "TextureCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
namespace
{
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object);
    Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool Result = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("VERDAN_ASSET %s %s"), *Object->GetPathName(), Result ? TEXT("SAVED") : TEXT("FAILED"));
    return Result;
}
template<class T> T* Asset(const FString& Name)
{
    return NewObject<T>(CreatePackage(*MemoriaVerdanArt::Package(Name)), *Name, RF_Public | RF_Standalone);
}
template<class T> T* Expression(UMaterial* Material)
{
    auto* Node = NewObject<T>(Material);
    Material->GetExpressionCollection().AddExpression(Node);
    return Node;
}
}
UMemoriaVerdanAssetsCommandlet::UMemoriaVerdanAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaVerdanAssetsCommandlet::Main(const FString& Params)
{
    // Additive visual refinement: never rewrite the first import or a protected package.
    if (FParse::Param(*Params, TEXT("AddInteriorFloor")))
    {
        const FString Name = TEXT("SPR_FloorInterior");
        if (FPackageName::DoesPackageExist(MemoriaVerdanArt::Package(Name))) return 1;
        auto* Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/T_Market.T_Market"));
        if (!Texture) return 1;
        FTextureCompilingManager::Get().FinishCompilation({Texture});
        for (const auto& Region : MemoriaVerdanArt::Sprites()) if (FString(Region.Name) == TEXT("FloorInterior"))
        {
            auto* Sprite = Asset<UPaperSprite>(Name);
            FSpriteAssetInitParameters Init; Init.Texture = Texture; Init.Offset = Region.Offset;
            Init.Dimension = Region.Size; Init.SetPixelsPerUnrealUnit(Region.PixelsPerUnit);
            Sprite->InitializeSprite(Init);
            Sprite->SetPivotMode(ESpritePivotMode::Custom, FVector2D(Region.Offset) + Region.Pivot);
            return Save(Sprite) ? 0 : 1;
        }
        return 1;
    }
    TArray<FString> Names{TEXT("M_SoftLight")};
    for (const auto& Source : MemoriaVerdanArt::Textures()) Names.Add(FString(TEXT("T_")) + Source.Name);
    for (const auto& Region : MemoriaVerdanArt::Sprites()) Names.Add(FString(TEXT("SPR_")) + Region.Name);
    // Check every destination and source before the first write. Never refresh existing packages.
    for (const auto& Name : Names) if (FPackageName::DoesPackageExist(MemoriaVerdanArt::Package(Name)))
    { UE_LOG(LogTemp, Error, TEXT("Refusing existing presentation package %s"), *Name); return 1; }
    for (const auto& Source : MemoriaVerdanArt::Textures()) if (!FPaths::FileExists(FPaths::ProjectDir()/TEXT("../..")/Source.File)) return 1;
    TMap<FString, UTexture2D*> Textures;
    for (const auto& Source : MemoriaVerdanArt::Textures())
    {
        const FString Name = FString(TEXT("T_")) + Source.Name;
        auto* Package = CreatePackage(*MemoriaVerdanArt::Package(Name));
        auto* Factory = NewObject<UTextureFactory>(); bool Cancelled = false;
        const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../..")/Source.File);
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), Package, *Name, RF_Public | RF_Standalone, *File, nullptr, GWarn, Cancelled));
        if (!Texture || Cancelled) return 1;
        Texture->LODGroup = TEXTUREGROUP_Pixels2D; Texture->CompressionSettings = TC_EditorIcon;
        Texture->Filter = TF_Bilinear; Texture->MipGenSettings = TMGS_NoMipmaps;
        Texture->SRGB = true; Texture->NeverStream = true;
        Texture->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Texture});
        if (!Save(Texture)) return 1;
        Textures.Add(Source.Name, Texture);
    }
    for (const auto& Region : MemoriaVerdanArt::Sprites())
    {
        auto* Sprite = Asset<UPaperSprite>(FString(TEXT("SPR_")) + Region.Name);
        FSpriteAssetInitParameters Init; Init.Texture = Textures[Region.Texture];
        Init.Offset = Region.Offset; Init.Dimension = Region.Size; Init.SetPixelsPerUnrealUnit(Region.PixelsPerUnit);
        Sprite->InitializeSprite(Init);
        Sprite->SetPivotMode(ESpritePivotMode::Custom, FVector2D(Region.Offset) + Region.Pivot);
        if (!Save(Sprite)) return 1;
    }
    auto* Material = Asset<UMaterial>(TEXT("M_SoftLight"));
    Material->BlendMode = BLEND_Translucent; Material->SetShadingModel(MSM_Unlit); Material->TwoSided = true;
    auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material);
    auto* Falloff = Expression<UMaterialExpressionCustom>(Material);
    Falloff->Code = TEXT("return pow(saturate(1.0 - length(UV - 0.5) * 2.0), 2.0);");
    Falloff->OutputType = CMOT_Float1;
    FCustomInput Input; Input.InputName = TEXT("UV"); Input.Input.Connect(0, UV); Falloff->Inputs = {Input};
    auto* Tint = Expression<UMaterialExpressionVectorParameter>(Material); Tint->ParameterName = TEXT("Tint"); Tint->DefaultValue = FLinearColor::Black;
    auto* Alpha = Expression<UMaterialExpressionScalarParameter>(Material); Alpha->ParameterName = TEXT("Alpha"); Alpha->DefaultValue = 0.55f;
    auto* Opacity = Expression<UMaterialExpressionMultiply>(Material); Opacity->A.Connect(0, Falloff); Opacity->B.Connect(0, Alpha);
    Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Tint);
    Material->GetEditorOnlyData()->Opacity.Connect(0, Opacity);
    Material->PostEditChange();
    return Save(Material) ? 0 : 1;
}
