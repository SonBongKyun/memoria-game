#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "MemoriaFieldMonster.generated.h"
class UFloatingPawnMovement;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMemoriaFieldCharacterComponent;
class UMemoriaFieldCombatSubsystem;

// A void husk: Epic's mannequin in void-dark, chasing the player and striking after a telegraph.
UCLASS()
class MEMORIA_API AMemoriaFieldMonster : public APawn
{
    GENERATED_BODY()
public:
    AMemoriaFieldMonster();
    virtual void Tick(float DeltaSeconds) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;
    // Returns true when the hit landed (not dead). Staggers and cancels a windup.
    bool TakeHit(float Damage, const FVector& From);
    bool IsDead() const { return State == EMemoriaMonsterState::Dead; }
    EMemoriaMonsterState GetState() const { return State; }
    float GetHealth() const { return Health; }
    float GetMaxHealth() const { return MaxHealth; }
    UMemoriaFieldCharacterComponent* GetFigure() const { return Figure; }
    int32 GetStrikeCount() const { return Strikes; }
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UFloatingPawnMovement> Movement;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Telegraph;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TelegraphMaterial;
    EMemoriaMonsterState State = EMemoriaMonsterState::Idle;
    float Health = MemoriaCombatTuning::HuskHealth, MaxHealth = MemoriaCombatTuning::HuskHealth, StateTime = 0.f;
    int32 Strikes = 0;
    FVector PreviousLocation = FVector::ZeroVector;
    void Enter(EMemoriaMonsterState Next);
    UMemoriaFieldCombatSubsystem* Combat() const;
};
