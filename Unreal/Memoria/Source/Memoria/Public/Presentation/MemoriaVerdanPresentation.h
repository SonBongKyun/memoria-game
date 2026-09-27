#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemoriaVerdanPresentation.generated.h"
class AMemoriaFieldPawn;
class UMemoriaFieldCharacterComponent;
class UMemoriaCombatHudWidget;
class AMemoriaMaletActor;
class AStaticMeshActor;
class UPaperSprite;
class UPaperSpriteComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UPointLightComponent;

// Visual-only layer for the bounded Verdan slice. Does not own collision, input or story state.
UCLASS()
class MEMORIA_API AMemoriaVerdanPresentation : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaVerdanPresentation();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString Facing() const { return Direction; }
    bool IsWalking() const { return bWalking; }
    // Arrel's field figure (the S306 card; the 3D prototype stays out of the field).
    UMemoriaFieldCharacterComponent* CharacterFigure() const { return ArrelFigure; }
    UMemoriaFieldCharacterComponent* MaletFigure() const { return MaletCard; }
    // Standing heights in world units (FIELD_SPRITE_ART_SPEC.md proportions).
    static constexpr float ArrelHeight = 150.f;
    // Level geometry the 3D stage replaces visually: tagged actors, or engine basic shapes.
    static constexpr const TCHAR* PlaceholderTag = TEXT("MemoriaPlaceholder");
    static bool IsPlaceholderGeometry(const AStaticMeshActor& Actor);
protected:
    virtual void BeginPlay() override;
private:
    UPaperSpriteComponent* Picture(const FString& Name, const FVector& Location, FVector Scale = FVector::OneVector);
    UStaticMeshComponent* SoftQuad(const FVector& Location, const FVector& Scale, FLinearColor Tint, float Alpha);
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> ArrelFigure;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> MaletCard;
    UPROPERTY(Transient) TObjectPtr<UMemoriaCombatHudWidget> CombatHud;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PlayerShadow;
    void BuildDepthEnvironment();
    void BuildCourtyard(UMaterialInterface* Iron);
    void UpdateCameraAndVisibility();
    UMaterialInstanceDynamic* Surface(FName Name, FLinearColor Tint, float Mode, float Roughness = 0.82f, float Metallic = 0);
    void Solid(const TCHAR* MeshName, UMaterialInterface* Material, FVector Position, FVector Scale, FRotator Rotation = FRotator::ZeroRotator);
    void Box(UMaterialInterface* Material, FVector Position, FVector Size, FRotator Rotation = FRotator::ZeroRotator);
    void Beam(UMaterialInterface* Material, FVector A, FVector B, float Width);
    void Building(FVector Position, FVector Size, UMaterialInterface* Wall, UMaterialInterface* Timber, UMaterialInterface* Roof, UMaterialInterface* Glow);
    void Stall(FVector Position, UMaterialInterface* Timber, UMaterialInterface* Cloth, UMaterialInterface* Iron, UMaterialInterface* Glow);
    void Lantern(FVector Position, UMaterialInterface* Iron, UMaterialInterface* Glow, bool bShadow);
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> MeshBatches;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> SurfaceMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> LampLights;
    // Field life: presentation-only ash, lantern embers and ground mist. No collision, no gameplay state.
    // Plain components: M_SoftLight has no instanced-mesh usage flag, so ISM would fall back to the default material.
    void MoteGroup(TArray<TObjectPtr<UStaticMeshComponent>>& Out, int32 Count, FLinearColor Tint, float Alpha);
    void BuildFieldLife();
    void TickFieldLife();
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> AshMotes;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> EmberMotes;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> MistPatches;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> MistMaterials;
    TWeakObjectPtr<AMemoriaFieldPawn> Player;
    TWeakObjectPtr<AMemoriaMaletActor> Malet;
    FVector PreviousPosition = FVector::ZeroVector;
    FString Direction = TEXT("Down");
    float LightTime = 0;
    bool bWalking = false;
};
