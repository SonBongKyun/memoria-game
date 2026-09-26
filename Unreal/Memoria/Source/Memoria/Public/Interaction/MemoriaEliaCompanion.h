#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MemoriaInteractable.h"
#include "MemoriaEliaCompanion.generated.h"
class USphereComponent;
class UPaperSpriteComponent;
class UStaticMeshComponent;

// Source scripts/core/companion.gd in Verdan: Elia walks the player's trail and is
// talked to by facing her. Query-only; she never blocks movement or collides.
UCLASS()
class MEMORIA_API AMemoriaEliaCompanion : public AActor, public IMemoriaInteractable
{
    GENERATED_BODY()
public:
    AMemoriaEliaCompanion();
    void Follow(APawn* InTarget);
    virtual bool CanInteract(const APawn& Pawn) const override;
    virtual FString InteractionPrompt() const override { return TEXT("Elia  |  E / A: talk"); }
    virtual bool Interact(APawn& Pawn) override;
    virtual void Tick(float DeltaSeconds) override;
    FString Facing() const { return Direction; }
    // companion.gd constants, in source pixels (1 unit per pixel in this slice).
    static constexpr double FormationDistance = 48;
    static constexpr double FollowSpeed = 112;
    static constexpr double ArrivalRadius = 7;
    static constexpr double TrailSample = 8;
    static constexpr double WarpDistance = 310;
    static constexpr double FollowAccel = 900;
    static constexpr double SprintCatchup = 1.48;
    static constexpr int32 MaxTrailPoints = 96;
    static constexpr double InteractionRange = 80;
    static constexpr double FacingDot = .7;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Sprite;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shadow;
    TWeakObjectPtr<APawn> Target;
    TArray<FVector> Trail;
    FVector Velocity = FVector::ZeroVector, LastTargetPosition = FVector::ZeroVector;
    FString Direction = TEXT("Down");
    float Age = 0;
    FVector FollowPoint() const;
    void Face(const FVector& Move);
};
