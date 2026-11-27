#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemoriaHannaCompanion.generated.h"
class USphereComponent;
class UPaperSpriteComponent;
class UMemoriaFieldCharacterComponent;
class UStaticMeshComponent;
class APawn;

// T-21: Hanna walks Arrel's trail behind Tobias, then leaves. She never blocks movement, never fights, and never speaks.
// Opus: before setting story flag hanna_gone, call SetLeavingPoint with the alley mouth in world space
// (MemoriaChapterMaps::ToWorld if the chapter has source XY). Then set the flag. She walks there and is removed.
// If the point is still unset when the flag flips, she walks DefaultLeavingDistance toward -Y and is removed.
// A later load with hanna_gone set does not spawn her. To bring her in mid-map, call Join; the chapter spawn block runs only when the map loads.
UCLASS()
class MEMORIA_API AMemoriaHannaCompanion : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaHannaCompanion();
    void Follow(APawn* InTarget);
    virtual void Tick(float DeltaSeconds) override;
    UMemoriaFieldCharacterComponent* GetFigure() const { return Figure; }
    static AMemoriaHannaCompanion* Join(UWorld* World, APawn* Leader);
    static void SetLeavingPoint(const FVector& WorldPoint);
    static bool HasLeavingPoint();
    static FVector GetLeavingPoint();
    static void ClearLeavingPoint();
    // Tobias rests near 198. 310 keeps Hanna behind him, with a clear gap.
    static constexpr double FormationDistance = 310;
    static constexpr double ClearGap = 84;
    static constexpr double FollowSpeed = 112;
    static constexpr double ArrivalRadius = 7;
    static constexpr double TrailSample = 8;
    static constexpr double WarpDistance = 460;
    static constexpr double FollowAccel = 900;
    static constexpr double SprintCatchup = 1.48;
    static constexpr int32 MaxTrailPoints = 96;
    static constexpr double DefaultLeavingDistance = 280;
    static constexpr double LeaveSpeed = 140;
    static constexpr double LeaveArrival = 28;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Sprite;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shadow;
    TWeakObjectPtr<APawn> Target;
    TArray<FVector> Trail;
    FVector Velocity = FVector::ZeroVector, LastTargetPosition = FVector::ZeroVector, LeaveTarget = FVector::ZeroVector;
    float Age = 0;
    bool bLeaving = false;
    FVector FollowPoint() const;
    void Face(const FVector& Move);
};
