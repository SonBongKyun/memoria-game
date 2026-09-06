#include "Framework/MemoriaLocalPlayer.h"
#include "Framework/MemoriaFieldPawn.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"
#include "Math/ScaleMatrix.h"

bool UMemoriaLocalPlayer::GetProjectionData(FViewport* Viewport, FSceneViewProjectionData& ProjectionData, int32 StereoViewIndex) const
{
    if (!Super::GetProjectionData(Viewport, ProjectionData, StereoViewIndex)) { return false; }
    if (StereoViewIndex == INDEX_NONE && PlayerController && Cast<AMemoriaFieldPawn>(PlayerController->GetViewTarget()))
    {
        // A -Z camera with world +Y at the top otherwise puts +X on the left.
        // Reflect view X so rendering AND screen projection/deprojection share
        // source-compatible axes. The negative view determinant also lets UE
        // select the matching culling sign, unlike a postprocess image flip.
        ProjectionData.ViewRotationMatrix *= FScaleMatrix(FVector(-1.0, 1.0, 1.0));
    }
    return true;
}
