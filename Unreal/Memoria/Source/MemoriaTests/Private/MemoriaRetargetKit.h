#pragma once
// Shared by the combat retarget commandlets (S311 mannequin melee, S312 UAL2 sword): saving, IK rig
// characterization and the editor-style retargeter setup.
#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Rig/IKRigDefinition.h"
#include "Retargeter/IKRetargeter.h"
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "RigEditor/IKRigAutoCharacterizer.h"
#include "RigEditor/IKRigAutoFBIK.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/RootMotionGeneratorOp.h"
#include "Retargeter/RetargetOps/RunIKRigOp.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
namespace MemoriaRetargetKit
{
inline const TCHAR* CombatRoot = TEXT("/Game/Memoria/Presentation/Combat/");
inline bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Object->GetOutermost(), Object, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("COMBAT_ASSET %s %s"), *Object->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved;
}
inline void Remove(const FString& Package)
{ IFileManager::Get().Delete(*FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension()), false, true, true); }
template<class T> T* Load(const FString& Path)
{ return LoadObject<T>(nullptr, *(Path + TEXT(".") + FPackageName::GetShortName(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet); }
// An IK rig characterized the way the editor's batch retarget does it: auto chains (spine, arms, legs,
// head) and full-body IK, keeping the characterization for the retarget pose and root setup.
inline UIKRigDefinition* MakeRig(const FString& Name, USkeletalMesh* Mesh, FAutoCharacterizeResults& Results)
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
// SRetargetAnimAssetsWindow's auto retarget: a retargeter made with NewObject has no op stack, so
// nothing moves until the default ops (pelvis, FK chains, IK chains, IK solve, root motion) are added.
inline UIKRetargeter* MakeRetargeter(const FString& Name, UIKRigDefinition* SourceRig, const FAutoCharacterizeResults& SourceResults,
    UIKRigDefinition* TargetRig, const FAutoCharacterizeResults& TargetResults)
{
    auto* Retargeter = NewObject<UIKRetargeter>(CreatePackage(*(FString(CombatRoot) + Name)), *Name, RF_Public | RF_Standalone);
    const UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
    {
        FScopedReinitializeIKRetargeter Reinitialize(Controller, ERetargetRefreshMode::ProcessorAndOpStack);
        Controller->SetIKRig(ERetargetSourceOrTarget::Source, SourceRig);
        Controller->SetIKRig(ERetargetSourceOrTarget::Target, TargetRig);
        Controller->RemoveAllOps(); Controller->AddDefaultOps();
        if (FIKRetargetFKChainsOp* FK = Controller->GetFirstRetargetOpOfType<FIKRetargetFKChainsOp>())
            FK->Settings.ChainMapping.AutoMapChains(EAutoMapChainType::Exact, true);
        // The target is authored in an A-pose; align its retarget pose to the source, keeping excluded bones (feet) flat.
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
    for (int32 Op = 0; Op < Controller->GetNumRetargetOps(); ++Op)
        UE_LOG(LogTemp, Display, TEXT("COMBAT_OP %d %s"), Op, *Controller->GetOpName(Op).ToString());
    UE_LOG(LogTemp, Display, TEXT("COMBAT_RETARGETER %s ops=%d"), *Name, Controller->GetNumRetargetOps());
    return Retargeter;
}
}
