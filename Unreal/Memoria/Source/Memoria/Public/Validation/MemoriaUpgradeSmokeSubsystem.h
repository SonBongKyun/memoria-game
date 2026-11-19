#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MemoriaUpgradeSmokeSubsystem.generated.h"

namespace MemoriaUpgradeSmoke
{
    // Root's GameInstance::Init must call this BEFORE Super::Init. False means explicit smoke startup
    // was refused and the process has requested an error exit; do not initialize game subsystems.
    // Normal/Shipping startup returns true and changes nothing.
    MEMORIA_API bool BeforeGameInstanceInit();
    // Armed only by the verified cooked Development bootstrap before GI initialization. Remains true
    // through process teardown; never sets the engine automation-test global or starts a framework test.
    MEMORIA_API bool IsIsolatedSmokeActive();
}
struct FMemoriaUpgradeSmokeState;
// Generated UObject VTable constructors also instantiate member destruction. Keep deletion itself
// out of every generated/header translation unit, where the smoke state is intentionally incomplete.
struct MEMORIA_API FMemoriaUpgradeSmokeStateDeleter
{
    void operator()(FMemoriaUpgradeSmokeState* Pointer) const;
};

// Cooked Development validation only. There is no instance/tick on a normal or Shipping game run.
// Opt-in runtime counters include bounded CPU scopes; no GPU execution timing is inferred.
UCLASS()
class MEMORIA_API UMemoriaUpgradeSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UMemoriaUpgradeSmokeSubsystem();
    virtual ~UMemoriaUpgradeSmokeSubsystem() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual UWorld* GetTickableGameObjectWorld() const override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMemoriaUpgradeSmokeSubsystem, STATGROUP_Tickables); }
private:
    TUniquePtr<FMemoriaUpgradeSmokeState, FMemoriaUpgradeSmokeStateDeleter> State;
};
