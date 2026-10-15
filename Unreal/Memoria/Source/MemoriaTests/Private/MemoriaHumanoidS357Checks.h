#pragma once
#include "Misc/AutomationTest.h"
#include "Combat/MemoriaFieldMonster.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "SkeletalRenderPublic.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

namespace MemoriaHumanoidS357
{
inline FString SculptId(const FString& Id)
{
    return Id==TEXT("Husk") || Id==TEXT("AshWalker")?TEXT("AshWalker"):
        Id==TEXT("Thief") || Id==TEXT("BeltScavenger")?TEXT("BeltScavenger"):TEXT("");
}
inline FString Folder(const FString& Id)
{
    return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/R4/")+SculptId(Id)+TEXT("/");
}
inline FString MeshPath(const FString& Id)
{
    const FString Name=TEXT("SK_")+SculptId(Id);return TEXT("/Game/Memoria/Presentation/Field3D/HumanoidBodiesS358/R3/")+SculptId(Id)+TEXT("/")+Name+TEXT(".")+Name;
}
inline void CheckAssets(FAutomationTestBase* Test,AMemoriaFieldMonster* F)
{
    auto* Figure=F?F->GetFigure():nullptr;auto* Skin=Figure?Figure->GetSkeletalMesh():nullptr;
    if(!Test->TestTrue(TEXT("S357 live humanoid has its actual skeletal component"),F && Skin && Skin->GetSkeletalMeshAsset()))return;
    const FString Id=Figure->GetCharacterId(), Sculpt=SculptId(Id);
    auto* Mesh=Skin->GetSkeletalMeshAsset();
    Test->TestEqual(TEXT("S357 humanoid loads its immutable sculpt revision"),Mesh->GetPathName(),MeshPath(Id));
    Test->TestTrue(TEXT("S357 sculpt retains the encounter's numeric height"),FMath::IsNearlyEqual(Figure->GetWorldHeight(),F->Spec().Height));
    const FString OldId=Sculpt==TEXT("AshWalker")?TEXT("Husk"):TEXT("Thief");
    auto* Original=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Memoria/Presentation/Field3D/")+OldId+TEXT("/SK_")+OldId+TEXT(".SK_")+OldId));
    if(Test->TestNotNull(TEXT("S357 original humanoid remains available for bind comparison"),Original))
    {
        Test->TestEqual(TEXT("S357 sculpt reuses its original skeleton object"),Mesh->GetSkeleton(),Original->GetSkeleton());
        const auto& A=Mesh->GetRefSkeleton();const auto& B=Original->GetRefSkeleton();
        Test->TestTrue(TEXT("S357 retains exactly all 77 original bones"),A.GetNum()==77 && B.GetNum()==77);
        for(int32 I=0;I<FMath::Min(A.GetNum(),B.GetNum());++I)
        {
            Test->TestEqual(TEXT("S357 bone names remain compatible"),A.GetBoneName(I),B.GetBoneName(I));
            Test->TestEqual(TEXT("S357 bone parents remain compatible"),A.GetParentIndex(I),B.GetParentIndex(I));
            Test->TestTrue(TEXT("S357 bind transforms remain compatible"),A.GetRefBonePose()[I].Equals(B.GetRefBonePose()[I],.015f));
        }
    }
    Test->TestEqual(TEXT("S357 separates body, gear and complete sculpted face"),Mesh->GetMaterials().Num(),3);
    Test->TestEqual(TEXT("S357 live figure binds all three material slots"),Skin->GetNumMaterials(),3);
    auto* Atlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Memoria/Presentation/Field3D/HumanoidFacesS357/T_SurfaceAtlas.T_SurfaceAtlas"));
    if(Test->TestNotNull(TEXT("S357 original surface atlas loads"),Atlas))
        Test->TestTrue(TEXT("S357 surface atlas has a real colour source"),Atlas->SRGB && Atlas->Source.GetSizeX()==1254 && Atlas->Source.GetSizeY()==1254);
    const TCHAR* Roles[]={TEXT("Body"),TEXT("Gear"),TEXT("Face")};
    const auto* Render=Skin->GetSkeletalMeshRenderData();
    if(Test->TestTrue(TEXT("S357 imported mesh exposes native LOD render data"),Render && Render->LODRenderData.Num()>0))
    {
        const auto& LOD=Render->LODRenderData[0];const auto& Colors=LOD.StaticVertexBuffers.ColorVertexBuffer;
        // UFbxFactory truncates linear FBX channels to uint8(255 * channel). Check the
        // actual imported buffer as well as the source roundtrip; a white fallback or
        // stale RNA UV/color alias can otherwise leave valid geometry and a green shader.
        const bool Inspectable=Colors.GetNumVertices()==LOD.GetNumVertices() && Colors.GetNumVertices()>0 && Colors.GetAllowCPUAccess();
        if(Test->TestTrue(TEXT("S357 every native render vertex retains inspectable authored RGBA"),Inspectable))
        {
            int32 BadAlpha=0,BadRGB=0,MaxRGB=0;const int32 RGBLimit=FMath::CeilToInt(.61f*255.f);
            for(uint32 V=0;V<Colors.GetNumVertices();++V)
            {
                const FColor C=Colors.VertexColor(V);
                const bool Category=C.A==0 || C.A==63 || C.A==64 || C.A==127 || C.A==128 || C.A==191 || C.A==192 || C.A==255;
                if(!Category)++BadAlpha;
                const int32 Brightest=FMath::Max(int32(C.R),FMath::Max(int32(C.G),int32(C.B)));
                MaxRGB=FMath::Max(MaxRGB,Brightest);if(Brightest>RGBLimit)++BadRGB;
            }
            Test->AddInfo(FString::Printf(TEXT("S357_IMPORTED_RGBA %s vertices=%u invalid_alpha=%d excessive_RGB=%d max_RGB=%d allowed_RGB=%d"),*Id,Colors.GetNumVertices(),BadAlpha,BadRGB,MaxRGB,RGBLimit));
            Test->TestEqual(TEXT("S357 imported alpha remains one of the authored surface categories"),BadAlpha,0);
            Test->TestEqual(TEXT("S357 imported albedo stays inside the source range without fallback white or UV spikes"),BadRGB,0);
        }
    }
    GShaderCompilingManager->FinishAllCompilation();
    for(int32 Slot=0;Slot<3;++Slot)
    {
        uint32 Triangles=0;
        if(Render && Render->LODRenderData.Num()>0)for(const auto& Section:Render->LODRenderData[0].RenderSections)if(Section.MaterialIndex==Slot)Triangles+=Section.NumTriangles;
        Test->TestTrue(TEXT("S357 each authored material binds real body, gear or face triangles"),Triangles>0);
        const FString Name=TEXT("M_")+Sculpt+Roles[Slot];
        auto* Parent=LoadObject<UMaterial>(nullptr,*(Folder(Id)+Name+TEXT(".")+Name));
        auto* MID=Cast<UMaterialInstanceDynamic>(Skin->GetMaterial(Slot));
        if(!Test->TestTrue(TEXT("S357 authored sections and live MIDs bind the exact intended parent"),Parent && MID && MID->Parent==Parent
            && Mesh->GetMaterials().IsValidIndex(Slot) && Mesh->GetMaterials()[Slot].MaterialInterface.Get()==Parent))continue;
        Test->TestTrue(TEXT("S357 human surfaces are opaque lit skeletal materials"),Parent->BlendMode==BLEND_Opaque && Parent->GetShadingModels().HasShadingModel(MSM_DefaultLit) && Parent->GetUsageByFlag(MATUSAGE_SkeletalMesh));
        const auto* Resource=Parent->GetMaterialResource(GMaxRHIShaderPlatform);
        Test->TestTrue(TEXT("S357 each body, gear and face shader compiles without default substitution"),Resource && Resource->GetCompileErrors().IsEmpty() && Resource->GetMaterialInterface()==Parent);
        if(Resource)for(const FString& Error:Resource->GetCompileErrors())Test->AddError(Error);
        FLinearColor Color;float Value=0.f;
        for(const TCHAR* Parameter:{TEXT("SurfaceTint"),TEXT("RimColor"),TEXT("HitColor")})
            Test->TestTrue(*FString::Printf(TEXT("S357 %s retains %s"),*Name,Parameter),Parent->GetVectorParameterValue(FMaterialParameterInfo(Parameter),Color));
        for(const TCHAR* Parameter:{TEXT("Weathering"),TEXT("RimStrength"),TEXT("Hit"),TEXT("DetailStrength")})
            Test->TestTrue(*FString::Printf(TEXT("S357 %s retains %s"),*Name,Parameter),Parent->GetScalarParameterValue(FMaterialParameterInfo(Parameter),Value));
        const bool HasGlow=Parent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Glow")),Color);
        const bool HasCrack=Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CrackStrength")),Value);
        Test->TestTrue(TEXT("S357 ash retains controlled glow while scavenger stays physical"),Sculpt==TEXT("AshWalker")?HasGlow && HasCrack:!HasGlow && !HasCrack);
        bool UsesAtlas=false,UsesVertexColor=false;TSet<UMaterialExpression*> Seen;
        TFunction<void(UMaterialExpression*)> Visit=[&](UMaterialExpression* Expression)
        {
            if(!Expression || Seen.Contains(Expression))return;Seen.Add(Expression);
            if(auto* Sample=Cast<UMaterialExpressionTextureSample>(Expression);Sample && Sample->Texture==Atlas)UsesAtlas=true;
            if(Cast<UMaterialExpressionVertexColor>(Expression))UsesVertexColor=true;
            for(FExpressionInput* Input:Expression->GetInputsView())if(Input)Visit(Input->Expression);
        };
        Test->TestNotNull(TEXT("S357 actual normal detail remains connected"),Parent->GetEditorOnlyData()->Normal.Expression);
        Test->TestNotNull(TEXT("S357 actual surface roughness remains connected"),Parent->GetEditorOnlyData()->Roughness.Expression);
        Visit(Parent->GetEditorOnlyData()->BaseColor.Expression);
        Test->TestTrue(TEXT("S357 visible albedo uses the authored atlas and mesh colour data"),Atlas && UsesAtlas && UsesVertexColor);
    }
}
// Dense face vertices must not outvote a collapsed retained body in visibility/volume checks.
inline TArray<FFinalSkinVertex> BodyVertices(USkeletalMeshComponent* Skin,const TArray<FFinalSkinVertex>& All)
{
    if(!Skin || !Skin->GetSkeletalMeshAsset() || !Skin->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("HumanoidFacesS357/")))return All;
    TArray<FFinalSkinVertex> Body;const auto* Render=Skin->GetSkeletalMeshRenderData();
    if(!Render || Render->LODRenderData.Num()==0)return Body;
    for(const auto& Section:Render->LODRenderData[0].RenderSections)
    {
        if(Section.MaterialIndex==2)continue;
        for(uint32 V=Section.BaseVertexIndex;V<FMath::Min(uint32(All.Num()),Section.BaseVertexIndex+Section.NumVertices);++V)Body.Add(All[V]);
    }
    return Body;
}
// Audit the actual face render section. Vertex alpha is a surface category in S357,
// so neither alpha zero nor mesh bounds can establish that the new head is attached.
inline void CheckFace(FAutomationTestBase* Test,USkeletalMeshComponent* Skin,AMemoriaFieldMonster* F,APlayerController* PC,const TCHAR* Pose,bool Close=false)
{
    if(!Skin || !F || !PC || SculptId(F->GetFigure()->GetCharacterId()).IsEmpty())return;
    TArray<FFinalSkinVertex> Vertices;Skin->GetCPUSkinnedVertices(Vertices,0);
    const auto* Render=Skin->GetSkeletalMeshRenderData();const auto* Weights=Skin->GetSkinWeightBuffer(0);
    if(!Test->TestTrue(TEXT("S357 sculpt has inspectable CPU skin and bone weights"),Render && Render->LODRenderData.Num()>0 && Weights && Weights->GetNumVertices()==uint32(Vertices.Num())))return;
    const auto& LOD=Render->LODRenderData[0];const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton();
    const int32 Head=Ref.FindBoneIndex(TEXT("head"));
    if(!Test->TestTrue(TEXT("S357 sculpt keeps the actual head bone"),Head!=INDEX_NONE))return;
    auto IsHead=[&](int32 Bone)
    {for(int32 I=Bone;I!=INDEX_NONE;I=Ref.GetParentIndex(I))if(I==Head)return true;return false;};
    int32 Width=0,HeightPixels=0;PC->GetViewportSize(Width,HeightPixels);
    int32 Count=0,Finite=0,Weighted=0,Visible=0,Above=0,Invalid=0,HeadCount=0,ValidUV=0;
    uint32 Triangles=0;FBox Face(ForceInit),HeadBody(ForceInit);FBox2D Projected(ForceInit);
    const FVector Actor=F->GetActorLocation(),HeadAt=Skin->GetSocketLocation(TEXT("head"));const double Height=F->Spec().Height;
    for(const auto& Section:LOD.RenderSections)
    {
        if(Section.MaterialIndex!=2)continue;Triangles+=Section.NumTriangles;
        const TConstArrayView<FBoneIndexType> BoneMap=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
        const uint32 End=FMath::Min(uint32(Vertices.Num()),Section.BaseVertexIndex+Section.NumVertices);
        for(uint32 V=Section.BaseVertexIndex;V<End;++V)
        {
            ++Count;uint32 Total=0,HeadWeight=0,FaceWeight=0;
            if(LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()>0)
            {
                const auto UV=LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(V,0);
                if(FMath::IsFinite(UV.X) && FMath::IsFinite(UV.Y) && UV.X>=-.001f && UV.X<=1.001f && UV.Y>=-.001f && UV.Y<=1.001f)++ValidUV;
            }
            for(uint32 I=0;I<Weights->GetMaxBoneInfluences();++I)
            {
                const uint32 Weight=Weights->GetBoneWeight(V,I);if(!Weight)continue;
                Total+=Weight;const uint32 Local=Weights->GetBoneIndex(V,I);
                if(Local>=uint32(BoneMap.Num())){++Invalid;continue;}
                const int32 Bone=BoneMap[Local];if(Bone<0 || Bone>=Ref.GetNum()){++Invalid;continue;}
                if(IsHead(Bone)){HeadWeight+=Weight;FaceWeight+=Weight;}
                else if(Ref.GetBoneName(Bone)==TEXT("neck_01") || Ref.GetBoneName(Bone)==TEXT("neck_02"))FaceWeight+=Weight;
            }
            if(Total && FaceWeight*4>=Total*3)++Weighted;
            const FVector Point=Skin->GetComponentTransform().TransformPosition(FVector(Vertices[V].Position));
            if(Point.ContainsNaN())continue;++Finite;Face+=Point;
            if(Total && HeadWeight*4>=Total*3){++HeadCount;HeadBody+=Point;}
            if(Point.Z>=Actor.Z-35.f)++Above;
            FVector2D Pixel;if(PC->ProjectWorldLocationToScreen(Point,Pixel))
            {
                Projected+=Pixel;if(Pixel.X>=0 && Pixel.X<Width && Pixel.Y>=0 && Pixel.Y<HeightPixels)++Visible;
            }
        }
    }
    const double N=FMath::Max(1,Count);const FVector Size=HeadBody.IsValid?HeadBody.GetSize():FVector::ZeroVector;
    const FVector2D Pixels=Projected.bIsValid?Projected.GetSize():FVector2D::ZeroVector;
    Test->AddInfo(FString::Printf(TEXT("S357_FACE %s pose=%s vertices=%d triangles=%u head_vertices=%d finite=%d weighted=%.3f visible=%.3f above=%.3f invalid=%d head_size=%s head_offset=%.3f projected=%s"),
        *F->GetFigure()->GetCharacterId(),Pose,Count,Triangles,HeadCount,Finite,Weighted/N,Visible/N,Above/N,Invalid,*Size.ToString(),HeadBody.IsValid?FVector::Dist(HeadBody.GetCenter(),HeadAt):-1.,*Pixels.ToString()));
    Test->TestTrue(TEXT("S357 new face is actual nonempty render geometry weighted to head and neck"),Count>0 && Triangles>0 && Invalid==0 && Weighted==Count && Finite==Count);
    Test->TestTrue(TEXT("S357 face UVs remain finite inside the authored surface domain"),ValidUV==Count);
    Test->TestTrue(TEXT("S357 head-weighted sculpt retains volume and follows its live head bone"),HeadCount>0 && HeadBody.IsValid && Size.GetMax()>=Height*.06 && Size.GetMin()>=Height*.02 && Size.GetMax()<=Height*.4 && FVector::Dist(HeadBody.GetCenter(),HeadAt)<=Height*.22);
    Test->TestTrue(TEXT("S357 face remains above ground and visibly inside its intended camera"),Face.IsValid && Above/N>=.90 && Visible/N>=.70 && Width>0 && HeightPixels>0 && Pixels.GetMax()>=HeightPixels*(Close?.1:.007));
}
inline bool LookAtFace(USkeletalMeshComponent* Skin,ACameraActor* Camera,APlayerController* PC,float WorldHeight,bool Profile)
{
    if(!Skin || !Camera || !PC)return false;
    TArray<FFinalSkinVertex> Vertices;Skin->GetCPUSkinnedVertices(Vertices,0);
    const auto* Render=Skin->GetSkeletalMeshRenderData();if(!Render || Render->LODRenderData.Num()==0)return false;
    FBox Face(ForceInit);
    for(const auto& Section:Render->LODRenderData[0].RenderSections)
    {
        if(Section.MaterialIndex!=2)continue;
        for(uint32 V=Section.BaseVertexIndex;V<FMath::Min(uint32(Vertices.Num()),Section.BaseVertexIndex+Section.NumVertices);++V)
            Face+=Skin->GetComponentTransform().TransformPosition(FVector(Vertices[V].Position));
    }
    if(!Face.IsValid)return false;
    const FVector Center=Face.GetCenter();
    const FVector Direction=Profile?Skin->GetRightVector():Skin->GetForwardVector();
    const FVector Eye=Center+Direction*WorldHeight*.65f+FVector(0,0,WorldHeight*.025f);
    Camera->SetActorLocation(Eye);Camera->SetActorRotation((Center-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(35.f);PC->SetViewTarget(Camera);return true;
}
}
