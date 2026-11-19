#include "MemoriaCharacterUpgradeS356Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "AssetImportTask.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Misc/App.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionCustom.h"
#include "SkinnedAssetCompiler.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
using namespace MemoriaRetargetKit;
namespace
{
const FString Revision = TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/");
FString Base(const FString& Name) { return Revision + Name + TEXT("/"); }
UObject* Import(const FString& File, const FString& Path, USkeleton* Skeleton, bool bAnimation)
{
    // Main preflights the entire revision; this second guard rejects any target created meanwhile.
    if (FPackageName::DoesPackageExist(Path)) return nullptr;
    auto* Factory=NewObject<UFbxFactory>(); Factory->SetDetectImportTypeOnImport(false);
    auto* UI=Factory->ImportUI.Get();
    UI->MeshTypeToImport=UI->OriginalImportType=bAnimation?FBXIT_Animation:FBXIT_SkeletalMesh;
    UI->bAutomatedImportShouldDetectType=false; UI->bImportAsSkeletal=true;
    UI->bImportMesh=!bAnimation; UI->bImportAnimations=bAnimation; UI->Skeleton=Skeleton;
    UI->bCreatePhysicsAsset=false; UI->bImportMaterials=false; UI->bImportTextures=false; UI->bOverrideFullName=true;
    UI->OverrideAnimationName=FPackageName::GetShortName(Path);
    for (UFbxAssetImportData* Data:{static_cast<UFbxAssetImportData*>(UI->SkeletalMeshImportData.Get()),static_cast<UFbxAssetImportData*>(UI->AnimSequenceImportData.Get())})
    { Data->bConvertScene=true; Data->bForceFrontXAxis=true; Data->bConvertSceneUnit=true; Data->ImportRotation=FRotator(0,-90,0); }
    UI->SkeletalMeshImportData->bUseT0AsRefPose=false;
    UI->SkeletalMeshImportData->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;
    UI->SkeletalMeshImportData->VertexColorImportOption=EVertexColorImportOption::Replace;
    UI->AnimSequenceImportData->bUseDefaultSampleRate=true;
    UI->AnimSequenceImportData->AnimationLength=FBXALIT_ExportedTime;
    auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
    bool Cancelled=false;
    UObject* Result=Factory->ImportObject(bAnimation?UAnimSequence::StaticClass():USkeletalMesh::StaticClass(),CreatePackage(*Path),
        *FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled);
    return Cancelled?nullptr:Result;
}
template<class T> T* Node(UMaterial* M) { auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N; }
UTexture2D* Atlas()
{
    const FString Path=Revision+TEXT("T_SupernaturalAtlas");
    if (FPackageName::DoesPackageExist(Path))return nullptr;
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgradeS356/Accepted/SupernaturalAtlas.png"));
    auto* Factory=NewObject<UTextureFactory>();bool Cancelled=false;
    auto* T=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),CreatePackage(*Path),TEXT("T_SupernaturalAtlas"),RF_Public|RF_Standalone,File,nullptr,GWarn,Cancelled));
    if (!T || Cancelled)return nullptr;
    FTextureCompilingManager::Get().FinishCompilation({T});T->PreEditChange(nullptr);
    T->SRGB=true;T->CompressionSettings=TC_Default;T->LODGroup=TEXTUREGROUP_Character;
    T->PowerOfTwoMode=ETexturePowerOfTwoSetting::StretchToPowerOfTwo;T->NeverStream=true;T->Filter=TF_Trilinear;
    T->PostEditChange();FTextureCompilingManager::Get().FinishCompilation({T});return Save(T)?T:nullptr;
}
UMaterialExpressionCustom* Custom(UMaterial* M,const FString& Code,ECustomMaterialOutputType Type,std::initializer_list<TPair<FName,UMaterialExpression*>> Inputs)
{
    auto* N=Node<UMaterialExpressionCustom>(M);N->Code=Code;N->OutputType=Type;
    for (const auto& Pair:Inputs){FCustomInput I;I.InputName=Pair.Key;I.Input.Connect(0,Pair.Value);N->Inputs.Add(I);}return N;
}
TArray<FString> HumanClips(const FString& Name)
{
    return {Name==TEXT("AshWalker")?TEXT("Zombie_Idle_Loop"):TEXT("Idle"),Name==TEXT("AshWalker")?TEXT("Zombie_Walk_Fwd_Loop"):TEXT("Walk"),
        Name==TEXT("AshWalker")?TEXT("Zombie_Scratch"):TEXT("Sword_Regular_A"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")};
}
bool RetimeAndCheck(UAnimSequence* Sequence,UAnimSequence* Original,const FString& Name,const FString& Clip)
{
    if (!Sequence || !Original || Sequence->GetSkeleton()!=Original->GetSkeleton())return false;
    Sequence->WaitOnExistingCompression(true);Original->WaitOnExistingCompression(true);
    const auto* Model=Sequence->GetDataModel();const auto* Old=Original->GetDataModel();
    if (!Model || !Old || Model->GetNumberOfFrames()<1 || Old->GetNumberOfFrames()<1)return false;
    struct FTrack { FName Name;TArray<FVector3f> Positions,Scales;TArray<FQuat4f> Rotations; };
    TArray<FTrack> Tracks;TArray<FName> Names;Model->GetBoneTrackNames(Names);
    const int32 Frames=Old->GetNumberOfFrames();const FFrameRate Rate=Old->GetFrameRate();
    for (FName Bone:Names)
    {
        FTrack& Track=Tracks.AddDefaulted_GetRef();Track.Name=Bone;
        for (int32 I=0;I<=Frames;++I)
        {
            const FTransform Pose=Model->EvaluateBoneTrackTransform(Bone,FFrameTime::FromDecimal(double(I)*Model->GetNumberOfFrames()/Frames),EAnimInterpolationType::Linear);
            if (Pose.ContainsNaN() || !Pose.GetScale3D().Equals(FVector::OneVector,.015f))
            { UE_LOG(LogTemp,Error,TEXT("S356_FINAL_SOURCE_SCALE_FAILED %s %s bone=%s frame=%d scale=%s"),*Name,*Clip,*Bone.ToString(),I,*Pose.GetScale3D().ToString());return false; }
            Track.Positions.Add(FVector3f(Pose.GetTranslation()));Track.Rotations.Add(FQuat4f(Pose.GetRotation()));Track.Scales.Add(FVector3f(Pose.GetScale3D()));
        }
    }
    auto& Controller=Sequence->GetController();Controller.OpenBracket(NSLOCTEXT("S356","Retime","Preserve original clip timing"),false);
    Controller.ResetModel(false);Controller.SetFrameRate(Rate,false);Controller.SetNumberOfFrames(Frames,false);
    for (const FTrack& Track:Tracks)
        if (!Controller.AddBoneCurve(Track.Name,false) || !Controller.SetBoneTrackKeys(Track.Name,Track.Positions,Track.Rotations,Track.Scales,false))
        { Controller.CloseBracket(false);return false; }
    Controller.NotifyPopulated();Controller.CloseBracket(false);Sequence->PostEditChange();Sequence->WaitOnExistingCompression(true);
    if (!FMath::IsNearlyEqual(Sequence->GetPlayLength(),Original->GetPlayLength(),.0001f) || Sequence->GetDataModel()->GetNumberOfFrames()!=Frames || Sequence->GetDataModel()->GetFrameRate()!=Rate)return false;
    const auto& Ref=Sequence->GetSkeleton()->GetReferenceSkeleton();
    for (float Fraction:{0.f,.2666667f,.55f,1.f})
        for (int32 Bone=0;Bone<Ref.GetNum();++Bone)
        {
            FTransform NewPose,OldPose;Sequence->GetBoneTransform(NewPose,FSkeletonPoseBoneIndex(Bone),FAnimExtractContext(Fraction*Sequence->GetPlayLength(),false),false);
            if (NewPose.ContainsNaN() || !NewPose.GetScale3D().Equals(FVector::OneVector,.015f))
            { UE_LOG(LogTemp,Error,TEXT("S356_FINAL_COMPRESSED_SCALE_FAILED %s %s bone=%d fraction=%.4f scale=%s"),*Name,*Clip,Bone,Fraction,*NewPose.GetScale3D().ToString());return false; }
            if (Bone==0 || Ref.GetBoneName(Bone)==TEXT("head"))
            {
                Original->GetBoneTransform(OldPose,FSkeletonPoseBoneIndex(Bone),FAnimExtractContext(Fraction*Original->GetPlayLength(),false),false);
                UE_LOG(LogTemp,Display,TEXT("S356_FINAL_CLIP_COMPARE %s %s bone=%s fraction=%.4f old_scale=%s new_scale=%s"),*Name,*Clip,*Ref.GetBoneName(Bone).ToString(),Fraction,*OldPose.GetScale3D().ToString(),*NewPose.GetScale3D().ToString());
            }
        }
    UE_LOG(LogTemp,Display,TEXT("S356_FINAL_CLIP %s %s frames=%d rate=%d/%d length=%.6f original_length=%.6f bones=%d scale=unit"),*Name,*Clip,Frames,Rate.Numerator,Rate.Denominator,Sequence->GetPlayLength(),Original->GetPlayLength(),Ref.GetNum());
    return true;
}
UMaterial* BodyMaterial(const FString& Name,const FString& OriginalName,UTexture2D* PigmentAtlas)
{
    const bool Human=!OriginalName.IsEmpty(),Wisp=Name==TEXT("VoidWisp"),Ash=Name==TEXT("AshWalker");
    const FString Path=Base(Name)+TEXT("M_")+Name+TEXT("Refined");
    if (FPackageName::DoesPackageExist(Path)) return nullptr;
    UTexture2D* Paint=Human?Load<UTexture2D>(TEXT("/Game/Memoria/Presentation/Field3D/")+OriginalName+TEXT("/T_")+OriginalName):PigmentAtlas;
    const FString DetailPath=TEXT("/Game/Memoria/Presentation/SurfaceDetail/T_")+(Human?(Ash?FString(TEXT("HuskDetail")):Name==TEXT("BeltScavenger")?FString(TEXT("ThiefDetail")):FString(TEXT("HeroDetail"))):FString(TEXT("HeroDetail")))+TEXT("_Normal");
    auto* Normal=Load<UTexture2D>(DetailPath);if (!Paint || !Normal)return nullptr;
    auto* M=NewObject<UMaterial>(CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit);M->SetUsageByFlag(MATUSAGE_SkeletalMesh,true);M->TwoSided=true;
    auto* Regions=Node<UMaterialExpressionVertexColor>(M);
    auto* Mask=Node<UMaterialExpressionComponentMask>(M);Mask->Input.Connect(4,Regions);Mask->R=true;
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
    auto* SurfaceUV=Custom(M,Human?TEXT("return UV;"):Wisp?TEXT("return float2(.015+.47*frac(UV.x*4),.015+.97*frac(UV.y*4));"):TEXT("return float2(.515+.47*frac(UV.x*4),.015+.97*frac(UV.y*4));"),CMOT_Float2,{{TEXT("UV"),UV}});
    auto* Texture=Node<UMaterialExpressionTextureSampleParameter2D>(M);Texture->ParameterName=Human?TEXT("OriginalPaint"):TEXT("PigmentAtlas");Texture->Texture=Paint;Texture->SamplerType=SAMPLERTYPE_Color;Texture->Coordinates.Connect(0,SurfaceUV);
    auto* NormalUV=Custom(M,TEXT("return UV*float2(22,22);"),CMOT_Float2,{{TEXT("UV"),UV}});
    auto* N=Node<UMaterialExpressionTextureSampleParameter2D>(M);N->ParameterName=TEXT("DetailNormal");N->Texture=Normal;N->SamplerType=SAMPLERTYPE_Normal;N->Coordinates.Connect(0,NormalUV);
    auto* Tint=Node<UMaterialExpressionVectorParameter>(M);Tint->ParameterName=TEXT("SurfaceTint");Tint->DefaultValue=FLinearColor::White;
    auto* Wear=Node<UMaterialExpressionScalarParameter>(M);Wear->ParameterName=TEXT("Weathering");Wear->DefaultValue=0;
    auto* Color=Custom(M,Human?
        TEXT("float weave=.92+.08*sin(UV.x*377)*sin(UV.y*419);float3 cloth=Regions*weave;return lerp(cloth*Tint,float3(.055,.045,.03),saturate(Wear)*.08);"):
        Wisp?TEXT("return saturate(Paint*float3(.31,.43,.48)*Tint*(.65+.35*saturate(Regions.b*2))); "):
        TEXT("float mouth=1-smoothstep(.02,.055,Regions.r);float teeth=smoothstep(.30,.40,Regions.r)*(1-smoothstep(1.6,2.2,Regions.r/max(.005,Regions.g)));float3 skin=Paint*float3(.38,.29,.31)*Tint;return lerp(lerp(skin,float3(.006,.003,.008),mouth),float3(.55,.46,.33),teeth);"),CMOT_Float3,
        {{TEXT("Paint"),Texture},{TEXT("Regions"),Regions},{TEXT("GarmentMask"),Mask},{TEXT("Tint"),Tint},{TEXT("Wear"),Wear},{TEXT("UV"),UV}});
    M->GetEditorOnlyData()->BaseColor.Connect(0,Color);
    auto* Relief=Custom(M,Human?TEXT("return normalize(float3(N.xy*.18,1));"):Wisp?TEXT("return normalize(float3(N.xy*.15,1));"):TEXT("return normalize(float3(N.xy*.08,1));"),CMOT_Float3,{{TEXT("N"),N},{TEXT("GarmentMask"),Mask}});
    M->GetEditorOnlyData()->Normal.Connect(0,Relief);
    auto* Rough=Custom(M,Human?TEXT("return .86;"):Wisp?TEXT("return .48;"):TEXT("return .72;"),CMOT_Float1,{{TEXT("GarmentMask"),Mask}});
    M->GetEditorOnlyData()->Roughness.Connect(0,Rough);
    auto* Hit=Node<UMaterialExpressionScalarParameter>(M);Hit->ParameterName=TEXT("Hit");Hit->DefaultValue=0;
    auto* HitColor=Node<UMaterialExpressionVectorParameter>(M);HitColor->ParameterName=TEXT("HitColor");HitColor->DefaultValue=FLinearColor(1,.75f,.5f);
    auto* Flash=Node<UMaterialExpressionMultiply>(M);Flash->A.Connect(0,Hit);Flash->B.Connect(0,HitColor);
    auto* Rim=Node<UMaterialExpressionVectorParameter>(M);Rim->ParameterName=TEXT("RimColor");Rim->DefaultValue=FLinearColor::White;
    auto* Amount=Node<UMaterialExpressionScalarParameter>(M);Amount->ParameterName=TEXT("RimStrength");Amount->DefaultValue=.12f;
    auto* Fresnel=Node<UMaterialExpressionFresnel>(M);Fresnel->Exponent=5;
    auto* RimA=Node<UMaterialExpressionMultiply>(M);RimA->A.Connect(0,Rim);RimA->B.Connect(0,Amount);
    auto* RimB=Node<UMaterialExpressionMultiply>(M);RimB->A.Connect(0,RimA);RimB->B.Connect(0,Fresnel);
    auto* RimSoft=Node<UMaterialExpressionMultiply>(M);RimSoft->A.Connect(0,RimB);RimSoft->ConstB=.16f;
    UMaterialExpression* Cues=RimSoft;
    if (!Human || Ash)
    {
        auto* Glow=Node<UMaterialExpressionVectorParameter>(M);Glow->ParameterName=TEXT("Glow");Glow->DefaultValue=FLinearColor(.2f,.5f,.8f);
        auto* Crack=Node<UMaterialExpressionScalarParameter>(M);Crack->ParameterName=TEXT("CrackStrength");Crack->DefaultValue=1;
        auto* GlowMask=Custom(M,Wisp?TEXT("return smoothstep(.75,.95,Regions.b)*.10;"):Ash?TEXT("return GarmentMask*(1-smoothstep(.07,.16,dot(Regions,float3(.3,.6,.1))))*.014;"):TEXT("return smoothstep(3.0,4.8,Regions.r/max(.005,Regions.g))*.01;"),CMOT_Float1,{{TEXT("Regions"),Regions},{TEXT("GarmentMask"),Mask}});
        auto* GlowA=Node<UMaterialExpressionMultiply>(M);GlowA->A.Connect(0,Glow);GlowA->B.Connect(0,Crack);
        auto* GlowB=Node<UMaterialExpressionMultiply>(M);GlowB->A.Connect(0,GlowA);GlowB->B.Connect(0,GlowMask);
        auto* Light=Node<UMaterialExpressionAdd>(M);Light->A.Connect(0,GlowB);Light->B.Connect(0,RimSoft);Cues=Light;
    }
    auto* Fill=Node<UMaterialExpressionMultiply>(M);Fill->A.Connect(0,Color);Fill->ConstB=.035f;
    auto* BaseLight=Node<UMaterialExpressionAdd>(M);BaseLight->A.Connect(0,Fill);BaseLight->B.Connect(0,Cues);
    auto* Emissive=Node<UMaterialExpressionAdd>(M);Emissive->A.Connect(0,BaseLight);Emissive->B.Connect(0,Flash);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Emissive);M->PostEditChange();
    if (GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    const auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);
    if (!Resource || Resource->GetCompileErrors().Num()) { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SHADER_FAILED %s"),*Name);if(Resource)for(const auto& Error:Resource->GetCompileErrors())UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SHADER_DETAIL %s"),*Error);return nullptr; }
    return Save(M)?M:nullptr;
}
int32 ImportHeadRefinement()
{
    const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgradeS356/HeadRefinement"));
    const FString HeadRoot=TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/HeadRefinement/");
    const TPair<FString,FString> People[]={{TEXT("AshWalker"),TEXT("Husk")},{TEXT("BeltScavenger"),TEXT("Thief")}};
    // Two new mesh targets only. Accepted materials/clips and original source rigs are dependencies,
    // never reimported or saved by this mode. Refuse the complete pair before creating either target.
    for(const auto& Entry:People)
    {
        const FString Target=HeadRoot+Entry.Key+TEXT("/SK_")+Entry.Key;
        if(FPackageName::DoesPackageExist(Target))
        { UE_LOG(LogTemp,Error,TEXT("S356_HEAD_IMMUTABLE_TARGET_EXISTS %s"),*Target);return 1; }
        if(!IFileManager::Get().FileExists(*(Art/Entry.Key/(Entry.Key+TEXT(".fbx")))))return 1;
        auto* Original=Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/")+Entry.Value+TEXT("/SK_")+Entry.Value);
        auto* Material=Load<UMaterial>(Base(Entry.Key)+TEXT("M_")+Entry.Key+TEXT("Refined"));
        if(!Original || !Original->GetSkeleton() || Original->GetRefSkeleton().GetNum()!=77 || Original->GetMaterials().Num()!=1 || !Material)return 1;
        for(const FString& Clip:HumanClips(Entry.Key))
        {
            auto* Sequence=Load<UAnimSequence>(Base(Entry.Key)+TEXT("Combat/A_")+Entry.Key+TEXT("_")+Clip);
            if(!Sequence || Sequence->GetSkeleton()!=Original->GetSkeleton() || Sequence->GetPlayLength()<=.2f)return 1;
        }
    }
    for(const auto& Entry:People)
    {
        auto* Original=Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/")+Entry.Value+TEXT("/SK_")+Entry.Value);
        auto* Material=Load<UMaterial>(Base(Entry.Key)+TEXT("M_")+Entry.Key+TEXT("Refined"));
        const FString Target=HeadRoot+Entry.Key+TEXT("/SK_")+Entry.Key;
        auto* Mesh=Cast<USkeletalMesh>(Import(Art/Entry.Key/(Entry.Key+TEXT(".fbx")),Target,Original->GetSkeleton(),false));
        if(!Mesh || Mesh->GetSkeleton()!=Original->GetSkeleton())return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const auto& Ref=Mesh->GetRefSkeleton();const auto& Old=Original->GetRefSkeleton();
        if(Ref.GetNum()!=77 || Mesh->GetMaterials().Num()!=2)return 1;
        for(int32 I=0;I<Ref.GetNum();++I)
            if(Ref.GetBoneName(I)!=Old.GetBoneName(I) || Ref.GetParentIndex(I)!=Old.GetParentIndex(I) || !Ref.GetRefBonePose()[I].Equals(Old.GetRefBonePose()[I],.015f))return 1;
        Mesh->GetMaterials()[0].MaterialInterface=Original->GetMaterials()[0].MaterialInterface;
        Mesh->GetMaterials()[1].MaterialInterface=Material;
        Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if(!Save(Mesh))return 1;
        UE_LOG(LogTemp,Display,TEXT("S356_HEAD_REFINEMENT %s bones=%d height=%.4f slots=2 original_body=preserved accepted_material=referenced"),*Entry.Key,Ref.GetNum(),Mesh->GetBounds().BoxExtent.Z*2);
    }
    UE_LOG(LogTemp,Display,TEXT("S356_HEAD_REFINEMENT_COMPLETE meshes=2 dependencies_unchanged=33"));return 0;
}
}
UMemoriaCharacterUpgradeS356Commandlet::UMemoriaCharacterUpgradeS356Commandlet()
{ IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true; }
int32 UMemoriaCharacterUpgradeS356Commandlet::Main(const FString& Params)
{
    if (!FApp::CanEverRender()) { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_RENDER_REQUIRED -AllowCommandletRendering -RenderOffscreen"));return 1; }
    if(FParse::Param(*Params,TEXT("HeadRefinement")))return ImportHeadRefinement();
    const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/CharacterUpgradeS356/Accepted"));
    const TPair<FString,FString> Humanoids[]={{TEXT("Elia"),TEXT("Elia")},{TEXT("Malet"),TEXT("Malet")},{TEXT("AshWalker"),TEXT("Husk")},{TEXT("BeltScavenger"),TEXT("Thief")}};
    TArray<FString> Sources, Targets;
    for (const auto& Entry:Humanoids)
    {
        const bool HeadFigure=Entry.Key==TEXT("AshWalker") || Entry.Key==TEXT("BeltScavenger");
        if(!HeadFigure)
        { Sources.Add(Art/Entry.Key/(Entry.Key+TEXT(".fbx")));Targets.Add(Base(Entry.Key)+TEXT("SK_")+Entry.Key); }
        Targets.Add(Base(Entry.Key)+TEXT("M_")+Entry.Key+TEXT("Refined"));
        if (Entry.Key==TEXT("AshWalker") || Entry.Key==TEXT("BeltScavenger"))
            for (const FString& Clip:HumanClips(Entry.Key))
            {
                Sources.Add(Art/Entry.Key/(Clip+TEXT(".fbx")));
                Targets.Add(Base(Entry.Key)+TEXT("Combat/A_")+Entry.Key+TEXT("_")+Clip);
                auto* OriginalClip=Load<UAnimSequence>(TEXT("/Game/Memoria/Presentation/Field3D/")+Entry.Value+TEXT("/Combat/A_")+Entry.Value+TEXT("_")+Clip);
                if (!OriginalClip || OriginalClip->GetPlayLength()<=.2f || !OriginalClip->GetDataModel())
                { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SOURCE_CLIP_MISSING %s %s"),*Entry.Value,*Clip);return 1; }
            }
        auto* Original=Load<USkeletalMesh>(TEXT("/Game/Memoria/Presentation/Field3D/")+Entry.Value+TEXT("/SK_")+Entry.Value);
        if (!Original || !Original->GetSkeleton() || Original->GetRefSkeleton().GetNum()!=77 || Original->GetMaterials().IsEmpty())
        { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SOURCE_BIND_MISSING %s"),*Entry.Value);return 1; }
    }
    for (const TCHAR* Character:{TEXT("VoidWisp"),TEXT("MemoryLeech")})
    {
        const FString Name=Character;
        Sources.Add(Art/Name/(Name+TEXT(".fbx")));
        Targets.Add(Base(Name)+TEXT("SK_")+Name);
        Targets.Add(Base(Name)+TEXT("SK_")+Name+TEXT("_Skeleton"));
        Targets.Add(Base(Name)+TEXT("M_")+Name+TEXT("Refined"));
        for (const TCHAR* Clip:{TEXT("Idle"),TEXT("Walk"),Name==TEXT("VoidWisp")?TEXT("Cast"):TEXT("Bite"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")})
        {
            Sources.Add(Art/Name/(FString(Clip)+TEXT(".fbx")));
            Targets.Add(Base(Name)+TEXT("Combat/A_")+Name+TEXT("_")+Clip);
        }
    }
    Sources.Add(Art/TEXT("SupernaturalAtlas.png"));Targets.Add(Revision+TEXT("T_SupernaturalAtlas"));
    // Refuse partial revision reruns before creating or saving anything. Repair only in a disposable
    // scratch/new revision; original character meshes, skeletons, materials and paint are read-only.
    for (const FString& File:Sources)
        if (!IFileManager::Get().FileExists(*File)) { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SOURCE_MISSING %s"),*File);return 1; }
    for (const FString& Target:Targets)
        if (FPackageName::DoesPackageExist(Target)) { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_IMMUTABLE_TARGET_EXISTS %s"),*Target);return 1; }
    auto* PigmentAtlas=Atlas();if (!PigmentAtlas)return 1;
    for (const auto& Entry:Humanoids)
    {
        const FString& Name=Entry.Key;const FString OriginalBase=TEXT("/Game/Memoria/Presentation/Field3D/")+Entry.Value+TEXT("/");
        auto* Original=Load<USkeletalMesh>(OriginalBase+TEXT("SK_")+Entry.Value);
        if (!Original || !Original->GetSkeleton())return 1;
        const bool HeadFigure=Name==TEXT("AshWalker") || Name==TEXT("BeltScavenger");
        // Final encounter heads are authored by the separate two-mesh mode. In the base mode,
        // use the immutable original rig only for clip validation; never modify its mesh/materials.
        auto* Mesh=HeadFigure?Original:Cast<USkeletalMesh>(Import(Art/Name/(Name+TEXT(".fbx")),Base(Name)+TEXT("SK_")+Name,Original->GetSkeleton(),false));
        if (!Mesh || Mesh->GetSkeleton()!=Original->GetSkeleton())return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        const auto& Ref=Mesh->GetRefSkeleton();const auto& Old=Original->GetRefSkeleton();
        if (Ref.GetNum()!=77 || Ref.GetNum()!=Old.GetNum())return 1;
        for (int32 I=0;I<Ref.GetNum();++I)
            if (Ref.GetBoneName(I)!=Old.GetBoneName(I) || Ref.GetParentIndex(I)!=Old.GetParentIndex(I) || !Ref.GetRefBonePose()[I].Equals(Old.GetRefBonePose()[I],.015f))
            { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_BIND_MISMATCH %s bone=%d"),*Name,I);return 1; }
        auto* Material=BodyMaterial(Name,Entry.Value,PigmentAtlas);if (!Material)return 1;
        if(!HeadFigure)
        {
        // The complete original painted outfit and its original shader stay in the first section.
        // Accessory vertex palette is independent; it cannot cover or alter the source body paint.
        if (Mesh->GetMaterials().Num()!=2 || Original->GetMaterials().IsEmpty())
        { UE_LOG(LogTemp,Error,TEXT("S356_CHARACTER_SECTION_MISMATCH %s slots=%d"),*Name,Mesh->GetMaterials().Num());return 1; }
        Mesh->GetMaterials()[0].MaterialInterface=Original->GetMaterials()[0].MaterialInterface;
        Mesh->GetMaterials()[1].MaterialInterface=Material;
        Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!Save(Mesh))return 1;
        }
        if (Name==TEXT("AshWalker") || Name==TEXT("BeltScavenger"))
            for (const FString& Clip:HumanClips(Name))
            {
                auto* Animation=Cast<UAnimSequence>(Import(Art/Name/(Clip+TEXT(".fbx")),Base(Name)+TEXT("Combat/A_")+Name+TEXT("_")+Clip,Mesh->GetSkeleton(),true));
                auto* OriginalClip=Load<UAnimSequence>(OriginalBase+TEXT("Combat/A_")+Entry.Value+TEXT("_")+Clip);
                if (!RetimeAndCheck(Animation,OriginalClip,Name,Clip) || !Save(Animation))return 1;
            }
        UE_LOG(LogTemp,Display,TEXT("S356_CHARACTER_HUMANOID %s bones=%d height=%.2f original=%s bind=preserved"),*Name,Ref.GetNum(),Mesh->GetBounds().BoxExtent.Z*2,*Entry.Value);
    }
    for (const TCHAR* Character:{TEXT("VoidWisp"),TEXT("MemoryLeech")})
    {
        const FString Name=Character;
        auto* Material=BodyMaterial(Name,TEXT(""),PigmentAtlas);if (!Material)return 1;
        auto* Mesh=Cast<USkeletalMesh>(Import(Art/Name/(Name+TEXT(".fbx")),Base(Name)+TEXT("SK_")+Name,nullptr,false));
        if (!Mesh || !Mesh->GetSkeleton())return 1;
        FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        for (auto& Slot:Mesh->GetMaterials())Slot.MaterialInterface=Material;
        Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
        if (!Save(Mesh) || !Save(Mesh->GetSkeleton()))return 1;
        for (const TCHAR* Clip:{TEXT("Idle"),TEXT("Walk"),Name==TEXT("VoidWisp")?TEXT("Cast"):TEXT("Bite"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")})
        {
            auto* Animation=Cast<UAnimSequence>(Import(Art/Name/(FString(Clip)+TEXT(".fbx")),Base(Name)+TEXT("Combat/A_")+Name+TEXT("_")+Clip,Mesh->GetSkeleton(),true));
            if (!Animation || Animation->GetSkeleton()!=Mesh->GetSkeleton() || Animation->GetPlayLength()<=.2f || !Save(Animation))return 1;
        }
        UE_LOG(LogTemp,Display,TEXT("S356_CHARACTER_CREATURE %s bones=%d height=%.2f clips=5"),*Name,Mesh->GetRefSkeleton().GetNum(),Mesh->GetBounds().BoxExtent.Z*2);
    }
    UE_LOG(LogTemp,Display,TEXT("S356_CHARACTER_COMPLETE npc_meshes=2 creature_meshes=2 humanoid_rigs=4 clips=20 packages=33; run -HeadRefinement for final two encounter meshes"));return 0;
}