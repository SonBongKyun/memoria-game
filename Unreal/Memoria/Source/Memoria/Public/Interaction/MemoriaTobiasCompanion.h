#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemoriaTobiasCompanion.generated.h"
class USphereComponent;
class UPaperSpriteComponent;
class UMemoriaFieldCharacterComponent;
class UStaticMeshComponent;
class APawn;

// T-12: from 23화, Tobias walks Arrel's trail behind Elia. He never blocks movement and never fights.
UCLASS()
class MEMORIA_API AMemoriaTobiasCompanion : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaTobiasCompanion();
    void Follow(APawn* InTarget);
    virtual void Tick(float DeltaSeconds) override;
    UMemoriaFieldCharacterComponent* GetFigure() const { return Figure; }
    // Spawned beside the Elia block, further back so the two do not overlap on the first frame.
    static AMemoriaTobiasCompanion* Join(UWorld* World, APawn* Leader);
    // Elia rests 90 cm back. 198 keeps Tobias behind her, with a clear gap from both.
    static constexpr double FormationDistance = 198;
    static constexpr double ClearGap = 72;
    static constexpr double FollowSpeed = 112;
    static constexpr double ArrivalRadius = 7;
    static constexpr double TrailSample = 8;
    static constexpr double WarpDistance = 460;
    static constexpr double FollowAccel = 900;
    static constexpr double SprintCatchup = 1.48;
    static constexpr int32 MaxTrailPoints = 96;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Sprite;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shadow;
    TWeakObjectPtr<APawn> Target;
    TArray<FVector> Trail;
    FVector Velocity = FVector::ZeroVector, LastTargetPosition = FVector::ZeroVector;
    float Age = 0;
    FVector FollowPoint() const;
    void Face(const FVector& Move);
};
