#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Presentation/MemoriaCombatClips.h"
#include "Presentation/MemoriaHeroPropMount.h"
#include "Presentation/MemoriaFieldAnimInstance.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "Json.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "SkeletalRenderPublic.h"
#include "StaticMeshResources.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "HighResScreenshot.h"
#include "Misc/SecureHash.h"
#include "Misc/Guid.h"
#include "EngineUtils.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
constexpr double CellSize = 8.;
constexpr double Clearance = .25; // 2.5mm, excluding the explicit mounting/contact zone.
struct FTriangle
{
    FVector P[3]; FBox Bounds;
    explicit FTriangle(const FVector& A,const FVector& B,const FVector& C) : P{A,B,C}, Bounds(ForceInit)
    { Bounds+=A; Bounds+=B; Bounds+=C; }
};
FIntVector Cell(const FVector& P)
{ return FIntVector(FMath::FloorToInt(P.X/CellSize),FMath::FloorToInt(P.Y/CellSize),FMath::FloorToInt(P.Z/CellSize)); }
double BoxDistanceSquared(const FBox& A,const FBox& B)
{
    double Result=0.;
    for(int32 Axis=0;Axis<3;++Axis)
    { const double D=FMath::Max(0.,FMath::Max(A.Min[Axis]-B.Max[Axis],B.Min[Axis]-A.Max[Axis])); Result+=D*D; }
    return Result;
}
double SoleSupportArea(const TArray<FVector>& Positions,int32& OutHullVertices)
{
    TArray<FVector2D> Points;TSet<FIntPoint> Seen;
    for(const FVector& P:Positions)
    {
        const FIntPoint Key(FMath::RoundToInt(P.X*10000),FMath::RoundToInt(P.Y*10000));
        if(!Seen.Contains(Key)){Seen.Add(Key);Points.Emplace(P.X,P.Y);}
    }
    Points.Sort([](const FVector2D& A,const FVector2D& B){return A.X<B.X || (A.X==B.X && A.Y<B.Y);});
    auto Cross=[](const FVector2D& A,const FVector2D& B,const FVector2D& C)
    {return (B.X-A.X)*(C.Y-A.Y)-(B.Y-A.Y)*(C.X-A.X);};
    TArray<FVector2D> Lower,Upper;
    for(const FVector2D& P:Points)
    {while(Lower.Num()>=2 && Cross(Lower[Lower.Num()-2],Lower.Last(),P)<=1e-9)Lower.Pop(EAllowShrinking::No);Lower.Add(P);}
    for(int32 I=Points.Num()-1;I>=0;--I)
    {const auto& P=Points[I];while(Upper.Num()>=2 && Cross(Upper[Upper.Num()-2],Upper.Last(),P)<=1e-9)Upper.Pop(EAllowShrinking::No);Upper.Add(P);}
    if(!Lower.IsEmpty())Lower.Pop(EAllowShrinking::No);if(!Upper.IsEmpty())Upper.Pop(EAllowShrinking::No);Lower.Append(Upper);
    OutHullVertices=Lower.Num();if(OutHullVertices<3)return 0.;
    double Area=0.;for(int32 I=0;I<Lower.Num();++I)
    {const auto& A=Lower[I];const auto& B=Lower[(I+1)%Lower.Num()];Area+=A.X*B.Y-B.X*A.Y;}
    return FMath::Abs(Area)*.5;
}
bool SegmentTriangle(const FVector& Start,const FVector& End,const FTriangle& Triangle,FVector& Hit)
{
    const FVector D=End-Start,E1=Triangle.P[1]-Triangle.P[0],E2=Triangle.P[2]-Triangle.P[0];
    const FVector P=FVector::CrossProduct(D,E2); const double Det=FVector::DotProduct(E1,P);
    if(FMath::Abs(Det)<1e-10) return false;
    const double Inv=1./Det; const FVector S=Start-Triangle.P[0];
    const double U=FVector::DotProduct(S,P)*Inv; if(U<0. || U>1.) return false;
    const FVector Q=FVector::CrossProduct(S,E1); const double V=FVector::DotProduct(D,Q)*Inv;
    if(V<0. || U+V>1.) return false;
    const double T=FVector::DotProduct(E2,Q)*Inv;
    if(T<0. || T>1.) return false; Hit=Start+D*T; return true;
}
double TriangleDistance(const FTriangle& A,const FTriangle& B,FVector& Contact,bool& bCrossing)
{
    double Minimum=DBL_MAX; bCrossing=false;
    for(int32 I=0;I<3;++I)
    {
        if(SegmentTriangle(A.P[I],A.P[(I+1)%3],B,Contact)
            || SegmentTriangle(B.P[I],B.P[(I+1)%3],A,Contact)) { bCrossing=true; return 0.; }
        Minimum=FMath::Min(Minimum,FVector::DistSquared(A.P[I],FMath::ClosestPointOnTriangleToPoint(A.P[I],B.P[0],B.P[1],B.P[2])));
        Minimum=FMath::Min(Minimum,FVector::DistSquared(B.P[I],FMath::ClosestPointOnTriangleToPoint(B.P[I],A.P[0],A.P[1],A.P[2])));
        for(int32 J=0;J<3;++J)
        {
            FVector PA,PB; FMath::SegmentDistToSegmentSafe(A.P[I],A.P[(I+1)%3],B.P[J],B.P[(J+1)%3],PA,PB);
            Minimum=FMath::Min(Minimum,FVector::DistSquared(PA,PB));
        }
    }
    return FMath::Sqrt(Minimum);
}
struct FSurfaceGrid
{
    TArray<FTriangle> Triangles;
    TMap<FIntVector,TArray<int32>> Buckets;
    void Add(const FTriangle& Triangle)
    {
        if(FVector::CrossProduct(Triangle.P[1]-Triangle.P[0],Triangle.P[2]-Triangle.P[0]).SizeSquared()<1e-12) return;
        const int32 Index=Triangles.Add(Triangle);
        const FIntVector A=Cell(Triangle.Bounds.Min),B=Cell(Triangle.Bounds.Max);
        for(int32 X=A.X;X<=B.X;++X) for(int32 Y=A.Y;Y<=B.Y;++Y) for(int32 Z=A.Z;Z<=B.Z;++Z)
            Buckets.FindOrAdd(FIntVector(X,Y,Z)).Add(Index);
    }
    void Candidates(const FBox& Box,TSet<int32>& Out) const
    {
        const FIntVector A=Cell(Box.Min),B=Cell(Box.Max);
        for(int32 X=A.X;X<=B.X;++X) for(int32 Y=A.Y;Y<=B.Y;++Y) for(int32 Z=A.Z;Z<=B.Z;++Z)
            if(const auto* Values=Buckets.Find(FIntVector(X,Y,Z))) for(int32 Index:*Values) Out.Add(Index);
    }
};
struct FContact
{
    int32 CrossedPropTriangles=0,NearPropTriangles=0,MountExemptTriangles=0,TestedTriangles=0;
    int32 SteelCrossings=0,HiltCrossings=0;
    double Minimum=DBL_MAX; FBox Crossings=FBox(ForceInit);
};

