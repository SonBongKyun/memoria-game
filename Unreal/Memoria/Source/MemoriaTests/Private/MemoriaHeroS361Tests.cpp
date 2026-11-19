#include "Misc/AutomationTest.h"
#include "MemoriaArrelSoleS362Validation.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const TCHAR* HeroNames[] = {TEXT("Arrel"), TEXT("Elia")};
const float HeroHeights[] = {150.f, 138.f};
const TCHAR* ActionRoles[] = {TEXT("Attack_01"), TEXT("Attack_02"), TEXT("Attack_03"),
    TEXT("ChargedAttack"), TEXT("Dash"), TEXT("HitReact_Front_Lgt_01"), TEXT("Death_Front_01")};

TArray<FTransform> ComponentBind(const FReferenceSkeleton& Ref)
{
    TArray<FTransform> Result = Ref.GetRefBonePose();
    for (int32 I = 0; I < Result.Num(); ++I)
        if (Ref.GetParentIndex(I) != INDEX_NONE) Result[I] = Result[I] * Result[Ref.GetParentIndex(I)];
    return Result;
}

// This is the same shared field component initialized by the player/companion. The fixture owns
// only its temporary actors and inspection camera; the product camera, lights and gameplay stay intact.
class FHeroS361Replay final : public IAutomationLatentCommand
{
    enum class EPhase { ViewsAndWalk, StartAction, Action, Recovery, HeldDeath, NextHero };
    FAutomationTestBase* Test;
    const bool bBaseline;
    double Started = FPlatformTime::Seconds(), OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    bool bFixed = false, bOldFixed = false, bWorldReady = false;
    int32 Hero = 0, Frame = 0, ActionIndex = 0, ActionFrames = 0, RecoveryFrames = 0;
    bool bActionSampled[3] = {false, false, false};
    EPhase Phase = EPhase::ViewsAndWalk;
    float Height = 150.f, ActionLength = 0.f, LastGaitPhase = 0.f;
    float MaxGripError = 0.f, MaxGripDistance = 0.f;
    int32 GripSamples = 0, WalkSamples = 0;
    int32 CompatibleClipRoleChecks = 0, RenderedActions = 0, HeroRenderedActions = 0, MotionFrames = 0;
    FTransform GripRelative;
    FVector WalkHandStart = FVector::ZeroVector;
    bool bHandMoved = false;
    TWeakObjectPtr<UWorld> FixtureWorld;
    TWeakObjectPtr<AActor> Actor, OldViewTarget;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    TWeakObjectPtr<UStaticMeshComponent> Grip;
    struct FHidden { TWeakObjectPtr<USceneComponent> Component; bool bWasVisible; };
    TArray<FHidden> HiddenFigures;

