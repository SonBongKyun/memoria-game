#include "Combat/MemoriaFieldHudWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Combat/MemoriaFieldCombatSubsystem.h"
#include "Combat/MemoriaFieldCombatTypes.h"
#include "Engine/World.h"
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
    for (const auto& Item : MemoriaCombatTuning::FieldItems)
        if (auto* Icon = MemoriaHudKit::LoadItem(Item.Id)) { Icons.Add(Item.Id, Icon); MemoriaHudKit::Brush(IconBrushes.Add(Item.Id), Icon); }
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
    // S356: the five existing quick slots read as a travelling apothecary's kit. Original cutout paintings
    // have room to breathe; a category seam, a dark key cap and a separate count keep the art legible.
    if (const auto* Combat = GetWorld() ? GetWorld()->GetSubsystem<UMemoriaFieldCombatSubsystem>() : nullptr; Combat && Combat->GetPlayer())
    {
        static const TCHAR* Keys[MemoriaCombatTuning::QuickSlots] = {TEXT("Z"), TEXT("X"), TEXT("C"), TEXT("V"), TEXT("B")};
        const FVector2D Cell(90, 98); const float Gap = 7.f;
        const float TrayWidth = MemoriaCombatTuning::QuickSlots * (Cell.X + Gap) - Gap;
        const FVector2D TrayAt(Size.X - 26 - TrayWidth, Size.Y - Cell.Y - 28);
        const float Cooling = FMath::Clamp(Combat->GetItemCooldown() / MemoriaCombatTuning::ItemCooldown, 0.f, 1.f);
        Box(Elements, Layer, Geometry, TrayAt - FVector2D(7, 7), FVector2D(TrayWidth + 14, Cell.Y + 14), Srgb(.012f, .012f, .018f, .54f));
        for (int32 I = 0; I < MemoriaCombatTuning::QuickSlots; ++I)
        {
            const FString Id = Combat->QuickItemId(I); const int64 Count = Combat->QuickItemCount(I);
            const FVector2D SlotAt = TrayAt + FVector2D(I * (Cell.X + Gap), 0);
            const float Lit = Count > 0 ? 1.f : .28f;
            FLinearColor Accent = Id == TEXT("antidote") ? Srgb(.47f, .70f, .48f) : Id == TEXT("firebomb") ? Srgb(.92f, .47f, .22f)
                : Id == TEXT("smoke_bomb") ? Srgb(.53f, .64f, .78f) : Id == TEXT("witness_ink") ? Srgb(.68f, .53f, .84f)
                : Id == TEXT("hi_potion") ? Srgb(.96f, .72f, .41f) : Srgb(.35f, .61f, .87f);
            Box(Elements, Layer, Geometry, SlotAt - FVector2D(1, 1), Cell + FVector2D(2, 2), Srgb(.48f, .40f, .29f, .7f * Lit));
            Box(Elements, Layer, Geometry, SlotAt, Cell, Srgb(.028f, .025f, .035f, .94f));
            Box(Elements, Layer + 1, Geometry, SlotAt + FVector2D(3, 3), FVector2D(Cell.X - 6, 2), FLinearColor(Accent.R, Accent.G, Accent.B, .85f * Lit));
            // 80px of art, up from 62px. The image alpha leaves the frame and category colour visible.
            if (const FSlateBrush* Icon = IconBrushes.Find(Id)) Image(Elements, Layer + 1, Geometry, *Icon, SlotAt + FVector2D(5, 4), FVector2D(80, 80), FLinearColor(1, 1, 1, Lit));
            else Text(Elements, Layer + 1, Geometry, MemoriaCombatTuning::ItemName(Id, View.bKorean).Left(2), SlotAt + FVector2D(Cell.X * .5, 43), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 15), Srgb(.86f, .82f, .76f, Lit));
            if (Cooling > 0.f && Count > 0)
            {
                Box(Elements, Layer + 2, Geometry, SlotAt + FVector2D(4, 6), FVector2D(Cell.X - 8, 76 * Cooling), Srgb(.005f, .008f, .016f, .65f));
                Box(Elements, Layer + 2, Geometry, SlotAt + FVector2D(4, 80), FVector2D((Cell.X - 8) * (1.f - Cooling), 2), FLinearColor(Accent.R, Accent.G, Accent.B, .9f));
            }
            // Hardware-like key cap and count seal remain readable over every painting and during cooldown.
            Box(Elements, Layer + 3, Geometry, SlotAt + FVector2D(5, 7), FVector2D(22, 22), Srgb(.055f, .047f, .045f, .97f));
            Box(Elements, Layer + 3, Geometry, SlotAt + FVector2D(5, 27), FVector2D(22, 2), Srgb(.65f, .53f, .34f, .8f));
            Text(Elements, Layer + 4, Geometry, Keys[I], SlotAt + FVector2D(16, 17), .5f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.98f, .91f, .77f));
            const FString Amount = FString::Printf(TEXT("%lld"), Count);
            const FSlateFontInfo CountFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 16);
            const float CountWidth = FMath::Max(25.f, Width(Amount, CountFont) + 12.f);
            Box(Elements, Layer + 3, Geometry, SlotAt + FVector2D(Cell.X - CountWidth - 4, 59), FVector2D(CountWidth, 23), Srgb(.018f, .017f, .024f, .96f));
            Text(Elements, Layer + 4, Geometry, Amount, SlotAt + FVector2D(Cell.X - 10, 70), 1.f, CountFont, Count > 0 ? Srgb(1.f, .96f, .87f) : Srgb(.58f, .56f, .55f));
            Box(Elements, Layer + 3, Geometry, SlotAt + FVector2D(4, 84), FVector2D(Cell.X - 8, 1), Srgb(.48f, .40f, .29f, .34f));
            const FString Name = MemoriaCombatTuning::ItemName(Id, View.bKorean);
            const FSlateFontInfo NameFont = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 10);
            Text(Elements, Layer + 4, Geometry, Name, SlotAt + FVector2D(Cell.X * .5, 91), .5f, NameFont, Srgb(.84f, .79f, .70f, Count > 0 ? .94f : .48f));
        }
    }
    // The controls, faint, at the bottom left.
    Text(Elements, Layer, Geometry, View.bKorean ? TEXT("WASD  이동      TAB  기억      ESC  메뉴") : TEXT("WASD  Move      TAB  Memories      ESC  Menu"),
        FVector2D(30, Size.Y - 28), 0.f, MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, 13), Srgb(.78f, .74f, .68f, .62f));
    return Layer + 6;
}
