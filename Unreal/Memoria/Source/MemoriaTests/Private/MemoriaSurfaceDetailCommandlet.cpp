#include "MemoriaSurfaceDetailCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionLocalPosition.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "TextureCompiler.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "RHI.h"
namespace
{
struct FProfile { const TCHAR* Source; const TCHAR* Name; };
const FProfile Profiles[] = {
    {TEXT("hero_detail"),TEXT("HeroDetail")}, {TEXT("ambient_detail"),TEXT("AmbientDetail")},
    {TEXT("husk_detail"),TEXT("HuskDetail")}, {TEXT("thief_detail"),TEXT("ThiefDetail")}};
struct FBody { const TCHAR* Name; int32 Profile; bool bHero; bool bHusk; };
const FBody Bodies[] = {
    {TEXT("Arrel"),0,true,false},{TEXT("Elia"),0,true,false},{TEXT("Malet"),0,true,false},
    {TEXT("Traveler"),1,false,false},{TEXT("Bureauagent"),1,false,false},{TEXT("Guard"),1,false,false},
    {TEXT("Villagerf"),1,false,false},{TEXT("Villagerm"),1,false,false},{TEXT("Fisherman"),1,false,false},
    {TEXT("Elder"),1,false,false},{TEXT("Child"),1,false,false},{TEXT("Scholar"),1,false,false},
    {TEXT("Husk"),2,false,true},{TEXT("Thief"),3,false,false}};
template<class T> T* Load(const FString& Path)
{ return LoadObject<T>(nullptr,*(Path+TEXT(".")+FPackageName::GetShortName(Path))); }
bool Save(UObject* Object)
{
    Object->GetOutermost()->FullyLoad();
    Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    const FString File=FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    const bool Saved=UPackage::SavePackage(Object->GetOutermost(),Object,*File,Args);
    UE_LOG(LogTemp,Display,TEXT("SURFACE_DETAIL_SAVE %s %s"),*Object->GetPathName(),Saved?TEXT("OK"):TEXT("FAILED"));
    return Saved;
}
UTexture2D* Import(const FProfile& Profile,bool bNormal)
{
    const FString Suffix=bNormal?TEXT("Normal"):TEXT("Surface");
    const FString Path=FString(TEXT("/Game/Memoria/Presentation/SurfaceDetail/T_"))+Profile.Name+TEXT("_")+Suffix;
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/SurfaceDetail")/
        (FString(Profile.Source)+(bNormal?TEXT("_normal.png"):TEXT("_surface.png"))));
    if(!FPaths::FileExists(File)){UE_LOG(LogTemp,Error,TEXT("SURFACE_DETAIL_MISSING %s"),*File);return nullptr;}
    // Explicitly reimport the owned data maps, including when the material already exists.
    // This avoids the legacy commandlets' early-return-on-existing behaviour.
    auto* Factory=NewObject<UTextureFactory>(); bool bCancelled=false;
    UTextureFactory::SuppressImportOverwriteDialog(true);
    auto* Package=CreatePackage(*Path);
    Package->FullyLoad(); // Load prior exports before overwriting an existing owned texture.
    auto* Texture=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),Package,
        *FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,GWarn,bCancelled));
    if(!Texture||bCancelled)return nullptr;
    Texture->SRGB=false;Texture->CompressionSettings=bNormal?TC_Normalmap:TC_Masks;
    Texture->LODGroup=TEXTUREGROUP_Character;Texture->Filter=TF_Trilinear;Texture->NeverStream=true;
    Texture->MipGenSettings=TMGS_SimpleAverage;Texture->bFlipGreenChannel=false;
    Texture->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({Texture});
    FAssetRegistryModule::AssetCreated(Texture);
    return Save(Texture)?Texture:nullptr;
}
template<class T> T* Node(UMaterial* M)
{auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N;}
UMaterialExpressionScalarParameter* Scalar(UMaterial* M,const TCHAR* Name,float Value)
{auto* N=Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;return N;}
UMaterialExpressionVectorParameter* Vector(UMaterial* M,const TCHAR* Name,FLinearColor Value)
{auto* N=Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;return N;}
UMaterialExpressionCustom* Custom(UMaterial* M,const TCHAR* Description,const FString& Code,
    ECustomMaterialOutputType Type,std::initializer_list<TPair<FName,UMaterialExpression*>> Inputs)
{
    auto* N=Node<UMaterialExpressionCustom>(M);N->Description=Description;N->Code=Code;N->OutputType=Type;
    for(const auto& Pair:Inputs){FCustomInput In;In.InputName=Pair.Key;In.Input.Connect(0,Pair.Value);N->Inputs.Add(In);}
    return N;
}
UMaterial* Rebuild(const FBody& Body,UTexture2D* Normal,UTexture2D* Surface)
{
    const FString Base=FString(TEXT("/Game/Memoria/Presentation/Field3D/"))+Body.Name+TEXT("/");
    auto* M=Load<UMaterial>(Base+TEXT("M_")+Body.Name);
    auto* Color=Load<UTexture2D>(Base+TEXT("T_")+Body.Name);
    auto* CrackMask=Body.bHusk?Load<UTexture2D>(Base+TEXT("T_Husk_Emissive")):nullptr;
    if(!M||!Color||(Body.bHusk&&!CrackMask))return nullptr;
    M->GetOutermost()->FullyLoad();
    M->GetExpressionCollection().Empty();
    auto* Data=M->GetEditorOnlyData();
    M->SetShadingModel(MSM_DefaultLit);M->SetUsageByFlag(MATUSAGE_SkeletalMesh,true);M->TwoSided=true;
    auto* Paint=Node<UMaterialExpressionTextureSample>(M);Paint->Texture=Color;Paint->SamplerType=SAMPLERTYPE_Color;
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
    UV->UTiling=UV->VTiling=Body.bHero?2.f:1.f;
    auto* NormalSample=Node<UMaterialExpressionTextureSampleParameter2D>(M);
    NormalSample->ParameterName=TEXT("DetailNormal");NormalSample->Texture=Normal;NormalSample->SamplerType=SAMPLERTYPE_Normal;NormalSample->Coordinates.Connect(0,UV);
    auto* SurfaceSample=Node<UMaterialExpressionTextureSampleParameter2D>(M);
    SurfaceSample->ParameterName=TEXT("SurfaceData");SurfaceSample->Texture=Surface;SurfaceSample->SamplerType=SAMPLERTYPE_Masks;SurfaceSample->Coordinates.Connect(0,UV);
    // Texture RGB output omits alpha; expose the complete packed data to the wear layer.
    auto* SurfaceRGBA=Node<UMaterialExpressionComponentMask>(M);
    SurfaceRGBA->Input.Connect(0,SurfaceSample);SurfaceRGBA->R=SurfaceRGBA->G=SurfaceRGBA->B=true;
    auto* WearMask=Node<UMaterialExpressionComponentMask>(M);
    WearMask->Input.Connect(4,SurfaceSample);WearMask->R=true;
    auto* Strength=Scalar(M,TEXT("DetailStrength"),1.f);
    const bool bFoe=Body.Profile>=2;
    auto* Tint=Vector(M,TEXT("SurfaceTint"),FLinearColor::White);
    auto* Wear=Scalar(M,TEXT("Weathering"),0.f);
    // Tint and dust act only on reflected surface colour; all existing emissive attack/void cues stay intact.
    auto* Albedo=Custom(M,TEXT("Preserved paint with non luminous wear"),bFoe?
        TEXT("float w=saturate(Weathering)*(0.18+0.30*WearNoise); return saturate(lerp(Paint*SurfaceTint, float3(0.20,0.18,0.145), w));"):
        TEXT("return Paint;"),CMOT_Float3,{{TEXT("Paint"),Paint},{TEXT("SurfaceTint"),Tint},{TEXT("Weathering"),Wear},{TEXT("WearNoise"),WearMask}});
    Data->BaseColor.Connect(0,Albedo);
    const FString Skin=TEXT("float skin=saturate((Paint.r-Paint.b-0.012)*12)*saturate((Paint.r-Paint.g-0.008)*18)*smoothstep(0.08,0.25,Paint.r); ");
    FString RoughCode=TEXT("return clamp(Surface.g+Weathering*0.12,0.36,0.96);");
    FString NormalCode=TEXT("return normalize(float3(Detail.xy*DetailStrength,lerp(1.0,Detail.z,saturate(DetailStrength))));");
    UMaterialExpressionLocalPosition* Local=nullptr;
    FString HeadGuard;
    const bool bArrel=FString(Body.Name)==TEXT("Arrel");
    if(Body.bHero)
    {
        Local=Node<UMaterialExpressionLocalPosition>(M);
        Local->LocalOrigin=ELocalPositionOrigin::Instance;
        Local->IncludedOffsets=EPositionIncludedOffsets::ExcludeOffsets;
        const float Height=FString(Body.Name)==TEXT("Elia")?166.f:180.f;
        HeadGuard=FString::Printf(TEXT("float headGuard=smoothstep(%.3f,%.3f,Local.z); "),Height*.76f,Height*.82f);
        const FString Armor=TEXT("float neutral=saturate(1.0-(max(Paint.r,max(Paint.g,Paint.b))-min(Paint.r,min(Paint.g,Paint.b)))*12); float armor=neutral*smoothstep(0.18,0.44,dot(Paint,float3(0.3,0.6,0.1)))*(1.0-headGuard)*(1.0-skin); ");
        RoughCode=Skin+HeadGuard+TEXT("float rough=lerp(Surface.g,0.58,skin); ")+
            (bArrel?Armor+TEXT("rough=lerp(rough,0.44,armor); "):TEXT(""))+
            TEXT("return clamp(lerp(rough,0.74,headGuard),0.44,0.9);");
        // Source rigs have different heights. Attenuate the head/neck in posed local space,
        // plus painted skin (hands included); do not treat irregular hero UVs as a head atlas.
        NormalCode=Skin+HeadGuard+TEXT("float gain=lerp(1.25,0.02,headGuard); gain=lerp(gain,min(gain,0.08),skin)*DetailStrength; return normalize(float3(Detail.xy*gain,1.0));");
    }
    auto* Rough=Custom(M,TEXT("Surface specific roughness"),RoughCode,CMOT_Float1,
        {{TEXT("Paint"),Paint},{TEXT("Surface"),SurfaceRGBA},{TEXT("Weathering"),Wear}});
    auto* Relief=Custom(M,TEXT("Tangent detail with protected painted skin"),NormalCode,CMOT_Float3,
        {{TEXT("Paint"),Paint},{TEXT("Detail"),NormalSample},{TEXT("DetailStrength"),Strength}});
    if(Local)
        for(auto* N:{Rough,Relief}){FCustomInput In;In.InputName=TEXT("Local");In.Input.Connect(0,Local);N->Inputs.Add(In);}
    UMaterialExpression* Metal=nullptr;
    if(bArrel)
    {
        // Arrel's neutral pale armour has a subdued metal response; white hair/face are excluded.
        const FString MetalCode=Skin+HeadGuard+
            TEXT("float neutral=saturate(1.0-(max(Paint.r,max(Paint.g,Paint.b))-min(Paint.r,min(Paint.g,Paint.b)))*12); return 0.30*neutral*smoothstep(0.18,0.44,dot(Paint,float3(0.3,0.6,0.1)))*(1.0-headGuard)*(1.0-skin);");
        Metal=Custom(M,TEXT("Restrained armour metal with protected head"),MetalCode,CMOT_Float1,
            {{TEXT("Paint"),Paint},{TEXT("Local"),Local}});
    }
    else
    {
        auto* Mask=Node<UMaterialExpressionComponentMask>(M);Mask->Input.Connect(0,SurfaceSample);Mask->B=true;Metal=Mask;
    }
    auto* AO=Node<UMaterialExpressionComponentMask>(M);AO->Input.Connect(0,SurfaceSample);AO->R=true;
    Data->Normal.Connect(0,Relief);Data->Roughness.Connect(0,Rough);Data->Metallic.Connect(0,Metal);Data->AmbientOcclusion.Connect(0,AO);
    auto* Fill=Node<UMaterialExpressionMultiply>(M);Fill->A.Connect(0,Albedo);Fill->ConstB=bFoe?.10f:.075f;
    UMaterialExpression* Emissive=Fill;
    if(bFoe)
    {
        auto* Hit=Node<UMaterialExpressionMultiply>(M);Hit->A.Connect(0,Vector(M,TEXT("HitColor"),FLinearColor(1.f,.75f,.5f)));Hit->B.Connect(0,Scalar(M,TEXT("Hit"),0.f));
        auto* Lit=Node<UMaterialExpressionAdd>(M);Lit->A.Connect(0,Fill);Lit->B.Connect(0,Hit);
        auto* Fresnel=Node<UMaterialExpressionFresnel>(M);Fresnel->Exponent=4.f;
        auto* Rim=Node<UMaterialExpressionMultiply>(M);Rim->A.Connect(0,Fresnel);
        Rim->B.Connect(0,Vector(M,TEXT("RimColor"),Body.bHusk?FLinearColor(.55f,.18f,1.f):FLinearColor(1.f,.62f,.30f)));
        auto* RimAmount=Node<UMaterialExpressionMultiply>(M);RimAmount->A.Connect(0,Rim);RimAmount->B.Connect(0,Scalar(M,TEXT("RimStrength"),.12f));
        auto* WithRim=Node<UMaterialExpressionAdd>(M);WithRim->A.Connect(0,Lit);WithRim->B.Connect(0,RimAmount);Emissive=WithRim;
        if(Body.bHusk)
        {
            auto* Mask=Node<UMaterialExpressionTextureSample>(M);Mask->Texture=CrackMask;Mask->SamplerType=SAMPLERTYPE_LinearGrayscale;
            auto* Glow=Node<UMaterialExpressionMultiply>(M);Glow->A.Connect(0,Mask);Glow->B.Connect(0,Vector(M,TEXT("Glow"),FLinearColor(.32f,.008f,.72f)));
            auto* Cracks=Node<UMaterialExpressionMultiply>(M);Cracks->A.Connect(0,Glow);Cracks->B.Connect(0,Scalar(M,TEXT("CrackStrength"),2.5f));
            auto* WithCracks=Node<UMaterialExpressionAdd>(M);WithCracks->A.Connect(0,WithRim);WithCracks->B.Connect(0,Cracks);Emissive=WithCracks;
        }
    }
    Data->EmissiveColor.Connect(0,Emissive);M->PostEditChange();return M;
}
}
UMemoriaSurfaceDetailCommandlet::UMemoriaSurfaceDetailCommandlet()
{IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaSurfaceDetailCommandlet::Main(const FString&)
{
    if(!FApp::CanEverRender())
    {
        UE_LOG(LogTemp,Error,TEXT("SURFACE_DETAIL_RENDER_REQUIRED rerun with -AllowCommandletRendering -RenderOffscreen to verify real shaders before saving materials"));
        return 1;
    }
    UTexture2D* Normal[4]={};UTexture2D* Surface[4]={};
    // Preflight all existing paint/material packages before changing any graph.
    for(const FBody& B:Bodies)
    {
        const FString P=FString(TEXT("/Game/Memoria/Presentation/Field3D/"))+B.Name+TEXT("/");
        if(!Load<UMaterial>(P+TEXT("M_")+B.Name)||!Load<UTexture2D>(P+TEXT("T_")+B.Name))return 1;
    }
    for(int32 I=0;I<4;++I)
    {Normal[I]=Import(Profiles[I],true);Surface[I]=Import(Profiles[I],false);if(!Normal[I]||!Surface[I])return 1;}
    TArray<UMaterial*> Materials;
    for(const FBody& B:Bodies)
    {auto* M=Rebuild(B,Normal[B.Profile],Surface[B.Profile]);if(!M)return 1;Materials.Add(M);}
    GShaderCompilingManager->FinishAllCompilation();
    for(auto* M:Materials)
    {
        const auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);
        if(!Resource||Resource->GetCompileErrors().Num()>0)
        {
            UE_LOG(LogTemp,Error,TEXT("SURFACE_DETAIL_COMPILE_FAILED %s"),*M->GetName());
            if(Resource)for(const FString& E:Resource->GetCompileErrors())UE_LOG(LogTemp,Error,TEXT("%s"),*E);
            return 1;
        }
    }
    for(auto* M:Materials)if(!Save(M))return 1;
    UE_LOG(LogTemp,Display,TEXT("SURFACE_DETAIL_COMPLETE textures=8 materials=14 mesh_skeleton_animation_paint_unchanged=1"));
    return 0;
}
