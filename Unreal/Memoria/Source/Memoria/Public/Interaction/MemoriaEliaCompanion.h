#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MemoriaInteractable.h"
#include "MemoriaEliaCompanion.generated.h"
class USphereComponent;
class UPaperSpriteComponent;
class UMemoriaFieldCharacterComponent;
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
    UMemoriaFieldCharacterComponent* GetFigure() const { return Figure; }
    // companion.gd constants, in source pixels (1 unit per pixel in this slice).
    // Godot companion.gd FORMATION_DISTANCE, kept for the source fixture.
    static constexpr double SourceFormationDistance = 48;
    // S307 deviation (user-approved): the illustrated figures are ~80 units wide, so the source 48 put Elia
    // inside Arrel's silhouette. 90 keeps a clear gap behind him along the trail.
    static constexpr double FormationDistance = 90;
    static constexpr double FollowSpeed = 112;
    static constexpr double ArrivalRadius = 7;
    static constexpr double TrailSample = 8;
    static constexpr double WarpDistance = 310;
    static constexpr double FollowAccel = 900;
    static constexpr double SprintCatchup = 1.48;
    static constexpr int32 MaxTrailPoints = 96;
    // The source reaches her 32 beyond where she rests (80 against 48); the same margin past the S307 formation,
    // or the player could not talk to her from where she stops.
    static constexpr double InteractionRange = FormationDistance + 32;
    static constexpr double FacingDot = .7;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Sprite;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shadow;
    TWeakObjectPtr<APawn> Target;
    TArray<FVector> Trail;
    FVector Velocity = FVector::ZeroVector, LastTargetPosition = FVector::ZeroVector;
    FString Direction = TEXT("Down");
    float Age = 0;
    FVector FollowPoint() const;
    void Face(const FVector& Move);
};
