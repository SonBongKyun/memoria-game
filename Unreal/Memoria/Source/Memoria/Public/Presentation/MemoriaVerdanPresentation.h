#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemoriaVerdanPresentation.generated.h"
class AMemoriaFieldPawn;
class UMemoriaFieldCharacterComponent;
class UMemoriaCombatHudWidget;
class UMemoriaExplorationHudWidget;
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
    UMemoriaExplorationHudWidget* GetExplorationHud() const { return ExplorationHud; }
    // S346: the market ring the source's canvas paints round the square, and its warm lights.
    int32 GetMarketStallCount() const { return MarketStalls; }
    int32 GetMarketLightCount() const { return MarketLights.Num(); }
    int32 GetMarketPropCount() const { return MarketProps; }
    // S348: the market's townsfolk (verdan_market.gd S55), on Codex's S347 models.
    int32 GetTownsfolkCount() const { return Townsfolk.Num(); }
    int32 GetRiggedTownsfolkCount() const;
    UMemoriaFieldCharacterComponent* GetTownsfolk(int32 Index) const { return Townsfolk.IsValidIndex(Index) ? Townsfolk[Index].Get() : nullptr; }
    // S349: they stroll near their places and turn to Arrel when he comes close (the source's S59 add_npc_wander,
    // as the chapter maps' NPCs do since S340), keeping off the stalls, the story's places and the square's edges.
    FVector GetTownsHome(int32 Index) const { return TownMinds.IsValidIndex(Index) ? TownMinds[Index].Home : FVector::ZeroVector; }
    float GetTownsYaw(int32 Index) const { return TownMinds.IsValidIndex(Index) ? TownMinds[Index].Yaw : 0.f; }
    float GetTownsTravel() const { return TownTravel; }
    bool CanTownsfolkStand(const FVector& World) const;
    bool CanTownsfolkTravel(const FVector& From, const FVector& To) const;
    static bool TownPathsStayApart(const FVector& From, const FVector& To, const FVector& OtherFrom, const FVector& OtherTo);
    UStaticMeshComponent* GetTownsShadow(int32 Index) const { return TownShadows.IsValidIndex(Index) ? TownShadows[Index].Get() : nullptr; }
    static constexpr float TownRoam = 120.f;    // how far from its place a townsperson strolls
    static constexpr float TownNotice = 170.f;  // how close Arrel comes before it turns to him
    static constexpr float TownSpeed = 50.f;    // a market amble
    // S350: verdan_market.gd's interactive props (MapEffects.add_interactive_prop): a barrel, a crate, the market
    // sign and a campfire, each acting once (its source flag) as Arrel steps up to it.
    struct FInteractive { const TCHAR* Kind; FVector At; const TCHAR* Flag; };
    static const TArray<FInteractive>& Interactives();
    static constexpr float InteractiveReach = 60.f;
    int32 GetInteractiveUses() const { return InteractiveUses; }
protected:
    virtual void BeginPlay() override;
private:
    UPaperSpriteComponent* Picture(const FString& Name, const FVector& Location, FVector Scale = FVector::OneVector);
    UStaticMeshComponent* SoftQuad(const FVector& Location, const FVector& Scale, FLinearColor Tint, float Alpha);
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> ArrelFigure;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> MaletCard;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaFieldCharacterComponent>> Townsfolk;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> TownShadows;
    struct FTownMind { FVector Home = FVector::ZeroVector, Target = FVector::ZeroVector; float Wait = 0.f, Yaw = -90.f; bool bWalking = false; };
    TArray<FTownMind> TownMinds;
    TArray<FVector> StallSpots, StoryClear;
    FRandomStream TownRng{20261004};
    float TownTravel = 0.f;
    void TickTownsfolk(float DeltaSeconds);
    void BuildInteractives(UMaterialInterface* Timber, UMaterialInterface* Iron, UMaterialInterface* Stone, UMaterialInterface* Glow);
    void TickInteractives();
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> CampfireLight;
    int32 InteractiveUses = 0;
    UPROPERTY(Transient) TObjectPtr<UMemoriaCombatHudWidget> CombatHud;
    UPROPERTY(Transient) TObjectPtr<UMemoriaExplorationHudWidget> ExplorationHud;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PlayerShadow;
    void BuildDepthEnvironment();
    void BuildCourtyard(UMaterialInterface* Iron);
    void UpdateCameraAndVisibility();
    UMaterialInstanceDynamic* Surface(FName Name, FLinearColor Tint, float Mode, float Roughness = 0.82f, float Metallic = 0);
    void Solid(const TCHAR* MeshName, UMaterialInterface* Material, FVector Position, FVector Scale, FRotator Rotation = FRotator::ZeroRotator);
    void Box(UMaterialInterface* Material, FVector Position, FVector Size, FRotator Rotation = FRotator::ZeroRotator);
    void Beam(UMaterialInterface* Material, FVector A, FVector B, float Width);
    void Building(FVector Position, FVector Size, UMaterialInterface* Wall, UMaterialInterface* Timber, UMaterialInterface* Roof, UMaterialInterface* Glow);
    void Stall(FVector Position, UMaterialInterface* Timber, UMaterialInterface* Cloth, UMaterialInterface* Iron, UMaterialInterface* Glow, bool bLantern = true);
    void BuildMarketRing(UMaterialInterface* Timber, UMaterialInterface* Iron, UMaterialInterface* Glow);
    void MarketLight(const FVector& Position, float Intensity, float Radius);
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> MarketLights;
    TArray<float> MarketBase;
    int32 MarketStalls = 0, MarketProps = 0;
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
