#include "Presentation/MemoriaGameOverWidget.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace
{
using MemoriaUiKit::Srgb;
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
void Frame(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, float Width, const FLinearColor& Color)
{
    Box(Elements, Layer, G, At, FVector2D(Size.X, Width), Color); Box(Elements, Layer, G, At + FVector2D(0, Size.Y - Width), FVector2D(Size.X, Width), Color);
    Box(Elements, Layer, G, At, FVector2D(Width, Size.Y), Color); Box(Elements, Layer, G, At + FVector2D(Size.X - Width, 0), FVector2D(Width, Size.Y), Color);
}
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& Center, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font);
    const FVector2D At = Center - Size * .5;
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1.5, 1.5)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .85f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
}
void UMemoriaGameOverWidget::Configure(bool bInCanLoad, bool bInKo) { bCanLoad = bInCanLoad; bKo = bInKo; Selected = 0; Age = 0.f; }
void UMemoriaGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UTexture2D* Texture = MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_game_over_void_backdrop.png")))
    { Backdrop.SetResourceObject(Texture); Backdrop.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()); Backdrop.DrawAs = ESlateBrushDrawType::Image; }
}
void UMemoriaGameOverWidget::NativeTick(const FGeometry& Geometry, float DeltaTime) { Super::NativeTick(Geometry, DeltaTime); Age += DeltaTime; }
FString UMemoriaGameOverWidget::ItemLabel(int32 Index) const
{
    static const TCHAR* En[ItemCount] = {TEXT("Stagger On (HP 30%)"), TEXT("Load Save"), TEXT("Return to Title")};
    static const TCHAR* Ko[ItemCount] = {TEXT("버티고 일어선다 (HP 30%)"), TEXT("저장 불러오기"), TEXT("타이틀로")};
    return Index >= 0 && Index < ItemCount ? FString(bKo ? Ko[Index] : En[Index]) : FString();
}
void UMemoriaGameOverWidget::Navigate(int32 Direction) { Selected = (Selected + Direction + ItemCount) % ItemCount; }
void UMemoriaGameOverWidget::Activate(int32 Index)
{
    if (Index < 0 || Index >= ItemCount) return;
    Selected = Index;
    // The source keeps Load Save listed and answers a missing save with the cancel sound (the caller's).
    OnAction.ExecuteIfBound(static_cast<EMemoriaGameOverAction>(Index));
}
FReply UMemoriaGameOverWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    for (int32 I = 0; I < Rows.Num(); ++I) if (Rows[I].IsInside(Local)) Selected = I;
    return FReply::Handled();
}
FReply UMemoriaGameOverWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    for (int32 I = 0; I < Rows.Num(); ++I) if (Rows[I].IsInside(Local)) { Activate(I); break; }
    return FReply::Handled();
}
int32 UMemoriaGameOverWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const float In = FMath::Clamp(Age / .6f, 0.f, 1.f);
    // The void backdrop covers the screen (keep aspect, covered), dimmed as the source modulates it.
    Box(Elements, L, Geometry, FVector2D::ZeroVector, Size, FLinearColor(0, 0, 0, In));
    if (Backdrop.GetResourceObject())
    {
        const float Cover = FMath::Max(Size.X / Backdrop.ImageSize.X, Size.Y / Backdrop.ImageSize.Y);
        const FVector2D Art = Backdrop.ImageSize * Cover;
        FSlateDrawElement::MakeBox(Elements, L + 1, Geometry.ToPaintGeometry(FVector2f(Art), FSlateLayoutTransform(FVector2f((Size - Art) * .5))),
            &Backdrop, ESlateDrawEffect::None, Srgb(.92f, .88f, .82f, .86f * In));
    }
    Box(Elements, L + 2, Geometry, FVector2D::ZeroVector, Size, Srgb(.02f, .015f, .03f, .58f * In));
    // The centred panel (460 x 380 in the source's 720p units), a dull red border.
    const FVector2D Panel(690, 570), At = (Size - Panel) * .5;
    Box(Elements, L + 3, Geometry, At, Panel, Srgb(.025f, .020f, .034f, .84f * In));
    Frame(Elements, L + 4, Geometry, At, Panel, 2.f, Srgb(.62f, .28f, .22f, .72f * In));
    Text(Elements, L + 5, Geometry, bKo ? TEXT("쓰러졌다.") : TEXT("You fell."), FVector2D(Size.X * .5, At.Y + 78), MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 38), Srgb(.60f, .20f, .20f, In));
    Text(Elements, L + 5, Geometry, bKo ? TEXT("무언가가 벼랑 끝에서 너를 끌어당긴다...") : TEXT("Something pulls you back from the edge..."),
        FVector2D(Size.X * .5, At.Y + 138), MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 17), Srgb(.50f, .45f, .40f, In));
    Box(Elements, L + 4, Geometry, FVector2D(At.X + 36, At.Y + 182), FVector2D(Panel.X - 72, 1.5f), Srgb(.62f, .28f, .22f, .35f * In));
    Rows.Reset();
    for (int32 I = 0; I < ItemCount; ++I)
    {
        const FVector2D R(At.X + 36, At.Y + 216 + I * 90.f), RS(Panel.X - 72, 66);
        const bool bOn = I == Selected, bDim = I == 1 && !bCanLoad;
        Box(Elements, L + 4, Geometry, R, RS, bOn ? Srgb(.14f, .10f, .16f, .95f * In) : Srgb(.08f, .06f, .10f, .90f * In));
        Frame(Elements, L + 5, Geometry, R, RS, 1.f, bOn ? Srgb(.70f, .40f, .30f, .80f * In) : Srgb(.40f, .20f, .20f, .60f * In));
        const FLinearColor Ink = bDim ? Srgb(.40f, .36f, .34f, In) : bOn ? Srgb(.95f, .75f, .45f, In) : Srgb(.70f, .60f, .55f, In);
        Text(Elements, L + 6, Geometry, ItemLabel(I), R + RS * .5, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 19), Ink);
        Rows.Add(FBox2D(R, R + RS));
    }
    return L + 8;
}
