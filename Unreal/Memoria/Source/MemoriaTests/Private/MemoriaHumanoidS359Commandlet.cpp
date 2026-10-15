#include "MemoriaHumanoidS359Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "AssetImportTask.h"
#include "Engine/SkeletalMesh.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
using namespace MemoriaRetargetKit;
namespace {
const FString Root=TEXT("/Game/Memoria/Presentation/Field3D/HumanoidNaturalS359/R6/");
const TCHAR* People[]={TEXT("Traveler"),TEXT("Bureauagent"),TEXT("Guard"),TEXT("Villagerf"),TEXT("Villagerm"),TEXT("Fisherman"),TEXT("Elder"),TEXT("Child"),TEXT("Scholar"),TEXT("Elia"),TEXT("Malet"),TEXT("AshWalker"),TEXT("BeltScavenger")};
bool Foe(const FString& Name){return Name==TEXT("AshWalker") || Name==TEXT("BeltScavenger");}
bool Ambient(const FString& Name){return !Foe(Name) && Name!=TEXT("Elia") && Name!=TEXT("Malet");}
FString MeshPath(const FString& Name){return Root+Name+TEXT("/SK_")+Name+(Ambient(Name)?TEXT("_Detail"):TEXT(""));}
FString OldMeshPath(const FString& Name){return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidBodiesS358/R3/")+Name+TEXT("/SK_")+Name+(Ambient(Name)?TEXT("_Detail"):TEXT(""));}
TArray<FString> Roles(const FString& Name)
{
 if(!Foe(Name))return {TEXT("Idle"),TEXT("Walk")};
 return {Name==TEXT("AshWalker")?TEXT("Zombie_Idle_Loop"):TEXT("Idle"),Name==TEXT("AshWalker")?TEXT("Zombie_Walk_Fwd_Loop"):TEXT("Walk"),Name==TEXT("AshWalker")?TEXT("Zombie_Scratch"):TEXT("Sword_Regular_A"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")};
}
UObject* Import(const FString& File,const FString& Path,USkeleton* Skeleton,bool Animation)
{
 if(FPackageName::DoesPackageExist(Path))return nullptr;
 auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);auto* UI=Factory->ImportUI.Get();
 UI->MeshTypeToImport=UI->OriginalImportType=Animation?FBXIT_Animation:FBXIT_SkeletalMesh;UI->bAutomatedImportShouldDetectType=false;
 UI->bImportAsSkeletal=true;UI->bImportMesh=!Animation;UI->bImportAnimations=Animation;UI->Skeleton=Skeleton;
 UI->bCreatePhysicsAsset=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;UI->OverrideAnimationName=FPackageName::GetShortName(Path);
 for(UFbxAssetImportData* Data:{static_cast<UFbxAssetImportData*>(UI->SkeletalMeshImportData.Get()),static_cast<UFbxAssetImportData*>(UI->AnimSequenceImportData.Get())})
 {Data->bConvertScene=true;Data->bForceFrontXAxis=true;Data->bConvertSceneUnit=true;Data->ImportRotation=FRotator(0,-90,0);}
 UI->SkeletalMeshImportData->bUseT0AsRefPose=false;UI->SkeletalMeshImportData->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;UI->SkeletalMeshImportData->VertexColorImportOption=EVertexColorImportOption::Replace;
 UI->AnimSequenceImportData->bUseDefaultSampleRate=true;UI->AnimSequenceImportData->AnimationLength=FBXALIT_ExportedTime;
 auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
 bool Cancelled=false;auto* Object=Factory->ImportObject(Animation?UAnimSequence::StaticClass():USkeletalMesh::StaticClass(),CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled);
 return Cancelled?nullptr:Object;
}
template<class T>T* Node(UMaterial* M){auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N;}
UMaterialExpressionCustom* Custom(UMaterial* M,const FString& Code,ECustomMaterialOutputType Type,std::initializer_list<TPair<FName,UMaterialExpression*>> Inputs)
{
 auto* N=Node<UMaterialExpressionCustom>(M);N->Code=Code;N->OutputType=Type;
 for(const auto& Pair:Inputs){FCustomInput I;I.InputName=Pair.Key;I.Input.Connect(0,Pair.Value);N->Inputs.Add(I);}return N;
}
UMaterial* Material()
{
 const FString Path=Root+TEXT("M_NaturalHuman");if(FPackageName::DoesPackageExist(Path))return nullptr;
 auto* M=NewObject<UMaterial>(CreatePackage(*Path),TEXT("M_NaturalHuman"),RF_Public|RF_Standalone);M->SetShadingModel(MSM_DefaultLit);M->bUsedWithSkeletalMesh=true;M->TwoSided=true;
 auto* Regions=Node<UMaterialExpressionVertexColor>(M);auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
 auto* Category=Node<UMaterialExpressionComponentMask>(M);Category->Input.Connect(4,Regions);Category->R=true;
 auto* Color=Custom(M,TEXT("float cloth=1-step(.10,abs(Category-.75));float weave=1-cloth*.035*(.5+.5*sin(UV.x*447)*sin(UV.y*393));return C.rgb*weave;"),CMOT_Float3,{{TEXT("C"),Regions},{TEXT("Category"),Category},{TEXT("UV"),UV}});
 auto* Rough=Custom(M,TEXT("return Category>.92?.40:(Category<.12?.66:(Category<.62?.73:.89));"),CMOT_Float1,{{TEXT("Category"),Category}});
 auto* Spec=Custom(M,TEXT("return Category<.12?.28:(Category>.92?.48:.20);"),CMOT_Float1,{{TEXT("Category"),Category}});
 M->GetEditorOnlyData()->BaseColor.Connect(0,Color);M->GetEditorOnlyData()->Roughness.Connect(0,Rough);M->GetEditorOnlyData()->Specular.Connect(0,Spec);
 M->PostEditChange();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
 const auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);if(!Resource || Resource->GetCompileErrors().Num())return nullptr;
 return Save(M)?M:nullptr;
}
// Resample authored foe motion to existing combat deadlines. NPC clips retain their authored 4s/1.2s loops.
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
            { UE_LOG(LogTemp,Error,TEXT("S359_FINAL_SOURCE_SCALE_FAILED %s %s bone=%s frame=%d scale=%s"),*Name,*Clip,*Bone.ToString(),I,*Pose.GetScale3D().ToString());return false; }
            Track.Positions.Add(FVector3f(Pose.GetTranslation()));Track.Rotations.Add(FQuat4f(Pose.GetRotation()));Track.Scales.Add(FVector3f(Pose.GetScale3D()));
        }
    }
    auto& Controller=Sequence->GetController();Controller.OpenBracket(NSLOCTEXT("S359","Retime","Preserve original clip timing"),false);
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
            { UE_LOG(LogTemp,Error,TEXT("S359_FINAL_COMPRESSED_SCALE_FAILED %s %s bone=%d fraction=%.4f scale=%s"),*Name,*Clip,Bone,Fraction,*NewPose.GetScale3D().ToString());return false; }
            if (Bone==0 || Ref.GetBoneName(Bone)==TEXT("head"))
            {
                Original->GetBoneTransform(OldPose,FSkeletonPoseBoneIndex(Bone),FAnimExtractContext(Fraction*Original->GetPlayLength(),false),false);
                UE_LOG(LogTemp,Display,TEXT("S359_FINAL_CLIP_COMPARE %s %s bone=%s fraction=%.4f old_scale=%s new_scale=%s"),*Name,*Clip,*Ref.GetBoneName(Bone).ToString(),Fraction,*OldPose.GetScale3D().ToString(),*NewPose.GetScale3D().ToString());
            }
        }
    UE_LOG(LogTemp,Display,TEXT("S359_FINAL_CLIP %s %s frames=%d rate=%d/%d length=%.6f original_length=%.6f bones=%d scale=unit"),*Name,*Clip,Frames,Rate.Numerator,Rate.Denominator,Sequence->GetPlayLength(),Original->GetPlayLength(),Ref.GetNum());
    return true;
}

