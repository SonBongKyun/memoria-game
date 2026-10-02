#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Battle/MemoriaEncounterModel.h"
#include "MemoriaChapterPresentation.generated.h"
class AMemoriaFieldPawn;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMemoriaFieldCharacterComponent;
class UMemoriaCombatHudWidget;
class UMemoriaExplorationHudWidget;
class UMemoriaChapterCardWidget;
class UStaticMeshComponent;
class UPointLightComponent;
class UTexture2D;

// S320: a content-first chapter field (Chapter 3 on). It builds the level from the map's IR the way the
// Godot map script does: the tile grid as ground, walls and ruins (blocking), the atmosphere as light and
// haze, the quarter-view camera, Arrel and Elia, the combat and exploration HUDs. It then runs the map's
// story: the arrival chain (each link's flag, dialogue group, end-handler flags and toasts), story
// triggers, the exit that closes the chapter, and after it the chests, clues and one-time fights.
// S338: the level is dressed past its tiles (MemoriaChapterEnvironment.cpp): painted ground, masonry, a border
// that reads as a place, lamps, dust or rain.
UCLASS()
class MEMORIA_API AMemoriaChapterPresentation : public AActor
{
    GENERATED_BODY()
public:
    AMemoriaChapterPresentation();
    void Configure(const FString& InMap) { Map = InMap; }
    virtual void Tick(float DeltaSeconds) override;
    const FString& GetMap() const { return Map; }
    const FMemoriaChapterMapSpec* GetSpec() const { return Spec; }
    UMemoriaFieldCharacterComponent* GetArrelFigure() const { return ArrelFigure; }
    UMemoriaChapterCardWidget* GetCard() const { return Card; }
    UMemoriaExplorationHudWidget* GetExplorationHud() const { return ExplorationHud; }
    UMemoriaCombatHudWidget* GetCombatHud() const { return CombatHud; }
    bool IsChapterComplete() const { return bComplete; }
    // S331: the props of _setup_map_decorations, the revisit's ambient NPCs and its random encounters.
    int32 GetDecorationCount() const { return DecorationCount; }
    int32 GetVisibleNpcCount() const;
    // S335: how many ambient NPCs wear a rigged model, and how many props are Codex's models.
    int32 GetRiggedNpcCount() const;
    int32 GetModelPropCount() const { return ModelPropCount; }
    // S340: the ambient NPCs live a little. Each idles, strolls to a spot near where the source stands it, and
    // idles again; it stops and turns to Arrel when he comes close, and to the foes while a fight is on.
    UMemoriaFieldCharacterComponent* GetAmbientNpc(int32 Index) const { return AmbientNpcs.IsValidIndex(Index) ? AmbientNpcs[Index].Get() : nullptr; }
    FVector GetAmbientHome(int32 Index) const { return NpcMinds.IsValidIndex(Index) ? NpcMinds[Index].Home : FVector::ZeroVector; }
    float GetAmbientYaw(int32 Index) const { return NpcMinds.IsValidIndex(Index) ? NpcMinds[Index].Yaw : 0.f; }
    float GetNpcTravel() const { return NpcTravel; }
    static constexpr float NpcRoam = 150.f;     // how far from its place an NPC strolls
    static constexpr float NpcNotice = 170.f;   // how close Arrel comes before it turns to him
    static constexpr float NpcSpeed = 55.f;     // an unhurried walk
    // S337: the dressed map. Whether the ground wears the painted material, and how many lamps burn.
    bool IsGroundPainted() const { return GroundMaterial != nullptr; }
    int32 GetLampCount() const { return Lamps.Num(); }
    int32 GetMoteCount() const { return Motes.Num(); }
    // S344: Codex's environment kit stands in the map. How many of its models stand, and how many kinds.
    int32 GetKitPropCount() const { return KitPropCount; }
    int32 GetKitKindCount() const { return KitKinds.Num(); }
    const FMemoriaEncounterModel& GetEncounterModel() const { return Encounter; }
    bool AreEncountersOpen() const;
    int32 GetBlockerCount() const;
    // Source-pixel position of the player (the map IR's frame).
    FVector2D PlayerSource() const;
    static constexpr float ArrelHeight = 150.f;
    static constexpr float WalkSpeed = 150.f;      // the source's 120 px/s at the map's 3x scale, within Elia's catch-up
    static constexpr float StepDelay = 1.f;         // belt_waystation.gd waits about a second between chained groups
    static constexpr float TravelDelay = 4.f;       // the completion card holds before the road moves on
    static constexpr float SceneDelay = 6.f;        // the completion card before a story scene (Chapter 5)
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    FString Map;
    const FMemoriaChapterMapSpec* Spec = nullptr;
    TWeakObjectPtr<AMemoriaFieldPawn> Player;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldCharacterComponent> ArrelFigure;
    UPROPERTY(Transient) TObjectPtr<UMemoriaCombatHudWidget> CombatHud;
    UPROPERTY(Transient) TObjectPtr<UMemoriaExplorationHudWidget> ExplorationHud;
    UPROPERTY(Transient) TObjectPtr<UMemoriaChapterCardWidget> Card;
    UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Blockers;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UStaticMeshComponent>> Markers;
    FDelegateHandle FinishedHandle;
    FVector PreviousPosition = FVector::ZeroVector;
    float StepAt = -1.f, TravelAt = -1.f, SceneAt = -1.f, Clock = 0.f;
    bool bComplete = false, bDeparting = false, bInExit = false;
    UPROPERTY(Transient) TArray<TObjectPtr<UMemoriaFieldCharacterComponent>> AmbientNpcs;
    int32 DecorationCount = 0, ModelPropCount = 0;
    FMemoriaEncounterModel Encounter;
    FMemoriaEncounterRng EncounterRng = FMemoriaEncounterRng::Random();
    bool bEncounterReady = false;
    void BuildDecorations();
    void BuildAmbientNpcs();
    struct FAmbientMind { FVector Home = FVector::ZeroVector, Target = FVector::ZeroVector; float Wait = 0.f, Yaw = -90.f; bool bWalking = false; };
    TArray<FAmbientMind> NpcMinds;
    FRandomStream NpcRng{20261002};
    float NpcTravel = 0.f;
    void TickAmbientNpcs(float DeltaSeconds);
    bool CanStand(const FVector& World) const;
    void UpdateEncounters();
    UMaterialInstanceDynamic* Surface(const FLinearColor& Srgb, float Roughness = .9f);
    UInstancedStaticMeshComponent* Layer(const TCHAR* Mesh, UMaterialInstanceDynamic* Material, bool bCollide);
    void BuildTerrain();
    void BuildLight();
    // S337 (MemoriaChapterEnvironment.cpp): the map dressed past its tiles. The ground is one painted surface
    // under a tile mask; the walls are masonry with broken tops; the border, the lamps, the props of the
    // source's map canvas, the air (dust or rain) and the world beyond the map follow.
    struct FDressing;
    static const FDressing& DressingFor(const FString& Map);
    void BuildGround(const FDressing& Look);
    void BuildWalls(const FDressing& Look);
    void BuildBorder(const FDressing& Look);
    void BuildSetPieces(const FDressing& Look);
    void BuildAir(const FDressing& Look);
    void TickEnvironment(float DeltaSeconds);
    UMaterialInstanceDynamic* Focus(const TCHAR* Name, const FLinearColor& Tint, float Mode, float Roughness = .82f, float Metallic = 0.f);
    void Solid(const TCHAR* Mesh, UMaterialInterface* Material, const FVector& Position, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator, bool bShadow = true);
    void Box(UMaterialInterface* Material, const FVector& Position, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator, bool bShadow = true);
    void Beam(UMaterialInterface* Material, const FVector& A, const FVector& B, float Width);
    void Lamp(const FVector& Position, float Radius, float Intensity, bool bPane = true);
    // S344: Codex's S343 environment kit (-run=MemoriaEnvironmentAssets). Kit is one of its atlases as a material,
    // Prop stands a model on the ground, Fence runs the chain fence from post to post. Claimed are the solid tiles
    // a set piece stands on, which the ruins leave to it.
    UMaterialInstanceDynamic* Kit(const TCHAR* Atlas, const FLinearColor& Tint = FLinearColor::White, const TCHAR* Glow = nullptr);
    void Prop(const TCHAR* Name, UMaterialInterface* Material, const FVector& Ground, float Yaw = 0.f, const FVector& Scale = FVector::OneVector, bool bShadow = true);
    void Fence(UMaterialInterface* Material, const FVector& A, const FVector& B);
    bool Claimed(int32 X, int32 Y) const;
    void BuildGrass(const FDressing& Look);
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Kits;
    TSet<FString> KitKinds;
    int32 KitPropCount = 0;
    FVector TileCentre(float X, float Y) const;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> GroundMask;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GroundMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> FocusMaterials;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lamps;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Motes;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> GlowMaterial;
    TArray<float> LampBase;
    double EnvTime = 0.;
    bool bRain = false;
    void BuildMarkers();
    void StartNextStep();
    // S330: the memories the chapter brings (add_chapter_memories), and their toasts held for the arrival chain's end.
    void GrantChapterMemories(int32 Chapter);
    TArray<FString> MemoryToasts;
    void OnFieldFinished(const FString& Group);
    void CheckTriggers();
    bool Flag(const FString& Id) const;
    bool GateOpen(const FString& Gate) const;
    bool SceneReady() const;
    bool RoadOpen() const;
    FString StoryMovedOn() const;
    void SetFlag(const FString& Id);
    FString Localized(const FString& Text) const;
};
