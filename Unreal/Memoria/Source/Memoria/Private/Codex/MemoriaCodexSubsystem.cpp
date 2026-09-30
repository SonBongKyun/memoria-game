#include "Codex/MemoriaCodexSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Presentation/MemoriaArchiveView.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
const TArray<TPair<FString, FString>>& UMemoriaCodexSubsystem::Roster()
{
    static const TArray<TPair<FString, FString>> Values = {
        {TEXT("Alley Rat"), TEXT("골목쥐")}, {TEXT("Ash Crawler"), TEXT("잿빛 크롤러")}, {TEXT("Ash Phantom"), TEXT("재의 환영")},
        {TEXT("Ash Walker"), TEXT("재의 방랑자")}, {TEXT("Belt Scavenger"), TEXT("벨트 약탈자")}, {TEXT("Cliff Stalker"), TEXT("절벽 추적자")},
        {TEXT("Coastal Void Beast"), TEXT("해안의 보이드 비스트")}, {TEXT("Colorless Wraith"), TEXT("무색의 망령")}, {TEXT("Depth Crawler"), TEXT("심연 크롤러")},
        {TEXT("Dust Crawler"), TEXT("먼지 크롤러")}, {TEXT("Forest Shade"), TEXT("숲의 그림자")}, {TEXT("Hollow"), TEXT("공허체")},
        {TEXT("Hollow Walker"), TEXT("빈껍데기 방랑자")}, {TEXT("Kairos, Authority Editor"), TEXT("카이로스 · 관리국 편집관")}, {TEXT("Market Thief"), TEXT("시장 도적")},
        {TEXT("Memory Eater"), TEXT("기억 포식자")}, {TEXT("Memory Leech"), TEXT("기억 거머리")}, {TEXT("Null Wisp"), TEXT("무의 도깨비불")},
        {TEXT("Remnant"), TEXT("잔존체")}, {TEXT("Root Shade"), TEXT("뿌리 그림자")}, {TEXT("Rubble Rat"), TEXT("잔해쥐")},
        {TEXT("Seam Lurker"), TEXT("심 틈새의 잠복자")}, {TEXT("Shade Sentinel"), TEXT("그림자 파수꾼")}, {TEXT("Shore Wraith"), TEXT("해안 망령")},
        {TEXT("Threshold Crawler"), TEXT("경계 크롤러")}, {TEXT("Threshold Shade"), TEXT("경계의 그림자")}, {TEXT("Void Beast"), TEXT("보이드 비스트")},
        {TEXT("Void Fragment"), TEXT("보이드 파편")}, {TEXT("Void Sentinel"), TEXT("보이드 파수꾼")}, {TEXT("Void Watcher"), TEXT("보이드 감시자")},
        {TEXT("Void Wisp"), TEXT("보이드 도깨비불")}, {TEXT("Void Wraith"), TEXT("보이드 망령")},
        // The field's stand-in void foe (S313) has no source name; its Korean follows the combat HUD.
        {TEXT("Void Husk"), TEXT("보이드 허스크")}};
    return Values;
}
FString UMemoriaCodexSubsystem::EnemyNameKo(const FString& Name)
{
    for (const auto& Pair : Roster()) if (Pair.Key == Name) return Pair.Value;
    return Name;
}
FString UMemoriaCodexSubsystem::Stars(int32 RawGrade)
{
    // The source's comment and colours rank Grade 1 (core) highest; its 5 - grade gave the reverse. The
    // comment's intent is kept: Grade 5 (raw 0) one star .. Grade 1 (raw 4) five.
    const int32 Count = FMath::Clamp(RawGrade + 1, 1, 5);
    FString Out; for (int32 I = 0; I < 5; ++I) Out += I < Count ? TEXT("★") : TEXT("☆");
    return Out;
}
FString UMemoriaCodexSubsystem::DefeatBadge(int32 Defeated)
{ return Defeated >= 50 ? TEXT(" ●") : Defeated >= 25 ? TEXT(" ○") : Defeated >= 10 ? TEXT(" ◦") : TEXT(""); }
FString UMemoriaCodexSubsystem::Path() const { return FPaths::ProjectSavedDir() / TEXT("Memoria/codex.json"); }
void UMemoriaCodexSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bPersist = !GIsAutomationTesting && !IsRunningCommandlet() && !FApp::IsUnattended();
    FString Text; TSharedPtr<FJsonObject> O;
    if (!bPersist || !FFileHelper::LoadFileToString(Text, *Path()) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), O) || !O) return;
    const TArray<TSharedPtr<FJsonValue>>* List;
    if (O->TryGetArrayField(TEXT("enemies"), List))
        for (const auto& V : *List)
        {
            const auto E = V->AsObject(); FMemoriaCodexEnemy R;
            if (!E || !E->TryGetStringField(TEXT("name"), R.Name)) continue;
            E->TryGetNumberField(TEXT("encounters"), R.Encounters); E->TryGetNumberField(TEXT("defeated"), R.Defeated);
            E->TryGetBoolField(TEXT("is_void"), R.bVoid); E->TryGetBoolField(TEXT("is_boss"), R.bBoss);
            E->TryGetNumberField(TEXT("max_hp"), R.MaxHp); E->TryGetNumberField(TEXT("atk"), R.Atk);
            Enemies.Add(R);
        }
    if (O->TryGetArrayField(TEXT("memories"), List))
        for (const auto& V : *List)
        {
            const auto M = V->AsObject(); FMemoriaCodexMemory R;
            if (!M || !M->TryGetStringField(TEXT("id"), R.Id)) continue;
            M->TryGetStringField(TEXT("title"), R.Title); M->TryGetStringField(TEXT("desc"), R.Desc);
            M->TryGetNumberField(TEXT("grade"), R.Grade); M->TryGetBoolField(TEXT("burned"), R.bBurned);
            Memories.Add(R);
        }
}
void UMemoriaCodexSubsystem::Save() const
{
    if (!bPersist) return;
    auto O = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> E, M;
    for (const auto& R : Enemies)
    {
        auto J = MakeShared<FJsonObject>(); J->SetStringField(TEXT("name"), R.Name);
        J->SetNumberField(TEXT("encounters"), R.Encounters); J->SetNumberField(TEXT("defeated"), R.Defeated);
        J->SetBoolField(TEXT("is_void"), R.bVoid); J->SetBoolField(TEXT("is_boss"), R.bBoss);
        J->SetNumberField(TEXT("max_hp"), R.MaxHp); J->SetNumberField(TEXT("atk"), R.Atk);
        E.Add(MakeShared<FJsonValueObject>(J));
    }
    for (const auto& R : Memories)
    {
        auto J = MakeShared<FJsonObject>(); J->SetStringField(TEXT("id"), R.Id); J->SetStringField(TEXT("title"), R.Title);
        J->SetStringField(TEXT("desc"), R.Desc); J->SetNumberField(TEXT("grade"), R.Grade); J->SetBoolField(TEXT("burned"), R.bBurned);
        M.Add(MakeShared<FJsonValueObject>(J));
    }
    O->SetArrayField(TEXT("enemies"), E); O->SetArrayField(TEXT("memories"), M);
    FString Text; FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text, *Path());
}
const FMemoriaCodexEnemy* UMemoriaCodexSubsystem::FindEnemy(const FString& Name) const
{ return Enemies.FindByPredicate([&](const FMemoriaCodexEnemy& E) { return E.Name == Name; }); }
void UMemoriaCodexSubsystem::RecordEncounter(const FString& Name, bool bVoid, bool bBoss, int32 MaxHp, int32 Atk)
{
    auto* E = Enemies.FindByPredicate([&](const FMemoriaCodexEnemy& R) { return R.Name == Name; });
    if (!E) { FMemoriaCodexEnemy R; R.Name = Name; R.bVoid = bVoid; R.bBoss = bBoss; R.MaxHp = MaxHp; R.Atk = Atk; E = &Enemies.Add_GetRef(R); }
    ++E->Encounters; Save();
}
void UMemoriaCodexSubsystem::RecordDefeat(const FString& Name)
{
    if (auto* E = Enemies.FindByPredicate([&](const FMemoriaCodexEnemy& R) { return R.Name == Name; })) { ++E->Defeated; Save(); }
}
void UMemoriaCodexSubsystem::Observe(const UMemoriaRunSubsystem& Run)
{
    if (!Run.HasActiveRun() || !Run.GetPlayerMemory()) return;
    // The archive is rebuilt only when the run's memories change: a new run, a memory gained, a burn.
    const auto Snapshot = Run.GetPlayerMemory()->GetSnapshot();
    const FGuid RunId = Run.GetRunSnapshot().RunId;
    if (RunId == SeenRun && Snapshot.Owned.Num() == SeenOwned && Snapshot.BurnedHistory.Num() == SeenBurned) return;
    SeenRun = RunId; SeenOwned = Snapshot.Owned.Num(); SeenBurned = Snapshot.BurnedHistory.Num();
    bool bChanged = false;
    for (const auto& Row : MemoriaArchive::Build(Run).Rows)
    {
        auto* M = Memories.FindByPredicate([&](const FMemoriaCodexMemory& R) { return R.Id == Row.Id; });
        if (!M) { FMemoriaCodexMemory R; R.Id = Row.Id; R.Title = Row.Title; R.Desc = Row.Description; R.Grade = Row.Grade; M = &Memories.Add_GetRef(R); bChanged = true; }
        if (Row.bBurned && !M->bBurned) { M->bBurned = true; bChanged = true; }
    }
    if (bChanged) Save();
}
TArray<FString> UMemoriaCodexSubsystem::Unmet() const
{
    TArray<FString> Out;
    for (const auto& Pair : Roster()) if (!FindEnemy(Pair.Key) && Pair.Key != TEXT("Void Husk")) Out.Add(Pair.Key);
    return Out;
}
int32 UMemoriaCodexSubsystem::KnownTotal() const
{
    int32 Total = Unmet().Num();
    return Total + Enemies.Num();
}
