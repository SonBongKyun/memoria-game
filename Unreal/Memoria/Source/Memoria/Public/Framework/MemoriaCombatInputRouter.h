#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "MemoriaCombatInputRouter.generated.h"
class AMemoriaSliceController;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class UMemoriaFieldCombatSubsystem;
struct FInputActionValue;

// Runtime Enhanced Input objects are local to one controller; no authored input
// asset is replaced. Semantic combat calls never re-enter Controller::InputKey.
UCLASS()
class MEMORIA_API UMemoriaCombatInputRouter : public UActorComponent
{
    GENERATED_BODY()
public:
    UMemoriaCombatInputRouter();
    bool Initialize(AMemoriaSliceController* Controller,UEnhancedInputComponent* Input);
    void Shutdown();
    // Called at the top of the controller's physical InputKey, even for menus.
    // Repeats do not become fresh gestures; only an actual release clears a latch.
    void ObservePhysicalKey(FKey Key,EInputEvent Event);
    void RefreshContext();
    bool ClaimsRawKey(FKey Key) const;
    // Used after the existing higher-priority modal branches, before burn picker.
    // Mapped repeats must be consumed, not fed to PlayerInput's press synthesis.
    bool ShouldFeedEnhanced() const;
    bool HasGamepadAim() const { return bGamepadAim; }
    // Hint preference only; independent of aiming and combat modality.
    bool IsUsingGamepad() const { return bUsingGamepad; }
    FVector AimPoint() const;
    int32 GetMappingCount() const;
    bool IsHoldingAttack() const { return bAttack; }
    bool IsHoldingGuard() const { return bGuard; }
    void CancelHeld();
    static int32 LayeredSlot(int32 Direction,bool SkillLayer,bool ItemLayer);
protected:
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    enum ECommand : int32 { Attack,Guard,Dodge,Burn,Skills,Items,Up,Right,Down,Left,Ink };
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Context;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> Actions;
    TWeakObjectPtr<AMemoriaSliceController> Controller;
    TWeakObjectPtr<UEnhancedInputComponent> Enhanced;
    TArray<uint32> BindingHandles;
    TSet<FKey> PhysicalDown,BlockedUntilRelease;
    FVector2D LastAim=FVector2D::ZeroVector;
    bool bUsingGamepad=false;
    float LastDeviceAxes[6]={0,0,0,0,0,0};
    void TrackGamepadAxes();
    bool bGamepadAim=false,bAttack=false,bGuard=false,bSkills=false,bItems=false,bFieldWasAllowed=false,bContextActive=false;
    bool IsHigherModal() const;
    bool IsFieldAllowed() const;
    bool CanStart(ECommand Command) const;
    UMemoriaFieldCombatSubsystem* Combat() const;
    static FKey KeyFor(ECommand Command);
    void AddButton(UEnhancedInputComponent* Input,const TCHAR* Name,ECommand Command);
    void Started(const FInputActionValue& Value,int32 Command);
    void Finished(const FInputActionValue& Value,int32 Command);
    void Aim(const FInputActionValue& Value);
};
