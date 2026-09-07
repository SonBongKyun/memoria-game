#include "MemoriaSliceAssetsCommandlet.h"
#include "Factories/WorldFactory.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/MemoriaSliceHost.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Misc/Parse.h"

UMemoriaSliceAssetsCommandlet::UMemoriaSliceAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaSliceAssetsCommandlet::Main(const FString& Params)
{
    const FString Base = TEXT("/Game/Tests/Campaign/");
    const TArray<FString> Names = {TEXT("L_Ch2VerdanSlice"), TEXT("L_VerdanHost"), TEXT("L_VerdanUnseenFixture")};
    const bool Refresh = FParse::Param(*Params, TEXT("Refresh"));
    for (const auto& Name : Names) if (FPackageName::DoesPackageExist(Base + Name) && !Refresh)
    { UE_LOG(LogTemp, Error, TEXT("Refusing to overwrite %s"), *Name); return 1; }
    for (const auto& Name : Names)
    {
        if (Refresh)
        {
            auto* Existing = LoadObject<UWorld>(nullptr, *(Base + Name + TEXT(".") + Name));
            if (!Existing || Existing->GetWorldSettings()->DefaultGameMode != AMemoriaSliceGameMode::StaticClass()) return 1;
            // Loaded commandlet worlds have no active level collection, so
            // TActorIterator's active-level filter can silently yield no actors.
            const auto Actors = Existing->PersistentLevel->Actors;
            int32 RemovedLabels = 0;
            for (AActor* Actor : Actors)
            {
                if (auto* Mesh = Cast<AStaticMeshActor>(Actor)) Mesh->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
                if (auto* Light = Cast<ADirectionalLight>(Actor)) Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
                if (Cast<ATextRenderActor>(Actor))
                {
                    if (!Existing->EditorDestroyActor(Actor, true)) return 1;
                    ++RemovedLabels;
                }
            }
            UE_LOG(LogTemp, Display, TEXT("MEMORIA_SLICE_REFRESH actors=%d labels_removed=%d"), Actors.Num(), RemovedLabels);
            Existing->GetWorldSettings()->bForceNoPrecomputedLighting = true;
            FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
            const FString File = FPackageName::LongPackageNameToFilename(Base + Name, FPackageName::GetMapPackageExtension());
            if (!UPackage::SavePackage(Existing->GetOutermost(), Existing, *File, Args)) return 1;
            UE_LOG(LogTemp, Display, TEXT("MEMORIA_SLICE_ASSET_SAVED %s"), *File); continue;
        }
        auto* Package = CreatePackage(*(Base + Name)); auto* Factory = NewObject<UWorldFactory>(); Factory->bCreateWorldPartition = false;
        auto* World = Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, FName(Name), RF_Public | RF_Standalone, nullptr, GWarn));
        if (!World) return 1;
        World->GetWorldSettings()->DefaultGameMode = AMemoriaSliceGameMode::StaticClass();
        World->SpawnActor<APlayerStart>(FVector::ZeroVector, FRotator::ZeroRotator);
        World->GetWorldSettings()->bForceNoPrecomputedLighting = true;
        World->SpawnActor<ADirectionalLight>(FVector(0, 0, 500), FRotator(-90, 0, 0))->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        auto Box = [&](const FVector& Position, const FVector& Scale)
        {
            auto* Actor = World->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
            Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            Actor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
            Actor->GetStaticMeshComponent()->SetWorldScale3D(Scale);
        };
        Box(FVector(0, 0, -30), FVector(18, 12, 0.2));
        for (double X : {-900.0, 900.0}) Box(FVector(X, 0, 0), FVector(0.2, 12, 1));
        for (double Y : {-600.0, 600.0}) Box(FVector(0, Y, 0), FVector(18, 0.2, 1));
        // Simple landmarks make camera-follow movement visible. No final map art.
        for (double X : {-400.0, 400.0}) Box(FVector(X, 200, -5), FVector(1, 1, 0.1));
        Package->SetPackageFlags(PKG_ContainsMap); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        const FString File = FPackageName::LongPackageNameToFilename(Base + Name, FPackageName::GetMapPackageExtension());
        if (!UPackage::SavePackage(Package, World, *File, Args)) return 1;
        UE_LOG(LogTemp, Display, TEXT("MEMORIA_SLICE_ASSET_SAVED %s"), *File);
    }
    return 0;
}
