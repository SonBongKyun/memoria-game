#pragma once
#include "Commandlets/Commandlet.h"
#include "MemoriaChapterLevelsCommandlet.generated.h"
// S320: creates the level of every ported chapter map (/Game/Memoria/Maps/L_<Map>) when missing: an empty
// world on the slice game mode with a player start at the map's spawn. AMemoriaChapterPresentation builds
// the field at play time from the map IR, so the level holds nothing else. Additive.
UCLASS()
class UMemoriaChapterLevelsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UMemoriaChapterLevelsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