    FString Name() const { return HeroNames[Hero]; }
    // Arrel additionally renders Block before the final held death. These are requested
    // actions, distinct from the 21 compatible combat role checks inspected statically.
    // A base role can resolve to the same sword asset as a later explicit sword role.
    const TCHAR* RequestedRole() const
    {
        if (Hero == 0 && ActionIndex == 6) return TEXT("Block");
        return ActionRoles[Hero == 0 && ActionIndex == 7 ? 6 : ActionIndex];
    }
    bool IsDeath() const { return ActionIndex == (Hero == 0 ? 7 : 6); }
    bool IsBlock() const { return Hero == 0 && ActionIndex == 6; }
    bool IsArrelKnockback() const
    {
        return Hero == 0 && FString(RequestedRole()) == TEXT("HitReact_Front_Lgt_01")
            && Figure.IsValid() && Figure->GetActionClip()
            && Figure->GetActionClip()->GetName().EndsWith(TEXT("Hit_Knockback"));
    }
    void Capture(const FString& View)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture"))) return;
        const FString Folder = bBaseline ? TEXT("Validation/HeroS361Baseline") : TEXT("Validation/HeroS361");
        if (View.StartsWith(TEXT("Motion"))) ++MotionFrames;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / Folder / (Name() + View + TEXT(".png")), true, false);
    }
    void Look(int32 Direction = 0, bool bFace = false)
    {
        auto* Skin = Figure->GetSkeletalMesh();
        const FVector Base = Actor->GetActorLocation();
        const FVector Target = bFace ? Skin->GetSocketLocation(TEXT("head")) + FVector(0, 0, Height * .055f)
            : Base + FVector(0, 0, Height * .43f);
        FVector Toward = Skin->GetForwardVector();
        if (Direction == 1) Toward = Skin->GetRightVector();
        if (Direction == 2) Toward = -Toward;
        const FVector Eye = Target + Toward * Height * (bFace ? .82f : 3.1f)
            + FVector(0, 0, Height * (bFace ? .025f : .10f));
        Camera->SetActorLocation(Eye);
        Camera->SetActorRotation((Target - Eye).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(bFace ? 35.f : 42.f);
        FixtureWorld->GetFirstPlayerController()->SetViewTarget(Camera.Get());
    }
    void HideOtherFigures(UWorld* World)
    {
        TSet<USceneComponent*> Seen;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UMemoriaFieldCharacterComponent*> Parts;
            It->GetComponents(Parts);
            for (auto* Part : Parts)
            {
                TArray<USceneComponent*> Descendants;
                Part->GetChildrenComponents(true, Descendants);
                Descendants.Add(Part);
                for (auto* Component : Descendants)
                {
                    if (!Component || Seen.Contains(Component)) continue;
                    Seen.Add(Component);
                    HiddenFigures.Add({Component, Component->GetVisibleFlag()});
                    Component->SetVisibility(false, false);
                }
            }
        }
        // A presentation actor can also own the market scenery and fill lights. Only the
        // field-component subtrees above are hidden; no owning actor is hidden or disabled.
    }
    bool CheckClip(UAnimSequence* Clip, USkeleton* Skeleton, const FString& Role)
    {
        if (!Test->TestNotNull(*FString::Printf(TEXT("S361 %s compatible clip %s loads"), *Name(), *Role), Clip)) return false;
        const bool bCompatible = Clip->GetSkeleton() == Skeleton && Clip->GetPlayLength() > .05f;
        Test->TestTrue(TEXT("S361 existing clip retains its exact original skeleton and positive duration"), bCompatible);
        if (!bCompatible) return false;
        Clip->WaitOnExistingCompression(true);
        bool bFinite = true;
        const int32 Bones = Skeleton->GetReferenceSkeleton().GetNum();
        for (int32 Bone = 0; Bone < Bones; ++Bone)
            for (int32 Sample = 0; Sample <= 8; ++Sample)
            {
                FTransform Pose;
                Clip->GetBoneTransform(Pose, FSkeletonPoseBoneIndex(Bone),
                    FAnimExtractContext(Clip->GetPlayLength() * Sample / 8., false), false);
                const FVector Scale = Pose.GetScale3D();
                bFinite &= !Pose.ContainsNaN() && Scale.GetAbsMin() > .001f && Scale.GetAbsMax() < 10.f;
            }
        Test->TestTrue(TEXT("S361 all sampled existing animation bone transforms remain finite and nonsingular"), bFinite);
        Test->AddInfo(FString::Printf(TEXT("S361_COMPATIBLE_CLIP %s role=%s asset=%s duration=%.6f bones=%d finite=%d"),
            *Name(), *Role, *Clip->GetPathName(), Clip->GetPlayLength(), Bones, bFinite ? 1 : 0));
        return bFinite;
    }
    bool Initialize(UWorld* World, APlayerController* PC)
    {
        Height = HeroHeights[Hero];
        Actor = World->SpawnActor<AActor>();
        if (!Test->TestTrue(TEXT("S361 fixture actor and inspection camera exist"), Actor.IsValid() && Camera.IsValid())) return false;
        auto* Root = NewObject<USceneComponent>(Actor.Get());
        Actor->SetRootComponent(Root); Root->RegisterComponent();
        FVector At = PC->GetPawn()->GetActorLocation() + FVector(-200, -240, 0); At.Z = 0;
        Actor->SetActorLocation(At);
        auto* F = NewObject<UMemoriaFieldCharacterComponent>(Actor.Get());
        F->SetupAttachment(Root); F->RegisterComponent(); Figure = F;
        if (!Test->TestTrue(TEXT("S361 actual shared field character initialization succeeds"), F->InitializeCharacter(Name().ToLower(), Height))) return false;
        auto* Skin = F->GetSkeletalMesh();
        if (!Test->TestNotNull(TEXT("S361 initialized hero has its real skeletal component"), Skin)) return false;
        const FString OriginalFolder = Hero == 0 ? TEXT("/Game/Memoria/Presentation/Field3D/Arrel/")
            : TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/Elia/");
        const FString Asset = TEXT("SK_") + Name();
        auto* Original = LoadObject<USkeletalMesh>(nullptr, *(OriginalFolder + Asset + TEXT(".") + Asset));
        if (!Test->TestNotNull(TEXT("S361 immutable original body loads for bind comparison"), Original)) return false;
        if (bBaseline)
        {
            // Initialize through production first, then explicitly select the immutable baseline SK.
            // Preserve its existing runtime animation pointers and source-height normalization.
            auto* Before = Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
            UAnimSequence* Idle = Before ? Before->Idle.Get() : nullptr;
            UAnimSequence* Walk = Before ? Before->Walk.Get() : nullptr;
            Skin->SetSkeletalMesh(Original);
            Skin->EmptyOverrideMaterials();
            Skin->SetRelativeScale3D(FVector(Height / FMath::Max(1.f, float(Original->GetBounds().BoxExtent.Z * 2))));
            auto* After = Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
            if (!Test->TestNotNull(TEXT("S361 baseline retains the actual field animation instance"), After)) return false;
            After->Idle = Idle; After->Walk = Walk;
        }
        USkeletalMesh* Mesh = Skin->GetSkeletalMeshAsset();
        if (!Test->TestNotNull(TEXT("S361 actual live component has its inspected mesh"), Mesh)) return false;
        if (!bBaseline)
        {
            // Keep the explicit original baseline mode above; production acceptance follows the
            // separately named current revision without weakening bind/clip/pose/grip checks below.
            const bool bSoleR4=Hero==0 && Mesh->GetPathName()==TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel.SK_Arrel");
            const bool bFinish = bSoleR4 || Mesh->GetPathName().Contains(TEXT("/HeroFinishS362/R2/"));
            const FString Package = (bSoleR4 ? TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/")
                : bFinish ? TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/")
                : TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/")) + Name() + TEXT("/") + Asset;
            if(bSoleR4 && !Test->TestTrue(TEXT("Arrel R4 changes only approved20sole skins/4Z; every other native normal/UV/paint/bind/topology exact"),MemoriaArrelSoleNativeS362::ValidateDelivered(Mesh)))return false;
            auto* Delivered = LoadObject<USkeletalMesh>(nullptr, *(Package + TEXT(".") + Asset));
            if (!Test->TestNotNull(TEXT("S361 new immutable hero mesh loads"), Delivered)) return false;
            Test->TestEqual(TEXT("S361 actual production figure binds the delivered hero mesh"), Mesh, Delivered);
            Test->TestEqual(TEXT("S361 new hero material slot count"), Mesh->GetMaterials().Num(), Hero == 0 ? 2 : (bFinish ? 5 : 4));
            const int32 RetainedSlots = Hero == 0 ? 1 : 2;
            for (int32 I = 0; I < RetainedSlots; ++I)
                Test->TestTrue(TEXT("S361 original painted clothing material pointers are retained"),
                    Mesh->GetMaterials().IsValidIndex(I) && Original->GetMaterials().IsValidIndex(I)
                    && Mesh->GetMaterials()[I].MaterialInterface == Original->GetMaterials()[I].MaterialInterface);
            const auto* RenderData = Mesh->GetResourceForRendering();
            if (!Test->TestTrue(TEXT("S361 new hero has actual LOD0 render sections"), RenderData && !RenderData->LODRenderData.IsEmpty())) return false;
            TSet<int32> Used;
            for (const auto& Section : RenderData->LODRenderData[0].RenderSections)
                if (Section.NumVertices > 0) Used.Add(Section.MaterialIndex);
            for (int32 I = RetainedSlots; I < Mesh->GetMaterials().Num(); ++I)
                Test->TestTrue(TEXT("S361 additional facial/detail material is assigned to rendered geometry"),
                    Mesh->GetMaterials()[I].MaterialInterface != nullptr && Used.Contains(I));
        }
        else Test->TestEqual(TEXT("S361 baseline explicitly binds its immutable original mesh"), Mesh, Original);
        Test->TestEqual(TEXT("S361 hero retains its exact original skeleton object"), Mesh->GetSkeleton(), Original->GetSkeleton());
        const auto& OldRef = Original->GetRefSkeleton(); const auto& NewRef = Mesh->GetRefSkeleton();
        if (!Test->TestEqual(TEXT("S361 original bone count remains unchanged"), NewRef.GetNum(), OldRef.GetNum())) return false;
        const auto OldBind = ComponentBind(OldRef), NewBind = ComponentBind(NewRef);
        bool bNames = true, bParents = true, bBind = true;
        for (int32 I = 0; I < OldRef.GetNum(); ++I)
        {
            bNames &= NewRef.GetBoneName(I) == OldRef.GetBoneName(I);
            bParents &= NewRef.GetParentIndex(I) == OldRef.GetParentIndex(I);
            bBind &= !NewBind[I].ContainsNaN() && NewBind[I].Equals(OldBind[I], .001f);
        }
        Test->TestTrue(TEXT("S361 bone names and ordering remain exact"), bNames);
        Test->TestTrue(TEXT("S361 bone parents remain exact"), bParents);
        Test->TestTrue(TEXT("S361 all component-space reference bind matrices remain unchanged"), bBind);
        auto* Anim = Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
        if (!Test->TestNotNull(TEXT("S361 initialized hero uses the actual field animation instance"), Anim)) return false;
        const FString LoopFolder = Hero == 0 ? OriginalFolder : TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/Elia/");
        const FString IdleAsset = TEXT("A_") + Name() + TEXT("_Idle"), WalkAsset = TEXT("A_") + Name() + TEXT("_Walk");
        auto* ExpectedIdle = LoadObject<UAnimSequence>(nullptr, *(LoopFolder + IdleAsset + TEXT(".") + IdleAsset));
        auto* ExpectedWalk = LoadObject<UAnimSequence>(nullptr, *(LoopFolder + WalkAsset + TEXT(".") + WalkAsset));
        Test->TestTrue(TEXT("S361 original Idle/Walk remain the live animation inputs"),
            ExpectedIdle && ExpectedWalk && Anim->Idle == ExpectedIdle && Anim->Walk == ExpectedWalk);
        CheckClip(Anim->Idle, Original->GetSkeleton(), TEXT("Idle"));
        CheckClip(Anim->Walk, Original->GetSkeleton(), TEXT("Walk"));
        int32 CombatRoleChecks = 0;
        for (const auto& Clip : MemoriaCombatClips::Clips())
        {
            // PlayAction performs this exact mapping before loading. In particular Arrel's
            // hit uses Hit_Knockback, not the preserved unused legacy raw HitReact sequence.
            const TCHAR* EffectiveRole = MemoriaCombatClips::ForAction(Name(), Clip.Name);
            const FString Role = FString(Clip.Name) + TEXT(" -> ") + EffectiveRole;
            CombatRoleChecks += CheckClip(MemoriaCombatClips::Load(Name(), EffectiveRole), Original->GetSkeleton(), Role) ? 1 : 0;
        }
        if (Hero == 0) for (const TCHAR* Clip : MemoriaCombatClips::SwordClips())
            CombatRoleChecks += CheckClip(MemoriaCombatClips::Load(Name(), Clip), Original->GetSkeleton(), Clip) ? 1 : 0;
        Test->TestEqual(TEXT("S361 every effective production hero combat role is compatible"), CombatRoleChecks, Hero == 0 ? 14 : 7);
        CompatibleClipRoleChecks += CombatRoleChecks;
        if (Hero == 0)
        {
            const TCHAR* HitRole = MemoriaCombatClips::ForAction(Name(), TEXT("HitReact_Front_Lgt_01"));
            auto* EffectiveHit = MemoriaCombatClips::Load(Name(), HitRole);
            auto* LegacyHit = MemoriaCombatClips::Load(Name(), TEXT("HitReact_Front_Lgt_01"));
            Test->TestTrue(TEXT("S361 Arrel's production hit selects valid Hit_Knockback instead of unused legacy raw HitReact"),
                FString(HitRole) == TEXT("Hit_Knockback") && EffectiveHit && LegacyHit && EffectiveHit != LegacyHit
                && EffectiveHit->GetSkeleton() == Original->GetSkeleton());
            // Do not repair or validate the unused raw asset as a production role. It has
            // inherited zero scales and is immutable/out of scope; baseline hash auditing
            // separately proves this original package remains byte-identical.
            int32 LegacyZeroScaleBones = 0;
            if (LegacyHit && LegacyHit->GetSkeleton())
            {
                LegacyHit->WaitOnExistingCompression(true);
                for (int32 Bone = 0; Bone < LegacyHit->GetSkeleton()->GetReferenceSkeleton().GetNum(); ++Bone)
                {
                    FTransform Pose;
                    LegacyHit->GetBoneTransform(Pose, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(LegacyHit->GetPlayLength() * .5, false), false);
                    LegacyZeroScaleBones += Pose.GetScale3D().GetAbsMin() <= .001f ? 1 : 0;
                }
            }
            Test->AddInfo(FString::Printf(TEXT("S361_PRESERVED_UNUSED_LEGACY_ASSET asset=%s runtime_hit=%s zero_scale_bones_at_midpoint=%d disposition=preserved_unchanged_out_of_scope unused_by_runtime=1"),
                *GetPathNameSafe(LegacyHit), *GetPathNameSafe(EffectiveHit), LegacyZeroScaleBones));
        }
        HeroRenderedActions = 0;
        F->Face(TEXT("Right"));
        if (Hero == 0)
        {
            Test->TestTrue(TEXT("S361 Arrel retains complete sword/scabbard swap props"), F->HasSword());
            F->SetSwordDrawn(true); Grip = F->GetBlade();
        }
        else
        {
            TArray<UStaticMeshComponent*> Parts; Actor->GetComponents(Parts);
            for (auto* Part : Parts)
                if (Part->GetStaticMesh() && Part->GetStaticMesh()->GetName() == TEXT("SM_Elia_staff")) Grip = Part;
            Test->TestTrue(TEXT("S361 Elia retains her listed staff prop"), F->GetPropCount() > 0);
        }
        if (!Test->TestTrue(TEXT("S361 hero grip prop exists"), Grip.IsValid())) return false;
        GripRelative = Grip->GetRelativeTransform();
        MaxGripError = MaxGripDistance = 0.f; GripSamples = WalkSamples = 0; bHandMoved = false;
        LastGaitPhase = F->GaitPhase();
        Skin->TickAnimation(0.f, false); Skin->RefreshBoneTransforms();
        WalkHandStart = Skin->GetSocketLocation(Hero == 0 ? TEXT("hand_r") : TEXT("hand_l")) - Actor->GetActorLocation();
        Look();
        Test->AddInfo(FString::Printf(TEXT("S361_LIVE_HERO %s baseline=%d mesh=%s original=%s skeleton=%s bones=%d height=%.3f combat_clip_role_checks=%d"),
            *Name(), bBaseline ? 1 : 0, *Mesh->GetPathName(), *Original->GetPathName(), *GetPathNameSafe(Mesh->GetSkeleton()), NewRef.GetNum(), Height, CombatRoleChecks));
        return true;
    }
    void CheckGrip(const TCHAR* Pose)
    {
        auto* Skin = Figure->GetSkeletalMesh();
        if (!Test->TestTrue(TEXT("S361 grip remains attached to the live hero"), Grip.IsValid())) return;
        const FName Hand = Hero == 0 ? TEXT("hand_r") : TEXT("hand_l");
        const FTransform Bone = Skin->GetSocketTransform(Hand, RTS_World);
        const FTransform Expected = GripRelative * Bone, Actual = Grip->GetComponentTransform();
        const float Error = FVector::Dist(Expected.GetLocation(), Actual.GetLocation());
        const float Distance = FVector::Dist(Bone.GetLocation(), Actual.GetLocation());
        MaxGripError = FMath::Max(MaxGripError, Error); MaxGripDistance = FMath::Max(MaxGripDistance, Distance); ++GripSamples;
        Test->TestTrue(TEXT("S361 sword/staff retains its hand attachment and finite coherent grip transform"),
            Grip->GetAttachParent() == Skin && Grip->GetAttachSocketName() == Hand
            && !Actual.ContainsNaN() && !Bone.ContainsNaN() && Grip->GetRelativeTransform().Equals(GripRelative, .001f)
            && Actual.Equals(Expected, .1f) && Error < .1f && Distance < Height * .15f);
        Test->AddInfo(FString::Printf(TEXT("S361_GRIP %s pose=%s hand=%s world_error_cm=%.6f hand_distance_cm=%.6f"), *Name(), Pose, *Hand.ToString(), Error, Distance));
    }
    void CheckBody(const TCHAR* Pose, bool bGrounded = false, bool bFallen = false)
    {
        auto* Skin = Figure->GetSkeletalMesh();
        TArray<FFinalSkinVertex> Vertices; Skin->GetCPUSkinnedVertices(Vertices, 0);
        FBox Body(ForceInit); int32 Finite = 0, Near = 0, Above = 0;
        const FVector Origin = Actor->GetActorLocation();
        for (const auto& Vertex : Vertices)
        {
            const FVector P = Skin->GetComponentTransform().TransformPosition(FVector(Vertex.Position));
            if (P.ContainsNaN()) continue;
            ++Finite; Body += P;
            Near += FVector::Dist(P, Origin + FVector(0, 0, Height * .4f)) < Height * 1.7f;
            Above += P.Z >= Origin.Z - Height * .20f;
        }
        const FVector Size = Body.IsValid ? Body.GetSize() : FVector::ZeroVector;
        const FVector Center = Body.IsValid ? Body.GetCenter() - Origin : FVector::ZeroVector;
        const bool bVolume = bFallen ? Size.GetMax() > Height * .35f : Size.Z > Height * .45f;
        Test->TestTrue(TEXT("S361 actual animated body remains finite with nonsingular bounded volume"),
            Vertices.Num() > 500 && Finite == Vertices.Num() && Near > Vertices.Num() * .98f && Above > Vertices.Num() * .90f
            && Body.IsValid && bVolume && Size.GetMax() < Height * 2.f && Size.Size() > Height * .50f
            && Center.Size2D() < Height && Center.Z > -Height * .15f && Center.Z < Height * 1.25f);
        if (bGrounded)
            Test->TestTrue(TEXT("S361 upright hero retains standing height and ground support"),
                Size.Z > Height * .78f && Size.Z < Height * 1.12f && FMath::Abs(Body.Min.Z - Origin.Z + 8.f) < Height * .055f);
        Test->AddInfo(FString::Printf(TEXT("S361_NATIVE_BODY %s pose=%s vertices=%d finite=%d size=%s center=%s ground_cm=%.6f action=%s time=%.6f"),
            *Name(), Pose, Vertices.Num(), Finite, *Size.ToString(), *Center.ToString(), Body.IsValid ? Body.Min.Z - Origin.Z + 8.f : -999.f,
            *GetNameSafe(Figure->GetActionClip()), Figure->GetActionTime()));
        CheckGrip(Pose);
    }
    void DestroyHero()
    {
        if (Actor.IsValid()) Actor->Destroy();
        Figure.Reset(); Actor.Reset(); Grip.Reset();
    }
public:
    explicit FHeroS361Replay(FAutomationTestBase* InTest)
        : Test(InTest), bBaseline(FParse::Param(FCommandLine::Get(), TEXT("S361Baseline"))) {}
    ~FHeroS361Replay() override
    {
        if (FixtureWorld.IsValid())
            if (auto* PC = FixtureWorld->GetFirstPlayerController())
                if (OldViewTarget.IsValid()) PC->SetViewTarget(OldViewTarget.Get());
        DestroyHero();
        if (Camera.IsValid()) Camera->Destroy();
        for (const auto& Hidden : HiddenFigures)
            if (Hidden.Component.IsValid()) Hidden.Component->SetVisibility(Hidden.bWasVisible, false);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 480.) { Test->AddError(TEXT("S361 hero replay timed out")); return true; }
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        auto* PC = World ? World->GetFirstPlayerController() : nullptr;
        if (!PC || !PC->GetPawn() || World->GetTimeSeconds() < .3f) return false;
        auto* Story = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>() : nullptr;
        if (!Story) { Test->AddError(TEXT("S361 narrative subsystem is missing")); return true; }
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1. / 60.);
            for (int32 I = 0; I < 20 && Story->GetState() == EMemoriaSliceState::VN; ++I) Story->Confirm(1);
            return false;
        }
        if (!bWorldReady)
        {
            if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Story->GetState() != EMemoriaSliceState::Exploration) return false;
            FixtureWorld = World; OldViewTarget = PC->GetViewTarget();
            HideOtherFigures(World); Camera = World->SpawnActor<ACameraActor>(); bWorldReady = true;
        }
        if (World != FixtureWorld.Get() || Story->GetState() != EMemoriaSliceState::Exploration)
        { Test->AddError(TEXT("S361 inspection world or exploration state changed unexpectedly")); return true; }
        if (Hero >= UE_ARRAY_COUNT(HeroNames))
        {
            Test->TestEqual(TEXT("S361 static compatible production combat role check count"), CompatibleClipRoleChecks, 21);
            Test->TestEqual(TEXT("S361 rendered requested action count including Arrel block"), RenderedActions, 15);
            Test->AddInfo(FString::Printf(TEXT("S361_HERO_REPLAY_COMPLETE heroes=2 baseline=%d compatible_clip_role_checks=%d rendered_actions=%d motion_frames=%d"),
                bBaseline ? 1 : 0, CompatibleClipRoleChecks, RenderedActions, MotionFrames));
            return true;
        }
        if (!Camera.IsValid()) { Test->AddError(TEXT("S361 inspection camera was lost")); return true; }
        if (Frame == 0 && !Actor.IsValid())
            if (!Initialize(World, PC)) return true;
        if (!Actor.IsValid() || !Figure.IsValid() || !Figure->GetSkeletalMesh())
        { Test->AddError(TEXT("S361 initialized live hero was lost")); return true; }
        auto* Skin = Figure->GetSkeletalMesh();
        switch (Phase)
        {
        case EPhase::ViewsAndWalk:
            if (Frame == 20) { CheckBody(TEXT("idle"), true); Capture(TEXT("Front")); }
            if (Frame == 25) Look(1);
            if (Frame == 35) Capture(TEXT("Profile"));
            if (Frame == 40) Look(2);
            if (Frame == 50) Capture(TEXT("Back"));
            if (Frame == 55) Look(0, true);
            if (Frame == 65) Capture(TEXT("Face"));
            if (Frame >= 80 && Frame < 240)
            {
                if (Frame % 20 == 0) CheckBody(TEXT("walk"), true);
                if (Frame == 105 || Frame == 125 || Frame == 145 || Frame == 165) Capture(FString::Printf(TEXT("Walk%03d"), Frame));
                if (Frame >= 90 && Frame <= 210 && Frame % 3 == 0) Capture(FString::Printf(TEXT("Motion%03d"), Frame));
                const FVector Hand = Skin->GetSocketLocation(Hero == 0 ? TEXT("hand_r") : TEXT("hand_l")) - Actor->GetActorLocation();
                bHandMoved |= FVector::Dist(Hand, WalkHandStart) > Height * .005f;
                WalkSamples += Figure->LocomotionWeight() > .5f ? 1 : 0;
                const FVector Step(.5f, 0, 0); Figure->AdvanceLocomotion(Step, 1.f / 60.f); Actor->AddActorWorldOffset(Step); Look();
            }
            if (Frame >= 240) Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f / 60.f);
            if (Frame == 275)
            {
                CheckBody(TEXT("recovered_walk"), true); Capture(TEXT("RecoveredIdle"));
                Test->TestTrue(TEXT("S361 actual travel advances locomotion and animated hand joints"), WalkSamples > 80 && bHandMoved && Figure->GaitPhase() != LastGaitPhase);
                Test->TestTrue(TEXT("S361 stopping naturally settles locomotion back to idle"), Figure->LocomotionWeight() < .03f);
            }
            if (Frame == 285) { Phase = EPhase::StartAction; ActionIndex = 0; }
            break;
        case EPhase::StartAction:
            Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f / 60.f);
            if (!Test->TestTrue(TEXT("S361 actual PlayAction accepts the unchanged hero combat asset"), Figure->PlayAction(RequestedRole(), 1.f, IsDeath()))) return true;
            Test->TestTrue(TEXT("S361 real action state starts"), Figure->IsActing());
            if (IsBlock())
                Test->TestTrue(TEXT("S361 real block resolves to Arrel's retained Sword_Block clip"),
                    Figure->GetActionClip() && Figure->GetActionClip()->GetName().EndsWith(TEXT("Sword_Block")));
            if (Hero == 0 && FString(RequestedRole()) == TEXT("HitReact_Front_Lgt_01"))
                Test->TestTrue(TEXT("S361 actual Arrel hit plays the retained nonsingular Hit_Knockback sequence"), IsArrelKnockback());
            ActionLength = Figure->GetActionLength(); ActionFrames = 0;
            for (bool& Sample : bActionSampled) Sample = false;
            Look(); Phase = EPhase::Action;
            break;
        case EPhase::Action:
        {
            Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f / 60.f); ++ActionFrames;
            // Let the requested front screenshot render before moving to the block profile.
            if (IsBlock() && bActionSampled[1] && !bActionSampled[2] && Figure->GetActionTime() >= ActionLength * .55f) Look(1);
            const float Fractions[] = {.20f, .45f, .70f};
            for (int32 I = 0; I < 3; ++I)
                if (!bActionSampled[I] && Figure->GetActionTime() >= ActionLength * Fractions[I])
                {
                    bActionSampled[I] = true;
                    Test->TestTrue(TEXT("S361 sampled real action is still active"), Figure->IsActing());
                    const FString Pose = FString(RequestedRole()) + FString::Printf(TEXT("_%d"), I);
                    // The retained Arrel knockback deliberately falls horizontally at the
                    // later samples. Use the same orientation-aware volume policy as death
                    // only for that effective clip; finite, extent, locality and grip guards
                    // remain strict, as does upright ground support for idle/walk/recovery.
                    CheckBody(*Pose, false, (IsDeath() || IsArrelKnockback()) && I > 0);
                    if (IsArrelKnockback())
                        Test->AddInfo(FString::Printf(TEXT("S361_KNOCKBACK_ORIENTATION %s sample=%d fallen_policy=%d clip=%s"),
                            *Name(), I, I > 0 ? 1 : 0, *GetNameSafe(Figure->GetActionClip())));
                    if (I == 1)
                    {
                        Capture(TEXT("Action_") + FString(RequestedRole()));
                    }
                    if (IsBlock())
                    {
                        if (I == 2) Capture(TEXT("BlockProfile"));
                        Test->AddInfo(FString::Printf(TEXT("S361_BLOCK_POSE %s sample=%d clip=%s sword_origin=%s hand=%s"),
                            *Name(), I, *GetNameSafe(Figure->GetActionClip()), *Grip->GetComponentLocation().ToString(),
                            *Skin->GetSocketLocation(TEXT("hand_r")).ToString()));
                    }
                }
            if (IsDeath() && ActionFrames > FMath::CeilToInt(ActionLength * 60.f) + 10)
            { RecoveryFrames = 0; Phase = EPhase::HeldDeath; break; }
            if (!IsDeath() && !Figure->IsActing())
            {
                Test->TestTrue(TEXT("S361 action was inspected at three actual pose times"), bActionSampled[0] && bActionSampled[1] && bActionSampled[2]);
                RecoveryFrames = 0; Phase = EPhase::Recovery;
            }
            else if (ActionFrames > FMath::CeilToInt(ActionLength * 60.f) + 120)
            { Test->AddError(TEXT("S361 non-held action did not recover naturally")); return true; }
            break;
        }
        case EPhase::Recovery:
            Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f / 60.f);
            if (++RecoveryFrames == 12)
            {
                Test->TestFalse(TEXT("S361 natural action recovery returns to locomotion"), Figure->IsActing());
                CheckBody(TEXT("action_recovered"), true);
                ++RenderedActions; ++HeroRenderedActions;
                ++ActionIndex; Phase = EPhase::StartAction;
            }
            break;
        case EPhase::HeldDeath:
            Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f / 60.f);
            if (++RecoveryFrames == 12)
            {
                Test->TestTrue(TEXT("S361 death remains held at its actual final pose"), Figure->IsActing() && FMath::IsNearlyEqual(Figure->GetActionTime(), ActionLength, .025f));
                Test->TestTrue(TEXT("S361 held death was inspected at three actual pose times"), bActionSampled[0] && bActionSampled[1] && bActionSampled[2]);
                CheckBody(TEXT("death_held"), false, true); Capture(TEXT("DeathHeld")); Phase = EPhase::NextHero;
                ++RenderedActions; ++HeroRenderedActions;
                Test->TestEqual(TEXT("S361 this hero rendered every requested action"), HeroRenderedActions, Hero == 0 ? 8 : 7);
                Test->AddInfo(FString::Printf(TEXT("S361_HERO_COMPLETE %s baseline=%d rendered_actions=%d walk_samples=%d grip_samples=%d max_grip_error_cm=%.6f max_hand_distance_cm=%.6f"),
                    *Name(), bBaseline ? 1 : 0, HeroRenderedActions, WalkSamples, GripSamples, MaxGripError, MaxGripDistance));
            }
            break;
        case EPhase::NextHero:
            DestroyHero(); ++Hero; Frame = 0; Phase = EPhase::ViewsAndWalk; return false;
        }
        ++Frame;
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS361Test, "MemoriaGraphics.S361", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeroS361Test::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHeroS361Replay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
