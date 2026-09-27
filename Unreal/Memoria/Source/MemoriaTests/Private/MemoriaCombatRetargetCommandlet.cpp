#include "MemoriaCombatRetargetCommandlet.h"
#include "Presentation/MemoriaCombatClips.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Rig/IKRigDefinition.h"
#include "Retargeter/IKRetargeter.h"
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "RigEditor/IKRigAutoCharacterizer.h"
#include "RigEditor/IKRigAutoFBIK.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/RootMotionGeneratorOp.h"
#include "Retargeter/RetargetOps/RunIKRigOp.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
namespace
{
const TCHAR* CombatRoot = TEXT("/Game/Memoria/Presentation/Combat/");
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("COMBAT_ASSET %s %s"), *Object->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved;
}
void Remove(const FString& Package)
{ IFileManager::Get().Delete(*FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension()), false, true, true); }
template<class T> T* Load(const FString& Path)
{ return LoadObject<T>(nullptr, *(Path + TEXT(".") + FPackageName::GetShortName(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet); }
// An IK rig characterized the way the editor's batch retarget does it: auto chains (spine, arms, legs,
// head) and full-body IK, keeping the characterization for the retarget pose and root setup.
UIKRigDefinition* MakeRig(const FString& Name, USkeletalMesh* Mesh, FAutoCharacterizeResults& Results)
{
    auto* Rig = NewObject<UIKRigDefinition>(CreatePackage(*(FString(CombatRoot) + Name)), *Name, RF_Public | RF_Standalone);
    const UIKRigController* Controller = UIKRigController::GetController(Rig);
    if (!Controller->SetSkeletalMesh(Mesh)) { UE_LOG(LogTemp, Error, TEXT("COMBAT_RIG_FAILED %s"), *Name); return nullptr; }
    Controller->AutoGenerateRetargetDefinition(Results);
    Controller->SetRetargetDefinition(Results.AutoRetargetDefinition.RetargetDefinition);
    FAutoFBIKResults IK; Controller->AutoGenerateFBIK(IK);
    UE_LOG(LogTemp, Display, TEXT("COMBAT_RIG %s chains=%d pelvis=%s success=%d"), *Name,
        Results.AutoRetargetDefinition.RetargetDefinition.BoneChains.Num(), *Results.AutoRetargetDefinition.RetargetDefinition.PelvisBone.ToString(), Results.bUsedTemplate ? 1 : 0);
    return Save(Rig) ? Rig : nullptr;
}
}
UMemoriaCombatRetargetCommandlet::UMemoriaCombatRetargetCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaCombatRetargetCommandlet::Main(const FString& Params)
{
    const bool bForce = false; // See the header: regenerate from deleted folders, never in place.
    if (FParse::Param(*Params, TEXT("Inspect")))
    {
        // Diagnostics: does each clip actually move the arm away from the reference pose?
        for (const TCHAR* Id : {TEXT("Mannequin"), TEXT("Arrel")})
            for (const auto& Clip : MemoriaCombatClips::Clips())
            {
                auto* Seq = MemoriaCombatClips::Load(Id, Clip.Name);
                if (!Seq) { UE_LOG(LogTemp, Display, TEXT("COMBAT_INSPECT %s %s missing"), Id, Clip.Name); continue; }
                const FReferenceSkeleton& Ref = Seq->GetSkeleton()->GetReferenceSkeleton();
                const int32 Bone = Ref.FindBoneIndex(TEXT("upperarm_r"));
                FString Line = FString::Printf(TEXT("COMBAT_INSPECT %s %s len=%.2f keys=%d bone=%d"), Id, Clip.Name, Seq->GetPlayLength(), Seq->GetNumberOfSampledKeys(), Bone);
                if (Bone != INDEX_NONE)
                {
                    const FQuat RefRot = Ref.GetRefBonePose()[Bone].GetRotation();
                    for (float T : {0.f, .25f, .5f, .75f})
                    {
                        FTransform Out; Seq->GetBoneTransform(Out, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(double(T * Seq->GetPlayLength())), false);
                        Line += FString::Printf(TEXT(" t%.2f=%.1fdeg"), T, FMath::RadiansToDegrees(Out.GetRotation().AngularDistance(RefRot)));
                    }
                }
                UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
            }
        return 0;
    }
    auto* Source = Load<USkeletalMesh>(MemoriaCombatClips::MannequinMesh());
    if (!Source) { UE_LOG(LogTemp, Error, TEXT("COMBAT_MANNEQUIN_MISSING run Unreal/Tools/install_mannequin.py")); return 1; }
    TArray<FAssetData> Clips;
    for (const auto& Clip : MemoriaCombatClips::Clips())
    {
        auto* Sequence = Load<UAnimSequence>(Clip.MannequinPath);
        if (!Sequence) { UE_LOG(LogTemp, Error, TEXT("COMBAT_CLIP_MISSING %s"), Clip.MannequinPath); return 1; }
        Clips.Add(FAssetData(Sequence));
    }
    const FString SourceRigPath = FString(CombatRoot) + TEXT("IK_Mannequin");
    if (bForce) Remove(SourceRigPath);
    // The source rig is rebuilt every run: its characterization feeds each retargeter's root setup.
    Remove(SourceRigPath);
    FAutoCharacterizeResults SourceResults;
    UIKRigDefinition* SourceRig = MakeRig(TEXT("IK_Mannequin"), Source, SourceResults);
    if (!SourceRig) return 1;
    int32 Made = 0;
    FString Only; FParse::Value(*Params, TEXT("Only="), Only);
    for (const TCHAR* Id : {TEXT("Arrel"), TEXT("Elia"), TEXT("Malet")})
    {
        if (!Only.IsEmpty() && Only != Id) continue;
        auto* Target = Load<USkeletalMesh>(FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/%s/SK_%s"), Id, Id));
        if (!Target) { UE_LOG(LogTemp, Warning, TEXT("COMBAT_TARGET_MISSING %s"), Id); continue; }
        const FString Out = MemoriaCombatClips::ClipPath(Id, MemoriaCombatClips::Clips()[0].Name);
        if (!bForce && Load<UAnimSequence>(Out)) { UE_LOG(LogTemp, Display, TEXT("COMBAT_KEPT %s"), Id); continue; }
        const FString RigName = FString(TEXT("IK_")) + Id, RetargeterName = FString(TEXT("RTG_Mannequin_")) + Id;
        if (bForce)
        {
            Remove(FString(CombatRoot) + RigName); Remove(FString(CombatRoot) + RetargeterName);
            for (const auto& Clip : MemoriaCombatClips::Clips()) Remove(MemoriaCombatClips::ClipPath(Id, Clip.Name));
        }
        FAutoCharacterizeResults TargetResults;
        UIKRigDefinition* TargetRig = MakeRig(RigName, Target, TargetResults);
        if (!TargetRig) return 1;
        // SRetargetAnimAssetsWindow's auto retarget: a retargeter made with NewObject has no op stack, so
        // nothing moves until the default ops (pelvis, FK chains, IK chains, IK solve, root motion) are added.
        auto* Retargeter = NewObject<UIKRetargeter>(CreatePackage(*(FString(CombatRoot) + RetargeterName)), *RetargeterName, RF_Public | RF_Standalone);
        const UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
        {
            FScopedReinitializeIKRetargeter Reinitialize(Controller, ERetargetRefreshMode::ProcessorAndOpStack);
            Controller->SetIKRig(ERetargetSourceOrTarget::Source, SourceRig);
            Controller->SetIKRig(ERetargetSourceOrTarget::Target, TargetRig);
            Controller->RemoveAllOps(); Controller->AddDefaultOps();
            if (FIKRetargetFKChainsOp* FK = Controller->GetFirstRetargetOpOfType<FIKRetargetFKChainsOp>())
                FK->Settings.ChainMapping.AutoMapChains(EAutoMapChainType::Exact, true);
            // The target is authored in an A-pose; align its retarget pose to the mannequin, keeping excluded bones (feet) flat.
            const FName Pose = Controller->GetCurrentRetargetPoseName(ERetargetSourceOrTarget::Target);
            Controller->ResetRetargetPose(Pose, TArray<FName>(), ERetargetSourceOrTarget::Target);
            Controller->AutoAlignAllBones(ERetargetSourceOrTarget::Target);
            if (!TargetResults.AutoRetargetDefinition.BonesToExcludeFromAutoPose.IsEmpty())
                Controller->ResetRetargetPose(Pose, TargetResults.AutoRetargetDefinition.BonesToExcludeFromAutoPose, ERetargetSourceOrTarget::Target);
            if (FIKRetargetRunIKRigOp* RunIK = Controller->GetFirstRetargetOpOfType<FIKRetargetRunIKRigOp>()) RunIK->SetEnabled(false);
            if (FIKRetargetRootMotionOp* Root = Controller->GetFirstRetargetOpOfType<FIKRetargetRootMotionOp>())
            {
                const FName TargetRoot = TargetRig->GetSkeleton().BoneNames[0], TargetPelvis = TargetResults.AutoRetargetDefinition.RetargetDefinition.PelvisBone;
                if (TargetRoot == TargetPelvis) Root->SetEnabled(false);
                else if (SourceRig->GetSkeleton().BoneNames[0] == SourceResults.AutoRetargetDefinition.RetargetDefinition.PelvisBone)
                {
                    Root->Settings.RootMotionSource = ERootMotionSource::GenerateFromTargetPelvis;
                    Root->Settings.RootHeightSource = ERootMotionHeightSource::SnapToGround;
                    Root->Settings.bRotateWithPelvis = true; Root->Settings.bMaintainOffsetFromPelvis = true;
                }
            }
        }
        // -DisableOps=0,3 switches ops off by index (diagnostics while an op misbehaves under batch conversion).
        FString Disable; FParse::Value(*Params, TEXT("DisableOps="), Disable);
        TArray<FString> Indices; Disable.ParseIntoArray(Indices, TEXT(","));
        for (const FString& Index : Indices) Controller->SetRetargetOpEnabled(FCString::Atoi(*Index), false);
        for (int32 Op = 0; Op < Controller->GetNumRetargetOps(); ++Op)
            UE_LOG(LogTemp, Display, TEXT("COMBAT_OP %d %s"), Op, *Controller->GetOpName(Op).ToString());
        UE_LOG(LogTemp, Display, TEXT("COMBAT_RETARGETER %s ops=%d"), *RetargeterName, Controller->GetNumRetargetOps());
        if (!Save(Retargeter)) return 1;
        FIKRetargetBatchOperationInputs Inputs;
        Inputs.AssetsToRetarget = Clips; Inputs.SourceMesh = Source; Inputs.TargetMesh = Target; Inputs.IKRetargetAsset = Retargeter;
        Inputs.TargetPath = FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/%s/Combat"), Id);
        Inputs.Search = TEXT("MM_"); Inputs.Replace = TEXT(""); Inputs.Prefix = FString::Printf(TEXT("A_%s_"), Id);
        Inputs.bIncludeReferencedAssets = false; Inputs.bOverwriteExistingFiles = true;
        const TArray<FAssetData> Created = UIKRetargetBatchOperation::RunBatchRetarget(Inputs);
        for (const FAssetData& Asset : Created)
            if (UObject* Object = Asset.GetAsset()) { if (!Save(Object)) return 1; ++Made; }
        UE_LOG(LogTemp, Display, TEXT("COMBAT_RETARGETED %s %d"), Id, Created.Num());
    }
    UE_LOG(LogTemp, Display, TEXT("COMBAT_RETARGET_DONE %d"), Made);
    return 0;
}
