"""S370: emit the C++ table of manuscript line overrides.
Old Korean is read verbatim from the narrative IR; new Korean follows the final manuscript (chapters 1-5).
Usage: py -3 s370_lines.py <lane root> <out.inc>"""
import json, os, sys

ROOT, OUT = sys.argv[1], sys.argv[2]
IR = os.path.join(ROOT, 'docs', 'unreal-migration', 'ir', 'narrative')

# (scene file stem, original index, text field) -> new Korean. Choice keys: (stem, index, 'choice', n).
M = {
# Chapter 1
('ch1_prologue.vn', 11, 'text_ko'): '얼마나 태웠어.',
('ch1_prologue.vn', 12, 'text_ko'): '하나. 작은 거. 없어도 되는 걸로.',
('ch1_prologue.vn', 20, 'text_ko'): '움직이자. 해 지면 더 내려.',
('ch1_after_forest.vn', 0, 'narrate_ko'): '나무 사이가 넓어졌다. 숲 바닥을 덮던 잎이 줄고 회색 풀이 늘었다.',
('ch1_after_forest.vn', 1, 'text_ko'): '그 뒤로는 안 태웠네.',
('ch1_after_forest.vn', 2, 'text_ko'): '알아.',
('ch1_after_forest.vn', 3, 'text_ko'): '세고 있구나.',
('ch1_after_forest.vn', 4, 'text_ko'): '늘 세.',
('ch1_after_forest.vn', 5, 'narrate_ko'): '능선 끝에 어린 나무 한 그루가 서 있었다. 줄기는 곧고 껍질은 매끈했다. 주위의 어느 가지에도 없는 짙은 초록이었다.',
('ch1_after_forest.vn', 6, 'narrate_ko'): '엘리아가 걸음을 멈췄다. 대답을 기다리듯 손가락을 펴서 줄기에 댔다.',
('ch1_after_forest.vn', 7, 'text_ko'): '아직 따뜻해.',
('ch1_after_forest.vn', 8, 'text_ko'): '겨울까지는 못 가겠지.',
('ch1_after_forest.vn', 9, 'text_ko'): '아니. 못 가겠지.',
('ch1_after_forest.vn', 10, 'narrate_ko'): '숲 너머 남쪽 멀리, 관리국의 탑이 구름을 긁고 서 있었다.',
('ch1_after_forest.vn', 11, 'narrate_ko'): '베르단에서 물어야 할 것이 있었다. 지명 대신 붙은 호칭 하나. 관리국이 지워 버린 도시, 지도에는 그 자리에 호수가 그려져 있다고 했다.',
('ch1_after_forest.vn', 12, 'narrate_ko'): '아침이면 벨트 경계. 그다음 날 베르단. 길이 그대로라면.',
# Chapter 2: Verdan and Malet
('verdan_arrival.field', 1, 'text_ko'): '베르단은 탄 향신료와 오줌 냄새가 났다. 집들이 보이기 전부터 냄새가 따라붙었다.',
('verdan_arrival.field', 2, 'text_ko'): '엘리아가 스카프 안으로 머리카락을 밀어 넣었다. 은빛이 천 아래로 사라졌다. 아렐도 깃을 세우고 그녀 곁으로 붙었다.',
('verdan_arrival.field', 3, 'text_ko'): '셈터. 지하야. 따라와.',
('malet_encounter.field', 0, 'text_ko'): '돌계단을 세 층 내려갔다. 마지막 층에서는 앰플을 다루는 곳의 금속성 단내가 났다.',
('malet_encounter.field', 1, 'text_ko'): '여기서는 자리를 잡고 기다려야 했다. 반 시간쯤. 아렐은 구석 탁자에 벽을 등지고 앉아 출구를 셌다. 셋이었다.',
('malet_encounter.field', 2, 'text_ko'): '먼 벽의 어두운 틈에서 회색 구두가 나왔다. 몸집이 작고 어깨가 좁은 남자였다.',
('malet_encounter.field', 3, 'text_ko'): '회색 옷에 둘러싸인 얼굴에서 호박색 눈이 유난히 선명했다. 그 눈이 코트의 얼룩을 훑고 칼자루에 멎었다.',
('malet_encounter.field', 4, 'text_ko'): '아헨 숲에서 공허수를 잡았더군. 성체. 3등급, 어쩌면 4. 혼자서.',
('malet_encounter.field', 5, 'text_ko'): '사십 마일 안의 추출소들이 이상 징후를 올렸어. 관리국이 이틀 안에 대략적인 위치를 잡을 거다. 더 빠를 수도 있고.',
('malet_encounter.field', 6, 'text_ko'): '관리국 지도에 없는 곳을 찾고 있어. 공백 7호.',
('malet_encounter.field', 7, 'text_ko'): '그 숫자를 말하고 오래 사는 사람은 드물어. 그레인으로 살 수 있는 정보도 아니고.',
('malet_encounter.field', 8, 'text_ko'): '첫 입맞춤 같은 건 가져오지 마. 네가 지금의 네가 된 데 쓰인 기억이 필요해. 검을 처음 쥔 날.',
('malet_encounter.field', 9, 'choice', 0): '검을 처음 쥔 날을 내준다.',
('malet_encounter.field', 9, 'choice', 1): '거절한다.',
('malet_deal.field', 0, 'text_ko'): '관리국 표식을 긁어낸 작은 놋쇠 장치였다. 차가운 촉이 관자놀이에 닿았다. "태울 때처럼 한꺼번에 끝나지 않아."',
('malet_deal.field', 1, 'text_ko'): '마당이었다. 작은 손이 나무 손잡이를 쥐고 있었다. 검 끝이 아래로 처졌다. 더 큰 손이 그의 손가락을 감쌌다.',
('malet_deal.field', 2, 'text_ko'): '누군가가 말했다. 소리는 들리지 않았다. 그는 조금 더 높이 들어 보려 했다. 검 끝이 올라갔다. 큰 손이 잡아 준 채였다.',
('malet_deal.field', 3, 'text_ko'): '흙먼지가 먼저 보이지 않게 됐다. 이어서 손잡이의 길이를 알 수 없었다. 손이 남았다. 누구의 손인지 생각할 틈도 없이 그것마저 없어졌다.',
('malet_reward.field', 0, 'text_ko'): '셋.',
('malet_reward.field', 1, 'text_ko'): '경로. 붕괴의 해안을 지나 마지막 추출소 너머로 나가는 길이야. 지난 십 년 동안 세 번 답사했지. 선을 벗어나지 마.',
('malet_reward.field', 2, 'text_ko'): '이름. 공허 구멍에 들어갔다가 제정신으로 나온 사람 중 내가 아는 유일한 사람이야. 세이블. 더 심. 베르단 북동쪽. 색을 따르게.',
('malet_reward.field', 3, 'text_ko'): '셋째는 경고야. 숲에 남긴 흔적 때문에 네 등급이 올라갔어.',
('malet_reward.field', 5, 'text_ko'): '관리국은 이번에 징수원을 보내지 않았어. 에디터를 보냈지. 카이로스. 조용하고 꼼꼼해. 거리를 좁히기까지 사흘쯤.',
('malet_reward.field', 6, 'text_ko'): '엘리아가 잔을 다시 잡았다. 도기가 탁자 위에서 짧게 끌렸다.',
('malet_reward.field', 7, 'text_ko'): '사흘이래. 그것보다 짧을 수도 있고.',
('malet_refused.field', 0, 'text_ko'): '그럼 더 할 얘기 없어. 계단은 저쪽이야.',
('elia_ch2_talk.field', 0, 'burned_text_ko'): '전에 네가 말한 시장이 있었는데... 하나도 기억 안 나지?',
('elia_ch2_talk.field', 2, 'burned_text_ko'): '누가 모닥불 옆에서 노래를 불러 줬다고 했잖아. 지금은 그 가락도 기억 안 나지.',
# Chapter 3: the waystation
('waystation_arrival.field', 0, 'text_ko'): '북동쪽 길에는 아직 포석이 남아 있었다. 길 밖의 나무들은 껍질과 가지가 같은 회색이었다.',
('waystation_arrival.field', 1, 'text_ko'): '베르단을 떠난 것은 새벽이었다. 말레트의 마지막 말이 갈비뼈 안쪽에 남아 있었다. "네가 찾는 그곳. 누군가가 이미 길 위에 있어."',
('waystation_arrival.field', 2, 'text_ko'): '어느 길인지도, 어디까지 갔는지도 몰라. 세이블한테 가는 길부터 놓치지 말아야 해.',
('waystation_arrival.field', 3, 'text_ko'): '그는 지도를 펼쳤다. 맨 앞 글자는 알아보았다. 다음 글자를 붙여 읽으려 하면 처음부터 다시 확인해야 했다.',
('waystation_arrival.field', 4, 'text_ko'): '역참이 있어. 베르단에서 반나절쯤. 거기서 쉬고 동쪽 길로 가면 돼.',
('waystation_arrival.field', 5, 'text_ko'): '두 시간쯤 걸었을 때 블랭커 배급소가 나왔다. 아홉 살쯤 된 아이가 흰 알약을 물도 없이 삼켰다. 저 손을 치워야 한다고 생각했다. 화가 뒤따르지 않았다.',
('waystation_arrival.field', 6, 'text_ko'): '이른 오후, 두 길이 만나는 자리에 낮은 지붕의 역참이 나타났다. 문 위에는 읽을 만한 표지가 없었다.',
('waystation_arrival.field', 7, 'text_ko'): '안에서 소리는 안 나. 문은 틀에 끼어 있고.',
('blank_book_discovery.field', 0, 'text_ko'): '구석에 천 자루 하나가 벽에 기대 있었다. 관리국 규격의 잠금장치가 달려 있었다. 바닥의 먼지는 두꺼운데, 자루 위의 먼지만 얇았다.',
('blank_book_discovery.field', 1, 'text_ko'): '건조 식량은 시큼하게 상해 있었다. 붕대 한 두름, 그레인 세 알. 접힌 천 밑에서 제목도 인장도 등록 도장도 없는 작은 가죽 책이 나왔다.',
('blank_book_discovery.field', 2, 'text_ko'): '아무것도 안 적혔어.',
('blank_book_discovery.field', 3, 'text_ko'): '안쪽에 있어. 감정.',
('blank_book_discovery.field', 5, 'text_ko'): '잔존이야. 아주 많이. 누가 이걸 오래 쥐고 있었어. 한 사람이 평생 쏟아도 이만큼일까 싶을 정도로.',
('blank_book_discovery.field', 6, 'text_ko'): '백 노트. 기록목 섬유로 만든 종이야. 관리국이 찾는 대로 태웠다고 들었어.',
('blank_book_discovery.field', 7, 'text_ko'): '우리한테 필요한 거야?',
('blank_book_discovery.field', 8, 'text_ko'): '아직 몰라. 내가 들고 갈게. 뭔지 알게 되면 말할게.',
('waystation_night.field', 0, 'text_ko'): '그들은 가져온 식량을 먹었다. 말린 고기와 납작빵. 아렐은 문 가까운 바닥에 앉아 칼집째 벗긴 검을 무릎에 놓았다.',
('waystation_night.field', 1, 'text_ko'): '불은 피우지 않았다. 어두워진 뒤에는 벌레 소리도 없었다. 문틈 너머의 나뭇가지가 움직이지 않았다.',
('waystation_night.field', 2, 'text_ko'): '자정이 지나 두 번째 보초를 설 때, 잠든 엘리아의 입술 사이에서 낮은 음이 나왔다. 세 음이었다. 협곡을 내려온 뒤 그의 입에서 흘러나왔던 가락이었다.',
('waystation_night.field', 3, 'text_ko'): '그의 오른손이 무릎에서 조금 들려 있었다. 그는 손을 더 뻗지 않고 주먹을 쥐었다. 노래는 이어지지 않았다.',
('class_seven_wall_message.field', 0, 'text_ko'): '아침, 문턱을 넘어 두 걸음쯤 갔다가 아렐이 멈췄다. 어제 건물을 돌아볼 때 그곳에는 아무것도 없었다.',
('class_seven_wall_message.field', 1, 'text_ko'): '문 오른쪽 돌 위에 청흑색 글씨가 한 줄로 놓여 있었다. 배급소의 표지와 시민 기록에서 보던 관리국의 표준 서체였다.',
('class_seven_wall_message.field', 2, 'text_ko'): '대상은 7급 연소 효율을 보인다.',
('class_seven_wall_message.field', 4, 'text_ko'): '엘리아는 다시 읽었다. 코트 안의 책을 누르던 팔이 몸에 더 붙었다.',
('class_seven_wall_message.field', 5, 'text_ko'): '떠나야 해.',
('waystation_departure.field', 0, 'text_ko'): '일 분도 되지 않아 엘리아가 짐을 메고 나왔다. 둘은 동쪽 길로 들어섰다.',
('waystation_departure.field', 1, 'text_ko'): '아렐이 지키고 앉아 있던 문은 열린 채 남았다.',
('waystation_departure.field', 2, 'text_ko'): '걸으면서 먹자. 해 있을 때 최대한 멀리.',
('belt_atmosphere.field', 0, 'text_ko'): '회색 나무들이 역참 주위에 서 있었다. 낮에도 새를 거의 보지 못했다.',
# Chapter 4: drift
('drift_arrival.field', 0, 'text_ko'): '첫 표지판은 길가로 기울어 있었다. 못에서 흘러내린 녹물이 첫 글자의 한 획을 덮고 있었다.',
('drift_arrival.field', 1, 'text_ko'): '다음 경유지까지 아홉 마일.',
('drift_arrival.field', 2, 'text_ko'): '말로 들으니 이해됐다. 아렐은 그 뜻을 붙들고 다시 판자를 보았지만, 어디까지가 경유지이고 어디서부터 거리인지 나눌 수 없었다.',
('drift_arrival.field', 3, 'text_ko'): '동쪽 길은 점점 메말라 갔다. 바퀴가 오래전에 파 놓은 홈에는 재가 고여 있었다.',
('drift_arrival.field', 4, 'text_ko'): '글씨가 안 보여?',
('drift_arrival.field', 5, 'text_ko'): '보여. 글자가 안 붙어.',
('drift_arrival.field', 6, 'text_ko'): '앉자. 길에서는 못 해.',
('drift_arrival.field', 7, 'text_ko'): '마흔 걸음쯤 떨어진 곳에 무너진 돌벽이 있었다. 동쪽에만 남은 벽이었다. 오래된 피난처 같았다.',
('reading_deterioration.field', 0, 'text_ko'): '맨 앞 글자는 알아. 다음 걸 확인하고 돌아오면, 방금 읽은 소리가 맞는지 모르겠어.',
('reading_deterioration.field', 1, 'text_ko'): '언제부터.',
('reading_deterioration.field', 2, 'text_ko'): '삼 주쯤. 읽힐 때도 있었어. 아침에 벽도 읽었고.',
('reading_deterioration.field', 3, 'text_ko'): '왜 말 안 했어. ...손 줘.',
('anchoring_session.field', 0, 'text_ko'): '엘리아가 밑에서 두 손을 받쳤다. 오른손에 닿은 엄지가 딱지를 피해 조금 위로 옮겨 갔다. 그녀의 손은 차가웠다.',
('anchoring_session.field', 2, 'text_ko'): '눈 뒤의 통증이 조금씩 약해졌다. 왼손이 튀었다. 엘리아가 손목을 받쳐 잡았다. 눈꺼풀 뒤에서 흰빛이 번졌다.',
('anchoring_session.field', 3, 'text_ko'): '나랑 있어. ...이름.',
('anchoring_session.field', 5, 'text_ko'): '어디야.',
('anchoring_session.field', 6, 'text_ko'): '역참 동쪽. 림 경계로 가는 길.',
('anchoring_session.field', 7, 'text_ko'): '아침에 뭐 먹었어.',
('anchoring_session.field', 8, 'text_ko'): '먹긴 했어. ...모르겠어.',
('anchoring_session.field', 9, 'text_ko'): '글자와 뜻을 잇는 데가 풀려 있었어. 붙들어 놨어. 읽는 건 나아질 거야. 옆에 있던 기억이 같이 버티질 못했어. 붙잡는 동안 끊겼어.',
('anchoring_session.field', 10, 'text_ko'): '얼마나 가.',
('anchoring_session.field', 12, 'text_ko'): '돌 하나에 작은 글씨가 남아 있었다. 베르단 무역 관리국 소유. 침입자는 기록됩니다. 문장을 끝까지 읽고 나자 뜻도 남았다.',
('night_watch.field', 0, 'text_ko'): '그 뒤로 아렐은 표지판을 그냥 지나치지 않았다. 엘리아는 가방 끈을 묶는 데 평소보다 오래 걸렸다.',
('night_watch.field', 1, 'text_ko'): '그러다 바람이 끊겼다. 코트 자락이 다리에 붙었다. 목덜미가 조여 왔다. 등 뒤의 길은 비어 있었다.',
('night_watch.field', 2, 'text_ko'): '뭐 있어?',
('night_watch.field', 3, 'text_ko'): '바람이 멎었어.',
('night_watch.field', 4, 'text_ko'): '해가 낮아질 무렵, 달리던 모양 그대로 굳어 죽은 순환 동물을 지나쳤다. 기록이 얇은 땅에서는 짐승도 걸렸다.',
('night_watch.field', 5, 'text_ko'): '빗물에 파인 얕은 굴. 아렐은 빵의 신맛과 돌 맛이 나는 물을 놓치지 않으려고 혀를 입천장에 댔다. 떠올릴 수 있었다.',
('night_watch.field', 6, 'text_ko'): '오늘처럼 오래 기다리지 마. 짧은 글이든 긴 글이든, 안 읽히면 말해.',
('night_watch.field', 7, 'text_ko'): '말할게.',
('night_watch.field', 8, 'text_ko'): '자. 내가 먼저 볼 테니까.',
('night_watch.field', 9, 'text_ko'): '엘리아는 그의 호흡이 고르게 이어질 때까지 기다렸다. 코 안쪽이 다시 뜨거워졌다. 윗입술에 손을 댔다. 피였다.',
('drift_departure.field', 0, 'text_ko'): '그녀는 손등으로 코를 닦고 코트 안감에 문질렀다. 젖은 안감을 안으로 접었다.',
('drift_departure.field', 1, 'text_ko'): '그러고는 입구가 잘 보이는 자리로 조금 옮겨 앉았다.',
# Chapter 5: the classifier
('ch5_classifier.vn', 0, 'narrate_ko'): '짐승이 죽고 이틀 뒤였다. 갈색 코트에 먼지가 묻은 남자가 공터 가장자리에 가죽 자루를 내려놓고, 천 의자부터 펼쳤다.',
('ch5_classifier.vn', 1, 'narrate_ko'): '오래 걸은 부츠와 모래색 머리카락. 이곳에 온 사람이 관리국의 에디터라는 것을 알기는 어려웠다. 그는 검은 가죽 노트를 무릎에 폈다.',
('ch5_classifier.vn', 2, 'text_ko'): '사건 현장 11호. 공허수, 미성체. 연소로 처치.',
('ch5_classifier.vn', 3, 'narrate_ko'): '놋쇠 기기를 얼룩 위에 들자 눈금 없는 유리 바늘이 돌았다. 얼룩의 한쪽에서 바늘이 오래 머물렀다.',
('ch5_classifier.vn', 4, 'text_ko'): '시그니처 잔여 0.6 감쇠 단위. 예상 산출량의 약 10배.',
('ch5_classifier.vn', 5, 'narrate_ko'): '연소 잉크에 적신 뼈 펜으로 짧은 구절을 쓰자, 발밑의 흰 풀이 뿌리 쪽부터 회녹색으로 돌아왔다.',
('ch5_classifier.vn', 6, 'narrate_ko'): '주머니 속 그을린 나무 조각은 매끈한 원통이 되어 있었다. 갈라진 끝이 없어져 있었다.',
('ch5_classifier.vn', 7, 'text_ko'): '연소자 분류, 7급. 확인.',
('ch5_classifier.vn', 8, 'narrate_ko'): '십일 년 전의 림. 육 년 전 아르케인 도로. 사십 개월 전 베르단 외곽 농가. 같은 사람이 남긴 흔적이었다. 그는 전송 기호를 쓰지 않았다.',
('ch5_classifier.vn', 9, 'text_ko'): '모노리스에 올리면 벨로르가 보게 된다. 이레이저가 움직이겠지. 7급 보고 보류.',
('ch5_classifier.vn', 10, 'narrate_ko'): '붉은 노트에 한 줄을 덧붙였다. 지침 제4호 위반. 십일 년 동안 남에게 보여 주지 않은 책이었다.',
('ch5_classifier.vn', 11, 'narrate_ko'): '다음 날, 베르단에서 말레트의 보고서가 왔다. 짧은 암호문이었다. 무명자 남성. 첫 검의 기억 매각.',
('ch5_classifier.vn', 12, 'narrate_ko'): '연소자가 기억을 팔았다. 이번에는 태우지 않고. 대가로 무엇을 받아 갔는지는 이 문장에 없었다.',
('ch5_classifier.vn', 13, 'text_ko'): '짐승을 죽인 뒤에도 하나를 더 내놓았군.',
('ch5_classifier.vn', 14, 'narrate_ko'): '끝부분에 동행이 언급돼 있었다. 한 줄을 다 쓰지도 않은 기록이었다. 흐린 자 여성 동행.',
('ch5_classifier.vn', 15, 'text_ko'): '흐린 자가, 저만한 연소 뒤에서 사람을 붙들어 놓았다.',
('ch5_classifier.vn', 16, 'narrate_ko'): '그는 그 구절에 동그라미를 쳤다. 조금 떨어져 하나를 더 그렸다. 분류를 정정하지는 않았다.',
('ch5_classifier.vn', 17, 'text_ko'): '물을 일이 늘어나는군.',
('ch5_classifier.vn', 18, 'narrate_ko'): '남자는 북쪽으로 걸어갔다. 카이로스는 접은 의자를 팔 아래에 끼고 같은 쪽으로 향했다.',
('ch5_classifier.vn', 19, 'text_ko'): '관찰 국면: 종료. 접촉 국면: 개시. 권장 접근법: 신중함.',
('ch5_classifier.vn', 20, 'narrate_ko'): '주머니 속 원통의 한쪽 끝에 작은 모서리가 다시 생겨 있었다. 고친 것이 하루를 버티지 못했다.',
}

