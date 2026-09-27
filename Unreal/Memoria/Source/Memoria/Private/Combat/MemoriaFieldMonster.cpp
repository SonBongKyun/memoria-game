#include "Combat/MemoriaFieldMonster.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Materials/MaterialInstanceDynamic.h"
using namespace MemoriaCombatTuning;
AMemoriaFieldMonster::AMemoriaFieldMonster()
{
    PrimaryActorTick.bCanEverTick = true;
    auto* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Box->SetBoxExtent(FVector(30, 30, 8)); Box->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(Box);
    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Box); Movement->SetPlaneConstraintNormal(FVector::UpVector); Movement->SetPlaneConstraintEnabled(true);
    Movement->MaxSpeed = HuskSpeed; Movement->Acceleration = 900.f; Movement->Deceleration = 1400.f;
    Figure = CreateDefaultSubobject<UMemoriaFieldCharacterComponent>(TEXT("Figure"));
    Figure->SetupAttachment(Box);
    // The telegraph: a thin red disc under the husk while it winds up, the reach of its strike.
    Telegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Telegraph"));
    Telegraph->SetupAttachment(Box); Telegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision); Telegraph->SetCastShadow(false);
    Telegraph->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Telegraph->SetRelativeLocation(FVector(0, 0, -8.5)); Telegraph->SetRelativeScale3D(FVector(HuskReach / 50.f, HuskReach / 50.f, .004f));
    Telegraph->SetHiddenInGame(true);
    AutoPossessAI = EAutoPossessAI::Disabled;
}
UPawnMovementComponent* AMemoriaFieldMonster::GetMovementComponent() const { return Movement; }
UMemoriaFieldCombatSubsystem* AMemoriaFieldMonster::Combat() const { return GetWorld() ? GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>() : nullptr; }
void AMemoriaFieldMonster::BeginPlay()
{
    Super::BeginPlay();
    Figure->InitializeMannequin(HuskHeight, FLinearColor(.045f, .03f, .075f));
    if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        TelegraphMaterial = UMaterialInstanceDynamic::Create(Base, this);
        TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.45f, .02f, .03f));
        Telegraph->SetMaterial(0, TelegraphMaterial);
    }
    PreviousLocation = GetActorLocation();
}
void AMemoriaFieldMonster::Enter(EMemoriaMonsterState Next)
{
    State = Next; StateTime = 0.f;
    Telegraph->SetHiddenInGame(Next != EMemoriaMonsterState::Windup);
    // The strike reuses the mannequin's heavy swing, slowed across the windup and the blow.
    if (Next == EMemoriaMonsterState::Windup) Figure->PlayAction(MemoriaCombatClips::Charged(), 1.1f);
}
bool AMemoriaFieldMonster::TakeHit(float Damage, const FVector& From)
{
    if (IsDead()) return false;
    Health = FMath::Max(0.f, Health - Damage);
    const FVector Away = (GetActorLocation() - From).GetSafeNormal2D();
    if (Health <= 0.f)
    {
        Enter(EMemoriaMonsterState::Dead);
        Figure->SetAim((-Away).Rotation().Yaw);
        Figure->PlayAction(MemoriaCombatClips::Death(), 1.f, true);
        SetActorEnableCollision(false);
        if (auto* C = Combat()) C->NotifyMonsterDied(this);
        return true;
    }
    Enter(EMemoriaMonsterState::Stagger);
    Figure->PlayAction(MemoriaCombatClips::Hit(), 1.2f);
    // A short shove away from the blow.
    SetActorLocation(GetActorLocation() + Away * 14.f, true);
    return true;
}
void AMemoriaFieldMonster::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    StateTime += DeltaSeconds;
    const FVector Step = GetActorLocation() - PreviousLocation; PreviousLocation = GetActorLocation();
    Figure->AdvanceLocomotion(Step, DeltaSeconds);
    if (IsDead()) { if (StateTime > HuskCorpseTime) Destroy(); return; }
    auto* C = Combat(); APawn* Target = C ? C->GetPlayer() : nullptr;
    if (!Target || (C && C->IsDefeated())) { if (State != EMemoriaMonsterState::Idle) Enter(EMemoriaMonsterState::Idle); return; }
    const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()) * FVector(1, 1, 0);
    const float Distance = ToTarget.Size();
    auto FaceTarget = [&] { if (Distance > 1.f) Figure->SetAim(ToTarget.Rotation().Yaw); };
    switch (State)
    {
    case EMemoriaMonsterState::Idle:
        if (Distance < HuskAggro) Enter(EMemoriaMonsterState::Chase);
        break;
    case EMemoriaMonsterState::Chase:
        FaceTarget();
        if (Distance <= HuskReach * .85f) Enter(EMemoriaMonsterState::Windup);
        // Moved directly: pawn movement only applies input under a local controller, and husks have none.
        else SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal() * HuskSpeed * DeltaSeconds, true);
        break;
    case EMemoriaMonsterState::Windup:
        FaceTarget();
        if (TelegraphMaterial) TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.45f + .5f * StateTime / HuskWindup, .02f, .03f));
        if (StateTime >= HuskWindup)
        {
            ++Strikes;
            if (C) C->StrikePlayer(this, HuskDamage);
            Enter(EMemoriaMonsterState::Recover);
        }
        break;
    case EMemoriaMonsterState::Recover:
        if (StateTime >= HuskRecover) Enter(EMemoriaMonsterState::Chase);
        break;
    case EMemoriaMonsterState::Stagger:
        if (StateTime >= HuskStagger) Enter(EMemoriaMonsterState::Chase);
        break;
    default: break;
    }
}
