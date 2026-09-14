#pragma once
#include "CoreMinimal.h"
class UTexture2D;
struct FMemoriaArtworkSource { const TCHAR* Source; const TCHAR* Package; bool bReuse; };
// Offline-imported presentation textures; never reads source files at runtime.
namespace MemoriaNarrativeArtwork
{
    MEMORIA_API const TArray<FMemoriaArtworkSource>& Sources();
    MEMORIA_API FString PortraitSource(const FString& Key);
    MEMORIA_API UTexture2D* Load(const FString& Source);
}