def esc(s):
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

rows = {}
for (stem, idx, kind, *rest) in M:
    if stem not in rows:
        d = json.load(open(os.path.join(IR, stem + '.v1.json'), encoding='utf-8'))
        rows[stem] = {r['original_index']: r for r in (d['definition'].get('rows') or d['definition'].get('steps'))}
out, seen = [], {}
for key, new in M.items():
    stem, idx, kind = key[0], key[1], key[2]
    row = rows[stem].get(idx)
    if row is None: sys.exit(f'missing row {key}')
    if kind == 'choice':
        old = row['choices'][key[3]]['text'].get('text_ko')
    else:
        old = (row.get('text') or {}).get(kind)
    if not old: sys.exit(f'missing text {key}')
    if old in seen and seen[old] != new: sys.exit(f'conflict for {old!r}: {seen[old]!r} vs {new!r}')
    if old == new or old in seen: continue
    seen[old] = new
    out.append(f'        {{TEXT("{esc(old)}"), TEXT("{esc(new)}")}},  // {stem} {idx}{" choice " + str(key[3]) if kind == "choice" else ""}\n')
with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('// Generated by S370 s370_lines.py from the narrative IR and the final manuscript; edit the script, not this file.\n')
    f.writelines(out)
print(len(out), 'lines ->', OUT)
