#pragma once
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
// Static instances of the bundled Noto fonts at the ui_theme.gd weights (S230):
// serif for story text and titles, sans for interface text.
namespace MemoriaFonts
{
    enum class EStyle : uint8 { Body, Title, Ui };
    MEMORIA_API const TArray<const TCHAR*>& Faces();
    MEMORIA_API FString FacePackage(const TCHAR* Face);
    MEMORIA_API FString FontPackage(const TCHAR* Face);
    // Falls back to the engine default font when the asset is missing.
    MEMORIA_API FSlateFontInfo Get(EStyle Style, int32 Size);
    MEMORIA_API bool IsImported(EStyle Style);
}
