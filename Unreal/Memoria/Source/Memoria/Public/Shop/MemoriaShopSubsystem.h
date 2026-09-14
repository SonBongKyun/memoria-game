#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MemoriaShopSubsystem.generated.h"
class UMemoriaRunSubsystem;
class UMemoriaMemoryCatalog;
struct MEMORIA_API FMemoriaShopOffer
{
    FString Id, Title, Description;
    int32 Grade = 0;
    int64 BurnPower = 0, Price = 0;
    FString StoryEffect, RelatedNpc;
};
struct MEMORIA_API FMemoriaShopRow
{
    FString Id, Title, Description, StoryEffect;
    int32 Grade = 0;
    int64 Price = 0;
};
// Values only. The widget cannot sell, grant, change chapters, or persist a profile.
struct MEMORIA_API FMemoriaShopView
{
    bool bOpen = false, bEmptyAvailable = false;
    FString Merchant, Mode, Title, Caption, GrainsText, EmptyDetail, PortraitSource;
    TArray<FMemoriaShopRow> Rows;
    TArray<FMemoriaShopOffer> Stock;
    TArray<FString> Requests;
};
UCLASS()
class MEMORIA_API UMemoriaShopSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // Transient owner-bound first screen; duplicate opens are side-effect-free.
    bool OpenMalet(UWorld* Owner = nullptr);
    FMemoriaShopView GetView() const;
    bool IsOpen() const;
    int32 GetOpenCount() const { return OpenCount; }
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaMemoryCatalog> Catalog;
    TWeakObjectPtr<UWorld> OwnerWorld;
    FGuid OwnerRun;
    bool bWorldBound = false;
    int32 OpenCount = 0;
    TArray<FMemoriaShopOffer> Stock;
    TArray<FString> Requests;
    void Reset();
    void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
};
