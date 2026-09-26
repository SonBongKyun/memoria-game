#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MemoriaInteractable.h"
#include "MemoriaStoryPointActor.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// A Verdan story beat the player can approach. Query-only: it never blocks movement.
// The narrative subsystem owns flags and the Field; this actor only presents and forwards.
UCLASS()
class MEMORIA_API AMemoriaStoryPointActor : public AActor, public IMemoriaInteractable
{
    GENERATED_BODY()
public:
    AMemoriaStoryPointActor();
    void Configure(const FString& InGroup, const FString& InPrompt);
    const FString& GetGroup() const { return Group; }
    virtual bool CanInteract(const APawn& Pawn) const override;
    virtual FString InteractionPrompt() const override;
    virtual bool Interact(APawn& Pawn) override;
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glow;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
    FString Group, Prompt;
    float Age = 0;
};
