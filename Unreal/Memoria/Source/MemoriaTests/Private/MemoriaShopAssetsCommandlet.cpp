#include "MemoriaShopAssetsCommandlet.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaShopAssetsCommandlet::UMemoriaShopAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaShopAssetsCommandlet::Main(const FString&)
{
    const FString Base = TEXT("/Game/Memoria/Presentation/Shop/");
    const TCHAR* Names[] = {TEXT("T_MaletPortrait"), TEXT("T_ShopBackdrop")};
    const TCHAR* Sources[] = {TEXT("assets/portraits/malet_face_neutral.png"), TEXT("assets/cg/generated/ui_memory_shop_backdrop_v2.png")};
    for (const auto* Name : Names) if(FPackageName::DoesPackageExist(Base + Name))
    { UE_LOG(LogTemp, Error, TEXT("Refusing existing package: %s"), Name); return 1; }
    for (int32 I = 0; I < 2; ++I)
    {
        auto* Package = CreatePackage(*(Base + Names[I])); auto* Factory = NewObject<UTextureFactory>();
        bool Cancelled = false;
        const auto File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../..")/Sources[I]);
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),Package,Names[I],RF_Public|RF_Standalone,*File,nullptr,GWarn,Cancelled));
        if(!Texture || Cancelled) return 1;
        Texture->LODGroup = TEXTUREGROUP_UI; Texture->CompressionSettings = TC_EditorIcon;
        Texture->MipGenSettings = TMGS_NoMipmaps; Texture->SRGB = true; Texture->NeverStream = true;
        Texture->PostEditChange(); FAssetRegistryModule::AssetCreated(Texture); Texture->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const auto Target = FPackageName::LongPackageNameToFilename(Base+Names[I],FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,Texture,*Target,Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("SHOP_ASSET_SAVED %s source=%s"), *Texture->GetPathName(), *File);
    }
    return 0;
}
