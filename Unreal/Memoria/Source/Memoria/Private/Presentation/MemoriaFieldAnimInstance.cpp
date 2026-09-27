#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
namespace
{
class FFieldAnimProxy final : public FAnimInstanceProxy
{
public:
    explicit FFieldAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        Blend.A.SetLinkNode(&Idle); Blend.B.SetLinkNode(&Walk);
        Idle.SetTeleportToExplicitTime(true); Walk.SetTeleportToExplicitTime(true);
        Blend.Initialize_AnyThread(FAnimationInitializeContext(this));
    }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Inputs = CastChecked<UMemoriaFieldAnimInstance>(Instance);
        Idle.SetSequence(Inputs->Idle); Walk.SetSequence(Inputs->Walk);
        Idle.SetExplicitTime(Inputs->Idle ? FMath::Fmod(Inputs->Age, FMath::Max(.01f, Inputs->Idle->GetPlayLength())) : 0.f);
        Walk.SetExplicitTime(Inputs->Walk ? Inputs->Phase * Inputs->Walk->GetPlayLength() : 0.f);
        Blend.Alpha = FMath::Clamp(Inputs->Weight, 0.f, 1.f);
    }
    virtual void CacheBones() override { Blend.CacheBones_AnyThread(FAnimationCacheBonesContext(this)); }
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& Context) override { Blend.Update_AnyThread(Context); }
    virtual bool Evaluate(FPoseContext& Output) override { Blend.Evaluate_AnyThread(Output); return true; }
private:
    FAnimNode_SequenceEvaluator_Standalone Idle, Walk;
    FAnimNode_TwoWayBlend Blend;
};
}
FAnimInstanceProxy* UMemoriaFieldAnimInstance::CreateAnimInstanceProxy() { return new FFieldAnimProxy(this); }
void UMemoriaFieldAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
