#include "Chapter/MemoriaChapterMap.h"
#include "Narrative/MemoriaManuscript.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaChapterMapSources.inl"
namespace
{
using Obj = TSharedPtr<FJsonObject>;
FVector2D Vec(const TArray<TSharedPtr<FJsonValue>>& A) { return A.Num() >= 2 ? FVector2D(A[0]->AsNumber(), A[1]->AsNumber()) : FVector2D::ZeroVector; }
FVector2D VecField(const Obj& O, const TCHAR* Key) { const TArray<TSharedPtr<FJsonValue>>* A; return O->TryGetArrayField(Key, A) ? Vec(*A) : FVector2D::ZeroVector; }
FLinearColor Col(const Obj& O, const TCHAR* Key, const FLinearColor& Default)
{
    const TArray<TSharedPtr<FJsonValue>>* A;
    if (!O->TryGetArrayField(Key, A) || A->Num() < 3) return Default;
    return FLinearColor((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber(), A->Num() > 3 ? (*A)[3]->AsNumber() : 1.0);
}
FString Str(const Obj& O, const TCHAR* Key) { FString S; if (O) O->TryGetStringField(Key, S); return S; }
TArray<FString> Strings(const Obj& O, const TCHAR* Key)
{
    TArray<FString> Out; const TArray<TSharedPtr<FJsonValue>>* A;
    if (O->TryGetArrayField(Key, A)) for (const auto& V : *A) Out.Add(V->AsString());
    return Out;
}
FMemoriaChapterRect Rect(const Obj& O)
{
    FMemoriaChapterRect R;
    if (O->HasField(TEXT("origin"))) { R.Origin = VecField(O, TEXT("origin")); R.Size = VecField(O, TEXT("size")); }
    else { R.Size = VecField(O, TEXT("size")); R.Origin = VecField(O, TEXT("center")) - R.Size * .5; }
    return R;
}
FMemoriaChapterMapSpec Parse(const TCHAR* Json)
{
    FMemoriaChapterMapSpec S; Obj O;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O) return S;
    S.Map = Str(O, TEXT("map")); S.AssetPrefix = Str(O, TEXT("asset_prefix")); S.DialogueFile = Str(O, TEXT("dialogue_file")); S.Source = Str(O, TEXT("source"));
    S.Chapter = O->GetIntegerField(TEXT("chapter")); S.TileSize = O->GetIntegerField(TEXT("tile_size"));
    S.Width = O->GetIntegerField(TEXT("width")); S.Height = O->GetIntegerField(TEXT("height"));
    if (const Obj* Title; O->TryGetObjectField(TEXT("title"), Title)) { S.TitleName = Str(*Title, TEXT("name")); S.Subtitle = Str(*Title, TEXT("subtitle")); }
    for (const auto& Row : O->GetArrayField(TEXT("tiles"))) for (const auto& V : Row->AsArray()) S.Tiles.Add(int32(V->AsNumber()));
    for (const auto& D : O->GetArrayField(TEXT("tile_defs")))
    { const Obj T = D->AsObject(); S.TileColors.Add(Col(T, TEXT("color"), FLinearColor::Gray)); S.TileDetails.Add(Str(T, TEXT("detail"))); }
    S.TileNames = Strings(O, TEXT("tile_names"));
    for (const auto& V : O->GetArrayField(TEXT("solid"))) S.Solid.Add(int32(V->AsNumber()));
    if (const Obj* A; O->TryGetObjectField(TEXT("atmosphere"), A))
    {
        S.Hue = Col(*A, TEXT("hue"), FLinearColor::White); S.Light = Col(*A, TEXT("light"), FLinearColor::White);
        (*A)->TryGetNumberField(TEXT("mood"), S.Mood); (*A)->TryGetNumberField(TEXT("brightness"), S.Brightness);
        (*A)->TryGetNumberField(TEXT("saturation"), S.Saturation); S.Splash = Str(*A, TEXT("splash"));
    }
    S.Spawn = VecField(O, TEXT("spawn")); S.EliaRepeat = Str(O, TEXT("elia_repeat"));
    for (const auto& V : O->GetArrayField(TEXT("sequence")))
    {
        const Obj Q = V->AsObject(); FMemoriaChapterStep Step;
        Step.Flag = Str(Q, TEXT("flag")); Step.Group = Str(Q, TEXT("group")); Step.Flags = Strings(Q, TEXT("flags")); Step.Toasts = Strings(Q, TEXT("toasts"));
        Q->TryGetNumberField(TEXT("memories_chapter"), Step.MemoriesChapter);
        S.Sequence.Add(Step);
    }
    if (const Obj* E; O->TryGetObjectField(TEXT("exit"), E))
    {
        S.Exit.Rect = Rect(*E); S.Exit.Requires = Str(*E, TEXT("requires")); S.Exit.Completes = Str(*E, TEXT("completes"));
        S.Exit.Group = Str(*E, TEXT("group")); S.Exit.NextMap = Str(*E, TEXT("next_map")); S.Exit.NextChapter = (*E)->GetIntegerField(TEXT("next_chapter"));
        S.Exit.NextScene = Str(*E, TEXT("next_scene")); S.Exit.Flags = Strings(*E, TEXT("flags"));
        if (const Obj* N; (*E)->TryGetObjectField(TEXT("notice"), N)) { S.Exit.NoticeEn = Str(*N, TEXT("en")); S.Exit.NoticeKo = Str(*N, TEXT("ko")); }
    }
    for (const auto& V : O->GetArrayField(TEXT("triggers")))
    {
        const Obj T = V->AsObject(); FMemoriaChapterTrigger Trigger;
        Trigger.Rect = Rect(T->GetObjectField(TEXT("rect"))); Trigger.Group = Str(T, TEXT("group")); Trigger.Flag = Str(T, TEXT("flag")); Trigger.Gate = Str(T, TEXT("gate"));
        S.Triggers.Add(Trigger);
    }
    S.ObjectsGate = Str(O, TEXT("objects_gate")); S.BattlesGate = Str(O, TEXT("battles_gate")); S.EncountersGate = Str(O, TEXT("encounters_gate"));
    S.ResumeBlocked = Strings(O, TEXT("resume_blocked"));
    for (const auto& V : O->GetArrayField(TEXT("chests")))
    {
        const Obj C = V->AsObject(); FMemoriaChapterChest Chest;
        Chest.Origin = VecField(C, TEXT("origin")); Chest.Flag = Str(C, TEXT("flag"));
        const Obj R = C->GetObjectField(TEXT("rewards")); double Grains = 0; R->TryGetNumberField(TEXT("grains"), Grains); Chest.Grains = int64(Grains);
        if (const Obj* Items; R->TryGetObjectField(TEXT("items"), Items)) for (const auto& Pair : (*Items)->Values) Chest.Items.Emplace(Pair.Key, int64(Pair.Value->AsNumber()));
        S.Chests.Add(Chest);
    }
    for (const auto& V : O->GetArrayField(TEXT("clues")))
    { const Obj C = V->AsObject(); S.Clues.Add({VecField(C, TEXT("origin")), Str(C, TEXT("flag")), Str(C, TEXT("text"))}); }
    for (const auto& V : O->GetArrayField(TEXT("battles")))
    {
        const Obj B = V->AsObject(); FMemoriaChapterBattle Battle;
        Battle.Rect = Rect(B->GetObjectField(TEXT("rect"))); Battle.Name = Str(B, TEXT("name"));
        Battle.Hp = B->GetIntegerField(TEXT("hp")); Battle.Atk = B->GetIntegerField(TEXT("atk")); Battle.bVoid = B->GetBoolField(TEXT("is_void"));
        S.Battles.Add(Battle);
    }
    for (const auto& V : O->GetArrayField(TEXT("encounters")))
    {
        const Obj E = V->AsObject(); FMemoriaChapterEncounter Encounter;
        Encounter.Name = Str(E, TEXT("name")); Encounter.Hp = E->GetIntegerField(TEXT("hp")); Encounter.Atk = E->GetIntegerField(TEXT("atk"));
        Encounter.bVoid = E->GetBoolField(TEXT("is_void"));
        S.Encounters.Add(Encounter);
    }
    if (const TArray<TSharedPtr<FJsonValue>>* Range = nullptr; O->TryGetArrayField(TEXT("encounter_range"), Range) && Range && Range->Num() == 2)
    { S.EncounterMin = (*Range)[0]->AsNumber(); S.EncounterMax = (*Range)[1]->AsNumber(); }
    if (const TArray<TSharedPtr<FJsonValue>>* Props = nullptr; O->TryGetArrayField(TEXT("decorations"), Props) && Props)
        for (const auto& V : *Props)
        {
            const Obj D = V->AsObject(); FMemoriaChapterDecoration Decoration;
            Decoration.Kind = Str(D, TEXT("kind")); D->TryGetBoolField(TEXT("light"), Decoration.bLight);
            Decoration.Origin = VecField(D, TEXT("origin")); Decoration.Size = VecField(D, TEXT("size"));
            Decoration.Color = Col(D, TEXT("color"), FLinearColor::White);
            D->TryGetNumberField(TEXT("rotation"), Decoration.Rotation); D->TryGetNumberField(TEXT("energy"), Decoration.Energy);
            D->TryGetNumberField(TEXT("scale"), Decoration.Scale);
            S.Decorations.Add(Decoration);
        }
    if (const TArray<TSharedPtr<FJsonValue>>* Npcs = nullptr; O->TryGetArrayField(TEXT("ambient_npcs"), Npcs) && Npcs)
        for (const auto& V : *Npcs) { const Obj N = V->AsObject(); S.AmbientNpcs.Add({VecField(N, TEXT("position")), Str(N, TEXT("preset"))}); }
    S.AmbientNpcsGate = Str(O, TEXT("ambient_npcs_gate"));
    return S;
}
FString Pascal(const FString& Snake)
{
    TArray<FString> Parts; Snake.ParseIntoArray(Parts, TEXT("_"));
    FString Out; for (const FString& P : Parts) Out += P.Left(1).ToUpper() + P.Mid(1);
    return Out;
}
}
namespace MemoriaChapterMaps
{
const FMemoriaChapterMapSpec* Find(const FString& Map)
{
    static TMap<FString, FMemoriaChapterMapSpec> Specs = []
    {
        TMap<FString, FMemoriaChapterMapSpec> Out;
        for (const auto& Source : MemoriaChapterMapSources) Out.Add(Source.Key, Parse(Source.Value));
        return Out;
    }();
    const FMemoriaChapterMapSpec* Spec = Specs.Find(Map);
    return Spec && !Spec->Map.IsEmpty() && Spec->Tiles.Num() == Spec->Width * Spec->Height ? Spec : nullptr;
}
// The small set of authored strings the chapter maps raise outside dialogue, in Korean. Titles follow
// game_manager.gd's RUNTIME_TEXT_KO; place names follow the map scripts' headers.
FString Korean(const FString& En)
{
    static const TMap<FString, FString> Table = {
        {TEXT("Obtained: Blank Book"), TEXT("획득: 백서")},
        {TEXT("The Belt"), TEXT("벨트")}, {TEXT("Weight of Pages"), TEXT("페이지의 무게")},
        {TEXT("Drift"), TEXT("드리프트")}, {TEXT("The architecture crumbles"), TEXT("무너져 내리는 구조")},
        {TEXT("The Classifier"), TEXT("분류자")}, {TEXT("The Seam"), TEXT("심")},
        {TEXT("Next: Chapter 6, The Seam"), TEXT("다음 여정: 6장, 심")},
        {TEXT("drift_shelter"), TEXT("드리프트 셸터")}, {TEXT("belt_waystation"), TEXT("벨트 중간역")},
        // S371: the Seam (최종본 6-7화).
        {TEXT("the_seam"), TEXT("더 심")}, {TEXT("Between what was and what will be"), TEXT("있던 것과 올 것 사이")},
        {TEXT("The Ridge Road"), TEXT("능선길")},
        // S372: the listening forest (최종본 8-10화).
        {TEXT("forgotten_forest"), TEXT("듣는 숲")}, {TEXT("The Listening Forest"), TEXT("듣는 숲")},
        {TEXT("Voices of those who passed, and of those who are not here"), TEXT("지나간 사람들, 여기 없는 사람들의 목소리")},
        {TEXT("Between the Two Stone Ridges"), TEXT("두 돌 능선 사이")},
        // S373: the Crumbling Coast and Blank 7 (최종본 11-12화).
        {TEXT("crumbling_coast"), TEXT("붕괴의 해안")}, {TEXT("The Crumbling Coast"), TEXT("붕괴의 해안")},
        {TEXT("What fear used to do"), TEXT("공포가 하던 일")}, {TEXT("The Crossroads"), TEXT("갈림길")},
        {TEXT("Lost: fear"), TEXT("잃음: 공포")}, {TEXT("Returned: a line of the lullaby"), TEXT("돌아옴: 자장가 한 줄")},
        // S374: Haren's clearing (최종본 13-14화).
        {TEXT("haren_clearing"), TEXT("하렌의 공터")}, {TEXT("Haren's Clearing"), TEXT("하렌의 공터")}, {TEXT("Haren Clearing"), TEXT("하렌의 공터")},
        {TEXT("This isn't memory. It's arithmetic."), TEXT("이건 기억이 아니야. 이건 산수야.")}, {TEXT("The Dry Channel"), TEXT("마른 수로")},
        // S382: Verdan again (최종본 21화).
        {TEXT("verdan_return"), TEXT("다시 베르단")}, {TEXT("Verdan, Again"), TEXT("다시 베르단")}, {TEXT("Filed"), TEXT("파일됨")},
        {TEXT("The Old Furrows"), TEXT("묵은 고랑")}, {TEXT("Parted: Sable"), TEXT("작별: 세이블")}, {TEXT("Malet: filed"), TEXT("말레트: 파일됨")},
        {TEXT("North-west. When it gets light, we go."), TEXT("북서쪽. 밝아지면 가 보자.")},
        {TEXT("Cord from the frame to the door, and red wax where the cords cross. The Bureau seal. No sound below."), TEXT("문틀에서 문으로 이어진 끈, 두 줄이 겹친 곳의 붉은 밀랍. 관리국의 인장. 아래에서는 아무 소리도 없다.")},
        {TEXT("It used to be loud even at this hour."), TEXT("전에는 이 시간에도 시끄러웠어.")},
        // S379: the basin waystation (최종본 19-20화).
        {TEXT("basin_station"), TEXT("분지의 역참")}, {TEXT("The Basin Waystation"), TEXT("분지의 역참")},
        {TEXT("The grey pen"), TEXT("회색 우리")}, {TEXT("The Sheep Pen"), TEXT("양 우리")},
        {TEXT("The ration window: blankers"), TEXT("배급 창구: 블랭커")},
        {TEXT("What to do at the next window, he had not yet chosen."), TEXT("다음 창구 앞에서 무엇을 할지는 아직 고르지 않았다.")},
        {TEXT("The flag on the signal tower hangs at half-mast. Not mourning. The wind holds south-south-west."), TEXT("신호탑의 깃발이 기둥 중간에 걸려 있다. 애도가 아니다. 바람은 남남서풍이다.")},
        {TEXT("I didn't touch my coat there."), TEXT("거기서 코트에 손 안 댔어.")},
        // S377: the roofless farmhouses (최종본 17-18화).
        {TEXT("roofless_farms"), TEXT("지붕 없는 농가")}, {TEXT("The Roofless Farmhouses"), TEXT("지붕 없는 농가")},
        {TEXT("The reader"), TEXT("읽는 자")}, {TEXT("The Second Farmhouse"), TEXT("두 번째 농가")},
        {TEXT("The blank book: the relay"), TEXT("백 노트: 중계")}, {TEXT("A name: Fel"), TEXT("이름 하나: 펠")},
        {TEXT("Before asking who, he would first tell her he had heard."), TEXT("누구냐고 묻기 전에, 자기가 들었다는 말부터 해야 했다.")},
        {TEXT("A grey leather glove in the ditch, its cuff open. No one has come back for it."), TEXT("골에 회색 가죽 장갑 하나. 손목이 벌어져 있다. 찾으러 온 사람은 없다.")},
        {TEXT("It's nothing."), TEXT("아무것도 아니야.")},
        // S375: the Seam, leaving (최종본 15-16화).
        {TEXT("seam_return"), TEXT("떠나는 더 심")}, {TEXT("The Seam, Leaving"), TEXT("떠나는 더 심")},
        {TEXT("What the hand remembers"), TEXT("손이 기억하는 것")}, {TEXT("The Crossroads below the Ridge"), TEXT("능선 아래 교차로")},
        {TEXT("Two halves of one cradle song"), TEXT("한 요람가의 두 반쪽")}, {TEXT("Gained: anger"), TEXT("얻음: 분노")},
        {TEXT("Three more places like that before the Belt road."), TEXT("벨트 도로까지 저런 곳이 셋 더 있다.")},
        {TEXT("A bowl left at a door. It did not fit in the basket."), TEXT("문 앞에 남은 그릇 하나. 바구니에 들어가지 못했다.")},
        {TEXT("Still."), TEXT("지금도.")}, {TEXT("I wanted to stay a little longer."), TEXT("난 조금 더 있고 싶었어.")},
        {TEXT("Don't turn toward the sounds."), TEXT("소리 쪽으로 고개를 돌리지 마.")}, {TEXT("Nine years, and still."), TEXT("아홉 해가 지났는데.")}, {TEXT("I'm watching the same thing you are."), TEXT("네가 보는 걸 나도 보고 있어.")},
        {TEXT("A new line in the blank book"), TEXT("백 노트에 새 줄")}, {TEXT("Returned: six notes"), TEXT("돌아옴: 여섯 음")},
        {TEXT("In the morning, back to the Seam. At least ask her to her face."), TEXT("아침이 되면 더 심으로. 적어도 그 사람 앞에서 물어야지.")},
        {TEXT("The cut face is pale and still damp. Fine strokes lie between the rings, deeper than a finger could rub away."),
         TEXT("잘린 면은 희고 아직 젖어 있다. 나이테 사이에 가는 획들이 놓여 있다. 손가락으로 문질러도 지워지지 않을 깊이다.")},
        {TEXT("Before the river the road forks twice more. To the north the rain has stopped."), TEXT("강 전까지 길은 두 번 더 갈라진다. 북쪽에서는 비가 그쳤다.")},
        {TEXT("Where the grass ends, every blade is cut along one line. Nine years, and the line has not moved."),
         TEXT("풀이 끝나는 자리에서 줄기들이 한 줄로 반듯하게 끊겨 있다. 아홉 해가 지났는데도 그 선은 그대로다.")}, {TEXT("Obtained: Echo shell"), TEXT("획득: 에코 셸")},
        {TEXT("Under the slanted slab, the echo shell says the counting is wrong."), TEXT("기울어진 돌판 아래에서, 에코 셸이 셈이 틀렸다고 말한다.")},
        {TEXT("The roots pulse faintly. Not with sap: with residue. These trees are digesting someone's childhood."),
         TEXT("뿌리가 희미하게 맥박친다. 수액이 아니라 잔존으로. 이 나무들은 누군가의 어린 시절을 삭이고 있다.")},
        {TEXT("Scratches in the bark. Someone tried to carve their name. The letters stop halfway: they forgot the rest."),
         TEXT("나무껍질의 긁힌 자국. 누군가 자기 이름을 새기려 했다. 글자는 절반에서 멈췄다. 나머지를 잊은 것이다.")}, {TEXT("Obtained: Spiralwave shell"), TEXT("획득: 나선파 조개")},
        {TEXT("Obtained: Sable's bark map"), TEXT("획득: 세이블의 나무껍질 지도")},
        {TEXT("Somewhere north-north-east, the hole in the bark is waiting."), TEXT("북북동 어딘가에서, 나무껍질의 구멍이 가리키는 곳이 기다린다.")},
        {TEXT("A plank bridge over the brook. The pebbles show through, and the pattern of the water over them never stays the same."),
         TEXT("개울 위의 판자 다리. 바닥의 자갈까지 보이고, 그 위를 지나는 물의 무늬는 계속 바뀐다.")},
        {TEXT("A faded Bureau sign: 'RELAY STATION 14, All combustion events must be reported within 72 hours.'"),
         TEXT("빛바랜 관리국 표지판: '중계소 14, 모든 연소는 72시간 이내에 보고할 것.'")},
        {TEXT("Warm ashes. Someone camped here recently, the fire pit is lined with Bureau-issue kindling."),
         TEXT("아직 따뜻한 재. 최근 누군가 이곳에서 야영했다. 화덕에는 관리국 보급 불쏘시개가 깔려 있다.")},
        {TEXT("Scratched into the concrete: 'WE REMEMBER.' Below it, in different handwriting: 'For how long?'"),
         TEXT("콘크리트에 긁어 새긴 글씨: '우리는 기억한다.' 그 아래, 다른 필체로: '언제까지?'")}};
    // S370: Korean place and chapter names as the final manuscript has them (셈터, 무너진 돌벽, 더 심).
    const FString* Found = Table.Find(En); return Found ? MemoriaManuscript::Canon(*Found) : En;
}
FString GroupAsset(const FMemoriaChapterMapSpec& Spec, const FString& Group) { return TEXT("DA_Field_") + Spec.AssetPrefix + Pascal(Group); }
FString LevelPath(const FString& Map) { return TEXT("/Game/Memoria/Maps/L_") + Pascal(Map); }
FString MapFromLevel(const FString& LevelName)
{
    for (const auto& Source : MemoriaChapterMapSources)
        if (LevelName.EndsWith(TEXT("L_") + Pascal(Source.Key))) return Source.Key;
    return FString();
}
}
