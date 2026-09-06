#pragma once
#include "Engine/LocalPlayer.h"
#include "MemoriaLocalPlayer.generated.h"

// The source's screen axes and the -Z camera imply a reflected view basis.
// Keep this in the projection boundary, never in saved gameplay coordinates.
UCLASS()
class MEMORIA_API UMemoriaLocalPlayer : public ULocalPlayer
{
    GENERATED_BODY()
public:
    virtual bool GetProjectionData(FViewport* Viewport, FSceneViewProjectionData& ProjectionData, int32 StereoViewIndex = INDEX_NONE) const override;
};
