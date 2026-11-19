#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Presentation/MemoriaHeroFinish.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Animation/MorphTarget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FString PositionKey(const FVector3f& P)
{
    return FString::Printf(TEXT("%d,%d,%d"), FMath::RoundToInt(P.X*10000),
        FMath::RoundToInt(P.Y*10000), FMath::RoundToInt(P.Z*10000));
}

// Reconnect UV-split render vertices by actual reference position, then identify
// the same two610point eye spheres used by the source receipt. No painted skin,
// brows, or arbitrary screen pixel color can stand in for an iris sample.
TArray<TArray<uint32>> NativeEyeClouds(const FSkeletalMeshLODRenderData& LOD)
{
    TArray<int32> Parent; Parent.SetNumUninitialized(LOD.GetNumVertices());
    for (int32 I = 0; I < Parent.Num(); ++I) Parent[I] = I;
    auto Find = [&](int32 I) { while (Parent[I] != I) { Parent[I] = Parent[Parent[I]]; I = Parent[I]; } return I; };
    auto Join = [&](int32 A, int32 B) { A = Find(A); B = Find(B); if (A != B) Parent[B] = A; };
    TMap<FString, uint32> AtPosition;
    TSet<uint32> Used;
    const auto* Indices = LOD.MultiSizeIndexContainer.GetIndexBuffer();
    for (const auto& Section : LOD.RenderSections)
        if (Section.MaterialIndex == 2)
            for (uint32 T = 0; T < Section.NumTriangles; ++T)
            {
                uint32 V[3];
                for (uint32 I = 0; I < 3; ++I)
                {
                    V[I] = Indices->Get(Section.BaseIndex+T*3+I); Used.Add(V[I]);
                    const FString Key = PositionKey(LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V[I]));
                    if (const auto* Previous = AtPosition.Find(Key)) Join(V[I], *Previous);
                    else AtPosition.Add(Key, V[I]);
                }
                Join(V[0], V[1]); Join(V[0], V[2]);
            }
    TMap<int32, TArray<uint32>> Components;
    for (uint32 V : Used) Components.FindOrAdd(Find(V)).Add(V);
    TArray<TArray<uint32>> Eyes;
    for (const auto& Component : Components)
    {
        TMap<FString, uint32> Unique;
        FBox Bounds(ForceInit);
        for (uint32 V : Component.Value)
        {
            const auto P = LOD.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(V);
            Unique.Add(PositionKey(P), V); Bounds += FVector(P);
        }
        if (Unique.Num() != 610 || Bounds.GetSize().GetMax() > 3.f) continue;
        TArray<uint32> Cloud; Unique.GenerateValueArray(Cloud); Eyes.Add(MoveTemp(Cloud));
    }
    return Eyes;
}

bool RayTriangle(const FVector& Start, const FVector& End, const FVector& A, const FVector& B, const FVector& C)
{
    const FVector Direction = End-Start, E1 = B-A, E2 = C-A;
    const FVector P = FVector::CrossProduct(Direction, E2);
    const double Determinant = FVector::DotProduct(E1, P);
    if (FMath::Abs(Determinant) < 1e-10) return false;
    const double Inverse = 1./Determinant;
    const FVector S = Start-A;
    const double U = FVector::DotProduct(S, P)*Inverse;
    if (U < -1e-7 || U > 1.+1e-7) return false;
    const FVector Q = FVector::CrossProduct(S, E1);
    const double V = FVector::DotProduct(Direction, Q)*Inverse;
    if (V < -1e-7 || U+V > 1.+1e-7) return false;
    const double T = FVector::DotProduct(E2, Q)*Inverse;
    return T > 0. && T < 1.-1e-7;
}

