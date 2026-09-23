#include "Shop/MemoriaShopSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Save/MemoriaCheckpointSubsystem.h"
#include "Framework/MemoriaCoordinates.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
namespace
{
#include "MemoriaShopSource.inl"
#include "MemoriaShopTransactionText.inl"
}
void UMemoriaShopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
    Collection.InitializeDependency<UMemoriaCheckpointSubsystem>();
    Run = GetGameInstance()->GetSubsystem<UMemoriaRunSubsystem>();
    Run->OnRunReplaced.AddUObject(this, &UMemoriaShopSubsystem::Reset);
    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMemoriaShopSubsystem::OnWorldCleanup);
}
void UMemoriaShopSubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldCleanup.RemoveAll(this);
    if (Run) Run->OnRunReplaced.RemoveAll(this);
    Reset(); Catalog = nullptr; Run = nullptr;
    Super::Deinitialize();
}
void UMemoriaShopSubsystem::Reset()
{
    ++Revision; bClosed = false; Mode = TEXT("sell"); Toasts.Reset();
    OwnerRun.Invalidate(); OwnerWorld.Reset(); bWorldBound = false;
    OpenCount = 0; Stock.Reset(); Requests.Reset();
}
void UMemoriaShopSubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
    if (bWorldBound && OwnerWorld.Get() == World) Reset();
}
bool UMemoriaShopSubsystem::IsOpen() const
{
    return !bClosed && Run && Run->HasActiveRun() && OwnerRun.IsValid() && Run->GetRunSnapshot().RunId == OwnerRun
        && (!bWorldBound || (OwnerWorld.IsValid() && !OwnerWorld->bIsTearingDown));
}
bool UMemoriaShopSubsystem::OpenMalet(UWorld* Owner)
{
    if (bBusy || !Run || !Run->HasActiveRun() || IsOpen() || (bClosed && OwnerRun == Run->GetRunSnapshot().RunId)) return false;
    if (Owner && (Owner->GetGameInstance() != GetGameInstance() || Owner->bIsTearingDown)) return false;
    Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    if (!Catalog) return false;
    Reset(); OwnerRun = Run->GetRunSnapshot().RunId; OwnerWorld = Owner; bWorldBound = Owner != nullptr;
    Stock = SourceMaletOffers(); OpenCount = 1;
    // Source order. These requests are recorded, not executed by profile/audio/tutorial handlers.
    Requests = {TEXT("request:audio:ui_open"), TEXT("request:achievement:check_grains"), TEXT("request:tutorial:first_shop")};
    return true;
}
FMemoriaShopView UMemoriaShopSubsystem::GetView() const
{
    FMemoriaShopView V;
    V.Revision = Revision; V.bClosed = bClosed;
    if (!IsOpen() && !bClosed) return V;
    const auto S = Run->GetRunSnapshot(); const bool Ko = S.CurrentLocale == TEXT("ko");
    V.bOpen = IsOpen(); V.Merchant = TEXT("Malet"); V.Mode = Mode; V.Grains = S.Player.Grains;
    if (!Toasts.IsEmpty()) V.Feedback = Toasts.Last().Text;
    if (Toasts.Num() > 1 && Toasts[Toasts.Num()-2].Text.Contains(Ko ? TEXT("\ub9f9\uc138") : TEXT("Oath")))
        V.Feedback = Toasts[Toasts.Num()-2].Text + TEXT("\n") + V.Feedback;
    V.Title = Ko ? SourceTitleKo : SourceTitleEn; V.Caption = Ko ? SourceCaptionKo : SourceCaptionEn;
    V.EmptyDetail = Ko ? SourceEmptyDetailKo : SourceEmptyDetailEn;
    V.PortraitSource = Ko ? SourcePortraitKo : SourcePortraitEn;
    V.GrainsText = FString::Printf(TEXT("%lld"), S.Player.Grains) + (Ko ? TEXT(" \uadf8\ub808\uc778") : TEXT(" Grains"));
    V.Stock = Stock; V.Requests = Requests;
    if (Mode == TEXT("buy"))
    {
        for (const auto& O : Stock) if (!O.bSold)
            V.Rows.Add({O.Id, O.Title, O.Description, O.StoryEffect, O.Grade, O.Price});
        V.bEmptyAvailable = V.Rows.IsEmpty(); return V;
    }
    const auto Available = Run->GetPlayerMemory()->GetAvailable(); V.bEmptyAvailable = Available.IsEmpty();
    for (const auto& Id : Available)
    {
        const auto* D = Run->GetPlayerMemory()->GetDefinitions().FindByPredicate([&](const auto& X){ return X.Id == Id; });
        if (!D || D->RawGrade == EMemoriaMemoryGrade::Grade1) continue;
        FMemoriaShopRow R; R.Id = Id; R.Title = D->Title; R.Description = D->Description; R.StoryEffect = D->StoryEffect;
        R.Grade = static_cast<int32>(D->RawGrade); R.Price = SourceSellPrices[R.Grade];
        if (const auto* L = Catalog->LocalizedText.FindByPredicate([&](const auto& X){return X.MemoryId == Id && X.Locale == S.CurrentLocale;}))
        { R.Title = L->Title; R.Description = L->Description; if(L->bHasStoryEffect) R.StoryEffect = L->StoryEffect; }
        V.Rows.Add(MoveTemp(R));
    }
    return V;
}

