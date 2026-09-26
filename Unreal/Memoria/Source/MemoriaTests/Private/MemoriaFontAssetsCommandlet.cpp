#include "MemoriaFontAssetsCommandlet.h"
#include "Presentation/MemoriaFonts.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaFontAssetsCommandlet::UMemoriaFontAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
namespace
{
bool SaveAsset(UObject* Asset, const FString& PackagePath)
{
    FAssetRegistryModule::AssetCreated(Asset); Asset->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const auto File = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Asset->GetOutermost(), Asset, *File, Args);
}
}
int32 UMemoriaFontAssetsCommandlet::Main(const FString&)
{
    // Additive: existing packages are kept, never refreshed.
    const FString Sources = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("FontSources"));
    for (const auto& Face : MemoriaFonts::Faces())
    {
        const FString FacePath = MemoriaFonts::FacePackage(Face), FontPath = MemoriaFonts::FontPackage(Face);
        if (FPackageName::DoesPackageExist(FontPath)) { UE_LOG(LogTemp, Display, TEXT("MEMORIA_FONT_KEPT %s"), *FontPath); continue; }
        TArray<uint8> Bytes; const FString File = Sources / (FString(Face) + TEXT(".ttf"));
        if (!FFileHelper::LoadFileToArray(Bytes, *File)) { UE_LOG(LogTemp, Error, TEXT("MEMORIA_FONT missing %s; run generate_font_sources.py"), *File); return 1; }
        auto* FaceAsset = NewObject<UFontFace>(CreatePackage(*FacePath), *FPaths::GetBaseFilename(FacePath), RF_Public | RF_Standalone);
        FaceAsset->InitializeFromBulkData(File, EFontHinting::Default, Bytes.GetData(), Bytes.Num());
        FaceAsset->LoadingPolicy = EFontLoadingPolicy::Inline;
        if (!SaveAsset(FaceAsset, FacePath)) return 1;
        auto* Font = NewObject<UFont>(CreatePackage(*FontPath), *FPaths::GetBaseFilename(FontPath), RF_Public | RF_Standalone);
        Font->FontCacheType = EFontCacheType::Runtime;
        FTypefaceEntry Entry(TEXT("Regular")); Entry.Font = FFontData(FaceAsset);
        Font->CompositeFont.DefaultTypeface.Fonts.Add(Entry);
        if (!SaveAsset(Font, FontPath)) return 1;
        UE_LOG(LogTemp, Display, TEXT("MEMORIA_FONT_ASSET %s bytes=%d"), *Font->GetPathName(), Bytes.Num());
    }
    return 0;
}
