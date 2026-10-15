#include "Misc/AutomationTest.h"
#include "MemoriaHumanoidS357Checks.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/MemoriaSliceHost.h"
#include "Framework/MemoriaFieldPawn.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalRenderPublic.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/MemoriaEliaCompanion.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "MaterialShared.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Check actual deformed body vertices during the gameplay replay. Bone names and imported bounds
// can survive even when a bad hit clip has collapsed the rendered figure to one point underground.
void CheckBaseFoeDeformation(FAutomationTestBase* Test,AMemoriaFieldMonster* F,APlayerController* PC,const TCHAR* Pose,bool bFallen=false)
{
    auto* Figure=F?F->GetFigure():nullptr;auto* Skin=Figure?Figure->GetSkeletalMesh():nullptr;
    if (!Test->TestTrue(TEXT("Base foe deformation inspection has its live actor, rig and camera"),F && Skin && PC))return;
    TArray<FFinalSkinVertex> Vertices;Skin->GetCPUSkinnedVertices(Vertices,0);
    if (!Test->TestTrue(TEXT("Base foe has actual CPU-skinned rendered vertices"),Vertices.Num()>100))return;
    const auto BodyVertices=MemoriaHumanoidS357::BodyVertices(Skin,Vertices);
    if(!Test->TestTrue(TEXT("Retained humanoid body has actual render vertices independently of the dense face"),BodyVertices.Num()>100))return;
    const FVector Actor=F->GetActorLocation();const double Height=F->Spec().Height;
    int32 Width=0,ScreenHeight=0;PC->GetViewportSize(Width,ScreenHeight);
    FBox Body(ForceInit);FBox2D Projection(ForceInit);int32 Finite=0,Above=0,Near=0,Visible=0;
    for (const auto& Vertex:BodyVertices)
    {
        const FVector Point=Skin->GetComponentTransform().TransformPosition(FVector(Vertex.Position));
        if (Point.ContainsNaN())continue;
        ++Finite;Body+=Point;
        if (Point.Z>=Actor.Z-35.f)++Above;
        if (FVector::Dist2D(Point,Actor)<=Height*1.65 && FMath::Abs(Point.Z-Actor.Z)<=Height*2.0)++Near;
        FVector2D Pixel;if (PC->ProjectWorldLocationToScreen(Point,Pixel))
        {
            Projection+=Pixel;if (Pixel.X>=0 && Pixel.X<Width && Pixel.Y>=0 && Pixel.Y<ScreenHeight)++Visible;
        }
    }
    const double Count=BodyVertices.Num();const FVector Size=Body.IsValid?Body.GetSize():FVector::ZeroVector;
    const FVector Center=Body.IsValid?Body.GetCenter()-Actor:FVector::ZeroVector;
    const FVector2D Pixels=Projection.bIsValid?Projection.GetSize():FVector2D::ZeroVector;
    const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
    Test->AddInfo(FString::Printf(TEXT("S356_BASE_FOE_SKIN %s pose=%s action=%s time=%.4f weight=%.4f vertices=%d finite=%d above=%.3f near=%.3f visible=%.3f center=%s size=%s projected=%s root_scale=%s head_scale=%s"),
        *Figure->GetCharacterId(),Pose,*GetNameSafe(Figure->GetActionClip()),Figure->GetActionTime(),Anim?Anim->ActionWeight:-1.f,BodyVertices.Num(),Finite,Above/Count,Near/Count,Visible/Count,*Center.ToString(),*Size.ToString(),*Pixels.ToString(),
        *Skin->GetSocketTransform(TEXT("root"),RTS_Component).GetScale3D().ToString(),*Skin->GetSocketTransform(TEXT("head"),RTS_Component).GetScale3D().ToString()));
    Test->TestTrue(TEXT("Base foe body remains finite, above ground and close to its actual encounter"),Finite==BodyVertices.Num() && Above/Count>=.85 && Near/Count>=.85);
    const bool PlausibleShape=bFallen?Center.Z>=-Height*.05 && Size.GetMax()>=Height*.35:Center.Z>=Height*.10 && Size.Z>=Height*.45;
    Test->TestTrue(TEXT("Base foe animated body retains plausible position and nonzero volume"),Body.IsValid && Center.Size2D()<=Height*.95 && Center.Z<=Height*1.25 && Size.Z<=Height*2.0 && PlausibleShape);
    const bool Readable=bFallen?Pixels.GetMax()>=FMath::Min(Width,ScreenHeight)*.06:Pixels.X>=Width*.015 && Pixels.Y>=ScreenHeight*.075;
    Test->TestTrue(TEXT("Base foe rendered body is visible with a readable projected extent"),Width>0 && ScreenHeight>0 && Visible/Count>=.75 && Readable);
    MemoriaHumanoidS357::CheckFace(Test,Skin,F,PC,Pose);
}
// S313: the field foes. A void husk shambles on UAL2's zombie clips and scratches; a market thief runs in
// fast with a short blade and cuts. Close captures of each, then both chasing Arrel from the game camera.
// S326: they wear Codex's models (the husk's violet cracks from its mask, the thief's dagger in hand), the
// clips retargeted onto them from the mannequin.
class FFieldFoesReplay final : public IAutomationLatentCommand
{
public:
    explicit FFieldFoesReplay(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    ~FFieldFoesReplay() override { if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds() - Started > 180) { Test->AddError(FString::Printf(TEXT("Field foes timeout in phase %d"), Phase)); return true; }
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World ? Cast<AMemoriaSliceController>(World->GetFirstPlayerController()) : nullptr;
        auto* Pawn = PC ? Cast<AMemoriaFieldPawn>(PC->GetPawn()) : nullptr;
        if (!Pawn || World->GetTimeSeconds() < .3) return false;
        auto* Host = World->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0 / 60.0);
            for (int32 I = 0; I < 20 && Host->GetState() == EMemoriaSliceState::VN; ++I) Host->Confirm(1);
            return false;
        }
        if (!World->GetMapName().EndsWith(TEXT("L_VerdanHost")) || World->GetTimeSeconds() == LastWorldTime) return false;
        LastWorldTime = World->GetTimeSeconds(); ++Frame;
        auto* Combat = World->GetSubsystem<UMemoriaFieldCombatSubsystem>();
        if (!Combat || !Combat->GetPlayer() || Host->GetState() != EMemoriaSliceState::Exploration) return false;
        auto Capture = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Validation/FieldFoes") / (FString(Name) + TEXT(".png")), true, false); };
        auto Look = [&](const AActor* At, float Yaw)
        {
            if (!Camera.IsValid()) Camera = World->SpawnActor<ACameraActor>();
            const FVector Center = At->GetActorLocation();
            const FVector Eye = Center + FRotator(0, Yaw, 0).Vector() * 320.f + FVector(0, 0, 130.f);
            Camera->SetActorLocation(Eye); Camera->SetActorRotation((Center + FVector(0, 0, 85.f) - Eye).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(50.f); PC->SetViewTarget(Camera.Get());
        };
        auto CheckBaseSurface = [&](AMemoriaFieldMonster* Monster)
        {
            auto* Mesh = Monster ? Monster->GetFigure()->GetSkeletalMesh() : nullptr;
            if (!Mesh) return;
            for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I)
            {
                auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                if (!Test->TestNotNull(TEXT("Base foe body has a runtime material"), Material)) continue;
                FLinearColor Tint;
                float Weathering = -1.f, Detail = -1.f;
                Test->TestTrue(*FString::Printf(TEXT("%s body %d explicitly retains its painted colour"), Monster->Spec().Name, I),
                    Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SurfaceTint")), Tint, true) && Tint.Equals(FLinearColor::White, .001f));
                Test->TestTrue(*FString::Printf(TEXT("%s body %d explicitly retains its base wear"), Monster->Spec().Name, I),
                    Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Weathering")), Weathering, true) && FMath::IsNearlyZero(Weathering));
                const float ExpectedDetail=I==2?.08f:.14f;
                // DetailStrength is inherited from the authored parent, unlike runtime hit/tint
                // overrides. Read the effective value, then verify the real normal graph uses it.
                Test->TestTrue(TEXT("The base foe retains restrained face and body normal detail"),Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("DetailStrength")),Detail) && FMath::IsNearlyEqual(Detail,ExpectedDetail));
                const FString NormalId=Monster->GetFigure()->GetCharacterId()==TEXT("Husk")?TEXT("Husk"):TEXT("Thief");
                auto* ExpectedNormal=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/Memoria/Presentation/SurfaceDetail/T_")+NormalId+TEXT("Detail_Normal.T_")+NormalId+TEXT("Detail_Normal")));
                UTexture* LiveNormal=nullptr;
                Test->TestTrue(TEXT("The base foe keeps its original effective normal texture"),ExpectedNormal && Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DetailNormal")),LiveNormal) && LiveNormal==ExpectedNormal);
                auto* Parent=Material->Parent?Material->Parent->GetMaterial():nullptr;
                bool HasStrength=false,HasNormal=false;TSet<UMaterialExpression*> Seen;
                TFunction<void(UMaterialExpression*)> InspectNormal=[&](UMaterialExpression* Expression)
                {
                    if(!Expression || Seen.Contains(Expression))return;Seen.Add(Expression);
                    if(auto* Scalar=Cast<UMaterialExpressionScalarParameter>(Expression);Scalar && Scalar->ParameterName==TEXT("DetailStrength") && FMath::IsNearlyEqual(Scalar->DefaultValue,ExpectedDetail))HasStrength=true;
                    if(auto* Texture=Cast<UMaterialExpressionTextureSample>(Expression);Texture && Texture->Texture==ExpectedNormal)HasNormal=true;
                    for(FExpressionInput* Input:Expression->GetInputsView())if(Input)InspectNormal(Input->Expression);
                };
                if(Parent)InspectNormal(Parent->GetEditorOnlyData()->Normal.Expression);
                Test->TestTrue(TEXT("The base foe actual normal output connects the authored strength and original texture"),Parent && ExpectedNormal && HasStrength && HasNormal);
                Test->AddInfo(FString::Printf(TEXT("S357_BASE_NORMAL %s slot=%d effective=%.4f expected=%.4f texture=%s connected=%d"),*Monster->GetFigure()->GetCharacterId(),I,Detail,ExpectedDetail,*GetPathNameSafe(LiveNormal),HasStrength && HasNormal));
            }
            // The new surface controls must not replace the existing dynamic hit flash.
            Monster->GetFigure()->SetHitFlash(.65f);
            for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I)
            {
                auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I));
                float Hit = -1.f;
                FLinearColor Tint;
                Test->TestTrue(TEXT("Surface detail preserves the live body hit flash"), Material &&
                    Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Hit")), Hit, true) && FMath::IsNearlyEqual(Hit, .65f));
                Test->TestTrue(TEXT("Hit flash does not replace the base surface tint"), Material &&
                    Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("SurfaceTint")), Tint, true) && Tint.Equals(FLinearColor::White, .001f));
            }
            Monster->GetFigure()->SetHitFlash(0.f);
        };
        const FVector Me = Pawn->GetActorLocation();
        AMemoriaFieldMonster* H = Husk.Get(); AMemoriaFieldMonster* T = Thief.Get();
        if (Phase == 0)
        {
            for (TActorIterator<AMemoriaEliaCompanion> It(World); It; ++It) It->SetActorHiddenInGame(true);
            for (const TCHAR* Clip : MemoriaCombatClips::FoeClips())
                Test->TestNotNull(*FString::Printf(TEXT("Foe clip %s"), Clip), MemoriaCombatClips::Load(TEXT("Mannequin"), Clip));
            // The husk comes from the east and the thief from further north: the thief still strikes first.
            Husk = Combat->SpawnWave(1, Me + FVector(460, 0, 0), 0.f, EMemoriaFoeKind::VoidHusk)[0];
            Thief = Combat->SpawnWave(1, Me + FVector(0, 560, 0), 0.f, EMemoriaFoeKind::MarketThief)[0];
            Phase = 1; Mark = Frame; return false;
        }
        if (Phase == 1)
        {
            if (Frame == Mark + 4 && H && T)
            {
                Test->TestTrue(TEXT("The husk is a void husk"), H->GetKind() == EMemoriaFoeKind::VoidHusk && H->GetMaxHealth() == FoeSpec(EMemoriaFoeKind::VoidHusk).Health);
                Test->TestTrue(TEXT("The thief is a market thief"), T->GetKind() == EMemoriaFoeKind::MarketThief && T->GetMaxHealth() == FoeSpec(EMemoriaFoeKind::MarketThief).Health);
                Test->TestTrue(TEXT("The husk wears its model"), H->GetFigure()->IsFoeModel() && H->GetFigure()->GetCharacterId() == TEXT("Husk") &&
                    H->GetFigure()->GetSkeletalMesh()->GetSkeletalMeshAsset()->GetPathName() == MemoriaHumanoidS357::MeshPath(TEXT("Husk")));
                Test->TestTrue(TEXT("The thief wears its model"), T->GetFigure()->IsFoeModel() && T->GetFigure()->GetSkeletalMesh()->GetSkeletalMeshAsset()->GetPathName() == MemoriaHumanoidS357::MeshPath(TEXT("Thief")));
                MemoriaHumanoidS357::CheckAssets(Test,H);MemoriaHumanoidS357::CheckAssets(Test,T);
                CheckBaseSurface(H); CheckBaseSurface(T);
                const auto* HuskAnim = Cast<UMemoriaFieldAnimInstance>(H->GetFigure()->GetSkeletalMesh()->GetAnimInstance());
                Test->TestTrue(TEXT("The husk shambles on the zombie clips"), HuskAnim && HuskAnim->Idle == MemoriaCombatClips::Load(TEXT("Husk"), TEXT("Zombie_Idle_Loop")) &&
                    HuskAnim->Walk == MemoriaCombatClips::Load(TEXT("Husk"), TEXT("Zombie_Walk_Fwd_Loop")));
                Test->TestTrue(TEXT("The thief carries a blade, the husk none"), T->GetFigure()->GetBlade() != nullptr && H->GetFigure()->GetBlade() == nullptr);
                Test->TestTrue(TEXT("The thief holds its own dagger"), T->GetFigure()->GetBlade() && T->GetFigure()->GetBlade()->GetStaticMesh() && T->GetFigure()->GetBlade()->GetStaticMesh()->GetName() == TEXT("SM_Thief_dagger"));
                Look(H, (Me - H->GetActorLocation()).Rotation().Yaw + 25.f);
            }
            // Close captures while each closes in: the husk's shamble, the thief's run with the blade.
            if (Frame == Mark + 22) { CheckBaseFoeDeformation(Test,H,PC,TEXT("approach"));Capture(TEXT("HuskApproach")); }
            // From Arrel's side of each foe, so the camera never sits inside the market's walls.
            if (Frame == Mark + 26 && T) Look(T, (Me - T->GetActorLocation()).Rotation().Yaw + 25.f);
            if (Frame == Mark + 44) { CheckBaseFoeDeformation(Test,T,PC,TEXT("approach"));Capture(TEXT("ThiefApproach")); }
            if (Frame < Mark + 48) return false;
            PC->SetViewTarget(Pawn);
            Phase = 2; Mark = Frame; return false;
        }
        if (Phase == 2)
        {
            if (Frame == Mark + 30)
            {
                Test->TestTrue(TEXT("Both give chase"), H && T && H->GetState() != EMemoriaMonsterState::Idle && T->GetState() != EMemoriaMonsterState::Idle);
                Capture(TEXT("FoesChase"));
            }
            if (T && T->GetState() == EMemoriaMonsterState::Windup && !bThiefStrike)
            {
                bThiefStrike = true;
                Test->TestTrue(TEXT("The thief cuts with the blade clip"), T->GetFigure()->GetActionClip() == MemoriaCombatClips::Load(TEXT("Thief"), TEXT("Sword_Regular_A")));
                Look(T, 60.f); ThiefFrame = Frame;
            }
            if (bThiefStrike && Frame == ThiefFrame + 18) { CheckBaseFoeDeformation(Test,T,PC,TEXT("strike"));Capture(TEXT("ThiefStrike")); }
            if (bThiefStrike && Frame == ThiefFrame + 22) PC->SetViewTarget(Pawn);
            if (H && H->GetState() == EMemoriaMonsterState::Windup && !bHuskStrike)
            {
                bHuskStrike = true;
                Test->TestTrue(TEXT("The husk scratches"), H->GetFigure()->GetActionClip() == MemoriaCombatClips::Load(TEXT("Husk"), TEXT("Zombie_Scratch")));
                HuskFrame = Frame;
                if (!bThiefStrike || Frame > ThiefFrame + 22) Look(H, -40.f);
            }
            if (bHuskStrike && Frame == HuskFrame + 30) { CheckBaseFoeDeformation(Test,H,PC,TEXT("strike"));Capture(TEXT("HuskStrike")); }
            if (bHuskStrike && bThiefStrike && Frame > FMath::Max(HuskFrame, ThiefFrame) + 34)
            {
                // The thief started further off yet struck first: it closes faster than the husk.
                Test->TestTrue(TEXT("The thief outran the husk"), ThiefFrame < HuskFrame);
                Test->TestTrue(TEXT("Arrel was struck"), Combat->GetPlayerHp() < Combat->GetPlayerMaxHp());
                PC->SetViewTarget(Pawn);
                Phase = 3; Mark = Frame;
            }
            if (Frame > Mark + 900) { Test->AddError(TEXT("The foes never struck")); return true; }
            return false;
        }
        if (Phase == 3 && Frame == Mark + 4)
        {
            if (!H || !T) { Test->AddError(TEXT("Base foes were lost before real hit-reaction validation"));return true; }
            // Keep the existing approach, chase and attack assertions above. Afterwards, apply the
            // actual gameplay stun, observe the hit, and let it finish without resetting the action.
            H->Stun(30.f);T->Stun(30.f);Look(H,60.f);
            RecoveryFrame=Frame+18+FMath::CeilToInt(FMath::Max(H->GetFigure()->GetActionLength(),T->GetFigure()->GetActionLength())/.8f*60.f)+8;
            Phase=4;Mark=Frame;return false;
        }
        if (Phase==4)
        {
            if (!H || !T) { Test->AddError(TEXT("Base foe hit inspection lost its actor"));return true; }
            auto CheckHit=[&](AMemoriaFieldMonster* Monster)
            {
                auto* Figure=Monster->GetFigure();auto* Skin=Figure->GetSkeletalMesh();
                const auto* Clip=MemoriaCombatClips::Load(Figure->GetCharacterId(),MemoriaCombatClips::ForAction(Figure->GetCharacterId(),MemoriaCombatClips::Hit()));
                Test->TestTrue(TEXT("Base foe uses the compatible authored final hit role during the real stun"),Clip && Figure->GetActionClip()==Clip
                    && Clip->GetPathName().Contains(TEXT("HumanoidNaturalS359/R6/")) && Clip->GetSkeleton()==Skin->GetSkeletalMeshAsset()->GetSkeleton());
                CheckBaseFoeDeformation(Test,Monster,PC,TEXT("real_stun_hit"));
            };
            if (Frame==Mark+12) { CheckHit(H);Capture(TEXT("HuskHit")); }
            if (Frame==Mark+18) { T->Stun(30.f);Look(T,60.f); }
            if (Frame==Mark+30) { CheckHit(T);Capture(TEXT("ThiefHit")); }
            if (Frame==RecoveryFrame)
            {
                Test->TestFalse(TEXT("Husk real hit action naturally returns to locomotion"),H->GetFigure()->IsActing());
                Test->TestFalse(TEXT("Thief real hit action naturally returns to locomotion"),T->GetFigure()->IsActing());
                Look(H,60.f);
            }
            if (Frame==RecoveryFrame+2) { CheckBaseFoeDeformation(Test,H,PC,TEXT("idle_after_hit"));Capture(TEXT("HuskRecoveredIdle")); }
            if (Frame==RecoveryFrame+4)Look(T,60.f);
            if (Frame==RecoveryFrame+6) { CheckBaseFoeDeformation(Test,T,PC,TEXT("idle_after_hit"));Capture(TEXT("ThiefRecoveredIdle")); }
            if (Frame>RecoveryFrame+6 && Frame<=RecoveryFrame+32)T->GetFigure()->AdvanceLocomotion(FVector(0,2,0),1.f/60.f);
            if (Frame==RecoveryFrame+32)
            {
                Test->TestFalse(TEXT("Base thief recovered walk has no surviving hit override"),T->GetFigure()->IsActing());
                CheckBaseFoeDeformation(Test,T,PC,TEXT("walk_after_hit"));Capture(TEXT("ThiefRecoveredWalk"));
            }
            if (Frame==RecoveryFrame+34)Look(H,60.f);
            if (Frame>RecoveryFrame+34 && Frame<=RecoveryFrame+60)H->GetFigure()->AdvanceLocomotion(FVector(0,2,0),1.f/60.f);
            if (Frame==RecoveryFrame+60)
            {
                Test->TestFalse(TEXT("Base husk recovered walk has no surviving hit override"),H->GetFigure()->IsActing());
                CheckBaseFoeDeformation(Test,H,PC,TEXT("walk_after_hit"));Capture(TEXT("HuskRecoveredWalk"));
            }
            if (Frame>=RecoveryFrame+66)
            {
                Test->TestTrue(TEXT("Base husk plays its compatible held death"),H->GetFigure()->PlayAction(MemoriaCombatClips::Death(),1.f,true));
                Test->TestTrue(TEXT("Base thief plays its compatible held death"),T->GetFigure()->PlayAction(MemoriaCombatClips::Death(),1.f,true));
                DeathFrame=Frame+FMath::Max(90,FMath::CeilToInt(FMath::Max(H->GetFigure()->GetActionLength(),T->GetFigure()->GetActionLength())*60.f)+3);
                Phase=5;return false;
            }
        }
        if (Phase==5)
        {
            if (!H || !T) { Test->AddError(TEXT("Base foe death inspection lost its actor"));return true; }
            if (Frame==DeathFrame)Look(H,60.f);
            auto CheckDeath=[&](AMemoriaFieldMonster* Monster)
            {
                auto* Figure=Monster->GetFigure();auto* Skin=Figure->GetSkeletalMesh();
                const auto* Clip=MemoriaCombatClips::Load(Figure->GetCharacterId(),MemoriaCombatClips::Death());
                Test->TestTrue(TEXT("Base foe death uses the compatible S359 role"),Clip && Figure->GetActionClip()==Clip && Clip->GetSkeleton()==Skin->GetSkeletalMeshAsset()->GetSkeleton()
                    && Clip->GetPathName().Contains(TEXT("HumanoidNaturalS359/R6/")));
                Test->TestTrue(TEXT("Base foe death remains held instead of returning to idle"),Figure->IsActing() && FMath::IsNearlyEqual(Figure->GetActionTime(),Figure->GetActionLength(),.02f));
                CheckBaseFoeDeformation(Test,Monster,PC,TEXT("held_death"),true);
            };
            if (Frame==DeathFrame+4){CheckDeath(H);Capture(TEXT("HuskDeath"));}
            if (Frame==DeathFrame+8)Look(T,60.f);
            if (Frame==DeathFrame+12){CheckDeath(T);Capture(TEXT("ThiefDeath"));}
            if (Frame>=DeathFrame+18){PC->SetViewTarget(Pawn);return true;}
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, OldDelta = 0, LastWorldTime = -1;
    uint64 LastFrame = MAX_uint64;
    int32 Phase = 0, Frame = 0, Mark = 0, ThiefFrame = 0, HuskFrame = 0, RecoveryFrame = 0, DeathFrame = 0;
    bool bFixed = false, bOldFixed = false, bThiefStrike = false, bHuskStrike = false;
    TWeakObjectPtr<AMemoriaFieldMonster> Husk, Thief;
    TWeakObjectPtr<ACameraActor> Camera;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldFoesTest, "MemoriaVisual.FieldFoes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFieldFoesTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FFieldFoesReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
