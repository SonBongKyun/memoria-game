#include "Narrative/MemoriaManuscript.h"
#include "Narrative/MemoriaNarrativeData.h"
#include "Run/MemoriaRunTypes.h"

// S371: Chapter 6, the Seam, as 최종본 6화 「더 심」 and 7화 「세이블」 tell it (final-1, approved 2026-09-25).
// S372: Chapter 7, the listening forest, as 8화 「듣는 숲」, 9화 「빌려온 입」 and 10화 「대가」 tell it.
// Lines are the manuscript's, shortened to a scene's length; nothing here is invented beyond joining them.
// Sable is blind (canon 2026-07-03): she hears and touches, she never looks.
namespace
{
struct FLine { const TCHAR* Speaker; const TCHAR* Portrait; const TCHAR* Cg; const TCHAR* En; const TCHAR* Ko; };
const TCHAR* FirstLight = TEXT("res://assets/cg/generated/story_ch5_seam_first_light.png");
const TCHAR* Streets = TEXT("res://assets/cg/generated/chapter_expansion/ch06_seam_living_streets_v1.png");
const TCHAR* ShellVoices = TEXT("res://assets/cg/generated/chapter_expansion/ch07_echo_shell_voices_v1.png");
const TCHAR* Splash = TEXT("res://assets/cg/generated/chapter_splash_the_seam.png");
// S372: the listening forest. Canon art only: no picture with Tobias or a fourth companion inside the forest.
const TCHAR* Ridgeline = TEXT("res://assets/cg/generated/story_ch7_crossing_the_ridgeline.png");
const TCHAR* ForestEntry = TEXT("res://assets/cg/generated/chapter_expansion/ch08_memory_forest_entry_v1.png");
const TCHAR* GhostMother = TEXT("res://assets/cg/generated/story_ch8_ghost_mother.png");
const TCHAR* ForestPath = TEXT("res://assets/cg/generated/chapter_splash_forgotten_forest.png");
// S373: the coast and Blank 7. "No water could be seen": the two walking into grey, flattened land; the city in the
// rock beside them; the still city inside.
const TCHAR* GreyShore = TEXT("res://assets/cg/generated/chapter_expansion/ch09_equalized_waste_v1.png");
const TCHAR* CityInRock = TEXT("res://assets/cg/generated/chapter_expansion/ch05_city_in_bedrock_v1.png");
const TCHAR* CityStill = TEXT("res://assets/cg/game_image/sealed_city_ruins.png");

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
    // S372 Chapter 7, the listening forest: 최종본 8화 「듣는 숲」, 9화 「빌려온 입」, 10화 「대가」.
    {TEXT("ms_forest_ridge"), {
        {nullptr, nullptr, Ridgeline, TEXT("Haren stopped at the last house. The path climbed the valley's east wall, turned, and turned back higher up."), TEXT("하렌은 마지막 집 앞에서 멈췄다. 집 옆으로 난 길은 골짜기의 동쪽 벽을 따라 올라갔다. 조금 위에서 꺾였고, 그보다 높은 곳에서 다시 꺾였다.")},
        {TEXT("Haren"), nullptr, nullptr, TEXT("From up there you'll see down the other side. Take the straight road down."), TEXT("저기까지 올라가면 아래쪽이 보여. 곧게 난 길로 내려가.")},
        {nullptr, nullptr, nullptr, TEXT("Sable was coming up behind them, one hand on the top of the stone wall, small steps where the stones broke off."), TEXT("세이블이 올라오고 있었다. 한 손으로 돌담 윗면을 짚고, 돌이 끊긴 자리에서는 발을 작게 옮겼다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Keep off the deer paths. It isn't only deer that use them. Don't go in because they look easy."), TEXT("사슴길은 피해. 거기로 다니는 게 사슴만은 아니야. 편해 보인다고 들어가지 마.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Follow the road while you can see it. A straight road lasts longest. ...And in the forest, don't turn your head toward a sound. Don't go looking for what it was."), TEXT("보이는 동안은 따라가. 곧은 길이 그나마 오래 남아. ...숲에서는 소리가 난다고 그쪽으로 고개를 돌리지 마. 무슨 소리인지 찾으러 가지도 말고.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("If I go without burning... can I go further than what you said last night?"), TEXT("태우지 않고 가면요. ...어젯밤 말한 것보다 더 갈 수 있어요?")},
        {nullptr, nullptr, nullptr, TEXT("Up ahead, Elia's footsteps stopped."), TEXT("앞서 가던 엘리아의 발소리가 멎었다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("Burning less would be better. How much further, I can't tell you."), TEXT("덜 태우는 편이 낫겠지. 얼마나 더 갈지는 못 말해.")},
        {nullptr, nullptr, nullptr, TEXT("He nodded, then remembered she couldn't see it."), TEXT("아렐은 고개를 끄덕였다가, 그녀가 보지 못한다는 것을 떠올렸다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("All right."), TEXT("알겠어요.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("You heard."), TEXT("들었구나.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Yes."), TEXT("응.")}}},
    {TEXT("ms_forest_enter"), {
        {nullptr, nullptr, ForestEntry, TEXT("Past the ridge the old road ran on through the brush. The branches closed overhead; a root crossed the wheel ruts, and after it no one could tell which had been the ruts."), TEXT("능선을 넘자 덤불 사이로 오래된 도로가 이어졌다. 머리 위로 가지가 겹쳤고, 뿌리가 바퀴 자국을 가로지른 뒤에는 어느 쪽이 바퀴 자국이었는지 찾기 어려웠다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("This is the listening forest."), TEXT("듣는 숲이야.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("It's quiet."), TEXT("조용한데.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("At night there are voices. People who passed through here. And people who aren't here."), TEXT("밤에는 목소리가 난대. 여기를 지나간 사람들. 여기 없는 사람들 것도.")},
        {nullptr, nullptr, nullptr, TEXT("White mushrooms grew in rings along the roots. He began counting his breaths. At two hundred he couldn't tell if the last one was counted. He started again and lost it at forty."), TEXT("나무 밑의 흰 버섯이 뿌리를 따라 둥근 줄을 이루었다. 그는 호흡을 세기 시작했다. 이백까지 갔을 때 방금 들이쉰 것을 셈에 넣었는지 알 수 없었다. 다시 세다가 마흔에서 놓쳤다.")},
        {nullptr, nullptr, nullptr, TEXT("The gap ahead had widened. The two trunks she had turned her shoulder to pass now stood far enough apart to walk through. The mushrooms had moved with them."), TEXT("앞의 틈이 넓어져 있었다. 엘리아가 어깨를 틀고 지나간 두 줄기가 이제는 몸을 틀지 않아도 될 만큼 벌어져 있었다. 버섯도 함께 옮겨져 있었다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Elia. The trees move."), TEXT("엘리아. 나무가 움직여.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I see them. Since the second stream."), TEXT("보고 있어. 두 번째 개울 뒤부터.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Why didn't you say?"), TEXT("그걸 왜 말 안 했어.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("If you keep looking they move faster. Sable said not to stop and study them."), TEXT("계속 쳐다보면 더 빨리 움직인대. 세이블이 멈춰서 살피지 말라고 했어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("She didn't tell me."), TEXT("나한테는 안 했어.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I just did."), TEXT("지금 했잖아.")}}},
    {TEXT("ms_forest_echoes"), {
        {nullptr, nullptr, nullptr, TEXT("First, water dripping, one drop at a time, like a finger on an empty bowl. He kept his head where it was."), TEXT("처음 들린 것은 물 떨어지는 소리였다. 땅에 닿을 때마다 작은 빈 그릇을 두드리는 소리가 났다. 아렐은 고개를 그대로 두었다.")},
        {nullptr, nullptr, nullptr, TEXT("Then cold, from the inside out, though the air on his cheek hadn't changed. His body kept taking up the posture of someone enduring a cold he had never been in."), TEXT("그 뒤에 추위가 왔다. 뺨에 닿는 공기는 그대로였는데 안쪽에서부터 몸이 식었다. 어디에 웅크려 있었는지 알 수 없는 추위를 견디는 자세가 자꾸 몸에 붙었다.")},
        {nullptr, nullptr, nullptr, TEXT("Then the smell of bread, hot from the oven. A low red oven showed between the trees, a wooden peel coming out of it, a woman's hand, hair tied with a green cord."), TEXT("이번에는 빵 냄새가 났다. 나무 사이에 낮은 붉은 화덕이 겹쳐 보였다. 둥근 입구에서 나무주걱이 나오고 있었고, 그것을 쥔 여자의 머리는 초록 끈으로 묶여 있었다.")},
        {nullptr, nullptr, nullptr, TEXT("Arrel didn't know her. The leaves under his boot made a wet sound that didn't belong to the dry floor before the oven, and the peel blurred into a trunk."), TEXT("아렐은 저 여자를 알지 못했다. 그가 밟은 낙엽에서는 젖은 소리가 났다. 화덕 앞의 마른 바닥과 맞지 않았다. 주걱이 흐려졌고, 조금 뒤에는 나무줄기만 보였다.")},
        {nullptr, nullptr, nullptr, TEXT("The spiralwave in his coat pressed against his ribs, still warm. He didn't stop to try it. He didn't want to test it alone here."), TEXT("코트 안의 나선파가 갈비뼈에 닿았다. 조개는 여전히 따뜻했다. 여기서 혼자 멈춰 그것을 시험하고 싶지는 않았다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Do you see it too?"), TEXT("너도 보여?")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("For me it was a song."), TEXT("나한테는 노래였어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("One you know?"), TEXT("아는 노래야?")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("I've never heard it. ...But I know what the next word is. I didn't mean to sing it."), TEXT("처음 들어. ...그런데 다음 말이 뭔지 알아. 부르려고 하지 않았는데.")}}},
    {TEXT("ms_forest_mother"), {
        {nullptr, nullptr, GhostMother, TEXT("A low stone wall by the path, yellow flowers between its stones. Beyond it a woman was calling him."), TEXT("길가에 낮은 돌담이 보였다. 돌 사이에서 노란 꽃이 피어 있었고, 담 너머로 여자가 그를 부르고 있었다.")},
        {nullptr, nullptr, nullptr, TEXT("Not Arrel. Before the name was finished he knew it was his. It was his mother."), TEXT("아렐이 아니었다. 그 이름이 끝나기도 전에 자신을 부른다는 것을 알았다. 어머니였다.")},
        {nullptr, nullptr, nullptr, TEXT("A voice he had not found for months. Now he could hear even the breath she took after his name. He didn't answer. He waited for the next word."), TEXT("몇 달째 떠올리지 못한 목소리였다. 지금은 이름을 부른 뒤 들이마시는 숨까지 들렸다. 대답은 하지 않았다. 다음 말을 기다렸다.")},
        {TEXT("Arrel"), TEXT("arrel_pain"), nullptr, TEXT("Elia. ...Elia."), TEXT("엘리아. ...엘리아.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Don't go. Don't hold on to what it gives you."), TEXT("가지 마. 거기서 주는 거, 붙잡지 마.")},
        {TEXT("Arrel"), TEXT("arrel_pain"), nullptr, TEXT("It's already in."), TEXT("벌써 들어왔어.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("I know. Don't try to learn it by heart."), TEXT("알아. 외우려고 하지 마.")},
        {nullptr, nullptr, nullptr, TEXT("The voice called a fourth time, louder. A little longer and he could keep it. Open his eyes and the place with the yellow flowers would go first."), TEXT("목소리가 네 번째로 이름을 불렀다. 조금만 더 들으면 다음에도 떠올릴 수 있을 것 같았다. 지금 눈을 뜨면 노란 꽃이 있는 곳부터 없어질 것 같았다.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Arrel."), TEXT("아렐.")},
        {nullptr, nullptr, nullptr, TEXT("He opened his eyes. He stopped shaping the sound in his mouth and moved his feet. Her eyes had blue in them, red threads at the edges, and dried blood above her lip."), TEXT("그는 눈을 떴다. 입안에서 따라 하려던 소리를 그만두고 발을 옮겼다. 엘리아의 회색 눈에 푸른빛이 올라와 있었고, 흰자 끝에는 붉은 가닥이, 윗입술 위로 피가 가늘게 말라 있었다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("It keeps coming into you too?"), TEXT("너한테도 계속 들어와?")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Since the second stream."), TEXT("두 번째 개울부터.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Any of it yours?"), TEXT("네 것도 있어?")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Not one."), TEXT("하나도.")},
        {nullptr, nullptr, nullptr, TEXT("At dusk Kairos came down into the Seam. Sable sat on a low stone step carving a spoon; the knife stopped when he halted three paces off."), TEXT("황혼에 카이로스가 더 심으로 내려왔다. 세이블은 집 앞의 낮은 돌턱에 앉아 숟가락을 깎고 있었다. 카이로스가 세 걸음 앞에 멈추자 칼질이 멎었다.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("You're late."), TEXT("늦었군.")},
        {TEXT("Kairos"), TEXT("kairos_neutral"), nullptr, TEXT("They've gone?"), TEXT("떠났습니까.")},
        {TEXT("Sable"), TEXT("sable_calm"), nullptr, TEXT("At noon. By the east road. ...Going to catch them?"), TEXT("정오에. 동쪽 길로. ...붙잡으러 가나.")},
        {TEXT("Kairos"), TEXT("kairos_neutral"), nullptr, TEXT("I'll ask him. I don't intend to hurry."), TEXT("그 사람에게 물을 겁니다. 서두를 생각은 없습니다.")},
        {nullptr, nullptr, nullptr, TEXT("At the foot of the east climb he opened the red notebook and added a line. Observed. Did not intervene."), TEXT("동쪽 오르막에 이르러 그는 붉은 노트를 다시 폈다. 한 줄을 더했다. 관찰함. 개입하지 않음.")}}},
    {TEXT("ms_forest_lost_hour"), {
        {nullptr, nullptr, ForestPath, TEXT("He couldn't tell where the fifth hour had ended. He stood in a small clearing; his boots were wet to the ankle. He had never put his feet in the water."), TEXT("다섯 번째 시간이 어디서 끝났는지 알 수 없었다. 아렐은 작은 공터에 서 있었고, 부츠의 발목까지 젖어 있었다. 물속에 발을 넣은 적은 없었다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Did we cross another stream?"), TEXT("개울을 또 건넜어?")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("We must have. ...I don't have the middle either."), TEXT("건넜나 봐. ...나도 중간이 없어.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("The mushrooms are gone."), TEXT("버섯이 없어.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("I saw them go. Before the clearing. I thought it was the fifth hour by then."), TEXT("없어지는 건 봤어. 공터에 오기 전에. 그쯤이면 다섯 시간째라고 생각했는데.")},
        {nullptr, nullptr, nullptr, TEXT("He took the lead and chose the gaps with the fewest roots. He didn't call it the road. She followed close enough to stop the moment he turned."), TEXT("아렐은 그녀를 지나 앞으로 갔다. 뿌리가 적은 틈을 골랐다. 그것이 도로라고 말하지는 않았다. 엘리아는 그가 돌아보면 바로 멈출 만큼 가까이 따라왔다.")},
        {nullptr, nullptr, nullptr, TEXT("\"Arrel.\" Elia's voice, low, the end lifting softly, the way she called him when she didn't want to startle him."), TEXT("\"아렐.\" 엘리아의 목소리였다. 조금 낮고, 끝이 부드럽게 올라갔다. 놀라게 하지 않고 그를 돌아보게 할 때 쓰는 억양이었다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("What."), TEXT("뭐.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("What."), TEXT("뭐.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("You just called me."), TEXT("방금 불렀잖아.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("You called. My name."), TEXT("네가 불렀어. 내 이름을.")},
        {nullptr, nullptr, nullptr, TEXT("Between the two trees ahead, something drew a breath. Arrel drew his sword. There was a shape where the trunk should be, a hollow the shape of a shoulder."), TEXT("앞의 두 나무 사이에서 숨을 들이마시는 소리가 났다. 아렐은 검을 뽑았다. 줄기의 가운데가 보이지 않았다. 사람의 어깨처럼 벌어진 자리에서만 껍질의 무늬가 끊겼다.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Don't cut it. That's what it's waiting for. Hit it and it'll know where your blade is."), TEXT("베지 마. 그걸 기다리고 있어. 맞히면 네 칼이 어디 있는지 알게 돼.")},
        {TEXT("Arrel"), TEXT("arrel_determined"), nullptr, TEXT("I have to burn."), TEXT("태워야겠어.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("How much? ...Make it a small one."), TEXT("얼마나. ...작은 걸로 해.")}}},
    {TEXT("ms_forest_escape"), {
        {nullptr, nullptr, ForestPath, TEXT("He had burned a morning: a clay cup on a windowsill, thin ice on the water, cracking from the edge under his thumbnail. The blade had gone cold, not blue, and the shape had folded inward."), TEXT("그는 한 아침을 태웠다. 창틀 위의 찰흙잔, 물 위의 얇은 얼음, 엄지손톱을 대면 가장자리부터 가던 금. 칼날에 푸른 불 대신 차가움이 흘렀고, 형상의 어깨가 안쪽으로 접혔다.")},
        {TEXT("Arrel"), TEXT("arrel_determined"), nullptr, TEXT("Run."), TEXT("뛰어.")},
        {nullptr, nullptr, nullptr, TEXT("Where Elia ran there were trunks she didn't avoid. Hot iron, salt on his tongue, a small cold hand in his left hand. He opened his fingers one by one, and it slid away."), TEXT("엘리아가 지나간 곳에 줄기가 있었다. 달궈진 쇠 냄새, 혀에 붙은 소금기, 왼손을 쥔 작은 손. 그는 손가락을 하나씩 폈고, 그 손은 새끼손가락 끝에 걸렸다가 미끄러져 나갔다.")},
        {nullptr, nullptr, nullptr, TEXT("The trees grew lower. Light that wasn't green fell on her shoulder. A slope, grey grass, a low basin with two wheel ruts. The sun was gone behind cloud."), TEXT("나무가 낮아졌다. 엘리아의 어깨에 초록이 아닌 빛이 닿았다. 비탈이었다. 회색 풀이 자라고, 땅은 낮은 분지로 이어졌다. 바닥에는 바퀴가 지나간 두 줄이 보였다.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Your nose."), TEXT("코. ...네 코.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("What did you burn?"), TEXT("뭘 태웠어.")},
        {TEXT("Arrel"), TEXT("arrel_pain"), nullptr, TEXT("A morning. My room."), TEXT("아침 하나. 내 방이었어.")},
        {nullptr, nullptr, nullptr, TEXT("He tried again. The cup was still there. Which hand had held it, where he had set it down after. The burn had gone somewhere else."), TEXT("다시 해 보았다. 잔은 여전히 거기 있었다. 어느 쪽 손으로 들었는지도, 마시고 나서 어디에 내려놓았는지도 떠올랐다. 태우고 난 자리는 거기서 떨어진 곳에 있었다.")},
        {TEXT("Arrel"), TEXT("arrel_pain"), nullptr, TEXT("It's still there. Down to the ice. ...I think I used something else. What I chose is still here."), TEXT("아직 있어. 얼음까지. ...다른 걸 쓴 것 같아. 고른 건 그대로인데.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("Do you know which?"), TEXT("어떤 건지 알겠어?")},
        {TEXT("Arrel"), TEXT("arrel_pain"), nullptr, TEXT("No."), TEXT("아니.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Sable told me of a place where the ground folds. If we lose the road, go down that way. South. Two long stone ridges; go between them."), TEXT("세이블이 말한 곳이 있어. 땅이 접히는 곳. 길을 놓치면 그쪽으로 내려가랬어. 남쪽. 긴 돌 능선 두 개가 나올 거래. 그 사이로 가면 돼.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("You heard a lot from her."), TEXT("그 사람한테 많이 들었네.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Yes."), TEXT("응.")}}},
    {TEXT("ms_forest_valley"), {
        {nullptr, nullptr, ForestPath, TEXT("The ground sank into a long fold. Stone rose to the shoulder, then overhead, and the same stone ridged the other side. Where the gully widened, a slab had broken from the west ridge and hung at a slant."), TEXT("땅이 길게 꺼져 있었다. 한쪽 벽이 어깨까지, 이어서 머리 위까지 올라왔고, 맞은편에도 같은 빛깔의 돌 능선이 이어졌다. 골이 넓어지는 자리에서 서쪽 능선의 돌이 판째로 떨어져 나와 비스듬히 걸려 있었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Here. ...I've never been. She said it looked like this."), TEXT("여기야. ...와 본 적은 없어. 이렇게 생겼다고 했어.")},
        {nullptr, nullptr, nullptr, TEXT("Under the slab she broke the last of the bread and gave him the bigger half. He wrung out his socks and laid them on the stone. They wouldn't dry soon."), TEXT("돌판 아래에서 엘리아가 남은 빵을 둘로 나누어 큰 쪽을 내밀었다. 아렐은 양말을 짜서 돌 위에 펴 놓았다. 곧 마를 것 같지는 않았다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Nothing about people who came through that forest?"), TEXT("그 숲을 지나온 사람 이야기는 없었어?")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Only how many went in. Forty-one in ten years. The seventeen were the ones who went for the city."), TEXT("들어간 사람 수만 들었어. 지난 십 년 동안 마흔한 명. 열일곱은 도시를 찾으러 간 사람들이고.")},
        {nullptr, nullptr, nullptr, TEXT("A stone rolled on the east ridge, struck rock once, twice, and stopped. No one showed against the sky."), TEXT("동쪽 능선 위에서 돌이 굴렀다. 바위에 부딪히고 한 번 더 부딪힌 뒤 멎었다. 거기 서 있는 사람은 보이지 않았다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("In the forest, when my name was called. It wasn't just your voice. It was the way you call me when you don't want me to turn too fast."), TEXT("숲에서 내 이름을 부른 거. 그냥 목소리만 같은 게 아니었어. 네가 나를 갑자기 돌아보게 하기 싫을 때. 그때처럼 불렀어.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("You lift the end a little. And you wait once, even if no one answers. ...I didn't learn it from you. It's the way I learned."), TEXT("마지막을 조금 올리잖아. 대답 안 해도 한 번 기다리고. ...나한테서 배운 게 아니야. 내가 배운 방식이었으니까.")},
        {nullptr, nullptr, nullptr, TEXT("A second stone, closer. Not rolling: set down on another stone. On the inside of her left hand there was a small mark he had never seen, like a letter no one could read."), TEXT("돌이 하나 더 굴렀다. 아까보다 가까웠고, 돌 하나를 다른 돌 위에 내려놓은 듯한 소리였다. 엘리아의 왼손 안쪽에 그가 본 적 없는 작은 표식이 있었다. 글자 하나처럼 보였지만 어느 글자인지는 알 수 없었다.")},
        {nullptr, nullptr, nullptr, TEXT("Her bag trembled on the stone. She took out a shell the size of a fist, pale, its outside cut with small marks. Bigger than Sable's spiralwave."), TEXT("가방이 돌바닥에서 작게 떨렸다. 엘리아가 꺼낸 것은 주먹만 한 조개였다. 창백하게 감긴 껍질의 바깥에는 작은 표식들이 새겨져 있었다. 세이블에게 받은 나선파보다 컸다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("It listens. To the things that use my voice when I'm not speaking. ...You keep it. It's going somewhere there are no voices."), TEXT("듣는 거. 내 목소리를 쓰는 것들을. 내가 말하고 있지 않을 때. ...네가 가지고 있어. 이건 목소리가 없는 곳으로 가는 거야.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Tonight, the voice is mine."), TEXT("오늘 밤 목소리는 내 거니까.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("What's it called?"), TEXT("이건 뭐라고 불러.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("An echo shell."), TEXT("에코 셸.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Patience. Waiting when you look for a word. Not saying it first even when I know it. ...And the ferns. I was touching them someone else's way. I can't tell which is mine."), TEXT("인내. 네가 단어를 찾을 때 기다리는 거. 다음 말을 알 것 같아도 먼저 말하지 않는 거. ...그리고 고사리. 내가 만지고 있는데, 내 방식이 아니었어. 나는 그게 안 돼.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Elia. How much of you is you?"), TEXT("엘리아. 너 중에 너인 게 얼마나 되는데.")},
        {nullptr, nullptr, nullptr, TEXT("She didn't answer. In the dark he went looking for what the burn had taken. The place was small. The word child came. No face. A child-shaped absence; that was as far as he could know."), TEXT("그녀는 대답하지 않았다. 어둠 속에서 아렐은 태운 자리를 더듬었다. 그 자리는 작았다. 아이라는 말이 떠올랐다. 얼굴은 보이지 않았다. 아이 형태의 부재였다. 거기까지밖에 알 수 없었다.")},
        {nullptr, nullptr, nullptr, TEXT("The shell pushed against his thigh. In his palm a thin sound became words. The counting was wrong, it said. Wrong from the seventeenth."), TEXT("왼쪽 허벅지에서 조개가 밀렸다. 손바닥 안의 가는 소리가 말이 되었다. 세는 것이 틀렸다고 했다. 열이레째부터 셈이 틀렸다고 했다.")},
        {nullptr, nullptr, nullptr, TEXT("Then his name. Not the one Elia gave him: his own, with his mother's voice on it. He opened his mouth. The first sound would not come."), TEXT("그다음은 그의 이름이었다. 엘리아가 지어 준 이름이 아니었다. 어머니가 부르던 소리가 붙어 있는 자기 이름이었다. 아렐은 입을 벌렸다. 첫 소리가 나오지 않았다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("It said the counting is wrong. ...That's all."), TEXT("세는 게 틀렸다고 했어. ...그게 다야.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("All right."), TEXT("알겠어.")},
        {nullptr, nullptr, nullptr, TEXT("Toward dawn, with her asleep, he put the shell to his ear. \"Say it again.\" There was no answer. The shell was lukewarm. He didn't take it from his ear."), TEXT("새벽 가까이, 엘리아가 잠든 뒤 그는 조개를 귀에 가져갔다. \"다시 말해.\" 대답은 없었다. 조개는 미지근했다. 아렐은 귀에서 떼지 않았다.")}}},
    // S373 Chapter 8, the Crumbling Coast and Blank 7: 최종본 11화 「공포가 하던 일」, 12화 「편집자」.
    {TEXT("ms_coast_morning"), {
        {nullptr, nullptr, GreyShore, TEXT("His arm went numb and he lowered the shell. No more words came out of it."), TEXT("팔이 저려 와 아렐은 조개를 내렸다. 조개에서는 더 말이 나오지 않았다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Did it speak again?"), TEXT("또 말했어?")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("No."), TEXT("아니.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("East."), TEXT("동쪽.")},
        {nullptr, nullptr, nullptr, TEXT("The cold from the burn in the forest was still inside his right hand. Open the hand and it stayed."), TEXT("숲에서 연소할 때 생긴 차가움이 손 안쪽에 남아 있었다. 자루에서 손을 떼어 펴도 없어지지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("He looked at her shoulder. The name he hadn't said in the night came to mind: no sound to it, only that he had heard it. And the seventeenth."), TEXT("아렐은 엘리아의 어깨를 보았다. 밤에 말하지 않은 이름이 떠올랐다. 이름의 소리는 없고, 그것을 들었다는 사실만 있었다. 열이레째라는 말도 그대로 남아 있었다.")},
        {nullptr, nullptr, nullptr, TEXT("She walked on, looking ahead. He added nothing."), TEXT("그녀는 앞을 보고 걸었다. 그는 아무것도 덧붙이지 않았다.")}}},
    {TEXT("ms_coast_shore"), {
        {nullptr, nullptr, nullptr, TEXT("The last grass ended as if along a rule. After it, gravel. One blade reached over the line, cut clean where the gravel began."), TEXT("마지막 풀들이 줄을 맞춘 듯 끝나 있었다. 그 앞에서부터 자갈이었다. 한 줄기는 자갈이 시작되는 선에서 반듯하게 끊긴 채 뻗어 있었다.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Nine years, and still."), TEXT("아홉 해가 지났는데.")},
        {nullptr, nullptr, nullptr, TEXT("On the Crumbling Coast there was no water to be seen. Gravel where the waves should reach. No wind. The air had almost no smell."), TEXT("붕괴의 해안에는 물이 보이지 않았다. 물결이 닿아야 할 자리에도 같은 돌들이 있었다. 바람도 없었다. 들이마신 공기에는 냄새가 거의 없었다.")},
        {nullptr, nullptr, nullptr, TEXT("Elia's coat was fading. The brown of the torn shoulder and the lining grew hard to tell apart. Even the dried blood on his hand was going grey."), TEXT("엘리아의 코트가 옅어지고 있었다. 찢어진 어깨의 갈색 천과 안감이 점점 구분하기 어려워졌다. 손등의 마른 피마저 회색에 가까워져 있었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("What do you keep looking at?"), TEXT("뭘 자꾸 봐.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Your eyes. ...It's going out of here too."), TEXT("눈. ...여기도 빠지고 있어.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("I see it."), TEXT("보고 있어.")}}},
    {TEXT("ms_coast_hole"), {
        {nullptr, nullptr, CityInRock, TEXT("Thirty paces ahead the gravel broke off in a circle, five paces across. Its edge would not hold still under the eye. Blue-white light came up from inside."), TEXT("삼십 걸음쯤 앞에 구멍이 있었다. 자갈밭 한가운데가 둥글게 끊겨 있었다. 가장자리를 보려고 하면 선이 조금씩 어긋났다. 안쪽에서는 청백색 빛이 올라왔다.")},
        {nullptr, nullptr, nullptr, TEXT("Inside there was a city. Pale buildings, lit windows, towers. Stars hung over it, though above his head it was afternoon."), TEXT("안에는 도시가 있었다. 창백한 건물 사이로 거리가 뻗어 있었고, 창문에는 빛이 있었다. 도시 위에는 별이 떠 있었다. 고개를 들면 오후의 하늘이었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("This is it."), TEXT("여기야.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Blank 7?"), TEXT("공백 7호?")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("It's still here."), TEXT("아직 있어.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Where you were, nine years ago?"), TEXT("아홉 해 전에 네가 있던 곳?")},
        {nullptr, nullptr, nullptr, TEXT("She nodded. Her hand pressed the gravel until the stones marked her palm, and her eyes stayed on one part of the city."), TEXT("엘리아가 다시 고개를 끄덕였다. 손바닥에 돌이 눌린 자국이 생겼는데도 자세를 바꾸지 않았다. 시선은 줄곧 도시의 한쪽에 머물렀다.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("If we go in, we may not come out the same."), TEXT("들어가면 같은 상태로 못 나올 수도 있어.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("I know."), TEXT("알아.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("Don't just say you know."), TEXT("안다는 말만 하지 마.")},
        {TEXT("Arrel"), TEXT("arrel_determined"), nullptr, TEXT("I'm losing things out here too. I didn't come this far to turn back without looking."), TEXT("밖에서도 계속 잃고 있어. 여기까지 와서 안 보고 돌아가지는 않을 거야.")},
        {TEXT("Arrel"), TEXT("arrel_determined"), nullptr, TEXT("Together."), TEXT("같이.")}}},
    {TEXT("ms_coast_city"), {
        {nullptr, nullptr, CityStill, TEXT("His boots stopped level with the gravel. No ground came up to meet them. The cold in his right hand was gone."), TEXT("부츠는 자갈과 같은 높이에서 멎었다. 아래에 땅이 닿는 감각은 없었다. 손 안쪽에 박혀 있던 차가움이 느껴지지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("A woman stood mid-step with a basket, half a loaf about to fall from her fingertips. He waited for it to fall. It didn't."), TEXT("여자가 서 있었다. 옆구리에 바구니를 낀 채 한 발을 내딛고 있었고, 빵 반쪽이 손끝에서 떨어질 모양이었다. 아렐은 빵이 떨어지기를 기다렸다. 떨어지지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("A man with his arm round a child who had stopped mid-laugh. Two old men at a table, a cup that never reached a mouth. A young woman alone, her face turned up."), TEXT("남자의 팔이 아이의 어깨를 감싸고 있었고, 아이는 웃다가 입을 벌린 채였다. 탁자 앞의 노인이 들어 올린 잔은 입에 닿지 못하고 멈춰 있었다. 젊은 여자가 혼자 얼굴을 위로 든 채 서 있었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("They're warm."), TEXT("따뜻해.")},
        {nullptr, nullptr, nullptr, TEXT("He saw the inside of the basket, the fingers under its handle, the man's jaw from where the child stood, the round mouth of the cup, all at once, without moving. No border between one and the next."), TEXT("바구니의 안쪽이 보였다. 손잡이 아래의 손가락이, 아이의 자리에서 올려다본 남자의 턱이, 탁자 위 잔의 둥근 입구가 한꺼번에 들어왔다. 아이가 보던 것에서 노인의 것으로 넘어가는 경계가 없었다.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("Everything was everything. All at once. ...There was no loneliness."), TEXT("전부가 전부였어. 동시에. ...외로움이 없었어.")},
        {nullptr, nullptr, nullptr, TEXT("Her shoulder went behind the man. A few steps and he would see her again. In the forest he had called her name before his mouth opened. Now nothing happened."), TEXT("엘리아의 어깨가 남자의 몸 뒤로 가려졌다. 몇 걸음이면 다시 볼 수 있었다. 숲에서는 입이 열리기 전에 이미 안쪽이 조여 왔다. 지금은 아무 일도 없었다.")},
        {nullptr, nullptr, nullptr, TEXT("The fear that she might not be there did not come. The last thing that had stayed was gone."), TEXT("그녀가 없을지도 모른다는 공포는 오지 않았다. 계속 남아 있던 마지막 것이 없어져 있었다.")},
        {nullptr, nullptr, nullptr, TEXT("A low melody behind him, and words with it. The river will not flow. His mother used to sing it. A small room, a small window, the lower left corner of the glass broken; lamplight made a little star there."), TEXT("등 뒤에서 낮은 가락이 들렸고, 말이 붙었다. 강물은 흐르지 않으리. 어머니가 불러 주던 것이었다. 작은 방, 작은 창문. 유리의 왼쪽 아래 모서리가 깨져 있어, 등불을 켜면 그 자리에 작은 별 같은 빛이 생겼다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Something came back. ...I won't say it yet."), TEXT("뭔가 돌아왔어. ...지금은 말 안 할래.")},
        {TEXT("Elia"), TEXT("elia_gentle_smile"), nullptr, TEXT("Keep it."), TEXT("가지고 있어.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("It's quiet. Not the sound. ...I can't feel you. The city comes. Yours doesn't."), TEXT("조용해. 소리 말고. ...네가 안 느껴져. 도시는 와. 네 것은 안 와.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("We have to get out."), TEXT("나가야겠어.")},
        {nullptr, nullptr, nullptr, TEXT("She stopped and laid her hand on nothing. It went no further. She leaned on it."), TEXT("엘리아가 아무것도 보이지 않는 곳에 손을 댔다. 손바닥은 더 나아가지 않았다. 그녀는 거기에 몸을 기댔다.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Five years. Walk into a room and I'd feel who had been there first. What the ones who left had left. Even alone. ...Now it's mine."), TEXT("오 년 동안. 방에 들어가면 누가 먼저 와 있었는지 느껴졌어. 나간 사람 것도 남아 있었고. 나 혼자 있어도. ...지금은 내 거야.")},
        {nullptr, nullptr, GreyShore, TEXT("Gravel shifted under his boots. The cold came back into his right hand. Four paces from the hole she sat and held the open water flask without drinking."), TEXT("자갈이 부츠 밑에서 밀렸다. 오른손의 차가움도 돌아왔다. 엘리아가 구멍에서 네 걸음쯤 떨어져 앉아, 마개를 연 물통을 마시지 않고 들고 있었다.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("You still don't come. I know you're here. I can see you."), TEXT("지금도 안 와. 있는 건 알아. 보이니까.")},
        {nullptr, nullptr, nullptr, TEXT("She opened the blank book. There was writing on the first page, set deeper than the paper. She held her left palm beside it: the same mark."), TEXT("엘리아가 백 노트를 펼쳤다. 첫 장에 글씨가 있었다. 획은 종이의 표면보다 안쪽에 있는 것처럼 보였다. 그녀가 왼손바닥을 페이지 곁에 폈다. 같은 모양의 획이었다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("The same."), TEXT("같아.")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("I don't know what letter it is. ...It's quiet. Too quiet."), TEXT("모르겠어. ...조용해. 너무.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Shall we sit a little longer?"), TEXT("조금 더 앉아 있을까.")},
        {nullptr, nullptr, nullptr, TEXT("Thinking of losing her, his chest did not tighten. The fear that was gone was not gone only inside the city."), TEXT("그녀가 없어지는 일을 생각하는 동안에도 가슴은 조여 오지 않았다. 없어진 공포는 도시 안에만 없는 것이 아니었다.")}}},
    {TEXT("ms_coast_tree"), {
        {nullptr, nullptr, nullptr, TEXT("North, on a narrow path people had walked, a young tree. Low on its trunk the bark was paler: a vertical line, a small box open on one side. Cut sitting down."), TEXT("북쪽, 사람들이 다녔던 좁은 길에 어린 나무가 있었다. 줄기 아래쪽에 표식이 있었다. 세로선 아래에 한쪽이 열린 작은 네모. 앉아서 새겼을 만한 자리였다.")},
        {nullptr, nullptr, nullptr, TEXT("The grass beneath was pressed flat, two places side by side where boots had rested. Sitting there, back to the tree, you could watch the coast."), TEXT("나무 밑의 풀이 한쪽으로 눌려 있었고, 부츠를 놓았을 만한 자리가 나란히 있었다. 거기에 앉으면 나무에 등을 대고 해안 쪽을 볼 수 있었다.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("Someone waited. For us to come out."), TEXT("기다렸나 봐. 우리가 나오는 걸.")},
        {nullptr, nullptr, nullptr, TEXT("Under the mark, small writing. I will not stop you yet."), TEXT("표식 아래에 작은 글씨가 있었다. 아직 너를 막지 않겠다.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("Kairos?"), TEXT("카이로스일까.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("The waystation wall was in this hand."), TEXT("역참에서도 이런 글씨였어.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("So he could stop us."), TEXT("막을 수는 있다는 거네.")},
        {TEXT("Elia"), TEXT("elia_neutral"), nullptr, TEXT("How far ahead, do you think?"), TEXT("얼마나 앞서 있을까.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("No idea. Not from this."), TEXT("모르겠어. 이걸로는.")}}},
    {TEXT("ms_coast_chair"), {
        {nullptr, nullptr, GreyShore, TEXT("Where the road forked, a chair stood empty. A flat stone beside it, a closed notebook and a corked ink bottle on the stone."), TEXT("갈림길에 의자 하나가 비어 있었다. 넓적한 돌 위에는 닫힌 노트와 마개를 꽂은 잉크병이 있었다.")},
        {TEXT("Elia"), TEXT("elia_concern"), nullptr, TEXT("Don't draw."), TEXT("뽑지 마.")},
        {nullptr, nullptr, nullptr, TEXT("Beyond her hand, a man was sitting in the chair. A brown coat. He hadn't come in from any side. Arrel couldn't find the moment he sat down."), TEXT("그 너머에 사내가 앉아 있었다. 갈색 코트였다. 돌 옆의 틈도 의자 뒤의 길도 비어 있었다. 서 있는 사람이 앉는 모습까지 떠오르지 않았다.")},
        {TEXT("Arrel"), TEXT("arrel_neutral"), nullptr, TEXT("Kairos?"), TEXT("카이로스?")},
        {TEXT("Brown Coat"), nullptr, nullptr, TEXT("Sit."), TEXT("앉아.")},
        {nullptr, nullptr, nullptr, TEXT("There was one chair. Elia knelt by the stone. Arrel stayed standing. In his chest, nothing happened."), TEXT("의자는 하나였다. 엘리아가 돌 옆에 무릎을 대고 앉았다. 아렐은 서 있었다. 가슴에서는 아무 일도 일어나지 않았다.")},
        {nullptr, nullptr, nullptr, TEXT("The notebook's cover lifted with no finger on it. He turned it to Arrel. The hand of the young tree."), TEXT("노트의 표지가 들렸다. 표지를 들어 올리는 손가락은 없었다. 사내가 노트를 돌려 아렐 쪽으로 밀었다. 해안의 나무에서 읽은 문장과 닮은 필체였다.")},
        {TEXT("Brown Coat"), nullptr, nullptr, TEXT("Read."), TEXT("읽어.")},
        {nullptr, nullptr, nullptr, TEXT("First: the breakfast leaving the waystation. Second: fear. Third: a short line where the small absence from the forest was. Fourth: three words. A shut door. Someone going out of a room."), TEXT("첫째, 역참을 떠나던 아침의 식사. 둘째, 공포. 셋째, 숲을 나온 뒤의 작은 공백이 걸린 짧은 한 줄. 넷째는 세 단어였다. 닫힌 문. 문 너머로 누군가 나가고 있었다.")},
        {nullptr, nullptr, nullptr, TEXT("At the third reading a voice came up: only its beginning, the pitch someone used to call him when he was small. He parted his lips. It was gone."), TEXT("세 번째로 읽었을 때 목소리가 올라왔다. 첫머리만 있었다. 누군가 어린 그를 부를 때 내던 높이였다. 아렐은 입술을 떼었다. 목소리가 없어졌다.")},
        {nullptr, nullptr, nullptr, TEXT("The man said one word. Three syllables, the middle long. Elia's right hand opened her coat and took out the blank book."), TEXT("사내가 한 단어를 말했다. 세 음절이었다. 가운데 소리가 길었다. 엘리아의 오른손이 코트 앞섶을 벌려 백 노트를 꺼냈다.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("I'm not taking it out. ...That isn't me."), TEXT("안 꺼내. ...내가 한 거 아니야.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Stop. ...Stop it."), TEXT("멈춰. ...그만해.")},
        {TEXT("Arrel"), TEXT("arrel_determined"), nullptr, TEXT("What did you just do? ...Look at me."), TEXT("지금 뭘 한 거야. ...이쪽 봐.")},
        {nullptr, nullptr, nullptr, TEXT("He knew how fast he could draw. He didn't know if it was faster than one word."), TEXT("검을 얼마나 빨리 뽑을 수 있는지는 알고 있었다. 저 말 하나보다 빨리 뽑을 수 있는지는 몰랐다.")},
        {TEXT("Elia"), TEXT("elia_worried"), nullptr, TEXT("Then you read it. My hand took it out, so you tell me what you know."), TEXT("그럼 당신이 읽어. 내 손으로 꺼냈으니까, 당신은 뭘 아는지 말해.")},
        {nullptr, nullptr, nullptr, TEXT("The man closed his notebook. Elia's hand shut the blank book too, smoothly. She hadn't meant to."), TEXT("사내가 노트를 덮었다. 엘리아의 오른손도 백 노트의 표지를 들어 매끈하게 덮었다. 그녀가 한 일이 아니었다.")},
        {TEXT("Brown Coat"), nullptr, nullptr, TEXT("Before the river the road forks twice more. To the north the rain has stopped."), TEXT("강 전까지 길은 두 번 더 갈라진다. 북쪽에서는 비가 그쳤다.")},
        {nullptr, nullptr, nullptr, TEXT("He went off down the right-hand road without folding the chair. Over the seat, a hand's width up, a word hung in the air. It lasted one slow breath."), TEXT("사내는 의자도 접지 않고 오른쪽 길로 돌아섰다. 좌석 위, 천에서 한 손너비쯤 떨어진 허공에 한 단어가 있었다. 숨을 천천히 내쉬는 동안 남아 있었다.")},
        {TEXT("Arrel"), TEXT("arrel_pensive"), nullptr, TEXT("Is it still there?"), TEXT("남아 있어?")},
        {TEXT("Elia"), TEXT("elia_sad"), nullptr, TEXT("Only that it was."), TEXT("있었다는 것만.")},
        {nullptr, nullptr, nullptr, TEXT("His thumb caught on the hilt: a small roughness under the familiar groove, where he had been gripping while he watched the man's face. He had not seen the man touch the sword."), TEXT("오른손 엄지가 칼자루의 익숙한 홈 아래에서 걸렸다. 매끈하던 곳에 작은 거칠음이 있었다. 사내가 검에 손을 대는 모습은 보지 못했다.")}}},
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
