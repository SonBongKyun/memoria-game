#include "Misc/AutomationTest.h"
#include "Presentation/MemoriaArrel3DComponent.h"
#include "Presentation/MemoriaVerdanPresentation.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
// Transient game world for component/actor probes; never a saved map or PIE session.
struct FPresentationProbeWorld
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ~FPresentationProbeWorld() { if (World) World->DestroyWorld(false); }
};
}
#define PRESENTATION_TEST(Class,Name) IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class,Name,EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)

PRESENTATION_TEST(FArrelGaitBonesResolved,"Memoria.Presentation.ArrelGaitBonesResolved")
bool FArrelGaitBonesResolved::RunTest(const FString&)
{
    FPresentationProbeWorld Probe;
    if (!TestNotNull(TEXT("Transient probe world"), Probe.World)) return false;
    auto* Owner = Probe.World->SpawnActor<AActor>();
    auto* Arrel = NewObject<UMemoriaArrel3DComponent>(Owner); Arrel->RegisterComponent();
    if (!TestTrue(TEXT("Refined Arrel rig loads"), Arrel->InitializePrototype())) return false;
    const auto& B = Arrel->GaitBones();
    for (const int32 I : {B.Pelvis, B.Chest, B.CapeUpper, B.CapeMid, B.CapeTip, B.Thigh[0], B.Thigh[1], B.Calf[0], B.Calf[1],
        B.Foot[0], B.Foot[1], B.UpperArm[0], B.UpperArm[1], B.Forearm[0], B.Forearm[1]})
        TestTrue(TEXT("Every gait bone resolves on the shipped rig"), I != INDEX_NONE);
    return !HasAnyErrors();
}

PRESENTATION_TEST(FArrelRigWithoutGaitBones,"Memoria.Presentation.ArrelRigWithoutGaitBones")
bool FArrelRigWithoutGaitBones::RunTest(const FString&)
{
    FPresentationProbeWorld Probe;
    if (!TestNotNull(TEXT("Transient probe world"), Probe.World)) return false;
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));
    if (!TestNotNull(TEXT("Engine rig without Arrel bone names"), Mesh)) return false;
    TestEqual(TEXT("Probe rig has no pelvis"), Mesh->GetRefSkeleton().FindBoneIndex(TEXT("pelvis")), int32(INDEX_NONE));
    // A swapped rig reports what it cannot animate, once, instead of indexing INDEX_NONE.
    AddExpectedMessage(TEXT("Arrel gait: SkeletalCube has no bone 'pelvis'"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    AddExpectedMessage(TEXT("Arrel gait: SkeletalCube has no bone"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0, false);
    auto* Owner = Probe.World->SpawnActor<AActor>();
    auto* Arrel = NewObject<UMemoriaArrel3DComponent>(Owner); Arrel->RegisterComponent();
    Arrel->SetSkinnedAssetAndUpdate(Mesh);
    for (int32 Frame = 0; Frame < 90; ++Frame) Arrel->AdvanceLocomotion(FVector(2, 0, 0), 1.f / 60.f);
    TestEqual(TEXT("Missing pelvis stays unresolved"), Arrel->GaitBones().Pelvis, int32(INDEX_NONE));
    TestFalse(TEXT("Walking a foreign rig keeps a finite pose"), Arrel->GetBoneTransform(0).ContainsNaN());
    TestTrue(TEXT("Locomotion still advances on a foreign rig"), Arrel->LocomotionWeight() > .5f);
    return !HasAnyErrors();
}

PRESENTATION_TEST(FPlaceholderIdentification,"Memoria.Presentation.PlaceholderIdentification")
bool FPlaceholderIdentification::RunTest(const FString&)
{
    FPresentationProbeWorld Probe;
    if (!TestNotNull(TEXT("Transient probe world"), Probe.World)) return false;
    auto Spawn = [&](const TCHAR* MeshPath, const FVector& Position)
    {
        auto* Actor = Probe.World->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
        Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Actor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath));
        return Actor;
    };
    const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* Roof = TEXT("/Game/Memoria/Presentation/Depth/SM_PitchedRoof.SM_PitchedRoof");
    auto* Moved = Spawn(Cube, FVector(1, 0, -30));
    auto* Art = Spawn(Roof, FVector(0, 0, -30));
    auto* Tagged = Spawn(Roof, FVector(5, 5, 5)); Tagged->Tags.Add(AMemoriaVerdanPresentation::PlaceholderTag);
    if (!TestNotNull(TEXT("Authored roof mesh loads"), Art->GetStaticMeshComponent()->GetStaticMesh().Get())) return false;
    TestTrue(TEXT("A moved basic-shape body is still placeholder geometry"), AMemoriaVerdanPresentation::IsPlaceholderGeometry(*Moved));
    TestFalse(TEXT("Authored art at an old placeholder coordinate stays visible"), AMemoriaVerdanPresentation::IsPlaceholderGeometry(*Art));
    TestTrue(TEXT("An explicit tag marks placeholder geometry"), AMemoriaVerdanPresentation::IsPlaceholderGeometry(*Tagged));
    return !HasAnyErrors();
}
#undef PRESENTATION_TEST
#endif
