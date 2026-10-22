#include "MemoriaHeroS361Commandlet.h"
#include "MemoriaRetargetKit.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "AssetImportTask.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "SkinnedAssetCompiler.h"
#include "Rendering/SkeletalMeshRenderData.h"
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
FString Root;
template<class T>T* Node(UMaterial* M){auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N;}
UMaterialExpressionCustom* Custom(UMaterial* M,const FString& Code,ECustomMaterialOutputType Type,std::initializer_list<TPair<FName,UMaterialExpression*>> Inputs)
{
 auto* N=Node<UMaterialExpressionCustom>(M);N->Code=Code;N->OutputType=Type;
 for(const auto& Pair:Inputs){FCustomInput I;I.InputName=Pair.Key;I.Input.Connect(0,Pair.Value);N->Inputs.Add(I);}return N;
}
UMaterial* Surface(bool Face)
{
 const FString Name=Face?TEXT("M_EliaFace"):TEXT("M_HeroCraft"),Path=Root+Name;
 if(FPackageName::DoesPackageExist(Path))return nullptr;
 auto* M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
 M->SetShadingModel(MSM_DefaultLit);M->bUsedWithSkeletalMesh=true;M->TwoSided=true;
 auto* C=Node<UMaterialExpressionVertexColor>(M);auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
 auto* A=Node<UMaterialExpressionComponentMask>(M);A->Input.Connect(4,C);A->R=true;
 auto* Base=Custom(M,Face?TEXT("float pore=sin(UV.x*701)*sin(UV.y*673)*.003;return saturate(C.rgb*(.985+pore));"):TEXT("float cloth=1-step(.08,abs(A-.5));float weave=sin(UV.x*431)*sin(UV.y*379);return saturate(C.rgb*(1-cloth*.022*(.5+.5*weave)));"),CMOT_Float3,{{TEXT("C"),C},{TEXT("UV"),UV},{TEXT("A"),A}});
 auto* Rough=Custom(M,Face?TEXT("return .52;"):TEXT("return A<.1?.52:(A<.35?.36:(A<.65?.82:(A<.9?.32:.18)));"),CMOT_Float1,{{TEXT("A"),A}});
 auto* Metal=Custom(M,Face?TEXT("return 0;"):TEXT("return A>.65 && A<.9?.72:0;"),CMOT_Float1,{{TEXT("A"),A}});
 auto* Spec=Custom(M,Face?TEXT("return .28;"):TEXT("return A<.35?.34:(A<.65?.22:.48);"),CMOT_Float1,{{TEXT("A"),A}});
 M->GetEditorOnlyData()->BaseColor.Connect(0,Base);M->GetEditorOnlyData()->Roughness.Connect(0,Rough);M->GetEditorOnlyData()->Metallic.Connect(0,Metal);M->GetEditorOnlyData()->Specular.Connect(0,Spec);
 M->PostEditChange();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
 const auto* R=M->GetMaterialResource(GMaxRHIShaderPlatform);if(!R || R->GetCompileErrors().Num())return nullptr;
 if(!Save(M))return nullptr;UE_LOG(LogTemp,Display,TEXT("S361_MATERIAL %s shader_errors=0"),*Name);return M;
}
FString OldPath(const FString& Name)
{return Name==TEXT("Arrel")?TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SK_Arrel"):TEXT("/Game/Memoria/Presentation/Field3D/HumanoidAnatomyS360/R7/Elia/SK_Elia");}
USkeletalMesh* Import(const FString& File,const FString& Path,USkeleton* Rig)
{
 if(FPackageName::DoesPackageExist(Path))return nullptr;
 auto* Factory=NewObject<UFbxFactory>();Factory->SetDetectImportTypeOnImport(false);auto* UI=Factory->ImportUI.Get();
 UI->MeshTypeToImport=UI->OriginalImportType=FBXIT_SkeletalMesh;UI->bAutomatedImportShouldDetectType=false;UI->bImportAsSkeletal=true;UI->bImportMesh=true;UI->bImportAnimations=false;UI->Skeleton=Rig;
 UI->bCreatePhysicsAsset=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bOverrideFullName=true;
 auto* Data=UI->SkeletalMeshImportData.Get();Data->bConvertScene=true;Data->bForceFrontXAxis=true;Data->bConvertSceneUnit=true;Data->ImportRotation=FRotator(0,-90,0);
 Data->bUpdateSkeletonReferencePose=false;Data->bUseT0AsRefPose=false;Data->NormalImportMethod=FBXNIM_ImportNormalsAndTangents;Data->VertexColorImportOption=EVertexColorImportOption::Replace;
 auto* Task=NewObject<UAssetImportTask>();Task->bAutomated=true;Task->Options=UI;Factory->SetAssetImportTask(Task);
 bool Cancelled=false;auto* Result=Cast<USkeletalMesh>(Factory->ImportObject(USkeletalMesh::StaticClass(),CreatePackage(*Path),*FPackageName::GetShortName(Path),RF_Public|RF_Standalone,File,nullptr,Cancelled));
 return Cancelled?nullptr:Result;
}
bool CheckPreserved(USkeletalMesh* Mesh,USkeletalMesh* Old,const FString& Name)
{
 if(Mesh->GetSkeleton()!=Old->GetSkeleton())return false;
 const auto& A=Mesh->GetRefSkeleton();const auto& B=Old->GetRefSkeleton();if(A.GetNum()!=77 || B.GetNum()!=77)return false;
 for(int32 I=0;I<77;++I)if(A.GetBoneName(I)!=B.GetBoneName(I) || A.GetParentIndex(I)!=B.GetParentIndex(I) || !A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.015f))return false;
 const auto* NR=Mesh->GetResourceForRendering();const auto* OR=Old->GetResourceForRendering();if(!NR || !OR || NR->LODRenderData.IsEmpty() || OR->LODRenderData.IsEmpty())return false;
 const auto& N=NR->LODRenderData[0];const auto& O=OR->LODRenderData[0];
 const int32 Triangles=N.MultiSizeIndexContainer.GetIndexBuffer()->Num()/3;
 if(Triangles<1000 || Triangles>100000 || FMath::Abs(Mesh->GetBounds().BoxExtent.Z*2-Old->GetBounds().BoxExtent.Z*2)>.5)return false;
 // Surface refinements must stay local; reject thin outlier geometry even when most vertices remain valid.
 const FBox NewBox=Mesh->GetBounds().GetBox();const FBox OldBox=Old->GetBounds().GetBox();
 for(int32 Axis=0;Axis<2;++Axis)if(NewBox.Min[Axis]<OldBox.Min[Axis]-1.f || NewBox.Max[Axis]>OldBox.Max[Axis]+1.f)return false;
 UE_LOG(LogTemp,Display,TEXT("S361_LOCAL_ENVELOPE %s new=%s original=%s margin_cm=1"),*Name,*NewBox.GetSize().ToString(),*OldBox.GetSize().ToString());
 auto Key=[](const FVector3f& P,const FVector2f& UV){return FString::Printf(TEXT("%d,%d,%d:%d,%d"),FMath::RoundToInt(P.X*1000),FMath::RoundToInt(P.Y*1000),FMath::RoundToInt(P.Z*1000),FMath::RoundToInt(UV.X*100000),FMath::RoundToInt(UV.Y*100000));};
 TSet<FString> Retained;for(const auto& Section:N.RenderSections)if(Section.MaterialIndex<(Name==TEXT("Arrel")?1:2))for(uint32 I=Section.BaseVertexIndex;I<Section.BaseVertexIndex+Section.NumVertices;++I)Retained.Add(Key(N.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I),N.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(I,0)));
 int32 Count=0,Match=0;for(const auto& Section:O.RenderSections)if(Section.MaterialIndex<(Name==TEXT("Arrel")?1:2))for(uint32 I=Section.BaseVertexIndex;I<Section.BaseVertexIndex+Section.NumVertices;++I){++Count;Match+=Retained.Contains(Key(O.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(I),O.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(I,0)));}
 if(Count<1000 || Match<Count*.99f)return false;
 const auto& Colors=N.StaticVertexBuffers.ColorVertexBuffer;if(Colors.GetNumVertices()!=N.GetNumVertices() || !Colors.GetAllowCPUAccess())return false;
 int32 Added=0,Pigment=0;for(const auto& Section:N.RenderSections)if(Section.MaterialIndex==Mesh->GetMaterials().Num()-1)for(uint32 I=Section.BaseVertexIndex;I<Section.BaseVertexIndex+Section.NumVertices;++I){++Added;const auto C=Colors.VertexColor(I);Pigment+=C.R<250 || C.G<250 || C.B<250;}
 if(Added<100 || Pigment<Added*.99f)return false;
 UE_LOG(LogTemp,Display,TEXT("S361_HERO %s triangles=%d height=%.5f original_height=%.5f retained_corners=%d/%d added_vertices=%d pigmented=%d skeleton=%s"),*Name,Triangles,Mesh->GetBounds().BoxExtent.Z*2,Old->GetBounds().BoxExtent.Z*2,Match,Count,Added,Pigment,*Mesh->GetSkeleton()->GetPathName());return true;
}
}
UMemoriaHeroS361Commandlet::UMemoriaHeroS361Commandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UMemoriaHeroS361Commandlet::Main(const FString& Params)
{
 if(!FApp::CanEverRender())return 1;FString Revision=TEXT("R1");FParse::Value(*Params,TEXT("Revision="),Revision);
 if(Revision.Len()<2 || Revision[0]!=TCHAR('R') || !Revision.Mid(1).IsNumeric())return 1;
 Root=TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/")+Revision+TEXT("/");
 const FString Art=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../ArtSource/HeroPolishS361")/Revision);
 for(const TCHAR* N:{TEXT("Arrel"),TEXT("Elia")})if(FPackageName::DoesPackageExist(Root+N+TEXT("/SK_")+N) || !IFileManager::Get().FileExists(*(Art/N/(FString(N)+TEXT(".fbx")))))return 1;
 if(FPackageName::DoesPackageExist(Root+TEXT("M_HeroCraft")) || FPackageName::DoesPackageExist(Root+TEXT("M_EliaFace")))return 1;
 auto* Detail=Surface(false);auto* Face=Surface(true);if(!Detail || !Face)return 1;
 int32 Count=0;
 for(const TCHAR* N:{TEXT("Arrel"),TEXT("Elia")})
 {
  const FString Name=N;auto* Old=Load<USkeletalMesh>(OldPath(Name));if(!Old || !Old->GetSkeleton())return 1;
  const FReferenceSkeleton OriginalRig=Old->GetSkeleton()->GetReferenceSkeleton();
  auto* Mesh=Import(Art/Name/(Name+TEXT(".fbx")),Root+Name+TEXT("/SK_")+Name,Old->GetSkeleton());if(!Mesh)return 1;
  const auto& AfterRig=Old->GetSkeleton()->GetReferenceSkeleton();if(AfterRig.GetNum()!=OriginalRig.GetNum())return 1;
  for(int32 I=0;I<OriginalRig.GetNum();++I)if(AfterRig.GetBoneName(I)!=OriginalRig.GetBoneName(I) || AfterRig.GetParentIndex(I)!=OriginalRig.GetParentIndex(I) || !AfterRig.GetRefBonePose()[I].Equals(OriginalRig.GetRefBonePose()[I],.00001f))return 1;
  FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});if(Mesh->GetMaterials().Num()!=Old->GetMaterials().Num()+1)return 1;
  for(int32 I=0;I<Old->GetMaterials().Num();++I)Mesh->GetMaterials()[I].MaterialInterface=Old->GetMaterials()[I].MaterialInterface.Get();
  if(Name==TEXT("Elia"))Mesh->GetMaterials()[2].MaterialInterface=Face;
  Mesh->GetMaterials().Last().MaterialInterface=Detail;Mesh->PostEditChange();FSkinnedAssetCompilingManager::Get().FinishCompilation({Mesh});
  if(!CheckPreserved(Mesh,Old,Name) || !Save(Mesh))return 1;++Count;
 }
 UE_LOG(LogTemp,Display,TEXT("S361_IMPORT_COMPLETE meshes=%d materials=2 skeletons=0 clips=0 textures=0"),Count);return Count==2?0:1;
}
