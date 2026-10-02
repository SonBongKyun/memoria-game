#include "MemoriaHudAssetsCommandlet.h"
#include "Presentation/MemoriaHudKit.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TextureCompiler.h"
#include "UObject/SavePackage.h"
UMemoriaHudAssetsCommandlet::UMemoriaHudAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaHudAssetsCommandlet::Main(const FString& Params)
{
    struct FPlate { MemoriaHudKit::EArt Art; const TCHAR* File; };
    const FPlate Plates[] = {{MemoriaHudKit::EArt::Plate, TEXT("hud_plate.png")}, {MemoriaHudKit::EArt::Toast, TEXT("hud_toast.png")}, {MemoriaHudKit::EArt::Ribbon, TEXT("hud_ribbon.png")}};
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/Hud"));
    const bool bRefresh = FParse::Param(*Params, TEXT("Refresh"));
    for (const FPlate& Plate : Plates)
    {
        const FString Path(MemoriaHudKit::Package(Plate.Art)), Name = FPackageName::GetShortName(Path);
        if (!bRefresh && MemoriaHudKit::Load(Plate.Art)) { UE_LOG(LogTemp, Display, TEXT("HUD_ASSET_KEPT %s"), *Name); continue; }
        const FString File = Art / Plate.File;
        auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
        auto* T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*Path), *Name, RF_Public | RF_Standalone, *File, nullptr, GWarn, bCancelled));
        if (!T || bCancelled) { UE_LOG(LogTemp, Error, TEXT("HUD_ASSET_FAILED %s"), *File); return 1; }
        // Interface art: its own pixels, no mips, never streamed, alpha kept.
        T->LODGroup = TEXTUREGROUP_UI; T->CompressionSettings = TC_EditorIcon; T->MipGenSettings = TMGS_NoMipmaps;
        T->Filter = TF_Bilinear; T->SRGB = true; T->NeverStream = true;
        T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
        FAssetRegistryModule::AssetCreated(T); T->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const bool bSaved = UPackage::SavePackage(T->GetOutermost(), T, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
        UE_LOG(LogTemp, Display, TEXT("HUD_ASSET %s %dx%d %s"), *Name, T->GetSizeX(), T->GetSizeY(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
        if (!bSaved) return 1;
    }
    return 0;
}
