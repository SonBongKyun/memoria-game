#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "MemoriaCart.generated.h"
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMemoriaFieldCharacterComponent;
class AMemoriaEliaCompanion;
class AMemoriaTobiasCompanion;
class APawn;

// T-24: the uncovered goods cart from 32–35화. One horse, low sides, a hooded driver who
// does not show a face. No lantern and no weapon.
//
// Opus, Chapter 18 — the cart does not know the miles. The chapter owns the stops.
//   AMemoriaCart* Cart = AMemoriaCart::SpawnOnRoute(World, Stops); // world points, cart at Stops[0]
//   Cart->Board();                                                 // seats Arrel, Elia, Tobias; holds move input
//   // Each tick, wait on Cart->IsDriving() or Cart->HasArrived().
//   // GetRouteIndex() is the point being driven toward, and equals Stops.Num() once HasArrived().
//   // Intermediate points are not stops. At the seventh mile call Cart->TurnDriverTowardBed().
//   // Cart->TurnDriverAhead() turns him back toward the horse.
//   Cart->GetDown();        // the three stand on open ground to the cart's right; input returns
//   Cart->SetRoute(NextLeg); // the yard: a new list, then Board() again
//   Cart->Board();
// HasArrived stays true after GetDown until the next Board. SetRoute while they are still
// riding replaces the list and drives immediately. DriveSpeed is a walking 120 cm/s.
UCLASS()
class MEMORIA_API AMemoriaCart : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaCart();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    // Places the cart on the first point, facing the next. Does not move until Board.
    static AMemoriaCart* SpawnOnRoute(UWorld* World, const TArray<FVector>& WorldPoints);
    // Replaces the route. Points the cart already occupies are skipped. If they are riding,
    // the cart drives the new list immediately.
    void SetRoute(const TArray<FVector>& WorldPoints);
    // Seats Arrel, Elia and Tobias, holds the player's move input, and drives. False if any of
    // the three is missing. A second call while riding returns true and does not stack the hold.
    bool Board();
    // Puts the three on the road beside the cart and gives move input back.
    void GetDown();
    // 34화: head and shoulders toward the bed. The hips stay facing the horse.
    void TurnDriverTowardBed();
    void TurnDriverAhead();

    bool IsDriving() const { return bDriving; }
    bool HasArrived() const { return bArrived; }
    bool IsRiding() const { return bRiding; }
    // Index of the point being driven toward. Equals the route length once the cart has arrived.
    int32 GetRouteIndex() const { return RouteIndex; }
    float GetDriverYaw() const;
    bool IsDriverTurnedTowardBed() const;

    static constexpr float DriveSpeed = 120.f;
    static constexpr float ArrivalRadius = 12.f;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> DriverUpper;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SeatArrel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SeatElia;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SeatTobias;
    UPROPERTY() TObjectPtr<UMaterialInterface> WoodMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> HorseMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> HoodMaterial;

    TArray<FVector> Route;
    int32 RouteIndex = 0;
    bool bRiding = false;
    bool bDriving = false;
    bool bArrived = false;
    bool bHeldInput = false;
    bool bRestoreMovementTick = false;
    bool bMovementTickWasEnabled = true;
    bool bRestoreEliaTick = false;
    bool bEliaTickWasEnabled = true;
    bool bRestoreTobiasTick = false;
    bool bTobiasTickWasEnabled = true;
    TWeakObjectPtr<APawn> Rider;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Arrel;
    TWeakObjectPtr<AMemoriaEliaCompanion> Elia;
    TWeakObjectPtr<AMemoriaTobiasCompanion> Tobias;

    void EnsureLooks();
    void ApplyLooks();
    void ConsumeRouteProgress();
    void FaceRiders();
    // One step per engine frame. Actor tick and the core ticker both call this.
    void Advance(float DeltaSeconds);

    FTSTicker::FDelegateHandle DriveTicker;
    uint64 DrivenFrame = MAX_uint64;
};
