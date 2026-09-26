#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MemoriaFieldCharacterComponent.generated.h"
class UPaperSprite;
class UPaperSpriteComponent;

// One field character drawn as a camera-facing card, the same way for Arrel, Elia and Malet.
// Uses the high-resolution illustrated set (FieldHD, FIELD_SPRITE_ART_SPEC.md) when it has
// been imported, and otherwise the source pixel field sprites, rendered with point sampling.
UCLASS()
class MEMORIA_API UMemoriaFieldCharacterComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UMemoriaFieldCharacterComponent();
    // Resolves the art for a source character id (arrel, elia, malet...). WorldHeight is the
    // standing figure's height in world units; the card is scaled to it.
    bool InitializeCharacter(const FString& Id, float WorldHeight);
    // Facing follows real travel; gait advances with distance, not time.
    void AdvanceLocomotion(const FVector& Step, float DeltaSeconds);
    void Face(const FString& Direction);
    const FString& GetFacing() const { return Facing; }
    float GaitPhase() const { return Phase; }
    float LocomotionWeight() const { return Weight; }
    FVector FocusPosition() const;
    float GetWorldHeight() const { return Height; }
    bool IsHighResolution() const { return bHighResolution; }
    FString GetFrameName() const;
    UPaperSpriteComponent* GetCard() const { return Card; }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    // Which art a character id resolves to: "hd", "pixel" or "missing".
    static FString DescribeArt(const FString& Id);
    // Distance for one full two-step gait cycle.
    static constexpr float StrideLength = 58.f;
private:
    UPROPERTY(Transient) TObjectPtr<UPaperSpriteComponent> Card;
    // Down, Up, Left, Right; a missing Left mirrors Right.
    UPROPERTY(Transient) TObjectPtr<UPaperSprite> Stand[4];
    UPROPERTY(Transient) TArray<TObjectPtr<UPaperSprite>> Walk;
    int32 WalkFrames = 0;
    bool bMirrorLeft = false, bHighResolution = false;
    FString Facing = TEXT("Down");
    float Phase = 0.f, Weight = 0.f, Age = 0.f, Height = 140.f, Scale = 1.f;
    static int32 DirectionIndex(const FString& Direction);
    void ApplyFrame();
};
