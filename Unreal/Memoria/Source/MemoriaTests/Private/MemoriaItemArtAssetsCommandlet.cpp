#include "MemoriaItemArtAssetsCommandlet.h"
#include "Presentation/MemoriaHudKit.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"

UMemoriaItemArtAssetsCommandlet::UMemoriaItemArtAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }

int32 UMemoriaItemArtAssetsCommandlet::Main(const FString& Params)
{
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/ItemsS356"));
    // Check every target before creating anything. Older art and this revision are immutable.
    for (const auto& Item : MemoriaCombatTuning::FieldItems)
    {
        const FString Path = MemoriaHudKit::ItemPackage(Item.Id);
        const FString File = Art / (FString(Item.Id) + TEXT(".png"));
        if (!Path.StartsWith(TEXT("/Game/Memoria/Presentation/ItemsS356/")) || FPackageName::DoesPackageExist(Path))
        { UE_LOG(LogTemp, Error, TEXT("ITEM_ART_REFUSED existing or unexpected target %s"), *Path); return 1; }
        if (!IFileManager::Get().FileExists(*File))
        { UE_LOG(LogTemp, Error, TEXT("ITEM_ART_MISSING %s"), *File); return 1; }
    }
    TArray<UTexture2D*> Imported;
    // Validate all paintings before saving the first package.
    for (const auto& Item : MemoriaCombatTuning::FieldItems)
    {
        const FString Path = MemoriaHudKit::ItemPackage(Item.Id), Name = FPackageName::GetShortName(Path);
        const FString File = Art / (FString(Item.Id) + TEXT(".png"));
        auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
        auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*Path), *Name,
            RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
        if (!Texture || bCancelled || Texture->Source.GetSizeX() < 1024 || Texture->Source.GetSizeY() < 1024
            || Texture->Source.GetFormat() != TSF_BGRA8)
        { UE_LOG(LogTemp, Error, TEXT("ITEM_ART_INVALID must be >=1024px RGBA painting %s"), *File); return 1; }
        TArray64<uint8> Pixels;
        if (!Texture->Source.GetMipData(Pixels, 0))
        { UE_LOG(LogTemp, Error, TEXT("ITEM_ART_INVALID source pixels %s"), *File); return 1; }
        int64 Transparent = 0, Opaque = 0;
        for (int64 I = 3; I < Pixels.Num(); I += 4)
        { Transparent += Pixels[I] <= 2; Opaque += Pixels[I] >= 245; }
        const int64 PixelCount = Pixels.Num() / 4;
        if (Transparent < PixelCount / 5 || Opaque < PixelCount / 20)
        { UE_LOG(LogTemp, Error, TEXT("ITEM_ART_INVALID alpha cutout %s transparent=%lld opaque=%lld"), *File, Transparent, Opaque); return 1; }
        // Keep the complete original painting in source art, but budget its UI resource at 512px.
        Texture->LODGroup = TEXTUREGROUP_UI; Texture->CompressionSettings = TC_EditorIcon;
        Texture->MipGenSettings = TMGS_NoMipmaps; Texture->MaxTextureSize = 512;
        Texture->AddressX = TA_Clamp; Texture->AddressY = TA_Clamp;
        Texture->Filter = TF_Bilinear; Texture->SRGB = true; Texture->NeverStream = true;
        Texture->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Texture});
        Imported.Add(Texture);
        UE_LOG(LogTemp, Display, TEXT("ITEM_ART_VALID %s source=%dx%d ui=%dx%d transparent=%lld opaque=%lld"), *Name,
            int32(Texture->Source.GetSizeX()), int32(Texture->Source.GetSizeY()), Texture->GetSizeX(), Texture->GetSizeY(), Transparent, Opaque);
    }
    for (UTexture2D* Texture : Imported)
    {
        FAssetRegistryModule::AssetCreated(Texture); Texture->MarkPackageDirty();
        const FString Path = Texture->GetOutermost()->GetName();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const bool bSaved = UPackage::SavePackage(Texture->GetOutermost(), Texture,
            *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
        UE_LOG(LogTemp, Display, TEXT("ITEM_ART %s %s"), *Path, bSaved ? TEXT("SAVED") : TEXT("FAILED"));
        if (!bSaved) return 1;
    }
    return 0;
}
