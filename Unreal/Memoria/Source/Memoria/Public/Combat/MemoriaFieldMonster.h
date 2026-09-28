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

// A field foe (S313: a void husk or a market thief, FoeSpec) on Epic's mannequin, chasing the player and
// striking after a telegraph. Set the kind before BeginPlay (SpawnWave spawns deferred).
UCLASS()
class MEMORIA_API AMemoriaFieldMonster : public APawn
{
    GENERATED_BODY()
public:
    AMemoriaFieldMonster();
    void SetKind(EMemoriaFoeKind InKind) { Kind = InKind; }
    EMemoriaFoeKind GetKind() const { return Kind; }
    const FMemoriaFoeSpec& Spec() const { return FoeSpec(Kind); }
    virtual void Tick(float DeltaSeconds) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;
    // Returns true when the hit landed (not dead). Staggers and cancels a windup.
    bool TakeHit(float Damage, const FVector& From, float Shove = 14.f);
    // S314: the source burn status on a foe, ticking once a second.
    void Ignite(float TickDamage, int32 Ticks);
    bool IsBurning() const { return IgniteLeft > 0; }
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
    EMemoriaFoeKind Kind = EMemoriaFoeKind::VoidHusk;
    float Health = MemoriaCombatTuning::HuskHealth, MaxHealth = MemoriaCombatTuning::HuskHealth, StateTime = 0.f;
    int32 Strikes = 0, IgniteLeft = 0;
    float IgniteDamage = 0.f, IgniteClock = 0.f;
    FVector PreviousLocation = FVector::ZeroVector;
    void Enter(EMemoriaMonsterState Next);
    UMemoriaFieldCombatSubsystem* Combat() const;
};
