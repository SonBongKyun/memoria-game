#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Run/MemoriaRunTypes.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"

namespace
{
// Whole lines the manuscript rewrote: the IR's Korean (verbatim) -> the final manuscript's.
const TMap<FString, FString>& Lines()
{
    static const TMap<FString, FString> Values = {
#include "MemoriaManuscriptLines.inl"
    };
    return Values;
}
// Names and units fixed by the user's canon decisions (2026-09-24/27) and the final manuscript.
// Longer phrases first, so a phrase is never half-replaced by a shorter one.
const TArray<TPair<FString, FString>>& Terms()
{
    static const TArray<TPair<FString, FString>> Values = {
        // The particle follows the new name's last sound (BL-07 and 말렛 end in a consonant; 공백 7호 and 말레트 do not).
        {TEXT("BL-07이"), TEXT("공백 7호가")}, {TEXT("BL-07은"), TEXT("공백 7호는")}, {TEXT("BL-07을"), TEXT("공백 7호를")},
        {TEXT("BL-07과"), TEXT("공백 7호와")}, {TEXT("BL-07으로"), TEXT("공백 7호로")},
        {TEXT("말렛이"), TEXT("말레트가")}, {TEXT("말렛은"), TEXT("말레트는")}, {TEXT("말렛을"), TEXT("말레트를")},
        {TEXT("말렛과"), TEXT("말레트와")}, {TEXT("말렛으로"), TEXT("말레트로")},
        {TEXT("드리프트 셸터"), TEXT("무너진 돌벽")}, {TEXT("벨트 중간역"), TEXT("벨트 역참")}, {TEXT("벨트 중계소"), TEXT("벨트 역참")},
        {TEXT("웅덩이의 장부"), TEXT("셈터의 장부")}, {TEXT("웅덩이에서 장부"), TEXT("셈터에서 장부")},
        {TEXT("웅덩이에 숨겨진"), TEXT("셈터에 숨겨진")}, {TEXT("웅덩이 계단"), TEXT("셈터 계단")}, {TEXT("웅덩이  /"), TEXT("셈터  /")},
        {TEXT("보이드 비스트"), TEXT("공허수")}, {TEXT("보이드 홀"), TEXT("공허 구멍")}, {TEXT("14킬로미터"), TEXT("아홉 마일")}, {TEXT("6장, 심"), TEXT("6장, 더 심")},
        {TEXT("아켄 숲"), TEXT("아헨 숲")}, {TEXT("더 섬프"), TEXT("셈터")}, {TEXT("섬프"), TEXT("셈터")},
        {TEXT("씸에서"), TEXT("더 심에서")}, {TEXT("BL-07"), TEXT("공백 7호")}, {TEXT("말렛"), TEXT("말레트")},
        {TEXT("분류자"), TEXT("분류하는 자")}, {TEXT("7등급"), TEXT("7급")}, {TEXT("백서"), TEXT("백 노트")}};
    return Values;
}
// Whole strings only: these words are too short to replace inside other text.
const TMap<FString, FString>& Exact()
{
    static const TMap<FString, FString> Values = {
        {TEXT("웅덩이"), TEXT("셈터")}, {TEXT("심"), TEXT("더 심")}, {TEXT("드리프트"), TEXT("표류")}};
    return Values;
}
const TCHAR* AnchorPrefix = TEXT("ms_drift_anchor_burns_");
constexpr int32 AnchorMax = 64;

struct FSignText { const TCHAR* Speaker; const TCHAR* En; const TCHAR* Ko; };
struct FSign { FVector2D Origin; const TCHAR* Prompt; TArray<FSignText> Clear, Loose; };
// Tile centres on the drift map's walkable ground: the road north of the spawn, the open ground on the
// way to the east exit, and the shelter floor beside the broken wall (drift_shelter.v1.json tiles).
const TArray<FSign>& Signs()
{
    static const TArray<FSign> Values = {
        {FVector2D(368, 464), TEXT("Leaning sign  |  E / A: read"),
            {{nullptr, TEXT("The board leaned toward the road. Rust from the nail had run over one stroke of the first letter."), TEXT("판자는 길가로 기울어 있었다. 못에서 흘러내린 녹물이 첫 글자의 한 획을 덮고 있었다.")},
             {TEXT("Arrel"), TEXT("Verdan."), TEXT("베르단.")},
             {nullptr, TEXT("It pointed back the way they had come. There was no reason to go that way."), TEXT("판자 끝은 그들이 온 쪽을 가리켰다. 그쪽으로 갈 일은 없었다.")}},
            {{nullptr, TEXT("Rust covered one stroke of the first letter. The next letter was clean, and still would not join the first."), TEXT("녹물이 첫 글자의 한 획을 덮고 있었다. 그다음 글자는 깨끗했는데도, 앞의 것과 이어 읽을 수 없었다.")},
             {TEXT("Elia"), TEXT("Verdan. ...Not joining again?"), TEXT("베르단. ...또 안 붙어?")},
             {TEXT("Arrel"), TEXT("I'm telling you. Like you said."), TEXT("말하는 거야. 안 읽히면 말하라며.")}}},
        {FVector2D(560, 368), TEXT("Waymark  |  E / A: read"),
            {{nullptr, TEXT("The letters were cut deep. The light came in at a slant and every stroke threw a shadow."), TEXT("글씨가 깊이 파여 있었고, 빛이 비스듬히 들어 획마다 그림자가 졌다.")},
             {TEXT("Arrel"), TEXT("Nine miles to the next stop."), TEXT("다음 경유지까지 아홉 마일.")},
             {nullptr, TEXT("He read it, checked the meaning, and only then moved on."), TEXT("글씨를 읽고 뜻을 확인한 다음에 움직였다.")}},
            {{nullptr, TEXT("He sounded the first letter. By the time he came back from the next, he could not tell if the sound was right."), TEXT("맨 앞 글자를 속으로 발음했다. 다음 것을 확인하고 돌아오면 방금 읽은 소리가 맞는지 알 수 없었다.")},
             {TEXT("Arrel"), TEXT("Elia. What does that say?"), TEXT("엘리아. 저거 뭐라고 돼 있어.")},
             {TEXT("Elia"), TEXT("Nine miles to the next stop."), TEXT("다음 경유지까지 아홉 마일.")},
             {nullptr, TEXT("She wiped under her nose with the back of her hand. He let himself not see it."), TEXT("엘리아가 손등으로 코 밑을 닦았다. 아렐은 보지 못한 척했다.")}}},
        {FVector2D(272, 304), TEXT("Wall stone  |  E / A: read"),
            {{nullptr, TEXT("Small letters were left on one stone of the broken wall."), TEXT("무너진 돌벽의 돌 하나에 작은 글씨가 남아 있었다.")},
             {nullptr, TEXT("Property of the Verdan Trade Bureau. Trespassers will be recorded."), TEXT("베르단 무역 관리국 소유. 침입자는 기록됩니다.")},
             {TEXT("Arrel"), TEXT("It reads."), TEXT("읽혀.")},
             {TEXT("Elia"), TEXT("It'll hold for a while."), TEXT("당분간은 괜찮을 거야.")}},
            {{nullptr, TEXT("Small letters were left on one stone. He saw every stroke; no sound came with them."), TEXT("돌 하나에 작은 글씨가 남아 있었다. 획은 다 보였는데 소리가 따라오지 않았다.")},
             {TEXT("Elia"), TEXT("Property of the Verdan Trade Bureau. Trespassers will be recorded."), TEXT("베르단 무역 관리국 소유. 침입자는 기록됩니다.")},
             {TEXT("Elia"), TEXT("I told you. It depends on how often you burn."), TEXT("말했잖아. 네가 얼마나 자주 태우느냐에 달렸다고.")},
             {TEXT("Arrel"), TEXT("I know."), TEXT("알아.")}}},
    };
    return Values;
}
FString Mark(bool bDone, bool bCurrent, const FString& Text, bool bKo)
{
    if (bDone) return TEXT("  - ") + Text + (bKo ? TEXT("  (완료)") : TEXT("  (done)"));
    return (bCurrent ? TEXT("→ ") : TEXT("  ")) + Text;
}
}

