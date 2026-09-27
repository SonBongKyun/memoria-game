#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MemoriaFieldCharacterComponent.generated.h"
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UAnimSequence;
class USkeletalMesh;

// Rigged field figures share distance-driven locomotion. Illustrated/pixel cards remain a fallback.
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
    // Walk frames per view: 4 for the pixel cycle, 2 for illustrated contacts, 0 when walking is bob and sway only.
    int32 GetWalkFrameCount() const { return WalkFrames; }
    FString GetFrameName() const;
    UPaperSpriteComponent* GetCard() const { return Card; }
    USkeletalMeshComponent* GetSkeletalMesh() const { return Skeletal; }
    bool IsRigged() const { return Skeletal != nullptr; }
    int32 GetPropCount() const { return Props.Num(); }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    // Which art a character id resolves to: "rigged", "hd", "pixel" or "missing".
    static FString DescribeArt(const FString& Id);
    // S311 combat: the UE mannequin as the monster stand-in, one-shot action clips, and a free aim yaw.
    bool InitializeMannequin(float WorldHeight, const FLinearColor& Tint);
    bool PlayAction(const TCHAR* Clip, float Rate = 1.f, bool bHold = false);
    void StopAction();
    bool IsActing() const { return ActionClip != nullptr; }
    const UAnimSequence* GetActionClip() const { return ActionClip; }
    float GetActionTime() const { return ActionTime; }
    float GetActionLength() const;
    void SetAim(float Yaw) { bAim = true; AimYaw = Yaw; ApplyFrame(); }
    void ClearAim() { bAim = false; ApplyFrame(); }
    float GetYaw() const;
    const FString& GetCharacterId() const { return CharacterId; }
    // Distance for one full two-step gait cycle.
    static constexpr float StrideLength = 58.f;
private:
    UPROPERTY(Transient) TObjectPtr<UPaperSpriteComponent> Card;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Skeletal;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Props;
    bool InitializeRigged(const FString& Id);
    bool CreateSkeletal(USkeletalMesh* Mesh, UAnimSequence* Idle, UAnimSequence* WalkClip);
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActionClip;
    FString CharacterId;
    float ActionTime = 0.f, ActionRate = 1.f, ActionWeight = 0.f, AimYaw = 0.f, MeshYawOffset = 0.f;
    bool bActionHold = false, bAim = false;
    void AttachProp(const FString& Id, const FString& Prop, FName Bone, const FVector& Offset);
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
