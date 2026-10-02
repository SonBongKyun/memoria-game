#pragma once
#include "Chapter/MemoriaChapterPresentation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

// Check the physical map against source coordinates while inspecting the actual rendered layers.
// A visual asset missing from the checkout must not silently pass via the primitive fallback.
inline void CheckChapterVisualPolish(FAutomationTestBase* Test, AMemoriaChapterPresentation* Map)
{
    const auto* Spec = Map->GetSpec();
    if (!Spec) { Test->AddError(TEXT("Visual polish needs a chapter map")); return; }
    TArray<UInstancedStaticMeshComponent*> Layers;
    Map->GetComponents(Layers);
    UInstancedStaticMeshComponent* Blocks = nullptr;
    bool bStone = false, bMasonry = false, bBevel = false, bRubble = false;
    for (auto* Layer : Layers)
    {
        if (Layer->GetCollisionEnabled() != ECollisionEnabled::NoCollision && Layer->bHiddenInGame)
        { Blocks = Layer; continue; }
        if (!Layer->GetStaticMesh() || Layer->GetInstanceCount() == 0) continue;
        const FString Mesh = Layer->GetStaticMesh()->GetName();
        bBevel |= Mesh == TEXT("SM_BeveledBlock");
        bRubble |= Mesh == TEXT("SM_Rubble");
        auto* Material = Layer->GetMaterial(0);
        bStone |= Material && Material->GetBaseMaterial()->GetName() == TEXT("M_StoneSurface");
        bMasonry |= Material && Material->GetBaseMaterial()->GetName() == TEXT("M_FocusSurface");
        Test->TestEqual(TEXT("Visual terrain never changes collision"), Layer->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    // S338 replaces the tiled stone ground with painted ground and focus-aware masonry.
    Test->TestTrue(TEXT("Chapter ground uses the painted surface"), Map->IsGroundPainted());
    Test->TestTrue(TEXT("Walls use the detailed masonry material"), bMasonry);
    if (Spec->TileNames.Contains(TEXT("CONCRETE")))
        Test->TestTrue(TEXT("Concrete slabs retain the S337 weathered material"), bStone);
    Test->TestTrue(TEXT("Walls or concrete use the beveled asset"), bBevel);
    Test->TestTrue(TEXT("Terrain rubble uses the delivered stone model"), bRubble);
    if (!Test->TestNotNull(TEXT("Source collision layer remains separate"), Blocks)) return;
    int32 Index = 0;
    const float T = Spec->TileSize * MemoriaChapterMaps::Scale;
    for (int32 Y = 0; Y < Spec->Height; ++Y)
        for (int32 X = 0; X < Spec->Width; ++X)
        {
            if (!Spec->IsSolid(Spec->TileAt(X, Y))) continue;
            const FVector Center = MemoriaChapterMaps::ToWorld(FVector2D((X + .5f) * Spec->TileSize, (Y + .5f) * Spec->TileSize));
            FTransform Actual;
            if (!Blocks->GetInstanceTransform(Index++, Actual, true))
            { Test->AddError(TEXT("A source collision tile is missing")); continue; }
            Test->TestTrue(TEXT("Source blocker position remains exact"), Actual.GetLocation().Equals(Center + FVector(0, 0, 60.f), .001));
            Test->TestTrue(TEXT("Source blocker extent remains exact"), Actual.GetScale3D().Equals(FVector(T / 100.f, T / 100.f, 1.6f), .001));
            Test->TestTrue(TEXT("Source blocker rotation remains exact"), Actual.GetRotation().Equals(FQuat::Identity, .001));
        }
    Test->TestEqual(TEXT("There are no additional terrain blockers"), Blocks->GetInstanceCount(), Index);
}
