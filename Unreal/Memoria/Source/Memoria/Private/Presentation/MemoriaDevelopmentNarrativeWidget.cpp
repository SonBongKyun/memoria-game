#include "Presentation/MemoriaDevelopmentNarrativeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"

TSharedRef<SWidget> UMemoriaDevelopmentNarrativeWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
        auto* Panel = WidgetTree->ConstructWidget<UBorder>();
        Panel->SetBrushColor(FLinearColor(0.015f, 0.022f, 0.035f, 0.98f)); Panel->SetPadding(FMargin(32));
        auto* PanelSlot = Canvas->AddChildToCanvas(Panel);
        PanelSlot->SetAnchors(View.bCompactStatus ? FAnchors(0.03f, 0.03f, 0.97f, 0.25f) : FAnchors(0.08f, 0.12f, 0.92f, 0.9f)); PanelSlot->SetOffsets(FMargin(0));
        auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(); Panel->SetContent(Scroll);
        Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NarrativeText"));
        Message->SetAutoWrapText(true); Message->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.91f, 0.86f)));
        auto Font = Message->GetFont(); Font.Size = 22; Message->SetFont(Font); Scroll->AddChild(Message);
    }
    Refresh(); return Super::RebuildWidget();
}
void UMemoriaDevelopmentNarrativeWidget::Display(const FMemoriaNarrativeView& InView)
{
    const int32 Previous = SelectedOriginalIndex(); const bool SameStep = View.Header == InView.Header;
    View = InView; Selection = 0;
    if (SameStep) for (int32 I = 0; I < View.Choices.Num(); ++I) if (View.Choices[I].OriginalIndex == Previous) Selection = I;
    Refresh();
}
void UMemoriaDevelopmentNarrativeWidget::Navigate(int32 Direction)
{
    if (!View.bPaused && !View.Choices.IsEmpty()) { Selection = (Selection + Direction + View.Choices.Num()) % View.Choices.Num(); Refresh(); }
}
int32 UMemoriaDevelopmentNarrativeWidget::SelectedOriginalIndex() const
{ return View.Choices.IsValidIndex(Selection) ? View.Choices[Selection].OriginalIndex : INDEX_NONE; }
void UMemoriaDevelopmentNarrativeWidget::ConfirmIntent() { OnConfirm.ExecuteIfBound(SelectedOriginalIndex()); }
FString UMemoriaDevelopmentNarrativeWidget::VisibleText() const { return Message ? Message->GetText().ToString() : FString(); }
void UMemoriaDevelopmentNarrativeWidget::Refresh()
{
    if (!Message) return;
    FString Text = View.Header + TEXT("\n\n");
    if (View.bCompactStatus) Text += View.Body;
    else if (View.bPaused) Text += TEXT("PAUSED\n\nEnter / A or Back: return to the current line");
    else
    {
        if (!View.Speaker.IsEmpty()) Text += View.Speaker + TEXT("\n\n");
        if (!View.Narration.IsEmpty()) Text += View.Narration + TEXT("\n\n");
        if (!View.Body.IsEmpty()) Text += View.Body + TEXT("\n\n");
        for (int32 I = 0; I < View.Choices.Num(); ++I)
            Text += (I == Selection ? TEXT(">  ") : TEXT("    ")) + View.Choices[I].Text + TEXT("\n\n");
        Text += View.bDevelopmentStop ? TEXT("Development acceptance boundary reached.") : View.Choices.IsEmpty() ? TEXT("Enter / E / Space / A: continue") : TEXT("Up / Down / D-pad: select     Enter / A: confirm");
    }
    Message->SetText(FText::FromString(Text));
}