bool CheckClip(UAnimSequence* Clip,const FString& Name,const FString& Role)
{
 if(!Clip || !Clip->GetDataModel() || Clip->GetPlayLength()<.2f)return false;
 Clip->WaitOnExistingCompression(true);const auto& Ref=Clip->GetSkeleton()->GetReferenceSkeleton();
 const bool Loop=Role==TEXT("Idle") || Role==TEXT("Walk") || Role.EndsWith(TEXT("_Loop"));
 for(int32 I=0;I<Ref.GetNum();++I)
 {
  FTransform First,Last;
  for(int32 Sample=0;Sample<=24;++Sample)
  {
   FTransform P;Clip->GetBoneTransform(P,FSkeletonPoseBoneIndex(I),FAnimExtractContext(Clip->GetPlayLength()*Sample/24.,false),false);
   if(P.ContainsNaN() || !P.GetScale3D().Equals(FVector::OneVector,.015f))return false;
   if(Sample==0)First=P;if(Sample==24)Last=P;
  }
  if(Loop && !First.Equals(Last,.025f))return false;
 }
 UE_LOG(LogTemp,Display,TEXT("S359_CLIP %s %s length=%.6f bones=%d unit_scales=1 continuous_loop=%d"),*Name,*Role,Clip->GetPlayLength(),Ref.GetNum(),Loop?1:0);return true;
}
}
UMemoriaHumanoidS359Commandlet::UMemoriaHumanoidS359Commandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaHumanoidS359Commandlet::Main(const FString& Params)
{
 if(!FApp::CanEverRender())return 1;
 if(FParse::Param(*Params,TEXT("Audit")))
 {
  for(const TCHAR* N:People)
  {
   const FString Name=N;if(Foe(Name))continue;auto* Mesh=Load<USkeletalMesh>(MeshPath(Name));if(!Mesh)return 1;
   FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});const auto& LOD=Mesh->GetResourceForRendering()->LODRenderData[0];const auto& Colors=LOD.StaticVertexBuffers.ColorVertexBuffer;
   FVector4 Sum(0,0,0,0);uint32 White=0;
   if(Colors.GetAllowCPUAccess())for(uint32 I=0;I<Colors.GetNumVertices();++I){const FColor C=Colors.VertexColor(I);Sum+=FVector4(C.R,C.G,C.B,C.A);White+=C==FColor::White;}
   UE_LOG(LogTemp,Display,TEXT("S359_COLOR_AUDIT %s vertices=%d colors=%d cpu=%d white=%d mean=%s material=%s"),N,LOD.GetNumVertices(),Colors.GetNumVertices(),Colors.GetAllowCPUAccess(),White,*(Sum/FMath::Max(1u,Colors.GetNumVertices())).ToString(),*Mesh->GetMaterials().Last().MaterialInterface->GetPathName());
  }
  return 0;
 }

 const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HumanoidNaturalS359/R6"));
 if(FPackageName::DoesPackageExist(Root+TEXT("M_NaturalHuman")))return 1;
 for(const TCHAR* N:People)
 {
  const FString Name=N;auto* Old=Load<USkeletalMesh>(OldMeshPath(Name));if(!Old || !Old->GetSkeleton() || Old->GetRefSkeleton().GetNum()!=77)return 1;
  if(!Foe(Name) && (FPackageName::DoesPackageExist(MeshPath(Name)) || !IFileManager::Get().FileExists(*(Art/Name/(Name+TEXT(".fbx"))))))return 1;
  for(const FString& Role:Roles(Name))
   if(FPackageName::DoesPackageExist(Root+Name+TEXT("/A_")+Name+TEXT("_")+Role) || !IFileManager::Get().FileExists(*(Art/Name/(Role+TEXT(".fbx")))))return 1;
 }
 auto* Surface=Material();if(!Surface)return 1;int32 Meshes=0,Clips=0;
 for(const TCHAR* N:People)
 {
  const FString Name=N;auto* Old=Load<USkeletalMesh>(OldMeshPath(Name));
  if(!Foe(Name))
  {
   auto* Mesh=Cast<USkeletalMesh>(Import(Art/Name/(Name+TEXT(".fbx")),MeshPath(Name),Old->GetSkeleton(),false));if(!Mesh)return 1;
   FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});const auto& A=Mesh->GetRefSkeleton();const auto& B=Old->GetRefSkeleton();
   if(A.GetNum()!=77 || Mesh->GetMaterials().Num()!=(Ambient(Name)?1:3))return 1;
   for(int32 I=0;I<77;++I)if(A.GetBoneName(I)!=B.GetBoneName(I) || A.GetParentIndex(I)!=B.GetParentIndex(I) || !A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.015f))return 1;
   for(int32 I=0;I<Mesh->GetMaterials().Num();++I)Mesh->GetMaterials()[I].MaterialInterface=Ambient(Name) || I==2?Surface:Old->GetMaterials()[I].MaterialInterface.Get();
   Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
   const auto* Render=Mesh->GetResourceForRendering();if(!Render || Render->LODRenderData.IsEmpty())return 1;
   const int32 Triangles=Render->LODRenderData[0].MultiSizeIndexContainer.GetIndexBuffer()->Num()/3;
   if(Triangles<5000 || Triangles>(Ambient(Name)?48000:65000) || !FMath::IsNearlyEqual(Mesh->GetBounds().BoxExtent.Z*2,Old->GetBounds().BoxExtent.Z*2,.15f))return 1;
   const auto& ColorLOD=Render->LODRenderData[0];const auto& Colors=ColorLOD.StaticVertexBuffers.ColorVertexBuffer;int32 Colored=0,ColorCount=0;
   if(!Colors.GetAllowCPUAccess() || Colors.GetNumVertices()!=ColorLOD.GetNumVertices())return 1;
   for(const auto& Section:ColorLOD.RenderSections)if(Section.MaterialIndex==(Ambient(Name)?0:2))
    for(uint32 I=Section.BaseVertexIndex;I<Section.BaseVertexIndex+Section.NumVertices;++I){const FColor C=Colors.VertexColor(I);++ColorCount;Colored+=C.R<250 || C.G<250 || C.B<250;}
   if(ColorCount<1000 || Colored<ColorCount*.99f)return 1;
   UE_LOG(LogTemp,Display,TEXT("S359_IMPORTED_PIGMENT %s authored=%d checked=%d"),*Name,Colored,ColorCount);
   if(!Save(Mesh))return 1;++Meshes;
   UE_LOG(LogTemp,Display,TEXT("S359_MESH %s triangles=%d height=%.5f materials=%d bones=77"),*Name,Triangles,Mesh->GetBounds().BoxExtent.Z*2,Mesh->GetMaterials().Num());
  }
  for(const FString& Role:Roles(Name))
  {
   const FString Path=Root+Name+TEXT("/A_")+Name+TEXT("_")+Role;auto* Clip=Cast<UAnimSequence>(Import(Art/Name/(Role+TEXT(".fbx")),Path,Old->GetSkeleton(),true));
   if(!Clip || Clip->GetSkeleton()!=Old->GetSkeleton())return 1;
   if(Foe(Name))
   {
    auto* Original=Load<UAnimSequence>(TEXT("/Game/Memoria/Presentation/Field3D/CharacterUpgradeS356/Accepted/")+Name+TEXT("/Combat/A_")+Name+TEXT("_")+Role);
    if(!RetimeAndCheck(Clip,Original,Name,Role))return 1;
   }
   if(!CheckClip(Clip,Name,Role) || !Save(Clip))return 1;++Clips;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S359_IMPORT_COMPLETE meshes=%d clips=%d materials=1 textures=0"),Meshes,Clips);return Meshes==11 && Clips==32?0:1;
}
