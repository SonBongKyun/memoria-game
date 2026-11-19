#include "Misc/AutomationTest.h"
#include "MemoriaArrelSoleS362Validation.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Presentation/MemoriaHeroFinish.h"
#include "Presentation/MemoriaFieldCharacterComponent.h"
#include "Narrative/MemoriaNarrativeSubsystem.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
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
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/Material.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UnrealClient.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362BlinkContract, "MemoriaGraphics.S362.BlinkContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeroS362BlinkContract::RunTest(const FString&)
{
    float Last = 0.f, Maximum = 0.f, MaxStep = 0.f;
    int32 Closures = 0, DifferentSeedSamples = 0;
    double LastClosure = -10.;
    for (int32 I = 0; I < 60000; ++I)
    {
        const double Time = I / 1000.;
        const float Weight = FMemoriaHeroFinish::BlinkWeight(Time, 17u);
        if (!FMath::IsFinite(Weight) || Weight < 0 || Weight > 1)
        { AddError(TEXT("Blink curve produced a non-finite or unbounded value")); return false; }
        if (Weight > .95f && Last <= .95f)
        {
            if (Closures) TestTrue(TEXT("Blink rest intervals remain irregular and restrained"), Time-LastClosure > 2.8 && Time-LastClosure < 5.1);
            LastClosure = Time; ++Closures;
        }
        Maximum = FMath::Max(Maximum, Weight); MaxStep = FMath::Max(MaxStep, FMath::Abs(Weight-Last));
        Last = Weight;
        DifferentSeedSamples += FMath::Abs(Weight-FMemoriaHeroFinish::BlinkWeight(Time, 123u)) > .1f;
        if (FMemoriaHeroFinish::BlinkWeight(Time, 17u, true) != 0.f
            || Weight != FMemoriaHeroFinish::BlinkWeight(Time, 17u))
        { AddError(TEXT("Suppression or deterministic blink evaluation failed")); return false; }
    }
    TestTrue(TEXT("Lids reach closed contact rather than a partial eye squash"), Maximum == 1.f);
    TestTrue(TEXT("Closure and opening remain smooth at millisecond resolution"), MaxStep < .025f);
    TestTrue(TEXT("A minute contains a restrained blink count"), Closures >= 10 && Closures <= 21);
    TestTrue(TEXT("Stable seeds avoid synchronized blink timing"), DifferentSeedSamples > 1000);
    TestEqual(TEXT("Negative elapsed time is neutral"), FMemoriaHeroFinish::BlinkWeight(-1., 17u), 0.f);
    TestEqual(TEXT("NaN elapsed time is neutral"), FMemoriaHeroFinish::BlinkWeight(std::numeric_limits<double>::quiet_NaN(), 17u), 0.f);
    TestEqual(TEXT("Infinite elapsed time is neutral"), FMemoriaHeroFinish::BlinkWeight(std::numeric_limits<double>::infinity(), 17u), 0.f);
    TestEqual(TEXT("Null/fallback skeletal component is supported safely"), FMemoriaHeroFinish::ApplyBlink(nullptr, 1.f), false);
    AddInfo(FString::Printf(TEXT("S362_BLINK_CONTRACT closures_per_minute=%d maximum=%.3f max_millisecond_step=%.6f independent_seed_samples=%d"),
        Closures, Maximum, MaxStep, DifferentSeedSamples));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362ImportedMorphs, "MemoriaGraphics.S362.ImportedMorphs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeroS362ImportedMorphs::RunTest(const FString&)
{
    for (const TCHAR* Hero : {TEXT("Arrel"), TEXT("Elia")})
    {
        const FString Name = Hero;
        const bool bSoleR4=Name==TEXT("Arrel") && FPackageName::DoesPackageExist(TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/Arrel/SK_Arrel"));
        const FString Package = (bSoleR4?TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R4/"):TEXT("/Game/Memoria/Presentation/Field3D/HeroFinishS362/R2/"))+Name+TEXT("/SK_")+Name;
        auto* Mesh = LoadObject<USkeletalMesh>(nullptr, *(Package+TEXT(".SK_")+Name));
        if (!TestNotNull(TEXT("S362 immutable hero mesh loads"), Mesh)) return false;
        auto* Previous = LoadObject<USkeletalMesh>(nullptr,
            *(TEXT("/Game/Memoria/Presentation/Field3D/HeroPolishS361/R4/")+Name+TEXT("/SK_")+Name+TEXT(".SK_")+Name));
        if (!TestNotNull(TEXT("Accepted S361 source mesh remains available"), Previous)) return false;
        TestEqual(TEXT("Existing skeleton object remains exact"), Mesh->GetSkeleton(), Previous->GetSkeleton());
        TestEqual(TEXT("Existing77bind hierarchy remains intact"), Mesh->GetRefSkeleton().GetNum(), 77);
        if (Name == TEXT("Arrel"))
        {
            TestTrue(TEXT("Arrel retains painted finish without fake eye morphs"), Mesh->GetMorphTargets().IsEmpty());
            if(bSoleR4)TestTrue(TEXT("Only explicit20sole skins/4Z may change; every other native attribute/bind remains exact"),MemoriaArrelSoleNativeS362::ValidateDelivered(Mesh));
            TestEqual(TEXT("Arrel keeps every original material slot"), Mesh->GetMaterials().Num(), Previous->GetMaterials().Num());
            for (int32 I = 0; I < Previous->GetMaterials().Num(); ++I)
                TestEqual(TEXT("Arrel painted material identity remains exact"), Mesh->GetMaterials()[I].MaterialInterface.Get(), Previous->GetMaterials()[I].MaterialInterface.Get());
            continue;
        }
        TestTrue(TEXT("Elia has two actual imported dedicated lid morphs"), FMemoriaHeroFinish::HasDedicatedLids(Mesh));
        TestEqual(TEXT("No unrelated facial morphs were imported"), Mesh->GetMorphTargets().Num(), 2);
        const auto* LidMaterial = Mesh->GetMaterials().IsValidIndex(4)
            && Mesh->GetMaterials()[4].MaterialInterface ? Mesh->GetMaterials()[4].MaterialInterface->GetMaterial() : nullptr;
        TestTrue(TEXT("The actual lid material precompiles skeletal and morph vertex-factory variants"), LidMaterial
            && LidMaterial->GetUsageByFlag(MATUSAGE_SkeletalMesh) && LidMaterial->GetUsageByFlag(MATUSAGE_MorphTargets));
        const auto* Render = Mesh->GetResourceForRendering();
        if (!TestTrue(TEXT("Morph acceptance uses actual native LOD buffers"), Render && !Render->LODRenderData.IsEmpty())) return false;
        const auto& LOD = Render->LODRenderData[0];
        const auto& Colors = LOD.StaticVertexBuffers.ColorVertexBuffer;
        if (!TestTrue(TEXT("Imported pigment buffer is available for actual lid locality checks"),
            Colors.GetNumVertices() == LOD.GetNumVertices() && Colors.GetAllowCPUAccess())) return false;
        TSet<uint32> LidVertices;
        for (const auto& Section : LOD.RenderSections)
            if (Section.MaterialIndex == 4)
                for (uint32 I = Section.BaseVertexIndex; I < Section.BaseVertexIndex+Section.NumVertices; ++I)
                    if (Colors.VertexColor(I).A < 5) LidVertices.Add(I);
        for (FName MorphName : {FMemoriaHeroFinish::BlinkLeft, FMemoriaHeroFinish::BlinkRight})
        {
            auto* Morph = Mesh->FindMorphTarget(MorphName);
            if (!Morph) return false;
            const auto Deltas = Morph->GetMorphTargetDeltas(0);
            TestTrue(TEXT("Lid morph has a bounded nonempty native vertex delta buffer"), Deltas.Num() >= 100 && Deltas.Num() <= 1000);
            bool bDedicatedOnly = true, bFinite = true;
            float Maximum = 0.f;
            for (const auto& Delta : Deltas)
            {
                bDedicatedOnly &= LidVertices.Contains(Delta.SourceIdx);
                bFinite &= !Delta.PositionDelta.ContainsNaN();
                Maximum = FMath::Max(Maximum, Delta.PositionDelta.Size());
            }
            TestTrue(TEXT("Blink affects only newly authored lid skin, never eye/face/garment/hair vertices"), bDedicatedOnly);
            TestTrue(TEXT("Actual lid closure is finite and local to the eye"), bFinite && Maximum > .2f && Maximum < 3.f);
            AddInfo(FString::Printf(TEXT("S362_IMPORTED_MORPH %s native_deltas=%d max_displacement_cm=%.6f dedicated_only=%d"),
                *MorphName.ToString(), Deltas.Num(), Maximum, bDedicatedOnly));
        }
    }
    return true;
}

namespace
{
class FHeroS362BlinkReplay final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    double Started = FPlatformTime::Seconds(), OldDelta = 0.;
    uint64 LastFrame = MAX_uint64;
    int32 Frame = 0, DeathFrames = 0;
    bool bFixed = false, bOldFixed = false, bClosed = false, bReopened = false, bDeath = false;
    bool bCapturedClosing = false, bCapturedOpening = false;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> Actor, OldView;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<UMemoriaFieldCharacterComponent> Figure;
    struct FHidden { TWeakObjectPtr<USceneComponent> Part; bool bVisible; };
    TArray<FHidden> Hidden;

    void Capture(const TCHAR* View)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("MemoriaCapture")))
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/HeroS362")/(FString(View)+TEXT(".png")), true, false);
    }
    void Look()
    {
        auto* Skin = Figure->GetSkeletalMesh();
        const FVector Target = Skin->GetSocketLocation(TEXT("head"))+FVector(0, 0, 138.f*.055f);
        const FVector Eye = Target+Skin->GetForwardVector()*138.f*.82f+FVector(0, 0, 138.f*.025f);
        Camera->SetActorLocation(Eye); Camera->SetActorRotation((Target-Eye).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(35.f);
    }
public:
    explicit FHeroS362BlinkReplay(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FHeroS362BlinkReplay() override
    {
        if (World.IsValid() && World->GetFirstPlayerController() && OldView.IsValid())
            World->GetFirstPlayerController()->SetViewTarget(OldView.Get());
        if (Actor.IsValid()) Actor->Destroy();
        if (Camera.IsValid()) Camera->Destroy();
        for (const auto& Item : Hidden) if (Item.Part.IsValid()) Item.Part->SetVisibility(Item.bVisible, false);
        if (bFixed) { FApp::SetUseFixedTimeStep(bOldFixed); FApp::SetFixedDeltaTime(OldDelta); }
    }
    bool Update() override
    {
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        if (FPlatformTime::Seconds()-Started > 180.) { Test->AddError(TEXT("S362 blink replay timed out")); return true; }
        UWorld* Active = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        auto* PC = Active ? Active->GetFirstPlayerController() : nullptr;
        if (!PC || !PC->GetPawn() || Active->GetTimeSeconds() < .3f) return false;
        auto* Story = Active->GetGameInstance()->GetSubsystem<UMemoriaNarrativeSubsystem>();
        if (!Story) { Test->AddError(TEXT("S362 replay lacks narrative subsystem")); return true; }
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
            FVector At = PC->GetPawn()->GetActorLocation()+FVector(-200, -240, 0); At.Z = 0; Actor->SetActorLocation(At);
            auto* Part = NewObject<UMemoriaFieldCharacterComponent>(Actor.Get());
            Part->SetupAttachment(Root); Part->RegisterComponent(); Figure = Part;
            if (!Test->TestTrue(TEXT("S362 actual production field component initializes Elia"), Part->InitializeCharacter(TEXT("elia"), 138.f))) return true;
            if (!Test->TestTrue(TEXT("S362 actual live field mesh binds imported dedicated lids"),
                Part->GetSkeletalMesh() && FMemoriaHeroFinish::HasDedicatedLids(Part->GetSkeletalMesh()->GetSkeletalMeshAsset()))) return true;
            Part->Face(TEXT("Right")); Camera = Active->SpawnActor<ACameraActor>(); Look(); PC->SetViewTarget(Camera.Get());
        }
        if (Active != World.Get() || !Figure.IsValid() || !Camera.IsValid())
        { Test->AddError(TEXT("S362 replay lost its actual field character or world")); return true; }
        Figure->AdvanceLocomotion(FVector::ZeroVector, 1.f/60.f); Look(); ++Frame;
        auto* Skin = Figure->GetSkeletalMesh();
        const float Left = Skin->GetMorphTarget(FMemoriaHeroFinish::BlinkLeft);
        const float Right = Skin->GetMorphTarget(FMemoriaHeroFinish::BlinkRight);
        if (!FMath::IsNearlyEqual(Left, Right, .0001f))
        { Test->AddError(TEXT("S362 ordinary blink has inconsistent left/right lid closure")); return true; }
        if (bDeath)
        {
            ++DeathFrames;
            if (DeathFrames > 1 && (!FMath::IsNearlyZero(Left) || !FMath::IsNearlyZero(Right)))
            { Test->AddError(TEXT("Held death failed to suppress and clear a preexisting blink")); return true; }
            if (DeathFrames > FMath::CeilToInt(Figure->GetActionLength()*60.f)+15)
            {
                Test->TestTrue(TEXT("Death clip remains held while blink is suppressed"), Figure->IsActing());
                Figure->StopAction();
                FMemoriaHeroFinish::ClearBlink(Skin);
                Test->TestEqual(TEXT("Lifecycle cleanup leaves both lids neutral"), Skin->GetMorphTarget(FMemoriaHeroFinish::BlinkLeft), 0.f);
                Test->AddInfo(FString::Printf(TEXT("S362_RENDERED_BLINK_COMPLETE closed=1 reopened=1 closing=%d opening=%d death_frames=%d"),
                    bCapturedClosing, bCapturedOpening, DeathFrames));
                return true;
            }
            return false;
        }
        if (Frame == 30) Capture(TEXT("EliaEyesOpen"));
        if (!bClosed && !bCapturedClosing && Left > .15f && Left < .8f)
        { bCapturedClosing = true; Capture(TEXT("EliaLidsClosing")); }
        if (!bClosed && Left > .95f)
        { bClosed = true; Capture(TEXT("EliaLidsClosed")); }
        if (bClosed && !bCapturedOpening && Left > .15f && Left < .8f)
        { bCapturedOpening = true; Capture(TEXT("EliaLidsOpening")); }
        if (bClosed && Left < .01f && Frame > 30)
        { bReopened = true; Capture(TEXT("EliaEyesReopened")); }
        if (bReopened)
        {
            // Start death from an explicitly closed lid so suppression cannot
            // pass merely because the ordinary blink happened to be resting.
            FMemoriaHeroFinish::ApplyBlink(Skin, 1.f);
            if (!Test->TestTrue(TEXT("S362 actual hero death action starts"), Figure->PlayAction(TEXT("Death_Front_01"), 1.f, true))) return true;
            bDeath = true;
        }
        if (Frame > 420 && !bClosed)
        { Test->AddError(TEXT("Actual field runtime did not drive a natural dedicated-lid blink within7seconds")); return true; }
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroS362RenderedBlink, "MemoriaGraphics.S362.RenderedBlinkLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeroS362RenderedBlink::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Tests/Campaign/L_Ch2VerdanSlice"))) return false;
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(new FHeroS362BlinkReplay(this)));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
