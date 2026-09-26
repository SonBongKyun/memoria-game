#include "Battle/MemoriaBattleModel.h"
#include "Run/MemoriaRunSubsystem.h"
#include "Domain/MemoriaPlayerMemoryDomain.h"
#include "Import/MemoriaStartingCatalogImport.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
using Obj=TSharedPtr<FJsonObject>;using Val=TSharedPtr<FJsonValue>;
Val ReadCore(const FString& Name){FString Text;Val V;FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/battle_core")/Name));FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),V);return V;}
Obj FindCore(const FString& File,const FString& Id){const auto Root=ReadCore(File);if(!Root)return nullptr;for(const auto& V:Root->AsArray())if(V->AsObject()->GetStringField(TEXT("id"))==Id)return V->AsObject();return nullptr;}
TArray<FString> StringsCore(const TArray<Val>& Values){TArray<FString> R;for(const auto& V:Values)R.Add(V->AsString());return R;}
FString CanonCore(const Obj& O){return MemoriaCatalogImport::Canonical(MakeShared<FJsonValueObject>(O));}
TArray<Val> StatusCore(const TArray<FMemoriaBattleStatus>& Values){TArray<Val> R;for(const auto& S:Values){auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("effect"),S.Effect);O->SetNumberField(TEXT("turns"),S.Turns);O->SetNumberField(TEXT("power"),S.Power);R.Add(MakeShared<FJsonValueObject>(O));}return R;}
}
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FBattleCoreSource,"Memoria.BattleCore.Source",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FBattleCoreSource::GetTests(TArray<FString>& Names,TArray<FString>& Commands)const
{
    const auto V=ReadCore(TEXT("contract_inputs.v1.json"));if(!V)return;
    for(const auto& Row:V->AsArray()){const FString Id=Row->AsObject()->GetStringField(TEXT("id"));Names.Add(Id);Commands.Add(Id);}
}
bool FBattleCoreSource::RunTest(const FString& Id)
{
    const Obj Input=FindCore(TEXT("contract_inputs.v1.json"),Id),Gold=FindCore(TEXT("contract_expected.v1.json"),Id);
    if(!TestTrue(TEXT("Source fixture loaded"),Input.IsValid()&&Gold.IsValid()))return false;
    const auto States=Gold->GetArrayField(TEXT("states"));const auto Tape=States.Last()->AsObject()->GetArrayField(TEXT("rng"));int32 Cursor=0;
    const auto Draw=[&](const TCHAR* Kind,double Lo,double Hi){if(!TestTrue(TEXT("Recorded random call exists"),Tape.IsValidIndex(Cursor)))return Lo;const auto E=Tape[Cursor++]->AsObject();TestEqual(TEXT("RNG kind"),E->GetStringField(TEXT("kind")),FString(Kind));TestEqual(TEXT("RNG minimum"),E->GetNumberField(TEXT("min")),Lo);TestEqual(TEXT("RNG maximum"),E->GetNumberField(TEXT("max")),Hi);return E->GetNumberField(TEXT("value"));};
    FMemoriaEncounterRng Rng{[&](double A,double B){return Draw(TEXT("float"),A,B);},[&](int32 A,int32 B){return int32(Draw(TEXT("int"),A,B));}};
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());Game->Init();auto* Run=Game->GetSubsystem<UMemoriaRunSubsystem>();
    if(!TestTrue(TEXT("Real catalog"),Run->BeginStartingMemoryRun()==EMemoriaMemoryResult::Success)){Game->Shutdown();return false;}
    auto* Memory=Run->GetPlayerMemory();auto MS=Memory->GetSnapshot();
    for(auto& M:MS.Owned){double E=0;if(Input->GetObjectField(TEXT("erosion"))->TryGetNumberField(M.Id,E))M.Erosion=int64(E);}
    TestTrue(TEXT("Memory fixture restore"),Memory->Restore(Memory->GetDefinitions(),MS)==EMemoriaMemoryResult::Success);
    FMemoriaBattleModel M;M.Run=Run->GetRunSnapshot();M.Run.CurrentChapter=3;M.Run.CurrentLocale=Input->GetStringField(TEXT("locale"));
    M.Run.Player.Hp=Input->GetIntegerField(TEXT("hp"));M.Run.Player.MaxHp=Input->GetIntegerField(TEXT("max_hp"));M.Run.Player.Grains=17;M.Run.Player.Items.Reset();
    for(const auto& P:Input->GetObjectField(TEXT("items"))->Values)M.Run.Player.Items.Add({FString(P.Key.ToView()),int64(P.Value->AsNumber())});
    const auto Catalog=ReadCore(TEXT("source_catalog.v1.json"))->AsObject();
    FString EntryText;Val Entry;FFileHelper::LoadFileToString(EntryText,*(FPaths::ProjectDir()/TEXT("../../docs/unreal-migration/fixtures/battle_entry/source_catalog.v1.json")));FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(EntryText),Entry);
    const Obj Enemy=Entry->AsObject()->GetArrayField(TEXT("enemy_pool"))[Input->GetIntegerField(TEXT("enemy_index"))]->AsObject();
    M.EnemyName=Enemy->GetStringField(TEXT("name"));M.EnemyHp=M.EnemyMaxHp=Input->GetIntegerField(TEXT("enemy_hp"))>0?Input->GetIntegerField(TEXT("enemy_hp")):Enemy->GetIntegerField(TEXT("hp"));
    M.EnemyAttack=Input->GetIntegerField(TEXT("enemy_atk"))>0?Input->GetIntegerField(TEXT("enemy_atk")):Enemy->GetIntegerField(TEXT("atk"));
    M.Run.Player.DirectiveStreak=Input->GetIntegerField(TEXT("streak"));M.bVoid=Input->GetBoolField(TEXT("is_void"));M.ModifierEffect=Input->GetStringField(TEXT("modifier_effect"));M.ModifierValue=Input->GetIntegerField(TEXT("modifier_value"));
    for(const auto& F:Input->GetObjectField(TEXT("flags"))->Values)M.Run.StoryFlags.Add({FString(F.Key.ToView()),F.Value->AsBool()});
    M.Weakness=Input->GetStringField(TEXT("weakness"));if(M.Weakness.IsEmpty())M.Weakness=M.bVoid?TEXT("void"):TEXT("fire");M.Resistance=Input->GetStringField(TEXT("resistance"));
    M.Abilities=StringsCore(Input->GetArrayField(TEXT("abilities")));if(M.Abilities.IsEmpty())M.Abilities=StringsCore(Enemy->GetArrayField(TEXT("abilities")));
    M.ObjectiveId=Input->GetStringField(TEXT("objective"));
    for(const auto& O:Entry->AsObject()->GetArrayField(TEXT("objectives"))){const auto D=O->AsObject();if(D->GetStringField(TEXT("id"))!=M.ObjectiveId)continue;M.ObjectiveTitle=D->GetStringField(TEXT("title"));M.ObjectiveGrains=D->GetIntegerField(TEXT("reward_grains"));double Heal=0;D->TryGetNumberField(TEXT("reward_heal"),Heal);M.ObjectiveHeal=int64(Heal);D->TryGetStringField(TEXT("reward_item"),M.ObjectiveItem);}
    M.Momentum=Input->GetNumberField(TEXT("momentum"));M.Rank=M.BestRank=M.Momentum>=75?3:M.Momentum>=50?2:M.Momentum>=25?1:0;M.Break=Input->GetNumberField(TEXT("break_gauge"));
    for(const auto& V:Input->GetArrayField(TEXT("statuses"))){const auto S=V->AsObject();M.ApplyStatus(true,S->GetIntegerField(TEXT("effect")),S->GetIntegerField(TEXT("turns")),S->GetIntegerField(TEXT("power")));}
    for(const auto& V:Input->GetArrayField(TEXT("enemy_statuses"))){const auto S=V->AsObject();M.ApplyStatus(false,S->GetIntegerField(TEXT("effect")),S->GetIntegerField(TEXT("turns")),S->GetIntegerField(TEXT("power")));}
    const auto Actions=Input->GetArrayField(TEXT("actions"));
    for(int32 I=1;I<States.Num();++I)
    {
        const Obj A=Actions[I-1]->AsObject(),E=States[I]->AsObject();const FString Action=A->GetStringField(TEXT("action"));FString Arg;A->TryGetStringField(TEXT("id"),Arg);FMemoriaBattleBurn Burn;
        if(Action==TEXT("burn"))
        {
            const auto* D=Memory->GetDefinitions().FindByPredicate([&](const auto& V){return V.Id==Arg;});if(!D){AddError(TEXT("Missing burn definition"));break;}
            Burn.Id=Arg;Burn.Grade=int32(D->RawGrade);Burn.Power=D->BurnPower;Burn.Title=D->Title;
            TestTrue(TEXT("Source memory burn succeeds"),Memory->Burn(Arg,EMemoriaBurnMode::Normal,false,M.Run.MemoryContext())==EMemoriaMemoryResult::Success);
            if(D->RelatedNpc==TEXT("Elia")&&M.Run.MemoryContext().bStillHandsActive)M.Run.StoryFlags.Add({TEXT("oath_still_broken"),true});
            Burn.EffectivePower=Memory->GetEffectiveBurnPower(Arg);Burn.bEmberAffinity=Memory->HasPassive(TEXT("ember_affinity"));Burn.bVoidTouch=Memory->HasPassive(TEXT("void_touch"));Burn.bResidualWarmth=Memory->HasPassive(TEXT("residual_warmth"));Burn.bMemoryCascade=Memory->HasPassive(TEXT("memory_cascade"));
        }
        TestTrue(TEXT("Accept source action"),M.Act(Action,Arg,Action==TEXT("burn")?&Burn:nullptr,Rng));
        TArray<FMemoriaBattleHit> Hits=M.Hits;TArray<FString> Logs=M.Logs;
        for(int32 Safety=0;M.bPendingEnemy&&Safety<10;++Safety){M.EnemyTurn(Rng);Hits.Append(M.Hits);Logs.Append(M.Logs);}
        const auto Eq=[&](const TCHAR* Key,double Actual){TestEqual(FString::Printf(TEXT("Turn %d %s"),I,Key),Actual,E->GetNumberField(Key));};
        Eq(TEXT("hp"),M.Run.Player.Hp);Eq(TEXT("enemy_hp"),M.EnemyHp);Eq(TEXT("momentum"),M.Momentum);Eq(TEXT("rank"),M.Rank);Eq(TEXT("limit"),M.Limit);Eq(TEXT("break"),M.Break);Eq(TEXT("broken_turns"),M.BrokenTurns);Eq(TEXT("combo"),M.Combo);Eq(TEXT("chain"),M.Chain);Eq(TEXT("aftershock"),M.Aftershock);Eq(TEXT("turns"),M.Turns);Eq(TEXT("actions"),M.Actions);Eq(TEXT("grains"),M.Run.Player.Grains);Eq(TEXT("streak"),M.Run.Player.DirectiveStreak);Eq(TEXT("focus"),M.Run.Player.FieldFocus);Eq(TEXT("state"),M.bVictory?3:M.bDefeat?4:1);
        const auto Bool=[&](const TCHAR* K,bool V){TestEqual(FString::Printf(TEXT("Turn %d %s"),I,K),V,E->GetBoolField(K));};
        Bool(TEXT("defending"),M.bDefending);Bool(TEXT("shielded"),M.bShielded);Bool(TEXT("reflecting"),M.bReflecting);Bool(TEXT("charged"),M.bCharged);Bool(TEXT("last_stand"),M.bLastStand);Bool(TEXT("objective_complete"),M.bObjectiveComplete);Bool(TEXT("objective_failed"),M.bObjectiveFailed);
        auto Projection=MakeShared<FJsonObject>(),Expected=MakeShared<FJsonObject>();Projection->SetArrayField(TEXT("player_statuses"),StatusCore(M.PlayerStatuses));Projection->SetArrayField(TEXT("enemy_statuses"),StatusCore(M.EnemyStatuses));Expected->SetArrayField(TEXT("player_statuses"),E->GetArrayField(TEXT("player_statuses")));Expected->SetArrayField(TEXT("enemy_statuses"),E->GetArrayField(TEXT("enemy_statuses")));
        auto Items=MakeShared<FJsonObject>();for(const auto& Item:M.Run.Player.Items)Items->SetNumberField(Item.Id,Item.Count);Projection->SetObjectField(TEXT("items"),Items);Expected->SetObjectField(TEXT("items"),E->GetObjectField(TEXT("items")));
        TArray<Val> HitValues;for(const auto& H:Hits){auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("target"),H.Target);V->SetStringField(TEXT("skill"),H.Skill);V->SetNumberField(TEXT("amount"),H.Amount);HitValues.Add(MakeShared<FJsonValueObject>(V));}
        Projection->SetArrayField(TEXT("events"),HitValues);Expected->SetArrayField(TEXT("events"),E->GetArrayField(TEXT("events")));
        TestEqual(FString::Printf(TEXT("Turn %d status/inventory/damage events"),I),CanonCore(Projection),CanonCore(Expected));
        TestEqual(TEXT("Burn history matches source"),FString::Join(Memory->GetSnapshot().BurnedHistory,TEXT(",")),FString::Join(StringsCore(E->GetArrayField(TEXT("burned"))),TEXT(",")));
        TestEqual(TEXT("Exact RNG calls at action boundary"),Cursor,E->GetArrayField(TEXT("rng")).Num());
        if(E->GetObjectField(TEXT("flags"))->HasField(TEXT("oath_still_broken")))TestTrue(TEXT("Still Hands breaks on voluntary Elia burn"),M.Run.GetFlag(TEXT("oath_still_broken")));
        const auto Reward=E->GetObjectField(TEXT("reward"));if(M.bVictory){TestEqual(TEXT("Source reward grains"),M.Reward.Grains,int64(Reward->GetNumberField(TEXT("grains"))));TestEqual(TEXT("Source reward heal"),M.Reward.Heal,int64(Reward->GetNumberField(TEXT("heal"))));TestEqual(TEXT("Source drop"),M.Reward.Item,Reward->GetStringField(TEXT("item")));}
        // Numeric hit events above are exhaustive. Check the localized player-action
        // message key whenever that source branch emits it, without treating omitted
        // companion/profile presentation as part of the slice's numeric contract.
        for(const auto& L:Logs)if(L.Contains(TEXT("strikes!"))||L.Contains(TEXT("아렐의 일격!")))TestTrue(TEXT("Localized source action log"),StringsCore(E->GetArrayField(TEXT("logs"))).Contains(L));
    }
    TestEqual(TEXT("All recorded RNG consumed"),Cursor,Tape.Num());Game->Shutdown();return true;
}
#endif
