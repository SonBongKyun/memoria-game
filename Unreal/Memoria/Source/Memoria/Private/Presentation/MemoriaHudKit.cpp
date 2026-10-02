#include "Presentation/MemoriaHudKit.h"
#include "Presentation/MemoriaUiKit.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
namespace MemoriaHudKit
{
using MemoriaUiKit::Srgb;
const TCHAR* Package(EArt Art)
{
    switch (Art)
    {
    case EArt::Plate: return TEXT("/Game/Memoria/Presentation/Hud/T_HudPlate");
    case EArt::Toast: return TEXT("/Game/Memoria/Presentation/Hud/T_HudToast");
    default: return TEXT("/Game/Memoria/Presentation/Hud/T_HudRibbon");
    }
}
UTexture2D* Load(EArt Art)
{
    const FString Path(Package(Art));
    auto* Texture = LoadObject<UTexture2D>(nullptr, *(Path + TEXT(".") + FPaths::GetBaseFilename(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet);
#if WITH_EDITOR
    if (Texture) FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
FString ItemPackage(const FString& ItemId)
{
    // "hi_potion" -> T_ItemHiPotion
    FString Name; bool bUpper = true;
    for (const TCHAR C : ItemId) { if (C == TEXT('_')) { bUpper = true; continue; } Name.AppendChar(bUpper ? FChar::ToUpper(C) : C); bUpper = false; }
    return TEXT("/Game/Memoria/Presentation/Hud/T_Item") + Name;
}
UTexture2D* LoadItem(const FString& ItemId)
{
    const FString Path = ItemPackage(ItemId);
    auto* Texture = LoadObject<UTexture2D>(nullptr, *(Path + TEXT(".") + FPaths::GetBaseFilename(Path)), nullptr, LOAD_NoWarn | LOAD_Quiet);
#if WITH_EDITOR
    if (Texture) FTextureCompilingManager::Get().FinishCompilation({Texture});
#endif
    return Texture;
}
bool Brush(FSlateBrush& Out, UTexture2D* Texture)
{
    if (!Texture) return false;
    Out.SetResourceObject(Texture); Out.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()); Out.DrawAs = ESlateBrushDrawType::Image;
    return true;
}
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
void Image(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FSlateBrush& InBrush, const FVector2D& At, const FVector2D& Size, const FLinearColor& Tint)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), &InBrush, ESlateDrawEffect::None, Tint);
}
float Width(const FString& S, const FSlateFontInfo& Font)
{ return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font).X; }
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Anchor, float Align, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D At = Anchor - FVector2D(Size.X * Align, Size.Y * .5);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1.5, 1.5)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .85f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
void Diamond(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& Centre, const FVector2D& Half, float Thickness, const FLinearColor& Color)
{
    const TArray<FVector2f> Points{FVector2f(Centre + FVector2D(0, -Half.Y)), FVector2f(Centre + FVector2D(Half.X, 0)), FVector2f(Centre + FVector2D(0, Half.Y)),
        FVector2f(Centre + FVector2D(-Half.X, 0)), FVector2f(Centre + FVector2D(0, -Half.Y))};
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
}
void Arc(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& Centre, float Radius, float Fraction, float Thickness, const FLinearColor& Color)
{
    Fraction = FMath::Clamp(Fraction, 0.f, 1.f);
    if (Fraction <= 0.f) return;
    TArray<FVector2f> Points;
    const int32 Steps = FMath::Max(3, FMath::CeilToInt(56 * Fraction));
    for (int32 I = 0; I <= Steps; ++I)
    {
        const float A = 2.f * PI * Fraction * I / Steps - PI * .5f;
        Points.Add(FVector2f(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius));
    }
    FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
}
void Gauge(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Fraction, float Ghost, const FLinearColor& Fill)
{
    Fraction = FMath::Clamp(Fraction, 0.f, 1.f); Ghost = FMath::Clamp(Ghost, 0.f, 1.f);
    Box(Elements, Layer, G, At, Size, Srgb(.045f, .035f, .04f, .96f));
    if (Ghost > Fraction) Box(Elements, Layer + 1, G, At, FVector2D(Size.X * Ghost, Size.Y), FLinearColor(Fill.R, Fill.G * 1.6f + .12f, Fill.B * 1.6f + .1f, .55f));
    if (Fraction > 0.f)
    {
        Box(Elements, Layer + 2, G, At, FVector2D(Size.X * Fraction, Size.Y), Fill);
        Box(Elements, Layer + 2, G, At, FVector2D(Size.X * Fraction, FMath::Max(1.5, Size.Y * .22)), FLinearColor(FMath::Min(1.f, Fill.R * 1.5f + .1f), Fill.G * 1.5f + .08f, Fill.B * 1.5f + .06f, .7f));
        Box(Elements, Layer + 2, G, At + FVector2D(Size.X * Fraction - 2., 0), FVector2D(2, Size.Y), FLinearColor(1.f, .86f, .7f, .55f));
    }
}
void Panel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Alpha)
{
    Box(Elements, Layer, G, At - FVector2D(1.5, 1.5), Size + FVector2D(3, 3), Srgb(.70f, .56f, .34f, .42f * Alpha));
    Box(Elements, Layer, G, At, Size, Srgb(.030f, .024f, .040f, .78f * Alpha));
}
}
