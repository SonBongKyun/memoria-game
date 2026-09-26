#include "MemoriaDialogueAssetsCommandlet.h"
#include "Factories/TextureFactory.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaDialogueAssetsCommandlet::UMemoriaDialogueAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaDialogueAssetsCommandlet::Main(const FString&)
{
    // Additive: existing packages are never overwritten; only newly listed sources are imported.
    for (const auto& Entry : MemoriaNarrativeArtwork::Sources())
    {
        if (Entry.bReuse) continue;
        if (FPackageName::DoesPackageExist(Entry.Package)) { UE_LOG(LogTemp, Display, TEXT("DIALOGUE_ASSET_KEPT %s"), Entry.Package); continue; }
        const FString PackagePath(Entry.Package), Name = FPaths::GetBaseFilename(PackagePath);
        auto* Package = CreatePackage(*(PackagePath)); auto* Factory = NewObject<UTextureFactory>();
        bool Cancelled = false;
        const auto File = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../..")/FString(Entry.Source).RightChop(6));
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),Package,*Name,RF_Public|RF_Standalone,*File,nullptr,GWarn,Cancelled));
        if(!Texture || Cancelled) return 1;
        Texture->LODGroup = TEXTUREGROUP_UI; Texture->CompressionSettings = TC_EditorIcon;
        Texture->MipGenSettings = TMGS_NoMipmaps; Texture->SRGB = true; Texture->NeverStream = true;
        Texture->PostEditChange(); FAssetRegistryModule::AssetCreated(Texture); Texture->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const auto Target = FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,Texture,*Target,Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("DIALOGUE_ASSET_SAVED %s source=%s"), *Texture->GetPathName(), *File);
    }
    return 0;
}
