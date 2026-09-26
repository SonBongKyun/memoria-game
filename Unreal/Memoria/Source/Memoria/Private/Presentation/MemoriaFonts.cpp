#include "Presentation/MemoriaFonts.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
namespace MemoriaFonts
{
const TArray<const TCHAR*>& Faces()
{
    static const TArray<const TCHAR*> Values = {TEXT("MemoriaSerifMedium"), TEXT("MemoriaSerifSemiBold"), TEXT("MemoriaSansSemiBold")};
    return Values;
}
FString FacePackage(const TCHAR* Face) { return FString(TEXT("/Game/Memoria/Presentation/Fonts/FF_")) + Face; }
FString FontPackage(const TCHAR* Face) { return FString(TEXT("/Game/Memoria/Presentation/Fonts/F_")) + Face; }
namespace
{
const UFont* Load(EStyle Style)
{
    const TCHAR* Face = Faces()[Style == EStyle::Body ? 0 : Style == EStyle::Title ? 1 : 2];
    const FString Package = FontPackage(Face);
    return LoadObject<UFont>(nullptr, *(Package + TEXT(".") + FPaths::GetBaseFilename(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet);
}
}
bool IsImported(EStyle Style) { return Load(Style) != nullptr; }
FSlateFontInfo Get(EStyle Style, int32 Size)
{
    if (const UFont* Font = Load(Style)) return FSlateFontInfo(Font, Size);
    return FCoreStyle::GetDefaultFontStyle(Style == EStyle::Ui ? TEXT("Bold") : TEXT("Regular"), Size);
}
}
