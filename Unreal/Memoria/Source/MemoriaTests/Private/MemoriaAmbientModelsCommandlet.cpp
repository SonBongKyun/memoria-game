#include "MemoriaAmbientModelsCommandlet.h"
#include "MemoriaRetargetKit.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/TextureFactory.h"
#include "AssetImportTask.h"
#include "Animation/AnimSequence.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "SkinnedAssetCompiler.h"
#include "StaticMeshCompiler.h"
#include "TextureCompiler.h"
#include "Misc/Paths.h"
using namespace MemoriaRetargetKit;
namespace
{
// Source is Codex's folder name; Name is what the field figure title-cases a preset into ("bureau_agent" ->
// "bureauagent" -> "Bureauagent").
struct FAmbientNpc { const TCHAR* Source; const TCHAR* Name; };
const FAmbientNpc Npcs[] = {{TEXT("npc_traveler"), TEXT("Traveler")}, {TEXT("npc_bureau_agent"), TEXT("Bureauagent")}, {TEXT("npc_guard"), TEXT("Guard")},
    // S348: Codex's S347 townsfolk for Verdan's market and Drift's revisit.
    {TEXT("npc_villager_f"), TEXT("Villagerf")}, {TEXT("npc_villager_m"), TEXT("Villagerm")}, {TEXT("npc_fisherman"), TEXT("Fisherman")},
    {TEXT("npc_elder"), TEXT("Elder")}, {TEXT("npc_child"), TEXT("Child")}, {TEXT("npc_scholar"), TEXT("Scholar")}};
struct FAmbientProp { const TCHAR* Source; const TCHAR* Name; };
const FAmbientProp Props[] = {{TEXT("prop_water_tank"), TEXT("WaterTank")}, {TEXT("prop_campfire"), TEXT("Campfire")}, {TEXT("prop_rubble"), TEXT("Rubble")}};
FString Base(const TCHAR* Name) { return FString::Printf(TEXT("/Game/Memoria/Presentation/Field3D/%s/"), Name); }
UObject* ImportFbx(const FString& File, const FString& Path, bool bStatic)
{
    if (auto* Existing = Load<UObject>(Path)) return Existing;
    auto* Factory = NewObject<UFbxFactory>(); Factory->SetDetectImportTypeOnImport(false);
    auto* UI = Factory->ImportUI.Get();
    const EFBXImportType Type = bStatic ? FBXIT_StaticMesh : FBXIT_SkeletalMesh;
    UI->MeshTypeToImport = Type; UI->OriginalImportType = Type; UI->bAutomatedImportShouldDetectType = false;
    UI->bImportAsSkeletal = !bStatic; UI->bImportMesh = true; UI->bImportAnimations = false; UI->bCreatePhysicsAsset = false;
    UI->bImportMaterials = false; UI->bImportTextures = false; UI->bOverrideFullName = true;
    // The S310 basis: -Y forward, Z up, cm; the same rotation as Arrel, Elia and Malet.
    for (UFbxAssetImportData* Data : {static_cast<UFbxAssetImportData*>(UI->SkeletalMeshImportData.Get()), static_cast<UFbxAssetImportData*>(UI->StaticMeshImportData.Get())})
    { Data->bConvertScene = true; Data->bForceFrontXAxis = true; Data->bConvertSceneUnit = true; Data->ImportRotation = FRotator(0, -90, 0); }
    UI->SkeletalMeshImportData->bUseT0AsRefPose = false; UI->SkeletalMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    UI->StaticMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents; UI->StaticMeshImportData->bAutoGenerateCollision = false;
    UI->StaticMeshImportData->bCombineMeshes = true;
    auto* Task = NewObject<UAssetImportTask>(); Task->bAutomated = true; Task->Options = UI; Factory->SetAssetImportTask(Task);
    bool bCancelled = false;
    return Factory->ImportObject(bStatic ? UStaticMesh::StaticClass() : USkeletalMesh::StaticClass(), CreatePackage(*Path), *FPackageName::GetShortName(Path),
        RF_Public | RF_Standalone, File, nullptr, bCancelled);
}
UTexture2D* ImportTexture(const FString& File, const FString& Path)
{
    if (auto* Existing = Load<UTexture2D>(Path)) return Existing;
    auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
    auto* T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*Path), *FPackageName::GetShortName(Path),
        RF_Public | RF_Standalone, File, nullptr, GWarn, bCancelled));
    if (!T || bCancelled) return nullptr;
    T->LODGroup = TEXTUREGROUP_Character; T->SRGB = true; T->NeverStream = true; T->Filter = TF_Trilinear;
    T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
    return Save(T) ? T : nullptr;
}
template<class T> T* Node(UMaterial* M) { auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
// The leads' S310 material: the painted colour, lit, with a small neutral fill that keeps its midtones. The
// props keep the fill too: without it the water tank's shadowed side goes black (tried and reverted in S335).
UMaterial* MakeMaterial(const FString& Path, UTexture2D* Color, bool bSkeletal)
{
    if (auto* Existing = Load<UMaterial>(Path)) return Existing;
    auto* M = NewObject<UMaterial>(CreatePackage(*Path), *FPackageName::GetShortName(Path), RF_Public | RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit); M->bUsedWithSkeletalMesh = bSkeletal; M->TwoSided = true;
    auto* Sample = Node<UMaterialExpressionTextureSample>(M); Sample->Texture = Color;
    M->GetEditorOnlyData()->BaseColor.Connect(0, Sample);
    auto* Rough = Node<UMaterialExpressionConstant>(M); Rough->R = .8f; M->GetEditorOnlyData()->Roughness.Connect(0, Rough);
    auto* Fill = Node<UMaterialExpressionMultiply>(M); Fill->A.Connect(0, Sample); Fill->ConstB = .10f;
    M->GetEditorOnlyData()->EmissiveColor.Connect(0, Fill);
    M->PostEditChange();
    return Save(M) ? M : nullptr;
}
// One clip retargeted from the mannequin into the NPC's folder under the name the field figure loads.
bool Retarget(const FAssetData& Clip, USkeletalMesh* Source, USkeletalMesh* Target, UIKRetargeter* Retargeter, const TCHAR* Name, const TCHAR* Search, const TCHAR* Replace)
{
    FIKRetargetBatchOperationInputs Inputs;
    Inputs.AssetsToRetarget = {Clip}; Inputs.SourceMesh = Source; Inputs.TargetMesh = Target; Inputs.IKRetargetAsset = Retargeter;
    Inputs.TargetPath = Base(Name).LeftChop(1);
    Inputs.Search = Search; Inputs.Replace = Replace; Inputs.Prefix = FString::Printf(TEXT("A_%s_"), Name);
    Inputs.bIncludeReferencedAssets = false; Inputs.bOverwriteExistingFiles = true;
    int32 Made = 0;
    for (const FAssetData& Asset : UIKRetargetBatchOperation::RunBatchRetarget(Inputs))
        if (UObject* Object = Asset.GetAsset()) { UE_LOG(LogTemp, Display, TEXT("AMBIENT_MODEL_CLIP %s"), *Object->GetPathName()); if (!Save(Object)) return false; ++Made; }
    return Made == 1;
}
}
UMemoriaAmbientModelsCommandlet::UMemoriaAmbientModelsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaAmbientModelsCommandlet::Main(const FString& Params)
{
    const FString Characters = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/FieldCharacters"));
    const FString PropArt = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/FieldProps"));
    // The props: one static mesh, one basecolour and one lit material each. Their origin is on the ground at the centre.
    for (const FAmbientProp& Prop : Props)
    {
        const FString B = Base(TEXT("Props")), MeshPath = B + TEXT("SM_") + Prop.Name;
        if (Load<UStaticMesh>(MeshPath)) { UE_LOG(LogTemp, Display, TEXT("AMBIENT_MODEL_KEPT %s"), Prop.Name); continue; }
        UTexture2D* Color = ImportTexture(PropArt / (FString(Prop.Source) + TEXT("_basecolor.png")), B + TEXT("T_") + Prop.Name);
        UMaterial* Material = Color ? MakeMaterial(B + TEXT("M_") + Prop.Name, Color, false) : nullptr;
        auto* Mesh = Cast<UStaticMesh>(ImportFbx(PropArt / (FString(Prop.Source) + TEXT(".fbx")), MeshPath, true));
        if (!Material || !Mesh) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_IMPORT_FAILED %s"), Prop.Name); return 1; }
        FStaticMeshCompilingManager::Get().FinishCompilation({Mesh}); Mesh->SetMaterial(0, Material);
        if (!Save(Mesh)) return 1;
        const FVector Size = Mesh->GetBounds().BoxExtent * 2;
        UE_LOG(LogTemp, Display, TEXT("AMBIENT_MODEL_IMPORTED %s size=%.1fx%.1fx%.1f"), Prop.Name, Size.X, Size.Y, Size.Z);
    }
    // The NPCs.
    auto* Mannequin = Load<USkeletalMesh>(MemoriaCombatClips::MannequinMesh());
    if (!Mannequin) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_MANNEQUIN_MISSING run Unreal/Tools/install_mannequin.py")); return 1; }
    auto Clip = [](const FString& Path) { auto* S = Load<UAnimSequence>(Path); return S ? FAssetData(S) : FAssetData(); };
    const FAssetData Idle = Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")), Walk = Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd"));
    if (!Idle.IsValid() || !Walk.IsValid()) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_CLIP_MISSING")); return 1; }
    UIKRigDefinition* SourceRig = nullptr; FAutoCharacterizeResults SourceResults;
    for (const FAmbientNpc& Npc : Npcs)
    {
        const FString Dir = Characters / Npc.Source, B = Base(Npc.Name);
        if (Load<USkeletalMesh>(B + TEXT("SK_") + Npc.Name) && Load<UAnimSequence>(B + TEXT("A_") + Npc.Name + TEXT("_Idle")) && Load<UAnimSequence>(B + TEXT("A_") + Npc.Name + TEXT("_Walk")))
        { UE_LOG(LogTemp, Display, TEXT("AMBIENT_MODEL_KEPT %s"), Npc.Name); continue; }
        UTexture2D* Color = ImportTexture(Dir / (FString(Npc.Source) + TEXT("_basecolor.png")), B + TEXT("T_") + Npc.Name);
        UMaterial* Material = Color ? MakeMaterial(B + TEXT("M_") + Npc.Name, Color, true) : nullptr;
        auto* Mesh = Cast<USkeletalMesh>(ImportFbx(Dir / (FString(Npc.Source) + TEXT("_rigged.fbx")), B + TEXT("SK_") + Npc.Name, false));
        if (!Material || !Mesh || !Mesh->GetSkeleton()) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_IMPORT_FAILED %s"), Npc.Name); return 1; }
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (Mesh->GetRefSkeleton().GetNum() != 77) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_BONES %s %d"), Npc.Name, Mesh->GetRefSkeleton().GetNum()); return 1; }
        for (auto& Slot : Mesh->GetMaterials()) Slot.MaterialInterface = Material;
        Mesh->PostEditChange(); FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!Save(Mesh->GetSkeleton()) || !Save(Mesh)) return 1;
        if (!SourceRig)
        {
            Remove(FString(CombatRoot) + TEXT("IK_Mannequin_Ambient"));
            SourceRig = MakeRig(TEXT("IK_Mannequin_Ambient"), Mannequin, SourceResults);
            if (!SourceRig) return 1;
        }
        FAutoCharacterizeResults TargetResults;
        UIKRigDefinition* TargetRig = MakeRig(FString(TEXT("IK_")) + Npc.Name, Mesh, TargetResults);
        if (!TargetRig) return 1;
        UIKRetargeter* Retargeter = MakeRetargeter(FString(TEXT("RTG_Mannequin_")) + Npc.Name, SourceRig, SourceResults, TargetRig, TargetResults);
        if (!Save(Retargeter)) return 1;
        if (!Retarget(Idle, Mannequin, Mesh, Retargeter, Npc.Name, TEXT("MM_Idle"), TEXT("Idle"))
            || !Retarget(Walk, Mannequin, Mesh, Retargeter, Npc.Name, TEXT("MF_Unarmed_Walk_Fwd"), TEXT("Walk"))) { UE_LOG(LogTemp, Error, TEXT("AMBIENT_MODEL_RETARGET_FAILED %s"), Npc.Name); return 1; }
        UE_LOG(LogTemp, Display, TEXT("AMBIENT_MODEL_IMPORTED %s height=%.1f"), Npc.Name, Mesh->GetBounds().BoxExtent.Z * 2);
    }
    return 0;
}
