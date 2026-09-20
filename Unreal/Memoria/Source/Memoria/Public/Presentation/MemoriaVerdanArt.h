#pragma once
#include "CoreMinimal.h"
class UPaperSprite;
namespace MemoriaVerdanArt
{
struct FTextureSource { const TCHAR* Name; const TCHAR* File; };
struct FSpriteRegion { const TCHAR* Name; const TCHAR* Texture; FIntPoint Offset; FIntPoint Size; FVector2D Pivot; float PixelsPerUnit; };
MEMORIA_API const TArray<FTextureSource>& Textures();
MEMORIA_API const TArray<FSpriteRegion>& Sprites();
MEMORIA_API FString Package(const FString& Name);
MEMORIA_API UPaperSprite* LoadSprite(const FString& Name);
}