bool UMemoriaShopSubsystem::SetMode(const FString& InMode)
{
    if (!IsOpen() || bBusy || (InMode != TEXT("sell") && InMode != TEXT("buy"))) return false;
    if (Mode != InMode) { Mode = InMode; ++Revision; OnChanged.Broadcast(); }
    return true;
}
bool UMemoriaShopSubsystem::Transact(const FString& InMode, const FString& Id, uint64 ExpectedRevision)
{
    if (!IsOpen() || bBusy || ExpectedRevision != Revision || InMode != Mode
        || Run->GetPlayerMemory()->IsDispatchingEvent()) return false;
    const auto V = GetView();
    const auto* Found = V.Rows.FindByPredicate([&](const auto& R){return R.Id == Id;});
    if (!Found || Found->Price < 0) return false;
    const auto Row = *Found; // Never hold a pointer across a synchronous domain observer.
    const uint64 Token = Revision;
    TGuardValue<bool> Busy(bBusy, true);
    const bool Ko = Run->State.CurrentLocale == TEXT("ko");
    if (Mode == TEXT("sell"))
    {
        if (Run->State.Player.Grains > MAX_int64 - Row.Price) return false;
        if (Run->BurnMemory(Id) != EMemoriaMemoryResult::Success) return false;
        if (Revision != Token || !IsOpen()) return false;
        if (Run->State.GetFlag(TEXT("oath_ash_sworn")) && !Run->State.GetFlag(TEXT("oath_ash_broken")))
        {
            Run->SetStoryFlag(TEXT("oath_ash_broken"), true);
            Toasts.Add({Ko ? SourceAshBrokenKo : SourceAshBrokenEn, 2});
        }
        Run->State.Player.Grains += Row.Price;
        OnGrainsChanged.Broadcast(Run->State.Player.Grains);
        if (Revision != Token || !IsOpen()) return false;
        Requests.Add(TEXT("request:audio:confirm"));
        Toasts.Add({FString(Ko ? SourceSoldKo : SourceSoldEn).Replace(TEXT("%lld"), *LexToString(Row.Price)).Replace(TEXT("%s"), *Row.Title), 2});
    }
    else
    {
        if (Run->State.Player.Grains < Row.Price) return false;
        const auto* Offer = Stock.FindByPredicate([&](const auto& O){return O.Id == Id && !O.bSold;});
        if (!Offer || Run->GetPlayerMemory()->GetDefinitions().ContainsByPredicate([&](const auto& D){return D.Id == Id;})) return false;
        FMemoriaMemoryDefinition D; D.Id=Offer->Id; D.Title=Offer->Title; D.Description=Offer->Description;
        D.RawGrade=static_cast<EMemoriaMemoryGrade>(Offer->Grade); D.BurnPower=Offer->BurnPower;
        D.StoryEffect=Offer->StoryEffect; D.RelatedNpc=Offer->RelatedNpc;
        D.SourcePath=TEXT("scenes/maps/verdan_market.gd::_open_malet_shop"); D.TextId=D.Id;
        Run->State.Player.Grains -= Row.Price;
        OnGrainsChanged.Broadcast(Run->State.Player.Grains);
        if (Revision != Token || !IsOpen()) return false;
        if (Run->AcquireMemory(D) != EMemoriaMemoryResult::Success)
        { Run->State.Player.Grains += Row.Price; return false; }
        if (Revision != Token || !IsOpen()) return false;
        Stock.FindByPredicate([&](const auto& O){return O.Id == Id;})->bSold = true;
        Requests.Add(TEXT("request:audio:memory_add"));
        Requests.Add(TEXT("request:audio:confirm"));
        Toasts.Add({FString(SourceBought).Replace(TEXT("%lld"), *LexToString(Row.Price)).Replace(TEXT("%s"), *D.Title), 1});
    }
    Requests.Add(TEXT("request:achievement:check_grains"));
    ++Revision; OnChanged.Broadcast(); return true;
}
bool UMemoriaShopSubsystem::Close(uint64 ExpectedRevision)
{
    if (!IsOpen() || bBusy || Revision != ExpectedRevision || Run->GetPlayerMemory()->IsDispatchingEvent()) return false;
    TGuardValue<bool> Busy(bBusy, true);
    bClosed = true;
    Requests.Add(TEXT("request:audio:ui_close"));
    Run->SetStoryFlag(TEXT("ch2_complete"), true);
    Run->State.CurrentChapter = 3;
    Requests.Add(TEXT("request:autosave:chapter_transition"));
    // Source captures the current Verdan field after chapter/flag writes and
    // before achievement requests and the deferred next-map timer.
    FVector2D Position(500,340);
    if (auto* World=OwnerWorld.Get())
        if (auto* PC=World->GetFirstPlayerController())
            if (APawn* Pawn=PC->GetPawn()) Position=Memoria::Coordinates::ToSource(Pawn->GetActorLocation());
    GetGameInstance()->GetSubsystem<UMemoriaCheckpointSubsystem>()->SaveClosedBoundary(Position);
    Requests.Add(TEXT("request:achievement:chapter:2"));
    Requests.Add(TEXT("request:achievement:unlock:merchant"));
    Requests.Add(TEXT("deferred:chapter_transition_delay:1.5"));
    ++Revision; OnChanged.Broadcast(); return true;
}
