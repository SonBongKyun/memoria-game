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

// S320: a content-first chapter field (Chapter 3 on). It builds the level from the map's IR the way the
// Godot map script does: the tile grid as ground, walls and ruins (blocking), the atmosphere as light and
// haze, the quarter-view camera, Arrel and Elia, the combat and exploration HUDs. It then runs the map's
// story: the arrival chain (each link's flag, dialogue group, end-handler flags and toasts), story
// triggers, the exit that closes the chapter, and after it the chests, clues and one-time fights.
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
    bool IsChapterComplete() const { return bComplete; }
    // S331: the props of _setup_map_decorations, the revisit's ambient NPCs and its random encounters.
    int32 GetDecorationCount() const { return DecorationCount; }
    int32 GetVisibleNpcCount() const;
    // S335: how many ambient NPCs wear a rigged model, and how many props are Codex's models.
    int32 GetRiggedNpcCount() const;
    int32 GetModelPropCount() const { return ModelPropCount; }
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
    void UpdateEncounters();
    UMaterialInstanceDynamic* Surface(const FLinearColor& Srgb, float Roughness = .9f);
    UInstancedStaticMeshComponent* Layer(const TCHAR* Mesh, UMaterialInstanceDynamic* Material, bool bCollide);
    void BuildTerrain();
    void BuildLight();
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
