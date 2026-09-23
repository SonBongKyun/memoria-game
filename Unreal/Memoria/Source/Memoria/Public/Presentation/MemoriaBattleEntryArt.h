#pragma once
#include "CoreMinimal.h"
class UTexture2D;
struct FMemoriaBattleEntryArtSource
{
    const TCHAR* Source;
    const TCHAR* RelativeFile;
    const TCHAR* Package;
    bool bPixelArt=false;
};
// Offline imports only. Original art and the executed source fallback remain unchanged.
namespace MemoriaBattleEntryArt
{
    MEMORIA_API const TArray<FMemoriaBattleEntryArtSource>& Sources();
    MEMORIA_API UTexture2D* Load(const FString& Source);
    MEMORIA_API FString MarketThiefSource();
    MEMORIA_API FString MarketThiefStudySource();
    MEMORIA_API FString AlleyRatStudySource();
}