namespace MemoriaManuscript
{
FString Canon(const FString& Korean)
{
    if (Korean.IsEmpty()) return Korean;
    if (const FString* Line = Lines().Find(Korean)) return *Line;
    if (const FString* Word = Exact().Find(Korean)) return *Word;
    FString Out = Korean;
    for (const auto& Term : Terms())
        if (Out.Contains(Term.Key, ESearchCase::CaseSensitive)) Out.ReplaceInline(*Term.Key, *Term.Value, ESearchCase::CaseSensitive);
    return Out;
}
int32 LineCount() { return Lines().Num(); }

const TArray<FMainStep>& MainSteps()
{
    static const TArray<FMainStep> Values = {
        {TEXT("ch2_arrived"), TEXT("Leave the Ahen forest for Verdan"), TEXT("아헨 숲을 빠져나가 베르단으로")},
        {TEXT("ch2_malet_done"), TEXT("Ask Malet about BL-07 in the Sump"), TEXT("셈터에서 말레트에게 공백 7호를 묻는다")},
        {TEXT("ch3_waystation_night"), TEXT("Spend the night at the waystation northeast of Verdan"), TEXT("베르단 북동쪽 역참에서 밤을 넘긴다")},
        {TEXT("ch3_class_seven_message"), TEXT("Read the writing on the wall"), TEXT("벽의 글씨를 읽는다")},
        {TEXT("ch4_anchoring"), TEXT("Follow the road east of the waystation"), TEXT("역참 동쪽 길을 따라간다")},
        {TEXT("ch4_night_watch"), TEXT("Keep watch in the rain-cut cave"), TEXT("빗물 동굴에서 밤을 지킨다")},
        {TEXT("ch6_arrived"), TEXT("Find Sable at the Seam, northeast of Verdan. Follow the colour."), TEXT("더 심의 세이블을 찾는다. 베르단 북동쪽, 색을 따르게.")},
        // S371: 6-7화.
        {TEXT("ch6_sable_morning"), TEXT("Ask Sable the way"), TEXT("세이블에게 길을 묻는다")},
        {TEXT("ch6_complete"), TEXT("Up the ridge road with Haren"), TEXT("하렌을 따라 능선길로")},
        // S372: 8-10화.
        {TEXT("ch7_lost_hour"), TEXT("Through the listening forest, eyes off the sounds"), TEXT("듣는 숲을 지난다. 소리 쪽으로 고개를 돌리지 말 것")},
        {TEXT("ch7_escape"), TEXT("Get away from the shape between the trees"), TEXT("나무 사이의 형상에게서 벗어난다")},
        {TEXT("ch7_complete"), TEXT("A night under the slanted slab, between the two stone ridges"), TEXT("두 돌 능선 사이, 기울어진 돌판 아래에서 밤")},
        {TEXT("ms_never"), TEXT("Where the hole in the bark points: north-north-east, through the unrecorded stretch"), TEXT("나무껍질의 구멍이 가리키는 곳: 북북동, 기록되지 않은 구간 너머")}};
    return Values;
}
int32 MainStepIndex(const FMemoriaRunSnapshot& Run)
{
    const auto& Steps = MainSteps();
    for (int32 I = 0; I < Steps.Num(); ++I) if (!Run.GetFlag(Steps[I].Flag)) return I;
    return Steps.Num();
}
FMemoriaJournalRecord MainRecord(const FMemoriaRunSnapshot& Run, bool bKo)
{
    FMemoriaJournalRecord R;
    R.Title = bKo ? TEXT("공백 7호를 찾아서") : TEXT("The Road to BL-07");
    R.Label = (bKo ? TEXT("[메인] ") : TEXT("[MAIN] ")) + R.Title;
    R.Color = FLinearColor(.92f, .74f, .42f);
    R.Art = TEXT("res://assets/cg/generated/archive_ch2_information_price_v1.png");
    FString Body = bKo
        ? TEXT("관리국이 지워 버린 도시. 기록에서도 지도에서도 없어졌고, 지도에는 그 자리에 호수가 그려져 있다. 찾으러 간 사람은 열일곱. 돌아온 사람은 없다.")
        : TEXT("The city the Bureau erased. It is gone from the records and the maps; the maps show a lake where it stood. Seventeen went looking. None came back.");
    if (Run.GetFlag(TEXT("ch2_malet_done")))
        Body += bKo ? TEXT("\n\n말레트에게 받은 것: 경로(붕괴의 해안을 지나 마지막 추출소 너머), 이름(세이블, 더 심), 경고(에디터 카이로스).")
                    : TEXT("\n\nFrom Malet: a route (past the Crumbling Coast, beyond the last extraction post), a name (Sable, the Seam), a warning (the editor Kairos).");
    if (Run.GetFlag(TEXT("ch3_class_seven_message")))
        Body += bKo ? TEXT("\n역참 벽의 글씨: 대상은 7급 연소 효율을 보인다.") : TEXT("\nOn the waystation wall: Subject exhibits Class 7 combustion efficiency.");
    Body += TEXT("\n");
    const int32 Current = MainStepIndex(Run);
    const auto& Steps = MainSteps();
    for (int32 I = 0; I < Steps.Num() && I <= Current; ++I)
        Body += TEXT("\n") + Mark(I < Current, I == Current, bKo ? Steps[I].Ko : Steps[I].En, bKo);
    R.Body = Body;
    return R;
}

int32 SignCount() { return Signs().Num(); }
FString SignGroup(int32 Index) { return FString::Printf(TEXT("ms_drift_sign_%d"), Index + 1); }
int32 SignIndex(const FString& Group)
{
    for (int32 I = 0; I < SignCount(); ++I) if (Group.Equals(SignGroup(I), ESearchCase::CaseSensitive)) return I;
    return INDEX_NONE;
}
FVector2D SignOrigin(int32 Index) { return Signs().IsValidIndex(Index) ? Signs()[Index].Origin : FVector2D::ZeroVector; }
FString SignPrompt(int32 Index) { return Signs().IsValidIndex(Index) ? Signs()[Index].Prompt : FString(); }
bool SignsOpen(const FMemoriaRunSnapshot& Run) { return Run.GetFlag(AnchorFlag); }
int32 SignsRead(const FMemoriaRunSnapshot& Run)
{
    int32 N = 0;
    for (int32 I = 0; I < SignCount(); ++I) if (Run.GetFlag(SignGroup(I))) ++N;
    return N;
}
int32 AnchorBaseline(const FMemoriaRunSnapshot& Run)
{
    for (int32 I = 0; I <= AnchorMax; ++I) if (Run.GetFlag(FString::Printf(TEXT("%s%d"), AnchorPrefix, I))) return I;
    return INDEX_NONE;
}
void EnsureAnchorBaseline(UMemoriaRunSubsystem& Run)
{
    if (!Run.HasActiveRun() || !Run.GetPlayerMemory()) return;
    const auto& S = Run.GetRunSnapshot();
    if (!S.GetFlag(AnchorFlag) || AnchorBaseline(S) != INDEX_NONE) return;
    const int32 Burned = FMath::Clamp(Run.GetPlayerMemory()->GetSnapshot().BurnedHistory.Num(), 0, AnchorMax);
    Run.SetStoryFlag(FString::Printf(TEXT("%s%d"), AnchorPrefix, Burned), true);
}
bool Scrambled(const FMemoriaRunSnapshot& Run, int32 BurnedNow)
{
    const int32 Base = AnchorBaseline(Run);
    return Base != INDEX_NONE && FMath::Min(BurnedNow, AnchorMax) > Base;
}
UMemoriaFieldAsset* BuildSign(UObject* Outer, int32 Index, bool bScrambled)
{
    if (!Signs().IsValidIndex(Index)) return nullptr;
    auto* Asset = NewObject<UMemoriaFieldAsset>(Outer);
    const FString Id = SignGroup(Index);
    Asset->Definition.Id = Id; Asset->Definition.IndexMappingVersion = 1;
    const auto& Lines = bScrambled ? Signs()[Index].Loose : Signs()[Index].Clear;
    for (int32 I = 0; I < Lines.Num(); ++I)
    {
        FMemoriaFieldRow Row; Row.Id = FString::Printf(TEXT("field/%s/%d"), *Id, I);
        Row.OriginalIndex = I; Row.StorageIndex = I; Row.EffectPhase = TEXT("gate_then_effects");
        auto& T = Row.Text;
        if (Lines[I].Speaker)
        {
            T.bHasSpeaker = true; T.Speaker = Lines[I].Speaker;
            T.bHasText = true; T.Text = Lines[I].En; T.bHasTextKo = true; T.TextKo = Lines[I].Ko;
        }
        else { T.bHasNarrate = true; T.Narrate = Lines[I].En; T.bHasNarrateKo = true; T.NarrateKo = Lines[I].Ko; }
        Asset->Definition.Rows.Add(Row);
    }
    return Asset;
}
FMemoriaJournalRecord SignsRecord(const FMemoriaRunSnapshot& Run, bool bKo)
{
    FMemoriaJournalRecord R;
    R.Title = bKo ? TEXT("표지판") : TEXT("Signposts");
    const int32 Read = SignsRead(Run);
    const bool bDone = Run.GetFlag(SignsDoneFlag);
    R.Label = bDone ? (bKo ? TEXT("[완료] ") : TEXT("[DONE] ")) + R.Title : R.Title;
    R.Color = bDone ? FLinearColor(.4f, .7f, .4f) : FLinearColor(.85f, .7f, .4f);
    R.Art = TEXT("res://assets/cg/generated/archive_ch4_reading_loss_v1.png");
    R.Body = bKo ? TEXT("그 뒤로 아렐은 표지판을 그냥 지나치지 않았다. 글씨를 읽고 뜻을 확인한 다음에 움직였다.\n엘리아가 붙들어 놓은 읽기는 그가 태울 때마다 다시 풀린다.")
                 : TEXT("After that Arrel did not pass a sign without reading it, and checking the meaning, before he moved on.\nThe reading Elia anchored loosens again each time he burns.");
    R.Body += bDone ? (bKo ? TEXT("\n\n무너진 돌벽 근처의 표지판 셋을 모두 읽었다.") : TEXT("\n\nAll three signs near the broken wall are read."))
                    : bKo ? FString::Printf(TEXT("\n\n현재: 동쪽 길의 표지판 읽기 (%d / %d)"), Read, SignCount())
                          : FString::Printf(TEXT("\n\nCurrent: read the signs on the east road (%d / %d)"), Read, SignCount());
    return R;
}
}