class FHeroS362EvaluatedReplay final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    double Started = FPlatformTime::Seconds(), OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0;
    bool bFixed = false, bOldFixed = false;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> Actor, OldView;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    TArray<FFinalSkinVertex> OpenVertices;
    TArray<TArray<uint32>> Eyes;
    TSet<uint32> LidDeltaVertices;
    TArray<FIntVector> LidTriangles;
    struct FHidden { TWeakObjectPtr<USceneComponent> Part; bool bVisible; };
    TArray<FHidden> Hidden;

    void Capture(const TCHAR* View)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/HeroS362Evaluated")/(FString(View)+TEXT(".png")), true, false);
    }
    void LogWeights(const TCHAR* Stage)
    {
        auto* Skin = Figure->GetSkeletalMesh();
        FString Active;
        for (const auto& Pair : Skin->ActiveMorphTargets)
            Active += FString::Printf(TEXT("%s=%f;"), *GetNameSafe(Pair.Key),
                Skin->MorphTargetWeights.IsValidIndex(Pair.Value) ? Skin->MorphTargetWeights[Pair.Value] : -999.f);
        auto* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableMorphTargets"));
        const auto* Material = Skin->GetMaterial(4) ? Skin->GetMaterial(4)->GetMaterial() : nullptr;
        Test->AddInfo(FString::Printf(TEXT("S362_EVALUATED_WEIGHTS stage=%s stored_L=%f stored_R=%f active=%s disabled=%d cvar=%d material_morph_usage=%d nanite=%d"),
            Stage, Skin->GetMorphTarget(FMemoriaHeroFinish::BlinkLeft), Skin->GetMorphTarget(FMemoriaHeroFinish::BlinkRight),
            *Active, Skin->bDisableMorphTarget, Variable ? Variable->GetInt() : -1,
            Material ? Material->GetUsageByFlag(MATUSAGE_MorphTargets) : false, Skin->ShouldNaniteSkin()));
    }
    void CheckCoverage(const TArray<FFinalSkinVertex>& Vertices, bool bClosed)
    {
        auto* Skin = Figure->GetSkeletalMesh();
        const FTransform Transform = Skin->GetComponentTransform();
        int32 Total = 0, Occluded = 0;
        for (const auto& Cloud : Eyes)
        {
            FVector Center = FVector::ZeroVector;
            for (uint32 V : Cloud) Center += Transform.TransformPosition(FVector(Vertices[V].Position));
            Center /= Cloud.Num();
            double Radius = 0.;
            for (uint32 V : Cloud) Radius += FVector::Dist(Center, Transform.TransformPosition(FVector(Vertices[V].Position)));
            Radius /= Cloud.Num();
            const FVector Toward = (Camera->GetActorLocation()-Center).GetSafeNormal();
            const FVector Across = FVector::CrossProduct(Toward, FVector::UpVector).GetSafeNormal();
            const FVector Up = FVector::CrossProduct(Across, Toward).GetSafeNormal();
            for (int32 X = -3; X <= 3; ++X) for (int32 Z = -3; Z <= 3; ++Z)
            {
                const double DX = X*.12*Radius, DZ = Z*.12*Radius;
                if (FMath::Sqrt(DX*DX+DZ*DZ) > Radius*.40) continue;
                const FVector Target = Center+Across*DX+Up*DZ+Toward*FMath::Sqrt(Radius*Radius-DX*DX-DZ*DZ);
                ++Total;
                for (const auto& Triangle : LidTriangles)
                    if (RayTriangle(Camera->GetActorLocation(), Target,
                        Transform.TransformPosition(FVector(Vertices[Triangle.X].Position)),
                        Transform.TransformPosition(FVector(Vertices[Triangle.Y].Position)),
                        Transform.TransformPosition(FVector(Vertices[Triangle.Z].Position))))
                    { ++Occluded; break; }
            }
        }
        Test->TestTrue(TEXT("Both actual retained eye spheres produce native camera iris samples"), Total >= 70);
        Test->TestTrue(bClosed ? TEXT("Evaluated closed lid skin actually occludes at least90percent of both irises")
                              : TEXT("Evaluated open lids leave at least90percent of both irises visible"),
            bClosed ? Occluded >= Total*.90f : Occluded <= Total*.10f);
        Test->AddInfo(FString::Printf(TEXT("S362_EVALUATED_IRIS_COVERAGE closed=%d occluded=%d total=%d"), bClosed, Occluded, Total));
    }
