#include "Shop/MemoriaShopSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
namespace
{
#include "MemoriaShopSource.inl"
}
void UMemoriaShopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMemoriaRunSubsystem>();
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
    OwnerRun.Invalidate(); OwnerWorld.Reset(); bWorldBound = false;
    OpenCount = 0; Stock.Reset(); Requests.Reset();
}
void UMemoriaShopSubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
    if (bWorldBound && OwnerWorld.Get() == World) Reset();
}
bool UMemoriaShopSubsystem::IsOpen() const
{
    return Run && Run->HasActiveRun() && OwnerRun.IsValid() && Run->GetRunSnapshot().RunId == OwnerRun
        && (!bWorldBound || OwnerWorld.IsValid());
}
bool UMemoriaShopSubsystem::OpenMalet(UWorld* Owner)
{
    if (!Run || !Run->HasActiveRun() || IsOpen()) return false;
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
    if (!IsOpen()) return V;
    const auto S = Run->GetRunSnapshot(); const bool Ko = S.CurrentLocale == TEXT("ko");
    V.bOpen = true; V.Merchant = TEXT("Malet"); V.Mode = TEXT("sell");
    V.Title = Ko ? SourceTitleKo : SourceTitleEn; V.Caption = Ko ? SourceCaptionKo : SourceCaptionEn;
    V.EmptyDetail = Ko ? SourceEmptyDetailKo : SourceEmptyDetailEn;
    V.PortraitSource = Ko ? SourcePortraitKo : SourcePortraitEn;
    V.GrainsText = FString::Printf(TEXT("%lld"), S.Player.Grains) + (Ko ? TEXT(" \uadf8\ub808\uc778") : TEXT(" Grains"));
    V.Stock = Stock; V.Requests = Requests;
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
