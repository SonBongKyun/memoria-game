#include "Presentation/MemoriaPresentationMetrics.h"
#include <atomic>
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Async/Async.h"
#endif

namespace MemoriaPresentationMetrics
{
TAtomic<bool> GEnabled{false};
namespace
{
struct FCounter
{
    std::atomic<uint64> Cycles{0}, Calls{0};
    void Add(uint64 Elapsed)
    {
        Cycles.fetch_add(Elapsed, std::memory_order_relaxed);
        Calls.fetch_add(1, std::memory_order_relaxed);
    }
    FCounterSnapshot Read() const
    {
        FCounterSnapshot Out;
        Out.Cycles = Cycles.load(std::memory_order_relaxed); Out.Calls = Calls.load(std::memory_order_relaxed);
        return Out;
    }
};
FCounter Animation, Component;
FCounterSnapshot Difference(const FCounterSnapshot& Newer, const FCounterSnapshot& Older)
{
    FCounterSnapshot Out;
    Out.Cycles = Newer.Cycles >= Older.Cycles ? Newer.Cycles - Older.Cycles : 0;
    Out.Calls = Newer.Calls >= Older.Calls ? Newer.Calls - Older.Calls : 0;
    return Out;
}
}
void SetEnabled(bool bEnabled)
{
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    GEnabled.Store(bEnabled, EMemoryOrder::Relaxed);
#else
    GEnabled.Store(false, EMemoryOrder::Relaxed);
#endif
}
FSnapshot Capture()
{
    // Each field is atomic and cumulative. Concurrent completion may straddle the fields/window;
    // this is a race-free observation, not a globally synchronized animation-task barrier.
    FSnapshot Out; Out.AnimationEvaluate = Animation.Read(); Out.CharacterUpdate = Component.Read(); return Out;
}
FSnapshot Delta(const FSnapshot& Newer, const FSnapshot& Older)
{
    FSnapshot Out;
    Out.AnimationEvaluate = Difference(Newer.AnimationEvaluate, Older.AnimationEvaluate);
    Out.CharacterUpdate = Difference(Newer.CharacterUpdate, Older.CharacterUpdate);
    return Out;
}
void RecordCompleted(EScope Scope, uint64 ElapsedCycles)
{
    // A scope already in flight still commits once if collection is disabled during its lifetime.
    if (Scope == EScope::AnimationEvaluate) Animation.Add(ElapsedCycles);
    else if (Scope == EScope::CharacterUpdate) Component.Add(ElapsedCycles);
}
}

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationCpuMetricsS362, "Memoria.Presentation.S362.CpuMetrics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPresentationCpuMetricsS362::RunTest(const FString&)
{
    using namespace MemoriaPresentationMetrics;
    const bool bPrevious = IsEnabled();
    ON_SCOPE_EXIT { SetEnabled(bPrevious); };
    TestFalse(TEXT("Presentation metrics are disabled in a normal editor run"), bPrevious);
    SetEnabled(false);
    const auto DisabledBefore = Capture();
    { FScopedCpu A(EScope::AnimationEvaluate); FScopedCpu B(EScope::CharacterUpdate); }
    const auto Disabled = Delta(Capture(), DisabledBefore);
    TestTrue(TEXT("Disabled scopes add zero cycles and zero calls"), Disabled.AnimationEvaluate.Cycles == 0 && Disabled.AnimationEvaluate.Calls == 0 &&
        Disabled.CharacterUpdate.Cycles == 0 && Disabled.CharacterUpdate.Calls == 0);
    const auto Before = Capture();
    SetEnabled(true);
    TArray<TFuture<void>> Work;
    for (int32 Thread = 0; Thread < 3; ++Thread)
        Work.Add(Async(EAsyncExecution::ThreadPool, []
        {
            for (int32 Iteration = 0; Iteration < 8; ++Iteration)
            {
                FScopedCpu Scope(EScope::AnimationEvaluate);
                // Fixed bounded work: real platform reads prevent an optimized-away empty test scope.
                for (int32 I = 0; I < 16; ++I) { const uint64 Sample = FPlatformTime::Cycles64(); (void)Sample; }
            }
        }));
    for (auto& Job : Work) Job.Wait();
    {
        FScopedCpu Scope(EScope::CharacterUpdate);
        for (int32 I = 0; I < 16; ++I) { const uint64 Sample = FPlatformTime::Cycles64(); (void)Sample; }
    }
    const auto After = Capture(); const auto Measured = Delta(After, Before);
    TestTrue(TEXT("Concurrent evaluation scopes accumulate real elapsed cycles without lost calls"), Measured.AnimationEvaluate.Calls >= 24 && Measured.AnimationEvaluate.Cycles > 0);
    TestTrue(TEXT("Component scope has its own measured counter"), Measured.CharacterUpdate.Calls >= 1 && Measured.CharacterUpdate.Cycles > 0);
    TestTrue(TEXT("Counter snapshots are monotonic"), After.AnimationEvaluate.Cycles >= Before.AnimationEvaluate.Cycles && After.AnimationEvaluate.Calls >= Before.AnimationEvaluate.Calls);
    const auto Same = Delta(After, After);
    TestTrue(TEXT("Repeated identical snapshot has a zero delta"), Same.AnimationEvaluate.Cycles == 0 && Same.AnimationEvaluate.Calls == 0 && Same.CharacterUpdate.Cycles == 0 && Same.CharacterUpdate.Calls == 0);
    SetEnabled(false);
    const auto StoppedBefore = Capture();
    { FScopedCpu Scope(EScope::CharacterUpdate); }
    const auto Stopped = Delta(Capture(), StoppedBefore);
    TestTrue(TEXT("Disabling returns the component path to zero collection"), Stopped.CharacterUpdate.Cycles == 0 && Stopped.CharacterUpdate.Calls == 0);
    return !HasAnyErrors();
}
#endif