public:
    explicit FHeroS362EvaluatedReplay(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FHeroS362EvaluatedReplay() override
    {
        if (World.IsValid() && World->GetFirstPlayerController() && OldView.IsValid()) World->GetFirstPlayerController()->SetViewTarget(OldView.Get());
        if (Actor.IsValid()) Actor->Destroy(); if (Camera.IsValid()) Camera->Destroy();
        for (const auto& Item : Hidden) if (Item.Part.IsValid()) Item.Part->SetVisibility(Item.bVisible, false);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false; LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds()-Started > 180.) { Test->AddError(TEXT("Evaluated blink replay timed out")); return true; }
        UWorld* Active = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        auto* PC = Active ? Active->GetFirstPlayerController() : nullptr;
        if (!PC || !PC->GetPawn() || Active->GetTimeSeconds() < .3f) return false;
        auto* Story = Active->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!Story) return true;
        if (!bFixed)
        {
            bFixed = true; bOldFixed = FApp::UseFixedTimeStep(); OldDelta = FApp::GetFixedDeltaTime();
            FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1./60.);
            for (int32 I = 0; I < 20 && Story->GetState() == EMemoriaSliceState::VN; ++I) Story->Confirm(1);
            return false;
        }
        if (!Actor.IsValid())
        {
            if (!Active->GetMapName().EndsWith(TEXT("L_VerdanHost")) || Story->GetState() != EMemoriaSliceState::Exploration) return false;
            World = Active; OldView = PC->GetViewTarget();
            for (TActorIterator<AActor> It(Active); It; ++It)
            {
                TArray<UMemoriaFieldCharacterComponent*> Parts; It->GetComponents(Parts);
                for (auto* Part : Parts)
                {
                    TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
                    for (auto* Child : Children) { Hidden.Add({Child, Child->IsVisible()}); Child->SetVisibility(false, false); }
                }
            }
            Actor = Active->SpawnActor<AActor>();
            auto* Root = NewObject<USceneComponent>(Actor.Get()); Actor->SetRootComponent(Root); Root->RegisterComponent();
            FVector At = PC->GetPawn()->GetActorLocation()+FVector(-200,-240,0); At.Z=0; Actor->SetActorLocation(At);
            auto* Part = NewObject<UMemoriaFieldCharacterComponent>(Actor.Get()); Part->SetupAttachment(Root); Part->RegisterComponent(); Figure=Part;
            if (!Part->InitializeCharacter(TEXT("elia"),138.f)) { Test->AddError(TEXT("Evaluated blink initialization failed")); return true; }
            Part->Face(TEXT("Right")); Part->SetComponentTickEnabled(false);
            auto* Skin=Part->GetSkeletalMesh();
            if (!Skin || !FMemoriaHeroFinish::HasDedicatedLids(Skin->GetSkeletalMeshAsset())) { Test->AddError(TEXT("Evaluated blink has no actual lid morphs")); return true; }
            const auto& LOD=Skin->GetSkeletalMeshAsset()->GetResourceForRendering()->LODRenderData[0];
            Eyes=NativeEyeClouds(LOD);
            if (!Test->TestEqual(TEXT("Actual native topology identifies both original eye spheres"),Eyes.Num(),2)) return true;
            for(FName Name:{FMemoriaHeroFinish::BlinkLeft,FMemoriaHeroFinish::BlinkRight})
            {
                const auto* Morph=Skin->GetSkeletalMeshAsset()->FindMorphTarget(Name);
                FString Sections; for(int32 Section:Morph->GetMorphLODModels()[0].SectionIndices) Sections+=FString::FromInt(Section)+TEXT(",");
                Test->AddInfo(FString::Printf(TEXT("S362_NATIVE_MORPH_SECTIONS %s indices=%s"),*Name.ToString(),*Sections));
                for(const auto& Delta:Morph->GetMorphTargetDeltas(0)) LidDeltaVertices.Add(Delta.SourceIdx);
            }
            const auto* Indices=LOD.MultiSizeIndexContainer.GetIndexBuffer();
            const auto& Colors=LOD.StaticVertexBuffers.ColorVertexBuffer;
            for(const auto& Section:LOD.RenderSections) if(Section.MaterialIndex==4)
                for(uint32 T=0;T<Section.NumTriangles;++T)
                {
                    FIntVector V(Indices->Get(Section.BaseIndex+T*3),Indices->Get(Section.BaseIndex+T*3+1),Indices->Get(Section.BaseIndex+T*3+2));
                    if(Colors.VertexColor(V.X).A<5 && Colors.VertexColor(V.Y).A<5 && Colors.VertexColor(V.Z).A<5) LidTriangles.Add(V);
                }
            Camera=Active->SpawnActor<ACameraActor>();
            const FVector Target=Skin->GetSocketLocation(TEXT("head"))+FVector(0,0,138.f*.055f);
            const FVector Eye=Target+Skin->GetForwardVector()*138.f*.82f+FVector(0,0,138.f*.025f);
            Camera->SetActorLocation(Eye); Camera->SetActorRotation((Target-Eye).Rotation()); Camera->GetCameraComponent()->SetFieldOfView(35.f); PC->SetViewTarget(Camera.Get());
            FMemoriaHeroFinish::ApplyBlink(Skin,0.f);
        }
        auto* Skin=Figure->GetSkeletalMesh(); ++Frame;
        if(Frame==20)
        {
            Skin->GetCPUSkinnedVertices(OpenVertices,0); LogWeights(TEXT("open_evaluated"));
            CheckCoverage(OpenVertices,false); Capture(TEXT("EliaControlledOpen"));
        }
        if(Frame==35)
        {
            FMemoriaHeroFinish::ApplyBlink(Skin,1.f); LogWeights(TEXT("closed_stored_before_evaluation"));
        }
        if(Frame==55)
        {
            LogWeights(TEXT("closed_rendered_before_CPU_probe"));
            const auto* Material=Skin->GetMaterial(4) ? Skin->GetMaterial(4)->GetMaterial() : nullptr;
            Test->TestTrue(TEXT("Actual rendered lid material supports the morph vertex-factory variant"),
                Material && Material->GetUsageByFlag(MATUSAGE_MorphTargets));
            TArray<FFinalSkinVertex> Closed; Skin->GetCPUSkinnedVertices(Closed,0); LogWeights(TEXT("closed_CPU_evaluated"));
            if(!Test->TestEqual(TEXT("Actual open/closed evaluated buffers have matching topology"),Closed.Num(),OpenVertices.Num())) return true;
            int32 Moved=0; float Maximum=0.f,Outside=0.f;
            for(int32 I=0;I<Closed.Num();++I)
            {
                const float Distance=(Closed[I].Position-OpenVertices[I].Position).Size();
                if(LidDeltaVertices.Contains(I)) { Moved+=Distance>.01f; Maximum=FMath::Max(Maximum,Distance); }
                else Outside=FMath::Max(Outside,Distance);
            }
            Test->TestTrue(TEXT("Actual evaluated lid vertices move significantly beyond stored curve metadata"),Moved>200 && Maximum>.3f);
            Test->TestTrue(TEXT("No evaluated eye/face/garment vertex moves during the controlled blink"),Outside<.01f);
            Test->AddInfo(FString::Printf(TEXT("S362_EVALUATED_LID_DELTA moved=%d max_cm=%.6f outside_max_cm=%.6f"),Moved,Maximum,Outside));
            CheckCoverage(Closed,true);
        }
        if(Frame==65) Capture(TEXT("EliaControlledClosedHeld"));
        if(Frame==80) FMemoriaHeroFinish::ApplyBlink(Skin,0.f);
        if(Frame==100)
        {
            TArray<FFinalSkinVertex> Reopened; Skin->GetCPUSkinnedVertices(Reopened,0);
            float Error=0.f; for(int32 I=0;I<FMath::Min(Reopened.Num(),OpenVertices.Num());++I) Error=FMath::Max(Error,(Reopened[I].Position-OpenVertices[I].Position).Size());
            Test->TestTrue(TEXT("Evaluated blink reopens to the exact same frozen pose"),Reopened.Num()==OpenVertices.Num() && Error<.01f);
            CheckCoverage(Reopened,false); Capture(TEXT("EliaControlledReopened"));
        }
        if(Frame>=110) { Test->AddInfo(TEXT("S362_EVALUATED_BLINK_COMPLETE actual_CPU_delta_and_camera_iris_occlusion_checked=1")); return true; }
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362EvaluatedBlink,"MemoriaGraphics.S362.EvaluatedBlink",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeroS362EvaluatedBlink::RunTest(const FString&)
{
    if(!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHeroS362EvaluatedReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
