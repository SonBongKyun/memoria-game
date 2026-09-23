#include "MemoriaBattleEntryAssetsCommandlet.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "TextureCompiler.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaBattleEntryAssetsCommandlet::UMemoriaBattleEntryAssetsCommandlet()
{ IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true; }
int32 UMemoriaBattleEntryAssetsCommandlet::Main(const FString& Params)
{
    const bool bCheck=FParse::Param(*Params,TEXT("CheckOnly"));
    const bool bStudyOnly=FParse::Param(*Params,TEXT("StudyOnly"));
    const bool bAlleyRatStudyOnly=FParse::Param(*Params,TEXT("AlleyRatStudyOnly"));
    if(bStudyOnly && bAlleyRatStudyOnly)return 1;
    const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../.."));
    // Preflight every fixed source and destination before creating any package.
    for(const auto& Entry:MemoriaBattleEntryArt::Sources())
    {
        if((bStudyOnly && FString(Entry.Source)!=MemoriaBattleEntryArt::MarketThiefStudySource()) ||
           (bAlleyRatStudyOnly && FString(Entry.Source)!=MemoriaBattleEntryArt::AlleyRatStudySource()))continue;
        if(!FString(Entry.Package).StartsWith(TEXT("/Game/Memoria/Presentation/BattleEntry/")))return 1;
        const FString File=Root/Entry.RelativeFile;
        if(!FPaths::FileExists(File))
        { UE_LOG(LogTemp,Error,TEXT("BATTLE_ENTRY_SOURCE_MISSING %s"),*File);return 1; }
        if(!bCheck && FPackageName::DoesPackageExist(Entry.Package))
        { UE_LOG(LogTemp,Error,TEXT("Refusing existing battle-entry package: %s"),Entry.Package);return 1; }
    }
    for(const auto& Entry:MemoriaBattleEntryArt::Sources())
    {
        if((bStudyOnly && FString(Entry.Source)!=MemoriaBattleEntryArt::MarketThiefStudySource()) ||
           (bAlleyRatStudyOnly && FString(Entry.Source)!=MemoriaBattleEntryArt::AlleyRatStudySource()))continue;
        if(bCheck)
        {
            auto* Texture=MemoriaBattleEntryArt::Load(Entry.Source);
            if(!Texture)
            { UE_LOG(LogTemp,Error,TEXT("BATTLE_ENTRY_ASSET_LOAD_FAILED %s"),Entry.Package);return 1; }
            // NullRHI deliberately skips platform data during PostLoad. Validate
            // the saved editor source; GPU resource validation belongs to the rendered test.
            const FIntPoint ImportedSize=Texture->GetImportedSize();
            if(!Texture->Source.IsValid() || ImportedSize.X<=0 || ImportedSize.Y<=0)
            { UE_LOG(LogTemp,Error,TEXT("BATTLE_ENTRY_ASSET_SOURCE_INVALID %s source=%dx%d platform=%dx%d"),
                *Texture->GetPathName(),ImportedSize.X,ImportedSize.Y,Texture->GetSizeX(),Texture->GetSizeY());return 1; }
            UE_LOG(LogTemp,Display,TEXT("BATTLE_ENTRY_ASSET_CHECKED %s %dx%d source_valid=true"),*Texture->GetPathName(),ImportedSize.X,ImportedSize.Y);
            continue;
        }
        const FString PackagePath(Entry.Package),Name=FPaths::GetBaseFilename(PackagePath),File=Root/Entry.RelativeFile;
        auto* Package=CreatePackage(*PackagePath);auto* Factory=NewObject<UTextureFactory>();bool Cancelled=false;
        auto* Texture=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),Package,*Name,RF_Public|RF_Standalone,*File,nullptr,GWarn,Cancelled));
        if(!Texture || Cancelled)return 1;
        Texture->LODGroup=TEXTUREGROUP_UI;Texture->CompressionSettings=TC_EditorIcon;
        Texture->MipGenSettings=TMGS_NoMipmaps;Texture->SRGB=true;Texture->NeverStream=true;
        Texture->Filter=Entry.bPixelArt?TF_Nearest:TF_Trilinear;
        Texture->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({Texture});
        FAssetRegistryModule::AssetCreated(Texture);Texture->MarkPackageDirty();
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
        const FString Target=FPackageName::LongPackageNameToFilename(PackagePath,FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,Texture,*Target,Args))return 1;
        const FIntPoint ImportedSize=Texture->GetImportedSize();
        UE_LOG(LogTemp,Display,TEXT("BATTLE_ENTRY_ASSET_SAVED %s source=%s size=%dx%d"),*Texture->GetPathName(),*File,ImportedSize.X,ImportedSize.Y);
    }
    return 0;
}
