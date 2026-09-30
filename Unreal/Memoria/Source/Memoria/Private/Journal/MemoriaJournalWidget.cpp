#include "Journal/MemoriaJournalWidget.h"
#include "Journal/MemoriaJournal.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaBattleEntryArt.h"
#include "Presentation/MemoriaNarrativeArtwork.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
namespace
{
using MemoriaUiKit::Srgb;
FVector2D Measure(const FString& S, const FSlateFontInfo& Font) { return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font); }
void Text(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& S, const FVector2D& At, const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = Measure(S, Font);
    FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At + FVector2D(1, 1)))), S, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A * .6f));
    FSlateDrawElement::MakeText(Elements, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), S, Font, ESlateDrawEffect::None, Color);
}
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{ FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color); }
void Panel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Fill, const FLinearColor& Border, float W)
{
    Box(Elements, Layer, G, At, Size, Fill);
    Box(Elements, Layer + 1, G, At, FVector2D(Size.X, W), Border); Box(Elements, Layer + 1, G, At + FVector2D(0, Size.Y - W), FVector2D(Size.X, W), Border);
    Box(Elements, Layer + 1, G, At, FVector2D(W, Size.Y), Border); Box(Elements, Layer + 1, G, At + FVector2D(Size.X - W, 0), FVector2D(W, Size.Y), Border);
}
TArray<FString> Wrap(const FString& Text, const FSlateFontInfo& Font, float Width)
{
    TArray<FString> Out, Paragraphs; Text.ParseIntoArray(Paragraphs, TEXT("\n"), false);
    for (const FString& P : Paragraphs)
    {
        TArray<FString> Words; P.ParseIntoArray(Words, TEXT(" "), true);
        FString Line;
        for (const FString& W : Words)
        {
            const FString Try = Line.IsEmpty() ? W : Line + TEXT(" ") + W;
            if (!Line.IsEmpty() && Measure(Try, Font).X > Width) { Out.Add(Line); Line = W; } else Line = Try;
        }
        Out.Add(Line);
    }
    return Out;
}
// UITheme.get_speaker_color for the people the journal lists.
FLinearColor PersonColor(const FString& Name)
{
    if (Name == TEXT("Elia")) return Srgb(.62f, .78f, .95f);
    if (Name == TEXT("Malet")) return Srgb(.85f, .68f, .35f);
    if (Name.StartsWith(TEXT("Tobias"))) return Srgb(.7f, .72f, .6f);
    if (Name == TEXT("Sable")) return Srgb(.72f, .6f, .82f);
    if (Name == TEXT("Kairos")) return Srgb(.82f, .45f, .45f);
    return Srgb(.75f, .7f, .65f);
}
}
void UMemoriaJournalWidget::Configure(const UMemoriaRunSubsystem* InRun, bool bInKo)
{
    Run = InRun; bKo = bInKo; Tab = 0; Selected = 0; FirstLine = 0;
    if (!Backdrop && (Backdrop = MemoriaBattleEntryArt::Load(TEXT("res://assets/cg/generated/ui_story_journal_backdrop_v3.png"))))
    { BackdropBrush.SetResourceObject(Backdrop); BackdropBrush.ImageSize = FVector2D(Backdrop->GetSizeX(), Backdrop->GetSizeY()); BackdropBrush.DrawAs = ESlateBrushDrawType::Image; }
    UpdateArt(); Summary = SummaryText();
}
void UMemoriaJournalWidget::SetTab(int32 InTab) { Tab = FMath::Clamp(InTab, 0, TabCount - 1); Selected = 0; FirstLine = 0; UpdateArt(); }
void UMemoriaJournalWidget::Move(int32 Delta)
{
    const int32 Count = SelectableLines().Num();
    if (Count) Selected = FMath::Clamp(Selected + Delta, 0, Count - 1);
    UpdateArt();
}
FReply UMemoriaJournalWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{ Move(Event.GetWheelDelta() > 0 ? -1 : 1); return FReply::Handled(); }
bool UMemoriaJournalWidget::Flag(const FString& Id) const { const auto* R = Run.Get(); return R && !Id.IsEmpty() && R->GetRunSnapshot().GetFlag(Id); }
const TArray<FMemoriaJournalEntry>& UMemoriaJournalWidget::TabEntries() const
{ return Tab == 0 ? MemoriaJournal::Events() : Tab == 1 ? MemoriaJournal::People() : Tab == 2 ? MemoriaJournal::World() : MemoriaJournal::Choices(); }
TArray<UMemoriaJournalWidget::FLine> UMemoriaJournalWidget::ListLines() const
{
    TArray<FLine> Out;
    const auto& Entries = TabEntries();
    int32 LastChapter = 0;
    for (int32 I = 0; I < Entries.Num(); ++I)
    {
        const auto& E = Entries[I];
        if (!Flag(E.Flag)) continue;
        // _populate_events / _populate_world: a header when the chapter changes.
        if ((Tab == 0 || Tab == 2) && E.Chapter != LastChapter)
        {
            LastChapter = E.Chapter;
            const FString Name = MemoriaJournal::ChapterName(E.Chapter, bKo);
            Out.Add({Tab == 0 ? (bKo ? FString::Printf(TEXT("%d장 · %s"), E.Chapter, *Name) : FString::Printf(TEXT("Chapter %d: %s"), E.Chapter, *Name))
                              : (bKo ? FString::Printf(TEXT("%d장에서 알게 된 것 · %s"), E.Chapter, *Name) : FString::Printf(TEXT("Learned in Chapter %d: %s"), E.Chapter, *Name)),
                Srgb(.62f, .58f, .66f), -1});
        }
        const FLinearColor Color = Tab == 1 ? PersonColor(E.Title) : Tab == 3 ? Srgb(.7f, .6f, .45f) : E.Title.StartsWith(TEXT("[Hidden]")) ? Srgb(.6f, .5f, .7f) : Srgb(.75f, .7f, .65f);
        Out.Add({E.TitleIn(bKo), Color, I});
    }
    if (Out.IsEmpty() && Tab == 3) Out.Add({Loc(TEXT("No major choices recorded yet."), TEXT("아직 기록된 큰 선택이 없습니다.")), Srgb(.5f, .47f, .45f), -1});
    return Out;
}
TArray<int32> UMemoriaJournalWidget::SelectableLines() const
{
    TArray<int32> Out; const auto Lines = ListLines();
    for (int32 I = 0; I < Lines.Num(); ++I) if (Lines[I].Entry >= 0) Out.Add(I);
    return Out;
}
const FMemoriaJournalEntry* UMemoriaJournalWidget::SelectedEntry() const
{
    const auto Lines = ListLines(); const auto Pickable = SelectableLines();
    if (!Pickable.IsValidIndex(Selected)) return nullptr;
    const int32 Entry = Lines[Pickable[Selected]].Entry;
    return TabEntries().IsValidIndex(Entry) ? &TabEntries()[Entry] : nullptr;
}
FString UMemoriaJournalWidget::DetailTitle() const
{
    const auto* E = SelectedEntry();
    return E ? E->TitleIn(bKo) : Loc(TEXT("Select an entry..."), TEXT("항목을 고르세요..."));
}
FString UMemoriaJournalWidget::DetailBody() const
{
    const auto* E = SelectedEntry();
    if (!E) return FString();
    // _populate_npcs: the role, then the description.
    if (Tab == 1) return (bKo && !E->RoleKo.IsEmpty() ? E->RoleKo : E->Role) + TEXT("\n\n") + E->DescIn(bKo);
    return E->DescIn(bKo);
}
bool UMemoriaJournalWidget::HasDetailArt() const { return Art != nullptr; }
void UMemoriaJournalWidget::UpdateArt()
{
    // The entry's illustration when the port carries that picture (the dialogue CGs and portraits).
    const auto* E = SelectedEntry();
    Art = E && !E->Art.IsEmpty() ? MemoriaNarrativeArtwork::Load(E->Art) : nullptr;
    if (!Art) Art = E && !E->Art.IsEmpty() ? MemoriaBattleEntryArt::Load(E->Art) : nullptr;
    if (Art) { ArtBrush.SetResourceObject(Art); ArtBrush.ImageSize = FVector2D(Art->GetSizeX(), Art->GetSizeY()); ArtBrush.DrawAs = ESlateBrushDrawType::Image; }
}
FString UMemoriaJournalWidget::SummaryText() const
{
    // _update_journal_summary, less the losses (the world rewrite's records are not ported).
    const auto* R = Run.Get();
    const int32 Chapter = R ? int32(R->GetRunSnapshot().CurrentChapter) : 0;
    int32 Burned = 0, Held = 0, Unlocked = 0, Illustrated = 0;
    if (R && R->GetPlayerMemory())
    {
        const auto Snapshot = R->GetPlayerMemory()->GetSnapshot();
        Burned = Snapshot.BurnedHistory.Num(); Held = FMath::Max(0, Snapshot.Owned.Num() - Burned);
    }
    for (const auto& E : MemoriaJournal::Events())
        if (Flag(E.Flag)) { ++Unlocked; if (!E.Art.IsEmpty() && (MemoriaNarrativeArtwork::Load(E.Art) || MemoriaBattleEntryArt::Load(E.Art))) ++Illustrated; }
    const FString Name = MemoriaJournal::ChapterName(Chapter, bKo);
    return bKo ? FString::Printf(TEXT("%d장 / %s    보유 %d    연소 %d    삽화 %d/%d"), Chapter, *Name, Held, Burned, Illustrated, Unlocked)
               : FString::Printf(TEXT("Ch.%d / %s    Held: %d    Burned: %d    Illustrated: %d/%d"), Chapter, *Name, Held, Burned, Illustrated, Unlocked);
}
int32 UMemoriaJournalWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const float S = Size.Y / 720.f;
    if (Backdrop)
    {
        const float Cover = FMath::Max(Size.X / Backdrop->GetSizeX(), Size.Y / Backdrop->GetSizeY());
        const FVector2D ArtSize(Backdrop->GetSizeX() * Cover, Backdrop->GetSizeY() * Cover);
        FSlateDrawElement::MakeBox(Elements, L, Geometry.ToPaintGeometry(FVector2f(ArtSize), FSlateLayoutTransform(FVector2f((Size - ArtSize) * .5))), &BackdropBrush, ESlateDrawEffect::None, FLinearColor::White);
    }
    Box(Elements, L + 1, Geometry, FVector2D::ZeroVector, Size, Srgb(.015f, .012f, .025f, Backdrop ? .46f : .88f));
    const FVector2D At(Size.X * .10f, Size.Y * .05f), P(Size.X * .80f, Size.Y * .90f);
    Panel(Elements, L + 2, Geometry, At, P, Srgb(.035f, .03f, .05f, .82f), Srgb(.5f, .38f, .22f, .78f), 2.f * S);
    const float M = 18.f * S, Inner = P.X - M * 2.f;
    auto Centre = [&](const FString& T, float Y, const FSlateFontInfo& F, const FLinearColor& C) { Text(Elements, L + 4, Geometry, T, FVector2D(At.X + (P.X - Measure(T, F).X) * .5f, Y), F, C); };
    const FSlateFontInfo Head = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, FMath::RoundToInt(20 * S)), Small = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(13 * S));
    float Y = At.Y + M;
    Centre(Loc(TEXT("JOURNAL, Field Notes of a Memory Carrier"), TEXT("일지 · 기억 운반자의 현장 기록")), Y, Head, Srgb(.8f, .7f, .5f)); Y += 34 * S;
    Centre(Summary, Y, Small, Srgb(.55f, .62f, .72f)); Y += 26 * S;
    const FString Tabs[TabCount] = {Loc(TEXT("Events"), TEXT("사건")), Loc(TEXT("People"), TEXT("인물")), Loc(TEXT("World"), TEXT("세계")), Loc(TEXT("Choices"), TEXT("선택"))};
    const FVector2D TabSize(110 * S, 30 * S);
    const float TabsWidth = TabCount * TabSize.X + (TabCount - 1) * 8 * S;
    for (int32 I = 0; I < TabCount; ++I)
    {
        const FVector2D T(At.X + (P.X - TabsWidth) * .5f + I * (TabSize.X + 8 * S), Y);
        Panel(Elements, L + 3, Geometry, T, TabSize, I == Tab ? Srgb(.16f, .12f, .10f, .95f) : Srgb(.08f, .07f, .09f, .85f), I == Tab ? Srgb(.85f, .65f, .35f, .9f) : Srgb(.35f, .28f, .2f, .5f), 1.f * S);
        Text(Elements, L + 5, Geometry, Tabs[I], T + (TabSize - Measure(Tabs[I], Small)) * .5f, Small, I == Tab ? Srgb(.95f, .82f, .5f) : Srgb(.6f, .56f, .5f));
    }
    Y += TabSize.Y + 10 * S;
    Box(Elements, L + 3, Geometry, FVector2D(At.X + M, Y), FVector2D(Inner, S), Srgb(.5f, .38f, .22f, .45f)); Y += 10 * S;
    const float Bottom = At.Y + P.Y - M - 22 * S, DetailW = 380 * S, ListW = Inner - DetailW - 12 * S, RowH = 28 * S, Gap = 3 * S;
    const auto Lines = ListLines(); const auto Pickable = SelectableLines();
    const int32 Chosen = Pickable.IsValidIndex(Selected) ? Pickable[Selected] : -1;
    const int32 Visible = FMath::Max(1, int32((Bottom - Y + Gap) / (RowH + Gap)));
    if (Chosen >= 0) { if (Chosen < FirstLine) FirstLine = Chosen; if (Chosen >= FirstLine + Visible) FirstLine = Chosen - Visible + 1; }
    FirstLine = FMath::Clamp(FirstLine, 0, FMath::Max(0, Lines.Num() - Visible));
    const FSlateFontInfo Row = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(14 * S));
    for (int32 N = 0; N < Visible && Lines.IsValidIndex(FirstLine + N); ++N)
    {
        const int32 I = FirstLine + N; const auto& Line = Lines[I];
        const FVector2D R(At.X + M, Y + N * (RowH + Gap));
        const FLinearColor C = Line.Color;
        if (Line.Entry >= 0)
        {
            // _add_list_button: the colour at a fifth as the fill and a 3 px left rule; the chosen one lit.
            const bool bOn = I == Chosen;
            Box(Elements, L + 3, Geometry, R, FVector2D(ListW, RowH), FLinearColor(C.R * (bOn ? .3f : .2f), C.G * (bOn ? .3f : .2f), C.B * (bOn ? .3f : .2f), bOn ? .6f : .4f));
            Box(Elements, L + 4, Geometry, R, FVector2D(3 * S, RowH), bOn ? C : FLinearColor(C.R * .4f, C.G * .4f, C.B * .4f, .3f));
            Text(Elements, L + 5, Geometry, Line.Label, R + FVector2D(10 * S, (RowH - Measure(Line.Label, Row).Y) * .5f), Row, bOn ? C : Srgb(.72f, .68f, .62f));
        }
        else Text(Elements, L + 5, Geometry, Line.Label, R + FVector2D(4 * S, (RowH - Measure(Line.Label, Small).Y) * .5f), Small, C);
    }
    // The detail: title, rule, the illustration when there is one, then the text.
    const FVector2D D(At.X + M + ListW + 12 * S, Y), DS(DetailW, Bottom - Y);
    Panel(Elements, L + 3, Geometry, D, DS, Srgb(.05f, .04f, .06f, .8f), Srgb(.3f, .26f, .22f, .6f), 1.f * S);
    const FSlateFontInfo DetailHead = MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(16 * S));
    Text(Elements, L + 5, Geometry, DetailTitle(), D + FVector2D(14, 12) * S, DetailHead, Srgb(.8f, .7f, .55f));
    Box(Elements, L + 5, Geometry, D + FVector2D(14 * S, 40 * S), FVector2D(DS.X - 28 * S, S), Srgb(.5f, .38f, .22f, .4f));
    float DY = D.Y + 50 * S;
    if (Art)
    {
        const float W = DS.X - 28 * S, H = FMath::Min(170.f * S, W * Art->GetSizeY() / FMath::Max(1.f, float(Art->GetSizeX())));
        const float ArtW = H * Art->GetSizeX() / FMath::Max(1.f, float(Art->GetSizeY()));
        FSlateDrawElement::MakeBox(Elements, L + 5, Geometry.ToPaintGeometry(FVector2f(ArtW, H), FSlateLayoutTransform(FVector2f(D.X + (DS.X - ArtW) * .5f, DY))), &ArtBrush, ESlateDrawEffect::None, FLinearColor::White);
        DY += H + 10 * S;
    }
    for (const FString& Line : Wrap(DetailBody(), Small, DS.X - 28 * S))
    {
        if (DY > D.Y + DS.Y - 20 * S) break;
        Text(Elements, L + 5, Geometry, Line, FVector2D(D.X + 14 * S, DY), Small, Srgb(.65f, .6f, .55f));
        DY += Measure(TEXT("가"), Small).Y * 1.25f;
    }
    const FString Hint = Loc(TEXT("TAB / ←→ Tabs    ↑↓ Choose    [ESC] Close"), TEXT("TAB / ←→ 탭    ↑↓ 선택    [ESC] 닫기"));
    Text(Elements, L + 4, Geometry, Hint, FVector2D(At.X + P.X - M - Measure(Hint, Small).X, Bottom + 4 * S), Small, Srgb(.4f, .35f, .3f));
    return L + 7;
}
