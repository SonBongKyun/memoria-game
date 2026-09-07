#include "Interaction/MemoriaMaletActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AMemoriaMaletActor::AMemoriaMaletActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBody")); SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(18, 24, 16)); Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic); Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder")); Placeholder->SetupAttachment(Collision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Placeholder->SetStaticMesh(Cube.Object); Placeholder->SetRelativeScale3D(FVector(.36, .48, .24));
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DevelopmentLabel")); Label->SetupAttachment(Collision);
    Label->SetText(FText::FromString(TEXT("MALET\nplaceholder"))); Label->SetWorldSize(20);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center); Label->SetTextRenderColor(FColor(235, 194, 106));
    Label->SetRelativeLocation(FVector(0, 65, 18));
    Label->SetRelativeRotation(FRotationMatrix::MakeFromXY(-FVector::UpVector, -FVector::ForwardVector).Rotator());
}
bool AMemoriaMaletActor::CanInteract(const APawn& Pawn) const
{
    return Pawn.GetWorld() == GetWorld() &&
        FVector::DistSquared(Pawn.GetActorLocation(), GetActorLocation()) <= InteractionRange * InteractionRange;
}
FString AMemoriaMaletActor::InteractionPrompt() const { return TEXT("Malet  |  E / A: talk"); }
bool AMemoriaMaletActor::Interact(APawn& Pawn)
{
    if (!CanInteract(Pawn)) return false;
    auto* Host = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    return Host && Host->InteractWithMalet();
}
