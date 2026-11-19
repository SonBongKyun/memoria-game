#include "Presentation/MemoriaDemoCompletionWidget.h"
#include "Presentation/MemoriaFonts.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Presentation/MemoriaHudKit.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"

namespace
{
void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Ink)
{ FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Ink); }
void InkText(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& G, const FString& Line, const FVector2D& At, const FSlateFontInfo& Font, const FLinearColor& Ink)
{ FSlateDrawElement::MakeText(Elements, Layer, G.ToPaintGeometry(FVector2f(MemoriaHudKit::Width(Line, Font), Font.Size * 2.f), FSlateLayoutTransform(FVector2f(At))), Line, Font, ESlateDrawEffect::None, Ink); }
TArray<FString> Wrap(const FString& Text, const FSlateFontInfo& Font, float Width)
{
    TArray<FString> Out, Paragraphs; Text.ParseIntoArray(Paragraphs, TEXT("\n"), false);
    for (const FString& Paragraph : Paragraphs)
    {
        FString Line;
        for (TCHAR C : Paragraph)
        {
            if (!Line.IsEmpty() && MemoriaHudKit::Width(Line + FString::Chr(C), Font) > Width)
            { Out.Add(Line); Line.Reset(); }
            Line.AppendChar(C);
        }
        Out.Add(Line);
    }
    return Out;
}
}
void UMemoriaDemoCompletionWidget::Configure(UMemoriaRunSubsystem* InRun, UMemoriaCheckpointSubsystem* InCheckpoint, const FVector2D& InPosition, bool bInKo)
{
    Run = InRun; Checkpoint = InCheckpoint; SourcePosition = InPosition; bKo = bInKo;
    View = Run ? MemoriaDemoCompletion::Build(*Run, bKo) : FMemoriaDemoCompletionView();
    Selected = ScrollLine = 0; bReview = bSaveAttempted = bSaveSucceeded = false; DiskStatus.Reset();
}
void UMemoriaDemoCompletionWidget::Navigate(int32 Delta) { Selected = (Selected + Delta + ItemCount) % ItemCount; }
void UMemoriaDemoCompletionWidget::Scroll(int32 Delta) { ScrollLine = FMath::Clamp(ScrollLine + Delta, 0, MaxScroll); }
void UMemoriaDemoCompletionWidget::Confirm() { Activate(Selected); }
void UMemoriaDemoCompletionWidget::Back()
{
    if (bReview) { bReview = false; ScrollLine = 0; return; }
    OnAction.ExecuteIfBound(EMemoriaDemoCompletionAction::ReturnToField);
}
void UMemoriaDemoCompletionWidget::Activate(int32 Index)
{
    if (Index < 0 || Index >= ItemCount) return;
    Selected = Index;
    if (Index == 0) { bReview = !bReview; ScrollLine = 0; }
    else if (Index == 1) SaveCurrentProgress();
    else OnAction.ExecuteIfBound(Index == 2 ? EMemoriaDemoCompletionAction::ReturnToField : EMemoriaDemoCompletionAction::Title);
}
bool UMemoriaDemoCompletionWidget::SaveCurrentProgress()
{
    bSaveAttempted = true;
    if (!Run || !Checkpoint || !MemoriaDemoCompletion::IsComplete(Run->GetRunSnapshot()))
    { bSaveSucceeded = false; DiskStatus.Reset(); return false; }
    bSaveSucceeded = Checkpoint->SaveChapterMap(TEXT("drift_shelter"), SourcePosition);
    DiskStatus = Checkpoint->GetStatusText();
    return bSaveSucceeded;
}
FString UMemoriaDemoCompletionWidget::ItemLabel(int32 Index) const
{
    if (Index == 0) return bReview ? (bKo ? TEXT("구간 요약") : TEXT("Segment summary")) : (bKo ? TEXT("기록 돌아보기") : TEXT("Review records"));
    if (Index == 1) return bSaveAttempted && !bSaveSucceeded ? (bKo ? TEXT("저장 다시 시도") : TEXT("Retry save")) : (bKo ? TEXT("현재 진행 저장") : TEXT("Save current progress"));
    if (Index == 2) return bKo ? TEXT("드리프트로 돌아가기") : TEXT("Return to Drift Shelter");
    return Index == 3 ? (bKo ? TEXT("타이틀로") : TEXT("Return to title")) : FString();
}
FString UMemoriaDemoCompletionWidget::VisibleText() const
{
    const FString Status = !bSaveAttempted ? (bKo ? TEXT("현재 진행을 저장할 수 있습니다.") : TEXT("You can save your current progress."))
        : bSaveSucceeded ? (bKo ? TEXT("현재 진행을 저장했습니다.") : TEXT("Current progress saved."))
        : (bKo ? TEXT("저장하지 못했습니다. 현재 진행은 유지됩니다. 다시 시도할 수 있습니다.") : TEXT("Could not save. Your current run is intact; you can retry."));
    return View.Title + TEXT("\n") + (bReview ? View.Review : View.Summary) + TEXT("\n\n") + Status;
}
int32 UMemoriaDemoCompletionWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
    Layer = Super::NativePaint(Args, G, Clip, Elements, Layer, Style, bEnabled) + 1;
    const FVector2D Size = G.GetLocalSize(); const float K = FMath::Clamp(FMath::Min(float(Size.X) / 1024.f, float(Size.Y) / 720.f), .1f, 1.f);
    const FVector2D Extent(960.f * K, 620.f * K), At = (Size - Extent) * .5f;
    const FLinearColor Gold(.66f, .49f, .27f), Paper(.88f, .85f, .77f);
    Box(Elements, Layer, G, FVector2D::ZeroVector, Size, FLinearColor(.006f, .008f, .012f, .90f));
    Box(Elements, Layer + 1, G, At - FVector2D(2, 2), Extent + FVector2D(4, 4), Gold);
    Box(Elements, Layer + 2, G, At, Extent, FLinearColor(.014f, .016f, .022f, .99f));
    auto Font = [&](int32 Points) { return MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::Max(8, FMath::RoundToInt(Points * K))); };
    InkText(Elements, Layer + 3, G, View.Title, At + FVector2D(32, 24) * K, Font(30), Gold);
    const auto BodyFont = Font(18); const float LineHeight = 27.f * K;
    const auto Lines = Wrap(bReview ? View.Review : View.Summary, BodyFont, Extent.X - 64.f * K);
    const int32 Visible = FMath::Max(1, FMath::FloorToInt(350.f * K / LineHeight));
    MaxScroll = FMath::Max(0, Lines.Num() - Visible);
    const int32 First = FMath::Clamp(ScrollLine, 0, MaxScroll);
    for (int32 I = First; I < Lines.Num() && I < First + Visible; ++I)
        InkText(Elements, Layer + 3, G, Lines[I], At + FVector2D(32.f * K, 91.f * K + (I - First) * LineHeight), BodyFont, Paper);
    const FString Status = VisibleText().RightChop((View.Title + TEXT("\n") + (bReview ? View.Review : View.Summary) + TEXT("\n\n")).Len());
    InkText(Elements, Layer + 3, G, Status, At + FVector2D(32, 463) * K, Font(15), bSaveAttempted && !bSaveSucceeded ? FLinearColor(.95f, .55f, .35f) : Paper);
    Rows.Reset();
    const float RowW = (Extent.X - 70.f * K) / ItemCount;
    for (int32 I = 0; I < ItemCount; ++I)
    {
        const FVector2D ActionAt = At + FVector2D(26.f * K + I * (RowW + 6.f * K), 515.f * K);
        const FVector2D RowSize(RowW, 50.f * K); Rows.Add(FBox2D(ActionAt, ActionAt + RowSize));
        Box(Elements, Layer + 3, G, ActionAt, RowSize, I == Selected ? FLinearColor(.16f, .10f, .04f) : FLinearColor(.035f, .04f, .055f));
        const FString Label = ItemLabel(I); auto LabelFont = Font(16);
        while (LabelFont.Size > 8 && MemoriaHudKit::Width(Label, LabelFont) > RowW - 12.f * K) --LabelFont.Size;
        InkText(Elements, Layer + 4, G, Label, ActionAt + FVector2D((RowW - MemoriaHudKit::Width(Label, LabelFont)) * .5f, 13.f * K), LabelFont, I == Selected ? Gold : Paper);
    }
    InkText(Elements, Layer + 4, G, bKo ? TEXT("휠 / PgUp·PgDn 기록 이동   ↑↓ 선택   Enter 실행   Esc 돌아가기") : TEXT("Wheel / PgUp·PgDn scroll   ↑↓ choose   Enter confirm   Esc return"), At + FVector2D(32, 586) * K, Font(12), Paper * .75f);
    return Layer + 5;
}
int32 UMemoriaDemoCompletionWidget::RowAt(const FGeometry& G, const FVector2D& Screen) const
{ const FVector2D Local = G.AbsoluteToLocal(Screen); for (int32 I = 0; I < Rows.Num(); ++I) if (Rows[I].IsInside(Local)) return I; return INDEX_NONE; }
FReply UMemoriaDemoCompletionWidget::NativeOnMouseMove(const FGeometry& G, const FPointerEvent& Event)
{ const int32 Row = RowAt(G, Event.GetScreenSpacePosition()); if (Row != INDEX_NONE) Selected = Row; return FReply::Handled(); }
FReply UMemoriaDemoCompletionWidget::NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{ const int32 Row = RowAt(G, Event.GetScreenSpacePosition()); if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Row != INDEX_NONE) Activate(Row); return FReply::Handled(); }
FReply UMemoriaDemoCompletionWidget::NativeOnMouseWheel(const FGeometry&, const FPointerEvent& Event)
{ Scroll(Event.GetWheelDelta() > 0.f ? -3 : 3); return FReply::Handled(); }
