#include "Localization/MemoriaKorean.h"
#include "Narrative/MemoriaManuscript.h"
#include "Chapter/MemoriaChapterMap.h"
#include "Domain/MemoriaChapterMemories.h"
#include "Domain/MemoriaMemoryTypes.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Settings/MemoriaSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
namespace
{
const TMap<FString, FString>& Table()
{
    static const TMap<FString, FString> Values = {
        // Achievements (achievement_manager.gd ACHIEVEMENTS): titles, then what they ask.
        {TEXT("ACHIEVEMENT UNLOCKED"), TEXT("업적 달성")},
        {TEXT("First Blood"), TEXT("첫 승리")}, {TEXT("Win your first battle."), TEXT("첫 전투에서 승리한다.")},
        {TEXT("Void Slayer"), TEXT("보이드 사냥꾼")}, {TEXT("Defeat a Void Beast."), TEXT("보이드 짐승을 쓰러뜨린다.")},
        {TEXT("Boss Hunter"), TEXT("강적 토벌")}, {TEXT("Defeat a boss enemy."), TEXT("보스를 쓰러뜨린다.")},
        {TEXT("Battle Veteran"), TEXT("역전의 용사")}, {TEXT("Win 10 battles."), TEXT("전투에서 10번 승리한다.")},
        {TEXT("Survivor"), TEXT("생존자")}, {TEXT("Win a battle with 10 HP or less."), TEXT("HP 10 이하로 전투에서 승리한다.")},
        {TEXT("Item Master"), TEXT("도구의 달인")}, {TEXT("Use 10 items in battle."), TEXT("전투 중 아이템을 10번 사용한다.")},
        {TEXT("Field Tactician"), TEXT("현장 전술가")}, {TEXT("Complete a tactical objective in battle."), TEXT("전투 중 전술 목표를 달성한다.")},
        {TEXT("Overbright"), TEXT("과잉 공명")}, {TEXT("Reach high combat resonance in a battle."), TEXT("한 전투에서 높은 전투 공명에 도달한다.")},
        {TEXT("First Burn"), TEXT("첫 연소")}, {TEXT("Burn your first memory."), TEXT("처음으로 기억을 태운다.")},
        {TEXT("Pyromaniac"), TEXT("불을 탐하는 자")}, {TEXT("Burn 5 memories."), TEXT("기억 5개를 태운다.")},
        {TEXT("Identity Crisis"), TEXT("정체성의 위기")}, {TEXT("Burn a Grade 2 (Identity) memory."), TEXT("2등급(정체성) 기억을 태운다.")},
        {TEXT("Zero Burn"), TEXT("제로 번")}, {TEXT("Burn the Core memory, your name."), TEXT("핵심 기억, 너의 이름을 태운다.")},
        {TEXT("Old Growth"), TEXT("오래된 숲")}, {TEXT("Find the hidden stump in Rim Forest."), TEXT("림 외곽 숲의 숨겨진 그루터기를 찾는다.")},
        {TEXT("Secret Garden"), TEXT("비밀의 정원")}, {TEXT("Find the hidden garden in The Seam."), TEXT("더 심의 숨겨진 정원을 찾는다.")},
        {TEXT("Explorer"), TEXT("탐험가")}, {TEXT("Visit all 5 maps."), TEXT("다섯 지역을 모두 방문한다.")},
        {TEXT("Rim Forest"), TEXT("림 외곽 숲")}, {TEXT("Complete Chapter 1."), TEXT("1장을 마친다.")},
        {TEXT("Verdan Market"), TEXT("베르단 시장")}, {TEXT("Complete Chapter 2."), TEXT("2장을 마친다.")},
        {TEXT("Weight of Pages"), TEXT("페이지의 무게")}, {TEXT("Complete Chapter 3."), TEXT("3장을 마친다.")},
        {TEXT("Drift"), TEXT("드리프트")}, {TEXT("Complete Chapter 4."), TEXT("4장을 마친다.")},
        {TEXT("The Classifier"), TEXT("분류자")}, {TEXT("Complete Chapter 5."), TEXT("5장을 마친다.")},
        {TEXT("Thread That Holds"), TEXT("이어진 실")}, {TEXT("Complete Chapter 6."), TEXT("6장을 마친다.")},
        {TEXT("The Threshold"), TEXT("문턱")}, {TEXT("Complete Chapter 7."), TEXT("7장을 마친다.")},
        {TEXT("Forest That Forgets"), TEXT("잊어버리는 숲")}, {TEXT("Complete Chapter 8."), TEXT("8장을 마친다.")},
        {TEXT("Where Colors Stop"), TEXT("색이 멎는 곳")}, {TEXT("Complete Chapter 9."), TEXT("9장을 마친다.")},
        {TEXT("Into the Void"), TEXT("보이드 속으로")}, {TEXT("Complete Chapter 10."), TEXT("10장을 마친다.")},
        {TEXT("The Seal Holds"), TEXT("봉인은 유지된다")}, {TEXT("Reach the Seal ending."), TEXT("봉인 엔딩에 도달한다.")},
        {TEXT("Nothing Remains"), TEXT("아무것도 남지 않는다")}, {TEXT("Reach the Zero Burn ending."), TEXT("제로 번 엔딩에 도달한다.")},
        {TEXT("Ash Ending"), TEXT("재의 결말")}, {TEXT("Reach the Ash ending."), TEXT("재 엔딩에 도달한다.")},
        {TEXT("The Seam Holds"), TEXT("심은 버틴다")}, {TEXT("Reach the Seam ending."), TEXT("심 엔딩에 도달한다.")},
        {TEXT("Preservation"), TEXT("보존")}, {TEXT("Keep your name and continue the search."), TEXT("이름을 지킨 채 탐색을 이어 간다.")},
        {TEXT("The Record Remains"), TEXT("기록은 남는다")}, {TEXT("Help Tobias carry the record beyond Authority control."), TEXT("토비아스가 기록을 관리국의 손이 닿지 않는 곳으로 옮기도록 돕는다.")},
        {TEXT("Hollow"), TEXT("공허")}, {TEXT("Reach the Hollow ending."), TEXT("공허 엔딩에 도달한다.")},
        {TEXT("The Weave"), TEXT("엮음")}, {TEXT("Reach the Weave ending, seal BL-07 without burning your name."), TEXT("이름을 태우지 않고 BL-07을 봉인하는 엮음 엔딩에 도달한다.")},
        {TEXT("Every Path"), TEXT("모든 길")}, {TEXT("See all 7 endings."), TEXT("엔딩 7개를 모두 본다.")},
        {TEXT("Merchant"), TEXT("거래 성사")}, {TEXT("Complete a trade with Malet."), TEXT("말렛과 거래를 마친다.")},
        {TEXT("Wealthy"), TEXT("부유함")}, {TEXT("Accumulate 100 Grains."), TEXT("그레인을 100 모은다.")},
        {TEXT("Memory Hunter"), TEXT("기억 사냥꾼")}, {TEXT("Complete all side quests."), TEXT("모든 사이드 퀘스트를 마친다.")},
        {TEXT("New Game+"), TEXT("회차 플레이")}, {TEXT("Start a New Game+ run."), TEXT("회차 플레이를 시작한다.")},
        // Saves and checkpoints (MemoriaCheckpointSubsystem status lines).
        {TEXT("No checkpoint saved in this session."), TEXT("이번 세션에 저장된 체크포인트가 없습니다.")},
        {TEXT("Disk saving is disabled for this test session."), TEXT("이 테스트 세션에서는 디스크 저장이 꺼져 있습니다.")},
        {TEXT("Autosave skipped: disk storage is unavailable in this session."), TEXT("자동 저장 건너뜀: 이번 세션에서는 디스크 저장소를 쓸 수 없습니다.")},
        {TEXT("Autosave skipped: no memory catalog."), TEXT("자동 저장 건너뜀: 기억 목록이 없습니다.")},
        {TEXT("Autosave skipped: this run is outside the supported boundary."), TEXT("자동 저장 건너뜀: 이 진행은 지원 범위를 벗어났습니다.")},
        {TEXT("Autosave skipped: serialization failed."), TEXT("자동 저장 건너뜀: 데이터 변환에 실패했습니다.")},
        {TEXT("Autosave skipped: cannot create the save folder."), TEXT("자동 저장 건너뜀: 저장 폴더를 만들 수 없습니다.")},
        {TEXT("Autosave failed: disk write failed."), TEXT("자동 저장 실패: 디스크에 쓰지 못했습니다.")},
        {TEXT("Chapter autosaved."), TEXT("장을 자동 저장했습니다.")},
        {TEXT("Autosave unavailable: disk storage is disabled or busy."), TEXT("자동 저장을 쓸 수 없음: 디스크 저장소가 꺼져 있거나 사용 중입니다.")},
        {TEXT("No valid chapter autosave found."), TEXT("유효한 장 자동 저장이 없습니다.")},
        {TEXT("Save skipped: disk storage is unavailable in this session."), TEXT("저장 건너뜀: 이번 세션에서는 디스크 저장소를 쓸 수 없습니다.")},
        {TEXT("Save skipped: this run is outside the supported boundary."), TEXT("저장 건너뜀: 이 진행은 지원 범위를 벗어났습니다.")},
        {TEXT("Save skipped: serialization failed."), TEXT("저장 건너뜀: 데이터 변환에 실패했습니다.")},
        {TEXT("Save skipped: cannot create the save folder."), TEXT("저장 건너뜀: 저장 폴더를 만들 수 없습니다.")},
        {TEXT("Save failed: disk write failed."), TEXT("저장 실패: 디스크에 쓰지 못했습니다.")},
        {TEXT("Saved in the field."), TEXT("현장에서 저장했습니다.")},
        {TEXT("Save unavailable: disk storage is disabled or busy."), TEXT("저장할 수 없음: 디스크 저장소가 꺼져 있거나 사용 중입니다.")},
        {TEXT("No valid field save found. The current run has not changed."), TEXT("유효한 현장 저장이 없습니다. 현재 진행은 그대로입니다.")},
        {TEXT("The field save could not be restored. The current run has not changed."), TEXT("현장 저장을 복원하지 못했습니다. 현재 진행은 그대로입니다.")},
        {TEXT("Field save loaded."), TEXT("현장 저장을 불러왔습니다.")},
        {TEXT("Checkpoint not saved: disk storage is unavailable in this session."), TEXT("체크포인트 저장 안 됨: 이번 세션에서는 디스크 저장소를 쓸 수 없습니다.")},
        {TEXT("Checkpoint not saved: no valid run."), TEXT("체크포인트 저장 안 됨: 유효한 진행이 없습니다.")},
        {TEXT("Checkpoint not saved: this run is outside the supported boundary."), TEXT("체크포인트 저장 안 됨: 이 진행은 지원 범위를 벗어났습니다.")},
        {TEXT("Checkpoint not saved: serialization failed."), TEXT("체크포인트 저장 안 됨: 데이터 변환에 실패했습니다.")},
        {TEXT("Checkpoint not saved: cannot create the save folder."), TEXT("체크포인트 저장 안 됨: 저장 폴더를 만들 수 없습니다.")},
        {TEXT("Checkpoint not saved: backup failed. Previous checkpoint retained."), TEXT("체크포인트 저장 안 됨: 백업에 실패했습니다. 이전 체크포인트는 남아 있습니다.")},
        {TEXT("Checkpoint not saved: disk write failed. Retry saving."), TEXT("체크포인트 저장 안 됨: 디스크에 쓰지 못했습니다. 다시 저장해 주세요.")},
        {TEXT("Checkpoint saved to disk."), TEXT("체크포인트를 디스크에 저장했습니다.")},
        {TEXT("Checkpoint unavailable: disk storage is disabled or busy."), TEXT("체크포인트를 쓸 수 없음: 디스크 저장소가 꺼져 있거나 사용 중입니다.")},
        {TEXT("No valid checkpoint found. The current run has not changed."), TEXT("유효한 체크포인트가 없습니다. 현재 진행은 그대로입니다.")},
        {TEXT("Checkpoint could not be restored. The current run has not changed."), TEXT("체크포인트를 복원하지 못했습니다. 현재 진행은 그대로입니다.")},
        {TEXT("Checkpoint loaded. Your exchange is complete."), TEXT("체크포인트를 불러왔습니다. 거래가 끝났습니다.")},
        {TEXT("Checkpoint recovered from backup. Primary save repaired."), TEXT("백업에서 체크포인트를 복구했습니다. 원본 저장도 고쳤습니다.")},
        {TEXT("Checkpoint loaded from backup. Primary repair failed; backup retained."), TEXT("백업에서 체크포인트를 불러왔습니다. 원본은 고치지 못해 백업을 남겨 두었습니다.")},
        {TEXT("Classifier report: Arrel identified"), TEXT("분류자 보고서: 아렐 신원 확인")},
        {TEXT("Classifier report: requester unknown"), TEXT("분류자 보고서: 요청자 불명")},
        {TEXT("Manual save unavailable."), TEXT("수동 저장을 쓸 수 없습니다.")},
        {TEXT("Confirm before replacing this manual save."), TEXT("이 수동 저장을 덮어쓰기 전에 확인해 주세요.")},
        {TEXT("Manual save skipped: this run is outside the supported boundary."), TEXT("수동 저장 건너뜀: 이 진행은 지원 범위를 벗어났습니다.")},
        {TEXT("Manual save failed: serialization failed."), TEXT("수동 저장 실패: 데이터 변환에 실패했습니다.")},
        {TEXT("Manual save failed: cannot create the save folder."), TEXT("수동 저장 실패: 저장 폴더를 만들 수 없습니다.")},
        {TEXT("Manual save failed: backup failed. Previous save retained."), TEXT("수동 저장 실패: 백업에 실패했습니다. 이전 저장은 남아 있습니다.")},
        {TEXT("Manual save failed: disk write failed or slot changed. Previous save retained."), TEXT("수동 저장 실패: 디스크에 쓰지 못했거나 슬롯이 바뀌었습니다. 이전 저장은 남아 있습니다.")},
        {TEXT("Manual slot saved."), TEXT("수동 슬롯에 저장했습니다.")},
        {TEXT("Manual load unavailable."), TEXT("수동 불러오기를 쓸 수 없습니다.")},
        {TEXT("No valid manual save found. The current run has not changed."), TEXT("유효한 수동 저장이 없습니다. 현재 진행은 그대로입니다.")},
        {TEXT("Manual save could not be restored. The current run has not changed."), TEXT("수동 저장을 복원하지 못했습니다. 현재 진행은 그대로입니다.")},
        {TEXT("Manual slot loaded."), TEXT("수동 슬롯을 불러왔습니다.")},
        {TEXT("Manual save recovered from backup. Primary repaired."), TEXT("백업에서 수동 저장을 복구했습니다. 원본도 고쳤습니다.")},
        {TEXT("Manual save loaded from backup. Repair failed; backup retained."), TEXT("백업에서 수동 저장을 불러왔습니다. 원본은 고치지 못해 백업을 남겨 두었습니다.")},
        // The exchange's end screen (MemoriaNarrativeSubsystem development stop) and the scene headers.
        {TEXT("VERDAN EXCHANGE COMPLETE / DEVELOPMENT BOUNDARY"), TEXT("베르단 거래 완료 / 개발 구간 경계")},
        {TEXT("MEMORIA / CONTINUE"), TEXT("MEMORIA / 이어하기")},
        {TEXT("VERDAN / EXCHANGE COMPLETE"), TEXT("베르단 / 거래 완료")},
        {TEXT("Return to Verdan"), TEXT("베르단으로 돌아가기")},
        {TEXT("Load checkpoint"), TEXT("체크포인트 불러오기")},
        {TEXT("Start a new slice"), TEXT("처음부터 시작하기")},
        {TEXT("Save checkpoint again"), TEXT("체크포인트 다시 저장하기")},
        {TEXT("Travel on: Chapter 3, The Belt"), TEXT("길을 떠난다: 3장, 벨트")},
        {TEXT("CHAPTER II  /  VERDAN"), TEXT("2장  /  베르단")},
        {TEXT("THE SUMP  /  MALET"), TEXT("웅덩이  /  말렛")},
        {TEXT("VERDAN  /  ELIA"), TEXT("베르단  /  엘리아")},
        {TEXT("VERDAN  /  THE SUMP LEDGER"), TEXT("베르단  /  웅덩이의 장부")},
        {TEXT("VERDAN  /  MEMORY MARKET"), TEXT("베르단  /  기억 시장")},
        {TEXT("VERDAN  /  MARKET EDGE"), TEXT("베르단  /  시장 변두리")},
        {TEXT("THE SUMP  /  ENTRANCE"), TEXT("웅덩이  /  입구")},
        {TEXT("THE SUMP"), TEXT("웅덩이")},
        {TEXT("DECISION"), TEXT("결정")},
        {TEXT("Some choices change what Arrel can keep, spend, or survive."), TEXT("어떤 선택은 아렐이 지킬 것, 쓸 것, 버텨 낼 것을 바꾼다.")},
        // The journal's compass readings for each loss (story_journal LOSS_RULES compass; the source has no Korean).
        {TEXT("Verdan taste map erased."), TEXT("베르단 미각 지도 소거됨.")},
        {TEXT("Anchor melody destabilized."), TEXT("닻 선율 불안정.")},
        {TEXT("Relationship contour torn."), TEXT("관계 윤곽 찢어짐.")},
        {TEXT("Identity architecture fractured."), TEXT("정체성 구조 균열.")},
        {TEXT("Name-bearing thread severed."), TEXT("이름을 잇는 실 끊어짐.")},
        {TEXT("Record-tree contour retained."), TEXT("기록수 윤곽 유지.")},
        {TEXT("Elia anchor pressure reduced."), TEXT("엘리아 닻 압력 감소.")},
        {TEXT("Void-walker witness weakened."), TEXT("보이드 보행자 증언 약화.")},
        {TEXT("Rim weather contour unmoored."), TEXT("림 날씨 윤곽 표류.")},
        {TEXT("Forest witness contour hollowed."), TEXT("숲 증언 윤곽 공동화.")},
        {TEXT("Colorless route no longer confirms north."), TEXT("무색 경로가 더는 북쪽을 확인하지 못함.")},
        {TEXT("BL-07 passage contour weakened."), TEXT("BL-07 통로 윤곽 약화.")},
        // Notices.
        {TEXT("Find the ledger in the Sump."), TEXT("웅덩이에서 장부를 찾아라.")},
        {TEXT("The Sump Ledger"), TEXT("웅덩이의 장부")},
        {TEXT("Signposts"), TEXT("표지판")},
        {TEXT("Memory noise closes in"), TEXT("기억 소음이 닫힌다")},
        {TEXT("Memory noise closes in..."), TEXT("기억 소음이 닫힌다...")},
        // The dialogue frame's own words.
        {TEXT("NEXT  ▼"), TEXT("다음  ▼")},
        {TEXT("YOUR CHOICE"), TEXT("선택")},
        {TEXT("PAUSED"), TEXT("일시정지")},
        {TEXT("Enter / A or Back: return to the current line"), TEXT("Enter / A 또는 뒤로: 지금 대사로 돌아가기")},
        {TEXT("Recorded development observation"), TEXT("개발 관찰 기록")},
        {TEXT("↑ ↓  Select      ENTER  Confirm"), TEXT("↑ ↓  선택      ENTER  결정")},
        {TEXT("ENTER  Next      ESC  Pause      TAB  Memories"), TEXT("ENTER  다음      ESC  일시정지      TAB  기억")},
        {TEXT("UP / DOWN  Select     ENTER / A  Confirm     or click a choice"), TEXT("↑ / ↓  선택     ENTER / A  결정     또는 선택지를 클릭")},
        {TEXT("ENTER / E / SPACE / A  Continue"), TEXT("ENTER / E / SPACE / A  계속")},
        // The memory exchange (shop) and the archive's frames.
        {TEXT("SELL"), TEXT("판매")}, {TEXT("BUY"), TEXT("구매")}, {TEXT("CLOSE / ESC"), TEXT("닫기 / ESC")},
        {TEXT("THE MEMORY EXCHANGE"), TEXT("기억 교환소")},
        {TEXT("Not enough Grains"), TEXT("그레인이 부족하다")},
        {TEXT("Select a memory"), TEXT("기억을 고르세요")},
        {TEXT("Up / Down: select   Left / Right: tab\nSelling burns the selected memory."), TEXT("↑ / ↓: 선택   ← / →: 탭\n판매하면 고른 기억이 연소된다.")},
        {TEXT("No memories available to sell."), TEXT("팔 수 있는 기억이 없다.")},
        {TEXT("No memories available to buy."), TEXT("살 수 있는 기억이 없다.")},
        {TEXT("Memory Archive"), TEXT("기억 서고")},
        {TEXT("CLOSE  /  TAB"), TEXT("닫기  /  TAB")},
        {TEXT("UP / DOWN  Browse     LEFT / RIGHT  Grade     TAB / M / ESC / B  Close"), TEXT("↑ / ↓  살펴보기     ← / →  등급     TAB / M / ESC / B  닫기")},
        {TEXT("MEMORY ARCHIVE  //  00"), TEXT("기억 서고  //  00")},
        {TEXT("Confirm received. Escape / B closes this modal."), TEXT("확인했습니다. Esc / B로 이 창을 닫습니다.")},
    };
    return Values;
}
const TMap<FString, FString>& PromptNames()
{
    static const TMap<FString, FString> Values = {
        {TEXT("Elia"), TEXT("엘리아")}, {TEXT("Malet"), TEXT("말렛")}, {TEXT("Nervous Trader"), TEXT("불안한 상인")},
        {TEXT("Loose stone"), TEXT("헐거운 돌")}, {TEXT("Memory stalls"), TEXT("기억 노점")}, {TEXT("Old man"), TEXT("노인")},
        {TEXT("Malet's table"), TEXT("말렛의 탁자")}, {TEXT("Sump stairs"), TEXT("웅덩이 계단")},
        {TEXT("Leaning sign"), TEXT("기울어진 표지판")}, {TEXT("Waymark"), TEXT("이정표")}, {TEXT("Wall stone"), TEXT("돌벽의 돌")}};
    return Values;
}
const TMap<FString, FString>& PromptVerbs()
{
    static const TMap<FString, FString> Values = {
        {TEXT("talk"), TEXT("대화하기")}, {TEXT("search"), TEXT("살피기")}, {TEXT("look"), TEXT("둘러보기")},
        {TEXT("sit"), TEXT("앉기")}, {TEXT("listen"), TEXT("듣기")}, {TEXT("read"), TEXT("읽기")}, {TEXT("open"), TEXT("열기")}};
    return Values;
}
FString Lookup(const TMap<FString, FString>& Map, const FString& Key)
{ const FString* Found = Map.Find(Key); return Found ? *Found : MemoriaChapterMaps::Korean(Key); }
}
namespace MemoriaKorean
{
bool IsKorean(const UObject* WorldContext)
{
    const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    const UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
    if (!Game) return true;
    if (const auto* Run = Game->GetSubsystem<UMemoriaRunSubsystem>(); Run && Run->HasActiveRun())
        return Run->GetRunSnapshot().CurrentLocale == TEXT("ko");
    const auto* Settings = Game->GetSubsystem<UMemoriaSettingsSubsystem>();
    return !Settings || Settings->GetLocale() != TEXT("en");
}
static FString TextRaw(const FString& English)
{
    if (English.IsEmpty()) return English;
    if (const FString* Found = Table().Find(English)) return *Found;
    // "Name  |  E / A: verb": the name and the verb apart, the keys and the frame kept for the HUD's split.
    FString Name, Rest, Keys, Verb;
    if (English.Split(TEXT("|"), &Name, &Rest) && Rest.Split(TEXT(":"), &Keys, &Verb))
    {
        const FString CleanName = Name.TrimStartAndEnd(), CleanVerb = Verb.TrimStartAndEnd();
        const FString* KoVerb = PromptVerbs().Find(CleanVerb);
        return FString::Printf(TEXT("%s  |  %s: %s"), *Lookup(PromptNames(), CleanName), *Keys.TrimStartAndEnd(), KoVerb ? **KoVerb : *CleanVerb);
    }
    if (English.StartsWith(TEXT("Quest Complete: "))) return TEXT("퀘스트 완료: ") + Text(English.RightChop(16));
    if (English.StartsWith(TEXT("Signposts "))) return TEXT("표지판 ") + English.RightChop(10);   // S370: "Signposts 1 / 3"
    if (English.StartsWith(TEXT("+")) && English.EndsWith(TEXT(" Grains"))) return English.LeftChop(7) + TEXT(" 그레인");
    if (English.StartsWith(TEXT("+")) && English.EndsWith(TEXT(" Hi-Potion"))) return English.LeftChop(10) + TEXT(" 하이포션");
    if (English.StartsWith(TEXT("Development boundary: resolved ")) && English.EndsWith(TEXT("; content deferred.")))
        return TEXT("개발 경계: ") + English.Mid(31, English.Len() - 31 - 19) + TEXT(" 처리됨, 내용 보류.");
    // Field and scene headers ("THE BELT / STORY   3 / 9"): the words, the counters kept.
    if (English.Contains(TEXT(" / STORY ")) || English.Contains(TEXT(" / VN ")))
        return English.Replace(TEXT(" / STORY "), TEXT(" / 이야기 ")).Replace(TEXT(" / VN "), TEXT(" / 장면 "))
            .Replace(TEXT("MALET / "), TEXT("말렛 / ")).Replace(TEXT("VERDAN"), TEXT("베르단")).Replace(TEXT("ENCOUNTER"), TEXT("조우"))
            .Replace(TEXT("REFUSAL"), TEXT("거절")).Replace(TEXT("DEAL"), TEXT("거래")).Replace(TEXT("REWARD"), TEXT("보상"))
            .Replace(TEXT("MEMORY REACTION"), TEXT("기억 반응")).Replace(TEXT("CH2 ARRIVAL"), TEXT("2장 도착"));
    return MemoriaChapterMaps::Korean(English);
}
// S370: whatever the Korean is, it is shown in the final manuscript's words and names.
FString Text(const FString& English) { return MemoriaManuscript::Canon(TextRaw(English)); }
bool MemoryText(const FString& MemoryId, FString& OutTitle, FString& OutDescription)
{
    if (FMemoriaMemoryLocalizedText Chapter; MemoriaChapterMemories::Korean(MemoryId, Chapter))
    { OutTitle = MemoriaManuscript::Canon(Chapter.Title); OutDescription = MemoriaManuscript::Canon(Chapter.Description); return true; }
    const auto* Catalog = LoadObject<UMemoriaMemoryCatalog>(nullptr, TEXT("/Game/Memoria/Generated/Memory/DA_StartingMemoryCatalog.DA_StartingMemoryCatalog"));
    if (const auto* Text = Catalog ? Catalog->LocalizedText.FindByPredicate([&](const auto& T) { return T.MemoryId == MemoryId && T.Locale == TEXT("ko"); }) : nullptr)
    { OutTitle = MemoriaManuscript::Canon(Text->Title); OutDescription = MemoriaManuscript::Canon(Text->Description); return true; }
    return false;
}
FString Grains(int64 Amount, bool bKorean)
{ return bKorean ? FString::Printf(TEXT("%lld 그레인"), Amount) : FString::Printf(TEXT("%lld Grains"), Amount); }
}
