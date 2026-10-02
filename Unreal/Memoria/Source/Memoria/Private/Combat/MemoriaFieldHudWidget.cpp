#include "Combat/MemoriaFieldHudWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaHudKit.h"
#include "Presentation/MemoriaUiKit.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
namespace
{
using MemoriaUiKit::Srgb;
// "Elia  |  E / A: talk" -> the name, the keys and the verb, so that each can be drawn in its own weight.
void SplitPrompt(const FString& Prompt, FString& Name, FString& Keys, FString& Verb)
{
    FString Rest = Prompt;
    if (!Prompt.Split(TEXT("|"), &Name, &Rest)) { Name.Reset(); Rest = Prompt; }
    if (!Rest.Split(TEXT(":"), &Keys, &Verb)) { Keys.Reset(); Verb = Rest; }
    Name.TrimStartAndEndInline(); Keys.TrimStartAndEndInline(); Verb.TrimStartAndEndInline();
}
}
void UMemoriaFieldHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Toast = MemoriaHudKit::Load(MemoriaHudKit::EArt::Toast);
    MemoriaHudKit::Brush(ToastBrush, Toast);
}
int32 UMemoriaFieldHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    using namespace MemoriaHudKit;
    // The place, on the toast frame at the top left. K takes the frame's own pixels (895 x 199) to the
    // viewport's units; its dark field runs from x 115 to 780 and y 64 to 145.
    constexpr float K = .70f;
    const FVector2D At(18, 10);
    float Below = At.Y + 199 * K + 2;
    if (!View.Title.IsEmpty())
    {
        if (Toast) Image(Elements, Layer, Geometry, ToastBrush, At, FVector2D(895, 199) * K);
        else { Panel(Elements, Layer, Geometry, At + FVector2D(60, 40), FVector2D(500, 62)); }
        const FVector2D Centre = At + FVector2D(447.5, 104) * K;
        Text(Elements, Layer + 1, Geometry, View.Title, Centre - FVector2D(0, View.Subtitle.IsEmpty() ? 0 : 11), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 19), Srgb(.96f, .84f, .58f));
        if (!View.Subtitle.IsEmpty())
            Text(Elements, Layer + 1, Geometry, View.Subtitle, Centre + FVector2D(0, 14), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.72f, .68f, .64f));
    }
    // Under it, the quest's line and the toasts: chips with a gem at the left, as wide as their words.
    const FSlateFontInfo ChipFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15);
    auto Chip = [&](const FString& Line, const FLinearColor& Ink, const FLinearColor& Gem)
    {
        const float W = Width(Line, ChipFont) + 58.f;
        const FVector2D ChipAt(At.X + 40, Below), ChipSize(W, 34);
        Box(Elements, Layer, Geometry, ChipAt, ChipSize, Srgb(.028f, .022f, .034f, .80f));
        Box(Elements, Layer, Geometry, ChipAt, FVector2D(ChipSize.X, 1.5), Srgb(.70f, .56f, .34f, .55f));
        Box(Elements, Layer, Geometry, ChipAt + FVector2D(0, ChipSize.Y - 1.5), FVector2D(ChipSize.X, 1.5), Srgb(.70f, .56f, .34f, .25f));
        Diamond(Elements, Layer + 1, Geometry, ChipAt + FVector2D(20, ChipSize.Y * .5), FVector2D(6, 9), 1.5f, Gem);
        Text(Elements, Layer + 1, Geometry, Line, ChipAt + FVector2D(40, ChipSize.Y * .5), 0.f, ChipFont, Ink);
        Below += ChipSize.Y + 6;
    };
    if (!View.Quest.IsEmpty()) Chip(View.Quest, Srgb(.80f, .86f, .96f), Srgb(.55f, .72f, .95f));
    for (const FString& Notice : View.Notices) if (!Notice.IsEmpty()) Chip(Notice, Srgb(.95f, .90f, .80f), Srgb(.95f, .70f, .34f));
    // The interaction in reach: a key cap and the verb, over the command ribbon.
    if (!View.Prompt.IsEmpty())
    {
        FString Name, Keys, Verb; SplitPrompt(View.Prompt, Name, Keys, Verb);
        const FSlateFontInfo NameFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, 18), KeyFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 14), VerbFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 16);
        const float NameW = Name.IsEmpty() ? 0.f : Width(Name, NameFont) + 22.f, KeyW = Keys.IsEmpty() ? 0.f : Width(Keys, KeyFont) + 22.f, VerbW = Width(Verb, VerbFont);
        const float Total = NameW + KeyW + (KeyW > 0 ? 12.f : 0.f) + VerbW + 44.f;
        const FVector2D PromptAt(Size.X * .5 - Total * .5, Size.Y - 330), PromptSize(Total, 44);
        Box(Elements, Layer, Geometry, PromptAt, PromptSize, Srgb(.028f, .022f, .034f, .84f));
        Box(Elements, Layer, Geometry, PromptAt, FVector2D(PromptSize.X, 1.5), Srgb(.78f, .62f, .38f, .7f));
        Box(Elements, Layer, Geometry, PromptAt + FVector2D(0, PromptSize.Y - 1.5), FVector2D(PromptSize.X, 1.5), Srgb(.78f, .62f, .38f, .35f));
        for (const float X : {0.f, float(PromptSize.X)}) Diamond(Elements, Layer + 1, Geometry, PromptAt + FVector2D(X, PromptSize.Y * .5), FVector2D(6, 9), 1.5f, Srgb(.95f, .74f, .40f));
        float X = PromptAt.X + 22;
        const float Mid = PromptAt.Y + PromptSize.Y * .5f;
        if (!Name.IsEmpty()) { Text(Elements, Layer + 1, Geometry, Name, FVector2D(X, Mid), 0.f, NameFont, Srgb(.96f, .84f, .58f)); X += NameW; }
        if (!Keys.IsEmpty())
        {
            Box(Elements, Layer + 1, Geometry, FVector2D(X, Mid - 13), FVector2D(KeyW, 26), Srgb(.16f, .14f, .13f, .95f));
            Box(Elements, Layer + 1, Geometry, FVector2D(X, Mid + 11), FVector2D(KeyW, 2), Srgb(.62f, .50f, .34f, .9f));
            Text(Elements, Layer + 2, Geometry, Keys, FVector2D(X + KeyW * .5f, Mid), .5f, KeyFont, Srgb(.94f, .90f, .84f));
            X += KeyW + 12.f;
        }
        Text(Elements, Layer + 1, Geometry, Verb, FVector2D(X, Mid), 0.f, VerbFont, Srgb(.90f, .88f, .84f));
    }
    // The controls, faint, at the bottom left.
    Text(Elements, Layer, Geometry, View.bKorean ? TEXT("WASD  이동      TAB  기억      ESC  메뉴") : TEXT("WASD  Move      TAB  Memories      ESC  Menu"),
        FVector2D(30, Size.Y - 28), 0.f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.78f, .74f, .68f, .62f));
    return Layer + 3;
}
