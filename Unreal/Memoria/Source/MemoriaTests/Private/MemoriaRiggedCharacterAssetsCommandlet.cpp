#include "MemoriaRiggedCharacterAssetsCommandlet.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/TextureFactory.h"
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "TextureCompiler.h"
#include "SkinnedAssetCompiler.h"
#include "StaticMeshCompiler.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
namespace
{
bool Save(UObject* Object)
{
    if (!Object) return false;
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const bool OK = UPackage::SavePackage(Object->GetOutermost(), Object,
        *FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
    UE_LOG(LogTemp, Display, TEXT("FIELD3D_SAVE %s %d"), *Object->GetPathName(), OK);
    return OK;
}
template<class T> T* Existing(const FString& Path)
{ return LoadObject<T>(nullptr, *(Path + TEXT(".") + FPaths::GetBaseFilename(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet); }
UObject* ImportFbx(const FString& File, const FString& Path, EFBXImportType Type, USkeleton* Skeleton = nullptr)
{
    if (auto* Asset = Existing<UObject>(Path)) return Asset;
    auto* Factory = NewObject<UFbxFactory>();
    Factory->SetDetectImportTypeOnImport(false);
    auto* UI = Factory->ImportUI.Get();
    UI->MeshTypeToImport = Type; UI->OriginalImportType = Type;
    UI->bAutomatedImportShouldDetectType = false;
    UI->bImportAsSkeletal = Type != FBXIT_StaticMesh;
    UI->bImportMesh = Type != FBXIT_Animation;
    UI->bImportAnimations = Type == FBXIT_Animation;
    UI->bCreatePhysicsAsset = false; UI->Skeleton = Skeleton;
    UI->bImportMaterials = false; UI->bImportTextures = false; UI->bOverrideFullName = true;
    UI->OverrideAnimationName = FPaths::GetBaseFilename(Path);
    for (UFbxAssetImportData* Data : {static_cast<UFbxAssetImportData*>(UI->SkeletalMeshImportData.Get()),
        static_cast<UFbxAssetImportData*>(UI->StaticMeshImportData.Get()), static_cast<UFbxAssetImportData*>(UI->AnimSequenceImportData.Get())})
    { Data->bConvertScene = true; Data->bForceFrontXAxis = true; Data->bConvertSceneUnit = true; Data->ImportRotation = FRotator(0, -90, 0); }
    UI->SkeletalMeshImportData->bUseT0AsRefPose = false;
    UI->SkeletalMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    UI->StaticMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    UI->StaticMeshImportData->bAutoGenerateCollision = false;
    UI->StaticMeshImportData->bCombineMeshes = true;
    UI->AnimSequenceImportData->bUseDefaultSampleRate = true;
    UI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
    auto* Task = NewObject<UAssetImportTask>(); Task->bAutomated = true; Task->Options = UI;
    Factory->SetAssetImportTask(Task);
    bool Cancelled = false;
    UClass* Class = Type == FBXIT_StaticMesh ? UStaticMesh::StaticClass() :
        (Type == FBXIT_Animation ? UAnimSequence::StaticClass() : USkeletalMesh::StaticClass());
    return Factory->ImportObject(Class, CreatePackage(*Path), *FPaths::GetBaseFilename(Path),
        RF_Public | RF_Standalone, File, nullptr, Cancelled);
}
UMaterial* Material(const FString& Source, const FString& Base, const FString& Suffix)
{
    const FString MPath = Base + TEXT("M_") + Suffix, TPath = Base + TEXT("T_") + Suffix;
    if (auto* M = Existing<UMaterial>(MPath)) return M;
    auto* Factory = NewObject<UTextureFactory>(); bool Cancelled = false;
    auto* T = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*TPath),
        *FPaths::GetBaseFilename(TPath), RF_Public | RF_Standalone, Source, nullptr, GWarn, Cancelled));
    if (!T || Cancelled) return nullptr;
    T->LODGroup = TEXTUREGROUP_Character; T->SRGB = true; T->NeverStream = true;
    T->Filter = TF_Trilinear; T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
    if (!Save(T)) return nullptr;
    auto* M = NewObject<UMaterial>(CreatePackage(*MPath), *FPaths::GetBaseFilename(MPath), RF_Public | RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit); M->bUsedWithSkeletalMesh = true; M->TwoSided = true;
    auto* Sample = NewObject<UMaterialExpressionTextureSample>(M); Sample->Texture = T;
    M->GetExpressionCollection().AddExpression(Sample); M->GetEditorOnlyData()->BaseColor.Expression = Sample;
    auto* Rough = NewObject<UMaterialExpressionConstant>(M); Rough->R = .8f;
    M->GetExpressionCollection().AddExpression(Rough); M->GetEditorOnlyData()->Roughness.Expression = Rough;
    // A small neutral fill preserves the painted midtones in the night market.
    auto* Glow = NewObject<UMaterialExpressionMultiply>(M); Glow->A.Expression = Sample; Glow->ConstB = .10f;
    M->GetExpressionCollection().AddExpression(Glow); M->GetEditorOnlyData()->EmissiveColor.Expression = Glow;
    M->PostEditChange();
    return Save(M) ? M : nullptr;
}
}
UMemoriaRiggedCharacterAssetsCommandlet::UMemoriaRiggedCharacterAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaRiggedCharacterAssetsCommandlet::Main(const FString& Params)
{
    const FString Source = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../ArtSource/FieldCharacters"));
    for (const FString Id : {TEXT("arrel"), TEXT("elia"), TEXT("malet")})
    {
        const FString Name = Id.Left(1).ToUpper() + Id.Mid(1);
        const FString Base = TEXT("/Game/Memoria/Presentation/Field3D/") + Name + TEXT("/");
        UMaterial* BodyMat = Material(Source / Id / (Id + TEXT("_basecolor.png")), Base, Name);
        if (!BodyMat) return 1;
        auto* Mesh = Cast<USkeletalMesh>(ImportFbx(Source / Id / (Id + TEXT("_rigged.fbx")), Base + TEXT("SK_") + Name, FBXIT_SkeletalMesh));
        if (!Mesh || !Mesh->GetSkeleton()) return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (Mesh->GetRefSkeleton().GetNum() != 77 || Mesh->GetMaterials().Num() != 1)
        { UE_LOG(LogTemp, Error, TEXT("FIELD3D unexpected skeleton/materials for %s"), *Id); return 1; }
        Mesh->GetMaterials()[0].MaterialInterface = BodyMat; Mesh->PostEditChange();
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!Save(Mesh->GetSkeleton()) || !Save(Mesh)) return 1;
        for (const FString Clip : {TEXT("Idle"), TEXT("Walk")})
        {
            auto* Anim = Cast<UAnimSequence>(ImportFbx(Source / Id / (Id + TEXT("_") + Clip.ToLower() + TEXT(".fbx")),
                Base + TEXT("A_") + Name + TEXT("_") + Clip, FBXIT_Animation, Mesh->GetSkeleton()));
            if (!Anim || !Save(Anim)) return 1;
            UE_LOG(LogTemp, Display, TEXT("FIELD3D_CLIP %s duration=%.3f"), *Anim->GetPathName(), Anim->GetPlayLength());
        }
        TArray<FString> Props;
        if (Id == TEXT("arrel")) Props = {TEXT("sword_sheathed")};
        else if (Id == TEXT("elia")) Props = {TEXT("staff")};
        else Props = {TEXT("ledger"), TEXT("vials")};
        UMaterial* PropMat = Id == TEXT("arrel") ? BodyMat : Material(Source / Id / (Id + TEXT("_props_basecolor.png")), Base, Name + TEXT("Props"));
        if (!PropMat) return 1;
        for (const FString& Prop : Props)
        {
            auto* Static = Cast<UStaticMesh>(ImportFbx(Source / Id / (Id + TEXT("_prop_") + Prop + TEXT(".fbx")),
                Base + TEXT("SM_") + Name + TEXT("_") + Prop, FBXIT_StaticMesh));
            if (!Static) return 1;
            FStaticMeshCompilingManager::Get().FinishCompilation({Static});
            Static->SetMaterial(0, PropMat); if (!Save(Static)) return 1;
        }
        UE_LOG(LogTemp, Display, TEXT("FIELD3D_IMPORTED %s bones=%d height=%.2f"), *Id, Mesh->GetRefSkeleton().GetNum(), Mesh->GetBounds().BoxExtent.Z * 2);
    }
    return 0;
}

