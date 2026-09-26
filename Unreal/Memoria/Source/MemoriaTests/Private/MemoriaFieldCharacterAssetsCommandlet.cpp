#include "MemoriaFieldCharacterAssetsCommandlet.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "TextureCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaFieldCharacterAssetsCommandlet::UMemoriaFieldCharacterAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
namespace
{
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
}
// down.png -> Down, walk_right_1.png -> WalkRight1.
FString ViewName(const FString& File)
{
    TArray<FString> Parts; FPaths::GetBaseFilename(File).ToLower().ParseIntoArray(Parts, TEXT("_"));
    FString Out; for (const auto& P : Parts) Out += P.Left(1).ToUpper() + P.Mid(1);
    return Out;
}
}
int32 UMemoriaFieldCharacterAssetsCommandlet::Main(const FString&)
{
    // Additive: existing packages are kept. Canvas 1024x1536, feet at (512, 1480).
    const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/sprites/field_hd"));
    TArray<FString> Ids; IFileManager::Get().FindFiles(Ids, *(Root / TEXT("*")), false, true);
    int32 Imported = 0;
    for (const FString& Id : Ids)
    {
        const FString Name = Id.Left(1).ToUpper() + Id.Mid(1).ToLower();
        TArray<FString> Files; IFileManager::Get().FindFiles(Files, *(Root / Id / TEXT("*.png")), true, false);
        for (const FString& File : Files)
        {
            const FString View = ViewName(File);
            const FString TexturePath = FString::Printf(TEXT("/Game/Memoria/Presentation/FieldHD/T_%s_%s"), *Name, *View);
            const FString SpritePath = FString::Printf(TEXT("/Game/Memoria/Presentation/FieldHD/SPR_%s_%s"), *Name, *View);
            if (FPackageName::DoesPackageExist(SpritePath)) { UE_LOG(LogTemp, Display, TEXT("FIELD_HD_KEPT %s"), *SpritePath); continue; }
            auto* Factory = NewObject<UTextureFactory>(); bool Cancelled = false;
            auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*TexturePath), *FPaths::GetBaseFilename(TexturePath),
                RF_Public | RF_Standalone, *(Root / Id / File), nullptr, GWarn, Cancelled));
            if (!Texture || Cancelled) { UE_LOG(LogTemp, Error, TEXT("FIELD_HD import failed %s/%s"), *Id, *File); return 1; }
            // Drawn at a fraction of its size: keep mips and filtering, alpha-capable compression.
            Texture->LODGroup = TEXTUREGROUP_Character; Texture->CompressionSettings = TC_Default; Texture->SRGB = true; Texture->Filter = TF_Trilinear;
            Texture->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({Texture});
            if (Texture->Source.GetSizeX() != 1024 || Texture->Source.GetSizeY() != 1536)
                UE_LOG(LogTemp, Warning, TEXT("FIELD_HD %s/%s is %dx%d, not the 1024x1536 spec canvas"), *Id, *File, int32(Texture->Source.GetSizeX()), int32(Texture->Source.GetSizeY()));
            if (!Save(Texture)) return 1;
            auto* Sprite = NewObject<UPaperSprite>(CreatePackage(*SpritePath), *FPaths::GetBaseFilename(SpritePath), RF_Public | RF_Standalone);
            FSpriteAssetInitParameters Init; Init.Texture = Texture; Init.Offset = FIntPoint(0, 0);
            Init.Dimension = FIntPoint(int32(Texture->Source.GetSizeX()), int32(Texture->Source.GetSizeY())); Init.SetPixelsPerUnrealUnit(10.f);
            Sprite->InitializeSprite(Init);
            const float Sx = Texture->Source.GetSizeX() / 1024.f, Sy = Texture->Source.GetSizeY() / 1536.f;
            Sprite->SetPivotMode(ESpritePivotMode::Custom, FVector2D(512.f * Sx, 1480.f * Sy));
            if (!Save(Sprite)) return 1;
            ++Imported; UE_LOG(LogTemp, Display, TEXT("FIELD_HD_ASSET %s"), *Sprite->GetPathName());
        }
    }
    UE_LOG(LogTemp, Display, TEXT("FIELD_HD_IMPORTED %d"), Imported);
    return 0;
}
