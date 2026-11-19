#pragma once
#include "CoreTypes.h"
#include "HAL/PlatformTime.h"
#include "Templates/Atomic.h"

// Plain process counters: safe inside animation workers. No UObject, world, allocation or lock in a scope.
namespace MemoriaPresentationMetrics
{
    enum class EScope : uint8 { AnimationEvaluate, CharacterUpdate };
    struct FCounterSnapshot { uint64 Cycles = 0, Calls = 0; };
    struct FSnapshot { FCounterSnapshot AnimationEvaluate, CharacterUpdate; };
    extern MEMORIA_API TAtomic<bool> GEnabled;
    FORCEINLINE bool IsEnabled()
    {
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        return GEnabled.Load(EMemoryOrder::Relaxed);
#else
        return false;
#endif
    }
    // Does not reset cumulative counters. Production calls never enable these outside verified smoke.
    MEMORIA_API void SetEnabled(bool bEnabled);
    MEMORIA_API FSnapshot Capture();
    MEMORIA_API FSnapshot Delta(const FSnapshot& Newer, const FSnapshot& Older);
    MEMORIA_API void RecordCompleted(EScope Scope, uint64 ElapsedCycles);
    class FScopedCpu final
    {
    public:
        FORCEINLINE explicit FScopedCpu(EScope InScope) : Scope(InScope), bMeasure(IsEnabled())
        { if (bMeasure) Started = FPlatformTime::Cycles64(); }
        FORCEINLINE ~FScopedCpu()
        {
            if (bMeasure) RecordCompleted(Scope, FPlatformTime::Cycles64() - Started);
        }
        FScopedCpu(const FScopedCpu&) = delete;
        FScopedCpu& operator=(const FScopedCpu&) = delete;
    private:
        EScope Scope;
        bool bMeasure = false;
        uint64 Started = 0;
    };
}
