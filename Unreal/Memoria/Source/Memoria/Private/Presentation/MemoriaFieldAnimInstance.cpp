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
        Top.A.SetLinkNode(&Blend); Top.B.SetLinkNode(&Action);
        Idle.SetTeleportToExplicitTime(true); Walk.SetTeleportToExplicitTime(true); Action.SetTeleportToExplicitTime(true);
        Top.Initialize_AnyThread(FAnimationInitializeContext(this));
    }
    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Inputs = CastChecked<UMemoriaFieldAnimInstance>(Instance);
        Idle.SetSequence(Inputs->Idle); Walk.SetSequence(Inputs->Walk);
        Idle.SetExplicitTime(Inputs->Idle ? FMath::Fmod(Inputs->Age, FMath::Max(.01f, Inputs->Idle->GetPlayLength())) : 0.f);
        Walk.SetExplicitTime(Inputs->Walk ? Inputs->Phase * Inputs->Walk->GetPlayLength() : 0.f);
        Blend.Alpha = FMath::Clamp(Inputs->Weight, 0.f, 1.f);
        Action.SetSequence(Inputs->Action);
        Action.SetExplicitTime(Inputs->Action ? FMath::Clamp(Inputs->ActionTime, 0.f, Inputs->Action->GetPlayLength()) : 0.f);
        Top.Alpha = Inputs->Action ? FMath::Clamp(Inputs->ActionWeight, 0.f, 1.f) : 0.f;
    }
    virtual void CacheBones() override { Top.CacheBones_AnyThread(FAnimationCacheBonesContext(this)); }
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& Context) override { Top.Update_AnyThread(Context); }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        Top.Evaluate_AnyThread(Output);
        // Root lock: combat clips carry the mannequin's lunge on the root bone. The pawn owns field position, so
        // the mesh must not wander off it (and snap back) while a clip plays.
        if (Output.Pose.GetNumBones() > 0)
        {
            const FCompactPoseBoneIndex Root(0);
            Output.Pose[Root].SetTranslation(Output.Pose.GetRefPose(Root).GetTranslation());
        }
        return true;
    }
private:
    FAnimNode_SequenceEvaluator_Standalone Idle, Walk, Action;
    FAnimNode_TwoWayBlend Blend, Top;
};
}
FAnimInstanceProxy* UMemoriaFieldAnimInstance::CreateAnimInstanceProxy() { return new FFieldAnimProxy(this); }
void UMemoriaFieldAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
