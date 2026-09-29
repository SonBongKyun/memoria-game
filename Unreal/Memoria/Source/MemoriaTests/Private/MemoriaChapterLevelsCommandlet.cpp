#include "MemoriaChapterLevelsCommandlet.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Framework/MemoriaSliceHost.h"
#include "Factories/WorldFactory.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
UMemoriaChapterLevelsCommandlet::UMemoriaChapterLevelsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaChapterLevelsCommandlet::Main(const FString& Params)
{
    for (const TCHAR* Map : {TEXT("belt_waystation"), TEXT("drift_shelter")})
    {
        const auto* Spec = MemoriaChapterMaps::Find(Map);
        if (!Spec) { UE_LOG(LogTemp, Error, TEXT("CHAPTER_LEVEL_SPEC_MISSING %s"), Map); return 1; }
        const FString Path = MemoriaChapterMaps::LevelPath(Map), Name = FPackageName::GetShortName(Path);
        if (FPackageName::DoesPackageExist(Path)) { UE_LOG(LogTemp, Display, TEXT("CHAPTER_LEVEL_KEPT %s"), *Path); continue; }
        auto* Package = CreatePackage(*Path); auto* Factory = NewObject<UWorldFactory>(); Factory->bCreateWorldPartition = false;
        auto* World = Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, FName(Name), RF_Public | RF_Standalone, nullptr, GWarn));
        if (!World) return 1;
        World->GetWorldSettings()->DefaultGameMode = AMemoriaSliceGameMode::StaticClass();
        World->GetWorldSettings()->bForceNoPrecomputedLighting = true;
        World->SpawnActor<APlayerStart>(MemoriaChapterMaps::ToWorld(Spec->Spawn) + FVector(0, 0, 60), FRotator::ZeroRotator);
        Package->SetPackageFlags(PKG_ContainsMap); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const FString File = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetMapPackageExtension());
        if (!UPackage::SavePackage(Package, World, *File, Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("CHAPTER_LEVEL_SAVED %s"), *File);
    }
    return 0;
}
