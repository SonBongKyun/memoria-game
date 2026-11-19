#include "MemoriaHumanoidS357Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "AssetImportTask.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Animation/AnimSequence.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
using namespace MemoriaRetargetKit;
namespace
{
const FString Root=TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/");
template<class T> T* Node(UMaterial* M) { auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N; }
UMaterialExpressionScalarParameter* Scalar(UMaterial* M,FName Name,float Value)
{auto* N=Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;return N;}
UMaterialExpressionVectorParameter* Vector(UMaterial* M,FName Name,FLinearColor Value)
{auto* N=Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;return N;}
UMaterialExpressionCustom* Custom(UMaterial* M,const FString& Code,ECustomMaterialOutputType Type,std::initializer_list<TPair<FName,UMaterialExpression*>> Inputs)
{
    auto* N=Node<UMaterialExpressionCustom>(M);N->Code=Code;N->OutputType=Type;
    for(const auto& Pair:Inputs){FCustomInput I;I.InputName=Pair.Key;I.Input.Connect(0,Pair.Value);N->Inputs.Add(I);}return N;
}
UTexture2D* Atlas(const FString& File)
{
    const FString Path=Root+TEXT("T_SurfaceAtlas");
    if(FPackageName::DoesPackageExist(Path))
    {
        auto* Existing=Load<UTexture2D>(Path);
        return Existing && Existing->Source.GetSizeX()==1254 && Existing->Source.GetSizeY()==1254 && Existing->SRGB && Existing->NeverStream?Existing:nullptr;
    }
    auto* Factory=NewObject<UTextureFactory>();bool Cancelled=false;
    auto* T=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),CreatePackage(*Path),TEXT("T_SurfaceAtlas"),RF_Public|RF_Standalone,File,nullptr,GWarn,Cancelled));
    if(!T || Cancelled)return nullptr;
    FTextureCompilingManager::Get().FinishCompilation({T});T->PreEditChange(nullptr);
    T->SRGB=true;T->CompressionSettings=TC_Default;T->LODGroup=TEXTUREGROUP_Character;
    T->PowerOfTwoMode=ETexturePowerOfTwoSetting::StretchToPowerOfTwo;T->NeverStream=true;T->Filter=TF_Trilinear;
    T->AddressX=TA_Clamp;T->AddressY=TA_Clamp;
    T->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({T});return Save(T)?T:nullptr;
}
UMaterial* Material(const FString& Path,bool Ash,bool Face,UTexture2D* Surface,UTexture2D* Normal)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* M=NewObject<UMaterial>(CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit);M->SetUsageByFlag(MATUSAGE_SkeletalMesh,true);M->TwoSided=true;
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
    auto* Regions=Node<UMaterialExpressionVertexColor>(M);
    auto* Category=Node<UMaterialExpressionComponentMask>(M);Category->Input.Connect(4,Regions);Category->R=true;
    // Four opaque source strips. Region alpha is categorical, independent of albedo RGB.
    // Inset each strip so mip filtering never samples its neighbour; metal borrows restrained leather wear.
    auto* AtlasUV=Custom(M,TEXT("float r=floor(saturate(Region)*4+.5);float strip=r>3.5?2:r;return float2(strip*.25+.018+.214*frac(UV.x*2.3),.018+.964*frac(UV.y*2.3));"),CMOT_Float2,{{TEXT("UV"),UV},{TEXT("Region"),Category}});
    auto* Paint=Node<UMaterialExpressionTextureSampleParameter2D>(M);Paint->ParameterName=TEXT("SurfaceAtlas");Paint->Texture=Surface;Paint->SamplerType=SAMPLERTYPE_Color;Paint->Coordinates.Connect(0,AtlasUV);
    auto* Tint=Vector(M,TEXT("SurfaceTint"),FLinearColor::White);
    auto* Wear=Scalar(M,TEXT("Weathering"),0.f);
    auto* Albedo=Custom(M,TEXT("float r=floor(saturate(Region)*4+.5);float mean=r<.5?.193683:r<1.5?.180769:r>2.5&&r<3.5?.036905:.036064;float l=dot(Paint,float3(.2126,.7152,.0722))/mean;float detail=clamp(.72+.28*l,.52,1.42);float3 c=Colors*detail*lerp(float3(1,1,1),Tint,.35);return lerp(c,c*float3(.85,.82,.77),saturate(Wear)*.14);"),CMOT_Float3,
        {{TEXT("Paint"),Paint},{TEXT("Colors"),Regions},{TEXT("Region"),Category},{TEXT("Tint"),Tint},{TEXT("Wear"),Wear}});
    M->GetEditorOnlyData()->BaseColor.Connect(0,Albedo);
    auto* NUV=Custom(M,TEXT("return UV*float2(7.3,9.1);"),CMOT_Float2,{{TEXT("UV"),UV}});
    auto* N=Node<UMaterialExpressionTextureSampleParameter2D>(M);N->ParameterName=TEXT("DetailNormal");N->Texture=Normal;N->SamplerType=SAMPLERTYPE_Normal;N->Coordinates.Connect(0,NUV);
    auto* Strength=Scalar(M,TEXT("DetailStrength"),Face?.08f:.14f);
    auto* Relief=Custom(M,TEXT("float r=floor(saturate(Region)*4+.5);float gain=r<.5?.35:r>3.5?.25:1;return normalize(float3(N.xy*Strength*gain,1));"),CMOT_Float3,{{TEXT("N"),N},{TEXT("Strength"),Strength},{TEXT("Region"),Category}});
    M->GetEditorOnlyData()->Normal.Connect(0,Relief);
    auto* Rough=Custom(M,TEXT("float r=floor(saturate(Region)*4+.5);return r<.5?.68:r<1.5?.88:r<2.5?.78:r<3.5?.94:.48;"),CMOT_Float1,{{TEXT("Region"),Category}});
    auto* Metal=Custom(M,TEXT("return smoothstep(.88,.97,Region)*.65;"),CMOT_Float1,{{TEXT("Region"),Category}});
    M->GetEditorOnlyData()->Roughness.Connect(0,Rough);M->GetEditorOnlyData()->Metallic.Connect(0,Metal);
    M->GetEditorOnlyData()->Specular.Connect(0,Scalar(M,TEXT("SurfaceSpecular"),.22f));
    auto* Hit=Scalar(M,TEXT("Hit"),0.f);auto* HitColor=Vector(M,TEXT("HitColor"),FLinearColor(1,.75f,.5f));
    auto* Flash=Node<UMaterialExpressionMultiply>(M);Flash->A.Connect(0,Hit);Flash->B.Connect(0,HitColor);
    auto* Rim=Vector(M,TEXT("RimColor"),Ash?FLinearColor(.55f,.18f,1.f):FLinearColor(1.f,.62f,.3f));
    auto* Amount=Scalar(M,TEXT("RimStrength"),.12f);
    auto* Fresnel=Node<UMaterialExpressionFresnel>(M);Fresnel->Exponent=5.f;
    auto* RimLight=Custom(M,TEXT("return Rim*Amount*Edge*.12;"),CMOT_Float3,{{TEXT("Rim"),Rim},{TEXT("Amount"),Amount},{TEXT("Edge"),Fresnel}});
    UMaterialExpression* Cues=RimLight;
    if(Ash)
    {
        auto* Glow=Vector(M,TEXT("Glow"),FLinearColor(.32f,.008f,.72f));auto* Crack=Scalar(M,TEXT("CrackStrength"),2.5f);
        auto* Fissure=Custom(M,TEXT("float bone=1-smoothstep(.02,.08,abs(Region-.25));float recess=1-smoothstep(.012,.045,dot(Colors,float3(.3,.6,.1)));return Glow*Crack*bone*recess*.035;"),CMOT_Float3,{{TEXT("Region"),Category},{TEXT("Colors"),Regions},{TEXT("Glow"),Glow},{TEXT("Crack"),Crack}});
        auto* Sum=Node<UMaterialExpressionAdd>(M);Sum->A.Connect(0,RimLight);Sum->B.Connect(0,Fissure);Cues=Sum;
    }
    auto* Light=Node<UMaterialExpressionAdd>(M);Light->A.Connect(0,Cues);Light->B.Connect(0,Flash);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Light);M->PostEditChange();
    if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    const auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);
    if(!Resource || Resource->GetCompileErrors().Num())
    {UE_LOG(LogTemp,Error,TEXT("S357_SHADER_FAILED %s"),*Path);if(Resource)for(const auto& Error:Resource->GetCompileErrors())UE_LOG(LogTemp,Error,TEXT("%s"),*Error);return nullptr;}
    return Save(M)?M:nullptr;
}
USkeletalMesh* Import(const FString& File,const FString& Path,USkeleton* Skeleton)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);auto* UI=Factory->ImportUI.Get();
    UI->MeshTypeToImport=UI->OriginalImportType=FBXIT_SkeletalMesh;UI->bAutomatedImportShouldDetectType=false;
    UI->bImportAsSkeletal=true;UI->bImportMesh=true;UI->bImportAnimations=false;UI->Skeleton=Skeleton;
    UI->bCreatePhysicsAsset=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;
    auto* Data=UI->SkeletalMeshImportData.Get();Data->bConvertScene=true;Data->bForceFrontXAxis=true;Data->bConvertSceneUnit=true;Data->ImportRotation=FRotator(0,-90,0);
    Data->bUseT0AsRefPose=false;Data->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;Data->VertexColorImportOption=EVertexColorImportOption::Replace;
    auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
    bool Cancelled=false;auto* Mesh=Cast<USkeletalMesh>(Factory->ImportObject(USkeletalMesh::StaticClass(),CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled));
    return Cancelled?nullptr:Mesh;
}
}
UMemoriaHumanoidS357Commandlet::UMemoriaHumanoidS357Commandlet()
{IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaHumanoidS357Commandlet::Main(const FString& Params)
{
    if(!FApp::CanEverRender()){UE_LOG(LogTemp,Error,TEXT("S357_RENDER_REQUIRED -AllowCommandletRendering -RenderOffscreen"));return 1;}
    FString Revision=TEXT("R4");FParse::Value(*Params,TEXT("Revision="),Revision);
    if(Revision.IsEmpty() || Revision.Len()>24)return 1;
    for(TCHAR C:Revision)if(!FChar::IsAlnum(C))return 1;
    const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HumanoidFacesS357"));
    const TPair<FString,FString> People[]={{TEXT("AshWalker"),TEXT("Husk")},{TEXT("BeltScavenger"),TEXT("Thief")}};
    if(!IFileManager::Get().FileExists(*(Art/TEXT("SurfaceAtlas.png"))))return 1;
    // Refuse the complete revision before creating targets; dependencies are never resaved.
    for(const auto& Person:People)
    {
        if(!IFileManager::Get().FileExists(*(Art/Revision/Person.Key/(Person.Key+TEXT(".fbx")))))return 1;
        const FString Base=Root+Revision+TEXT("/")+Person.Key+TEXT("/");
        for(const FString& Asset:{TEXT("SK_")+Person.Key,TEXT("M_")+Person.Key+TEXT("Body"),TEXT("M_")+Person.Key+TEXT("Gear"),TEXT("M_")+Person.Key+TEXT("Face")})
            if(FPackageName::DoesPackageExist(Base+Asset)){UE_LOG(LogTemp,Error,TEXT("S357_IMMUTABLE_TARGET_EXISTS %s"),*(Base+Asset));return 1;}
        auto* Original=Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/")+Person.Value+TEXT("/SK_")+Person.Value);
        auto* Normal=Load<UTexture2D>(TEXT("/Game/Memoria/Presentation/SurfaceDetail/T_")+Person.Value+TEXT("Detail_Normal"));
        if(!Original || !Original->GetSkeleton() || Original->GetRefSkeleton().GetNum()!=77 || !Normal)return 1;
        for(const TCHAR* Role:{TEXT("Idle"),TEXT("Walk"),TEXT("Strike"),TEXT("Hit"),TEXT("Death")})
        {
            const bool Ash=Person.Key==TEXT("AshWalker");
            FString Clip=FString(Role)==TEXT("Idle")?(Ash?TEXT("Zombie_Idle_Loop"):TEXT("Idle")):FString(Role)==TEXT("Walk")?(Ash?TEXT("Zombie_Walk_Fwd_Loop"):TEXT("Walk")):FString(Role)==TEXT("Strike")?(Ash?TEXT("Zombie_Scratch"):TEXT("Sword_Regular_A")):FString(Role)==TEXT("Hit")?TEXT("HitReact_Front_Lgt_01"):TEXT("Death_Front_01");
            auto* Sequence=Load<UAnimSequence>(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")+Person.Key+TEXT("/Combat/A_")+Person.Key+TEXT("_")+Clip);
            if(!Sequence || Sequence->GetSkeleton()!=Original->GetSkeleton() || Sequence->GetPlayLength()<=.2f)return 1;
        }
    }
    auto* Surface=Atlas(Art/TEXT("SurfaceAtlas.png"));if(!Surface)return 1;
    for(const auto& Person:People)
    {
        const bool Ash=Person.Key==TEXT("AshWalker");const FString Base=Root+Revision+TEXT("/")+Person.Key+TEXT("/");
        auto* Original=Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/")+Person.Value+TEXT("/SK_")+Person.Value);
        auto* Normal=Load<UTexture2D>(TEXT("/Game/Memoria/Presentation/SurfaceDetail/T_")+Person.Value+TEXT("Detail_Normal"));
        TArray<UMaterial*> Materials;
        for(const TCHAR* Slot:{TEXT("Body"),TEXT("Gear"),TEXT("Face")})
        {auto* M=Material(Base+TEXT("M_")+Person.Key+Slot,Ash,FString(Slot)==TEXT("Face"),Surface,Normal);if(!M)return 1;Materials.Add(M);}
        auto* Mesh=Import(Art/Revision/Person.Key/(Person.Key+TEXT(".fbx")),Base+TEXT("SK_")+Person.Key,Original->GetSkeleton());
        if(!Mesh || Mesh->GetSkeleton()!=Original->GetSkeleton())return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const auto& Ref=Mesh->GetRefSkeleton();const auto& Old=Original->GetRefSkeleton();
        if(Ref.GetNum()!=77 || Mesh->GetMaterials().Num()!=3)return 1;
        for(int32 I=0;I<77;++I)
            if(Ref.GetBoneName(I)!=Old.GetBoneName(I) || Ref.GetParentIndex(I)!=Old.GetParentIndex(I) || !Ref.GetRefBonePose()[I].Equals(Old.GetRefBonePose()[I],.015f))
            {UE_LOG(LogTemp,Error,TEXT("S357_BIND_FAILED %s bone=%d"),*Person.Key,I);return 1;}
        const TCHAR* Slots[]={TEXT("Body"),TEXT("Gear"),TEXT("Face")};
        for(int32 I=0;I<3;++I)
        {
            const FString Imported=Mesh->GetMaterials()[I].ImportedMaterialSlotName.ToString();
            if(!Imported.Contains(Slots[I])){UE_LOG(LogTemp,Error,TEXT("S357_SLOT_ORDER_FAILED %s slot=%d name=%s"),*Person.Key,I,*Imported);return 1;}
            Mesh->GetMaterials()[I].MaterialInterface=Materials[I];
        }
        Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const auto* Render=Mesh->GetResourceForRendering();if(!Render || Render->LODRenderData.IsEmpty())return 1;
        const auto& LOD=Render->LODRenderData[0];const int32 Triangles=LOD.MultiSizeIndexContainer.GetIndexBuffer()->Num()/3;
        const float Height=Mesh->GetBounds().BoxExtent.Z*2;
        if(Triangles<12000 || Triangles>35000 || !FMath::IsNearlyEqual(Height,Ash?185.025146f:164.968643f,.15f))
        {UE_LOG(LogTemp,Error,TEXT("S357_GEOMETRY_FAILED %s triangles=%d height=%.4f"),*Person.Key,Triangles,Height);return 1;}
        int32 FaceTriangles=0;for(const auto& Section:LOD.RenderSections)if(Section.MaterialIndex==2)FaceTriangles+=Section.NumTriangles;
        if(FaceTriangles<2000)return 1;
        if(!Save(Mesh))return 1;
        UE_LOG(LogTemp,Display,TEXT("S357_HUMANOID %s bones=77 materials=3 triangles=%d face_triangles=%d height=%.4f original_clips_preserved=1"),*Person.Key,Triangles,FaceTriangles,Height);
    }
    UE_LOG(LogTemp,Display,TEXT("S357_HUMANOID_COMPLETE revision=%s meshes=2 materials=6 atlas=1"),*Revision);return 0;
}
