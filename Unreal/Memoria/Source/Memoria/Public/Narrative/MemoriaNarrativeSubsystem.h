#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Narrative/MemoriaNarrativeRuntime.h"
#include "MemoriaNarrativeSubsystem.generated.h"

class UMemoriaRunSubsystem;
class UMemoriaRunSaveGame;
enum class EMemoriaSliceState : uint8 { Idle, VN, Travelling, Field, Exploration, Failed };
struct FMemoriaPresentedChoice { int32 OriginalIndex; FString Text; };
// Presentation receives values only. No conditions, effect data or mutable run.
struct MEMORIA_API FMemoriaNarrativeView
{
    FString Header, Speaker, Narration, Body;
    TArray<FMemoriaPresentedChoice> Choices;
    bool bPaused = false;
    bool bCompactStatus = false;
};

UCLASS()
class MEMORIA_API UMemoriaNarrativeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool StartDevelopmentVN();
    bool StartUnseenFieldFixture();
    bool EnterVerdan();
    void Confirm(int32 OriginalChoice = INDEX_NONE);
    void Back();
    FMemoriaNarrativeView GetView() const;
    EMemoriaSliceState GetState() const { return State; }
    bool IsPaused() const { return bPaused; }
    int32 GetRevision() const { return Revision; }
    int32 GetFieldInvocationCount() const { return FieldInvocationCount; }
    const TArray<FString>& GetTrace() const { return Trace; }
    FMemoriaVNContinuation GetContinuation() const;
    UMemoriaRunSaveGame* CaptureSave() const;
    bool PrepareRestore(const UMemoriaRunSaveGame& Save);
    bool ResumePrepared();
    void Record(const FString& Event);
    static constexpr const TCHAR* VerdanMap = TEXT("/Game/Tests/Campaign/L_VerdanHost");
private:
    UPROPERTY(Transient) TObjectPtr<UMemoriaRunSubsystem> Run;
    UPROPERTY(Transient) TObjectPtr<UMemoriaVNAsset> VNAsset;
    UPROPERTY(Transient) TObjectPtr<UMemoriaFieldAsset> FieldAsset;
    TUniquePtr<FMemoriaNarrativeContext> Context;
    TUniquePtr<FMemoriaVNInterpreter> VN;
    TUniquePtr<FMemoriaFieldInterpreter> Field;
    EMemoriaSliceState State = EMemoriaSliceState::Idle;
    bool bPaused = false;
    int32 Revision = 0, EventCursor = 0, FieldInvocationCount = 0;
    TArray<FString> Trace;
    void Reset();
    bool LoadContracts();
    void FlushEvents(const FString& Dialect);
    void AfterVN();
    void Explore();
};