class FHeroS362ContactReplay final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test; const bool bBlockOnly;
    double Started=FPlatformTime::Seconds(),OldDelta=0.; uint64 LastFrame=MAX_uint64;
    bool bFixed=false,bOldFixed=false,bInitialized=false; int32 Frame=0,SampleIndex=0;
    TWeakObjectPtr<UWorld> World; TWeakObjectPtr<AActor> Actor;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    TWeakObjectPtr<ACameraActor> CaptureCamera;
    TWeakObjectPtr<AActor> PreviousViewTarget;
    int32 CaptureView=0,CaptureViewFrame=0,CapturePollFrames=0;
    bool bCaptureRequested=false,bCaptureProcessed=false,bCaptureAbsentBefore=false;
    FDelegateHandle CaptureProcessedHandle;
    FString CaptureRunFolder,CaptureFile;
    FIntPoint CaptureExpectedSize=FIntPoint::ZeroValue;
    struct FHiddenFigurePart {TWeakObjectPtr<USceneComponent> Component;bool bVisible;};
    TArray<FHiddenFigurePart> HiddenFigureParts;
    TArray<TSharedPtr<FJsonValue>> CaptureEvidence;
    TWeakObjectPtr<UStaticMeshComponent> Blade,Scabbard,Sheathed;
    FTransform OriginalGrip;
    FTransform ReferencePelvis;
    FTransform FrozenOriginalMount;
    FVector MouthLocal=FVector::ZeroVector,TipLocal=FVector::ZeroVector;
    bool bMouthValid=false;
    TSharedPtr<FJsonObject> MouthGeometry;
    TArray<TSharedPtr<FJsonValue>> Rows;
    TArray<TSharedPtr<FJsonValue>> SoleGeometry;
    TArray<TSharedPtr<FJsonValue>> WalkDiagnostics;
    TArray<uint32> Sole[2]; FVector PreviousSole[2]; bool PreviousSupport[2]={false,false};
    TMap<uint32,FVector> PreviousSoleVertices[2];
    int32 CentroidGroundedSamples=0;double MaximumCentroidFootDrift=0.;
    bool bPreviousWalkSample=false;
    float PreviousWalkWeight=0.f;
    double MaximumWalkEntrySupportedMotion=0.;
    int32 GroundedFootSamples=0,AirFrames=0; double MaximumFootDrift=0.;
    struct FSample { FString Role; float Fraction; bool bHold; };
    TArray<FSample> Samples;

    TSharedPtr<FJsonObject> PoseSnapshot()
    {
        auto* Skin=Figure->GetSkeletalMesh();FSHA1 Hash;
        auto Number=[&Hash](double Value){Hash.Update(reinterpret_cast<const uint8*>(&Value),sizeof(Value));};
        const auto& Bones=Skin->GetComponentSpaceTransforms();
        for(const auto& Bone:Bones)
        {
            const auto P=Bone.GetTranslation(),S=Bone.GetScale3D();const auto Q=Bone.GetRotation();
            Number(P.X);Number(P.Y);Number(P.Z);Number(Q.X);Number(Q.Y);Number(Q.Z);Number(Q.W);Number(S.X);Number(S.Y);Number(S.Z);
        }
        Hash.Final();uint8 Digest[FSHA1::DigestSize];Hash.GetHash(Digest);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("bone_component_hash"),FString::FromHexBlob(Digest,FSHA1::DigestSize));
        Row->SetNumberField(TEXT("bone_count"),Bones.Num());Row->SetNumberField(TEXT("engine_frame"),double(GFrameCounter));
        Row->SetNumberField(TEXT("app_delta_seconds"),FApp::GetDeltaTime());Row->SetNumberField(TEXT("fixed_delta_seconds"),FApp::GetFixedDeltaTime());
        Row->SetBoolField(TEXT("fixed_time_step"),FApp::UseFixedTimeStep());Row->SetStringField(TEXT("skin_component_transform"),Skin->GetComponentTransform().ToString());
        if(const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance()))
        {Row->SetNumberField(TEXT("age"),Anim->Age);Row->SetNumberField(TEXT("phase"),Anim->Phase);Row->SetNumberField(TEXT("locomotion_weight"),Anim->Weight);Row->SetNumberField(TEXT("action_time"),Anim->ActionTime);Row->SetNumberField(TEXT("action_weight"),Anim->ActionWeight);}
        Row->SetStringField(TEXT("sampler"),TEXT("Current native ref-to-local matrices; ComputeSkinnedPositions; no pose re-evaluation"));return Row;
    }

    void RemoveCaptureCallback()
    {
        if(CaptureProcessedHandle.IsValid())FScreenshotRequest::OnScreenshotRequestProcessed().Remove(CaptureProcessedHandle);
        CaptureProcessedHandle.Reset();
    }
    void RestoreCaptureCamera()
    {
        RemoveCaptureCallback();
        for(const auto& Part:HiddenFigureParts)if(Part.Component.IsValid())
        {Part.Component->SetVisibility(Part.bVisible,false);Test->TestEqual(TEXT("Capture teardown restores each other figure part original visibility"),Part.Component->GetVisibleFlag(),Part.bVisible);}
        HiddenFigureParts.Reset();
        if(World.IsValid())if(auto* PC=World->GetFirstPlayerController())
            if(PreviousViewTarget.IsValid())PC->SetViewTarget(PreviousViewTarget.Get());
        if(CaptureCamera.IsValid())CaptureCamera->Destroy();CaptureCamera.Reset();
    }
    void HideOtherFieldFiguresForCapture()
    {
        TSet<USceneComponent*> Seen;
        for(TActorIterator<AActor> It(World.Get());It;++It)if(*It!=Actor.Get())
        {
            TArray<UMemoriaFieldCharacterComponent*> Figures;It->GetComponents(Figures);
            for(auto* Other:Figures)
            {
                TArray<USceneComponent*> Parts;Other->GetChildrenComponents(true,Parts);Parts.Add(Other);
                for(auto* Part:Parts)if(Part && !Seen.Contains(Part))
                {Seen.Add(Part);HiddenFigureParts.Add({Part,Part->GetVisibleFlag()});Part->SetVisibility(false,false);}
            }
        }
        Test->AddInfo(FString::Printf(TEXT("S362_CAPTURE_OTHER_FIGURES_HIDDEN parts=%d individual_visibility_recorded=1"),HiddenFigureParts.Num()));
    }
    bool CompleteCapturePNG(FIntPoint& Size,int64& Bytes,FString& Detail) const
    {
        TArray<uint8> Data;if(!FFileHelper::LoadFileToArray(Data,*CaptureFile)){Detail=TEXT("file_not_written");return false;}Bytes=Data.Num();
        static const uint8 Signature[8]={137,80,78,71,13,10,26,10};
        if(Data.Num()<45 || FMemory::Memcmp(Data.GetData(),Signature,8)){Detail=TEXT("invalid_PNG_signature_or_short");return false;}
        auto U32=[&](int64 I){return (uint32(Data[I])<<24)|(uint32(Data[I+1])<<16)|(uint32(Data[I+2])<<8)|uint32(Data[I+3]);};
        bool Header=false,Pixels=false;int64 At=8;
        while(At+12<=Data.Num())
        {
            const uint32 Length=U32(At),Type=U32(At+4);const int64 End=At+12+int64(Length);
            if(End>Data.Num()){Detail=TEXT("truncated_PNG_chunk");return false;}
            if(Type==0x49484452u)
            {
                if(Header || At!=8 || Length!=13){Detail=TEXT("invalid_IHDR");return false;}Header=true;
                const uint32 W=U32(At+8),H=U32(At+12);if(!W || !H || W>16384 || H>16384){Detail=TEXT("invalid_dimensions");return false;}Size=FIntPoint(int32(W),int32(H));
            }
            else if(Type==0x49444154u)Pixels|=Length>0;
            else if(Type==0x49454e44u)
            {
                const bool Valid=Header && Pixels && Length==0 && End==Data.Num() && Size==CaptureExpectedSize;
                Detail=Valid?TEXT("complete_IHDR_IDAT_IEND_matches_native_viewport"):TEXT("IEND_or_native_dimensions_mismatch");return Valid;
            }
            At=End;
        }
        Detail=TEXT("IEND_not_written_yet");return false;
    }
    bool CaptureDeliveredViews()
    {
        auto* PC=World.IsValid()?World->GetFirstPlayerController():nullptr;
        if(!PC){Test->AddError(TEXT("Contact capture lost player controller"));RestoreCaptureCamera();return true;}
        constexpr int32 ViewCount=8,SettleFrames=16,MaximumPollFrames=120;
        if(CaptureView>=ViewCount){RestoreCaptureCamera();return true;}
        const TCHAR* Labels[]={TEXT("ArrelR5Front"),TEXT("ArrelR5Profile"),TEXT("ArrelR5Back"),TEXT("ArrelR5HeldGuard"),TEXT("ArrelR4WalkToeOffBefore"),TEXT("ArrelR4WalkToeOffAfter"),TEXT("ArrelR4LandingBefore"),TEXT("ArrelR4LandingAfter")};
        if(CaptureRunFolder.IsEmpty())
        {
            CaptureRunFolder=FPaths::ProjectSavedDir()/TEXT("Validation/HeroContactS362/CapturesR5")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*CaptureRunFolder,true);
            PreviousViewTarget=PC->GetViewTarget();CaptureCamera=World->SpawnActor<ACameraActor>();HideOtherFieldFiguresForCapture();
            Test->TestTrue(TEXT("Native contact captures use delivered R5 steel mesh"),Blade->GetStaticMesh()->GetPathName().Contains(TEXT("HeroContactS362/R5/")));
        }
        if(!CaptureCamera.IsValid()){Test->AddError(TEXT("Contact capture camera could not spawn"));RestoreCaptureCamera();return true;}
        if(CaptureViewFrame==0)
        {
            Figure->StopAction();Figure->AdvanceLocomotion(FVector::ZeroVector,1.f);Figure->SetSwordDrawn(true);
            if(CaptureView==3)
            {
                Test->TestTrue(TEXT("Native contact capture loads actual held Block clip"),Figure->PlayAction(TEXT("Block"),1.f,true,MemoriaCombatTuning::BlockHoldAt));
                Figure->TickComponent(Figure->GetActionLength()*MemoriaCombatTuning::BlockHoldAt,LEVELTICK_All,nullptr);
            }
        }
        auto* Skin=Figure->GetSkeletalMesh();Skin->TickAnimation(0.f,false);Skin->RefreshBoneTransforms();
        const bool bWalkDetail=CaptureView>=4;
        const float CaptureWalkPhase=CaptureView==4?.58f:CaptureView==5?.615f:CaptureView==6?.9914f:.0086f;
        if(bWalkDetail)
        {
            auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance());
            if(!Anim){Test->AddError(TEXT("Walk detail capture requires actual proxy"));RestoreCaptureCamera();return true;}
            Anim->Action=nullptr;Anim->ActionWeight=0.f;Anim->Weight=1.f;Anim->Phase=CaptureWalkPhase;
            Skin->TickAnimation(0.f,false);Skin->RefreshBoneTransforms();
        }
        const FVector Target=Actor->GetActorLocation()+FVector(0,0,bWalkDetail?20.f:150.f*.45f);
        const FVector Direction=CaptureView==0?Skin->GetForwardVector():CaptureView==1?Skin->GetRightVector():CaptureView==2?-Skin->GetForwardVector():(Skin->GetRightVector()+Skin->GetForwardVector()*.35).GetSafeNormal();
        const FVector Eye=Target+Direction*(150.f*(bWalkDetail?1.5f:2.8f))+FVector(0,0,150.f*.03f);
        CaptureCamera->SetActorLocation(Eye);CaptureCamera->SetActorRotation((Target-Eye).Rotation());CaptureCamera->GetCameraComponent()->SetFieldOfView(42.f);PC->SetViewTarget(CaptureCamera.Get());
        ++CaptureViewFrame;
        if(!bCaptureRequested && CaptureViewFrame>=SettleFrames)
        {
            CaptureFile=CaptureRunFolder/(FString(Labels[CaptureView])+TEXT(".png"));bCaptureAbsentBefore=!IFileManager::Get().FileExists(*CaptureFile);
            if(!bCaptureAbsentBefore){Test->AddError(TEXT("Unique capture output unexpectedly exists: ")+CaptureFile);RestoreCaptureCamera();return true;}
            PC->GetViewportSize(CaptureExpectedSize.X,CaptureExpectedSize.Y);bCaptureProcessed=false;CapturePollFrames=0;
            CaptureProcessedHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda([this](){bCaptureProcessed=true;});
            bCaptureRequested=true;FScreenshotRequest::RequestScreenshot(CaptureFile,false,false);
            Test->AddInfo(TEXT("S362_CONTACT_NATIVE_CAPTURE ")+CaptureFile+TEXT(" settled_native_frames=16 new_GUID_output=1 native_viewport_UI_excluded=1"));return false;
        }
        if(!bCaptureRequested)return false;
        ++CapturePollFrames;FIntPoint Size=FIntPoint::ZeroValue;int64 Bytes=0;FString Detail;
        const bool Complete=CompleteCapturePNG(Size,Bytes,Detail);
        if(bCaptureProcessed && Complete)
        {
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("view"),Labels[CaptureView]);Row->SetStringField(TEXT("file"),CaptureFile);
            Row->SetBoolField(TEXT("absent_before_request"),bCaptureAbsentBefore);Row->SetBoolField(TEXT("native_processed_callback"),bCaptureProcessed);
            Row->SetBoolField(TEXT("complete_PNG_IHDR_IDAT_IEND"),Complete);Row->SetNumberField(TEXT("width"),Size.X);Row->SetNumberField(TEXT("height"),Size.Y);Row->SetNumberField(TEXT("bytes"),double(Bytes));
            Row->SetNumberField(TEXT("processed_poll_frames"),CapturePollFrames);Row->SetStringField(TEXT("actual_mesh"),Skin->GetSkeletalMeshAsset()->GetPathName());
            if(bWalkDetail){Row->SetNumberField(TEXT("controlled_walk_phase"),double(CaptureWalkPhase));Row->SetNumberField(TEXT("controlled_walk_weight"),1.);}Row->SetNumberField(TEXT("other_figure_parts_temporarily_hidden"),HiddenFigureParts.Num());CaptureEvidence.Add(MakeShared<FJsonValueObject>(Row));
            Test->AddInfo(FString::Printf(TEXT("S362_CONTACT_CAPTURE_COMPLETE %s pixels=%dx%d bytes=%lld callback=1 polls=%d"),*CaptureFile,Size.X,Size.Y,Bytes,CapturePollFrames));
            RemoveCaptureCallback();++CaptureView;CaptureViewFrame=0;bCaptureRequested=false;
            if(CaptureView>=ViewCount){RestoreCaptureCamera();return true;}return false;
        }
        if(CapturePollFrames>=MaximumPollFrames)
        {Test->AddError(FString::Printf(TEXT("Native contact capture did not complete: %s callback=%d detail=%s size=%dx%d expected=%dx%d"),*CaptureFile,bCaptureProcessed,*Detail,Size.X,Size.Y,CaptureExpectedSize.X,CaptureExpectedSize.Y));RestoreCaptureCamera();return true;}
        return false;
    }

    bool ReadCurrentPose(TArray<FFinalSkinVertex>& Out)
    {
        auto* Skin=Figure->GetSkeletalMesh();const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];
        TArray<FMatrix44f> Matrices;Skin->GetCurrentRefToLocalMatrices(Matrices,0);
        TArray<FVector3f> Positions;USkeletalMeshComponent::ComputeSkinnedPositions(Skin,Positions,Matrices,LOD,*LOD.GetSkinWeightVertexBuffer());
        Out.SetNum(Positions.Num());for(int32 I=0;I<Positions.Num();++I)Out[I].Position=Positions[I];return !Out.IsEmpty();
    }

    void FindScabbardMouth()
    {
        if(!Scabbard.IsValid() || !Scabbard->GetStaticMesh() || !Scabbard->GetStaticMesh()->GetRenderData()
            || Scabbard->GetStaticMesh()->GetRenderData()->LODResources.IsEmpty())
        {Test->AddError(TEXT("Mouth cant probe requires actual native scabbard LOD geometry"));return;}
        const auto& Positions=Scabbard->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
        FBox Bounds(ForceInit);for(uint32 V=0;V<Positions.GetNumVertices();++V)Bounds+=FVector(Positions.VertexPosition(V));
        const FVector Span=Bounds.GetSize();int32 Axis=0;
        for(int32 I=1;I<3;++I)if(Span[I]>Span[Axis])Axis=I;
        // The original sleeve, unlike the combined sheathed sword, has no
        // pommel. Its near-origin terminal plane is the actual open mouth:
        // source13.5cm from grip0, with an81.0025cm sleeve below it. Extract
        // from native positions so no legacy/Interchange import basis is assumed.
        const double MouthEnd=FMath::Abs(Bounds.Min[Axis])<FMath::Abs(Bounds.Max[Axis])?Bounds.Min[Axis]:Bounds.Max[Axis];
        const double TipEnd=MouthEnd==Bounds.Min[Axis]?Bounds.Max[Axis]:Bounds.Min[Axis];
        FBox MouthBounds(ForceInit),TipBounds(ForceInit);TSet<FIntVector> MouthPoints,TipPoints;
        for(uint32 V=0;V<Positions.GetNumVertices();++V)
        {
            const FVector P(Positions.VertexPosition(V));
            const FIntVector Key(FMath::RoundToInt(P.X*10000),FMath::RoundToInt(P.Y*10000),FMath::RoundToInt(P.Z*10000));
            if(FMath::Abs(P[Axis]-MouthEnd)<=.02 && !MouthPoints.Contains(Key)){MouthPoints.Add(Key);MouthBounds+=P;}
            if(FMath::Abs(P[Axis]-TipEnd)<=.02 && !TipPoints.Contains(Key)){TipPoints.Add(Key);TipBounds+=P;}
        }
        if(MouthBounds.IsValid){MouthLocal=MouthBounds.GetCenter();MouthLocal[Axis]=MouthEnd;}
        if(TipBounds.IsValid){TipLocal=TipBounds.GetCenter();TipLocal[Axis]=TipEnd;}
        const int32 A=(Axis+1)%3,B=(Axis+2)%3;
        const FVector RimSpan=MouthBounds.IsValid?MouthBounds.GetSize():FVector::ZeroVector;
        bMouthValid=MouthPoints.Num()>=4 && TipPoints.Num()>=2 && Span[Axis]>80. && Span[Axis]<82.
            && MouthLocal.Size()>13. && MouthLocal.Size()<14.
            && RimSpan[A]>1. && RimSpan[B]>1. && RimSpan[A]<7. && RimSpan[B]<7.;
        Test->TestTrue(TEXT("Cant pivot resolves the actual native sleeve mouth, not sword grip/pommel"),bMouthValid);
        Test->TestTrue(TEXT("Delivered value helper uses the measured immutable native sleeve mouth"),bMouthValid && MouthLocal.Equals(FMemoriaHeroPropMount::NativeScabbardMouth(),.001));
        MouthGeometry=MakeShared<FJsonObject>();MouthGeometry->SetStringField(TEXT("source_asset"),GetPathNameSafe(Scabbard->GetStaticMesh()));
        MouthGeometry->SetStringField(TEXT("extraction"),TEXT("Native sleeve dominant-axis near-origin terminal plane; unique positions within.02cm; transverse bounds center"));
        MouthGeometry->SetNumberField(TEXT("native_long_axis"),Axis);MouthGeometry->SetNumberField(TEXT("native_sleeve_length_cm"),Span[Axis]);
        MouthGeometry->SetStringField(TEXT("mouth_native_cm"),MouthLocal.ToString());MouthGeometry->SetStringField(TEXT("tip_native_cm"),TipLocal.ToString());
        MouthGeometry->SetNumberField(TEXT("mouth_to_native_grip_origin_cm"),MouthLocal.Size());
        MouthGeometry->SetNumberField(TEXT("mouth_unique_positions"),MouthPoints.Num());MouthGeometry->SetNumberField(TEXT("tip_unique_positions"),TipPoints.Num());
        MouthGeometry->SetStringField(TEXT("mouth_rim_span_native_cm"),RimSpan.ToString());MouthGeometry->SetBoolField(TEXT("validated"),bMouthValid);
    }

    TArray<TSharedPtr<FJsonValue>> ProbeMouthCant(UStaticMeshComponent* Mounted,const FSurfaceGrid& Body)
    {
        TArray<TSharedPtr<FJsonValue>> Result;if(!bMouthValid)return Result;
        const FTransform Original=Mounted->GetRelativeTransform();
        const FTransform ScabbardReference=FrozenOriginalMount*ReferencePelvis;
        const FTransform MountedReference=ScabbardReference;
        const FVector MouthReference=ScabbardReference.TransformPosition(MouthLocal),TipReference=ScabbardReference.TransformPosition(TipLocal);
        // Both props share the sleeve mouth pivot. In particular, NEVER derive
        // the sheathed pivot from its highest vertex (the unrelated pommel).
        const FVector MountedMouthLocal=MountedReference.InverseTransformPosition(MouthReference);
        const FVector MountedTipLocal=MountedReference.InverseTransformPosition(TipReference);
        const FVector Outward(0,-1,0),Axis(-1,0,0);
        const FQuat SmallCant(Axis,FMath::DegreesToRadians(5.));
        const double SmallTipOutward=FVector::DotProduct(SmallCant.RotateVector(TipReference-MouthReference)-(TipReference-MouthReference),Outward);
        Test->TestTrue(TEXT("Reference -X cant swings the native lower sleeve outward toward left -Y"),SmallTipOutward>1.);
        if(SmallTipOutward<=1.)return Result;
        auto* Skin=Figure->GetSkeletalMesh();
        const FTransform CurrentPelvisWorld=Skin->GetSocketTransform(Mounted->GetAttachSocketName(),RTS_World);
        const FVector OriginalMouthWorld=CurrentPelvisWorld.TransformPosition(FrozenOriginalMount.TransformPosition(MountedMouthLocal));
        for(double Degrees:{0.,5.,10.,15.})for(double Translation:{0.,2.,4.,6.})
        {
            const FQuat Cant(Axis,FMath::DegreesToRadians(Degrees));const FVector Delta=Outward*Translation;
            FTransform CandidateReference=MountedReference;
            CandidateReference.SetRotation((Cant*MountedReference.GetRotation()).GetNormalized());
            CandidateReference.SetTranslation(MouthReference+Delta+Cant.RotateVector(MountedReference.GetTranslation()-MouthReference));
            const FTransform Candidate=Degrees==0. && Translation==0.?FrozenOriginalMount:CandidateReference.GetRelativeTransform(ReferencePelvis);
            Mounted->SetRelativeTransform(Candidate);Mounted->UpdateComponentToWorld();
            const FVector MouthWorld=Mounted->GetComponentTransform().TransformPosition(MountedMouthLocal);
            const FVector TipWorld=Mounted->GetComponentTransform().TransformPosition(MountedTipLocal);
            const FVector ExpectedMouthWorld=CurrentPelvisWorld.TransformPosition(ReferencePelvis.InverseTransformPosition(MouthReference+Delta));
            const double PivotError=FVector::Dist(MouthWorld,ExpectedMouthWorld);
            Test->TestTrue(TEXT("Cant rotates about the actual mouth; only explicit outward translation moves its belt point"),PivotError<.001);
            double MouthSurfaceDistance=DBL_MAX;
            for(const auto& Triangle:Body.Triangles)
                MouthSurfaceDistance=FMath::Min(MouthSurfaceDistance,FVector::Dist(MouthWorld,FMath::ClosestPointOnTriangleToPoint(MouthWorld,Triangle.P[0],Triangle.P[1],Triangle.P[2])));
            // Keep the original grip-origin4cm exclusion for direct comparison
            // to all previous results. Also report the physically relevant4cm
            // mouth/belt exclusion separately; it never replaces old criteria.
            const FContact Contact=MeasureProp(Mounted,Body,Mounted->GetComponentLocation(),4.0);
            const FContact AtMouth=MeasureProp(Mounted,Body,MouthWorld,4.0);
            auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("outward_cant_degrees"),Degrees);
            Item->SetStringField(TEXT("reference_cant_axis"),Axis.ToString());Item->SetStringField(TEXT("reference_component_delta_cm"),Delta.ToString());
            Item->SetStringField(TEXT("mouth_pivot_reference_cm"),MouthReference.ToString());Item->SetStringField(TEXT("mouth_world_cm"),MouthWorld.ToString());
            Item->SetStringField(TEXT("original_mouth_world_cm"),OriginalMouthWorld.ToString());Item->SetStringField(TEXT("tip_world_cm"),TipWorld.ToString());
            Item->SetNumberField(TEXT("mouth_world_shift_cm"),FVector::Dist(MouthWorld,OriginalMouthWorld));Item->SetNumberField(TEXT("mouth_pivot_error_cm"),PivotError);
            Item->SetNumberField(TEXT("mouth_to_body_surface_cm"),MouthSurfaceDistance);
            Item->SetNumberField(TEXT("tip_outward_motion_from_cant_reference_cm"),FVector::DotProduct(Cant.RotateVector(TipReference-MouthReference)-(TipReference-MouthReference),Outward));
            Item->SetNumberField(TEXT("crossed_prop_triangles"),Contact.CrossedPropTriangles);Item->SetNumberField(TEXT("near_clearance_triangles"),Contact.NearPropTriangles);
            Item->SetNumberField(TEXT("clearance_lower_bound_cm"),Contact.Minimum==DBL_MAX?Clearance:Contact.Minimum);
            Item->SetNumberField(TEXT("legacy_grip_zone_exempt_triangles"),Contact.MountExemptTriangles);
            Item->SetNumberField(TEXT("mouth_zone_crossed_prop_triangles"),AtMouth.CrossedPropTriangles);Item->SetNumberField(TEXT("mouth_zone_near_clearance_triangles"),AtMouth.NearPropTriangles);
            Item->SetNumberField(TEXT("mouth_zone_exempt_triangles"),AtMouth.MountExemptTriangles);
            Item->SetStringField(TEXT("crossing_bounds_world"),Contact.Crossings.IsValid?Contact.Crossings.ToString():TEXT("none"));
            Result.Add(MakeShared<FJsonValueObject>(Item));
            // Restore after EVERY candidate so no previous cant can accumulate.
            Mounted->SetRelativeTransform(Original);Mounted->UpdateComponentToWorld();
            Test->TestTrue(TEXT("Mouth cant probe restores the exact original prop transform"),Mounted->GetRelativeTransform().Equals(Original,.00001f));
        }
        return Result;
    }

    FContact MeasureProp(UStaticMeshComponent* Prop,const FSurfaceGrid& Body,const FVector& Exempt,double ExemptRadius,bool bDrawnSword=false)
    {
        FContact Result; if(!Prop || !Prop->GetStaticMesh() || !Prop->GetStaticMesh()->GetRenderData()) return Result;
        Prop->UpdateComponentToWorld();
        const auto& LOD=Prop->GetStaticMesh()->GetRenderData()->LODResources[0];
        const auto Indices=LOD.IndexBuffer.GetArrayView(); const FTransform Transform=Prop->GetComponentTransform();
        TArray<int32> MaterialAtTriangle;MaterialAtTriangle.SetNumZeroed(Indices.Num()/3);
        for(const auto& Section:LOD.Sections)
            for(uint32 T=0;T<Section.NumTriangles;++T)MaterialAtTriangle[Section.FirstIndex/3+T]=Section.MaterialIndex;
        TSet<int32> Candidates;
        for(int32 T=0;T<Indices.Num()/3;++T)
        {
            FVector P[3]; for(int32 I=0;I<3;++I) P[I]=Transform.TransformPosition(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[T*3+I])));
            if(P[0].ContainsNaN() || P[1].ContainsNaN() || P[2].ContainsNaN()) { Test->AddError(TEXT("Contact probe found non-finite prop vertices")); continue; }
            // The handle intentionally touches the fist; a pelvis mount may
            // touch its belt. Exemption is geometrically bounded, not all hands/cape.
            if(FVector::Dist(P[0],Exempt)<ExemptRadius && FVector::Dist(P[1],Exempt)<ExemptRadius && FVector::Dist(P[2],Exempt)<ExemptRadius)
            { ++Result.MountExemptTriangles; continue; }
            FTriangle Triangle(P[0],P[1],P[2]); ++Result.TestedTriangles;
            Candidates.Reset(); Body.Candidates(Triangle.Bounds.ExpandBy(Clearance),Candidates);
            bool bCrossed=false,bNear=false;
            for(int32 Index:Candidates)
            {
                const auto& Other=Body.Triangles[Index];
                if(BoxDistanceSquared(Triangle.Bounds,Other.Bounds)>Clearance*Clearance) continue;
                FVector Point; bool bCrossing=false; const double Distance=TriangleDistance(Triangle,Other,Point,bCrossing);
                Result.Minimum=FMath::Min(Result.Minimum,Distance); bNear|=Distance<Clearance;
                if(bCrossing && FVector::Dist(Point,Exempt)>ExemptRadius)
                { bCrossed=true; Result.Crossings+=Point; }
            }
            Result.CrossedPropTriangles+=bCrossed; Result.NearPropTriangles+=bNear;
            if(bDrawnSword && bCrossed)
            {
                const auto& Slots=Prop->GetStaticMesh()->GetStaticMaterials();const int32 Material=MaterialAtTriangle[T];
                const FString Slot=Slots.IsValidIndex(Material)?Slots[Material].MaterialSlotName.ToString():TEXT("");
                const bool bSteel=Material==1 || Slot.Contains(TEXT("Blade")) || Slot.Contains(TEXT("Steel"));
                if(bSteel)++Result.SteelCrossings;else ++Result.HiltCrossings;
            }
        }
        return Result;
    }
    void FindSoles()
    {
        auto* Skin=Figure->GetSkeletalMesh(); const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];
        const auto& Ref=Skin->GetSkeletalMeshAsset()->GetRefSkeleton(); const auto* Weights=LOD.GetSkinWeightVertexBuffer();
        TArray<uint32> Feet[2]; double Minimum[2]={DBL_MAX,DBL_MAX};
        for(const auto& Section:LOD.RenderSections)
        {
            const TConstArrayView<FBoneIndexType> Map=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
            for(uint32 V=Section.BaseVertexIndex;V<Section.BaseVertexIndex+Section.NumVertices;++V)
            {
                uint32 Total=0,Foot[2]={0,0};
                for(uint32 I=0;I<Weights->GetMaxBoneInfluences();++I)
                {
                    const uint32 W=Weights->GetBoneWeight(V,I),Bone=Weights->GetBoneIndex(V,I); Total+=W;
                    if(!W || Bone>=uint32(Map.Num())) continue;
                    const FName Name=Ref.GetBoneName(Map[Bone]);
                    if(Name==TEXT("foot_l") || Name==TEXT("ball_l")) Foot[0]+=W;
                    if(Name==TEXT("foot_r") || Name==TEXT("ball_r")) Foot[1]+=W;
                }
                const auto P=LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V);
                for(int32 Side=0;Side<2;++Side) if(Total && Foot[Side]>Total*.50 && P.Z<36.f)
                { Feet[Side].Add(V); Minimum[Side]=FMath::Min(Minimum[Side],double(P.Z)); }
            }
        }
        for(int32 Side=0;Side<2;++Side)
        {
            for(uint32 V:Feet[Side]) if(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V).Z<Minimum[Side]+.65) Sole[Side].Add(V);
            // Raw render vertex count depends on UV/normal splits. The pinned
            // source legitimately has10distinct points in this unchanged band.
            // Require an actual non-collinear heel support patch instead.
            TSet<FIntVector> Unique;TArray<FVector> Positions;FBox Bounds(ForceInit);
            for(uint32 V:Sole[Side])
            {
                const FVector P(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V));
                const FIntVector Key(FMath::RoundToInt(P.X*10000),FMath::RoundToInt(P.Y*10000),FMath::RoundToInt(P.Z*10000));
                if(!Unique.Contains(Key)){Unique.Add(Key);Positions.Add(P);Bounds+=P;}
            }
            int32 HullVertices=0;const double Area=SoleSupportArea(Positions,HullVertices);
            const FVector Span=Bounds.IsValid?Bounds.GetSize():FVector::ZeroVector;
            Test->TestTrue(TEXT("Native boot sole has at least6distinct points and a real bounded2D heel support patch"),
                Unique.Num()>=6 && HullVertices>=3 && Area>=5. && FMath::Min(Span.X,Span.Y)>=2. && FMath::Max(Span.X,Span.Y)>=3.);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("side"),Side==0?TEXT("left"):TEXT("right"));
            Row->SetNumberField(TEXT("weighted_foot_render_vertices"),Feet[Side].Num());
            Row->SetNumberField(TEXT("sole_render_vertices"),Sole[Side].Num());Row->SetNumberField(TEXT("distinct_sole_positions"),Unique.Num());
            Row->SetNumberField(TEXT("source_band_cm"),.65);Row->SetNumberField(TEXT("reference_minimum_z_cm"),Minimum[Side]==DBL_MAX?0.:Minimum[Side]);
            Row->SetNumberField(TEXT("support_span_x_cm"),Span.X);Row->SetNumberField(TEXT("support_span_y_cm"),Span.Y);Row->SetNumberField(TEXT("support_span_z_cm"),Span.Z);
            Row->SetNumberField(TEXT("convex_support_hull_vertices"),HullVertices);Row->SetNumberField(TEXT("convex_support_area_xy_cm2"),Area);
            Row->SetStringField(TEXT("reference_sole_bounds"),Bounds.IsValid?Bounds.ToString():TEXT("empty"));
            TArray<TSharedPtr<FJsonValue>> SourceVertices;
            for(uint32 V:Sole[Side])
            {
                auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("native_vertex_ID"),V);
                const FVector P(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V));Item->SetStringField(TEXT("reference_position_cm"),P.ToString());
                TArray<TSharedPtr<FJsonValue>> Influences;
                for(const auto& Section:LOD.RenderSections)if(V>=Section.BaseVertexIndex && V<Section.BaseVertexIndex+Section.NumVertices)
                {
                    const uint32 WeightIndex=Section.GetVertexBufferIndex()+V-Section.BaseVertexIndex;
                    Item->SetNumberField(TEXT("skin_weight_buffer_index"),WeightIndex);Item->SetBoolField(TEXT("weight_and_position_indices_match"),WeightIndex==V);
                    const auto Map=Section.BoneMap.IsEmpty()?LOD.GetUnifiedBoneMap():TConstArrayView<FBoneIndexType>(Section.BoneMap);
                    for(uint32 I=0;I<Weights->GetMaxBoneInfluences();++I)
                    {
                        const uint32 W=Weights->GetBoneWeight(WeightIndex,I),Bone=Weights->GetBoneIndex(WeightIndex,I);if(!W || Bone>=uint32(Map.Num()))continue;
                        auto Influence=MakeShared<FJsonObject>();Influence->SetStringField(TEXT("bone"),Ref.GetBoneName(Map[Bone]).ToString());Influence->SetNumberField(TEXT("raw_weight"),W);Influences.Add(MakeShared<FJsonValueObject>(Influence));
                    }
                    break;
                }
                Item->SetArrayField(TEXT("native_influences"),Influences);SourceVertices.Add(MakeShared<FJsonValueObject>(Item));
            }
            Row->SetArrayField(TEXT("source_sole_vertices"),SourceVertices);SoleGeometry.Add(MakeShared<FJsonValueObject>(Row));
            Test->AddInfo(FString::Printf(TEXT("S362_SOLE_GEOMETRY side=%s weighted=%d render=%d distinct=%d hull=%d area_cm2=%.6f span_cm=%s band_cm=.65"),
                Side==0?TEXT("left"):TEXT("right"),Feet[Side].Num(),Sole[Side].Num(),Unique.Num(),HullVertices,Area,*Span.ToString()));
        }
    }
    void CheckFootContact(const TArray<FFinalSkinVertex>& Vertices,bool bWalk,bool bAssertSupport,const FString& Pose)
    {
        auto* Skin=Figure->GetSkeletalMesh(); const auto Transform=Skin->GetComponentTransform();
        const double Ground=Actor->GetActorLocation().Z-8.; double Minimum[2]={DBL_MAX,DBL_MAX},Maximum[2]={-DBL_MAX,-DBL_MAX};
        FVector Center[2]={FVector::ZeroVector,FVector::ZeroVector}; bool Support[2]={false,false};
        const float CurrentWalkWeight=Figure->LocomotionWeight();
        const bool bSteadyWalk=CurrentWalkWeight>.98f && PreviousWalkWeight>.98f;
        for(int32 Side=0;Side<2;++Side)
        {
            for(uint32 V:Sole[Side])
            { const FVector P=Transform.TransformPosition(FVector(Vertices[V].Position)); Minimum[Side]=FMath::Min(Minimum[Side],P.Z); Maximum[Side]=FMath::Max(Maximum[Side],P.Z); Center[Side]+=P; }
            if(!Sole[Side].IsEmpty()) Center[Side]/=Sole[Side].Num();
            Support[Side]=FMath::Abs(Minimum[Side]-Ground)<.75 && Maximum[Side]-Minimum[Side]<.85;
            double Drift=0.;
            if(bWalk && bPreviousWalkSample && Support[Side] && PreviousSupport[Side] && bSteadyWalk)
            { MaximumCentroidFootDrift=FMath::Max(MaximumCentroidFootDrift,FVector::Dist2D(Center[Side],PreviousSole[Side])); ++CentroidGroundedSamples; }
            if(bWalk)
            {
                // Contact means the SAME real sole vertex IDs are within1mm
                // of the unchanged plane at both endpoints. A near-ground
                // landing/rolling band centroid is not a planted contact proxy.
                TMap<uint32,FVector> CurrentVertices;TArray<FVector> CurrentPatch,PreviousPatch;TSet<FIntVector> DistinctPatch;
                TArray<TSharedPtr<FJsonValue>> VertexRows,PatchRows;double PatchMaximumDrift=0.;
                const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];
                auto VectorValue=[](const FVector& P)
                {TArray<TSharedPtr<FJsonValue>> A;A.Add(MakeShared<FJsonValueNumber>(P.X));A.Add(MakeShared<FJsonValueNumber>(P.Y));A.Add(MakeShared<FJsonValueNumber>(P.Z));return A;};
                for(uint32 V:Sole[Side])
                {
                    const FVector P=Transform.TransformPosition(FVector(Vertices[V].Position));CurrentVertices.Add(V,P);
                    auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("native_vertex_ID"),V);Item->SetArrayField(TEXT("world_position_cm"),VectorValue(P));Item->SetNumberField(TEXT("height_over_ground_cm"),P.Z-Ground);VertexRows.Add(MakeShared<FJsonValueObject>(Item));
                    const FVector* Previous=PreviousSoleVertices[Side].Find(V);
                    if(!bPreviousWalkSample || !Previous || P.ContainsNaN() || Previous->ContainsNaN() || FMath::Abs(P.Z-Ground)>.1 || FMath::Abs(Previous->Z-Ground)>.1)continue;
                    const FVector Ref(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V));const FIntVector RefKey(FMath::RoundToInt(Ref.X*10000),FMath::RoundToInt(Ref.Y*10000),FMath::RoundToInt(Ref.Z*10000));
                    if(DistinctPatch.Contains(RefKey))continue;DistinctPatch.Add(RefKey);CurrentPatch.Add(P);PreviousPatch.Add(*Previous);
                    const double VertexDrift=FVector::Dist2D(P,*Previous);PatchMaximumDrift=FMath::Max(PatchMaximumDrift,VertexDrift);
                    auto Contact=MakeShared<FJsonObject>();Contact->SetNumberField(TEXT("native_vertex_ID"),V);Contact->SetArrayField(TEXT("previous_world_position_cm"),VectorValue(*Previous));Contact->SetArrayField(TEXT("current_world_position_cm"),VectorValue(P));Contact->SetNumberField(TEXT("world_drift_cm"),VertexDrift);PatchRows.Add(MakeShared<FJsonValueObject>(Contact));
                }
                int32 CurrentHull=0,PreviousHull=0;const double CurrentArea=SoleSupportArea(CurrentPatch,CurrentHull),PreviousArea=SoleSupportArea(PreviousPatch,PreviousHull);
                // Area epsilon only rejects collinear/numerical-zero geometry;
                // no horizontal velocity/phase exclusion can hide real slip.
                const bool PersistentPatch=bPreviousWalkSample && bSteadyWalk && DistinctPatch.Num()>=3 && CurrentHull>=3 && PreviousHull>=3 && CurrentArea>1e-4 && PreviousArea>1e-4;
                if(PersistentPatch){MaximumFootDrift=FMath::Max(MaximumFootDrift,PatchMaximumDrift);++GroundedFootSamples;}
                Drift=bPreviousWalkSample?FVector::Dist2D(Center[Side],PreviousSole[Side]):0.;
                if(bPreviousWalkSample && Support[Side] && PreviousSupport[Side] && !bSteadyWalk)MaximumWalkEntrySupportedMotion=FMath::Max(MaximumWalkEntrySupportedMotion,Drift);
                auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("frame"),Frame);Row->SetStringField(TEXT("side"),Side==0?TEXT("left"):TEXT("right"));
                Row->SetNumberField(TEXT("phase"),Figure->GaitPhase());Row->SetNumberField(TEXT("locomotion_weight"),Figure->LocomotionWeight());
                Row->SetNumberField(TEXT("previous_locomotion_weight"),PreviousWalkWeight);Row->SetBoolField(TEXT("steady_locomotion"),bSteadyWalk);
                Row->SetStringField(TEXT("foot_bone_world_cm"),Skin->GetSocketLocation(Side==0?TEXT("foot_l"):TEXT("foot_r")).ToString());
                Row->SetStringField(TEXT("ball_bone_world_cm"),Skin->GetSocketLocation(Side==0?TEXT("ball_l"):TEXT("ball_r")).ToString());
                Row->SetStringField(TEXT("pelvis_world_cm"),Skin->GetSocketLocation(TEXT("pelvis")).ToString());
                Row->SetStringField(TEXT("sole_center_world_cm"),Center[Side].ToString());Row->SetNumberField(TEXT("sole_minimum_over_ground_cm"),Minimum[Side]-Ground);
                Row->SetNumberField(TEXT("sole_height_spread_cm"),Maximum[Side]-Minimum[Side]);Row->SetNumberField(TEXT("world_drift_cm"),Drift);
                Row->SetBoolField(TEXT("geometric_support"),Support[Side]);Row->SetBoolField(TEXT("previous_geometric_support"),bPreviousWalkSample && PreviousSupport[Side]);
                Row->SetBoolField(TEXT("old_band_centroid_supported_interval"),bPreviousWalkSample && Support[Side] && PreviousSupport[Side] && bSteadyWalk);
                Row->SetBoolField(TEXT("counted_planted_interval"),PersistentPatch);Row->SetNumberField(TEXT("persistent_distinct_contact_vertex_IDs"),DistinctPatch.Num());
                Row->SetNumberField(TEXT("persistent_current_patch_area_cm2"),CurrentArea);Row->SetNumberField(TEXT("persistent_previous_patch_area_cm2"),PreviousArea);
                Row->SetNumberField(TEXT("persistent_point_maximum_world_drift_cm"),PatchMaximumDrift);Row->SetArrayField(TEXT("persistent_contact_points"),PatchRows);Row->SetArrayField(TEXT("all_band_sole_vertices"),VertexRows);
                Row->SetStringField(TEXT("foot_component_transform"),Skin->GetSocketTransform(Side==0?TEXT("foot_l"):TEXT("foot_r"),RTS_Component).ToString());
                Row->SetStringField(TEXT("ball_component_transform"),Skin->GetSocketTransform(Side==0?TEXT("ball_l"):TEXT("ball_r"),RTS_Component).ToString());
                Row->SetObjectField(TEXT("sample_pose_and_clock"),PoseSnapshot());
                if(const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance()))Row->SetStringField(TEXT("authored_walk_clip"),GetPathNameSafe(Anim->Walk));
                WalkDiagnostics.Add(MakeShared<FJsonValueObject>(Row));
                PreviousSole[Side]=Center[Side];PreviousSupport[Side]=Support[Side];PreviousSoleVertices[Side]=MoveTemp(CurrentVertices);
            }
        }
        if(bWalk){bPreviousWalkSample=true;PreviousWalkWeight=CurrentWalkWeight;}
        if(bWalk) AirFrames+=Minimum[0]-Ground>1. && Minimum[1]-Ground>1.;
        if(bAssertSupport)
            Test->TestTrue(TEXT("Idle/walk has an actual boot sole within1cm of the unchanged ground anchor"),FMath::Min(Minimum[0],Minimum[1])-Ground<1. && FMath::Min(Minimum[0],Minimum[1])-Ground>-.75);
        Test->AddInfo(FString::Printf(TEXT("S362_SOLE_CONTACT pose=%s left_cm=%.6f right_cm=%.6f support=%d,%d"),*Pose,Minimum[0]-Ground,Minimum[1]-Ground,Support[0],Support[1]));
    }
    void SamplePose(const FString& Role,float Fraction,bool bAssertClear)
    {
        auto* Skin=Figure->GetSkeletalMesh(); Skin->TickAnimation(0.f,false); Skin->RefreshBoneTransforms();
        const auto BeforePose=PoseSnapshot();
        TArray<FFinalSkinVertex> Vertices;ReadCurrentPose(Vertices);
        if(Vertices.IsEmpty()) { Test->AddError(TEXT("Contact probe received no actual CPU-skinned body")); return; }
        const auto& LOD=Skin->GetSkeletalMeshRenderData()->LODRenderData[0];
        const auto* Indices=LOD.MultiSizeIndexContainer.GetIndexBuffer(); const auto Transform=Skin->GetComponentTransform();
        FSurfaceGrid Body;
        for(const auto& Section:LOD.RenderSections)
            for(uint32 T=0;T<Section.NumTriangles;++T)
            {
                FVector P[3]; for(int32 I=0;I<3;++I) P[I]=Transform.TransformPosition(FVector(Vertices[Indices->Get(Section.BaseIndex+T*3+I)].Position));
                if(!P[0].ContainsNaN() && !P[1].ContainsNaN() && !P[2].ContainsNaN()) Body.Add(FTriangle(P[0],P[1],P[2]));
                else Test->AddError(TEXT("Contact probe body has non-finite posed triangles"));
            }
        // hand_r is a wrist joint. The actual grip origin was constructed from
        // the knuckles/palm by AttachGrip; center the intentional fist contact
        // exemption there rather than accidentally exempting the forearm.
        Blade->UpdateComponentToWorld();const FVector GripCenter=Blade->GetComponentLocation();
        const bool bSheathedPose=Role==TEXT("IdleSheathed");
        UStaticMeshComponent* Mounted=bSheathedPose?Sheathed.Get():Scabbard.Get();
        const FContact Drawn=bSheathedPose?FContact():MeasureProp(Blade.Get(),Body,GripCenter,7.0,true);
        const FContact Empty=MeasureProp(Mounted,Body,Mounted->GetComponentLocation(),4.0);
        const FTransform DeliveredMount=Mounted->GetRelativeTransform();
        UStaticMesh* const OriginalMountedAsset=Mounted->GetStaticMesh();
        const FString ExpectedMountedPath=bSheathedPose
            ?TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SM_Arrel_sword_sheathed.SM_Arrel_sword_sheathed")
            :TEXT("/Game/Memoria/Presentation/Field3D/Arrel/SM_Arrel_scabbard.SM_Arrel_scabbard");
        Test->TestEqual(TEXT("Mount comparison retains the accepted original painted sheathed/scabbard asset"),GetPathNameSafe(OriginalMountedAsset),ExpectedMountedPath);
        // Independent frozen S312 mount, evaluated against the SAME posed
        // native body and same prop asset. The helper is not used to compute
        // the baseline or its contact counts.
        Mounted->SetRelativeTransform(FrozenOriginalMount);Mounted->UpdateComponentToWorld();
        const FContact BaselineEmpty=MeasureProp(Mounted,Body,Mounted->GetComponentLocation(),4.0);
        const FContact BaselineDrawn=bSheathedPose?FContact():MeasureProp(Blade.Get(),Body,GripCenter,7.0,true);
        Mounted->SetRelativeTransform(DeliveredMount);Mounted->UpdateComponentToWorld();
        Test->TestTrue(TEXT("Original-mount comparison restores delivered prop and preserves its exact painted asset"),
            Mounted->GetRelativeTransform().Equals(DeliveredMount,.00001f) && Mounted->GetStaticMesh()==OriginalMountedAsset);
        // GetCPUSkinnedVertices itself refreshes/evaluates animation and swaps
        // render state. Read the current native bone matrices without another
        // evaluation so this compares prop edits, not a second action pose.
        TArray<FFinalSkinVertex> AfterComparison;ReadCurrentPose(AfterComparison);
        const auto AfterPose=PoseSnapshot();
        Test->TestEqual(TEXT("Prop-only comparison preserves exact native component-space bone transforms"),BeforePose->GetStringField(TEXT("bone_component_hash")),AfterPose->GetStringField(TEXT("bone_component_hash")));
        double MaximumBodyMotion=0.;
        if(AfterComparison.Num()==Vertices.Num())for(int32 I=0;I<Vertices.Num();++I)
            MaximumBodyMotion=FMath::Max(MaximumBodyMotion,double(FVector3f::Dist(Vertices[I].Position,AfterComparison[I].Position)));
        else MaximumBodyMotion=DBL_MAX;
        Test->TestTrue(TEXT("Prop-only comparison preserves all actual posed body/boot vertices and rig"),MaximumBodyMotion==0.);
        Blade->UpdateComponentToWorld(); const FTransform Expected=OriginalGrip*Skin->GetSocketTransform(TEXT("hand_r"),RTS_World);
        const double GripError=FVector::Dist(Expected.GetLocation(),Blade->GetComponentLocation());
        Test->TestTrue(TEXT("Contact sampling keeps the exact existing anatomical grip transform"),Blade->GetRelativeTransform().Equals(OriginalGrip,.0001f) && GripError<.1);
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("role"),Role); Row->SetNumberField(TEXT("fraction"),Fraction);
        Row->SetStringField(TEXT("effective_clip"),GetPathNameSafe(Figure->GetActionClip()));
        if(const auto* Anim=Cast<UMemoriaFieldAnimInstance>(Skin->GetAnimInstance()))
        {Row->SetNumberField(TEXT("presentation_age"),Anim->Age);Row->SetNumberField(TEXT("gait_phase"),Anim->Phase);Row->SetNumberField(TEXT("gait_weight"),Anim->Weight);Row->SetNumberField(TEXT("action_time"),Anim->ActionTime);Row->SetNumberField(TEXT("action_weight"),Anim->ActionWeight);}
        Row->SetStringField(TEXT("mounted_asset"),GetPathNameSafe(Mounted->GetStaticMesh()));
        Row->SetNumberField(TEXT("drawn_blade_crossed_prop_triangles"),Drawn.CrossedPropTriangles);
        Row->SetNumberField(TEXT("drawn_steel_crossed_triangles"),Drawn.SteelCrossings);
        Row->SetNumberField(TEXT("drawn_hilt_guard_crossed_triangles"),Drawn.HiltCrossings);
        Row->SetStringField(TEXT("intentional_grip_zone_center_world"),GripCenter.ToString());
        Row->SetNumberField(TEXT("scabbard_crossed_prop_triangles"),Empty.CrossedPropTriangles);
        Row->SetNumberField(TEXT("drawn_near_clearance_triangles"),Drawn.NearPropTriangles);
        Row->SetNumberField(TEXT("scabbard_near_clearance_triangles"),Empty.NearPropTriangles);
        Row->SetNumberField(TEXT("grip_world_error_cm"),GripError);
        Row->SetStringField(TEXT("drawn_crossing_bounds_world"),Drawn.Crossings.IsValid?Drawn.Crossings.ToString():TEXT("none"));
        Row->SetStringField(TEXT("scabbard_crossing_bounds_world"),Empty.Crossings.IsValid?Empty.Crossings.ToString():TEXT("none"));
        Row->SetNumberField(TEXT("drawn_mount_exempt_triangles"),Drawn.MountExemptTriangles);
        Row->SetNumberField(TEXT("scabbard_mount_exempt_triangles"),Empty.MountExemptTriangles);
        auto Comparison=MakeShared<FJsonObject>();
        Comparison->SetStringField(TEXT("baseline"),TEXT("Frozen original S312 desired yaw90, offset(0,-21,13), converted by original idle pelvis. Same asset and exact posed body; no clothing/grip changes."));
        Comparison->SetStringField(TEXT("baseline_relative_transform"),FrozenOriginalMount.ToString());Comparison->SetStringField(TEXT("delivered_relative_transform"),DeliveredMount.ToString());
        Comparison->SetNumberField(TEXT("baseline_crossed_prop_triangles"),BaselineEmpty.CrossedPropTriangles);Comparison->SetNumberField(TEXT("baseline_near_clearance_triangles"),BaselineEmpty.NearPropTriangles);
        Comparison->SetNumberField(TEXT("delivered_crossed_prop_triangles"),Empty.CrossedPropTriangles);Comparison->SetNumberField(TEXT("delivered_near_clearance_triangles"),Empty.NearPropTriangles);
        Comparison->SetNumberField(TEXT("crossed_triangle_reduction"),BaselineEmpty.CrossedPropTriangles-Empty.CrossedPropTriangles);Comparison->SetNumberField(TEXT("near_triangle_reduction"),BaselineEmpty.NearPropTriangles-Empty.NearPropTriangles);
        Comparison->SetNumberField(TEXT("baseline_drawn_crossed_triangles"),BaselineDrawn.CrossedPropTriangles);Comparison->SetNumberField(TEXT("baseline_drawn_near_triangles"),BaselineDrawn.NearPropTriangles);
        Comparison->SetNumberField(TEXT("posed_body_maximum_vertex_change_cm"),MaximumBodyMotion);
        Comparison->SetObjectField(TEXT("pose_before_matrix_skinning"),BeforePose);Comparison->SetObjectField(TEXT("pose_after_prop_comparison"),AfterPose);
        Comparison->SetBoolField(TEXT("residual_mounted_surface_contacts"),Empty.CrossedPropTriangles>0 || Empty.NearPropTriangles>0);
        Comparison->SetStringField(TEXT("scope"),TEXT("Measured reduction of inherited surface contacts, not zero cloth collision certification. All residual counts remain reported."));
        Row->SetObjectField(TEXT("original_mount_comparison"),Comparison);
        // Candidate translations are in the exact reference-component frame
        // consumed by AttachProp's offset. Probe, restore, and never mutate a
        // product mount or the anatomical drawn grip during this measurement.
        TArray<FVector> Deltas={FVector::ZeroVector};
        const bool bScanMount=Role.StartsWith(TEXT("Idle")) || Role==TEXT("BlockCurrentTuning")
            || (Role!=TEXT("Block") && FMath::IsNearlyEqual(Fraction,.45f));
        if(bScanMount)
        {
            for(double Y:{-2.,-4.,-6.,-8.,-10.})for(double X:{0.,2.,4.,6.})Deltas.Add(FVector(X,Y,0));
            for(double X:{-2.,-4.,-6.})Deltas.Add(FVector(X,0,0));
        }
        const FTransform OriginalMounted=Mounted->GetRelativeTransform();
        TArray<TSharedPtr<FJsonValue>> Candidates;
        for(const FVector& Delta:Deltas)
        {
            FTransform Candidate=FrozenOriginalMount;Candidate.AddToTranslation(ReferencePelvis.InverseTransformVector(Delta));
            Mounted->SetRelativeTransform(Candidate);Mounted->UpdateComponentToWorld();
            const FContact Contact=MeasureProp(Mounted,Body,Mounted->GetComponentLocation(),4.0);
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("reference_component_delta_cm"),Delta.ToString());
            Item->SetNumberField(TEXT("delta_length_cm"),Delta.Size());Item->SetNumberField(TEXT("crossed_prop_triangles"),Contact.CrossedPropTriangles);
            Item->SetNumberField(TEXT("near_clearance_triangles"),Contact.NearPropTriangles);
            Item->SetNumberField(TEXT("clearance_lower_bound_cm"),Contact.Minimum==DBL_MAX?Clearance:Contact.Minimum);
            Candidates.Add(MakeShared<FJsonValueObject>(Item));
        }
        Mounted->SetRelativeTransform(OriginalMounted);Mounted->UpdateComponentToWorld();
        Test->TestTrue(TEXT("Temporary mount scan restores the actual prop exactly"),Mounted->GetRelativeTransform().Equals(OriginalMounted,.00001f));
        Row->SetArrayField(TEXT("mounted_translation_candidates"),Candidates);
        Row->SetStringField(TEXT("candidate_scan_basis"),TEXT("Frozen original mount; delivered transform restored after each scan. Cant/translation values are not accumulated onto the delivered mount."));
        if(Role==TEXT("IdleSheathed") || Role==TEXT("IdleDrawn") || Role==TEXT("BlockCurrentTuning"))
            Row->SetArrayField(TEXT("mounted_mouth_cant_candidates"),ProbeMouthCant(Mounted,Body));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
        Test->AddInfo(FString::Printf(TEXT("S362_PROP_CONTACT role=%s fraction=%.3f drawn_crossings=%d steel=%d hilt_guard=%d sheath_crossings=%d near=%d,%d grip_error_cm=%.6f blade_bounds=%s sheath_bounds=%s"),
            *Role,Fraction,Drawn.CrossedPropTriangles,Drawn.SteelCrossings,Drawn.HiltCrossings,Empty.CrossedPropTriangles,Drawn.NearPropTriangles,Empty.NearPropTriangles,GripError,
            *Row->GetStringField(TEXT("drawn_crossing_bounds_world")),*Row->GetStringField(TEXT("scabbard_crossing_bounds_world"))));
        if(bAssertClear)
        {
            Test->TestTrue(TEXT("Mounted-prop improvement does not worsen actual drawn blade crossings or proximity"),
                Drawn.CrossedPropTriangles<=BaselineDrawn.CrossedPropTriangles && Drawn.NearPropTriangles<=BaselineDrawn.NearPropTriangles);
            const bool bCritical=Role==TEXT("IdleSheathed") || Role==TEXT("IdleDrawn") || Role==TEXT("BlockCurrentTuning");
            if(bCritical)
            {
                Test->TestTrue(TEXT("Critical idle/held guard strictly reduces inherited sheathed/scabbard surface crossings against frozen original mount"),Empty.CrossedPropTriangles<BaselineEmpty.CrossedPropTriangles);
                Test->TestTrue(TEXT("Critical idle/held guard strictly reduces inherited sheathed/scabbard proximity against frozen original mount"),Empty.NearPropTriangles<BaselineEmpty.NearPropTriangles);
            }
            else Test->TestTrue(TEXT("Real walk mounted contacts do not regress from the same-pose original mount"),
                Empty.CrossedPropTriangles<=BaselineEmpty.CrossedPropTriangles && Empty.NearPropTriangles<=BaselineEmpty.NearPropTriangles);
        }
        CheckFootContact(Vertices,false,Role.StartsWith(TEXT("Idle")) || Role==TEXT("Walk"),Role);
    }
    void Save()
    {
        auto Root=MakeShared<FJsonObject>(); Root->SetStringField(TEXT("schema"),TEXT("s362-native-prop-contact-v3-physical-patch"));
        Root->SetStringField(TEXT("mesh"),GetPathNameSafe(Figure->GetSkeletalMesh()->GetSkeletalMeshAsset()));
        Root->SetStringField(TEXT("blade"),GetPathNameSafe(Blade->GetStaticMesh())); Root->SetStringField(TEXT("scabbard"),GetPathNameSafe(Scabbard->GetStaticMesh()));
        Root->SetNumberField(TEXT("current_block_hold_fraction"),MemoriaCombatTuning::BlockHoldAt);
        Root->SetNumberField(TEXT("surface_clearance_cm"),Clearance);
        Root->SetNumberField(TEXT("walk_grounded_samples"),GroundedFootSamples); Root->SetNumberField(TEXT("walk_max_sole_world_drift_per_half_cm_step"),MaximumFootDrift);
        Root->SetNumberField(TEXT("walk_entry_supported_sole_motion_cm"),MaximumWalkEntrySupportedMotion);
        Root->SetNumberField(TEXT("old_band_centroid_supported_samples"),CentroidGroundedSamples);
        Root->SetNumberField(TEXT("old_band_centroid_maximum_world_drift_cm"),MaximumCentroidFootDrift);
        Root->SetStringField(TEXT("walk_steady_interval"),TEXT("Two adjacent real walk samples, both weights>.98; SAME native sole IDs within.1cm of unchanged ground at BOTH endpoints; >=3 distinct non-collinear points, finite XY area>1e-4cm2 at both endpoints. Measure maximum individual persistent-point drift. No phase or velocity exclusions; old band-centroid and landing/rolling drift remain raw diagnostics."));
        Root->SetNumberField(TEXT("walk_both_soles_above_ground_frames"),AirFrames); Root->SetArrayField(TEXT("samples"),Rows);
        Root->SetArrayField(TEXT("reference_sole_geometry"),SoleGeometry);
        Root->SetArrayField(TEXT("walk_step_geometry"),WalkDiagnostics);
        Root->SetStringField(TEXT("capture_GUID_folder"),CaptureRunFolder);Root->SetArrayField(TEXT("native_capture_completion"),CaptureEvidence);
        Root->SetStringField(TEXT("pose_sampling"),TEXT("Single manual skeletal evaluation; native ComputeSkinnedPositions from current ref-to-local matrices. No GetCPUSkinnedVertices re-evaluation/render-state mutation. Walk history begins at first actual walk sample."));
        if(MouthGeometry.IsValid())Root->SetObjectField(TEXT("native_scabbard_mouth_geometry"),MouthGeometry);
        Root->SetStringField(TEXT("acceptance"),TEXT("Each critical idle/held guard must strictly reduce both mounted crossings and proximity versus exact original mount on identical posed native vertices. Drawn grip/body/boot geometry cannot regress. Real walk support/drift thresholds retained. Residual contacts explicitly reported."));
        Root->SetStringField(TEXT("limits"),TEXT("Exact triangle surface crossings/proximity only; no closed-volume, cloth-self-collision or physics certification. Inherited source contacts remain and are reported, not assumed zero. Grip7cm and pelvis-mount4cm contact zones unchanged. Action support is reported, not assumed upright."));
        FString Json; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("Validation/HeroContactS362"); IFileManager::Get().MakeDirectory(*Folder,true);
        FFileHelper::SaveStringToFile(Json,*(Folder/(bBlockOnly?TEXT("BlockProbe.json"):TEXT("FullProbe.json"))));
        Test->AddInfo(TEXT("S362_CONTACT_PROBE_COMPLETE ")+Folder);
    }
