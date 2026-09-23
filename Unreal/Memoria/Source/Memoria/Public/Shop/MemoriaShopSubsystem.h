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
    bool bSold = false;
};
struct MEMORIA_API FMemoriaShopRow
{
    FString Id, Title, Description, StoryEffect;
    int32 Grade = 0;
    int64 Price = 0;
};
struct MEMORIA_API FMemoriaShopToast { FString Text; int32 Type = 0; };
DECLARE_MULTICAST_DELEGATE(FMemoriaShopChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FMemoriaShopGrainsChanged, int64);
// Values only. The widget cannot sell, grant, change chapters, or persist a profile.
struct MEMORIA_API FMemoriaShopView
{
    bool bOpen = false, bEmptyAvailable = false, bClosed = false;
    uint64 Revision = 0;
    int64 Grains = 0;
    FString Feedback;
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
    bool SetMode(const FString& Mode);
    bool Transact(const FString& Mode, const FString& Id, uint64 ExpectedRevision);
    bool Close(uint64 ExpectedRevision);
    const TArray<FMemoriaShopToast>& GetToasts() const { return Toasts; }
    FMemoriaShopChanged OnChanged;
    FMemoriaShopGrainsChanged OnGrainsChanged;
    int32 GetOpenCount() const { return OpenCount; }
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaMemoryCatalog> Catalog;
    TWeakObjectPtr<UWorld> OwnerWorld;
    FGuid OwnerRun;
    bool bWorldBound = false, bClosed = false, bBusy = false;
    uint64 Revision = 0;
    FString Mode = TEXT("sell");
    TArray<FMemoriaShopToast> Toasts;
    int32 OpenCount = 0;
    TArray<FMemoriaShopOffer> Stock;
    TArray<FString> Requests;
    void Reset();
    void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
};
