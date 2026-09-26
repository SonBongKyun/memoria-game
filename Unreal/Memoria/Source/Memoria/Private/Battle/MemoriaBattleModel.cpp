#include "Battle/MemoriaBattleModel.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MemoriaBattleCoreSource.inl"
namespace {
using Obj=TSharedPtr<FJsonObject>;
Obj Source(){static Obj Data=[](){Obj O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(MemoriaBattleCoreSourceJson),O);return O;}();return Data;}
FString N(int64 V){return FString::Printf(TEXT("%lld"),V);}
int32 RankAt(double V){return V>=100?4:V>=75?3:V>=50?2:V>=25?1:0;}
double Mult(int32 R){const double M[]={1.,1.04,1.08,1.13,1.20};return M[FMath::Clamp(R,0,4)];}
FString ItemName(const FString& Id){return Source()->GetObjectField(TEXT("items"))->GetObjectField(Id)->GetStringField(TEXT("name"));}
}
FString FMemoriaBattleModel::Text(const FString& Locale,const FString& Key,const TArray<FString>& Args)
{
    FString Result=Key;Source()->GetObjectField(TEXT("messages"))->GetObjectField(Locale==TEXT("ko")?TEXT("ko"):TEXT("en"))->TryGetStringField(Key,Result);
    for(const auto& Arg:Args){int32 S=Result.Find(TEXT("%s")),D=Result.Find(TEXT("%d"));int32 I=S<0?D:D<0?S:FMath::Min(S,D);if(I>=0)Result=Result.Left(I)+Arg+Result.Mid(I+2);}return Result;
}
void FMemoriaBattleModel::Log(const FString& Key,const TArray<FString>& Args){Logs.Add(Text(Run.CurrentLocale,Key,Args));}
void FMemoriaBattleModel::ClearEvents(){Logs.Reset();Hits.Reset();Sounds.Reset();Ability.Reset();}
bool FMemoriaBattleModel::SupportsObjective(const FString& I){return I==TEXT("keep_memory")||I==TEXT("swift_finish")||I==TEXT("combo_three")||I==TEXT("kindle_momentum")||I==TEXT("force_break")||I==TEXT("scan_first")||I==TEXT("witness_echo")||I==TEXT("no_items");}
// Bosses and the quiet_focus anchor passive are outside the ambient slice; both stay at the source floor of two.
int32 FMemoriaBattleModel::WitnessRequirement(bool bVoidBeast,bool bEliaAnchor){return FMath::Max(2,(bVoidBeast?3:2)-(bVoidBeast&&bEliaAnchor?1:0));}
FString FMemoriaBattleModel::WitnessKey(const FString& Name)
{
    const FString L=Name.ToLower();
    return L.Contains(TEXT("kairos"))?TEXT("kairos"):L.Contains(TEXT("sentinel"))||L.Contains(TEXT("guardian"))?TEXT("sentinel"):L.Contains(TEXT("void"))||L.Contains(TEXT("threshold"))?TEXT("void")
        :L.Contains(TEXT("forest"))||L.Contains(TEXT("shade"))?TEXT("forest"):L.Contains(TEXT("ash"))||L.Contains(TEXT("crawler"))?TEXT("ash"):TEXT("generic");
}
void FMemoriaBattleModel::AddLimit(double A){Limit=FMath::Min(100.,Limit+A*(bMemoryCascade?1.2:1.));}
void FMemoriaBattleModel::AddMomentum(double A,const FString& Reason)
{
    if(A<=0)return;Momentum=FMath::Clamp(Momentum+A,0.,100.);Rank=RankAt(Momentum);BestRank=FMath::Max(BestRank,Rank);
    Run.HighestMomentumRank=FMath::Max<int64>(Run.HighestMomentumRank,Rank);Log(TEXT("[RESONANCE] %s +%d"),{Reason,N(A)});CheckObjective();
}
void FMemoriaBattleModel::CheckObjective(bool Burn,bool Item)
{
    if(!bObjectiveSupported||bObjectiveComplete||bObjectiveFailed)return;
    if((ObjectiveId==TEXT("keep_memory")&&Burn)||(ObjectiveId==TEXT("no_items")&&Item)||(ObjectiveId==TEXT("swift_finish")&&Actions>4)){bObjectiveFailed=true;return;}
    if((ObjectiveId==TEXT("force_break")&&Breaks>0)||(ObjectiveId==TEXT("combo_three")&&MaxCombo>=3)||(ObjectiveId==TEXT("kindle_momentum")&&BestRank>=3)
        ||(ObjectiveId==TEXT("scan_first")&&bScanned)||(ObjectiveId==TEXT("witness_echo")&&bWitnessComplete))bObjectiveComplete=true;
}
// Source player_witness/_use_witness_ink after their action bookkeeping: read, record, guard, then release or pass the turn.
void FMemoriaBattleModel::Witness(int32 Power,bool bInk,FMemoriaEncounterRng& Rng)
{
    WitnessProgress=FMath::Min(WitnessProgress+FMath::Max(Power,1),WitnessRequired);
    const auto Table=Source()->GetObjectField(TEXT("witness_lines"))->GetObjectField(Run.CurrentLocale==TEXT("ko")?TEXT("ko"):TEXT("en"));
    const TArray<TSharedPtr<FJsonValue>>* Lines=nullptr;if(!Table->TryGetArrayField(WitnessKey(EnemyName),Lines))Lines=&Table->GetArrayField(TEXT("generic"));
    WitnessLine=(*Lines)[FMath::Clamp(WitnessProgress-1,0,Lines->Num()-1)]->AsString();
    bScanned=true;CheckObjective();
    AddLimit(bInk?10:8);AddMomentum(bInk?8:10,bInk?TEXT("Witness Ink"):TEXT("Witnessed echo"));bDefending=true;
    Log(bInk?TEXT("[WITNESS INK %d/%d] %s"):TEXT("[WITNESS %d/%d] %s"),{N(WitnessProgress),N(WitnessRequired),WitnessLine});Sounds.Add(TEXT("rising_tone"));
    if(WitnessProgress<WitnessRequired){EndPlayer(Rng);return;}
    bWitnessComplete=bResolvedByWitness=true;CheckObjective();
    const FString Flag=TEXT("witnessed_")+WitnessKey(EnemyName);
    if(auto* F=Run.StoryFlags.FindByPredicate([&](const auto& V){return V.Id==Flag;}))F->bValue=true;else Run.StoryFlags.Add({Flag,true});
    EnemyHp=0;Log(TEXT("[RELEASED] The echo lets go without another memory being burned."));Sounds.Add(TEXT("memory_add"));Win(Rng);
}
bool FMemoriaBattleModel::HasStatus(bool P,int32 E)const{for(const auto& S:P?PlayerStatuses:EnemyStatuses)if(S.Effect==E)return true;return false;}
double FMemoriaBattleModel::Weaken(bool P)const{for(const auto& S:P?PlayerStatuses:EnemyStatuses)if(S.Effect==1)return 1.-S.Power/100.;return 1.;}
double FMemoriaBattleModel::Element(const FString& E)const{return Weakness==E?1.5:Resistance==E?.7:1.;}
int64 FMemoriaBattleModel::Damage(int64 A,const FString& Skill){const int64 Actual=FMath::Min(A,EnemyHp);EnemyHp-=Actual;Hits.Add({EnemyName,Skill,Actual});return Actual;}
void FMemoriaBattleModel::ApplyStatus(bool P,int32 E,int32 T,int64 Power)
{
    auto& List=P?PlayerStatuses:EnemyStatuses;
    for(auto& S:List)if(S.Effect==E){if(Power>=S.Power){S.Power=Power;S.Turns=T;}return;}
    List.Add({E,T,Power});
}
void FMemoriaBattleModel::StatusTick(bool P)
{
    auto& List=P?PlayerStatuses:EnemyStatuses;
    for(auto& S:List){if(S.Effect==0||S.Effect==2){const FString Skill=S.Effect==0?TEXT("Poison"):TEXT("Burn");if(P){Run.Player.Hp=FMath::Max<int64>(0,Run.Player.Hp-S.Power);Hits.Add({TEXT("Arrel"),Skill,S.Power});}else{EnemyHp-=FMath::Min(S.Power,EnemyHp);Hits.Add({EnemyName,Skill,S.Power});}}--S.Turns;}
    List.RemoveAll([](const auto& S){return S.Turns<=0;});
}
void FMemoriaBattleModel::Pressure(const FString& E)
{
    if(E.IsEmpty()||BrokenTurns>0)return;double Gain=Weakness==E?42.:Resistance==E?0.:16.;if(bSteadyHand)Gain*=1.3;if(Gain<=0)return;
    Break=FMath::Clamp(Break+Gain,0.,100.);if(Weakness==E)AddMomentum(8,TEXT("Weakness pressure"));
    if(Break>=100){Break=0;BrokenTurns=2;++Breaks;AddMomentum(18,TEXT("BREAK triggered"));Log(TEXT("[BREAK] %s is staggered!"),{EnemyName});CheckObjective();}
}
void FMemoriaBattleModel::AddItem(const FString& Id){for(auto& I:Run.Player.Items)if(I.Id==Id){++I.Count;Run.RecordRecentItem(Id);return;}Run.Player.Items.Add({Id,1});Run.RecordRecentItem(Id);}
bool FMemoriaBattleModel::Act(const FString& Action,const FString& Id,const FMemoriaBattleBurn* Burn,FMemoriaEncounterRng& Rng)
{
    if(bPendingEnemy||bVictory||bDefeat||EnemyHp<=0)return false;
    if(Action!=TEXT("attack")&&Action!=TEXT("burn")&&Action!=TEXT("defend")&&Action!=TEXT("item")&&Action!=TEXT("witness"))return false;
    if(Action==TEXT("burn")&&!Burn)return false;
    // Source rejects a reading (and keeps the ink) once the echo is fully heard.
    if((Action==TEXT("witness")||(Action==TEXT("item")&&Id==TEXT("witness_ink")))&&WitnessProgress>=WitnessRequired)return false;
    if(Action==TEXT("item")){
        if(Id!=TEXT("potion")&&Id!=TEXT("antidote")&&Id!=TEXT("firebomb")&&Id!=TEXT("witness_ink"))return false;
        auto* Item=Run.Player.Items.FindByPredicate([&](const auto& I){return I.Id==Id&&I.Count>0;});if(!Item)return false;
        if(--Item->Count==0)Run.Player.Items.RemoveAll([&](const auto& I){return I.Id==Id;});
    }
    ClearEvents();++Actions;if(Action==TEXT("item"))++ItemsUsed;CheckObjective(false,Action==TEXT("item"));
    if(Action==TEXT("witness")){Combo=0;LastAction=Action;Witness(1,false,Rng);return true;}
    if(Action==TEXT("attack"))
    {
        Chain=0;Combo=LastAction==TEXT("attack")?Combo+1:1;LastAction=Action;
        if(ModifierEffect==TEXT("player_miss")&&Rng.Integer(0,99)<ModifierValue){Log(TEXT("The fog of burned memories clouds your strike... MISS!"));EndPlayer(Rng);return true;}
        int64 D=15+(Run.CurrentChapter-1)*3+Rng.Integer(0,10);
        const double CM=Combo==2?1.15:Combo==3?1.30:Combo==4?1.50:Combo==5?1.70:Combo>=6?2.:1.;
        D=int64(D*CM);D=int64(D*Weaken(true));D=int64(D*Element(TEXT("physical")));
        if(bVoid)D=FMath::Max<int64>(1,int64(D*(bUnbrokenEdge?.55:.3)));
        if(bShielded){D=FMath::Max<int64>(1,D/2);bShielded=false;Sounds.Add(TEXT("shield_break"));}
        const bool Broken=BrokenTurns>0;if(Broken)D=FMath::Max<int64>(1,int64(D*1.8));D=int64(D*Mult(Rank));
        const int64 Actual=Damage(D,TEXT("Attack"));Sounds.Add(TEXT("sword_slash"));
        Log(TEXT("Arrel strikes! %d damage.%s"),{N(Actual),Combo>=2?FString::Printf(TEXT(" (Combo x%d!)"),Combo):FString()});
        Pressure(TEXT("physical"));AddMomentum(Broken?6:Combo>=2?5:3,Broken?TEXT("Punished BREAK"):Combo>=2?TEXT("Combo maintained"):TEXT("Clean strike"));AddLimit(8);
        MaxCombo=FMath::Max(MaxCombo,Combo);CheckObjective();if(Combo==3)AddLimit(5);else if(Combo==5)AddLimit(10);else if(Combo==7)AddLimit(15);
        if(bReflecting){bReflecting=false;int64 Reflect=FMath::Max<int64>(1,int64(Actual*.3));Run.Player.Hp=FMath::Max<int64>(0,Run.Player.Hp-Reflect);Hits.Add({TEXT("Arrel"),TEXT("Reflect"),Reflect});}
    }
    else if(Action==TEXT("burn"))
    {
        Combo=0;LastAction=Action;++Burns;CheckObjective(true);Chain=Burn->Grade>=2?Chain+1:0;
        bMemoryCascade=Burn->bMemoryCascade;
        const int32 Raw=FMath::Max(Burn->Grade-1,0);Aftershock=FMath::Max(Aftershock,Raw-(Run.Player.bEliaWithParty&&Raw>=2?1:0));
        const auto Skill=Source()->GetObjectField(TEXT("burn_skills"))->GetObjectField(N(Burn->Grade));
        const FString Elem=Skill->GetStringField(TEXT("element"));int64 D=int64(Skill->GetNumberField(TEXT("base_damage")))+Burn->EffectivePower;
        if(Burn->bEmberAffinity)D=int64(D*1.1);if(Chain>=2)D=int64(D*(1.+(Chain-1)*.2));
        D=int64(D*Element(Elem)*(Elem==TEXT("void")&&Burn->bVoidTouch?1.15:1.));
        if(bShielded){D=FMath::Max<int64>(1,int64(D*.7));bShielded=false;}
        if(BrokenTurns>0)D=FMath::Max<int64>(1,int64(D*1.3));D=int64(D*Mult(Rank));
        const int64 Actual=Damage(D,Skill->GetStringField(TEXT("name")));Log(TEXT("%d damage to %s!"),{N(Actual),EnemyName});
        Pressure(Elem);AddMomentum(10.+Burn->Grade*3.+(Chain>=2?6.:0.),TEXT("Memory burn"));
        if(Burn->Grade>=3)ApplyStatus(false,2,2,int64(Burn->Power*.3)+5);
        if(Burn->bResidualWarmth){Run.Player.Hp=FMath::Min(Run.Player.MaxHp,Run.Player.Hp+5);Hits.Add({TEXT("Arrel"),TEXT("Residual Warmth"),-5});}
        AddLimit(12);if(Burn->Grade<3)Sounds.Add(TEXT("burn_ignite"));
    }
    else if(Action==TEXT("defend"))
    {
        Chain=0;Combo=0;LastAction=Action;bDefending=true;double Focus=12,M=6;
        if(!PlayerStatuses.IsEmpty()){for(auto& S:PlayerStatuses)--S.Turns;PlayerStatuses.RemoveAll([](const auto& S){return S.Turns<=0;});Focus+=4;M+=4;Log(TEXT("Guard Focus steadies Arrel. Status pressure weakens."));}
        else if(Run.Player.Hp<Run.Player.MaxHp){const int64 Heal=FMath::Min(FMath::Max<int64>(3,int64(Run.Player.MaxHp*.05)),Run.Player.MaxHp-Run.Player.Hp);Run.Player.Hp+=Heal;Hits.Add({TEXT("Arrel"),TEXT("Guard Focus"),-Heal});M+=2;Log(TEXT("Guard Focus restores %d HP."),{N(Heal)});}
        else{Focus+=4;M+=2;Log(TEXT("Guard Focus primes the Limit gauge."));}AddLimit(Focus);AddMomentum(M,TEXT("Guard Focus"));
    }
    else
    {
        Combo=0;LastAction=TEXT("item");Sounds.Add(TEXT("ui_select"));const FString Name=ItemName(Id);
        if(Id==TEXT("potion")){Run.Player.Hp=FMath::Min(Run.Player.Hp+40,Run.Player.MaxHp);Hits.Add({TEXT("Arrel"),Name,-40});Sounds.Add(TEXT("heal"));Log(TEXT("Used %s, restored %d HP."),{Name,TEXT("40")});}
        else if(Id==TEXT("antidote")){PlayerStatuses.RemoveAll([](const auto& S){return S.Effect==0||S.Effect==2;});const int64 H=FMath::Min<int64>(12,Run.Player.MaxHp-Run.Player.Hp);Run.Player.Hp+=H;if(H>0)Hits.Add({TEXT("Arrel"),Name,-H});Log(TEXT("Used %s, status effects cured!"),{Name});}
        else if(Id==TEXT("witness_ink")){Witness(1,true,Rng);return true;}
        else{Damage(12,Name);if(EnemyHp<=0){Win(Rng);return true;}ApplyStatus(false,2,2,15);}
    }
    if(EnemyHp<=0)Win(Rng);else EndPlayer(Rng);return true;
}
void FMemoriaBattleModel::EndPlayer(FMemoriaEncounterRng& Rng)
{
    ++Turns;
    if(ModifierEffect==TEXT("dot_both")||ModifierEffect==TEXT("player_dot")){Run.Player.Hp=FMath::Max<int64>(0,Run.Player.Hp-ModifierValue);if(ModifierEffect==TEXT("dot_both"))EnemyHp-=FMath::Min(ModifierValue,EnemyHp);}
    if(ModifierEffect==TEXT("turn_limit")&&Turns>=ModifierValue){if(LastStand(true))Turns=0;else{bDefeat=true;Run.Player.DirectiveStreak=0;bPendingEnemy=false;return;}}
    if(Run.Player.Hp<=0){CheckPlayer();return;}
    StatusTick(false);if(EnemyHp<=0){Win(Rng);return;}EnemyResponses=(ModifierEffect==TEXT("enemy_double_turn")&&ModifierValue>0&&Turns%ModifierValue==0)?2:1;bPendingEnemy=true;
}
FString FMemoriaBattleModel::SelectAbility(FMemoriaEncounterRng& Rng)const
{
    TArray<FString> List;for(const auto& A:Abilities){if((A==TEXT("poison")&&HasStatus(true,0))||(A==TEXT("weaken")&&HasStatus(true,1))||(A==TEXT("reflect")&&bReflecting)||(A==TEXT("charge")&&bCharged)||(A==TEXT("stun")&&bStunned))continue;List.Add(A);}if(List.IsEmpty())List=Abilities;
    TMap<FString,double> Weights;for(const auto& A:List)Weights.Add(A,1.);
    for(const auto& A:List){auto& W=Weights[A];if(double(Run.Player.Hp)/Run.Player.MaxHp<.3&&(A==TEXT("drain")||A==TEXT("stun")))W*=2.;if(double(EnemyHp)/EnemyMaxHp<.4&&(A==TEXT("shield")||A==TEXT("reflect")||A==TEXT("charge")||A==TEXT("drain")))W*=2.;}
    if(Combo>=3){if(auto* W=Weights.Find(TEXT("stun")))*W*=3.;if(auto* W=Weights.Find(TEXT("shield")))*W*=1.5;}
    if(HasStatus(true,0))if(auto* W=Weights.Find(TEXT("poison")))*W*=.1;
    if(bDefending){if(auto* W=Weights.Find(TEXT("poison")))*W*=1.5;if(auto* W=Weights.Find(TEXT("weaken")))*W*=1.5;}
    double Sum=0;for(const auto& A:List)Sum+=Weights[A];double Roll=Rng.Real(0,1)*Sum,Acc=0;for(const auto& A:List){Acc+=Weights[A];if(Roll<=Acc)return A;}return List[Rng.Integer(0,List.Num()-1)];
}
void FMemoriaBattleModel::EnemyTurn(FMemoriaEncounterRng& Rng)
{
    if(!bPendingEnemy||bVictory||bDefeat)return;ClearEvents();bPendingEnemy=false;EnemyResponses=FMath::Max(0,EnemyResponses-1);
    if(BrokenTurns>0){--BrokenTurns;Aftershock=FMath::Max(0,Aftershock-1);Log(TEXT("%s is broken and loses the turn!"),{EnemyName});bPendingEnemy=EnemyResponses>0;return;}
    if(!Abilities.IsEmpty()&&Rng.Real(0,1)<=.3)Ability=SelectAbility(Rng);
    int64 D=0;FString Skill;
    if(Ability==TEXT("poison"))ApplyStatus(true,0,3,int64(EnemyAttack*.3)+Rng.Integer(2,5));
    else if(Ability==TEXT("weaken"))ApplyStatus(true,1,3,30);
    else if(Ability==TEXT("shield")){bShielded=true;Sounds.Add(TEXT("shield"));}
    else if(Ability==TEXT("reflect")){bReflecting=true;bShielded=true;Sounds.Add(TEXT("shield"));}
    else if(Ability==TEXT("charge")){bCharged=true;Sounds.Add(TEXT("shield"));}
    else if(Ability==TEXT("drain")){D=int64((EnemyAttack+Rng.Integer(5,10))*(1.+Difficulty));if(bDefending)D=FMath::Max<int64>(1,D/2);EnemyHp=FMath::Min(EnemyMaxHp,EnemyHp+D/2);Skill=TEXT("Drain");Sounds.Add(TEXT("drain"));}
    else if(Ability==TEXT("stun")){D=int64((EnemyAttack*.4+Rng.Integer(0,5))*(1.+Difficulty));if(bDefending)D=FMath::Max<int64>(1,D/2);Skill=TEXT("Stun");bStunned=true;}
    else{D=EnemyAttack+Rng.Integer(0,5);if(Difficulty>0)D=int64(D*(1.+Difficulty));if(bCharged){D*=2;bCharged=false;}D=int64(D*Weaken(false));if(bDefending)D=FMath::Max<int64>(1,D/2);if(AnchorGuard>0)D=FMath::Max<int64>(1,int64(D*(1.-AnchorGuard)));Skill=EnemyName;}
    if(!Skill.IsEmpty()){bDefending=false;Run.Player.Hp=FMath::Max<int64>(0,Run.Player.Hp-D);Hits.Add({TEXT("Arrel"),Skill,D});AddLimit(15);Log(TEXT("%s attacks! %d damage to Arrel."),{EnemyName,N(D)});Sounds.Add(TEXT("hit"));}
    else if(Ability==TEXT("poison"))Log(TEXT("%s releases a toxic cloud!"),{EnemyName});
    else if(Ability==TEXT("weaken"))Log(TEXT("%s curses Arrel's strength!"),{EnemyName});
    else if(Ability==TEXT("shield"))Log(TEXT("%s raises a dark barrier."),{EnemyName});
    else if(Ability==TEXT("reflect"))Log(TEXT("%s conjures a mirror of void energy!"),{EnemyName});
    else if(Ability==TEXT("charge"))Log(TEXT("%s gathers dark energy..."),{EnemyName});
    Aftershock=FMath::Max(0,Aftershock-1);const bool LethalBefore=Run.Player.Hp<=0;CheckPlayer();
    if(LethalBefore&&!bDefeat)return;
    if(EnemyResponses>0&&!bDefeat)bPendingEnemy=true;
    if(bStunned&&!bDefeat&&Run.Player.Hp>0){bStunned=false;EndPlayer(Rng);}
}
bool FMemoriaBattleModel::LastStand(bool Lethal)
{
    if(bLastStand||(!Lethal&&double(Run.Player.Hp)/FMath::Max<int64>(1,Run.Player.MaxHp)>.25))return false;
    bLastStand=true;if(Run.Player.Hp<=0)Run.Player.Hp=1;bDefending=true;AddLimit(22);AddMomentum(16,TEXT("Last Stand"));Log(TEXT("[LAST STAND] Arrel refuses to vanish. HP holds at %d, next blow guarded."),{N(Run.Player.Hp)});return true;
}
void FMemoriaBattleModel::CheckPlayer()
{
    if(Run.Player.Hp<=0){if(LastStand(true))return;bDefeat=true;Run.Player.DirectiveStreak=0;Sounds.Add(TEXT("defeat"));return;}
    StatusTick(true);if(Run.Player.Hp<=0){if(LastStand(true))return;bDefeat=true;Run.Player.DirectiveStreak=0;Sounds.Add(TEXT("defeat"));return;}LastStand(false);
}
void FMemoriaBattleModel::Win(FMemoriaEncounterRng& Rng)
{
    bVictory=true;bPendingEnemy=false;Reward.Heal=int64(Run.Player.MaxHp*.2);Run.Player.Hp=FMath::Min(Run.Player.MaxHp,Run.Player.Hp+Reward.Heal);
    int64 PreservationFocus=0;
    if(bWitnessComplete)
    {
        // Source preservation: a reading completed in this fight, released or not.
        Reward.PreservationBonus=bResolvedByWitness&&bVoid?12:bResolvedByWitness?8:6;const int64 Before=Run.Player.FieldFocus;
        Run.Player.FieldFocus=FMath::Min<int64>(3,Run.Player.FieldFocus+1);PreservationFocus=Reward.FocusGained=Run.Player.FieldFocus-Before;
    }
    Reward.TacticalBonus=bScanned?1:0;Reward.Resolution=bResolvedByWitness?TEXT("witness"):bWitnessComplete?TEXT("insight"):TEXT("defeat");
    if(bObjectiveSupported&&!bObjectiveFailed&&((ObjectiveId==TEXT("keep_memory")&&Burns==0)||(ObjectiveId==TEXT("no_items")&&ItemsUsed==0)||(ObjectiveId==TEXT("swift_finish")&&Actions<=4)||(ObjectiveId==TEXT("kindle_momentum")&&BestRank>=3)))bObjectiveComplete=true;
    if(bObjectiveSupported&&bObjectiveComplete){Reward.ObjectiveBonus=ObjectiveGrains;Reward.ObjectiveHeal=ObjectiveHeal;Reward.ObjectiveItem=ObjectiveItem;if(!ObjectiveItem.IsEmpty())AddItem(ObjectiveItem);const int64 H=FMath::Min(ObjectiveHeal,Run.Player.MaxHp-Run.Player.Hp);Run.Player.Hp+=H;Reward.Heal+=H;if(ObjectiveHeal>0)Hits.Add({TEXT("Arrel"),TEXT("Objective Heal"),-H});}
    int32 Score=45+(bObjectiveComplete?20:0)+BestRank*5+(bWitnessComplete?10:0)+(Breaks>0?5:0)+FMath::Min(MaxCombo,5)*2+(Actions<=4?8:Actions<=7?4:0);
    const int64 GradeBonus=Score>=90?15:Score>=78?9:Score>=65?5:Score>=50?2:0;Reward.Score=FMath::Clamp(Score,0,100);
    Reward.Grade=Score>=90?TEXT("S"):Score>=78?TEXT("A"):Score>=65?TEXT("B"):Score>=50?TEXT("C"):TEXT("D");Reward.GradeBonus=GradeBonus;
    int64 StreakBonus=0;if(bObjectiveComplete){++Run.Player.DirectiveStreak;StreakBonus=FMath::Clamp<int64>(Run.Player.DirectiveStreak-1,0,5);if(Run.Player.DirectiveStreak%3==0){const int64 Before=Run.Player.FieldFocus;Run.Player.FieldFocus=FMath::Min<int64>(3,Run.Player.FieldFocus+1);Reward.FocusGained+=Run.Player.FieldFocus-Before;}if(Run.Player.DirectiveStreak%5==0)AddItem(TEXT("witness_ink"));}else Run.Player.DirectiveStreak=0;
    Reward.StreakBonus=StreakBonus;Reward.MomentumBonus=BestRank*2+(BestRank>=4?4:0);
    Reward.Grains=(bVoid?8:3)+EnemyMaxHp/20+Reward.TacticalBonus+Reward.ObjectiveBonus+Reward.MomentumBonus+Reward.PreservationBonus+GradeBonus+StreakBonus;Run.Player.Grains+=Reward.Grains;
    if(Rng.Real(0,1)<=.30){TArray<FString> Table={TEXT("potion"),TEXT("potion"),TEXT("potion"),TEXT("antidote"),TEXT("antidote"),TEXT("firebomb")};if(bVoid){Table.Add(TEXT("firebomb"));Table.Add(TEXT("hi_potion"));Table.Add(TEXT("witness_ink"));}Reward.ItemId=Table[Rng.Integer(0,Table.Num()-1)];Reward.Item=ItemName(Reward.ItemId);AddItem(Reward.ItemId);}
    Log(bResolvedByWitness?TEXT("%s is released from the hostile echo."):TEXT("%s is defeated!"),{EnemyName});Log(TEXT("Recovered %d HP."),{N(Reward.Heal)});
    if(bWitnessComplete)Log(TEXT("[PRESERVATION] +%d Grains%s"),{N(Reward.PreservationBonus),PreservationFocus>0?TEXT(" / Field Focus +1"):TEXT("")});
    Log(TEXT("Gained %d Grains."),{N(Reward.Grains)});
    if(Reward.TacticalBonus>0)Log(TEXT("[CODEX BONUS] Tactical record +%d Grains."),{N(Reward.TacticalBonus)});Sounds.Add(TEXT("enemy_die"));Sounds.Add(TEXT("heal"));
}
