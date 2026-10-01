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
// Offline imports only. Despite the name this is the UI art table (S333): the turn-based battle is retired.
namespace MemoriaBattleEntryArt
{
    MEMORIA_API const TArray<FMemoriaBattleEntryArtSource>& Sources();
    MEMORIA_API UTexture2D* Load(const FString& Source);
}
