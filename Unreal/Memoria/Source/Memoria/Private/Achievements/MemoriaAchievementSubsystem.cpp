#include "Achievements/MemoriaAchievementSubsystem.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Audio/MemoriaAudioSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace
{
// ACHIEVEMENTS, in the source's order.
const FMemoriaAchievement Table[] = {
    {TEXT("first_blood"), TEXT("First Blood"), TEXT("Win your first battle."), TEXT("sword")},
    {TEXT("void_slayer"), TEXT("Void Slayer"), TEXT("Defeat a Void Beast."), TEXT("skull")},
    {TEXT("boss_hunter"), TEXT("Boss Hunter"), TEXT("Defeat a boss enemy."), TEXT("crown")},
    {TEXT("battle_veteran"), TEXT("Battle Veteran"), TEXT("Win 10 battles."), TEXT("shield")},
    {TEXT("survivor"), TEXT("Survivor"), TEXT("Win a battle with 10 HP or less."), TEXT("heart")},
    {TEXT("item_master"), TEXT("Item Master"), TEXT("Use 10 items in battle."), TEXT("potion")},
    {TEXT("perfect_tactics"), TEXT("Field Tactician"), TEXT("Complete a tactical objective in battle."), TEXT("shield")},
    {TEXT("resonance_master"), TEXT("Overbright"), TEXT("Reach high combat resonance in a battle."), TEXT("flame")},
    {TEXT("first_burn"), TEXT("First Burn"), TEXT("Burn your first memory."), TEXT("flame")},
    {TEXT("pyromaniac"), TEXT("Pyromaniac"), TEXT("Burn 5 memories."), TEXT("flame")},
    {TEXT("identity_crisis"), TEXT("Identity Crisis"), TEXT("Burn a Grade 2 (Identity) memory."), TEXT("flame")},
    {TEXT("zero_burn"), TEXT("Zero Burn"), TEXT("Burn the Core memory, your name."), TEXT("skull")},
    {TEXT("hidden_stump"), TEXT("Old Growth"), TEXT("Find the hidden stump in Rim Forest."), TEXT("eye")},
    {TEXT("hidden_garden"), TEXT("Secret Garden"), TEXT("Find the hidden garden in The Seam."), TEXT("eye")},
    {TEXT("explorer"), TEXT("Explorer"), TEXT("Visit all 5 maps."), TEXT("map")},
    {TEXT("chapter_complete_1"), TEXT("Rim Forest"), TEXT("Complete Chapter 1."), TEXT("book")},
    {TEXT("chapter_complete_2"), TEXT("Verdan Market"), TEXT("Complete Chapter 2."), TEXT("book")},
    {TEXT("chapter_complete_3"), TEXT("Weight of Pages"), TEXT("Complete Chapter 3."), TEXT("book")},
    {TEXT("chapter_complete_4"), TEXT("Drift"), TEXT("Complete Chapter 4."), TEXT("book")},
    {TEXT("chapter_complete_5"), TEXT("The Classifier"), TEXT("Complete Chapter 5."), TEXT("book")},
    {TEXT("chapter_complete_6"), TEXT("Thread That Holds"), TEXT("Complete Chapter 6."), TEXT("book")},
    {TEXT("chapter_complete_7"), TEXT("The Threshold"), TEXT("Complete Chapter 7."), TEXT("book")},
    {TEXT("chapter_complete_8"), TEXT("Forest That Forgets"), TEXT("Complete Chapter 8."), TEXT("book")},
    {TEXT("chapter_complete_9"), TEXT("Where Colors Stop"), TEXT("Complete Chapter 9."), TEXT("book")},
    {TEXT("chapter_complete_10"), TEXT("Into the Void"), TEXT("Complete Chapter 10."), TEXT("book")},
    {TEXT("ending_seal"), TEXT("The Seal Holds"), TEXT("Reach the Seal ending."), TEXT("star")},
    {TEXT("ending_zero"), TEXT("Nothing Remains"), TEXT("Reach the Zero Burn ending."), TEXT("star")},
    {TEXT("ending_ash"), TEXT("Ash Ending"), TEXT("Reach the Ash ending."), TEXT("star")},
    {TEXT("ending_seam"), TEXT("The Seam Holds"), TEXT("Reach the Seam ending."), TEXT("star")},
    {TEXT("ending_preservation"), TEXT("Preservation"), TEXT("Keep your name and continue the search."), TEXT("star")},
    {TEXT("ending_tobias"), TEXT("The Record Remains"), TEXT("Help Tobias carry the record beyond Authority control."), TEXT("star")},
    {TEXT("ending_hollow"), TEXT("Hollow"), TEXT("Reach the Hollow ending."), TEXT("star")},
    {TEXT("ending_weave"), TEXT("The Weave"), TEXT("Reach the Weave ending, seal BL-07 without burning your name."), TEXT("crown")},
    {TEXT("all_endings"), TEXT("Every Path"), TEXT("See all 7 endings."), TEXT("crown")},
    {TEXT("merchant"), TEXT("Merchant"), TEXT("Complete a trade with Malet."), TEXT("coin")},
    {TEXT("wealthy"), TEXT("Wealthy"), TEXT("Accumulate 100 Grains."), TEXT("coin")},
    {TEXT("all_quests"), TEXT("Memory Hunter"), TEXT("Complete all side quests."), TEXT("star")},
    {TEXT("new_game_plus"), TEXT("New Game+"), TEXT("Start a New Game+ run."), TEXT("cycle")},
};
}
const TArray<FMemoriaAchievement>& UMemoriaAchievementSubsystem::All()
{
    static const TArray<FMemoriaAchievement> Values(Table, UE_ARRAY_COUNT(Table));
    return Values;
}
const FMemoriaAchievement* UMemoriaAchievementSubsystem::Find(const FString& Id)
{
    return All().FindByPredicate([&](const FMemoriaAchievement& A) { return Id.Equals(A.Id, ESearchCase::CaseSensitive); });
}
FString UMemoriaAchievementSubsystem::Path() const { return FPaths::ProjectSavedDir() / TEXT("Memoria/achievements.json"); }
void UMemoriaAchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Tests and commandlets never read or write the player's achievements.
    bPersist = !GIsAutomationTesting && !IsRunningCommandlet() && !FApp::IsUnattended();
    FString Text;
    if (!bPersist || !FFileHelper::LoadFileToString(Text, *Path())) return;
    TSharedPtr<FJsonObject> O;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), O) || !O) return;
    if (const TSharedPtr<FJsonObject>* U; O->TryGetObjectField(TEXT("unlocked"), U))
        for (const auto& A : All()) if ((*U)->HasField(A.Id)) Unlocked.Add(A.Id);
    if (const TSharedPtr<FJsonObject>* S; O->TryGetObjectField(TEXT("stats"), S))
    {
        (*S)->TryGetNumberField(TEXT("battles_won"), BattlesWon); (*S)->TryGetNumberField(TEXT("items_used"), ItemsUsed);
        (*S)->TryGetStringArrayField(TEXT("maps_visited"), MapsVisited);
    }
}
void UMemoriaAchievementSubsystem::Save() const
{
    if (!bPersist) return;
    auto O = MakeShared<FJsonObject>(), U = MakeShared<FJsonObject>(), S = MakeShared<FJsonObject>();
    for (const FString& Id : Unlocked) U->SetBoolField(Id, true);
    S->SetNumberField(TEXT("battles_won"), BattlesWon); S->SetNumberField(TEXT("items_used"), ItemsUsed);
    TArray<TSharedPtr<FJsonValue>> Maps; for (const FString& M : MapsVisited) Maps.Add(MakeShared<FJsonValueString>(M));
    S->SetArrayField(TEXT("maps_visited"), Maps);
    O->SetObjectField(TEXT("unlocked"), U); O->SetObjectField(TEXT("stats"), S);
    FString Text; FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text, *Path());
}
bool UMemoriaAchievementSubsystem::Unlock(const FString& Id)
{
    if (Unlocked.Contains(Id) || !Find(Id)) return false;
    Unlocked.Add(Id); Queue.Add(Id); Save();
    UE_LOG(LogTemp, Display, TEXT("MEMORIA_ACHIEVEMENT unlocked:%s"), *Id);
    // _check_all_endings.
    if (Id.StartsWith(TEXT("ending_")))
    {
        bool bAll = true;
        for (const TCHAR* E : {TEXT("ending_zero"), TEXT("ending_preservation"), TEXT("ending_ash"), TEXT("ending_seam"), TEXT("ending_tobias"), TEXT("ending_hollow"), TEXT("ending_weave")})
            bAll &= Unlocked.Contains(E);
        if (bAll) Unlock(TEXT("all_endings"));
    }
    return true;
}
void UMemoriaAchievementSubsystem::RecordBattleWon(int64 HpAtEnd)
{
    ++BattlesWon; Unlock(TEXT("first_blood"));
    if (BattlesWon >= 10) Unlock(TEXT("battle_veteran"));
    if (HpAtEnd <= 10) Unlock(TEXT("survivor"));
    Save();
}
void UMemoriaAchievementSubsystem::RecordChapterComplete(int32 Chapter)
{
    Unlock(FString::Printf(TEXT("chapter_complete_%d"), Chapter));
}
void UMemoriaAchievementSubsystem::RecordMapVisit(const FString& Map)
{
    if (!MapsVisited.Contains(Map)) { MapsVisited.Add(Map); Save(); }
    if (MapsVisited.Num() >= 5) Unlock(TEXT("explorer"));
}
void UMemoriaAchievementSubsystem::RecordItemUsed()
{
    ++ItemsUsed;
    if (ItemsUsed >= 10) Unlock(TEXT("item_master"));
    Save();
}
void UMemoriaAchievementSubsystem::Observe(const UMemoriaRunSubsystem& Run)
{
    if (!Run.HasActiveRun()) return;
    const auto& Snapshot = Run.GetRunSnapshot();
    const auto* Memory = Run.GetPlayerMemory();
    if (Memory)
    {
        // _on_memory_burned for each burn this run has made since the last look; a new run starts over.
        const auto& Burned = Memory->GetSnapshot().BurnedHistory;
        if (Snapshot.RunId != SeenRun) { SeenRun = Snapshot.RunId; SeenBurns = 0; }
        for (int32 I = SeenBurns; I < Burned.Num(); ++I)
        {
            Unlock(TEXT("first_burn"));
            if (I + 1 >= 5) Unlock(TEXT("pyromaniac"));
            const auto* Def = Memory->GetDefinitions().FindByPredicate([&](const FMemoriaMemoryDefinition& D) { return D.Id == Burned[I]; });
            if (Def && Def->RawGrade == EMemoriaMemoryGrade::Grade2) Unlock(TEXT("identity_crisis"));
            if (Burned[I] == TEXT("core_name_origin")) Unlock(TEXT("zero_burn"));
        }
        SeenBurns = FMath::Max(SeenBurns, Burned.Num());
    }
    // check_grains; Malet's trade (verdan_market.gd); the chapters the maps close with a chN_complete flag.
    if (Snapshot.Player.Grains >= 100) Unlock(TEXT("wealthy"));
    if (Snapshot.GetFlag(TEXT("ch2_malet_done"))) Unlock(TEXT("merchant"));
    for (int32 Chapter = 2; Chapter <= 4; ++Chapter)
        if (Snapshot.GetFlag(FString::Printf(TEXT("ch%d_complete"), Chapter))) RecordChapterComplete(Chapter);
}
void UMemoriaAchievementSubsystem::Advance(float DeltaSeconds)
{
    if (Popup.IsEmpty())
    {
        if (Queue.IsEmpty()) return;
        Popup = Queue[0]; Queue.RemoveAt(0); PopupAge = 0.f;
        if (auto* Audio = GetGameInstance()->GetSubsystem<UMemoriaAudioSubsystem>()) Audio->PlaySfx(TEXT("memory_add"));
        return;
    }
    PopupAge += DeltaSeconds;
    if (PopupAge >= PopupIn + PopupHold + PopupOut) Popup.Reset();
}
void UMemoriaAchievementSubsystem::ResetForTest()
{
    Unlocked.Reset(); Queue.Reset(); MapsVisited.Reset(); Popup.Reset(); PopupAge = 0.f;
    BattlesWon = ItemsUsed = 0; SeenRun.Invalidate(); SeenBurns = 0;
}
