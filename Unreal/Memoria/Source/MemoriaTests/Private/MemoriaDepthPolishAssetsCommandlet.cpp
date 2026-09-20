#include "MemoriaDepthPolishAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "TextureCompiler.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
namespace
{
const FString Base=TEXT("/Game/Memoria/Presentation/Depth2/");
template<class T> T* Node(UMaterial* M)
{ auto* N=NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
void Input(UMaterialExpressionCustom* N,const TCHAR* Name,UMaterialExpression* Value)
{ FCustomInput I; I.InputName=Name; I.Input.Connect(0,Value); N->Inputs.Add(I); }
UMaterialExpressionVectorParameter* Vector(UMaterial* M,const TCHAR* Name)
{ auto* V=Node<UMaterialExpressionVectorParameter>(M); V->ParameterName=Name; V->DefaultValue=FLinearColor::Black; return V; }
bool Save(UObject* Object)
{
    FAssetRegistryModule::AssetCreated(Object); Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    const FString File=FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    const bool OK=UPackage::SavePackage(Object->GetOutermost(),Object,*File,Args);
    UE_LOG(LogTemp,Display,TEXT("DEPTH2_ASSET %s %s"),*Object->GetPathName(),OK?TEXT("SAVED"):TEXT("FAILED")); return OK;
}
}
UMemoriaDepthPolishAssetsCommandlet::UMemoriaDepthPolishAssetsCommandlet()
{ IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaDepthPolishAssetsCommandlet::Main(const FString&)
{
    for (const TCHAR* N:{TEXT("M_FocusSurface"),TEXT("T_MemoryLantern"),TEXT("SPR_MemoryLantern")})
        if (FPackageName::DoesPackageExist(Base+N)) { UE_LOG(LogTemp,Error,TEXT("Refusing existing Depth2 package %s"),N); return 1; }
    auto* Source=LoadObject<UMaterial>(nullptr,TEXT("/Game/Memoria/Presentation/Depth/M_Surface.M_Surface"));
    auto* Dither=LoadObject<UMaterialFunctionInterface>(nullptr,TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA"));
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../../assets/environment/hybrid_depth/motif_memory_lantern_v1.png"));
    if (!Source || !Dither || !FPaths::FileExists(File)) return 1;
    // Duplicate into a new package; the protected Depth1 material is never edited.
    auto* M=DuplicateObject<UMaterial>(Source,CreatePackage(*(Base+TEXT("M_FocusSurface"))),TEXT("M_FocusSurface"));
    M->SetFlags(RF_Public|RF_Standalone); M->BlendMode=BLEND_Masked; M->OpacityMaskClipValue=0.333f;
    auto* Mask=Node<UMaterialExpressionCustom>(M); Mask->Inputs.Reset(); Mask->OutputType=CMOT_Float1;
    Mask->Code=TEXT(R"(
float3 ray=Focus.xyz-Eye.xyz;
float t=dot(P-Eye.xyz,ray)/max(dot(ray,ray),1.0);
float3 delta=P-(Eye.xyz+t*ray);
float2 offset=float2(delta.x,dot(delta,Up.xyz))/(float2(48,80)*max(t,0.2));
float hole=smoothstep(0.68,1.0,dot(offset,offset));
float alpha=(t>0.0 && t<1.0 && P.z>-5.0)?hole:1.0;
return lerp(1.0,alpha,saturate(Strength));
)");
    Input(Mask,TEXT("P"),Node<UMaterialExpressionWorldPosition>(M));
    Input(Mask,TEXT("Eye"),Vector(M,TEXT("OcclusionEye")));
    Input(Mask,TEXT("Focus"),Vector(M,TEXT("OcclusionFocus")));
    Input(Mask,TEXT("Up"),Vector(M,TEXT("OcclusionUp")));
    auto* Strength=Node<UMaterialExpressionScalarParameter>(M); Strength->ParameterName=TEXT("FocusStrength"); Strength->DefaultValue=1;
    Input(Mask,TEXT("Strength"),Strength);
    auto* Fade=Node<UMaterialExpressionMaterialFunctionCall>(M);
    if (!Fade->SetMaterialFunction(Dither)) return 1;
    bool Linked=false;
    for (int32 I=0; I<Fade->FunctionInputs.Num(); ++I)
        if (Fade->GetInputName(I).ToString().Contains(TEXT("Alpha"))) { Fade->FunctionInputs[I].Input.Connect(0,Mask); Linked=true; }
    if (!Linked) { UE_LOG(LogTemp,Error,TEXT("Dither alpha input missing")); return 1; }
    M->GetEditorOnlyData()->OpacityMask.Connect(0,Fade); M->PostEditChange(); if (!Save(M)) return 1;
    auto* Factory=NewObject<UTextureFactory>(); bool Cancelled=false;
    auto* T=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),CreatePackage(*(Base+TEXT("T_MemoryLantern"))),TEXT("T_MemoryLantern"),RF_Public|RF_Standalone,*File,nullptr,GWarn,Cancelled));
    if (!T || Cancelled) return 1;
    T->LODGroup=TEXTUREGROUP_World; T->CompressionSettings=TC_EditorIcon; T->Filter=TF_Trilinear;
    T->SRGB=true; T->NeverStream=true; T->PostEditChange(); FTextureCompilingManager::Get().FinishCompilation({T});
    if (!Save(T)) return 1;
    auto* S=NewObject<UPaperSprite>(CreatePackage(*(Base+TEXT("SPR_MemoryLantern"))),TEXT("SPR_MemoryLantern"),RF_Public|RF_Standalone);
    FSpriteAssetInitParameters Init; Init.Texture=T; Init.Dimension=FIntPoint(1024,1536); Init.SetPixelsPerUnrealUnit(20);
    S->InitializeSprite(Init); S->SetPivotMode(ESpritePivotMode::Custom,FVector2D(502,1381));
    return Save(S)?0:1;
}
