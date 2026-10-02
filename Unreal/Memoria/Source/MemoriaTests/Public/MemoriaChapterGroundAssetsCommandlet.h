#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MemoriaChapterGroundAssetsCommandlet.generated.h"

// S337: the chapter maps' ground. Imports the patches that Unreal/Tools/export_chapter_ground.py cuts from the
// source's painted map canvases (Unreal/ArtSource/ChapterGround) and authors M_ChapterGround, the one material
// the whole ground of a chapter map wears: soil and paving blended by a tile mask the map builds at run time,
// an interior floor, grime at the walls, puddles, and the Belt's round dial as an inlay.
// Additive: existing textures are kept. -Rebuild writes the material again.
UCLASS()
class UMemoriaChapterGroundAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaChapterGroundAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
