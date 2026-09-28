#include "MemoriaSwordRetargetCommandlet.h"
#include "MemoriaRetargetKit.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "InterchangeManager.h"
#include "InterchangeSourceData.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
using namespace MemoriaRetargetKit;
namespace
{
const TCHAR* ImportPath = TEXT("/Game/Memoria/Imported/UAL2");
const TCHAR* DefaultSource = TEXT("C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_raw/ual2/Universal Animation Library 2[Standard]/Unreal-Godot/UAL2_Standard.glb");
template<class T> TArray<T*> Imported()
{
    TArray<FAssetData> Assets; IAssetRegistry::GetChecked().GetAssetsByPath(FName(ImportPath), Assets, true);
    TArray<T*> Out;
    for (const FAssetData& Asset : Assets) if (Asset.IsInstanceOf(T::StaticClass())) if (T* Object = Cast<T>(Asset.GetAsset())) Out.Add(Object);
    return Out;
}
const TCHAR* PropSource = TEXT("C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models/_raw/s312_sword/");
// The drawn sword (hilt plus a new blade) and the empty scabbard, split from Codex's sheathed prop in Blender.
// The hilt and scabbard keep Arrel's atlas material; the blade keeps the steel material from the FBX.
bool ImportProp(const FString& File, const FString& Name)
{
    const FString Path = TEXT("/Game/Memoria/Presentation/Field3D/Arrel/") + Name;
    if (Load<UStaticMesh>(Path)) { UE_LOG(LogTemp, Display, TEXT("SWORD_PROP_KEPT %s"), *Name); return true; }
    if (!FPaths::FileExists(PropSource + File)) { UE_LOG(LogTemp, Error, TEXT("SWORD_PROP_SOURCE_MISSING %s"), *File); return false; }
    FImportAssetParameters Import; Import.bIsAutomated = true; Import.DestinationName = Name;
    TArray<UObject*> Objects;
    UInterchangeManager::GetInterchangeManager().ImportAsset(TEXT("/Game/Memoria/Presentation/Field3D/Arrel"), UInterchangeManager::CreateSourceData(PropSource + File), Import, Objects);
    UStaticMesh* Mesh = nullptr;
    for (UObject* Object : Objects) if (auto* M = Cast<UStaticMesh>(Object)) Mesh = M;
    auto* Atlas = Load<UMaterialInterface>(TEXT("/Game/Memoria/Presentation/Field3D/Arrel/M_Arrel"));
    if (!Mesh || !Atlas) { UE_LOG(LogTemp, Error, TEXT("SWORD_PROP_FAILED %s objects=%d"), *Name, Objects.Num()); return false; }
    TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
    for (int32 I = 0; I < Slots.Num(); ++I)
    {
        const bool bBlade = Slots[I].MaterialSlotName.ToString().Contains(TEXT("Blade"));
        if (!bBlade) Slots[I].MaterialInterface = Atlas;
        else if (Slots[I].MaterialInterface && !Save(Slots[I].MaterialInterface)) return false;
        UE_LOG(LogTemp, Display, TEXT("SWORD_PROP_SLOT %s %d %s -> %s"), *Name, I, *Slots[I].MaterialSlotName.ToString(), *GetNameSafe(Slots[I].MaterialInterface));
    }
    Mesh->PostEditChange();
    FBoxSphereBounds Bounds = Mesh->GetBounds();
    UE_LOG(LogTemp, Display, TEXT("SWORD_PROP %s extent=%s"), *Mesh->GetPathName(), *Bounds.BoxExtent.ToString());
    return Save(Mesh);
}
}
UMemoriaSwordRetargetCommandlet::UMemoriaSwordRetargetCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaSwordRetargetCommandlet::Main(const FString& Params)
{
    IAssetRegistry::GetChecked().SearchAllAssets(true);
    if (Imported<USkeletalMesh>().IsEmpty())
    {
        FString Source = DefaultSource; FParse::Value(*Params, TEXT("Source="), Source);
        if (!FPaths::FileExists(Source)) { UE_LOG(LogTemp, Error, TEXT("SWORD_SOURCE_MISSING %s"), *Source); return 1; }
        // The pack's own Unreal setup: a fresh skeleton (none given) and every animation imported.
        FImportAssetParameters Import; Import.bIsAutomated = true; Import.bReplaceExisting = true;
        TArray<UObject*> Objects;
        const bool bImported = UInterchangeManager::GetInterchangeManager().ImportAsset(ImportPath, UInterchangeManager::CreateSourceData(Source), Import, Objects);
        UE_LOG(LogTemp, Display, TEXT("SWORD_IMPORT %d objects=%d"), bImported ? 1 : 0, Objects.Num());
        for (UObject* Object : Objects) if (Object) Save(Object);
        IAssetRegistry::GetChecked().SearchAllAssets(true);
    }
    const TArray<USkeletalMesh*> Meshes = Imported<USkeletalMesh>();
    const TArray<UAnimSequence*> Anims = Imported<UAnimSequence>();
    for (USkeletalMesh* Mesh : Meshes)
        UE_LOG(LogTemp, Display, TEXT("SWORD_MESH %s bones=%d root=%s"), *Mesh->GetPathName(), Mesh->GetRefSkeleton().GetNum(), *Mesh->GetRefSkeleton().GetBoneName(0).ToString());
    for (UAnimSequence* Anim : Anims)
        UE_LOG(LogTemp, Display, TEXT("SWORD_ANIM %s len=%.2f keys=%d"), *Anim->GetName(), Anim->GetPlayLength(), Anim->GetNumberOfSampledKeys());
    if (Meshes.IsEmpty()) { UE_LOG(LogTemp, Error, TEXT("SWORD_IMPORT_FAILED no skeletal mesh")); return 1; }
    if (FParse::Param(*Params, TEXT("ImportOnly"))) return 0;
    if (!ImportProp(TEXT("arrel_prop_sword_drawn.fbx"), TEXT("SM_Arrel_sword_drawn")) || !ImportProp(TEXT("arrel_prop_scabbard.fbx"), TEXT("SM_Arrel_scabbard"))) return 1;
    // The clips Arrel uses, found by the animation's name at the end of the imported asset name.
    TArray<FAssetData> Clips; FString Prefix;
    for (const TCHAR* Name : MemoriaCombatClips::SwordClips())
    {
        UAnimSequence* const* Found = Anims.FindByPredicate([&](const UAnimSequence* A) { return A->GetName().EndsWith(Name); });
        if (!Found) { UE_LOG(LogTemp, Error, TEXT("SWORD_CLIP_MISSING %s"), Name); return 1; }
        Clips.Add(FAssetData(*Found));
        Prefix = (*Found)->GetName().LeftChop(FCString::Strlen(Name));
    }
    USkeletalMesh* Source = Meshes[0];
    auto* Target = Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SK_Arrel"));
    if (!Target) { UE_LOG(LogTemp, Error, TEXT("SWORD_TARGET_MISSING Arrel")); return 1; }
    if (Load<UAnimSequence>(MemoriaCombatClips::ClipPath(TEXT("Arrel"), MemoriaCombatClips::SwordClips()[0])))
    { UE_LOG(LogTemp, Display, TEXT("SWORD_KEPT Arrel")); return 0; }
    FAutoCharacterizeResults SourceResults, TargetResults;
    UIKRigDefinition* SourceRig = MakeRig(TEXT("IK_UAL2"), Source, SourceResults);
    // Arrel gets his own rig here, so the mannequin set's rig and retargeter stay untouched.
    UIKRigDefinition* TargetRig = MakeRig(TEXT("IK_Arrel_Sword"), Target, TargetResults);
    if (!SourceRig || !TargetRig) return 1;
    UIKRetargeter* Retargeter = MakeRetargeter(TEXT("RTG_UAL2_Arrel"), SourceRig, SourceResults, TargetRig, TargetResults);
    if (!Save(Retargeter)) return 1;
    FIKRetargetBatchOperationInputs Inputs;
    Inputs.AssetsToRetarget = Clips; Inputs.SourceMesh = Source; Inputs.TargetMesh = Target; Inputs.IKRetargetAsset = Retargeter;
    Inputs.TargetPath = TEXT("/Game/Memoria/Presentation/Field3D/Arrel/Combat");
    Inputs.Search = Prefix; Inputs.Replace = TEXT(""); Inputs.Prefix = TEXT("A_Arrel_");
    Inputs.bIncludeReferencedAssets = false; Inputs.bOverwriteExistingFiles = true;
    int32 Made = 0;
    for (const FAssetData& Asset : UIKRetargetBatchOperation::RunBatchRetarget(Inputs))
        if (UObject* Object = Asset.GetAsset()) { if (!Save(Object)) return 1; ++Made; }
    UE_LOG(LogTemp, Display, TEXT("SWORD_RETARGET_DONE %d prefix='%s'"), Made, *Prefix);
    return Made == Clips.Num() ? 0 : 1;
}
