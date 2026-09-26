#include "Interaction/MemoriaStoryPointActor.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Narrative/MemoriaVerdanStory.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AMemoriaStoryPointActor::AMemoriaStoryPointActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<USphereComponent>(TEXT("StoryBody")); SetRootComponent(Body);
    Body->SetSphereRadius(24); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionObjectType(ECC_WorldDynamic); Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); Body->SetGenerateOverlapEvents(false);
    Glow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoryGlow")); Glow->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Glow->SetStaticMesh(Plane.Object); Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision); Glow->SetCastShadow(false);
    Glow->SetRelativeLocation(FVector(0, 0, -8.4)); Glow->SetRelativeScale3D(FVector(1.3, 1.3, 1));
}
void AMemoriaStoryPointActor::Configure(const FString& InGroup, const FString& InPrompt)
{
    Group = InGroup; Prompt = InPrompt;
    // A warm memory glow on the paving; brighter when the beat is still unheard.
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Memoria/Presentation/Verdan/M_SoftLight.M_SoftLight")))
    {
        GlowMaterial = UMaterialInstanceDynamic::Create(Material, this);
        GlowMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.95f, .66f, .30f));
        GlowMaterial->SetScalarParameterValue(TEXT("Alpha"), .35f); Glow->SetMaterial(0, GlowMaterial);
    }
}
bool AMemoriaStoryPointActor::CanInteract(const APawn& Pawn) const
{
    const auto* Host = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
    return Host && Pawn.GetWorld() == GetWorld() && Host->IsStoryBeatAvailable(Group) &&
        FVector::DistSquared2D(Pawn.GetActorLocation(), GetActorLocation()) <= FMath::Square(MemoriaVerdanStory::InteractionRange);
}
FString AMemoriaStoryPointActor::InteractionPrompt() const { return Prompt; }
bool AMemoriaStoryPointActor::Interact(APawn& Pawn)
{
    if (!CanInteract(Pawn)) return false;
    auto* Host = GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
    return Host && Host->StartStoryBeat(Group);
}
void AMemoriaStoryPointActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age += DeltaSeconds;
    const auto* Host = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
    const bool Available = Host && Host->IsStoryBeatAvailable(Group);
    Glow->SetVisibility(Available);
    if (GlowMaterial && Available) GlowMaterial->SetScalarParameterValue(TEXT("Alpha"), .28f + .12f * FMath::Sin(Age * 2.1f));
}
