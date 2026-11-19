#include "MemoriaAfterimageAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
namespace
{
const TCHAR* AfterimagePackage = TEXT("/Game/Memoria/Presentation/Combat/M_Afterimage");
template<class T> T* Expression(UMaterial* Material)
{
    auto* Node = NewObject<T>(Material);
    Material->GetExpressionCollection().AddExpression(Node);
    return Node;
}
}
UMemoriaAfterimageAssetsCommandlet::UMemoriaAfterimageAssetsCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UMemoriaAfterimageAssetsCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(AfterimagePackage))
    { UE_LOG(LogTemp, Error, TEXT("Refusing existing presentation package %s"), AfterimagePackage); return 1; }
    auto* Material = NewObject<UMaterial>(CreatePackage(AfterimagePackage), TEXT("M_Afterimage"), RF_Public | RF_Standalone);
    // A ghost: unlit and see-through, brightest at its silhouette's edge so the pose reads without hiding the ground.
    Material->BlendMode = BLEND_Translucent; Material->SetShadingModel(MSM_Unlit); Material->TwoSided = false;
    Material->SetMaterialUsage(MATUSAGE_SkeletalMesh);
    auto* Rim = Expression<UMaterialExpressionFresnel>(Material); Rim->Exponent = 2.6f; Rim->BaseReflectFraction = .06f;
    auto* Tint = Expression<UMaterialExpressionVectorParameter>(Material); Tint->ParameterName = TEXT("Tint"); Tint->DefaultValue = FLinearColor(.62f, .74f, 1.f);
    auto* Alpha = Expression<UMaterialExpressionScalarParameter>(Material); Alpha->ParameterName = TEXT("Alpha"); Alpha->DefaultValue = .2f;
    auto* Glow = Expression<UMaterialExpressionCustom>(Material);
    Glow->Code = TEXT("return Tint * (0.55 + 1.9 * Rim);"); Glow->OutputType = CMOT_Float3;
    FCustomInput GlowTint; GlowTint.InputName = TEXT("Tint"); GlowTint.Input.Connect(0, Tint);
    FCustomInput GlowRim; GlowRim.InputName = TEXT("Rim"); GlowRim.Input.Connect(0, Rim);
    Glow->Inputs = {GlowTint, GlowRim};
    auto* Fade = Expression<UMaterialExpressionCustom>(Material);
    Fade->Code = TEXT("return saturate(Alpha * (0.3 + 0.7 * Rim));"); Fade->OutputType = CMOT_Float1;
    FCustomInput FadeAlpha; FadeAlpha.InputName = TEXT("Alpha"); FadeAlpha.Input.Connect(0, Alpha);
    FCustomInput FadeRim; FadeRim.InputName = TEXT("Rim"); FadeRim.Input.Connect(0, Rim);
    Fade->Inputs = {FadeAlpha, FadeRim};
    Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Glow);
    Material->GetEditorOnlyData()->Opacity.Connect(0, Fade);
    Material->PostEditChange();
    FAssetRegistryModule::AssetCreated(Material);
    Material->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString File = FPackageName::LongPackageNameToFilename(AfterimagePackage, FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Material->GetOutermost(), Material, *File, Args);
    UE_LOG(LogTemp, Display, TEXT("AFTERIMAGE_ASSET %s %s"), *Material->GetPathName(), bSaved ? TEXT("SAVED") : TEXT("FAILED"));
    return bSaved ? 0 : 1;
}