public:
    explicit FHeroS362ContactReplay(FAutomationTestBase* InTest,bool bOnlyBlock) : Test(InTest),bBlockOnly(bOnlyBlock) {}
    ~FHeroS362ContactReplay() override
    { RestoreCaptureCamera();if(Actor.IsValid()) Actor->Destroy(); if(bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); } }
    bool Update() override
    {
        if(LastFrame==GFrameCounter) return false; LastFrame=GFrameCounter;
        if(FPlatformTime::Seconds()-Started>600.) { Test->AddError(TEXT("Native contact probe timeout")); return true; }
        UWorld* Active=GEditor?GEditor->PlayWorld.Get():nullptr; auto* PC=Active?Active->GetFirstPlayerController():nullptr;
        if(!PC || !PC->GetPawn() || Active->GetTimeSeconds()<.3f) return false;
        auto* Story=Active->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>(); if(!Story) return true;
        if(!bFixed)
        { bFixed=true;bOldFixed=FApp::UseFixedTimeStep();OldDelta=FApp::GetFixedDeltaTime();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);for(int32 I=0;I<20 && Story->GetState()==EMemoriaSliceState::VN;++I)Story->Confirm(1);return false; }
        if(!bInitialized)
        {
            if(!Active->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Story->GetState()!=EMemoriaSliceState::Exploration) return false;
            World=Active;Actor=Active->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor.Get());Actor->SetRootComponent(Root);Root->RegisterComponent();
            FVector At=PC->GetPawn()->GetActorLocation()+FVector(-200,-240,0);At.Z=0;Actor->SetActorLocation(At);
            auto* Part=NewObject<UMemoriaFieldCharacterComponent>(Actor.Get());Part->SetupAttachment(Root);Part->RegisterComponent();Figure=Part;
            if(!Part->InitializeCharacter(TEXT("arrel"),150.f) || !Part->HasSword()) { Test->AddError(TEXT("Contact probe actual Arrel/sword initialization failed"));return true; }
            Part->Face(TEXT("Right"));Part->SetSwordDrawn(true);Part->SetComponentTickEnabled(false);Blade=Part->GetBlade();
            // The figure clock and skeletal evaluation are both owned by this
            // fixture. Child skeletal auto-ticks must not race manual sampling.
            auto* Skin=Part->GetSkeletalMesh();Skin->SetComponentTickEnabled(false);Skin->SetForcedLOD(1);
            Test->TestTrue(TEXT("Arrel matrix sampling has no morph or simulated-cloth position layer"),Skin->GetSkeletalMeshAsset()->GetMorphTargets().IsEmpty() && Skin->GetSkeletalMeshAsset()->GetMeshClothingAssets().IsEmpty());
            TArray<UStaticMeshComponent*> Props;Actor->GetComponents(Props);
            for(auto* Prop:Props) if(Prop->GetStaticMesh())
            {
                if(Prop->GetStaticMesh()->GetName().EndsWith(TEXT("scabbard")))Scabbard=Prop;
                if(Prop->GetStaticMesh()->GetName().EndsWith(TEXT("sword_sheathed")))Sheathed=Prop;
            }
            if(!Scabbard.IsValid() || !Blade.IsValid() || !Sheathed.IsValid()) {Test->AddError(TEXT("Contact probe cannot resolve actual drawn/sheathed props"));return true;}
            OriginalGrip=Blade->GetRelativeTransform();FindSoles();bInitialized=true;
            ReferencePelvis=Figure->GetSkeletalMesh()->GetSocketTransform(TEXT("pelvis"),RTS_Component);
            const FTransform OriginalReference(FRotator(0,90,0).Quaternion(),ReferencePelvis.GetLocation()+FVector(0,-21,13));
            FrozenOriginalMount=OriginalReference.GetRelativeTransform(ReferencePelvis);
            const FTransform DeliveredExpected=FMemoriaHeroPropMount::FinishScabbardMount(OriginalReference).GetRelativeTransform(ReferencePelvis);
            Test->TestTrue(TEXT("Both mounted props deliver the measured mouth cant; exact original mount remains independently reconstructible"),
                Sheathed->GetRelativeTransform().Equals(DeliveredExpected,.00001f) && Scabbard->GetRelativeTransform().Equals(DeliveredExpected,.00001f));
            FindScabbardMouth();
            for(float Fraction:{.05f,.12f,.20f,.30f,.38f,.45f,.55f,.65f,.70f,.75f,.85f,.95f})Samples.Add({TEXT("Block"),Fraction,true});
            Samples.Add({TEXT("BlockCurrentTuning"),MemoriaCombatTuning::BlockHoldAt,true});
            if(!bBlockOnly)for(const TCHAR* Role:{TEXT("Attack_01"),TEXT("Attack_02"),TEXT("Attack_03"),TEXT("ChargedAttack"),TEXT("Sword_Heavy_Combo"),TEXT("Dash"),TEXT("HitReact_Front_Lgt_01"),TEXT("Death_Front_01")})
                for(float Fraction:{.20f,.45f,.70f})Samples.Add({Role,Fraction,FString(Role).Contains(TEXT("Death"))});
        }
        if(Active!=World.Get()) {Test->AddError(TEXT("Contact probe world changed"));return true;}
        ++Frame;Figure->TickComponent(1.f/60.f,LEVELTICK_All,nullptr);
        if(Frame==20){Figure->SetSwordDrawn(false);SamplePose(TEXT("IdleSheathed"),0.f,true);Figure->SetSwordDrawn(true);}
        if(Frame==22)SamplePose(TEXT("IdleDrawn"),0.f,true);
        if(!bBlockOnly && Frame>=30 && Frame<174)
        {
            const FVector Step(.5f,0,0);Figure->AdvanceLocomotion(Step,1.f/60.f);Actor->AddActorWorldOffset(Step);
            auto* Skin=Figure->GetSkeletalMesh();Skin->TickAnimation(0.f,false);Skin->RefreshBoneTransforms();
            TArray<FFinalSkinVertex> Vertices;ReadCurrentPose(Vertices);CheckFootContact(Vertices,true,false,TEXT("WalkTemporal"));
            if(Frame==70 || Frame==110 || Frame==150)SamplePose(TEXT("Walk"),Figure->GaitPhase(),true);
            return false;
        }
        if(!bBlockOnly && Frame==174)
        {
            Figure->AdvanceLocomotion(FVector::ZeroVector,1.f);
            Test->TestTrue(TEXT("Real translated walk has more than20 persistent non-collinear native contact-patch intervals"),GroundedFootSamples>20);
            Test->TestTrue(TEXT("Every real persistent ground-contact point drifts below.25cm per.5cm actor step"),MaximumFootDrift<.25);
            Test->TestTrue(TEXT("Walking does not leave both boots visibly airborne for prolonged periods"),AirFrames<20);
        }
        if(Frame<(bBlockOnly?30:180))return false;
        if(SampleIndex>=Samples.Num())
        {if(!bBlockOnly && !CaptureDeliveredViews())return false;Save();return true;}
        const auto Sample=Samples[SampleIndex++];Figure->StopAction();
        const bool bCurrent=Sample.Role==TEXT("BlockCurrentTuning"); const FString Role=bCurrent?TEXT("Block"):Sample.Role;
        if(!Figure->PlayAction(*Role,1.f,Sample.bHold,Sample.Fraction)) {Test->AddError(TEXT("Requested real contact clip failed to load: ")+Role);return true;}
        Figure->TickComponent(Figure->GetActionLength()*Sample.Fraction,LEVELTICK_All,nullptr);
        SamplePose(Sample.Role,Sample.Fraction,bCurrent);
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362MountValueContract,"MemoriaGraphics.S362.ContactProbe.MountValueContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeroS362MountValueContract::RunTest(const FString&)
{
    const FTransform Original(FRotator(0,90,0).Quaternion(),FVector(-6.5,-21,107.5));
    const FTransform Delivered=FMemoriaHeroPropMount::FinishScabbardMount(Original);
    const FVector Mouth=FMemoriaHeroPropMount::NativeScabbardMouth();
    TestTrue(TEXT("Measured cant keeps actual mouth fixed except explicit4cm reference-left translation"),
        Delivered.TransformPosition(Mouth).Equals(Original.TransformPosition(Mouth)+FVector(0,-4,0),.000001));
    TestTrue(TEXT("Mount helper preserves original scale"),Delivered.GetScale3D()==Original.GetScale3D());
    TestTrue(TEXT("Mount helper adds exactly15degrees"),FMath::IsNearlyEqual(Original.GetRotation().AngularDistance(Delivered.GetRotation()),FMath::DegreesToRadians(15.),.000001));
    const FVector Tip(0,0,-94.50252437591553);
    const FVector ExtraTipMotion=Delivered.TransformPosition(Tip)-Original.TransformPosition(Tip)-FVector(0,-4,0);
    TestTrue(TEXT("Lower sleeve angles outward rather than inward"),ExtraTipMotion.Y<-20. && ExtraTipMotion.Z>2.);
    // Nontrivial bind/posed rotations exercise reference->pelvis conversion.
    const FTransform Bind(FRotator(13,-21,7),FVector(4,-3,96));
    const FTransform Pose(FRotator(-17,43,-11),FVector(2,5,82));
    const FTransform Relative=Delivered.GetRelativeTransform(Bind);
    const FVector Expected=Pose.TransformPosition(Bind.InverseTransformPosition(Original.TransformPosition(Mouth)+FVector(0,-4,0)));
    TestTrue(TEXT("Reference mouth remains correct through a different actual pelvis pose"),
        (Relative*Pose).TransformPosition(Mouth).Equals(Expected,.000001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362BlockContact,"MemoriaGraphics.S362.ContactProbe.Block",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeroS362BlockContact::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHeroS362ContactReplay(this,true)));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362FullContact,"MemoriaGraphics.S362.ContactProbe.Full",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeroS362FullContact::RunTest(const FString&)
{if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice")))return false;FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHeroS362ContactReplay(this,false)));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;}
#endif
