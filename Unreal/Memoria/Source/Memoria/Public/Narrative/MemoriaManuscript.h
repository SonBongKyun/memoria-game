#pragma once
#include "CoreMinimal.h"
#include "Journal/MemoriaJournal.h"

struct FMemoriaRunSnapshot;
class UMemoriaFieldAsset;
class UMemoriaRunSubsystem;

// S370: the final manuscript (Notion "MEMORIA | 최종본 (KR)", chapters 1-5, final-1) as the Korean the player reads.
// The IR and its English stay as imported; this layer only changes what is drawn in Korean.
namespace MemoriaManuscript
{
    // Korean display text in the manuscript's words: a whole line the manuscript rewrote, then its fixed
    // names and units (말레트, 셈터, 공백 7호, 아헨 숲, 마일). Idempotent; text without a match passes through.
    MEMORIA_API FString Canon(const FString& Korean);
    MEMORIA_API int32 LineCount();

    // The main quest, "공백 7호를 찾아서": one step per manuscript beat the port plays, done by the run's flags.
    struct FMainStep { const TCHAR* Flag; const TCHAR* En; const TCHAR* Ko; };
    MEMORIA_API const TArray<FMainStep>& MainSteps();
    MEMORIA_API int32 MainStepIndex(const FMemoriaRunSnapshot& Run);   // the first step not yet done
    MEMORIA_API FMemoriaJournalRecord MainRecord(const FMemoriaRunSnapshot& Run, bool bKo);

    // Chapter 4's signposts: "그 뒤로 아렐은 표지판을 그냥 지나치지 않았다." Three on the east road of the
    // drift map, open once Elia has anchored his reading. Burning after the anchoring loosens it again.
    MEMORIA_API int32 SignCount();
    MEMORIA_API FString SignGroup(int32 Index);              // ms_drift_sign_<n>; also the flag that it was read
    MEMORIA_API int32 SignIndex(const FString& Group);       // INDEX_NONE when the group is not a sign
    MEMORIA_API FVector2D SignOrigin(int32 Index);           // tile centre on drift_shelter, source pixels
    MEMORIA_API FString SignPrompt(int32 Index);
    MEMORIA_API bool SignsOpen(const FMemoriaRunSnapshot& Run);
    MEMORIA_API int32 SignsRead(const FMemoriaRunSnapshot& Run);
    inline const TCHAR* const SignsMap = TEXT("drift_shelter");
    inline const TCHAR* const SignsDoneFlag = TEXT("ms_drift_signs_done");
    inline const TCHAR* const AnchorFlag = TEXT("ch4_anchoring");
    inline constexpr int64 SignsRewardGrains = 8;
    // How many memories were burned when the anchoring held (INDEX_NONE until it is recorded).
    MEMORIA_API int32 AnchorBaseline(const FMemoriaRunSnapshot& Run);
    MEMORIA_API void EnsureAnchorBaseline(UMemoriaRunSubsystem& Run);
    // "네가 얼마나 자주 태우느냐에 달렸어." Any burn since the anchoring loosens the letters again.
    MEMORIA_API bool Scrambled(const FMemoriaRunSnapshot& Run, int32 BurnedNow);
    MEMORIA_API UMemoriaFieldAsset* BuildSign(UObject* Outer, int32 Index, bool bScrambled);
    MEMORIA_API FMemoriaJournalRecord SignsRecord(const FMemoriaRunSnapshot& Run, bool bKo);

    // S371: Chapter 6, the Seam (최종본 6화 「더 심」, 7화 「세이블」). Its scenes are built at runtime (ms_seam_*).
    inline const TCHAR* const SeamMap = TEXT("the_seam");
    // S372: Chapter 7, the listening forest (최종본 8-10화), on the Godot forgotten_forest ground.
    inline const TCHAR* const ForestMap = TEXT("forgotten_forest");
    // S373: Chapter 8, the Crumbling Coast and Blank 7 (최종본 11-12화), on the Godot crumbling_coast ground.
    inline const TCHAR* const CoastMap = TEXT("crumbling_coast");
    // S374: Chapter 9, Haren's clearing (최종본 13-14화), on the Godot rim_forest ground.
    inline const TCHAR* const ClearingMap = TEXT("haren_clearing");
    // S375: Chapter 10, the Seam, leaving (최종본 15-16화), on the Seam's own ground again.
    inline const TCHAR* const ReturnMap = TEXT("seam_return");
    // S377: Chapter 11, the roofless farmhouses (최종본 17-18화), on the Godot colorless_waste plain.
    inline const TCHAR* const FarmsMap = TEXT("roofless_farms");
    // S379: Chapter 12, the basin waystation (최종본 19-20화), on the Godot belt_waystation yard.
    inline const TCHAR* const BasinMap = TEXT("basin_station");
    // S382: Chapter 13, Verdan again (최종본 21화), on the Godot verdan_market ground.
    inline const TCHAR* const VerdanReturnMap = TEXT("verdan_return");
    inline const TCHAR* const EchoShellMemory = TEXT("rel_echo_shell");
    // Sable hears how much he has burned: from this many burns her "몇 달" is two, not three.
    inline constexpr int32 SableHeavyBurns = 4;
    MEMORIA_API bool IsScene(const FString& Group);
    MEMORIA_API TArray<FString> SceneGroups();
    MEMORIA_API UMemoriaFieldAsset* BuildScene(UObject* Outer, const FString& Group, int32 Burned);
    // The road a closed chapter's exit takes once the story has moved on (Drift -> the Seam after Chapter 5).
    MEMORIA_API FString RoadOnward(const FString& Map, const FMemoriaRunSnapshot& Run);
}
