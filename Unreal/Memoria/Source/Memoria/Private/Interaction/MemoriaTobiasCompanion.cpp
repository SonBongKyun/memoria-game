#include "Interaction/MemoriaTobiasCompanion.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AMemoriaTobiasCompanion::AMemoriaTobiasCompanion()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickGroup = TG_PostPhysics;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("CompanionBody")); SetRootComponent(Body);
    Body->SetSphereRadius(20); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionObjectType(ECC_WorldDynamic); Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); Body->SetGenerateOverlapEvents(false);
    Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CompanionSprite")); Sprite->SetupAttachment(Body);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sprite->SetGenerateOverlapEvents(false);
    Sprite->SetRelativeRotation(FRotator(0, 0, 42)); Sprite->SetRelativeLocation(FVector(0, 0, -8));
    Shadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CompanionShadow")); Shadow->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Shadow->SetStaticMesh(Plane.Object); Shadow->SetCollisionEnabled(ECollisionEnabled::NoCollision); Shadow->SetCastShadow(false);
    Shadow->SetRelativeLocation(FVector(0, 0, -8.8)); Shadow->SetRelativeScale3D(FVector(.62, .26, 1));
}
AMemoriaTobiasCompanion* AMemoriaTobiasCompanion::Join(UWorld* World, APawn* Leader)
{
    if (!World || !Leader) return nullptr;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Elia stands at (-30, -20). This is further down the same side, clear of her on the first frame.
    auto* Tobias = World->SpawnActor<AMemoriaTobiasCompanion>(Leader->GetActorLocation() + FVector(-36, -210, 0), FRotator::ZeroRotator, Params);
    if (Tobias) Tobias->Follow(Leader);
    return Tobias;
}
void AMemoriaTobiasCompanion::Follow(APawn* InTarget)
{
    Target = InTarget; Trail.Reset();
    if (InTarget) { Trail.Add(GetActorLocation()); Trail.Add(InTarget->GetActorLocation()); LastTargetPosition = InTarget->GetActorLocation(); }
    if (!Figure)
    {
        Figure = NewObject<UMemoriaFieldCharacterComponent>(this); AddInstanceComponent(Figure);
        Figure->SetupAttachment(Body); Figure->RegisterComponent();
    }
    const float Height = UMemoriaFieldCharacterComponent::AmbientHeight(TEXT("bureauagent"), AMemoriaVerdanPresentation::ArrelHeight);
    if (Figure->InitializeCharacter(TEXT("tobias"), Height)) Sprite->SetHiddenInGame(true);
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight")))
        if (auto* Instance = UMaterialInstanceDynamic::Create(Material, this))
        { Instance->SetVectorParameterValue(TEXT("Tint"), FLinearColor::Black); Instance->SetScalarParameterValue(TEXT("Alpha"), .6f); Shadow->SetMaterial(0, Instance); }
    Face(FVector(0, -1, 0));
}
void AMemoriaTobiasCompanion::Face(const FVector& Move)
{
    const FString Direction = FMath::Abs(Move.X) >= FMath::Abs(Move.Y) ? (Move.X > 0 ? TEXT("Right") : TEXT("Left")) : (Move.Y > 0 ? TEXT("Up") : TEXT("Down"));
    if (Figure) Figure->Face(Direction);
}
FVector AMemoriaTobiasCompanion::FollowPoint() const
{
    if (!Target.IsValid()) return GetActorLocation();
    if (Trail.Num() < 2) return Target->GetActorLocation();
    double Accumulated = 0;
    for (int32 Index = Trail.Num() - 1; Index > 0; --Index)
    {
        const double Segment = FVector::Dist2D(Trail[Index], Trail[Index - 1]);
        if (Segment <= .001) continue;
        if (Accumulated + Segment >= FormationDistance) return FMath::Lerp(Trail[Index], Trail[Index - 1], (FormationDistance - Accumulated) / Segment);
        Accumulated += Segment;
    }
    return Trail[0];
}
void AMemoriaTobiasCompanion::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age += DeltaSeconds;
    if (!Target.IsValid() || DeltaSeconds <= 0) return;
    const FVector TargetPosition = Target->GetActorLocation();
    if (Trail.IsEmpty() || FVector::Dist2D(Trail.Last(), TargetPosition) >= TrailSample)
    { Trail.Add(TargetPosition); if (Trail.Num() > MaxTrailPoints) Trail.RemoveAt(0, Trail.Num() - MaxTrailPoints); }
    FVector Point = FollowPoint();
    AMemoriaEliaCompanion* Elia = nullptr;
    for (TActorIterator<AMemoriaEliaCompanion> It(GetWorld()); It; ++It) { Elia = *It; break; }
    if (Elia)
    {
        const FVector EliaAt = Elia->GetActorLocation();
        const FVector Back = FVector(EliaAt.X - TargetPosition.X, EliaAt.Y - TargetPosition.Y, 0).GetSafeNormal();
        if (!Back.IsNearlyZero() && FVector::Dist2D(Point, EliaAt) < ClearGap) Point = EliaAt + Back * ClearGap;
    }
    const FVector ToPoint = FVector(Point.X - GetActorLocation().X, Point.Y - GetActorLocation().Y, 0);
    const double Distance = ToPoint.Size();
    if (Distance > WarpDistance)
    {
        SetActorLocation(FVector(Point.X, Point.Y, GetActorLocation().Z)); Velocity = FVector::ZeroVector;
        Trail.Reset(); Trail.Add(GetActorLocation()); Trail.Add(TargetPosition); return;
    }
    const double TargetSpeed = FVector::Dist2D(TargetPosition, LastTargetPosition) / DeltaSeconds;
    const double Speed = FollowSpeed * (TargetSpeed > FollowSpeed * 1.05 ? SprintCatchup : 1.0);
    const FVector Desired = Distance > ArrivalRadius ? ToPoint.GetSafeNormal() * Speed * FMath::Clamp((Distance - ArrivalRadius) / 72.0, 0.0, 1.0) : FVector::ZeroVector;
    Velocity = FMath::VInterpConstantTo(Velocity, Desired, DeltaSeconds, FollowAccel);
    const FVector Step = Velocity * DeltaSeconds;
    if (Step.SizeSquared2D() > .0001) { SetActorLocation(GetActorLocation() + Step); if (Velocity.Size2D() > 12) Face(Velocity); }
    if (Figure) Figure->AdvanceLocomotion(Velocity.Size2D() > 12 ? Step : FVector::ZeroVector, DeltaSeconds);
    LastTargetPosition = TargetPosition;
    Sprite->SetRelativeLocation(FVector(0, 0, -8 + .6 * FMath::Sin(Age * 1.8)));
}
