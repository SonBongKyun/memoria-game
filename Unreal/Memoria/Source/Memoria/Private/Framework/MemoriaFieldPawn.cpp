#include "Framework/MemoriaFieldPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "PaperSpriteComponent.h"

AMemoriaFieldPawn::AMemoriaFieldPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(8.0, 8.0, 8.0));
    Collision->SetCollisionProfileName(TEXT("Pawn"));
    Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
    Sprite->SetupAttachment(Collision);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sprite->SetRelativeRotation(FRotator(0.0, 0.0, -90.0));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Collision);
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    Camera->OrthoWidth = 1280.0f;
    Camera->SetRelativeLocation(FVector(0.0, 0.0, 1000.0));
    Camera->SetRelativeRotation(FRotator(-90.0, 0.0, 0.0));
    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision);
    Movement->SetPlaneConstraintNormal(FVector::UpVector);
    Movement->SetPlaneConstraintEnabled(true);
}
UPawnMovementComponent* AMemoriaFieldPawn::GetMovementComponent() const { return Movement; }
