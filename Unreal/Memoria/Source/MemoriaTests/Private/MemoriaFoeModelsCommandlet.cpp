#include "MemoriaFoeModelsCommandlet.h"
#include "MemoriaRetargetKit.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/TextureFactory.h"
#include "AssetImportTask.h"
#include "Animation/AnimSequence.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "RetargetEditor/IKRetargetBatchOperation.h"
#include "SkinnedAssetCompiler.h"
#include "StaticMeshCompiler.h"
#include "TextureCompiler.h"
#include "Misc/Paths.h"
using namespace MemoriaRetargetKit;
namespace
{
// S326: Codex's S313 models (models/void_husk, models/market_thief), kept in Unreal/ArtSource/FieldCharacters.
struct FFoeModel { const TCHAR* Source; const TCHAR* Name; bool bEmissive; const TCHAR* Prop; FLinearColor Rim; };
// The rim colours follow the foe specs' glow: violet for the husk, lantern-warm for the thief.
const FFoeModel Models[] = {{TEXT("void_husk"), TEXT("Husk"), true, nullptr, FLinearColor(.55f, .18f, 1.f)},
    {TEXT("market_thief"), TEXT("Thief"), false, TEXT("dagger"), FLinearColor(1.f, .62f, .30f)}};
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
UTexture2D* ImportTexture(const FString& File, const FString& Path, bool bSrgb)
{
    if (auto* Existing = Load<UTexture2D>(Path)) return Existing;
    auto* Factory = NewObject<UTextureFactory>(); bool bCancelled = false;
    auto* T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*Path), *FPackageName::GetShortName(Path),
        RF_Public | RF_Standalone, File, nullptr, GWarn, bCancelled));
    if (!T || bCancelled) return nullptr;
    // The emissive mask is data (white = glow): linear, as codex-review.md asks.
    T->LODGroup = TEXTUREGROUP_Character; T->SRGB = bSrgb; T->NeverStream = true; T->Filter = TF_Trilinear;
    if (!bSrgb) T->CompressionSettings = TC_Grayscale;
    T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
    return Save(T) ? T : nullptr;
}
template<class T> T* Node(UMaterial* M) { auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
// The model's body: its painted colour, a small neutral fill (as S310's), a fresnel rim (as M_FieldFoe's, so
// the dark bodies read against the night market), the violet cracks from the mask for the husk (Glow x
// CrackStrength), and the Hit flash.
UMaterial* MakeMaterial(const FString& Path, UTexture2D* Color, UTexture2D* Mask, const FLinearColor& RimColor)
{
    if (auto* Existing = Load<UMaterial>(Path)) return Existing;
    auto* M = NewObject<UMaterial>(CreatePackage(*Path), *FPackageName::GetShortName(Path), RF_Public | RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit); M->bUsedWithSkeletalMesh = true; M->TwoSided = true;
    auto* Sample = Node<UMaterialExpressionTextureSample>(M); Sample->Texture = Color;
    M->GetEditorOnlyData()->BaseColor.Connect(0, Sample);
    auto* Rough = Node<UMaterialExpressionConstant>(M); Rough->R = .8f; M->GetEditorOnlyData()->Roughness.Connect(0, Rough);
    auto* Fill = Node<UMaterialExpressionMultiply>(M); Fill->A.Connect(0, Sample); Fill->ConstB = .10f;
    auto* HitAmount = Node<UMaterialExpressionScalarParameter>(M); HitAmount->ParameterName = TEXT("Hit"); HitAmount->DefaultValue = 0.f;
    auto* HitColor = Node<UMaterialExpressionVectorParameter>(M); HitColor->ParameterName = TEXT("HitColor"); HitColor->DefaultValue = FLinearColor(1.f, .75f, .5f);
    auto* Flash = Node<UMaterialExpressionMultiply>(M); Flash->A.Connect(0, HitColor); Flash->B.Connect(0, HitAmount);
    auto* Lit = Node<UMaterialExpressionAdd>(M); Lit->A.Connect(0, Fill); Lit->B.Connect(0, Flash);
    auto* Fresnel = Node<UMaterialExpressionFresnel>(M); Fresnel->Exponent = 4.f;
    auto* Rim = Node<UMaterialExpressionVectorParameter>(M); Rim->ParameterName = TEXT("RimColor"); Rim->DefaultValue = RimColor;
    auto* RimAmount = Node<UMaterialExpressionScalarParameter>(M); RimAmount->ParameterName = TEXT("RimStrength"); RimAmount->DefaultValue = .12f;   // an edge, not a glow: the painted colour must stay readable
    auto* RimTint = Node<UMaterialExpressionMultiply>(M); RimTint->A.Connect(0, Fresnel); RimTint->B.Connect(0, Rim);
    auto* RimLight = Node<UMaterialExpressionMultiply>(M); RimLight->A.Connect(0, RimTint); RimLight->B.Connect(0, RimAmount);
    auto* Emissive = Node<UMaterialExpressionAdd>(M); Emissive->A.Connect(0, Lit); Emissive->B.Connect(0, RimLight);
    if (Mask)
    {
        auto* MaskSample = Node<UMaterialExpressionTextureSample>(M); MaskSample->Texture = Mask; MaskSample->SamplerType = SAMPLERTYPE_LinearGrayscale;
        auto* Glow = Node<UMaterialExpressionVectorParameter>(M); Glow->ParameterName = TEXT("Glow"); Glow->DefaultValue = FLinearColor(.32f, .008f, .72f);
        auto* Strength = Node<UMaterialExpressionScalarParameter>(M); Strength->ParameterName = TEXT("CrackStrength"); Strength->DefaultValue = 2.5f;
        auto* Tint = Node<UMaterialExpressionMultiply>(M); Tint->A.Connect(0, MaskSample); Tint->B.Connect(0, Glow);
        auto* Cracks = Node<UMaterialExpressionMultiply>(M); Cracks->A.Connect(0, Tint); Cracks->B.Connect(0, Strength);
        auto* WithCracks = Node<UMaterialExpressionAdd>(M); WithCracks->A.Connect(0, Emissive); WithCracks->B.Connect(0, Cracks);
        M->GetEditorOnlyData()->EmissiveColor.Connect(0, WithCracks);
    }
    else M->GetEditorOnlyData()->EmissiveColor.Connect(0, Emissive);
    M->PostEditChange();
    return Save(M) ? M : nullptr;
}
// One batch per naming group: the mannequin foe clips (A_Mannequin_<Clip>), the S311 melee set (MM_<Clip>),
// and the idle/walk pair renamed to <Name>_Idle / <Name>_Walk.
bool Batch(const TArray<FAssetData>& Clips, USkeletalMesh* Source, USkeletalMesh* Target, UIKRetargeter* Retargeter, const TCHAR* Name, const TCHAR* Search, const TCHAR* Replace)
{
    FIKRetargetBatchOperationInputs Inputs;
    Inputs.AssetsToRetarget = Clips; Inputs.SourceMesh = Source; Inputs.TargetMesh = Target; Inputs.IKRetargetAsset = Retargeter;
    Inputs.TargetPath = Base(Name) + TEXT("Combat");
    Inputs.Search = Search; Inputs.Replace = Replace; Inputs.Prefix = FString::Printf(TEXT("A_%s_"), Name);
    Inputs.bIncludeReferencedAssets = false; Inputs.bOverwriteExistingFiles = true;
    int32 Made = 0;
    for (const FAssetData& Asset : UIKRetargetBatchOperation::RunBatchRetarget(Inputs))
        if (UObject* Object = Asset.GetAsset()) { if (!Save(Object)) return false; ++Made; }
    UE_LOG(LogTemp, Display, TEXT("FOE_MODEL_RETARGET %s %s %d/%d"), Name, Search, Made, Clips.Num());
    return Made == Clips.Num();
}
}
UMemoriaFoeModelsCommandlet::UMemoriaFoeModelsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaFoeModelsCommandlet::Main(const FString& Params)
{
    const FString Art = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/FieldCharacters"));
    auto* Mannequin = Load<USkeletalMesh>(MemoriaCombatClips::MannequinMesh());
    if (!Mannequin) { UE_LOG(LogTemp, Error, TEXT("FOE_MODEL_MANNEQUIN_MISSING run Unreal/Tools/install_mannequin.py")); return 1; }
    // The clips the foes play, all on the mannequin skeleton: the UAL2 foe set (-run=MemoriaFoeAssets), the
    // melee set's hit and death, and Epic's unarmed idle and walk for the thief.
    auto Clip = [](const FString& Path) { auto* S = Load<UAnimSequence>(Path); return S ? FAssetData(S) : FAssetData(); };
    TArray<FAssetData> FoeSet, MeleeSet, IdleWalk;
    for (const TCHAR* Name : MemoriaCombatClips::FoeClips()) FoeSet.Add(Clip(MemoriaCombatClips::ClipPath(TEXT("Mannequin"), Name)));
    for (const TCHAR* Name : {MemoriaCombatClips::Hit(), MemoriaCombatClips::Death()}) MeleeSet.Add(Clip(MemoriaCombatClips::ClipPath(TEXT("Mannequin"), Name)));
    IdleWalk.Add(Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")));
    IdleWalk.Add(Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd")));
    for (const auto* Set : {&FoeSet, &MeleeSet, &IdleWalk})
        for (const FAssetData& A : *Set) if (!A.IsValid()) { UE_LOG(LogTemp, Error, TEXT("FOE_MODEL_CLIP_MISSING (run -run=MemoriaFoeAssets first)")); return 1; }
    FAutoCharacterizeResults SourceResults;
    Remove(FString(CombatRoot) + TEXT("IK_Mannequin_FoeModels"));
    UIKRigDefinition* SourceRig = MakeRig(TEXT("IK_Mannequin_FoeModels"), Mannequin, SourceResults);
    if (!SourceRig) return 1;
    for (const FFoeModel& Model : Models)
    {
        const FString Dir = Art / Model.Source, B = Base(Model.Name);
        UTexture2D* Color = ImportTexture(Dir / (FString(Model.Source) + TEXT("_basecolor.png")), B + TEXT("T_") + Model.Name, true);
        UTexture2D* Mask = Model.bEmissive ? ImportTexture(Dir / (FString(Model.Source) + TEXT("_emissive.png")), B + TEXT("T_") + Model.Name + TEXT("_Emissive"), false) : nullptr;
        const bool bFreshMaterial = !Load<UMaterial>(B + TEXT("M_") + Model.Name);
        UMaterial* Material = Color && (!Model.bEmissive || Mask) ? MakeMaterial(B + TEXT("M_") + Model.Name, Color, Mask, Model.Rim) : nullptr;
        if (auto* Kept = Load<USkeletalMesh>(B + TEXT("SK_") + Model.Name); Kept && Load<UAnimSequence>(MemoriaCombatClips::ClipPath(Model.Name, TEXT("Walk"))))
        {
            // Imported before: a material rebuilt from a deleted asset is put back on the body and the prop.
            if (Material && bFreshMaterial)
            {
                for (auto& Slot : Kept->GetMaterials()) Slot.MaterialInterface = Material;
                Kept->PostEditChange(); FSkinnedAssetCompilingManager::Get().FinishCompilation({Kept});
                if (!Save(Kept)) return 1;
                if (Model.Prop) if (auto* Prop = Load<UStaticMesh>(B + TEXT("SM_") + Model.Name + TEXT("_") + Model.Prop)) { Prop->SetMaterial(0, Material); if (!Save(Prop)) return 1; }
            }
            UE_LOG(LogTemp, Display, TEXT("FOE_MODEL_KEPT %s material=%s"), Model.Name, bFreshMaterial ? TEXT("rebuilt") : TEXT("kept"));
            continue;
        }
        auto* Mesh = Cast<USkeletalMesh>(ImportFbx(Dir / (FString(Model.Source) + TEXT("_rigged.fbx")), B + TEXT("SK_") + Model.Name, false));
        if (!Material || !Mesh || !Mesh->GetSkeleton()) { UE_LOG(LogTemp, Error, TEXT("FOE_MODEL_IMPORT_FAILED %s"), Model.Name); return 1; }
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (Mesh->GetRefSkeleton().GetNum() != 77) { UE_LOG(LogTemp, Error, TEXT("FOE_MODEL_BONES %s %d"), Model.Name, Mesh->GetRefSkeleton().GetNum()); return 1; }
        for (auto& Slot : Mesh->GetMaterials()) Slot.MaterialInterface = Material;
        Mesh->PostEditChange(); FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!Save(Mesh->GetSkeleton()) || !Save(Mesh)) return 1;
        if (Model.Prop)
        {
            // The thief's dagger shares the body atlas; its grip is at the origin, blade up +Z.
            auto* Prop = Cast<UStaticMesh>(ImportFbx(Dir / FString::Printf(TEXT("%s_prop_%s.fbx"), Model.Source, Model.Prop), B + TEXT("SM_") + Model.Name + TEXT("_") + Model.Prop, true));
            if (!Prop) return 1;
            FStaticMeshCompilingManager::Get().FinishCompilation({Prop}); Prop->SetMaterial(0, Material);
            if (!Save(Prop)) return 1;
        }
        FAutoCharacterizeResults TargetResults;
        UIKRigDefinition* TargetRig = MakeRig(FString(TEXT("IK_")) + Model.Name, Mesh, TargetResults);
        if (!TargetRig) return 1;
        UIKRetargeter* Retargeter = MakeRetargeter(FString(TEXT("RTG_Mannequin_")) + Model.Name, SourceRig, SourceResults, TargetRig, TargetResults);
        if (!Save(Retargeter)) return 1;
        if (!Batch(FoeSet, Mannequin, Mesh, Retargeter, Model.Name, TEXT("A_Mannequin_"), TEXT(""))
            || !Batch(MeleeSet, Mannequin, Mesh, Retargeter, Model.Name, TEXT("MM_"), TEXT(""))
            || !Batch({IdleWalk[0]}, Mannequin, Mesh, Retargeter, Model.Name, TEXT("MM_Idle"), TEXT("Idle"))
            || !Batch({IdleWalk[1]}, Mannequin, Mesh, Retargeter, Model.Name, TEXT("MF_Unarmed_Walk_Fwd"), TEXT("Walk"))) return 1;
        UE_LOG(LogTemp, Display, TEXT("FOE_MODEL_IMPORTED %s height=%.1f"), Model.Name, Mesh->GetBounds().BoxExtent.Z * 2);
    }
    return 0;
}
