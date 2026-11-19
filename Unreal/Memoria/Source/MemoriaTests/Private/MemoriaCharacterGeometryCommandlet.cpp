#include "MemoriaCharacterGeometryCommandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "AssetImportTask.h"
#include "Animation/AnimSequence.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "SkinnedAssetCompiler.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Misc/Paths.h"
using namespace MemoriaRetargetKit;
namespace
{
FString Base(const FString& Name) { return TEXT("/Game/Memoria/Presentation/Field3D/") + Name + TEXT("/"); }
UObject* Import(const FString& File, const FString& Path, USkeleton* Skeleton, bool bAnimation)
{
    // These generated packages are immutable source revisions. A clean import uses one
    // legacy FBX pipeline for both bind pose and clips. Never mix in automatic reimport.
    if(UObject* Existing=Load<UObject>(Path))return Existing;
    auto* Factory = NewObject<UFbxFactory>(); Factory->SetDetectImportTypeOnImport(false);
    auto* UI = Factory->ImportUI.Get();
    UI->MeshTypeToImport = UI->OriginalImportType = bAnimation ? FBXIT_Animation : FBXIT_SkeletalMesh;
    UI->bAutomatedImportShouldDetectType = false; UI->bImportAsSkeletal = true;
    UI->bImportMesh = !bAnimation; UI->bImportAnimations = bAnimation; UI->Skeleton = Skeleton;
    UI->bCreatePhysicsAsset = false; UI->bImportMaterials = false; UI->bImportTextures = false; UI->bOverrideFullName = true;
    UI->OverrideAnimationName = FPackageName::GetShortName(Path);
    for (UFbxAssetImportData* Data : {static_cast<UFbxAssetImportData*>(UI->SkeletalMeshImportData.Get()), static_cast<UFbxAssetImportData*>(UI->AnimSequenceImportData.Get())})
    { Data->bConvertScene = true; Data->bForceFrontXAxis = true; Data->bConvertSceneUnit = true; Data->ImportRotation = FRotator(0, -90, 0); }
    UI->SkeletalMeshImportData->bUseT0AsRefPose = false;
    UI->SkeletalMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    UI->SkeletalMeshImportData->VertexColorImportOption = EVertexColorImportOption::Replace;
    UI->AnimSequenceImportData->bUseDefaultSampleRate = true;
    UI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
    auto* Task = NewObject<UAssetImportTask>(); Task->bAutomated = true; Task->Options = UI; Factory->SetAssetImportTask(Task);
    bool Cancelled = false;
    UObject* Result = Factory->ImportObject(bAnimation ? UAnimSequence::StaticClass() : USkeletalMesh::StaticClass(), CreatePackage(*Path),
        *FPackageName::GetShortName(Path), RF_Public | RF_Standalone, File, nullptr, Cancelled);
    return Cancelled ? nullptr : Result;
}
template<class T> T* Node(UMaterial* M) { auto* N = NewObject<T>(M); M->GetExpressionCollection().AddExpression(N); return N; }
UTexture2D* SurfaceTexture(const FString& Profile, bool bNormal)
{
    const FString Name=TEXT("T_")+Profile+(bNormal?TEXT("_Normal"):TEXT("_Surface"));
    const FString Path=Base(TEXT("CreatureDetail"))+Name;
    if(auto* T=Load<UTexture2D>(Path))return T;
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgrade/Surface")/(Profile+(bNormal?TEXT("_Normal.png"):TEXT("_Surface.png"))));
    auto* Factory=NewObject<UTextureFactory>();bool Cancelled=false;
    auto* T=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),CreatePackage(*Path),*Name,RF_Public|RF_Standalone,File,nullptr,GWarn,Cancelled));
    if(!T || Cancelled)return nullptr;
    // Finish the factory build before changing gamma/compression; UE builds textures asynchronously.
    FTextureCompilingManager::Get().FinishCompilation({T});T->PreEditChange(nullptr);
    T->SRGB=false;T->CompressionSettings=bNormal?TC_Normalmap:TC_Masks;T->LODGroup=TEXTUREGROUP_Character;T->NeverStream=true;T->Filter=TF_Trilinear;
    T->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({T});return Save(T)?T:nullptr;
}
UTexture2D* PigmentAtlas()
{
    const FString Name=TEXT("T_CreaturePigmentAtlas"),Path=Base(TEXT("CreatureDetail"))+Name;
    if(auto* T=Load<UTexture2D>(Path))return T;
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgrade/Surface/CreaturePigmentAtlas.png"));
    auto* Factory=NewObject<UTextureFactory>();bool Cancelled=false;
    auto* T=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),CreatePackage(*Path),*Name,RF_Public|RF_Standalone,File,nullptr,GWarn,Cancelled));
    if(!T || Cancelled)return nullptr;
    FTextureCompilingManager::Get().FinishCompilation({T});T->PreEditChange(nullptr);
    T->SRGB=true;T->CompressionSettings=TC_Default;T->LODGroup=TEXTUREGROUP_Character;
    T->PowerOfTwoMode=ETexturePowerOfTwoSetting::StretchToPowerOfTwo;T->NeverStream=true;T->Filter=TF_Trilinear;
    T->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({T});return Save(T)?T:nullptr;
}
UMaterial* CreatureMaterial(bool bShell)
{
    auto* Normal=SurfaceTexture(bShell?TEXT("Shell"):TEXT("Fur"),true);
    auto* Surface=SurfaceTexture(bShell?TEXT("Shell"):TEXT("Fur"),false);
    auto* Atlas=PigmentAtlas();
    if(!Normal || !Surface || !Atlas)return nullptr;
    const FString Name=bShell?TEXT("M_CrawlerDetail"):TEXT("M_CreatureDetail");
    const FString Path = Base(TEXT("CreatureDetail")) + Name;
    auto* M=Load<UMaterial>(Path);
    if(!M)M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public | RF_Standalone);
    M->GetOutermost()->FullyLoad();M->GetExpressionCollection().Empty();
    M->SetShadingModel(MSM_DefaultLit); M->SetUsageByFlag(MATUSAGE_SkeletalMesh,true);
    auto* Regions = Node<UMaterialExpressionVertexColor>(M);
    auto* Tint = Node<UMaterialExpressionVectorParameter>(M); Tint->ParameterName=TEXT("SurfaceTint"); Tint->DefaultValue=FLinearColor::White;
    // Blender exports LINEAR vertex RGB explicitly; FBX importer preserves those numeric values.
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);UV->UTiling=UV->VTiling=4.f;
    auto* N=Node<UMaterialExpressionTextureSampleParameter2D>(M);N->Texture=Normal;N->ParameterName=TEXT("DetailNormal");N->SamplerType=SAMPLERTYPE_Normal;N->Coordinates.Connect(0,UV);
    auto* Data=Node<UMaterialExpressionTextureSampleParameter2D>(M);Data->Texture=Surface;Data->ParameterName=TEXT("SurfaceData");Data->SamplerType=SAMPLERTYPE_Masks;Data->Coordinates.Connect(0,UV);
    auto Input=[](UMaterialExpressionCustom* C,const TCHAR* Name,UMaterialExpression* E){FCustomInput I;I.InputName=Name;I.Input.Connect(0,E);C->Inputs.Add(I);};
    auto* AtlasUV=Node<UMaterialExpressionCustom>(M);AtlasUV->OutputType=CMOT_Float2;AtlasUV->Inputs.Empty();Input(AtlasUV,TEXT("UV"),UV);
    AtlasUV->Code=bShell?TEXT("return float2(.51+.48*frac(UV.x),.01+.98*frac(UV.y));"):TEXT("return float2(.01+.48*frac(UV.x),.01+.98*frac(UV.y));");
    auto* Paint=Node<UMaterialExpressionTextureSampleParameter2D>(M);Paint->Texture=Atlas;Paint->ParameterName=TEXT("PigmentAtlas");Paint->SamplerType=SAMPLERTYPE_Color;Paint->Coordinates.Connect(0,AtlasUV);
    auto* Pigment=Node<UMaterialExpressionCustom>(M);Pigment->OutputType=CMOT_Float3;Pigment->Inputs.Empty();Input(Pigment,TEXT("Regions"),Regions);Input(Pigment,TEXT("Paint"),Paint);
    Pigment->Code=bShell?TEXT("return Regions.rgb*(Paint.rgb*2+.20);"):TEXT("float skin=smoothstep(1.55,2.0,Regions.r/max(Regions.g,.001));return lerp(Regions.rgb*(Paint.rgb*3+.25),Regions.rgb*.55,skin);");
    auto* FineNormal=Node<UMaterialExpressionCustom>(M);FineNormal->OutputType=CMOT_Float3;FineNormal->Inputs.Empty();Input(FineNormal,TEXT("N"),N);Input(FineNormal,TEXT("Regions"),Regions);
    FineNormal->Code=bShell?TEXT("return normalize(float3(N.xy*.3,N.z));"):TEXT("float skin=smoothstep(1.55,2.0,Regions.r/max(Regions.g,.001));return normalize(float3(N.xy*.12*(1-skin),N.z));");
    M->GetEditorOnlyData()->Normal.Connect(0,FineNormal);
    auto* Color = Node<UMaterialExpressionMultiply>(M); Color->A.Connect(0,Pigment); Color->B.Connect(0,Tint);
    auto* Wear = Node<UMaterialExpressionScalarParameter>(M); Wear->ParameterName=TEXT("Weathering"); Wear->DefaultValue=0.f;
    auto* Dust = Node<UMaterialExpressionMultiply>(M); Dust->A.Connect(0,Wear); Dust->ConstB=.22f;
    auto* Worn = Node<UMaterialExpressionLinearInterpolate>(M); Worn->A.Connect(0,Color); Worn->ConstB=.18f; Worn->Alpha.Connect(0,Dust);
    M->GetEditorOnlyData()->BaseColor.Connect(0,Worn);
    auto* Rough = Node<UMaterialExpressionScalarParameter>(M); Rough->ParameterName=TEXT("Roughness"); Rough->DefaultValue=.78f;
    M->GetEditorOnlyData()->Roughness.Connect(2,Data);
    auto* Fill = Node<UMaterialExpressionMultiply>(M); Fill->A.Connect(0,Color); Fill->ConstB=.04f;
    auto* Hit = Node<UMaterialExpressionScalarParameter>(M); Hit->ParameterName=TEXT("Hit"); Hit->DefaultValue=0;
    auto* HitColor = Node<UMaterialExpressionVectorParameter>(M); HitColor->ParameterName=TEXT("HitColor"); HitColor->DefaultValue=FLinearColor(1,.75f,.5f);
    auto* Flash = Node<UMaterialExpressionMultiply>(M); Flash->A.Connect(0,Hit); Flash->B.Connect(0,HitColor);
    auto* Lit = Node<UMaterialExpressionAdd>(M); Lit->A.Connect(0,Fill); Lit->B.Connect(0,Flash);
    auto* Rim = Node<UMaterialExpressionVectorParameter>(M); Rim->ParameterName=TEXT("RimColor"); Rim->DefaultValue=FLinearColor::White;
    auto* Amount = Node<UMaterialExpressionScalarParameter>(M); Amount->ParameterName=TEXT("RimStrength"); Amount->DefaultValue=.12f;
    auto* Fresnel = Node<UMaterialExpressionFresnel>(M); Fresnel->Exponent=4;
    auto* RimA = Node<UMaterialExpressionMultiply>(M); RimA->A.Connect(0,Rim); RimA->B.Connect(0,Amount);
    auto* RimB = Node<UMaterialExpressionMultiply>(M); RimB->A.Connect(0,RimA); RimB->B.Connect(0,Fresnel);
    auto* Emissive = Node<UMaterialExpressionAdd>(M); Emissive->A.Connect(0,Lit); Emissive->B.Connect(0,RimB);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Emissive); M->PostEditChange();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    const auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);
    if(!Resource || Resource->GetCompileErrors().Num())
    { UE_LOG(LogTemp,Error,TEXT("GEOMETRY_MATERIAL_COMPILE_FAILED %s"),*Name);return nullptr; }
    return Save(M) ? M : nullptr;
}
}
UMemoriaCharacterGeometryCommandlet::UMemoriaCharacterGeometryCommandlet()
{ IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UMemoriaCharacterGeometryCommandlet::Main(const FString& Params)
{
    const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgrade"));
    for (const TCHAR* Name : {TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar")})
    {
        const FString B=Base(Name), N=Name;
        auto* Original=Load<USkeletalMesh>(B+TEXT("SK_")+N);
        if(!Original || !Original->GetSkeleton()) return 1;
        const auto Poses=Original->GetRefSkeleton().GetRefBonePose();
        auto* Mesh=Cast<USkeletalMesh>(Import(Art/N/(N+TEXT(".fbx")),B+TEXT("SK_")+N+TEXT("_Detail"),Original->GetSkeleton(),false));
        if(!Mesh || Mesh->GetSkeleton()!=Original->GetSkeleton()) return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const auto& Ref=Mesh->GetRefSkeleton();
        if(Ref.GetNum()!=77 || Ref.GetNum()!=Poses.Num()) return 1;
        for(int32 I=0;I<Ref.GetNum();++I)
        {
            if(Ref.GetBoneName(I)!=Original->GetRefSkeleton().GetBoneName(I) || !Ref.GetRefBonePose()[I].Equals(Poses[I],.015f))
            { UE_LOG(LogTemp,Error,TEXT("GEOMETRY_BIND_MISMATCH %s bone=%d"),Name,I); return 1; }
        }
        for(auto& Slot:Mesh->GetMaterials()) Slot.MaterialInterface=Original->GetMaterials()[0].MaterialInterface;
        Mesh->PostEditChange(); FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if(!Save(Mesh)) return 1;
        UE_LOG(LogTemp,Display,TEXT("GEOMETRY_NPC %s bones=%d height=%.2f bind=preserved"),Name,Ref.GetNum(),Mesh->GetBounds().BoxExtent.Z*2);
    }
    for(const TCHAR* Name:{TEXT("RubbleRat"),TEXT("DustCrawler")})
    {
        const FString N=Name,B=Base(N);
        auto* Material=CreatureMaterial(N==TEXT("DustCrawler"));if(!Material)return 1;
        auto* Mesh=Cast<USkeletalMesh>(Import(Art/N/(N+TEXT(".fbx")),B+TEXT("SK_")+N,nullptr,false));
        if(!Mesh || !Mesh->GetSkeleton())return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        for(auto& Slot:Mesh->GetMaterials())Slot.MaterialInterface=Material;
        Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if(!Save(Mesh)||!Save(Mesh->GetSkeleton()))return 1;
        for(const TCHAR* Clip:{TEXT("Idle"),TEXT("Walk"),TEXT("Bite"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")})
        {
            auto* Animation=Cast<UAnimSequence>(Import(Art/N/(FString(Clip)+TEXT(".fbx")),B+TEXT("Combat/A_")+N+TEXT("_")+Clip,Mesh->GetSkeleton(),true));
            if(!Animation || Animation->GetSkeleton()!=Mesh->GetSkeleton() || Animation->GetPlayLength()<=0.f || !Save(Animation))return 1;
        }
        UE_LOG(LogTemp,Display,TEXT("GEOMETRY_CREATURE %s bones=%d height=%.2f clips=5"),Name,Mesh->GetRefSkeleton().GetNum(),Mesh->GetBounds().BoxExtent.Z*2);
    }
    UE_LOG(LogTemp,Display,TEXT("GEOMETRY_COMPLETE npc=9 creatures=2 clips=10")); return 0;
}
