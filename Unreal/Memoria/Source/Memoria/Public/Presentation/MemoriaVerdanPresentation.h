#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemoriaVerdanPresentation.generated.h"
class AMemoriaFieldPawn;
class AMemoriaMaletActor;
class UPaperSprite;
class UPaperSpriteComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Visual-only layer for the bounded Verdan slice. Does not own collision, input or story state.
UCLASS()
class MEMORIA_API AMemoriaVerdanPresentation : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaVerdanPresentation();
    virtual void Tick(float DeltaSeconds) override;
    FString Facing() const { return Direction; }
    bool IsWalking() const { return bWalking; }
protected:
    virtual void BeginPlay() override;
private:
    UPaperSpriteComponent* Picture(const FString& Name, const FVector& Location, FVector Scale = FVector::OneVector);
    UStaticMeshComponent* SoftQuad(const FVector& Location, const FVector& Scale, FLinearColor Tint, float Alpha);
    UPROPERTY(Transient) TArray<TObjectPtr<UPaperSprite>> PlayerArt;
    UPROPERTY(Transient) TObjectPtr<UPaperSpriteComponent> MaletArt;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PlayerShadow;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> LampLights;
    TWeakObjectPtr<AMemoriaFieldPawn> Player;
    TWeakObjectPtr<AMemoriaMaletActor> Malet;
    FVector PreviousPosition = FVector::ZeroVector;
    FString Direction = TEXT("Down");
    float GaitTime = 0;
    float LightTime = 0;
    bool bWalking = false;
};
