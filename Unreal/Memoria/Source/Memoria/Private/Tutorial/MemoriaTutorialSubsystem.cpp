#include "Tutorial/MemoriaTutorialSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
namespace
{
const TCHAR* Section = TEXT("/Script/Memoria.MemoriaTutorial");
struct FHint { const TCHAR* Id; const TCHAR* En; const TCHAR* Ko; };
// HINTS / HINTS_KO. first_burn and first_shop are the source's words; first_battle and first_status_effect
// are rewritten for the action field (its controls, in the combat HUD's words; statuses that fade with time).
const FHint Hints[] = {
    {TEXT("first_battle"),
     TEXT("Left click strikes; hold it for a spinning slash. Right click guards, and a guard as the blow lands parries. Shift dodges. R burns a memory for power you will not get back."),
     TEXT("좌클릭으로 베고, 길게 누르면 회전베기입니다. 우클릭은 막기이며, 맞기 직전에 막으면 받아칩니다. Shift는 회피, R은 돌아오지 않는 기억을 태워 힘을 얻습니다.")},
    {TEXT("first_burn"),
     TEXT("That memory is gone for good. The world and the people in it will adjust around the hole it left."),
     TEXT("그 기억은 영영 사라집니다. 세계와 사람들은 그 빈자리에 맞춰 다시 정렬됩니다.")},
    {TEXT("first_status_effect"),
     TEXT("Status effects wear off with time. Use an Antidote to cure poison."),
     TEXT("상태 이상은 시간이 지나면 사라집니다. 독은 해독제로 치료할 수 있습니다.")},
    {TEXT("first_shop"),
     TEXT("Trade Grains for memories and items. Sell what you don't need."),
     TEXT("그레인으로 기억과 아이템을 거래할 수 있습니다. 필요 없는 물품은 판매하세요.")},
};
}
void UMemoriaTutorialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Tests and commandlets never read or write the player's profile.
    bPersist = !GIsAutomationTesting && !IsRunningCommandlet() && !FApp::IsUnattended();
    if (!bPersist || !GConfig) return;
    FString Saved; GConfig->GetString(Section, TEXT("ShownHints"), Saved, GGameUserSettingsIni);
    TArray<FString> Ids; Saved.ParseIntoArray(Ids, TEXT(","), true);
    for (const FString& Id : Ids) if (HintIds().Contains(Id)) Shown.AddUnique(Id);
}
const TArray<FString>& UMemoriaTutorialSubsystem::HintIds()
{
    static const TArray<FString> Ids = [] { TArray<FString> Out; for (const auto& H : Hints) Out.Add(H.Id); return Out; }();
    return Ids;
}
FString UMemoriaTutorialSubsystem::Text(const FString& Id, bool bKo)
{
    for (const auto& H : Hints) if (Id == H.Id) return bKo ? H.Ko : H.En;
    return FString();
}
bool UMemoriaTutorialSubsystem::ShowHint(const FString& Id)
{
    if (Shown.Contains(Id) || !HintIds().Contains(Id)) return false;
    Shown.Add(Id); Save();
    Current = Id; Age = 0.f; OutAge = -1.f;
    return true;
}
void UMemoriaTutorialSubsystem::Dismiss()
{
    if (IsShowing() && OutAge < 0.f) OutAge = 0.f;
}
void UMemoriaTutorialSubsystem::Advance(float DeltaSeconds)
{
    if (!IsShowing()) return;
    Age += DeltaSeconds;
    if (OutAge < 0.f && Age >= HoldSeconds) OutAge = 0.f;
    if (OutAge >= 0.f && (OutAge += DeltaSeconds) >= OutSeconds) { Current.Reset(); OutAge = -1.f; }
}
void UMemoriaTutorialSubsystem::Save() const
{
    if (!bPersist || !GConfig) return;
    GConfig->SetString(Section, TEXT("ShownHints"), *FString::Join(Shown, TEXT(",")), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
