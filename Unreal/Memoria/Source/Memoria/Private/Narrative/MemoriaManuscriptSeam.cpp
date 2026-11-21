#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Run/MemoriaRunTypes.h"

// S371: Chapter 6, the Seam, as 최종본 6화 「더 심」 and 7화 「세이블」 tell it (final-1, approved 2026-09-25).
// Lines are the manuscript's, shortened to a scene's length; nothing here is invented beyond joining them.
// Sable is blind (canon 2026-07-03): she hears and touches, she never looks.
namespace
{
struct FLine { const TCHAR* Speaker; const TCHAR* Portrait; const TCHAR* Cg; const TCHAR* En; const TCHAR* Ko; };
const TCHAR* FirstLight = TEXT("res://assets/cg/generated/story_ch5_seam_first_light.png");
const TCHAR* Streets = TEXT("res://assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png");
const TCHAR* ShellVoices = TEXT("res://assets/cg/generated/chapter_expansion/ch07_echo_shell_voices_v1.png");
const TCHAR* Splash = TEXT("res://assets/cg/generated/chapter_splash_the_seam.png");

const TMap<FString, TArray<FLine>>& Scenes()
{
    static const TMap<FString, TArray<FLine>> Values = {
    {TEXT("ms_seam_arrival"), {
        {nullptr, nullptr, FirstLight, TEXT("Arrel smelled the green before he saw it. Wet earth, and the sap of stripped bark."), TEXT("아렐은 초록을 보기 전에 냄새를 맡았다. 젖은 흙 냄새였고, 나무껍질이 벗겨진 자리의 진액 냄새도 섞였다.")},
        {nullptr, nullptr, nullptr, TEXT("The bark of a roadside tree was warm, even on the side the sun never reached."), TEXT("길가 나무의 껍질은 따뜻했다. 햇빛이 닿지 않는 반대편도 그랬다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I think we're here."), TEXT("도착한 것 같아.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("You said you'd never been."), TEXT("와 본 적 없다며.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I haven't. ...Where else could it be?"), TEXT("없어. ...여기 말고 어디겠어.")},
        {nullptr, nullptr, nullptr, TEXT("Follow the colour. He didn't need to take Malet's note out to check."), TEXT("색을 따르게. 말레트의 쪽지를 꺼내 확인할 필요는 없었다.")},
        {nullptr, nullptr, nullptr, TEXT("Elia lifted a fern on her fingers. Her shoulders shook once. Tears fell from under her chin."), TEXT("엘리아가 고사리 잎을 손가락으로 들어 올렸다. 어깨가 한 번 들썩였다. 눈물이 턱 아래로 떨어졌다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Are you hurt?"), TEXT("어디 아파?")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("I'm fine. ...Give me a moment."), TEXT("괜찮아. ...조금만 시간 줘.")},
        {nullptr, nullptr, nullptr, TEXT("She wiped her face with the end of her scarf. When she lowered it, a red line stayed on her upper lip."), TEXT("그녀가 스카프 끝으로 얼굴을 닦았다. 천을 내렸을 때 윗입술에 붉은 선이 남았다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("You're bleeding. ...Since when?"), TEXT("피 나. ...언제부터 그랬어.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Yesterday. While I held your hand. It's nearly stopped."), TEXT("어제. 네 손 잡고 있을 때. 지금은 거의 멎었어.")},
        {nullptr, nullptr, nullptr, TEXT("He poured a little water into her cupped hands. She washed the blood from her lip, and they went on down the road."), TEXT("아렐은 그녀가 오므린 손에 물을 조금 부었다. 그녀가 입술에 남은 피를 씻었고, 둘은 길 아래로 내려갔다.")}}},
    {TEXT("ms_seam_name"), {
        {nullptr, nullptr, Streets, TEXT("Thin smoke rose from the first roof. Here, people lit fires."), TEXT("첫 집의 지붕 위로 가느다란 연기가 올라왔다. 여기서는 불을 피우고 있었다.")},
        {TEXT("Villager"), nullptr, nullptr, TEXT("Came by the ridge road. Drink first. It's in there."), TEXT("능선길에서 왔군. 물부터 마셔. 저 안에 있어.")},
        {nullptr, nullptr, nullptr, TEXT("On the plank bridge over the brook, Arrel went to call to her. The name didn't come."), TEXT("개울 위 판자 다리에서, 아렐은 앞서 걷는 그녀를 부르려 했다. 이름이 나오지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("She had cried in the wood, held his hand yesterday, and walked a few steps ahead of him now. Only the name was missing."), TEXT("조금 전 숲에서 울었고, 어제 그의 손을 잡았고, 지금도 몇 걸음 앞에서 걷는 사람이었다. 입에 올릴 이름만 없었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Arrel?"), TEXT("아렐?")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("I can't find your name."), TEXT("네 이름이 안 나와.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Do you know who I am?"), TEXT("내가 누군지는 알아?")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("I know."), TEXT("알아.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Where are we?"), TEXT("어디야, 여기.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("The Seam."), TEXT("더 심.")},
        {nullptr, nullptr, nullptr, TEXT("She laid her fingers on his sleeve. She didn't tell him to close his eyes. The brook went on."), TEXT("그녀가 그의 소매 위에 손가락을 얹었다. 눈을 감으라는 말은 하지 않았다. 개울 소리가 들렸다.")},
        {nullptr, nullptr, nullptr, TEXT("Elia. The name came back whole, first sound to last."), TEXT("엘리아. 이름은 첫 소리부터 끝까지 한꺼번에 돌아왔다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Elia. ...I have it now."), TEXT("엘리아. ...지금은 알아.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("About half a minute. Anything else?"), TEXT("반 분쯤이었어. 다른 건?")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Not that I can tell."), TEXT("지금은 모르겠어.")}}},
    {TEXT("ms_seam_feren"), {
        {nullptr, nullptr, Streets, TEXT("The water room had a long table. The water was cold, and left no metal on the tongue."), TEXT("물을 마시는 곳에는 긴 탁자가 있었다. 물은 차가웠고, 삼킨 뒤 혀에 금속 맛이 남지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("An old man came out by the hearth. His eyes rested on the sword hilt, then longer on Elia."), TEXT("화덕 옆 문에서 노인이 나왔다. 그의 시선은 칼자루에 잠시 머물렀다가, 엘리아에게서 조금 더 오래 멎었다.")},
        {TEXT("Villager"), nullptr, nullptr, TEXT("Feren. They came by the ridge road."), TEXT("페렌. 능선길로 왔대요.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("We came to see Sable."), TEXT("세이블을 만나러 왔어.")},
        {TEXT("Feren"), nullptr, nullptr, TEXT("Rest first. There's a room inside. Sable can wait till morning."), TEXT("쉬고 나서 이야기해. 안쪽에 방이 있어. 세이블과는 아침에 얘기해도 될 거야.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("I only want to ask the way."), TEXT("길만 물어볼 건데.")},
        {TEXT("Feren"), nullptr, nullptr, TEXT("Then ask the way in the morning."), TEXT("그러면 아침에 길을 물어.")},
        {nullptr, nullptr, nullptr, TEXT("The old man filled his cup again and went back by the hearth door."), TEXT("노인은 아렐의 잔을 다시 채우고 화덕 옆 문으로 돌아갔다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Do you know him?"), TEXT("아는 사람이야?")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I just heard his name. He knew what I am, I think."), TEXT("페렌이라는 건 방금 들었어. 내가 뭔지는 알아본 것 같아.")}}},
    {TEXT("ms_seam_night"), {
        {nullptr, nullptr, Splash, TEXT("Two beds in the inner room. Elia set the blank book on her knees and didn't open it."), TEXT("안쪽 방에는 잠자리 두 개가 있었다. 엘리아는 백 노트를 무릎에 올렸지만 펼치지 않았다.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Can't we rest a little longer? Not just tonight."), TEXT("조금 더 쉬었다 가면 안 될까. 오늘 말고.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("After we ask Sable the way."), TEXT("세이블에게 길을 물어보고.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I'll go and talk to Sable."), TEXT("난 세이블한테 얘기하고 올게.")},
        {nullptr, nullptr, nullptr, TEXT("Voices woke him. The other bed was empty. Elia was talking beyond the door."), TEXT("아렐은 목소리 때문에 눈을 떴다. 맞은편 잠자리는 비어 있었다. 문밖에서 엘리아가 말하고 있었다.")},
        {TEXT("Elia"), nullptr, nullptr, TEXT("Class Seven. The waystation wall said so too."), TEXT("7급이야. 역참 벽에도 그렇게 적혀 있었어.")},
        {TEXT("Sable"), nullptr, nullptr, TEXT("Hold on to someone who keeps going down, and you'll give out first."), TEXT("계속 내려가는 사람을 붙잡고 있으면 네가 먼저 못 견뎌.")},
        {TEXT("Elia"), nullptr, nullptr, TEXT("I know, Sable. But today he couldn't find my name. Half a minute."), TEXT("알아, 세이블. 그런데 오늘 내 이름이 안 나왔어. 반 분 정도.")},
        {TEXT("Sable"), nullptr, nullptr, TEXT("Then that's as far as today went. ...Ask what you came to ask."), TEXT("그럼 오늘은 거기까지 본 거야. ...네가 물으러 온 걸 물어.")},
        {TEXT("Elia"), nullptr, nullptr, TEXT("How long does he have?"), TEXT("걔한테 얼마나 남았어?")},
        // [10] is Sable's answer; it depends on how much he has burned (see BuildScene).
        {TEXT("Sable"), nullptr, nullptr, TEXT("Months. Three. Four, if he holds as he is. Less, if he burns bigger."), TEXT("몇 달. 셋. 지금 정도로 버티면 넷까지 볼 수 있겠지. 더 크게 태우면 그보다 짧고.")},
        {TEXT("Sable"), nullptr, nullptr, TEXT("Burning leaves a sound. What's left in him is the sound I was taught as a girl to fear."), TEXT("연소에는 소리가 남아. 걔한테 남은 건 내가 소녀 때부터 위험하다고 배운 소리야.")},
        {TEXT("Sable"), nullptr, nullptr, TEXT("The body can go on. Walk, lift a sword. But he starts by forgetting why he fights."), TEXT("몸은 더 갈 수 있어. 걷고, 검을 들고. 그런데 자기가 왜 싸우는지부터 잊기 시작해.")},
        {nullptr, nullptr, nullptr, TEXT("He went back to bed and counted. After eleven the next number didn't come. Half a second at most. Twelve."), TEXT("아렐은 잠자리로 돌아와 수를 셌다. 열하나까지 갔을 때 다음 수가 나오지 않았다. 길어야 반 초였다. 열둘.")},
        {nullptr, nullptr, ShellVoices, TEXT("The knock was small. Sable came in. Her clouded white eyes faced where he sat, and didn't follow when he moved."), TEXT("노크는 작았다. 세이블이 들어왔다. 하얗게 흐린 눈은 그가 앉은 곳을 향했지만, 그가 몸을 옮겨도 따라오지 않았다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Take this."), TEXT("이거 가져.")},
        {nullptr, nullptr, nullptr, TEXT("A shell in his palm. A tight spiral, heavier than it looked. It was warm."), TEXT("손바닥에 조개가 놓였다. 둥글게 감긴 나선, 생김새보다 묵직했다. 조개는 따뜻했다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("We call it a spiralwave. It turns up where the ground has thinned. When the land up there goes strange, put it to your ear."), TEXT("나선파라고 불러. 땅이 얇아진 자리에서 가끔 나오지. 위쪽 땅이 이상해지면 귀에 대 봐.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Why can I take it?"), TEXT("왜 나는 가져가도 돼.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("The others weren't carrying what you carry. ...It's given. Sleep, if you can."), TEXT("그 사람들은 네가 지고 있는 걸 지고 있지 않았으니까. ...준 거야. 될 수 있으면 자.")}}},
    {TEXT("ms_seam_morning"), {
        {nullptr, nullptr, Streets, TEXT("In the morning Sable was carving a spoon. A bowl of porridge with black seeds waited on the table."), TEXT("아침에 세이블은 숟가락을 깎고 있었다. 탁자에는 검은 씨앗을 뿌린 죽이 떠 놓여 있었다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("You heard, at the door."), TEXT("문에서 들었지.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Yes. From when you talked about my class."), TEXT("응. 내 등급 얘기할 때부터.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("I heard you come to the door. It was about you. ...I haven't told Elia."), TEXT("네가 문까지 오는 걸 들었어. 네 얘기였으니까. ...엘리아한테 내가 말하지는 않았어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("The seventeen who went for the city. Did they come through here?"), TEXT("도시를 찾으러 간 열일곱 명. 그 사람들도 여기를 거쳤어?")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("They did. None who went that way came back. Not in nine years. ...One of them was my daughter."), TEXT("그래. 그쪽으로 떠난 사람들은 돌아오지 않았어. 아홉 해 동안 아무도. ...그중 하나가 내 딸이었으니까.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("She left at twenty-two. I helped her pack."), TEXT("스물둘에 떠났어. 짐 싸는 걸 도와줬어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("What happened to the city?"), TEXT("그 도시는 어떻게 된 거야.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Edited, nine years ago. Off the maps, the census, the tax rolls. The ground stayed. It went thin."), TEXT("아홉 해 전에 편집당했어. 지도에서, 인구조사와 세금 기록에서도. 땅은 남았어. 얇아졌지.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("North-north-east. Through a stretch no record covers. Two days' walk if the ground holds, four if it doesn't."), TEXT("북북동이야. 기록되지 않은 구간을 지나야 해. 땅이 그대로면 이틀 걸음, 아니면 나흘도 걸려.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("And leave today. Three days before you came, a man passed through asking if I'd met a high-grade burner."), TEXT("그리고 오늘 떠나. 너희가 들어오기 사흘 전에 사내 하나가 지나갔어. 고등급 연소자를 본 적 있느냐고 묻더군.")},
        {nullptr, nullptr, nullptr, TEXT("An axe haft moved by the doorpost. A man of about forty came in, a long-hafted axe against his shoulder."), TEXT("문설주 곁에서 도끼 자루가 움직였다. 마흔쯤 된 사내가 긴 자루 도끼를 어깨에 기대고 들어왔다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Haren. Take these two up the ridge road. As far as the fork."), TEXT("하렌. 둘을 능선길 끝까지 데려다줘. 갈라지기 전까지.")},
        {nullptr, nullptr, nullptr, TEXT("Sable cut three lines and two crosses into a piece of bark, then burned a hole above them with an awl from the hearth."), TEXT("세이블은 나무껍질에 선 셋과 십자 둘을 새기고, 화덕에서 달군 송곳으로 위쪽에 구멍을 하나 뚫었다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("The hole is where you're going. You'll feel the ground change when you're close. ...Take bread. Water too."), TEXT("구멍이 찾는 곳이야. 가까이 가면 땅이 달라지는 걸 느낄 거다. ...빵은 챙겨. 물도.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("I wanted to stay a little longer."), TEXT("난 조금 더 있고 싶었어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("I know."), TEXT("알아.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("I'm not asking you to know."), TEXT("알아 달라는 게 아니야.")}}},
    {TEXT("ms_seam_departure"), {
        {TEXT("Haren"), nullptr, Splash, TEXT("Ready?"), TEXT("됐나?")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Walk well."), TEXT("잘 걸어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("I will."), TEXT("그럴게요.")},
        {nullptr, nullptr, nullptr, TEXT("Haren led off between the houses. The two followed him."), TEXT("하렌이 집들 사이로 앞장섰다. 두 사람은 그 뒤를 따라 걸었다.")},
        {nullptr, nullptr, nullptr, TEXT("Sable opened the small box on the shelf. Seventeen pieces of bark, one kept back each time someone asked the way and left."), TEXT("세이블은 선반의 작은 상자를 열었다. 나무껍질 열일곱 장. 길을 물은 사람들이 떠날 때마다 남겨 둔 것들이었다.")},
        {nullptr, nullptr, nullptr, TEXT("Her fingertip stopped in a deep groove. Besa. She had cut her daughter's name twice."), TEXT("손끝이 깊은 홈에서 멈췄다. 베사. 딸의 이름은 두 번 새겼다.")},
        {nullptr, nullptr, nullptr, TEXT("On a fresh piece she cut a name with the iron point. Arrel. She left the place for the hole untouched."), TEXT("새 껍질 한 장에 쇠촉으로 새겼다. 아렐. 구멍을 낼 자리는 그대로 두었다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Not seventeen."), TEXT("열일곱은 아니야.")}}},
    };
    return Values;
}
}

namespace MemoriaManuscript
{
bool IsScene(const FString& Group) { return Scenes().Contains(Group); }
TArray<FString> SceneGroups() { TArray<FString> Out; Scenes().GetKeys(Out); return Out; }
UMemoriaFieldAsset* BuildScene(UObject* Outer, const FString& Group, int32 Burned)
{
    const auto* Lines = Scenes().Find(Group);
    if (!Lines) return nullptr;
    auto* Asset = NewObject<UMemoriaFieldAsset>(Outer);
    Asset->Definition.Id = Group; Asset->Definition.IndexMappingVersion = 1;
    for (int32 I = 0; I < Lines->Num(); ++I)
    {
        const FLine& L = (*Lines)[I];
        FMemoriaFieldRow Row; Row.Id = FString::Printf(TEXT("field/%s/%d"), *Group, I);
        Row.OriginalIndex = I; Row.StorageIndex = I; Row.EffectPhase = TEXT("gate_then_effects");
        FString En = L.En, Ko = L.Ko;
        // "더 크게 태우면 그보다 짧고": Sable hears more burning in a man who has burned more.
        if (Group == TEXT("ms_seam_night") && I == 10 && Burned >= SableHeavyBurns)
        {
            En = TEXT("Months. Two. His breath was thin already when he came in. Less, if he burns bigger.");
            Ko = TEXT("몇 달. 둘. 들어올 때 숨이 벌써 많이 얇았어. 더 크게 태우면 그보다 짧고.");
        }
        auto& T = Row.Text;
        if (L.Speaker) { T.bHasSpeaker = true; T.Speaker = L.Speaker; T.bHasText = true; T.Text = En; T.bHasTextKo = true; T.TextKo = Ko; }
        else { T.bHasNarrate = true; T.Narrate = En; T.bHasNarrateKo = true; T.NarrateKo = Ko; }
        if (L.Portrait) { Row.Presentation.bHasPortrait = true; Row.Presentation.Portrait = L.Portrait; }
        if (L.Cg) { Row.Presentation.bHasCg = true; Row.Presentation.Cg = L.Cg; }
        Asset->Definition.Rows.Add(Row);
    }
    return Asset;
}
FString RoadOnward(const FString& Map, const FMemoriaRunSnapshot& Run)
{
    // After Chapter 5's classifier the story returns to Drift; its east road now goes on to the Seam.
    if (Map == TEXT("drift_shelter") && (Run.GetFlag(TEXT("canon_ch6_seam_ready")) || Run.GetFlag(TEXT("ch6_arrived")))) return SeamMap;
    return FString();
}
}
