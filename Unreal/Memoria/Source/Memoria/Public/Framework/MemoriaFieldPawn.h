#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MemoriaFieldPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class UPaperSpriteComponent;
class UFloatingPawnMovement;

// Plane/camera foundation. Collision sizes, art, movement tuning and depth
// sorting must be characterized against Godot in the first editor test map.
UCLASS()
class MEMORIA_API AMemoriaFieldPawn : public APawn
{
    GENERATED_BODY()
public:
    AMemoriaFieldPawn();
    virtual UPawnMovementComponent* GetMovementComponent() const override;
    void ApplyVerdanMovementProfile();
    UCameraComponent* GetFieldCamera() const { return Camera; }
    UPaperSpriteComponent* GetFieldSprite() const { return Sprite; }
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperSpriteComponent> Sprite;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UFloatingPawnMovement> Movement;
};
