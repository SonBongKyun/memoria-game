#include "Save/MemoriaSaveSlotsWidget.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Presentation/MemoriaFonts.h"
#include "Presentation/MemoriaUiKit.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

namespace
{
using MemoriaUiKit::Srgb;
void Box(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2D& At, const FVector2D& Size, const FLinearColor& Color)
{
    FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
}
void Text(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FString& Label, const FVector2D& At,
    const FSlateFontInfo& Font, const FLinearColor& Color)
{
    const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font);
    FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At))), Label, Font, ESlateDrawEffect::None, Color);
}
// Keep the existing three row baselines and height. Measure real glyph bounds before drawing;
// long records elide horizontally, and compact viewports fit each baseline's vertical allowance.
void FittedText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, FString Label, const FVector2D& At,
    FSlateFontInfo Font, const FLinearColor& Color, const FVector2D& Maximum)
{
    if (Maximum.X <= 0. || Maximum.Y <= 0.) return;
    const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    while (Font.Size > 1 && Measure->Measure(Label, Font).Y > Maximum.Y) --Font.Size;
    if (Measure->Measure(Label, Font).Y > Maximum.Y) return;
    if (Measure->Measure(Label, Font).X > Maximum.X)
    {
        const FString Ellipsis(TEXT("…"));
        while (!Label.IsEmpty() && Measure->Measure(Label + Ellipsis, Font).X > Maximum.X) Label = Label.LeftChop(1);
        Label += Ellipsis;
        if (Measure->Measure(Label, Font).X > Maximum.X) return;
    }
    Text(Out, Layer, G, Label, At, Font, Color);
}
}
bool UMemoriaSaveSlotsWidget::Ko() const { return !Settings || Settings->GetLocale() != TEXT("en"); }
void UMemoriaSaveSlotsWidget::Configure(UMemoriaCheckpointSubsystem* InCheckpoint, UMemoriaSettingsSubsystem* InSettings,
    EMemoriaSaveBrowserMode InMode, const FString& InMap, const FVector2D& InPosition, bool bInCanSave, float InTextScale)
{
    Checkpoint = InCheckpoint; Settings = InSettings; Mode = InMode; Map = InMap; Position = InPosition;
    bCanSave = bInCanSave && Checkpoint && Checkpoint->CanSaveManualSlot(1, Map, Position);
    TextScale = FMath::Clamp(InTextScale, 1.f, 1.25f);
    Selected = 0; bAskOverwrite = false; OverwriteChoice = 1; Notice.Reset(); Refresh();
}
void UMemoriaSaveSlotsWidget::Refresh()
{
    Summaries.Reset();
    if (!Checkpoint) return;
    if (Mode == EMemoriaSaveBrowserMode::Load) Summaries.Add(Checkpoint->GetAutosaveSummary());
    for (int32 SlotIndex = 1; SlotIndex <= UMemoriaCheckpointSubsystem::ManualSlotCount; ++SlotIndex) Summaries.Add(Checkpoint->GetManualSlotSummary(SlotIndex));
}
int32 UMemoriaSaveSlotsWidget::RowToSlot(int32 Row) const
{
    return Summaries.IsValidIndex(Row) ? Summaries[Row].Slot : INDEX_NONE;
}
bool UMemoriaSaveSlotsWidget::IsRowEnabled(int32 Row) const
{
    if (Row == GetRowCount() - 1) return true;
    if (!Summaries.IsValidIndex(Row) || !Checkpoint) return false;
    return Mode == EMemoriaSaveBrowserMode::Save
        ? bCanSave && Checkpoint->IsStorageEnabled() : Summaries[Row].CanLoad();
}
void UMemoriaSaveSlotsWidget::Navigate(int32 Direction)
{
    if (!Direction) return;
    if (bAskOverwrite) { OverwriteChoice = 1 - OverwriteChoice; return; }
    // Disabled slots remain focusable so their error/empty state can be read with keyboard or controller.
    Selected = (Selected + (Direction > 0 ? 1 : -1) + GetRowCount()) % GetRowCount();
}
void UMemoriaSaveSlotsWidget::Adjust(int32 Direction)
{
    if (bAskOverwrite && Direction) OverwriteChoice = 1 - OverwriteChoice;
}
void UMemoriaSaveSlotsWidget::Select(int32 Row)
{
    if (!bAskOverwrite && Row >= 0 && Row < GetRowCount()) Selected = Row;
}
bool UMemoriaSaveSlotsWidget::Back()
{
    if (bAskOverwrite) { bAskOverwrite = false; OverwriteChoice = 1; return true; }
    return false;
}
void UMemoriaSaveSlotsWidget::WriteSelected(bool bConfirmed)
{
    const int32 SlotIndex = RowToSlot(Selected);
    const bool bSaved = Checkpoint && bCanSave && Checkpoint->SaveManualSlot(SlotIndex, Map, Position, bConfirmed);
    Notice = bSaved ? Loc(TEXT("Manual slot saved."), TEXT("수동 슬롯에 저장했습니다."))
        : Loc(TEXT("Could not save here. Previous files retained."), TEXT("지금 저장할 수 없습니다. 기존 파일은 유지됩니다."));
    Refresh();
    if (bSaved) OnSaved.ExecuteIfBound(SlotIndex);
}
void UMemoriaSaveSlotsWidget::Confirm()
{
    if (bAskOverwrite)
    {
        const bool bReplace = OverwriteChoice == 0;
        bAskOverwrite = false; OverwriteChoice = 1;
        if (bReplace) WriteSelected(true);
        return;
    }
    if (Selected == GetRowCount() - 1) { OnClose.ExecuteIfBound(); return; }
    if (!IsRowEnabled(Selected))
    {
        Notice = Mode == EMemoriaSaveBrowserMode::Save ? Loc(TEXT("Saving is unavailable at this point."), TEXT("지금은 저장할 수 없습니다."))
            : Loc(TEXT("This slot has no valid save to load."), TEXT("이 슬롯에는 불러올 수 있는 저장이 없습니다."));
        return;
    }
    if (Mode == EMemoriaSaveBrowserMode::Load) { OnLoadRequested.ExecuteIfBound(RowToSlot(Selected)); return; }
    // Any existing file (even unreadable) asks before replacement. Backup-only slots also ask.
    const auto State = Summaries[Selected].State;
    if (State != EMemoriaSaveSlotState::Empty) { bAskOverwrite = true; OverwriteChoice = 1; Notice.Reset(); return; }
    WriteSelected(false);
}
FString UMemoriaSaveSlotsWidget::RegionLabel(const FString& InMap, bool bKo)
{
    if (InMap == TEXT("verdan_market")) return bKo ? TEXT("베르단 시장") : TEXT("Verdan Market");
    if (InMap == TEXT("vn_host")) return bKo ? TEXT("이야기 진행") : TEXT("Story continuation");
    if (bKo) return MemoriaChapterMaps::Korean(InMap);
    TArray<FString> Words; InMap.ParseIntoArray(Words, TEXT("_"), true);
    for (FString& Word : Words) if (!Word.IsEmpty()) Word[0] = FChar::ToUpper(Word[0]);
    return FString::Join(Words, TEXT(" "));
}
FString UMemoriaSaveSlotsWidget::StateLabel(EMemoriaSaveSlotState State) const
{
    switch (State)
    {
    case EMemoriaSaveSlotState::Ready: return Loc(TEXT("Ready"), TEXT("불러오기 가능"));
    case EMemoriaSaveSlotState::Backup: return Loc(TEXT("Backup available"), TEXT("백업 복구 가능"));
    case EMemoriaSaveSlotState::Empty: return Loc(TEXT("Empty slot"), TEXT("비어 있는 슬롯"));
    case EMemoriaSaveSlotState::Corrupt: return Loc(TEXT("Unreadable or incompatible"), TEXT("손상되었거나 호환되지 않음"));
    default: return Loc(TEXT("Storage unavailable"), TEXT("저장소 사용 불가"));
    }
}
int32 UMemoriaSaveSlotsWidget::RowAt(const FGeometry& Geometry, const FVector2D& Screen) const
{
    const FVector2D P = Geometry.AbsoluteToLocal(Screen);
    for (int32 Row = 0; Row < Rows.Num(); ++Row) if (Rows[Row].IsInside(P)) return Row;
    return INDEX_NONE;
}
FReply UMemoriaSaveSlotsWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const int32 Row = RowAt(Geometry, Event.GetScreenSpacePosition());
    if (Row != INDEX_NONE) { if (bAskOverwrite) OverwriteChoice = Row; else Select(Row); }
    return FReply::Handled();
}
FReply UMemoriaSaveSlotsWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::RightMouseButton) { if (!Back()) OnClose.ExecuteIfBound(); return FReply::Handled(); }
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
    const int32 Row = RowAt(Geometry, Event.GetScreenSpacePosition());
    if (Row != INDEX_NONE) { if (bAskOverwrite) OverwriteChoice = Row; else Select(Row); Confirm(); }
    return FReply::Handled();
}
int32 UMemoriaSaveSlotsWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 L = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const FVector2D Panel(FMath::Min(Size.X * .92, 1000.), FMath::Min(Size.Y * .92, 700.));
    const FVector2D At = (Size - Panel) * .5;
    const float Pad = 26.f, W = Panel.X - Pad * 2;
    auto Font = [&](int32 Points) { return MemoriaFonts::Get(MemoriaFonts::EStyle::Ui, FMath::RoundToInt(Points * TextScale)); };
    const auto Title = MemoriaFonts::Get(MemoriaFonts::EStyle::Title, FMath::RoundToInt(28 * TextScale));
    const auto Body = Font(17), Small = Font(14);
    Rows.Reset();
    Box(Elements, L, Geometry, FVector2D::ZeroVector, Size, FLinearColor(0, 0, 0, .80f));
    Box(Elements, L + 1, Geometry, At, Panel, Srgb(.045f, .037f, .055f, .99f));
    Box(Elements, L + 2, Geometry, At, FVector2D(Panel.X, 2.f), Srgb(.70f, .54f, .30f));
    float Y = At.Y + 16.f;
    Text(Elements, L + 3, Geometry, Mode == EMemoriaSaveBrowserMode::Save ? Loc(TEXT("SAVE — MANUAL SLOTS"), TEXT("저장 — 수동 슬롯"))
        : Loc(TEXT("LOAD SAVE"), TEXT("저장 불러오기")), FVector2D(At.X + Pad, Y), Title, Srgb(.94f, .78f, .49f));
    Y += 46.f * TextScale;
    if (bAskOverwrite)
    {
        const FString Question = Ko() ? FString::Printf(TEXT("슬롯 %d의 기존 저장을 덮어쓸까요?"), RowToSlot(Selected))
            : FString::Printf(TEXT("Replace the existing save in slot %d?"), RowToSlot(Selected));
        Text(Elements, L + 3, Geometry, Question, FVector2D(At.X + Pad, Y + 28), Body, Srgb(.90f, .84f, .73f));
        Text(Elements, L + 3, Geometry, Loc(TEXT("A valid previous save is retained as a backup."), TEXT("유효한 이전 저장은 백업으로 유지됩니다.")),
            FVector2D(At.X + Pad, Y + 60 * TextScale), Small, Srgb(.72f, .67f, .62f));
        Y += 120.f * TextScale;
        for (int32 Choice = 0; Choice < 2; ++Choice)
        {
            const FVector2D R(At.X + Pad, Y), RS(W, 52 * TextScale);
            Box(Elements, L + 2, Geometry, R, RS, Choice == OverwriteChoice ? Srgb(.24f, .17f, .15f) : Srgb(.10f, .08f, .12f));
            Text(Elements, L + 3, Geometry, Choice == 0 ? Loc(TEXT("Replace save"), TEXT("덮어쓰기")) : Loc(TEXT("Keep existing save"), TEXT("기존 저장 유지")),
                R + FVector2D(18, 12 * TextScale), Body, Choice == OverwriteChoice ? Srgb(.98f, .82f, .53f) : Srgb(.70f, .65f, .59f));
            Rows.Add(FBox2D(R, R + RS)); Y += RS.Y + 10;
        }
    }
    else
    {
        const float FooterH = 100.f * TextScale;
        const int32 Count = Summaries.Num();
        const float AvailableRows = float(At.Y + Panel.Y) - Y - FooterH;
        const float RowH = FMath::Max(64.f, FMath::Min(104.f * TextScale, AvailableRows / FMath::Max(1, Count) - 7.f));
        for (int32 Row = 0; Row < Count; ++Row)
        {
            const auto& Info = Summaries[Row]; const bool bEnabled = IsRowEnabled(Row), bSelected = Row == Selected;
            const FVector2D R(At.X + Pad, Y), RS(W, RowH);
            Box(Elements, L + 2, Geometry, R, RS, bSelected ? Srgb(.18f, .14f, .18f) : Srgb(.09f, .075f, .11f));
            if (bSelected) Box(Elements, L + 3, Geometry, R, FVector2D(3, RowH), Srgb(.94f, .71f, .38f));
            const auto Ink = bEnabled ? Srgb(.91f, .83f, .70f) : Srgb(.60f, .57f, .54f);
            FString Label = Info.Slot == 0 ? Loc(TEXT("AUTOSAVE — most recent"), TEXT("자동 저장 — 최근 기록"))
                : Ko() ? FString::Printf(TEXT("수동 슬롯 %d"), Info.Slot) : FString::Printf(TEXT("MANUAL SLOT %d"), Info.Slot);
            if (Info.CanLoad()) Label += Ko() ? FString::Printf(TEXT(" · 태운 기억 %d개"), Info.BurnedMemoryCount)
                : FString::Printf(TEXT(" · Burned: %d"), Info.BurnedMemoryCount);
            FittedText(Elements, L + 3, Geometry, Label, R + FVector2D(17, 7), Body, Ink, FVector2D(W - 34, RowH * .38 - 9));
            if (Info.CanLoad())
            {
                FString Location = Ko() ? FString::Printf(TEXT("%lld장 · %s"), Info.Chapter, *RegionLabel(Info.Map, true))
                    : FString::Printf(TEXT("Chapter %lld · %s"), Info.Chapter, *RegionLabel(Info.Map, false));
                const FString& Record = Ko() ? Info.RecordTitleKo : Info.RecordTitle;
                if (!Record.IsEmpty()) Location += Loc(TEXT(" · Record: "), TEXT(" · 기록: ")) + Record;
                FittedText(Elements, L + 3, Geometry, Location, R + FVector2D(17, RowH * .38), Small, Ink, FVector2D(W - 34, RowH * .31 - 2));
                const FString Stamp = Info.SavedAtUtc.ToString(TEXT("%Y-%m-%d %H:%M UTC"))
                    + (Info.State == EMemoriaSaveSlotState::Backup ? Loc(TEXT(" · backup recovery"), TEXT(" · 백업 복구")) : FString());
                FittedText(Elements, L + 3, Geometry, Stamp, R + FVector2D(17, RowH * .69), Small, Srgb(.69f, .64f, .57f), FVector2D(W - 34, RowH * .31 - 3));
            }
            else Text(Elements, L + 3, Geometry, StateLabel(Info.State), R + FVector2D(17, RowH * .55), Small, Ink);
            Rows.Add(FBox2D(R, R + RS)); Y += RowH + 7.f;
        }
        const FVector2D R(At.X + Pad, Y), RS(W, 36.f * TextScale);
        Box(Elements, L + 2, Geometry, R, RS, Selected == GetRowCount() - 1 ? Srgb(.18f, .14f, .18f) : Srgb(.09f, .075f, .11f));
        Text(Elements, L + 3, Geometry, Loc(TEXT("Back"), TEXT("뒤로")), R + FVector2D(17, 5), Body, Srgb(.88f, .77f, .62f));
        Rows.Add(FBox2D(R, R + RS));
    }
    Text(Elements, L + 3, Geometry, Notice, FVector2D(At.X + Pad, At.Y + Panel.Y - 58.f * TextScale), Small, Srgb(.94f, .78f, .49f));
    Text(Elements, L + 3, Geometry, Loc(TEXT("D-pad / arrows: choose    A / Enter: confirm    B / Esc: back"), TEXT("방향키: 선택    A / Enter: 확인    B / Esc: 뒤로")),
        FVector2D(At.X + Pad, At.Y + Panel.Y - 30.f * TextScale), Small, Srgb(.65f, .60f, .55f));
    return L + 4;
}
